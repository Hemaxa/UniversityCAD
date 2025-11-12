#include "Control.h"
#include "Scene.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QListWidget>

// Конструктор панели управления.
Control::Control(QWidget *parent) : QWidget(parent)
{
    this->setObjectName("ControlPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(15);

    // --- 1. Группа "Параметры сцены" ---
    auto* sceneGroup = new QGroupBox("Параметры сцены");
    auto* sceneLayout = new QFormLayout(sceneGroup);
    sceneLayout->setLabelAlignment(Qt::AlignLeft); // Выравнивание по левому краю.

    m_gridStepSpinBox = new QSpinBox();
    m_gridStepSpinBox->setRange(10, 200);
    m_gridStepSpinBox->setValue(50);
    sceneLayout->addRow("Шаг сетки:", m_gridStepSpinBox);

    m_angleUnitComboBox = new QComboBox();
    m_angleUnitComboBox->addItem("Градусы", static_cast<int>(AngleUnit::Degrees));
    m_angleUnitComboBox->addItem("Радианы", static_cast<int>(AngleUnit::Radians));
    sceneLayout->addRow("Единицы углов:", m_angleUnitComboBox);

    auto* coordLayout = new QHBoxLayout();
    auto* coordGroup = new QButtonGroup(this);
    coordGroup->setExclusive(true);
    m_cartesianBtn = new QToolButton();
    m_cartesianBtn->setText("Декартова");
    m_cartesianBtn->setCheckable(true);
    m_cartesianBtn->setChecked(true);
    m_polarBtn = new QToolButton();
    m_polarBtn->setText("Полярная");
    m_polarBtn->setCheckable(true);
    coordGroup->addButton(m_cartesianBtn);
    coordGroup->addButton(m_polarBtn);
    coordLayout->addWidget(m_cartesianBtn);
    coordLayout->addWidget(m_polarBtn);
    sceneLayout->addRow("Координаты:", coordLayout);

    // --- 2. Группа "Объекты сцены" ---
    auto* objectsGroup = new QGroupBox("Объекты сцены");
    auto* objectsLayout = new QVBoxLayout(objectsGroup);
    m_objectListWidget = new QListWidget();
    m_deleteBtn = new QPushButton("Удалить выбранный");
    m_deleteBtn->setObjectName("deleteButton");
    objectsLayout->addWidget(m_objectListWidget);
    objectsLayout->addWidget(m_deleteBtn);

    // --- 3. Группа "Создание примитивов" ---
    auto* primitivesGroup = new QGroupBox("Создание объектов");
    auto* primitivesLayout = new QHBoxLayout(primitivesGroup);
    primitivesLayout->setAlignment(Qt::AlignLeft);

    // Группа для кнопок-инструментов (чтобы только одна была активна)
    m_primitiveToolsGroup = new QButtonGroup(this);
    m_primitiveToolsGroup->setExclusive(true);

    // Кнопка "Отрезок"
    m_createSegmentBtn = new QToolButton();
    m_createSegmentBtn->setCheckable(true);
    m_createSegmentBtn->setIcon(QIcon(":/icons/segment.svg"));
    m_createSegmentBtn->setIconSize(QSize(20, 20));
    m_createSegmentBtn->setProperty("isIconButton", true);

    m_primitiveToolsGroup->addButton(m_createSegmentBtn);
    primitivesLayout->addWidget(m_createSegmentBtn);

    // Сюда можно добавлять другие QToolButton для новых примитивов

    // --- Сборка панели ---
    mainLayout->addWidget(sceneGroup);
    mainLayout->addWidget(objectsGroup);
    mainLayout->addWidget(primitivesGroup);

    // --- Соединения ---
    connect(m_gridStepSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Control::gridStepChanged);
    connect(m_angleUnitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        emit angleUnitChanged(static_cast<AngleUnit>(m_angleUnitComboBox->itemData(index).toInt()));
    });
    connect(m_cartesianBtn, &QToolButton::clicked, this, &Control::onCartesianClicked);
    connect(m_polarBtn, &QToolButton::clicked, this, &Control::onPolarClicked);
    connect(m_objectListWidget, &QListWidget::itemSelectionChanged, this, &Control::onSelectionChanged);
    connect(m_deleteBtn, &QPushButton::clicked, this, &Control::deleteRequested);

    // Соединение для кнопки "Отрезок"
    connect(m_createSegmentBtn, &QToolButton::toggled, this, [this](bool checked){
        onPrimitiveToolToggled(checked, PrimitiveType::Segment);
    });
}

// Обновляет содержимое списка объектов на основе данных из сцены.
void Control::updateObjectList(const Scene* scene)
{
    m_objectListWidget->blockSignals(true);

    // Сохраняем ID выбранного объекта, чтобы восстановить выбор
    Object* currentSelectedObject = nullptr;
    if (m_objectListWidget->currentItem()) {
        currentSelectedObject = static_cast<Object*>(m_objectListWidget->currentItem()->data(Qt::UserRole).value<void*>());
    }

    m_objectListWidget->clear();
    if (!scene) {
        m_objectListWidget->blockSignals(false);
        return;
    }

    QListWidgetItem* itemToSelect = nullptr; // Элемент для восстановления выбора

    for (const auto& obj : scene->getPrimitives()) {

        QString itemName;

        // Формируем имя в зависимости от типа объекта
        if (obj->getType() == PrimitiveType::Segment) {
            itemName = QString("Отрезок %1").arg(obj->getID());
        }
        // else if (obj->getType() == PrimitiveType::Point) {
        //     itemName = QString("Точка %1").arg(obj->getID());
        // }
        else {
            itemName = QString("Объект %1").arg(obj->getID());
        }

        QListWidgetItem* item = new QListWidgetItem(itemName, m_objectListWidget);
        item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(obj.get())));

        // Проверяем, нужно ли восстановить выбор этого элемента
        if (obj.get() == currentSelectedObject) {
            itemToSelect = item;
        }
    }

    // Восстанавливаем выбор
    if (itemToSelect) {
        itemToSelect->setSelected(true);
    }

    m_objectListWidget->blockSignals(false);
}

// Срабатывает при выборе элемента в списке и испускает сигнал objectSelected.
void Control::onSelectionChanged()
{
    auto selectedItems = m_objectListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        Object* selectedObject = static_cast<Object*>(selectedItems.first()->data(Qt::UserRole).value<void*>());
        emit objectSelected(selectedObject);
    } else {
        emit objectSelected(nullptr);
    }
}

// Испускает сигнал о смене системы координат на декартову.
void Control::onCartesianClicked() { emit coordinateSystemChanged(CoordinateSystemType::Cartesian); }

// Испускает сигнал о смене системы координат на полярную.
void Control::onPolarClicked() { emit coordinateSystemChanged(CoordinateSystemType::Polar); }

// Слот для обработки переключения инструментов
void Control::onPrimitiveToolToggled(bool checked, PrimitiveType type)
{
    if (checked) {
        // Если кнопка включена, сообщаем, какой инструмент выбран
        emit primitiveTypeSelected(type);
    } else {
        // Если кнопка выключена (например, кликнули по ней же еще раз)
        // Проверяем, не является ли *другая* кнопка активной.
        if (!m_primitiveToolsGroup->checkedButton()) {
            emit primitiveTypeSelected(PrimitiveType::Generic); // Никакой инструмент не выбран
        }
    }
}
