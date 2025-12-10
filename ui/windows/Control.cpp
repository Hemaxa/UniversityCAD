#include "Control.h"
#include "Scene.h"
#include "Object.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QButtonGroup>
#include <QListWidget>
#include <QLabel>
#include <QKeySequence>

Control::Control(QWidget *parent) : QWidget(parent)
{
    this->setObjectName("ControlPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(15);

    auto* sceneGroup = new QGroupBox("Параметры сцены");
    auto* sceneLayout = new QGridLayout(sceneGroup);
    sceneLayout->setAlignment(Qt::AlignLeft);
    sceneLayout->setHorizontalSpacing(15);

    m_gridStepSpinBox = new QSpinBox();
    m_gridStepSpinBox->setRange(10, 200);
    m_gridStepSpinBox->setValue(50);
    sceneLayout->addWidget(new QLabel("Шаг сетки:"), 0, 0);
    sceneLayout->addWidget(m_gridStepSpinBox, 0, 1);

    m_angleUnitComboBox = new QComboBox();
    m_angleUnitComboBox->addItem("Градусы", static_cast<int>(AngleUnit::Degrees));
    m_angleUnitComboBox->addItem("Радианы", static_cast<int>(AngleUnit::Radians));
    sceneLayout->addWidget(new QLabel("Единицы:"), 1, 0);
    sceneLayout->addWidget(m_angleUnitComboBox, 1, 1);

    m_zoomStepSpinBox = new QDoubleSpinBox();
    m_zoomStepSpinBox->setRange(1.05, 2.0);
    m_zoomStepSpinBox->setSingleStep(0.05);
    m_zoomStepSpinBox->setValue(1.25);
    sceneLayout->addWidget(new QLabel("Шаг зума:"), 0, 2);
    sceneLayout->addWidget(m_zoomStepSpinBox, 0, 3);

    auto* coordLayout = new QHBoxLayout();
    coordLayout->setContentsMargins(0,0,0,0);
    auto* coordGroup = new QButtonGroup(this);
    coordGroup->setExclusive(true);
    m_cartesianBtn = new QToolButton();
    m_cartesianBtn->setText("XYZ");
    m_cartesianBtn->setCheckable(true);
    m_cartesianBtn->setChecked(true);
    m_polarBtn = new QToolButton();
    m_polarBtn->setText("Pol");
    m_polarBtn->setCheckable(true);
    coordGroup->addButton(m_cartesianBtn);
    coordGroup->addButton(m_polarBtn);
    coordLayout->addWidget(m_cartesianBtn);
    coordLayout->addWidget(m_polarBtn);
    sceneLayout->addWidget(new QLabel("Коорд.:"), 1, 2);
    sceneLayout->addLayout(coordLayout, 1, 3);

    auto* objectsGroup = new QGroupBox("Объекты сцены");
    auto* objectsLayout = new QVBoxLayout(objectsGroup);
    m_objectListWidget = new QListWidget();
    // Включаем расширенное выделение (Ctrl/Shift)
    m_objectListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_deleteBtn = new QPushButton("Удалить выбранный");
    m_deleteBtn->setObjectName("deleteButton");
    objectsLayout->addWidget(m_objectListWidget);
    objectsLayout->addWidget(m_deleteBtn);

    auto* primitivesGroup = new QGroupBox("Создание объектов");
    auto* primitivesLayout = new QHBoxLayout(primitivesGroup);
    primitivesLayout->setAlignment(Qt::AlignLeft);

    m_primitiveToolsGroup = new QButtonGroup(this);
    m_primitiveToolsGroup->setExclusive(true);

    m_createSegmentBtn = new QToolButton();
    m_createSegmentBtn->setCheckable(true);
    m_createSegmentBtn->setIcon(QIcon(":/icons/segment.svg"));
    m_createSegmentBtn->setIconSize(QSize(20, 20));
    m_createSegmentBtn->setProperty("isIconButton", true);
    m_createSegmentBtn->setShortcut(Qt::Key_S);
    m_createSegmentBtn->setToolTip("Отрезок (S)");

    m_primitiveToolsGroup->addButton(m_createSegmentBtn);
    primitivesLayout->addWidget(m_createSegmentBtn);

    mainLayout->addWidget(sceneGroup);
    mainLayout->addWidget(objectsGroup);
    mainLayout->addWidget(primitivesGroup);

    connect(m_gridStepSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Control::gridStepChanged);
    connect(m_zoomStepSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Control::zoomStepChanged);
    connect(m_angleUnitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        emit angleUnitChanged(static_cast<AngleUnit>(m_angleUnitComboBox->itemData(index).toInt()));
    });
    connect(m_cartesianBtn, &QToolButton::clicked, this, &Control::onCartesianClicked);
    connect(m_polarBtn, &QToolButton::clicked, this, &Control::onPolarClicked);

    connect(m_objectListWidget, &QListWidget::itemSelectionChanged, this, &Control::onSelectionChanged);

    connect(m_deleteBtn, &QPushButton::clicked, this, &Control::deleteRequested);

    connect(m_createSegmentBtn, &QToolButton::toggled, this, [this](bool checked){
        onPrimitiveToolToggled(checked, PrimitiveType::Segment);
    });
}

