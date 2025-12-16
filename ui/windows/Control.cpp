#include "Control.h"
#include "Scene.h"
#include "Object.h"
#include "LineSettingsMenu.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QListWidget>
#include <QLabel>
#include <QCheckBox>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QIcon>

// --- LongPressButton ---
LongPressButton::LongPressButton(QWidget* parent) : QToolButton(parent) {
    m_longPressTimer.setSingleShot(true); m_longPressTimer.setInterval(400);
    connect(&m_longPressTimer, &QTimer::timeout, this, &LongPressButton::onTimerTimeout);
}
void LongPressButton::paintEvent(QPaintEvent* e) {
    QToolButton::paintEvent(e);
    if (property("hasPopup").toBool()) {
        QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor(200, 200, 200)); painter.setPen(Qt::NoPen);
        int m = 4; int s = 5; QPolygonF triangle; triangle << QPointF(width()-m, height()-m) << QPointF(width()-m-s, height()-m) << QPointF(width()-m, height()-m-s);
        painter.drawPolygon(triangle);
    }
}
void LongPressButton::setPopupWidget(QWidget* popup) { m_popupWidget = popup; }
void LongPressButton::mousePressEvent(QMouseEvent* e) { if(e->button() == Qt::LeftButton) { m_isLongPressHandled = false; m_longPressTimer.start(); } QToolButton::mousePressEvent(e); }
void LongPressButton::mouseReleaseEvent(QMouseEvent* e) { m_longPressTimer.stop(); if (!m_isLongPressHandled) { QToolButton::mouseReleaseEvent(e); } else { setDown(false); } }
void LongPressButton::onTimerTimeout() { m_isLongPressHandled = true; if (m_popupWidget) { QPoint gp = mapToGlobal(QPoint(0,0)); m_popupWidget->adjustSize(); m_popupWidget->move(gp.x(), gp.y() - m_popupWidget->height() - 2); m_popupWidget->show(); } emit longPressActivated(); }

