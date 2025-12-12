#include "Properties.h"
#include "Point.h"
#include "Segment.h"
#include "Circle.h"
#include "Arc.h"
#include "Rectangle.h"
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
#include <QtMath>

// --- MATH UTILS ---
namespace MathUtils {
// Вычисление центра окружности по 3 точкам
bool getCircleFrom3Points(const Point& p1, const Point& p2, const Point& p3, Point& center, double& radius) {
    double x1 = p1.getX(), y1 = p1.getY();
    double x2 = p2.getX(), y2 = p2.getY();
    double x3 = p3.getX(), y3 = p3.getY();

    double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (std::abs(D) < 1e-9) return false; // Точки на одной прямой

    double Ux = ((x1*x1 + y1*y1) * (y2 - y3) + (x2*x2 + y2*y2) * (y3 - y1) + (x3*x3 + y3*y3) * (y1 - y2)) / D;
    double Uy = ((x1*x1 + y1*y1) * (x3 - x2) + (x2*x2 + y2*y2) * (x1 - x3) + (x3*x3 + y3*y3) * (x2 - x1)) / D;

    center = Point(Ux, Uy);
    radius = std::sqrt(std::pow(Ux - x1, 2) + std::pow(Uy - y1, 2));
    return true;
}

double dist(const Point& p1, const Point& p2) {
    return std::sqrt(std::pow(p1.getX() - p2.getX(), 2) + std::pow(p1.getY() - p2.getY(), 2));
}
}

// --- HELPERS ---
static QDoubleSpinBox* createSpin(double val=0, double min=-10000, double max=10000) {
    auto* s = new QDoubleSpinBox(); s->setRange(min, max); s->setValue(val); s->setDecimals(2); return s;
}

static QLabel* createLbl(const QString& text) {
    return new QLabel(text);
}

Properties::Properties(QWidget *parent)
    : QWidget(parent), m_isCreationMode(true)
{
    this->setObjectName("PropertiesPanel");
    auto* mainLayout = new QVBoxLayout(this);

    m_stack = new QStackedWidget();
    m_placeholderWidget = createPlaceholder();
    m_stack->addWidget(m_placeholderWidget);

    // Создаем виджеты
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

// Помощник для чтения точки с учетом полярной системы
Point Properties::readPoint(QDoubleSpinBox* xBox, QDoubleSpinBox* yBox) const {
    double v1 = xBox->value(); // X или R
    double v2 = yBox->value(); // Y или Angle

    if (m_coordSystem == CoordinateSystemType::Polar) {
        // v1 = Radius, v2 = Angle
        Point p;
        p.setPolar(v1, v2); // AngleUnit учитывается внутри Point::setPolar если v2 в градусах
        return p;
    } else {
        return Point(v1, v2);
    }
}

void Properties::setCoordinateSystem(CoordinateSystemType type) {
    m_coordSystem = type;
    updateLabels();
}

void Properties::updateLabels() {
    QString l1 = (m_coordSystem == CoordinateSystemType::Cartesian) ? "X:" : "R:";
    QString l2 = (m_coordSystem == CoordinateSystemType::Cartesian) ? "Y:" : "A:";

    // Вспомогательная лямбда для списка меток
    auto updateList = [&](const std::vector<QLabel*>& list) {
        for(size_t i = 0; i < list.size(); i+=2) {
            if(i+1 < list.size()) {
                // Если текст метки содержит цифру (например "X1:"), сохраняем цифру
                QString old1 = list[i]->text();
                QString old2 = list[i+1]->text();

                // Простая эвристика: если есть цифра, добавляем её
                QString suffix1 = old1.contains("1") ? "1:" : (old1.contains("2") ? "2:" : (old1.contains("3") ? "3:" : ":"));
                QString suffix2 = old2.contains("1") ? "1:" : (old2.contains("2") ? "2:" : (old2.contains("3") ? "3:" : ":"));

                // Для центра (CX, CY)
                if (old1.contains("Центр")) {
                    list[i]->setText(QString("Центр %1").arg(l1.left(1)));
                    list[i+1]->setText(QString("Центр %1").arg(l2.left(1)));
                }
                else if (old1.contains("Точка")) {
                    // "Точка 1 X:" -> "Точка 1 R:"
                    QString prefix = old1.split(" ").first(); // "Точка"
                    QString num = old1.split(" ")[1]; // "1"
                    list[i]->setText(QString("%1 %2 %3").arg(prefix).arg(num).arg(l1.left(1)));
                    list[i+1]->setText(QString("%1 %2 %3").arg(prefix).arg(num).arg(l2.left(1)));
                }
                else {
                    // Простые метки X1: Y1:
                    list[i]->setText(l1.left(1) + suffix1);
                    list[i+1]->setText(l2.left(1) + suffix2);
                }
            }
        }
    };

    updateList(m_circleLabels);
    updateList(m_arcLabels);
    updateList(m_rectLabels);
    updateList(m_ellLabels);
    updateList(m_polyLabels);

    // Сегмент хранили отдельно
    m_lblSeg1X->setText(l1 + "1"); m_lblSeg1Y->setText(l2 + "1");
    m_lblSeg2X->setText(l1 + "2"); m_lblSeg2Y->setText(l2 + "2");
}

// ... Widget Creators ...

QWidget* Properties::createPlaceholder() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    auto* lbl = new QLabel("Нет выбора"); lbl->setAlignment(Qt::AlignCenter); lbl->setObjectName("PlaceholderLabel");
    l->addWidget(lbl); return w;
}

