#include "Properties.h"
#include "Point.h"
#include "Segment.h"
#include "Circle.h"
#include "Arc.h"
#include "Rectangle.h" // Убедитесь, что имя файла совпадает (не RectanglePrim)
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include "StyleDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QMenu>
#include <QStyle>
#include <cmath>

// Хелпер
static QDoubleSpinBox* createSpin(double val=0, double min=-10000, double max=10000) {
    auto* s = new QDoubleSpinBox(); s->setRange(min, max); s->setValue(val); s->setDecimals(2); return s;
}

Properties::Properties(QWidget *parent)
    : QWidget(parent), m_isCreationMode(true)
{
    this->setObjectName("PropertiesPanel");
    auto* mainLayout = new QVBoxLayout(this);

    m_stack = new QStackedWidget();
    m_placeholderWidget = createPlaceholder();
    m_stack->addWidget(m_placeholderWidget);

    m_primitiveWidgets[PrimitiveType::Segment] = createSegmentWidget();
    m_primitiveWidgets[PrimitiveType::Circle] = createCircleWidget();
    m_primitiveWidgets[PrimitiveType::Arc] = createArcWidget();
    m_primitiveWidgets[PrimitiveType::Rectangle] = createRectangleWidget();
    m_primitiveWidgets[PrimitiveType::Ellipse] = createEllipseWidget();
    m_primitiveWidgets[PrimitiveType::Polygon] = createPolygonWidget();
    m_primitiveWidgets[PrimitiveType::Spline] = createSplineWidget();

    for (auto& pair : m_primitiveWidgets) {
        m_stack->addWidget(pair.second);
    }
    mainLayout->addWidget(m_stack);

    // ИСПРАВЛЕНО: Типы теперь совпадают (QGroupBox*)
    m_styleGroup = createStyleWidget();
    mainLayout->addWidget(m_styleGroup);

    m_applyButton = new QPushButton("Создать");
    m_applyButton->setObjectName("ApplyButton");
    mainLayout->addWidget(m_applyButton);

    connect(m_applyButton, &QPushButton::clicked, this, &Properties::onApplyClicked);

    m_availableStyles = {
        {LineStyleType::Solid, "Сплошная", 0.8, 0, 0, true},
        {LineStyleType::Dashed, "Штриховая", 0.8, 4.0, 2.0, true}
    };
    m_currentStyle = m_availableStyles[0];

    showCreationPropertiesFor(PrimitiveType::Generic);
}

// ... (оставьте методы createPlaceholder, createSegmentWidget и др. без изменений) ...

// Копируем методы создания виджетов из вашего исходного файла, чтобы код был полным
QWidget* Properties::createPlaceholder() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    auto* lbl = new QLabel("Нет выбора"); lbl->setAlignment(Qt::AlignCenter); lbl->setObjectName("PlaceholderLabel");
    l->addWidget(lbl); return w;
}

QWidget* Properties::createSegmentWidget() {
    auto* w = new QWidget(); auto* l = new QFormLayout(w);
    auto* g = new QGroupBox("Отрезок"); auto* gl = new QFormLayout(g);
    m_segX1 = createSpin(); m_segY1 = createSpin(); m_segX2 = createSpin(100); m_segY2 = createSpin(100);
    gl->addRow("X1:", m_segX1); gl->addRow("Y1:", m_segY1); gl->addRow("X2:", m_segX2); gl->addRow("Y2:", m_segY2);
    l->addWidget(g); return w;
}

