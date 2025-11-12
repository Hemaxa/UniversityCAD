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
CadWindow::CadWindow(QWidget *parent)
    : QMainWindow(parent),
    m_activePrimitiveType(PrimitiveType::Generic) // Инициализация
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

// Устанавливает все сигнально-слотовые соединения в приложении.
void CadWindow::createConnections()
{
    // Соединения для настроек сцены и вида.
    connect(m_controlPanel, &Control::gridStepChanged, this, &CadWindow::onGridStepChanged);
    connect(m_controlPanel, &Control::angleUnitChanged, this, &CadWindow::onAngleUnitChanged);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_propertiesPanel, &Properties::setCoordinateSystem);
    connect(m_controlPanel, &Control::coordinateSystemChanged, m_viewportPanel, &Viewport::setCoordinateSystem);

    // Соединение для создания объектов.
    connect(m_controlPanel, &Control::primitiveTypeSelected, this, &CadWindow::onPrimitiveTypeSelected);
    connect(m_propertiesPanel, &Properties::segmentCreateRequested, this, &CadWindow::createSegment);

    // Соединения для выбора, удаления и ИЗМЕНЕНИЯ объектов.
    connect(m_controlPanel, &Control::deleteRequested, this, &CadWindow::onDeleteRequested);
    connect(m_controlPanel, &Control::objectSelected, this, &CadWindow::onObjectSelected);
    connect(m_propertiesPanel, &Properties::objectModified, this, &CadWindow::onObjectModified);

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

// Слот для выбора инструмента создания примитива.
void CadWindow::onPrimitiveTypeSelected(PrimitiveType type)
{
    // Сохраняем, какой инструмент "Создания" сейчас активен.
    m_activePrimitiveType = type;

    // Показываем панель "Создания", ТОЛЬКО если сейчас не выбран объект.
    // Если объект выбран, приоритет у панели "Редактирования".
    if (m_selectedObject == nullptr) {
        m_propertiesPanel->showCreationPropertiesFor(type);
    }
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

        // Синхронизируем состояние всех панелей
        m_viewportPanel->setSelectedObject(nullptr); // Снимаем подсветку
        // Возвращаем панель свойств в режим "Создание"
        m_propertiesPanel->showCreationPropertiesFor(m_activePrimitiveType);

        m_viewportPanel->update();
        emit sceneChanged(m_scene); // Обновляем список и вьюпорт.
    }
}

// Слот, сохраняющий указатель на выбранный в списке объект.
void CadWindow::onObjectSelected(Object* selectedObject)
{
    m_selectedObject = selectedObject;

    // 1. Сообщаем Вьюпорту, какой объект подсветить.
    m_viewportPanel->setSelectedObject(m_selectedObject);

    // 2. Сообщаем Панели свойств, какой объект редактировать.
    if (m_selectedObject) {
        // Если выбрали объект - показываем панель редактирования.
        m_propertiesPanel->showEditingPropertiesFor(m_selectedObject);
    } else {
        // Если выбор сброшен - возвращаем панель в режим "Создание".
        m_propertiesPanel->showCreationPropertiesFor(m_activePrimitiveType);
    }
}

// Слот, реагирующий на изменение объекта в Properties.
void CadWindow::onObjectModified(Object* obj)
{
    // Просто запрашиваем перерисовку вьюпорта.
    m_viewportPanel->update();

    // Мы также можем обновить список (на случай, если бы имя изменилось).
    // Используем sceneChanged, т.к. он уже подключен к updateObjectList.
    emit sceneChanged(m_scene);
}