QWidget* Properties::createSegmentWidget() {
    auto* w = new QWidget(); auto* l = new QFormLayout(w);
    auto* g = new QGroupBox("Отрезок"); auto* gl = new QFormLayout(g);
    m_segX1 = createSpin(); m_segY1 = createSpin(); m_segX2 = createSpin(100); m_segY2 = createSpin(100);
    m_lblSeg1X = createLbl("X1:"); m_lblSeg1Y = createLbl("Y1:");
    m_lblSeg2X = createLbl("X2:"); m_lblSeg2Y = createLbl("Y2:");
    gl->addRow(m_lblSeg1X, m_segX1); gl->addRow(m_lblSeg1Y, m_segY1);
    gl->addRow(m_lblSeg2X, m_segX2); gl->addRow(m_lblSeg2Y, m_segY2);
    l->addWidget(g); return w;
}

QWidget* Properties::createCircleWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    m_circleMethodCombo = new QComboBox();
    m_circleMethodCombo->addItem("Центр и Радиус");  // 0
    m_circleMethodCombo->addItem("Центр и Диаметр"); // 1
    m_circleMethodCombo->addItem("Две точки");       // 2
    m_circleMethodCombo->addItem("Три точки");       // 3
    l->addWidget(new QLabel("Метод:")); l->addWidget(m_circleMethodCombo);

    m_circleStack = new QStackedWidget();

    // 0: Center + Radius
    auto* p0 = new QWidget(); auto* f0 = new QFormLayout(p0);
    m_circCX = createSpin(); m_circCY = createSpin(); m_circR = createSpin(50, 0);
    auto* lcx = createLbl("Центр X:"); auto* lcy = createLbl("Центр Y:");
    f0->addRow(lcx, m_circCX); f0->addRow(lcy, m_circCY); f0->addRow("Радиус:", m_circR);
    m_circleLabels.push_back(lcx); m_circleLabels.push_back(lcy);
    m_circleStack->addWidget(p0);

    // 1: Center + Diameter
    auto* p1 = new QWidget(); auto* f1 = new QFormLayout(p1);
    m_circCX_D = createSpin(); m_circCY_D = createSpin(); m_circD = createSpin(100, 0);
    auto* lcx_d = createLbl("Центр X:"); auto* lcy_d = createLbl("Центр Y:");
    f1->addRow(lcx_d, m_circCX_D); f1->addRow(lcy_d, m_circCY_D); f1->addRow("Диаметр:", m_circD);
    m_circleLabels.push_back(lcx_d); m_circleLabels.push_back(lcy_d);
    m_circleStack->addWidget(p1);

    // 2: Two Points
    auto* p2 = new QWidget(); auto* f2 = new QFormLayout(p2);
    m_circ2P1X = createSpin(); m_circ2P1Y = createSpin();
    m_circ2P2X = createSpin(100); m_circ2P2Y = createSpin(100);
    auto* l2p1x = createLbl("Точка 1 X:"); auto* l2p1y = createLbl("Точка 1 Y:");
    auto* l2p2x = createLbl("Точка 2 X:"); auto* l2p2y = createLbl("Точка 2 Y:");
    f2->addRow(l2p1x, m_circ2P1X); f2->addRow(l2p1y, m_circ2P1Y);
    f2->addRow(l2p2x, m_circ2P2X); f2->addRow(l2p2y, m_circ2P2Y);
    m_circleLabels.push_back(l2p1x); m_circleLabels.push_back(l2p1y);
    m_circleLabels.push_back(l2p2x); m_circleLabels.push_back(l2p2y);
    m_circleStack->addWidget(p2);

    // 3: Three Points
    auto* p3 = new QWidget(); auto* f3 = new QFormLayout(p3);
    m_circ3P1X = createSpin(); m_circ3P1Y = createSpin();
    m_circ3P2X = createSpin(50); m_circ3P2Y = createSpin(50);
    m_circ3P3X = createSpin(100); m_circ3P3Y = createSpin(0);
    auto* l3p1x = createLbl("Точка 1 X:"); auto* l3p1y = createLbl("Точка 1 Y:");
    auto* l3p2x = createLbl("Точка 2 X:"); auto* l3p2y = createLbl("Точка 2 Y:");
    auto* l3p3x = createLbl("Точка 3 X:"); auto* l3p3y = createLbl("Точка 3 Y:");
    f3->addRow(l3p1x, m_circ3P1X); f3->addRow(l3p1y, m_circ3P1Y);
    f3->addRow(l3p2x, m_circ3P2X); f3->addRow(l3p2y, m_circ3P2Y);
    f3->addRow(l3p3x, m_circ3P3X); f3->addRow(l3p3y, m_circ3P3Y);
    m_circleLabels.push_back(l3p1x); m_circleLabels.push_back(l3p1y);
    m_circleLabels.push_back(l3p2x); m_circleLabels.push_back(l3p2y);
    m_circleLabels.push_back(l3p3x); m_circleLabels.push_back(l3p3y);
    m_circleStack->addWidget(p3);

    connect(m_circleMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_circleStack, &QStackedWidget::setCurrentIndex);
    l->addWidget(m_circleStack);
    return w;
}

