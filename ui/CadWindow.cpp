#include "CadWindow.h"
#include "Viewport.h"
#include "Control.h"
#include "Properties.h"
#include "Scene.h"
#include "Point.h"
#include "Segment.h"
#include "Draw.h"
#include "SegmentDraw.h"

#include <QSplitter>
#include <QScreen>
#include <QGuiApplication>
#include <QShortcut>
#include <QKeySequence>

CadWindow::CadWindow(QWidget *parent)
    : QMainWindow(parent),
    m_activePrimitiveType(PrimitiveType::Generic)
{
    m_scene = new Scene();
    setupDrawingStrategies();
    setupUi();
    createConnections();

    m_viewportPanel->setScene(m_scene);
    m_viewportPanel->setDrawingStrategies(&m_drawingStrategies);

    emit sceneChanged(m_scene);
}

CadWindow::~CadWindow()
{
    delete m_scene;
}

void CadWindow::setupUi()
{
    m_viewportPanel = new Viewport(this);
    m_controlPanel = new Control(this);
    m_propertiesPanel = new Properties(this);
    m_rightColumnSplitter = new QSplitter(Qt::Vertical);
    m_rightColumnSplitter->addWidget(m_controlPanel);
    m_rightColumnSplitter->addWidget(m_propertiesPanel);
    m_rightColumnSplitter->setHandleWidth(1);
    m_rightColumnSplitter->setSizes({600, 250});
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(m_viewportPanel);
    m_mainSplitter->addWidget(m_rightColumnSplitter);
    m_mainSplitter->setHandleWidth(1);
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int leftWidth = static_cast<int>(screenGeometry.width() * 0.75);
    int rightWidth = static_cast<int>(screenGeometry.width() * 0.25);
    m_mainSplitter->setSizes({leftWidth, rightWidth});
    setCentralWidget(m_mainSplitter);
    setWindowTitle("UniversityCAD");
}

void CadWindow::createConnections()
{
    connect(m_controlPanel, &Control::gridStepChanged, this, &CadWindow::onGridStepChanged);
    connect(m_controlPanel, &Control::zoomStepChanged, m_viewportPanel, &Viewport::setZoomStep);
    connect(m_controlPanel, &Control::angleUnitChanged, this, &CadWindow::onAngleUnitChanged);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_propertiesPanel, &Properties::setCoordinateSystem);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_viewportPanel, &Viewport::setCoordinateSystem);

    connect(m_controlPanel, &Control::primitiveTypeSelected, this, &CadWindow::onPrimitiveTypeSelected);

    connect(m_propertiesPanel, &Properties::segmentCreateRequested, this, &CadWindow::createSegment);
    connect(m_controlPanel, &Control::deleteRequested, this, &CadWindow::onDeleteRequested);

    // Выбор во Viewport -> Обновляем окно и список
    connect(m_viewportPanel, &Viewport::selectionChanged, this, &CadWindow::onObjectsSelected);

    // ИСПРАВЛЕНИЕ 1: Выбор в Списке -> Обновляем окно и вьюпорт
    connect(m_controlPanel, &Control::objectsSelected, this, &CadWindow::onObjectsSelectedFromList);

    connect(m_propertiesPanel, &Properties::objectsModified, this, &CadWindow::onObjectsModified);

    // ИСПРАВЛЕНИЕ 3: Глобальный Escape
    auto* escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escapeShortcut, &QShortcut::activated, this, &CadWindow::onEscapePressed);

    connect(this, &CadWindow::sceneChanged, m_controlPanel, &Control::updateObjectList);
}

void CadWindow::setupDrawingStrategies()
{
    m_drawingStrategies[PrimitiveType::Segment] = std::make_unique<SegmentDraw>();
}

void CadWindow::onGridStepChanged(int step) { m_viewportPanel->setGridStep(step); }
void CadWindow::onAngleUnitChanged(AngleUnit unit) { Point::setAngleUnit(unit); m_propertiesPanel->updateAngleLabels(); }

void CadWindow::onPrimitiveTypeSelected(PrimitiveType type)
{
    m_activePrimitiveType = type;
    if (m_selectedObjects.empty()) {
        m_propertiesPanel->showCreationPropertiesFor(type);
    }
}

void CadWindow::createSegment(const Point& start, const Point& end, const QColor& color, const LineStyle& style)
{
    auto newSegment = std::make_unique<Segment>(start, end);
    newSegment->setColor(color);
    newSegment->setLineStyle(style);
    m_scene->addPrimitive(std::move(newSegment));
    m_viewportPanel->update();
    emit sceneChanged(m_scene);
}

void CadWindow::onDeleteRequested()
{
    // Теперь m_selectedObjects всегда актуален благодаря синхронизации
    if (!m_selectedObjects.empty()) {
        for (auto* obj : m_selectedObjects) {
            m_scene->removePrimitive(obj);
        }

        onEscapePressed(); // Сброс выделения и возврат к дефолтному состоянию

        m_viewportPanel->update();
        emit sceneChanged(m_scene);
    }
}

// Выбор произошел во Вьюпорте
void CadWindow::onObjectsSelected(const std::vector<Object*>& selectedObjects)
{
    m_selectedObjects = selectedObjects;

    // Синхронизируем список (без зацикливания, внутри есть защита)
    m_controlPanel->setSelectedObjects(selectedObjects);

    if (!m_selectedObjects.empty()) {
        m_propertiesPanel->showEditingPropertiesFor(m_selectedObjects);
    } else {
        m_propertiesPanel->showCreationPropertiesFor(m_activePrimitiveType);
    }
}

// Выбор произошел в Списке
void CadWindow::onObjectsSelectedFromList(const std::vector<Object*>& selectedObjects)
{
    m_selectedObjects = selectedObjects;

    // Синхронизируем вьюпорт (без зацикливания)
    m_viewportPanel->setSelectedObjects(selectedObjects);

    if (!m_selectedObjects.empty()) {
        m_propertiesPanel->showEditingPropertiesFor(m_selectedObjects);
    } else {
        m_propertiesPanel->showCreationPropertiesFor(m_activePrimitiveType);
    }
}

// Нажат Escape
void CadWindow::onEscapePressed()
{
    // 1. Сбрасываем выделение данных
    m_selectedObjects.clear();

    // 2. Сбрасываем UI выделения
    m_viewportPanel->setSelectedObjects({});
    m_controlPanel->clearSelection();

    // 3. Сбрасываем инструменты (кнопки)
    m_controlPanel->resetTools(); // Вернет Generic тип
    m_activePrimitiveType = PrimitiveType::Generic;

    // 4. Показываем панель создания (пустую/дефолтную)
    m_propertiesPanel->showCreationPropertiesFor(m_activePrimitiveType);
}

void CadWindow::onObjectsModified(const std::vector<Object*>& objs)
{
    Q_UNUSED(objs);
    m_viewportPanel->update();
}