// --- Control ---
Control::Control(QWidget *parent) : QWidget(parent)
{
    this->setObjectName("ControlPanel");
    auto* mainLayout = new QVBoxLayout(this); mainLayout->setContentsMargins(0,0,0,0);
    QScrollArea* scrollArea = new QScrollArea(this); scrollArea->setWidgetResizable(true); scrollArea->setFrameShape(QFrame::NoFrame);
    QWidget* scrollContent = new QWidget();
    auto* contentLayout = new QVBoxLayout(scrollContent); contentLayout->setContentsMargins(8, 8, 8, 8); contentLayout->setSpacing(10); contentLayout->setAlignment(Qt::AlignTop);

    auto* sceneGroup = new QGroupBox("Параметры сцены");
    auto* sceneLayout = new QGridLayout(sceneGroup);
    m_gridStepSpinBox = new QSpinBox(); m_gridStepSpinBox->setRange(10, 200); m_gridStepSpinBox->setValue(50);
    sceneLayout->addWidget(new QLabel("Сетка:"), 0, 0); sceneLayout->addWidget(m_gridStepSpinBox, 0, 1);
    m_zoomStepSpinBox = new QDoubleSpinBox(); m_zoomStepSpinBox->setRange(1.05, 2.0); m_zoomStepSpinBox->setSingleStep(0.05); m_zoomStepSpinBox->setValue(1.25);
    sceneLayout->addWidget(new QLabel("Зум:"), 0, 2); sceneLayout->addWidget(m_zoomStepSpinBox, 0, 3);
    m_angleUnitComboBox = new QComboBox(); m_angleUnitComboBox->addItem("Deg", 0); m_angleUnitComboBox->addItem("Rad", 1);
    sceneLayout->addWidget(new QLabel("Угол:"), 1, 0); sceneLayout->addWidget(m_angleUnitComboBox, 1, 1);

    auto* coordGroup = new QButtonGroup(this); auto* coordLayout = new QHBoxLayout();
    m_cartesianBtn = new QToolButton(); m_cartesianBtn->setText("XYZ"); m_cartesianBtn->setCheckable(true); m_cartesianBtn->setChecked(true);
    m_polarBtn = new QToolButton(); m_polarBtn->setText("Pol"); m_polarBtn->setCheckable(true);
    coordGroup->addButton(m_cartesianBtn); coordGroup->addButton(m_polarBtn);
    coordLayout->addWidget(m_cartesianBtn); coordLayout->addWidget(m_polarBtn);
    sceneLayout->addLayout(coordLayout, 1, 2, 1, 2);

    m_gridSnapCheck = new QCheckBox("Привязка к сетке");
    m_objSnapCheck = new QCheckBox("Привязка к объектам"); m_objSnapCheck->setChecked(true);
    sceneLayout->addWidget(m_gridSnapCheck, 2, 0, 1, 2);
    sceneLayout->addWidget(m_objSnapCheck, 2, 2, 1, 2);

    auto* lineSettingsBtn = new QPushButton("Настройки линий");
    lineSettingsBtn->setIcon(QIcon(":/icons/settings.svg"));
    auto* lineMenu = new LineSettingsMenu(this);
    connect(lineMenu, &LineSettingsMenu::settingsChanged, this, [this](){
        if(parentWidget()) parentWidget()->update();
    });
    lineSettingsBtn->setMenu(lineMenu);
    sceneLayout->addWidget(lineSettingsBtn, 3, 0, 1, 4);

    auto* primitivesGroup = new QGroupBox("Инструменты"); auto* primGrid = new QGridLayout(primitivesGroup); primGrid->setSpacing(5);
    m_primitiveToolsGroup = new QButtonGroup(this); m_primitiveToolsGroup->setExclusive(true);
    connect(m_primitiveToolsGroup, &QButtonGroup::idClicked, this, &Control::onPrimitiveToolClicked);

    int col = 0, row = 0;
    auto addTool = [&](QString iconPath, QString tooltip, PrimitiveType type, const std::vector<std::pair<QString, int>>& variants) {
        LongPressButton* btn = new LongPressButton(); btn->setIcon(QIcon(iconPath)); btn->setIconSize(QSize(24, 24)); btn->setToolTip(tooltip); btn->setCheckable(true); btn->setProperty("isIconButton", true);
        m_primitiveToolsGroup->addButton(btn, static_cast<int>(type)); m_activeSubMethods[type] = 0;
        if (variants.size() > 1) { btn->setProperty("hasPopup", true); QWidget* popup = createVariantPopup(btn, variants, type); btn->setPopupWidget(popup); }
        primGrid->addWidget(btn, row, col); col++; if(col > 3) { col = 0; row++; }
    };
    addTool(":/icons/segment.svg", "Отрезок", PrimitiveType::Segment, {});
    // Для окружности передаем варианты: Центр+Радиус(0), Центр+Диаметр(1), 2Точки(2), 3Точки(3)
    addTool(":/icons/circle-1.svg", "Окружность", PrimitiveType::Circle, {{":/icons/circle-1.svg", 0}, {":/icons/circle-2.svg", 1}, {":/icons/circle-3.svg", 2}, {":/icons/circle-4.svg", 3}});
    addTool(":/icons/arc-1.svg", "Дуга", PrimitiveType::Arc, {{":/icons/arc-1.svg", 0}, {":/icons/arc-2.svg", 1}});
    addTool(":/icons/rectangle-1.svg", "Прямоугольник", PrimitiveType::Rectangle, {{":/icons/rectangle-1.svg", 0}, {":/icons/rectangle-2.svg", 1}, {":/icons/rectangle-3.svg", 2}});
    addTool(":/icons/ellipse-1.svg", "Эллипс", PrimitiveType::Ellipse, {{":/icons/ellipse-1.svg", 0}, {":/icons/ellipse-2.svg", 1}});
    addTool(":/icons/polygon.svg", "Многоугольник", PrimitiveType::Polygon, {});
    addTool(":/icons/spline.svg", "Сплайн", PrimitiveType::Spline, {});

    auto* objectsGroup = new QGroupBox("Список объектов"); auto* objectsLayout = new QVBoxLayout(objectsGroup);
    m_objectListWidget = new QListWidget(); m_objectListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection); m_objectListWidget->setMinimumHeight(150);
    m_deleteBtn = new QPushButton("Удалить"); m_deleteBtn->setObjectName("deleteButton");
    objectsLayout->addWidget(m_objectListWidget); objectsLayout->addWidget(m_deleteBtn);

    contentLayout->addWidget(sceneGroup); contentLayout->addWidget(primitivesGroup); contentLayout->addWidget(objectsGroup); contentLayout->addStretch();
    scrollArea->setWidget(scrollContent); mainLayout->addWidget(scrollArea);

    connect(m_gridStepSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &Control::gridStepChanged);
    connect(m_zoomStepSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Control::zoomStepChanged);
    connect(m_cartesianBtn, &QToolButton::clicked, this, &Control::onCartesianClicked);
    connect(m_polarBtn, &QToolButton::clicked, this, &Control::onPolarClicked);
    connect(m_objectListWidget, &QListWidget::itemSelectionChanged, this, &Control::onSelectionChanged);
    connect(m_deleteBtn, &QPushButton::clicked, this, &Control::deleteRequested);
    connect(m_angleUnitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){ emit angleUnitChanged(static_cast<AngleUnit>(m_angleUnitComboBox->itemData(index).toInt())); });
    connect(m_gridSnapCheck, &QCheckBox::toggled, this, &Control::gridSnapToggled);
    connect(m_objSnapCheck, &QCheckBox::toggled, this, &Control::objectSnapToggled);
}