QWidget* Properties::createArcWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    m_arcMethodCombo = new QComboBox();
    m_arcMethodCombo->addItem("Центр, Угол"); // 0
    m_arcMethodCombo->addItem("Три точки");   // 1
    l->addWidget(new QLabel("Метод:")); l->addWidget(m_arcMethodCombo);
    m_arcStack = new QStackedWidget();

    // 0: Center + Angle
    auto* p0 = new QWidget(); auto* f0 = new QFormLayout(p0);
    m_arcCX = createSpin(); m_arcCY = createSpin(); m_arcR = createSpin(50, 0);
    m_arcStart = createSpin(0, -360, 360); m_arcSpan = createSpin(90, -360, 360);
    auto* acx = createLbl("Центр X:"); auto* acy = createLbl("Центр Y:");
    f0->addRow(acx, m_arcCX); f0->addRow(acy, m_arcCY); f0->addRow("Радиус:", m_arcR);
    f0->addRow("Начало (°):", m_arcStart); f0->addRow("Угол (°):", m_arcSpan);
    m_arcLabels.push_back(acx); m_arcLabels.push_back(acy);
    m_arcStack->addWidget(p0);

    // 1: 3 Points
    auto* p1 = new QWidget(); auto* f1 = new QFormLayout(p1);
    m_arc3P1X = createSpin(); m_arc3P1Y = createSpin();
    m_arc3P2X = createSpin(50); m_arc3P2Y = createSpin(25);
    m_arc3P3X = createSpin(100); m_arc3P3Y = createSpin(0);
    auto* a3p1x = createLbl("Начало X:"); auto* a3p1y = createLbl("Начало Y:");
    auto* a3p2x = createLbl("Середина X:"); auto* a3p2y = createLbl("Середина Y:");
    auto* a3p3x = createLbl("Конец X:"); auto* a3p3y = createLbl("Конец Y:");
    f1->addRow(a3p1x, m_arc3P1X); f1->addRow(a3p1y, m_arc3P1Y);
    f1->addRow(a3p2x, m_arc3P2X); f1->addRow(a3p2y, m_arc3P2Y);
    f1->addRow(a3p3x, m_arc3P3X); f1->addRow(a3p3y, m_arc3P3Y);
    m_arcLabels.push_back(a3p1x); m_arcLabels.push_back(a3p1y);
    m_arcLabels.push_back(a3p2x); m_arcLabels.push_back(a3p2y);
    m_arcLabels.push_back(a3p3x); m_arcLabels.push_back(a3p3y);
    m_arcStack->addWidget(p1);

    connect(m_arcMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_arcStack, &QStackedWidget::setCurrentIndex);
    l->addWidget(m_arcStack);
    return w;
}

