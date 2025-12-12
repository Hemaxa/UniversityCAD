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
#include <QMenu>
#include <QAction>

Control::Control(QWidget *parent) : QWidget(parent)
{
    this->setObjectName("ControlPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(10);

    // --- 1. Параметры сцены ---
    auto* sceneGroup = new QGroupBox("Параметры сцены");
    auto* sceneLayout = new QGridLayout(sceneGroup);

    m_gridStepSpinBox = new QSpinBox();
    m_gridStepSpinBox->setRange(10, 200); m_gridStepSpinBox->setValue(50);
    sceneLayout->addWidget(new QLabel("Сетка:"), 0, 0);
    sceneLayout->addWidget(m_gridStepSpinBox, 0, 1);

    m_zoomStepSpinBox = new QDoubleSpinBox();
    m_zoomStepSpinBox->setRange(1.05, 2.0); m_zoomStepSpinBox->setValue(1.25); m_zoomStepSpinBox->setSingleStep(0.05);
    sceneLayout->addWidget(new QLabel("Зум:"), 0, 2);
    sceneLayout->addWidget(m_zoomStepSpinBox, 0, 3);

    m_angleUnitComboBox = new QComboBox();
    m_angleUnitComboBox->addItem("Deg", static_cast<int>(AngleUnit::Degrees));
    m_angleUnitComboBox->addItem("Rad", static_cast<int>(AngleUnit::Radians));
    sceneLayout->addWidget(new QLabel("Угол:"), 1, 0);
    sceneLayout->addWidget(m_angleUnitComboBox, 1, 1);

    auto* coordGroup = new QButtonGroup(this);
    auto* coordLayout = new QHBoxLayout(); coordLayout->setContentsMargins(0,0,0,0);
    m_cartesianBtn = new QToolButton(); m_cartesianBtn->setText("XYZ"); m_cartesianBtn->setCheckable(true); m_cartesianBtn->setChecked(true);
    m_polarBtn = new QToolButton(); m_polarBtn->setText("Pol"); m_polarBtn->setCheckable(true);
    coordGroup->addButton(m_cartesianBtn); coordGroup->addButton(m_polarBtn);
    coordLayout->addWidget(m_cartesianBtn); coordLayout->addWidget(m_polarBtn);
    sceneLayout->addLayout(coordLayout, 1, 2, 1, 2);

    // --- 2. Объекты сцены (Список) ---
    auto* objectsGroup = new QGroupBox("Список объектов");
    auto* objectsLayout = new QVBoxLayout(objectsGroup);
    m_objectListWidget = new QListWidget();
    m_objectListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_deleteBtn = new QPushButton("Удалить");
    m_deleteBtn->setObjectName("deleteButton");
    objectsLayout->addWidget(m_objectListWidget);
    objectsLayout->addWidget(m_deleteBtn);

    // --- 3. Инструменты (Создание) ---
    auto* primitivesGroup = new QGroupBox("Инструменты");
    auto* primGrid = new QGridLayout(primitivesGroup);
    primGrid->setSpacing(5);

    m_primitiveToolsGroup = new QButtonGroup(this);
    m_primitiveToolsGroup->setExclusive(true);

    // Функция-хелпер для создания кнопок
    int col = 0, row = 0;
    auto addTool = [&](QString icon, QString text, PrimitiveType type, bool hasMenu = false) {
        QToolButton* btn = new QToolButton();
        btn->setIcon(QIcon(icon));
        btn->setToolTip(text);
        btn->setCheckable(true);
        btn->setProperty("isIconButton", true); // Для QSS
        btn->setIconSize(QSize(24,24));

        // Если нужны разные методы создания, добавляем меню
        if (hasMenu) {
            btn->setPopupMode(QToolButton::MenuButtonPopup);
            QMenu* menu = new QMenu(this);
            // Просто действия-заглушки, которые выбирают инструмент.
            // Конкретный метод выберем в Properties.
            menu->addAction("По умолчанию", this, [this, btn](){ btn->click(); });
            btn->setMenu(menu);
        }

        m_primitiveToolsGroup->addButton(btn, static_cast<int>(type));
        primGrid->addWidget(btn, row, col);

        col++;
        if(col > 3) { col = 0; row++; }

        connect(btn, &QToolButton::toggled, this, [this, type](bool checked){
            onPrimitiveToolToggled(checked, type);
        });
    };

    addTool(":/icons/segment.svg", "Отрезок", PrimitiveType::Segment);
    addTool(":/icons/circle.svg", "Окружность", PrimitiveType::Circle, true); // Has Menu
    addTool(":/icons/arc.svg", "Дуга", PrimitiveType::Arc, true); // Has Menu
    addTool(":/icons/rectangle.svg", "Прямоугольник", PrimitiveType::Rectangle, true); // Has Menu
    addTool(":/icons/ellipse.svg", "Эллипс", PrimitiveType::Ellipse);
    addTool(":/icons/polygon.svg", "Многоугольник", PrimitiveType::Polygon);
    addTool(":/icons/spline.svg", "Сплайн", PrimitiveType::Spline);

    mainLayout->addWidget(sceneGroup);
    mainLayout->addWidget(objectsGroup);
    mainLayout->addWidget(primitivesGroup);

    // --- Signals ---
    connect(m_gridStepSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Control::gridStepChanged);
    connect(m_zoomStepSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Control::zoomStepChanged);
    connect(m_angleUnitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        emit angleUnitChanged(static_cast<AngleUnit>(m_angleUnitComboBox->itemData(index).toInt()));
    });
    connect(m_cartesianBtn, &QToolButton::clicked, this, &Control::onCartesianClicked);
    connect(m_polarBtn, &QToolButton::clicked, this, &Control::onPolarClicked);
    connect(m_objectListWidget, &QListWidget::itemSelectionChanged, this, &Control::onSelectionChanged);
    connect(m_deleteBtn, &QPushButton::clicked, this, &Control::deleteRequested);
}