QWidget* Control::createVariantPopup(LongPressButton* mainBtn, const std::vector<std::pair<QString, int>>& variants, PrimitiveType type) {
    QWidget* popup = new QWidget(this); popup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint); popup->setAttribute(Qt::WA_TranslucentBackground);
    QVBoxLayout* layout = new QVBoxLayout(popup); layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
    popup->setStyleSheet("background-color: #282936; border: 1px solid #4A4A5A; border-radius: 4px;");

    for (const auto& var : variants) {
        QToolButton* btn = new QToolButton(popup);
        btn->setIcon(QIcon(var.first));
        btn->setIconSize(QSize(24, 24));
        btn->setProperty("isIconButton", true);

        int methodIndex = var.second;
        // Обработчик выбора варианта инструмента
        connect(btn, &QToolButton::clicked, this, [this, mainBtn, popup, type, methodIndex, var](){
            popup->close();
            
            // First update the internal state
            m_activeSubMethods[type] = methodIndex;
            
            // Update UI
            mainBtn->setIcon(QIcon(var.first));
            
            // Block signals to prevent double emission when setting checked state
            mainBtn->blockSignals(true);
            // Ensure main button is checked/active (this might trigger other signals, so do it after state update)
            mainBtn->setChecked(true);
            mainBtn->blockSignals(false);
           
            // Finally emit the signal with the CORRECT method index
            emit primitiveTypeSelected(type, methodIndex);
        });
        layout->addWidget(btn);
    }
    return popup;
}

void Control::onPrimitiveToolClicked(int id) {
    PrimitiveType type = static_cast<PrimitiveType>(id);
    int method = m_activeSubMethods[type];
    emit primitiveTypeSelected(type, method);
}

void Control::resetTools() {
    if (auto* btn = m_primitiveToolsGroup->checkedButton()) {
        m_primitiveToolsGroup->setExclusive(false);
        btn->setChecked(false);
        m_primitiveToolsGroup->setExclusive(true);
    }
    emit primitiveTypeSelected(PrimitiveType::Generic, 0);
}

void Control::onCartesianClicked() { emit coordinateSystemChanged(CoordinateSystemType::Cartesian); }
void Control::onPolarClicked() { emit coordinateSystemChanged(CoordinateSystemType::Polar); }

void Control::updateObjectList(const Scene* scene) {
    m_updatingSelection = true; m_objectListWidget->blockSignals(true);
    std::vector<Object*> oldSel;
    for(auto* item : m_objectListWidget->selectedItems())
        oldSel.push_back(static_cast<Object*>(item->data(Qt::UserRole).value<void*>()));

    m_objectListWidget->clear();
    if (scene) {
        for (const auto& obj : scene->getPrimitives()) {
            QString name;
            switch(obj->getType()){
            case PrimitiveType::Segment: name="Отрезок"; break;
            case PrimitiveType::Circle: name="Окружность"; break;
            case PrimitiveType::Arc: name="Дуга"; break;
            case PrimitiveType::Rectangle: name="Прямоугольник"; break;
            case PrimitiveType::Ellipse: name="Эллипс"; break;
            case PrimitiveType::Polygon: name="Полигон"; break;
            case PrimitiveType::Spline: name="Сплайн"; break;
            default: name="Объект"; break;
            }
            QListWidgetItem* item = new QListWidgetItem(QString("%1 %2").arg(name).arg(obj->getID()));
            item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(obj.get())));
            m_objectListWidget->addItem(item);
            for(auto* o : oldSel) if(o == obj.get()) item->setSelected(true);
        }
    }
    m_objectListWidget->blockSignals(false); m_updatingSelection = false;
}

void Control::onSelectionChanged() {
    if (m_updatingSelection) return;
    std::vector<Object*> selectedObjects;
    for (auto* item : m_objectListWidget->selectedItems())
        selectedObjects.push_back(static_cast<Object*>(item->data(Qt::UserRole).value<void*>()));
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