QWidget* Properties::createRectangleWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    m_rectMethodCombo = new QComboBox();
    m_rectMethodCombo->addItem("Две точки");       // 0
    m_rectMethodCombo->addItem("Точка и Размер");  // 1
    m_rectMethodCombo->addItem("Центр и Размер");  // 2
    l->addWidget(new QLabel("Метод:")); l->addWidget(m_rectMethodCombo);
    m_rectStack = new QStackedWidget();

    // 0: 2 Points
    auto* p0 = new QWidget(); auto* f0 = new QFormLayout(p0);
    m_rectP1X = createSpin(); m_rectP1Y = createSpin();
    m_rectP2X = createSpin(100); m_rectP2Y = createSpin(100);
    auto* r2p1x = createLbl("Точка 1 X:"); auto* r2p1y = createLbl("Точка 1 Y:");
    auto* r2p2x = createLbl("Точка 2 X:"); auto* r2p2y = createLbl("Точка 2 Y:");
    f0->addRow(r2p1x, m_rectP1X); f0->addRow(r2p1y, m_rectP1Y);
    f0->addRow(r2p2x, m_rectP2X); f0->addRow(r2p2y, m_rectP2Y);
    m_rectLabels.push_back(r2p1x); m_rectLabels.push_back(r2p1y);
    m_rectLabels.push_back(r2p2x); m_rectLabels.push_back(r2p2y);
    m_rectStack->addWidget(p0);

    // 1: Point + Size
    auto* p1 = new QWidget(); auto* f1 = new QFormLayout(p1);
    m_rect1PX = createSpin(); m_rect1PY = createSpin();
    m_rect1W = createSpin(100, 0); m_rect1H = createSpin(50, 0);
    auto* r1px = createLbl("Точка X:"); auto* r1py = createLbl("Точка Y:");
    f1->addRow(r1px, m_rect1PX); f1->addRow(r1py, m_rect1PY);
    f1->addRow("Ширина:", m_rect1W); f1->addRow("Высота:", m_rect1H);
    m_rectLabels.push_back(r1px); m_rectLabels.push_back(r1py);
    m_rectStack->addWidget(p1);

    // 2: Center + Size
    auto* p2 = new QWidget(); auto* f2 = new QFormLayout(p2);
    m_rectCX = createSpin(); m_rectCY = createSpin();
    m_rectCW = createSpin(100, 0); m_rectCH = createSpin(50, 0);
    auto* rcx = createLbl("Центр X:"); auto* rcy = createLbl("Центр Y:");
    f2->addRow(rcx, m_rectCX); f2->addRow(rcy, m_rectCY);
    f2->addRow("Ширина:", m_rectCW); f2->addRow("Высота:", m_rectCH);
    m_rectLabels.push_back(rcx); m_rectLabels.push_back(rcy);
    m_rectStack->addWidget(p2);

    connect(m_rectMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_rectStack, &QStackedWidget::setCurrentIndex);
    l->addWidget(m_rectStack);

    // Фаска
    m_rectChamfer = createSpin(0, 0, 50);
    auto* fCommon = new QFormLayout();
    fCommon->addRow("Радиус скруг.:", m_rectChamfer);
    l->addLayout(fCommon);
    return w;
}