void Control::updateObjectList(const Scene* scene) {
    m_updatingSelection = true;
    m_objectListWidget->blockSignals(true);

    // Сохраняем выделение
    std::vector<Object*> oldSel;
    for(auto* item : m_objectListWidget->selectedItems())
        oldSel.push_back(static_cast<Object*>(item->data(Qt::UserRole).value<void*>()));

    m_objectListWidget->clear();
    if (scene) {
        for (const auto& obj : scene->getPrimitives()) {
            QString name;
            switch(obj->getType()){
            case PrimitiveType::Segment: name = "Отрезок"; break;
            case PrimitiveType::Circle: name = "Окружность"; break;
            case PrimitiveType::Arc: name = "Дуга"; break;
            case PrimitiveType::Rectangle: name = "Прямоугольник"; break;
            case PrimitiveType::Ellipse: name = "Эллипс"; break;
            case PrimitiveType::Polygon: name = "Полигон"; break;
            case PrimitiveType::Spline: name = "Сплайн"; break;
            default: name = "Объект"; break;
            }
            QListWidgetItem* item = new QListWidgetItem(QString("%1 %2").arg(name).arg(obj->getID()));
            item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(obj.get())));
            m_objectListWidget->addItem(item);

            // Восстанавливаем выделение
            for(auto* o : oldSel) if(o == obj.get()) item->setSelected(true);
        }
    }
    m_objectListWidget->blockSignals(false);
    m_updatingSelection = false;
}

void Control::onSelectionChanged() {
    if (m_updatingSelection) return;
    std::vector<Object*> selectedObjects;
    for (auto* item : m_objectListWidget->selectedItems()) {
        selectedObjects.push_back(static_cast<Object*>(item->data(Qt::UserRole).value<void*>()));
    }
    emit objectsSelected(selectedObjects);
}

void Control::setSelectedObjects(const std::vector<Object*>& objects) {
    if (m_updatingSelection) return;
    m_updatingSelection = true;
    m_objectListWidget->blockSignals(true);
    m_objectListWidget->clearSelection();

    for(int i=0; i<m_objectListWidget->count(); ++i){
        auto* item = m_objectListWidget->item(i);
        Object* obj = static_cast<Object*>(item->data(Qt::UserRole).value<void*>());
        for(auto* sel : objects) if(sel == obj) item->setSelected(true);
    }
    m_objectListWidget->blockSignals(false);
    m_updatingSelection = false;
}

void Control::clearSelection() { m_objectListWidget->clearSelection(); }

void Control::resetTools() {
    if (auto* btn = m_primitiveToolsGroup->checkedButton()) {
        m_primitiveToolsGroup->setExclusive(false);
        btn->setChecked(false);
        m_primitiveToolsGroup->setExclusive(true);
    }
}

void Control::onCartesianClicked() { emit coordinateSystemChanged(CoordinateSystemType::Cartesian); }
void Control::onPolarClicked() { emit coordinateSystemChanged(CoordinateSystemType::Polar); }

void Control::onPrimitiveToolToggled(bool checked, PrimitiveType type) {
    if (checked) emit primitiveTypeSelected(type);
    else if (!m_primitiveToolsGroup->checkedButton()) emit primitiveTypeSelected(PrimitiveType::Generic);
}