QWidget* Properties::createCircleWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    m_circleMethodCombo = new QComboBox();
    m_circleMethodCombo->addItem("Центр и Радиус"); m_circleMethodCombo->addItem("Две точки (Диаметр)");
    l->addWidget(new QLabel("Метод:")); l->addWidget(m_circleMethodCombo);
    auto* stack = new QStackedWidget();
    auto* p1 = new QWidget(); auto* f1 = new QFormLayout(p1);
    m_circCX = createSpin(); m_circCY = createSpin(); m_circR = createSpin(50, 0);
    f1->addRow("Центр X:", m_circCX); f1->addRow("Центр Y:", m_circCY); f1->addRow("Радиус:", m_circR);
    stack->addWidget(p1);
    auto* p2 = new QWidget(); auto* f2 = new QFormLayout(p2);
    m_circP1X = createSpin(); m_circP1Y = createSpin(); m_circP2X = createSpin(100); m_circP2Y = createSpin(100);
    f2->addRow("Точка 1 X:", m_circP1X); f2->addRow("Точка 1 Y:", m_circP1Y); f2->addRow("Точка 2 X:", m_circP2X); f2->addRow("Точка 2 Y:", m_circP2Y);
    stack->addWidget(p2);
    connect(m_circleMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), stack, &QStackedWidget::setCurrentIndex);
    l->addWidget(stack); return w;
}

QWidget* Properties::createArcWidget() {
    auto* w = new QWidget(); auto* f = new QFormLayout(w);
    m_arcCX = createSpin(); m_arcCY = createSpin(); m_arcR = createSpin(50, 0);
    m_arcStart = createSpin(0, -360, 360); m_arcSpan = createSpin(90, -360, 360);
    f->addRow("Центр X:", m_arcCX); f->addRow("Центр Y:", m_arcCY); f->addRow("Радиус:", m_arcR);
    f->addRow("Начало (°):", m_arcStart); f->addRow("Угол (°):", m_arcSpan); return w;
}

QWidget* Properties::createRectangleWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    m_rectMethodCombo = new QComboBox();
    m_rectMethodCombo->addItem("Две точки"); m_rectMethodCombo->addItem("Центр и Размер");
    l->addWidget(new QLabel("Метод:")); l->addWidget(m_rectMethodCombo);
    auto* stack = new QStackedWidget();
    auto* p1 = new QWidget(); auto* f1 = new QFormLayout(p1);
    m_rectP1X = createSpin(); m_rectP1Y = createSpin(); m_rectP2X = createSpin(100); m_rectP2Y = createSpin(100);
    f1->addRow("Точка 1 X:", m_rectP1X); f1->addRow("Точка 1 Y:", m_rectP1Y); f1->addRow("Точка 2 X:", m_rectP2X); f1->addRow("Точка 2 Y:", m_rectP2Y);
    stack->addWidget(p1);
    auto* p2 = new QWidget(); auto* f2 = new QFormLayout(p2);
    m_rectCX = createSpin(); m_rectCY = createSpin(); m_rectW = createSpin(100, 0); m_rectH = createSpin(50, 0);
    f2->addRow("Центр X:", m_rectCX); f2->addRow("Центр Y:", m_rectCY); f2->addRow("Ширина:", m_rectW); f2->addRow("Высота:", m_rectH);
    stack->addWidget(p2);
    connect(m_rectMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), stack, &QStackedWidget::setCurrentIndex);
    l->addWidget(stack);
    m_rectChamfer = createSpin(0, 0, 50); auto* fCommon = new QFormLayout();
    fCommon->addRow("Радиус скруг.:", m_rectChamfer); l->addLayout(fCommon); return w;
}

QWidget* Properties::createEllipseWidget() {
    auto* w = new QWidget(); auto* f = new QFormLayout(w);
    m_ellCX = createSpin(); m_ellCY = createSpin(); m_ellRX = createSpin(60, 0); m_ellRY = createSpin(30, 0);
    f->addRow("Центр X:", m_ellCX); f->addRow("Центр Y:", m_ellCY); f->addRow("Радиус X:", m_ellRX); f->addRow("Радиус Y:", m_ellRY); return w;
}

QWidget* Properties::createPolygonWidget() {
    auto* w = new QWidget(); auto* f = new QFormLayout(w);
    m_polyCX = createSpin(); m_polyCY = createSpin(); m_polyR = createSpin(50, 0);
    m_polySides = new QSpinBox(); m_polySides->setRange(3, 100); m_polySides->setValue(5);
    m_polyInscribed = new QComboBox(); m_polyInscribed->addItem("Вписанный", true); m_polyInscribed->addItem("Описанный", false);
    f->addRow("Центр X:", m_polyCX); f->addRow("Центр Y:", m_polyCY); f->addRow("Радиус:", m_polyR);
    f->addRow("Сторон:", m_polySides); f->addRow("Тип:", m_polyInscribed); return w;
}

