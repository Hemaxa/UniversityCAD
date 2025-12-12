#include "Control.h"
#include "Scene.h"
#include "Object.h"

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
#include <QMouseEvent>
#include <QApplication>
#include <QPainter>       // Добавлено
#include <QPainterPath>   // Добавлено

// =========================================================
// Реализация LongPressButton
// =========================================================
LongPressButton::LongPressButton(QWidget* parent) : QToolButton(parent) {
    m_longPressTimer.setSingleShot(true);
    m_longPressTimer.setInterval(400);
    connect(&m_longPressTimer, &QTimer::timeout, this, &LongPressButton::onTimerTimeout);
}

void LongPressButton::setPopupWidget(QWidget* popup) {
    m_popupWidget = popup;
}

void LongPressButton::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_isLongPressHandled = false;
        m_longPressTimer.start();
    }
    QToolButton::mousePressEvent(e);
}

void LongPressButton::mouseReleaseEvent(QMouseEvent* e) {
    m_longPressTimer.stop();
    if (!m_isLongPressHandled) {
        QToolButton::mouseReleaseEvent(e);
    } else {
        setDown(false);
    }
}

// РИСУЕМ ТРЕУГОЛЬНИК ИНДИКАТОРА
void LongPressButton::paintEvent(QPaintEvent* e) {
    // 1. Рисуем стандартную кнопку (фон, иконку)
    QToolButton::paintEvent(e);

    // 2. Если есть попап, рисуем треугольник в углу
    if (property("hasPopup").toBool()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Цвет треугольника (светло-серый)
        painter.setBrush(QColor(200, 200, 200));
        painter.setPen(Qt::NoPen);

        // Координаты нижнего правого угла
        // Отступ от края кнопки
        int m = 2;
        // Размер треугольника
        int s = 5;

        QPolygonF triangle;
        triangle << QPointF(width() - m, height() - m)           // Низ-право (угол)
                 << QPointF(width() - m - s, height() - m)       // Низ-лево
                 << QPointF(width() - m, height() - m - s);      // Верх-право

        painter.drawPolygon(triangle);
    }
}

void LongPressButton::onTimerTimeout() {
    m_isLongPressHandled = true;
    if (m_popupWidget) {
        // Рассчитываем позицию СВЕРХУ от кнопки

        // Получаем глобальные координаты верхнего левого угла кнопки
        QPoint globalPos = mapToGlobal(QPoint(0, 0));

        // Убеждаемся, что размер попапа вычислен
        m_popupWidget->adjustSize();

        // Позиция X: выровнять по левому краю кнопки
        int x = globalPos.x();

        // Позиция Y: Верх кнопки минус высота попапа минус небольшой отступ
        int y = globalPos.y() - m_popupWidget->height() - 2;

        m_popupWidget->move(x, y);
        m_popupWidget->show();
    }
    emit longPressActivated();
}

// =========================================================
// Реализация Control
// =========================================================