QWidget* Properties::createEllipseWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    m_ellMethodCombo = new QComboBox();
    m_ellMethodCombo->addItem("Центр и Радиусы");  // 0
    m_ellMethodCombo->addItem("Центр и Оси");      // 1
    l->addWidget(new QLabel("Метод:")); l->addWidget(m_ellMethodCombo);
    m_ellStack = new QStackedWidget();

    // 0: Center + Radii
    auto* p0 = new QWidget(); auto* f0 = new QFormLayout(p0);
    m_ellCX = createSpin(); m_ellCY = createSpin();
    m_ellRX = createSpin(60, 0); m_ellRY = createSpin(30, 0);
    auto* ecx = createLbl("Центр X:"); auto* ecy = createLbl("Центр Y:");
    f0->addRow(ecx, m_ellCX); f0->addRow(ecy, m_ellCY);
    f0->addRow("Радиус X:", m_ellRX); f0->addRow("Радиус Y:", m_ellRY);
    m_ellLabels.push_back(ecx); m_ellLabels.push_back(ecy);
    m_ellStack->addWidget(p0);

    // 1: Center + Axis Points
    auto* p1 = new QWidget(); auto* f1 = new QFormLayout(p1);
    m_ell2CX = createSpin(); m_ell2CY = createSpin();
    m_ell2P1X = createSpin(50); m_ell2P1Y = createSpin(0); // Ось 1 конец
    m_ell2P2X = createSpin(0); m_ell2P2Y = createSpin(30); // Ось 2 конец
    auto* e2cx = createLbl("Центр X:"); auto* e2cy = createLbl("Центр Y:");
    auto* e2p1x = createLbl("Ось 1 X:"); auto* e2p1y = createLbl("Ось 1 Y:");
    auto* e2p2x = createLbl("Ось 2 X:"); auto* e2p2y = createLbl("Ось 2 Y:");
    f1->addRow(e2cx, m_ell2CX); f1->addRow(e2cy, m_ell2CY);
    f1->addRow(e2p1x, m_ell2P1X); f1->addRow(e2p1y, m_ell2P1Y);
    f1->addRow(e2p2x, m_ell2P2X); f1->addRow(e2p2y, m_ell2P2Y);
    m_ellLabels.push_back(e2cx); m_ellLabels.push_back(e2cy);
    m_ellLabels.push_back(e2p1x); m_ellLabels.push_back(e2p1y);
    m_ellLabels.push_back(e2p2x); m_ellLabels.push_back(e2p2y);
    m_ellStack->addWidget(p1);

    connect(m_ellMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_ellStack, &QStackedWidget::setCurrentIndex);
    l->addWidget(m_ellStack);
    return w;
}

QWidget* Properties::createPolygonWidget() {
    auto* w = new QWidget(); auto* f = new QFormLayout(w);
    m_polyCX = createSpin(); m_polyCY = createSpin(); m_polyR = createSpin(50, 0);
    m_polySides = new QSpinBox(); m_polySides->setRange(3, 100); m_polySides->setValue(5);
    m_polyInscribed = new QComboBox(); m_polyInscribed->addItem("Вписанный", true); m_polyInscribed->addItem("Описанный", false);
    auto* pcx = createLbl("Центр X:"); auto* pcy = createLbl("Центр Y:");
    f->addRow(pcx, m_polyCX); f->addRow(pcy, m_polyCY);
    f->addRow("Радиус:", m_polyR);
    f->addRow("Сторон:", m_polySides); f->addRow("Тип:", m_polyInscribed);
    m_polyLabels.push_back(pcx); m_polyLabels.push_back(pcy);
    return w;
}

