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

Control::Control(QWidget *parent) : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(15);

    // --- 1. Группа "Параметры сцены" ---
    auto* sceneGroup = new QGroupBox("Параметры сцены");
    auto* sceneLayout = new QFormLayout(sceneGroup);
    m_gridStepSpinBox = new QSpinBox();
    m_gridStepSpinBox->setRange(10, 200);
    m_gridStepSpinBox->setValue(50);
    m_gridStepSpinBox->setFixedWidth(150); // Установка фиксированной ширины
    sceneLayout->addRow("Шаг сетки:", m_gridStepSpinBox);

    m_angleUnitComboBox = new QComboBox();
    m_angleUnitComboBox->addItem("Градусы", static_cast<int>(AngleUnit::Degrees));
    m_angleUnitComboBox->addItem("Радианы", static_cast<int>(AngleUnit::Radians));
    m_angleUnitComboBox->setFixedWidth(150); // Установка фиксированной ширины
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

    // --- 2. Группа "Объекты сцены" (НОВАЯ) ---
    auto* objectsGroup = new QGroupBox("Объекты сцены");
    auto* objectsLayout = new QVBoxLayout(objectsGroup);
    m_objectListWidget = new QListWidget();
    m_deleteBtn = new QPushButton("Удалить выбранный");
    m_deleteBtn->setObjectName("deleteButton"); // Для особого стиля в QSS
    objectsLayout->addWidget(m_objectListWidget);
    objectsLayout->addWidget(m_deleteBtn);

    // --- 3. Группа "Создание примитивов" ---
    auto* primitivesGroup = new QGroupBox("Создание примитивов");
    auto* primitivesLayout = new QVBoxLayout(primitivesGroup);
    m_createSegmentBtn = new QPushButton("Создать отрезок");
    primitivesLayout->addWidget(m_createSegmentBtn);

    // --- Сборка панели ---
    mainLayout->addWidget(sceneGroup);
    mainLayout->addWidget(objectsGroup); // Добавляем новую группу в layout
    mainLayout->addWidget(primitivesGroup);

    // --- Соединения ---
    connect(m_gridStepSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Control::gridStepChanged);
    connect(m_angleUnitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        emit angleUnitChanged(static_cast<AngleUnit>(m_angleUnitComboBox->itemData(index).toInt()));
    });
    connect(m_cartesianBtn, &QToolButton::clicked, this, &Control::onCartesianClicked);
    connect(m_polarBtn, &QToolButton::clicked, this, &Control::onPolarClicked);
    connect(m_createSegmentBtn, &QPushButton::clicked, this, [this](){/* ... */});

    // Новые соединения для списка и удаления
    connect(m_objectListWidget, &QListWidget::itemSelectionChanged, this, &Control::onSelectionChanged);
    connect(m_deleteBtn, &QPushButton::clicked, this, &Control::deleteRequested);
}

/**
 * @brief Обновляет содержимое списка объектов на основе данных из сцены.
 */
void Control::updateObjectList(const Scene* scene)
{
    // Блокируем сигналы, чтобы избежать лишних срабатываний onSelectionChanged во время очистки
    m_objectListWidget->blockSignals(true);
    m_objectListWidget->clear();
    if (!scene) {
        m_objectListWidget->blockSignals(false);
        return;
    }

    int segmentCount = 1;
    for (const auto& obj : scene->getPrimitives()) {
        if (obj->getType() == PrimitiveType::Segment) {
            QString itemName = QString("Отрезок %1").arg(segmentCount++);
            QListWidgetItem* item = new QListWidgetItem(itemName, m_objectListWidget);
            // Сохраняем указатель на объект внутри элемента списка для последующего доступа
            item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(obj.get())));
        }
    }
    m_objectListWidget->blockSignals(false);
}

/**
 * @brief Срабатывает при выборе элемента в списке и испускает сигнал objectSelected.
 */
void Control::onSelectionChanged()
{
    auto selectedItems = m_objectListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        // Извлекаем сохраненный указатель на объект
        Object* selectedObject = static_cast<Object*>(selectedItems.first()->data(Qt::UserRole).value<void*>());
        emit objectSelected(selectedObject);
    } else {
        emit objectSelected(nullptr); // Если ничего не выбрано
    }
}

void Control::onCartesianClicked() { emit coordinateSystemChanged(CoordinateSystemType::Cartesian); }
void Control::onPolarClicked() { emit coordinateSystemChanged(CoordinateSystemType::Polar); }
