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

// Конструктор: создает сцену, настраивает стратегии отрисовки и UI.
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

// Деструктор: удаляет сцену.
CadWindow::~CadWindow()
{
    delete m_scene;
}

// Настраивает внешний вид: создает панели, сплиттеры и задает начальные размеры.
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

// Связывает сигналы от панелей управления с методами окна и вьюпорта.
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
    connect(m_controlPanel, &Control::objectSelected, this, &CadWindow::onObjectSelected);
    connect(m_propertiesPanel, &Properties::objectModified, this, &CadWindow::onObjectModified);

    connect(this, &CadWindow::sceneChanged, m_controlPanel, &Control::updateObjectList);
}

// Регистрирует классы отрисовки для поддерживаемых примитивов.
void CadWindow::setupDrawingStrategies()
{
    m_drawingStrategies[PrimitiveType::Segment] = std::make_unique<SegmentDraw>();
}

// Передает новый шаг сетки во вьюпорт.
void CadWindow::onGridStepChanged(int step)
{
    m_viewportPanel->setGridStep(step);
}

// Обновляет глобальную настройку единиц измерения углов.
void CadWindow::onAngleUnitChanged(AngleUnit unit)
{
    Point::setAngleUnit(unit);
    m_propertiesPanel->updateAngleLabels();
}

// Активирует режим создания выбранного примитива.
void CadWindow::onPrimitiveTypeSelected(PrimitiveType type)
{
    m_activePrimitiveType = type;

    if (m_selectedObject == nullptr) {
        m_propertiesPanel->showCreationPropertiesFor(type);
    }
}

// Создает отрезок, добавляет его в сцену и обновляет вид.
void CadWindow::createSegment(const Point& start, const Point& end, const QColor& color)
{
    auto newSegment = std::make_unique<Segment>(start, end);
    newSegment->setColor(color);
    m_scene->addPrimitive(std::move(newSegment));

    m_viewportPanel->update();
    emit sceneChanged(m_scene);
}

// Удаляет выбранный объект из сцены и сбрасывает выделение.
void CadWindow::onDeleteRequested()
{
    if (m_selectedObject) {
        m_scene->removePrimitive(m_selectedObject);
        m_selectedObject = nullptr;

        m_viewportPanel->setSelectedObject(nullptr);
        m_propertiesPanel->showCreationPropertiesFor(m_activePrimitiveType);

        m_viewportPanel->update();
        emit sceneChanged(m_scene);
    }
}

// Обрабатывает выбор объекта: обновляет вьюпорт и переключает панель свойств.
void CadWindow::onObjectSelected(Object* selectedObject)
{
    m_selectedObject = selectedObject;

    m_viewportPanel->setSelectedObject(m_selectedObject);

    if (m_selectedObject) {
        m_propertiesPanel->showEditingPropertiesFor(m_selectedObject);
    } else {
        m_propertiesPanel->showCreationPropertiesFor(m_activePrimitiveType);
    }
}

// Вызывается при изменении свойств объекта для перерисовки.
void CadWindow::onObjectModified(Object* obj)
{
    m_viewportPanel->update();
    emit sceneChanged(m_scene);
}