QWidget* Properties::createSplineWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    l->addWidget(new QLabel("Сплайн:\nРисование мышкой\n(Контрольные точки)")); return w;
}

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

void Properties::showCreationPropertiesFor(PrimitiveType type, int methodIndex) {
    m_isCreationMode = true;
    m_activeType = type;
    m_currentObjects.clear();
    m_applyButton->setText("Создать"); m_applyButton->setProperty("state", "create");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show();

    if (type == PrimitiveType::Generic) {
        m_stack->setCurrentWidget(m_placeholderWidget); m_styleGroup->hide(); m_applyButton->hide();
    } else {
        m_stack->setCurrentWidget(m_primitiveWidgets[type]);
        m_styleGroup->show();

        // Установка активного метода в комбобоксе
        if (type == PrimitiveType::Circle) m_circleMethodCombo->setCurrentIndex(methodIndex);
        if (type == PrimitiveType::Arc) m_arcMethodCombo->setCurrentIndex(methodIndex);
        if (type == PrimitiveType::Rectangle) m_rectMethodCombo->setCurrentIndex(methodIndex);
        if (type == PrimitiveType::Ellipse) m_ellMethodCombo->setCurrentIndex(methodIndex);
    }
}

void Properties::showEditingPropertiesFor(const std::vector<Object*>& objects) {
    // ... (без изменений логики редактирования, только скрытие лишних комбобоксов если нужно)
    // Для простоты оставляем как есть, в режиме редактирования обычно показывается только геометрия
    if (objects.empty()) { showCreationPropertiesFor(m_activeType, 0); return; }
    m_isCreationMode = false;
    m_currentObjects = objects;
    m_applyButton->setText("Обновить"); m_applyButton->setProperty("state", "update");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show(); m_styleGroup->show();

    PrimitiveType firstType = objects[0]->getType();
    if (objects.size() == 1) {
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
    // Добавить остальных при необходимости
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
            newObj = std::make_unique<Segment>(readPoint(m_segX1, m_segY1), readPoint(m_segX2, m_segY2));
            break;

        case PrimitiveType::Circle: {
            int method = m_circleMethodCombo->currentIndex();
            if (method == 0) { // Center + Radius
                newObj = std::make_unique<Circle>(readPoint(m_circCX, m_circCY), m_circR->value());
            } else if (method == 1) { // Center + Diameter
                newObj = std::make_unique<Circle>(readPoint(m_circCX_D, m_circCY_D), m_circD->value() / 2.0);
            } else if (method == 2) { // 2 Points (Diameter)
                Point p1 = readPoint(m_circ2P1X, m_circ2P1Y);
                Point p2 = readPoint(m_circ2P2X, m_circ2P2Y);
                double cx = (p1.getX() + p2.getX()) / 2.0;
                double cy = (p1.getY() + p2.getY()) / 2.0;
                double r = MathUtils::dist(p1, p2) / 2.0;
                newObj = std::make_unique<Circle>(Point(cx, cy), r);
            } else if (method == 3) { // 3 Points
                Point p1 = readPoint(m_circ3P1X, m_circ3P1Y);
                Point p2 = readPoint(m_circ3P2X, m_circ3P2Y);
                Point p3 = readPoint(m_circ3P3X, m_circ3P3Y);
                Point c; double r;
                if (MathUtils::getCircleFrom3Points(p1, p2, p3, c, r)) {
                    newObj = std::make_unique<Circle>(c, r);
                }
            }
            break;
        }

        case PrimitiveType::Arc: {
            int method = m_arcMethodCombo->currentIndex();
            if (method == 0) { // Center + Angles
                newObj = std::make_unique<Arc>(readPoint(m_arcCX, m_arcCY), m_arcR->value(), m_arcStart->value(), m_arcSpan->value());
            } else { // 3 Points
                Point p1 = readPoint(m_arc3P1X, m_arc3P1Y); // Start
                Point p2 = readPoint(m_arc3P2X, m_arc3P2Y); // Mid
                Point p3 = readPoint(m_arc3P3X, m_arc3P3Y); // End
                Point c; double r;
                if (MathUtils::getCircleFrom3Points(p1, p2, p3, c, r)) {
                    // Calculate angles
                    double aStart = std::atan2(p1.getY() - c.getY(), p1.getX() - c.getX()) * 180 / M_PI;
                    double aMid = std::atan2(p2.getY() - c.getY(), p2.getX() - c.getX()) * 180 / M_PI;
                    double aEnd = std::atan2(p3.getY() - c.getY(), p3.getX() - c.getX()) * 180 / M_PI;

                    // Normalize
                    if (aStart < 0) aStart += 360; if (aMid < 0) aMid += 360; if (aEnd < 0) aEnd += 360;

                    double span = aEnd - aStart;
                    // Check logic for direction (mid point must be inside)
                    // Simplified logic: assume CCW. If Mid is not in [Start, Start+Span], then subtract 360 or similar.
                    // For robust CAD, need vector cross products.
                    // Simple fix:
                    double tempEnd = (aEnd < aStart) ? aEnd + 360 : aEnd;
                    double tempMid = (aMid < aStart) ? aMid + 360 : aMid;

                    if (tempMid > aStart && tempMid < tempEnd) {
                        span = tempEnd - aStart;
                    } else {
                        // CW direction or wrap around
                        span = tempEnd - aStart - 360;
                        if (span < -360) span += 360;
                    }

                    newObj = std::make_unique<Arc>(c, r, aStart, span);
                }
            }
            break;
        }

        case PrimitiveType::Rectangle: {
            int method = m_rectMethodCombo->currentIndex();
            double r = m_rectChamfer->value();
            if (method == 0) { // 2 Points
                Point p1 = readPoint(m_rectP1X, m_rectP1Y);
                Point p2 = readPoint(m_rectP2X, m_rectP2Y);
                double x = std::min(p1.getX(), p2.getX());
                double y = std::min(p1.getY(), p2.getY());
                double w = std::abs(p1.getX() - p2.getX());
                double h = std::abs(p1.getY() - p2.getY());
                newObj = std::make_unique<Rectangle>(Point(x, y), w, h, r);
            } else if (method == 1) { // Point + Size
                Point p = readPoint(m_rect1PX, m_rect1PY);
                newObj = std::make_unique<Rectangle>(p, m_rect1W->value(), m_rect1H->value(), r);
            } else { // Center + Size
                Point c = readPoint(m_rectCX, m_rectCY);
                double w = m_rectCW->value();
                double h = m_rectCH->value();
                newObj = std::make_unique<Rectangle>(Point(c.getX() - w/2, c.getY() - h/2), w, h, r);
            }
            break;
        }

        case PrimitiveType::Ellipse: {
            int method = m_ellMethodCombo->currentIndex();
            if (method == 0) { // Center + Radii
                newObj = std::make_unique<Ellipse>(readPoint(m_ellCX, m_ellCY), m_ellRX->value(), m_ellRY->value());
            } else { // Center + Axis Points
                Point c = readPoint(m_ell2CX, m_ell2CY);
                Point p1 = readPoint(m_ell2P1X, m_ell2P1Y);
                Point p2 = readPoint(m_ell2P2X, m_ell2P2Y);
                // Simple axis aligned calculation based on distance
                double rx = MathUtils::dist(c, p1);
                double ry = MathUtils::dist(c, p2);
                newObj = std::make_unique<Ellipse>(c, rx, ry);
            }
            break;
        }

        case PrimitiveType::Polygon:
            newObj = std::make_unique<PolygonObj>(readPoint(m_polyCX, m_polyCY), m_polyR->value(), m_polySides->value(), m_polyInscribed->currentData().toBool());
            break;

        case PrimitiveType::Spline: {
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
            emit objectCreateRequested(newObj.release());
        }
    }
    else {
        // Режим редактирования
        for (auto* obj : m_currentObjects) {
            LineStyle s = m_currentStyle; s.width = m_lineWidthSpin->value();
            obj->setLineStyle(s);
            obj->setColor(m_selectedColor);
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