QWidget* Properties::createSplineWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    l->addWidget(new QLabel("Сплайн:\nРисование через точки\n(Заглушка для UI)")); return w;
}

// ИСПРАВЛЕНО: Возвращает QGroupBox*
QGroupBox* Properties::createStyleWidget() {
    auto* group = new QGroupBox("Стиль"); auto* layout = new QFormLayout(group);
    m_stylePresetButton = new QPushButton("Сплошная");
    m_stylePresetButton->setObjectName("StylePresetButton");
    connect(m_stylePresetButton, &QPushButton::clicked, this, &Properties::showStyleMenu);
    layout->addRow("Тип:", m_stylePresetButton);
    m_lineWidthSpin = new QDoubleSpinBox(); m_lineWidthSpin->setRange(0.1, 20); m_lineWidthSpin->setValue(0.8);
    layout->addRow("Толщина:", m_lineWidthSpin);
    m_colorButton = new QPushButton(); m_colorButton->setFixedSize(40, 20);
    m_colorButton->setObjectName("ColorPickerButton");
    m_colorButton->setStyleSheet("background-color: white; border: 1px solid gray;");
    connect(m_colorButton, &QPushButton::clicked, this, &Properties::onColorButtonClicked);
    layout->addRow("Цвет:", m_colorButton); return group;
}

void Properties::showCreationPropertiesFor(PrimitiveType type) {
    m_isCreationMode = true;
    m_activeType = type;
    m_currentObjects.clear();
    m_applyButton->setText("Создать"); m_applyButton->setProperty("state", "create");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show();
    if (type == PrimitiveType::Generic) {
        m_stack->setCurrentWidget(m_placeholderWidget); m_styleGroup->hide(); m_applyButton->hide();
    } else {
        m_stack->setCurrentWidget(m_primitiveWidgets[type]); m_styleGroup->show();
    }
}

void Properties::showEditingPropertiesFor(const std::vector<Object*>& objects) {
    if (objects.empty()) { showCreationPropertiesFor(m_activeType); return; }
    m_isCreationMode = false;
    m_currentObjects = objects;
    m_applyButton->setText("Обновить"); m_applyButton->setProperty("state", "update");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show(); m_styleGroup->show();

    PrimitiveType firstType = objects[0]->getType();
    bool allSame = true;
    for(auto* o : objects) if(o->getType() != firstType) allSame = false;
    if (allSame && objects.size() == 1) {
        m_stack->setCurrentWidget(m_primitiveWidgets[firstType]); populateFields(objects[0]);
    } else {
        m_stack->setCurrentWidget(m_placeholderWidget);
    }
    populateStyleFields(objects);
}

void Properties::populateFields(Object* obj) {
    if (auto* s = dynamic_cast<Segment*>(obj)) {
        m_segX1->setValue(s->getStart().getX()); m_segY1->setValue(s->getStart().getY());
        m_segX2->setValue(s->getEnd().getX()); m_segY2->setValue(s->getEnd().getY());
    }
    // Здесь можно добавить populate для остальных
}

void Properties::populateStyleFields(const std::vector<Object*>& objects) {
    if(objects.empty()) return;
    auto style = objects[0]->getLineStyle();
    m_lineWidthSpin->setValue(style.width);
    m_stylePresetButton->setText(style.name);
    m_selectedColor = objects[0]->getColor();
    m_colorButton->setStyleSheet(QString("background-color: %1").arg(m_selectedColor.name()));
}

