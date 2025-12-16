#include "CadWindow.h"
#include "Viewport.h"
#include "Control.h"
#include "Properties.h"
#include "Scene.h"
#include "PrimitiveStrategies.h"
#include "Segment.h"

#include <QSplitter>
#include <QGuiApplication>
#include <QShortcut>

CadWindow::CadWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_scene = new Scene();
    setupDrawingStrategies();
    setupUi();
    createConnections();

    m_viewportPanel->setScene(m_scene);
    m_viewportPanel->setDrawingStrategies(&m_drawingStrategies);
}

CadWindow::~CadWindow() { 
    // Очищаем выделенные объекты перед удалением сцены
    m_selectedObjects.clear();
    delete m_scene; 
}

void CadWindow::setupUi() {
    m_viewportPanel = new Viewport(this);
    m_controlPanel = new Control(this);
    m_propertiesPanel = new Properties(this);

    m_rightColumnSplitter = new QSplitter(Qt::Vertical);
    m_rightColumnSplitter->addWidget(m_controlPanel);
    m_rightColumnSplitter->addWidget(m_propertiesPanel);
    m_rightColumnSplitter->setSizes({500, 400});

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(m_viewportPanel);
    m_mainSplitter->addWidget(m_rightColumnSplitter);
    m_mainSplitter->setStretchFactor(0, 1);

    setCentralWidget(m_mainSplitter);
    resize(1200, 800);
}

void CadWindow::createConnections() {
    connect(m_controlPanel, &Control::gridStepChanged, m_viewportPanel, &Viewport::setGridStep);
    connect(m_controlPanel, &Control::zoomStepChanged, m_viewportPanel, &Viewport::setZoomStep);
    connect(m_controlPanel, &Control::angleUnitChanged, this, &CadWindow::onAngleUnitChanged);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_viewportPanel, &Viewport::setCoordinateSystem);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_propertiesPanel, &Properties::setCoordinateSystem);
    connect(m_controlPanel, &Control::primitiveTypeSelected, this, &CadWindow::onPrimitiveTypeSelected);
    connect(m_propertiesPanel, &Properties::objectCreateRequested, this, &CadWindow::onObjectCreateRequested);
    connect(m_propertiesPanel, &Properties::objectsModified, this, &CadWindow::onObjectsModified);
    connect(m_viewportPanel, &Viewport::selectionChanged, this, &CadWindow::onObjectsSelected);
    connect(m_controlPanel, &Control::objectsSelected, this, &CadWindow::onObjectsSelectedFromList);
    connect(this, &CadWindow::sceneChanged, m_controlPanel, &Control::updateObjectList);
    connect(m_controlPanel, &Control::deleteRequested, this, &CadWindow::onDeleteRequested);
    connect(m_viewportPanel, &Viewport::objectCreated, this, &CadWindow::onObjectCreateRequested);
    connect(m_controlPanel, &Control::primitiveTypeSelected, m_viewportPanel, &Viewport::setActiveTool);

    connect(m_controlPanel, &Control::gridSnapToggled, m_viewportPanel, &Viewport::setGridSnap);
    connect(m_controlPanel, &Control::objectSnapToggled, m_viewportPanel, &Viewport::setObjectSnap);

    new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(onEscapePressed()));
}

void CadWindow::setupDrawingStrategies() {
    m_drawingStrategies[PrimitiveType::Segment] = std::make_unique<SegmentDraw>();
    m_drawingStrategies[PrimitiveType::Circle] = std::make_unique<CircleDraw>();
    m_drawingStrategies[PrimitiveType::Arc] = std::make_unique<ArcDraw>();
    m_drawingStrategies[PrimitiveType::Rectangle] = std::make_unique<RectangleDraw>();
    m_drawingStrategies[PrimitiveType::Ellipse] = std::make_unique<EllipseDraw>();
    m_drawingStrategies[PrimitiveType::Polygon] = std::make_unique<PolygonDraw>();
    m_drawingStrategies[PrimitiveType::Spline] = std::make_unique<SplineDraw>();
}

void CadWindow::onPrimitiveTypeSelected(PrimitiveType type, int methodIndex) {
    m_activePrimitiveType = type;
    if(m_selectedObjects.empty()) {
        m_propertiesPanel->showCreationPropertiesFor(type, methodIndex);
    }
}

void CadWindow::onObjectCreateRequested(Object* obj) {
    if (!obj) {
        return;  // Защита от nullptr
    }
    m_propertiesPanel->applyCurrentStyleTo(obj);
    m_scene->addPrimitive(std::unique_ptr<Object>(obj));
    m_viewportPanel->update();
    emit sceneChanged(m_scene);
}

void CadWindow::onDeleteRequested() {
    // Сохраняем копию указателей перед удалением, так как removePrimitive
    // может инвалидировать указатели в m_selectedObjects
    std::vector<Object*> objectsToDelete = m_selectedObjects;
    
    // Очищаем выделение перед удалением объектов
    m_selectedObjects.clear();
    m_viewportPanel->setSelectedObjects({});
    
    // Теперь безопасно удаляем объекты
    for(auto* obj : objectsToDelete) {
        if (obj) {
            m_scene->removePrimitive(obj);
        }
    }
    
    // Обновляем UI
    m_controlPanel->clearSelection();
    m_viewportPanel->update();
    emit sceneChanged(m_scene);
}

void CadWindow::onObjectsSelected(const std::vector<Object*>& selectedObjects) {
    m_selectedObjects = selectedObjects;
    m_controlPanel->setSelectedObjects(selectedObjects);
    m_propertiesPanel->showEditingPropertiesFor(selectedObjects);
}

void CadWindow::onObjectsSelectedFromList(const std::vector<Object*>& selectedObjects) {
    m_selectedObjects = selectedObjects;
    m_viewportPanel->setSelectedObjects(selectedObjects);
    m_propertiesPanel->showEditingPropertiesFor(selectedObjects);
}

void CadWindow::onObjectsModified(const std::vector<Object*>&) {
    m_viewportPanel->update();
}

void CadWindow::onEscapePressed() {
    m_selectedObjects.clear();
    m_viewportPanel->setSelectedObjects({});
    m_controlPanel->clearSelection();
    m_controlPanel->resetTools();
    m_activePrimitiveType = PrimitiveType::Generic;
    m_propertiesPanel->showCreationPropertiesFor(PrimitiveType::Generic, 0);
    m_viewportPanel->resetTool();
}

void CadWindow::onGridStepChanged(int step) { m_viewportPanel->setGridStep(step); }
void CadWindow::onAngleUnitChanged(AngleUnit unit) { Point::setAngleUnit(unit); }
