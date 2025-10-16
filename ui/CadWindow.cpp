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

// Конструктор главного окна.
CadWindow::CadWindow(QWidget *parent) : QMainWindow(parent)
{
    m_scene = new Scene();
    setupDrawingStrategies();
    setupUi();
    createConnections();

    m_viewportPanel->setScene(m_scene);
    m_viewportPanel->setDrawingStrategies(&m_drawingStrategies);

    // Первоначальное обновление списка объектов при запуске.
    emit sceneChanged(m_scene);
}

// Деструктор.
CadWindow::~CadWindow()
{
    delete m_scene;
}

// Создает и компонует основной пользовательский интерфейс.
void CadWindow::setupUi()
{
    // Создание трех основных панелей.
    m_viewportPanel = new Viewport(this);
    m_controlPanel = new Control(this);
    m_propertiesPanel = new Properties(this);

    // Компоновка правой колонки с панелями управления и свойств.
    m_rightColumnSplitter = new QSplitter(Qt::Vertical);
    m_rightColumnSplitter->addWidget(m_controlPanel);
    m_rightColumnSplitter->addWidget(m_propertiesPanel);
    m_rightColumnSplitter->setHandleWidth(1);
    m_rightColumnSplitter->setSizes({600, 250});

    // Главный разделитель между вьюпортом и правой колонкой.
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->addWidget(m_viewportPanel);
    m_mainSplitter->addWidget(m_rightColumnSplitter);
    m_mainSplitter->setHandleWidth(1);

    // Установка начальных пропорций главного разделителя.
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    int leftWidth = static_cast<int>(screenGeometry.width() * 0.75);
    int rightWidth = static_cast<int>(screenGeometry.width() * 0.25);
    m_mainSplitter->setSizes({leftWidth, rightWidth});

    // Установка главного виджета и заголовка окна.
    setCentralWidget(m_mainSplitter);
    setWindowTitle("UniversityCAD");
}

// Устанавливает все сигнально-слотовые соединения в приложении.
void CadWindow::createConnections()
{
    // Соединения для настроек сцены и вида.
    connect(m_controlPanel, &Control::gridStepChanged, this, &CadWindow::onGridStepChanged);
    connect(m_controlPanel, &Control::angleUnitChanged, this, &CadWindow::onAngleUnitChanged);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_propertiesPanel, &Properties::setCoordinateSystem);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_viewportPanel, &Viewport::setCoordinateSystem);

    // Соединение для создания объектов.
    connect(m_propertiesPanel, &Properties::segmentCreateRequested, this, &CadWindow::createSegment);

    // Соединения для удаления и выбора объектов.
    connect(m_controlPanel, &Control::deleteRequested, this, &CadWindow::onDeleteRequested);
    connect(m_controlPanel, &Control::objectSelected, this, &CadWindow::onObjectSelected);

    // Соединение для обновления списка объектов при изменении сцены.
    connect(this, &CadWindow::sceneChanged, m_controlPanel, &Control::updateObjectList);
}

// Инициализирует стратегии отрисовки для каждого типа примитива.
void CadWindow::setupDrawingStrategies()
{
    m_drawingStrategies[PrimitiveType::Segment] = std::make_unique<SegmentDraw>();
}

// Слот для обработки изменения шага сетки.
void CadWindow::onGridStepChanged(int step)
{
    m_viewportPanel->setGridStep(step);
}

// Слот для обработки изменения единиц измерения углов.
void CadWindow::onAngleUnitChanged(AngleUnit unit)
{
    Point::setAngleUnit(unit);
    m_propertiesPanel->updateAngleLabels();
}

// Слот для создания нового отрезка.
void CadWindow::createSegment(const Point& start, const Point& end, const QColor& color)
{
    auto newSegment = std::make_unique<Segment>(start, end);
    newSegment->setColor(color);
    m_scene->addPrimitive(std::move(newSegment));

    m_viewportPanel->update();
    emit sceneChanged(m_scene); // Испускаем сигнал для обновления списка объектов.
}

// Слот, вызываемый при нажатии кнопки "Удалить".
void CadWindow::onDeleteRequested()
{
    if (m_selectedObject) {
        m_scene->removePrimitive(m_selectedObject);
        m_selectedObject = nullptr; // Сбрасываем указатель.
        m_viewportPanel->update();
        emit sceneChanged(m_scene); // Обновляем список и вьюпорт.
    }
}

// Слот, сохраняющий указатель на выбранный в списке объект.
void CadWindow::onObjectSelected(Object* selectedObject)
{
    m_selectedObject = selectedObject;
}