void Properties::onApplyClicked() {
    if (m_isCreationMode) {
        std::unique_ptr<Object> newObj;

        switch (m_activeType) {
        case PrimitiveType::Segment:
            newObj = std::make_unique<Segment>(Point(m_segX1->value(), m_segY1->value()), Point(m_segX2->value(), m_segY2->value()));
            break;
        case PrimitiveType::Circle:
            if (m_circleMethodCombo->currentIndex() == 0) {
                newObj = std::make_unique<Circle>(Point(m_circCX->value(), m_circCY->value()), m_circR->value());
            } else {
                double x1=m_circP1X->value(), y1=m_circP1Y->value();
                double x2=m_circP2X->value(), y2=m_circP2Y->value();
                double cx=(x1+x2)/2, cy=(y1+y2)/2;
                double r = std::sqrt(std::pow(x2-x1,2)+std::pow(y2-y1,2))/2;
                newObj = std::make_unique<Circle>(Point(cx,cy), r);
            }
            break;
        case PrimitiveType::Arc:
            newObj = std::make_unique<Arc>(Point(m_arcCX->value(), m_arcCY->value()), m_arcR->value(), m_arcStart->value(), m_arcSpan->value());
            break;
        case PrimitiveType::Rectangle:
            if (m_rectMethodCombo->currentIndex() == 0) {
                double x1=m_rectP1X->value(), y1=m_rectP1Y->value();
                double x2=m_rectP2X->value(), y2=m_rectP2Y->value();
                // ИСПРАВЛЕНО: теперь Rectangle корректно наследуется от Object и имеет конструктор
                newObj = std::make_unique<Rectangle>(Point(std::min(x1,x2), std::min(y1,y2)), std::abs(x1-x2), std::abs(y1-y2), m_rectChamfer->value());
            } else {
                double w=m_rectW->value(), h=m_rectH->value();
                double cx=m_rectCX->value(), cy=m_rectCY->value();
                newObj = std::make_unique<Rectangle>(Point(cx - w/2, cy - h/2), w, h, m_rectChamfer->value());
            }
            break;
        case PrimitiveType::Ellipse:
            newObj = std::make_unique<Ellipse>(Point(m_ellCX->value(), m_ellCY->value()), m_ellRX->value(), m_ellRY->value());
            break;
        case PrimitiveType::Polygon:
            newObj = std::make_unique<PolygonObj>(Point(m_polyCX->value(), m_polyCY->value()), m_polyR->value(), m_polySides->value(), m_polyInscribed->currentData().toBool());
            break;
        case PrimitiveType::Spline:
        {
            std::vector<Point> pts = {Point(0,0), Point(50,50), Point(100,0)};
            newObj = std::make_unique<Spline>(pts);
        }
        break;
        default: break;
        }

        if (newObj) {
            LineStyle s = m_currentStyle; s.width = m_lineWidthSpin->value();
            newObj->setLineStyle(s);
            newObj->setColor(m_selectedColor);

            // Используем .release() и сырой указатель (Object*) для сигнала
            emit objectCreateRequested(newObj.release());
        }
    }
    else {
        // Режим редактирования (Update)
        for (auto* obj : m_currentObjects) {
            LineStyle s = m_currentStyle; s.width = m_lineWidthSpin->value();
            obj->setLineStyle(s);
            obj->setColor(m_selectedColor);
            // Тут можно добавить апдейт координат
        }
        emit objectsModified(m_currentObjects);
    }
}

void Properties::onColorButtonClicked() {
    QColor c = QColorDialog::getColor(m_selectedColor, this);
    if(c.isValid()) {
        m_selectedColor = c;
        m_colorButton->setStyleSheet(QString("background-color: %1").arg(c.name()));
    }
}

void Properties::showStyleMenu() {
    QMenu menu(this);
    for(const auto& s : m_availableStyles) {
        menu.addAction(s.name, this, [this, s](){
            m_currentStyle = s;
            m_stylePresetButton->setText(s.name);
        });
    }
    menu.addSeparator();
    menu.addAction("Добавить...", this, &Properties::onAddCustomStyle);
    menu.exec(QCursor::pos());
}

void Properties::onAddCustomStyle() {
    StyleDialog dlg(this);
    if(dlg.exec()) {
        m_availableStyles.push_back(dlg.getStyle());
    }
}

void Properties::setCoordinateSystem(CoordinateSystemType) {}
void Properties::updateAngleLabels() {}