void Control::updateObjectList(const Scene* scene)
{
    m_updatingSelection = true; // Блокируем сигналы во время обновления
    m_objectListWidget->blockSignals(true);

    // Сохраняем текущее выделение (pointer set)
    std::vector<Object*> currentSelection;
    for (auto* item : m_objectListWidget->selectedItems()) {
        currentSelection.push_back(static_cast<Object*>(item->data(Qt::UserRole).value<void*>()));
    }

    m_objectListWidget->clear();
    if (!scene) {
        m_objectListWidget->blockSignals(false);
        m_updatingSelection = false;
        return;
    }

    for (const auto& obj : scene->getPrimitives()) {
        QString itemName;
        if (obj->getType() == PrimitiveType::Segment) {
            itemName = QString("Отрезок %1").arg(obj->getID());
        }
        else {
            itemName = QString("Объект %1").arg(obj->getID());
        }

        QListWidgetItem* item = new QListWidgetItem(itemName, m_objectListWidget);
        item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(obj.get())));

        // Восстанавливаем выделение
        for (auto* selObj : currentSelection) {
            if (selObj == obj.get()) {
                item->setSelected(true);
                break;
            }
        }
    }

    m_objectListWidget->blockSignals(false);
    m_updatingSelection = false;
}

void Control::onSelectionChanged()
{
    if (m_updatingSelection) return;

    std::vector<Object*> selectedObjects;
    auto selectedItems = m_objectListWidget->selectedItems();

    for (auto* item : selectedItems) {
        Object* obj = static_cast<Object*>(item->data(Qt::UserRole).value<void*>());
        selectedObjects.push_back(obj);
    }

    emit objectsSelected(selectedObjects);
}

void Control::setSelectedObjects(const std::vector<Object*>& objects)
{
    if (m_updatingSelection) return;
    m_updatingSelection = true;
    m_objectListWidget->blockSignals(true);

    m_objectListWidget->clearSelection();

    for (int i = 0; i < m_objectListWidget->count(); ++i) {
        QListWidgetItem* item = m_objectListWidget->item(i);
        Object* itemObj = static_cast<Object*>(item->data(Qt::UserRole).value<void*>());

        for (auto* obj : objects) {
            if (itemObj == obj) {
                item->setSelected(true);
                break;
            }
        }
    }

    m_objectListWidget->blockSignals(false);
    m_updatingSelection = false;
}

void Control::clearSelection()
{
    m_objectListWidget->clearSelection();
}

void Control::resetTools()
{
    // Снимаем выделение с группы кнопок (Reset to generic)
    if (QAbstractButton* checked = m_primitiveToolsGroup->checkedButton()) {
        m_primitiveToolsGroup->setExclusive(false);
        checked->setChecked(false);
        m_primitiveToolsGroup->setExclusive(true);
        emit primitiveTypeSelected(PrimitiveType::Generic);
    }
}

void Control::onCartesianClicked() { emit coordinateSystemChanged(CoordinateSystemType::Cartesian); }
void Control::onPolarClicked() { emit coordinateSystemChanged(CoordinateSystemType::Polar); }

void Control::onPrimitiveToolToggled(bool checked, PrimitiveType type)
{
    if (checked) {
        emit primitiveTypeSelected(type);
    } else {
        // Если кнопка отжалась и ни одна другая не нажата (группа exclusive handle this mostly, but for manual uncheck)
        if (!m_primitiveToolsGroup->checkedButton()) {
            emit primitiveTypeSelected(PrimitiveType::Generic);
        }
    }
}