Control::Control(QWidget *parent) : QWidget(parent)
{
    this->setObjectName("ControlPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setSpacing(10);

    // --- 1. Параметры сцены ---
    auto* sceneGroup = new QGroupBox("Параметры сцены");
    auto* sceneLayout = new QGridLayout(sceneGroup);
    m_gridStepSpinBox = new QSpinBox(); m_gridStepSpinBox->setRange(10, 200); m_gridStepSpinBox->setValue(50);
    sceneLayout->addWidget(new QLabel("Сетка:"), 0, 0); sceneLayout->addWidget(m_gridStepSpinBox, 0, 1);
    m_zoomStepSpinBox = new QDoubleSpinBox(); m_zoomStepSpinBox->setRange(1.05, 2.0); m_zoomStepSpinBox->setValue(1.25);
    sceneLayout->addWidget(new QLabel("Зум:"), 0, 2); sceneLayout->addWidget(m_zoomStepSpinBox, 0, 3);
    m_angleUnitComboBox = new QComboBox(); m_angleUnitComboBox->addItem("Deg", 0); m_angleUnitComboBox->addItem("Rad", 1);
    sceneLayout->addWidget(new QLabel("Угол:"), 1, 0); sceneLayout->addWidget(m_angleUnitComboBox, 1, 1);
    auto* coordGroup = new QButtonGroup(this); auto* coordLayout = new QHBoxLayout();
    m_cartesianBtn = new QToolButton(); m_cartesianBtn->setText("XYZ"); m_cartesianBtn->setCheckable(true); m_cartesianBtn->setChecked(true);
    m_polarBtn = new QToolButton(); m_polarBtn->setText("Pol"); m_polarBtn->setCheckable(true);
    coordGroup->addButton(m_cartesianBtn); coordGroup->addButton(m_polarBtn);
    coordLayout->addWidget(m_cartesianBtn); coordLayout->addWidget(m_polarBtn);
    sceneLayout->addLayout(coordLayout, 1, 2, 1, 2);

    // --- 2. Объекты сцены ---
    auto* objectsGroup = new QGroupBox("Список объектов");
    auto* objectsLayout = new QVBoxLayout(objectsGroup);
    m_objectListWidget = new QListWidget();
    m_objectListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_deleteBtn = new QPushButton("Удалить"); m_deleteBtn->setObjectName("deleteButton");
    objectsLayout->addWidget(m_objectListWidget); objectsLayout->addWidget(m_deleteBtn);

    // --- 3. Инструменты ---
    auto* primitivesGroup = new QGroupBox("Инструменты");
    auto* primGrid = new QGridLayout(primitivesGroup);
    primGrid->setSpacing(5);

    m_primitiveToolsGroup = new QButtonGroup(this);
    m_primitiveToolsGroup->setExclusive(true);

    int col = 0, row = 0;

    auto addTool = [&](QString iconPath, QString tooltip, PrimitiveType type,
                       const std::vector<std::pair<QString, PrimitiveType>>& variants = {}) {

        LongPressButton* btn = new LongPressButton();
        btn->setIcon(QIcon(iconPath));
        btn->setToolTip(tooltip);
        btn->setCheckable(true);
        // Важно: устанавливаем свойство для CSS стилизации
        btn->setProperty("isIconButton", true);
        btn->setIconSize(QSize(24, 24));

        m_primitiveToolsGroup->addButton(btn, static_cast<int>(type));

        if (!variants.empty()) {
            btn->setProperty("hasPopup", true); // Для paintEvent
            QWidget* popup = createVariantPopup(btn, variants);
            btn->setPopupWidget(popup);
        }

        primGrid->addWidget(btn, row, col);

        connect(btn, &QToolButton::toggled, this, [this, type](bool checked){
            onPrimitiveToolToggled(checked, type);
        });

        col++;
        if(col > 3) { col = 0; row++; }
    };

    addTool(":/icons/segment.svg", "Отрезок", PrimitiveType::Segment);

    // Пример для проверки списка
    addTool(":/icons/circle.svg", "Окружность", PrimitiveType::Circle, {
                                                                                     {":/icons/circle.svg", PrimitiveType::Circle},
                                                                                     {":/icons/circle.svg", PrimitiveType::Circle}
                                                                                 });

    addTool(":/icons/arc.svg", "Дуга", PrimitiveType::Arc);
    addTool(":/icons/rectangle.svg", "Прямоугольник", PrimitiveType::Rectangle);
    addTool(":/icons/ellipse.svg", "Эллипс", PrimitiveType::Ellipse);
    addTool(":/icons/polygon.svg", "Многоугольник", PrimitiveType::Polygon);
    addTool(":/icons/spline.svg", "Сплайн", PrimitiveType::Spline);

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
}

QWidget* Control::createVariantPopup(LongPressButton* mainBtn, const std::vector<std::pair<QString, PrimitiveType>>& variants) {
    QWidget* popup = new QWidget(this);
    popup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    popup->setAttribute(Qt::WA_TranslucentBackground);

    // ИСПРАВЛЕНО: QVBoxLayout для вертикального списка
    QVBoxLayout* layout = new QVBoxLayout(popup);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(2);

    popup->setStyleSheet("background-color: #282936; border: 1px solid #4A4A5A; border-radius: 4px;");

    for (const auto& var : variants) {
        QToolButton* btn = new QToolButton(popup);
        btn->setIcon(QIcon(var.first));
        btn->setIconSize(QSize(24, 24));

        // Устанавливаем то же свойство, чтобы подхватился стиль розовой рамки при наведении
        btn->setProperty("isIconButton", true);
        btn->setCheckable(true);

        connect(btn, &QToolButton::clicked, this, [this, mainBtn, popup, var](){
            mainBtn->setIcon(QIcon(var.first));
            mainBtn->setChecked(true);
            onPrimitiveToolToggled(true, var.second);
            popup->close();
        });

        layout->addWidget(btn);
    }

    return popup;
}

void Control::updateObjectList(const Scene* scene) {
    // ... (без изменений, скопируйте из предыдущей версии если нужно) ...
    // Внимание: для экономии места я пропустил тело метода, так как оно не менялось.
    // Если вы будете копипастить целиком, убедитесь, что этот метод полон, как в предыдущем ответе.
    m_updatingSelection = true;
    m_objectListWidget->blockSignals(true);
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
            for(auto* o : oldSel) if(o == obj.get()) item->setSelected(true);
        }
    }
    m_objectListWidget->blockSignals(false);
    m_updatingSelection = false;
}

// ... Остальные методы без изменений
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
