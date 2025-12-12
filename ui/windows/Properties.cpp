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
#include <QGridLayout>
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
#include <QScrollArea>
#include <cmath>
#include <QtMath>

// --- MATH UTILS ---
namespace MathUtils {
bool getCircleFrom3Points(const Point& p1, const Point& p2, const Point& p3, Point& center, double& radius) {
    double x1 = p1.getX(), y1 = p1.getY();
    double x2 = p2.getX(), y2 = p2.getY();
    double x3 = p3.getX(), y3 = p3.getY();
    double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (std::abs(D) < 1e-9) return false;
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
static QDoubleSpinBox* createSpin(double val=0, double min=-100000, double max=100000) {
    auto* s = new QDoubleSpinBox(); s->setRange(min, max); s->setValue(val); s->setDecimals(2);
    s->setFocusPolicy(Qt::ClickFocus);
    return s;
}

static QLabel* createLbl(const QString& text) {
    auto* l = new QLabel(text);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return l;
}

Properties::Properties(QWidget *parent)
    : QWidget(parent), m_isCreationMode(true)
{
    this->setObjectName("PropertiesPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_scrollContent = new QWidget();
    auto* contentLayout = new QVBoxLayout(m_scrollContent);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(15);

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
    contentLayout->addWidget(m_stack);

    m_styleGroup = createStyleWidget();
    contentLayout->addWidget(m_styleGroup);

    contentLayout->addStretch();
    m_scrollArea->setWidget(m_scrollContent);
    mainLayout->addWidget(m_scrollArea);

    m_applyButton = new QPushButton("Создать");
    m_applyButton->setObjectName("ApplyButton");
    m_applyButton->setMinimumHeight(40);

    auto* btnContainer = new QWidget();
    auto* btnLayout = new QVBoxLayout(btnContainer);
    btnLayout->setContentsMargins(8, 0, 8, 8);
    btnLayout->addWidget(m_applyButton);
    mainLayout->addWidget(btnContainer);

    connect(m_applyButton, &QPushButton::clicked, this, &Properties::onApplyClicked);

    // --- ИСПРАВЛЕНИЕ: Добавлены все стили ---
    m_availableStyles = {
        {LineStyleType::SolidMain,     "Сплошная основная",      2.0, 0, 0, true},
        {LineStyleType::SolidThin,     "Сплошная тонкая",        1.0, 0, 0, true},
        {LineStyleType::SolidWavy,     "Сплошная волнистая",     1.0, 0, 0, true},
        {LineStyleType::SolidZigZag,   "Сплошная с изломами",    1.0, 0, 0, true},
        {LineStyleType::Dashed,        "Штриховая",              1.0, 4.0, 2.0, true},
        {LineStyleType::DashDotThin,   "Штрихпунктирная тонкая", 1.0, 10.0, 3.0, true},
        {LineStyleType::DashDotThick,  "Штрихпунктирная толстая",2.0, 10.0, 3.0, true},
        {LineStyleType::DashDotDot,    "С двумя точками",        1.0, 10.0, 3.0, true}
    };
    m_currentStyle = m_availableStyles[0];

    showCreationPropertiesFor(PrimitiveType::Generic);
}

// ... Остальной код createSegmentWidget, createCircleWidget и т.д. без изменений ...
// Для краткости я привожу только измененные части и методы setup

QGridLayout* Properties::setupGridLayout(QGroupBox* group) {
    auto* grid = new QGridLayout(group);
    grid->setContentsMargins(8, 12, 8, 8);
    grid->setSpacing(10);
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 0);
    grid->setColumnStretch(3, 1);
    return grid;
}

void Properties::addRow(QGridLayout* layout, int row, QLabel* l1, QWidget* w1, QLabel* l2, QWidget* w2) {
    layout->addWidget(l1, row, 0);
    layout->addWidget(w1, row, 1);
    if (l2 && w2) {
        layout->addWidget(l2, row, 2);
        layout->addWidget(w2, row, 3);
    }
}

Point Properties::readPoint(QDoubleSpinBox* xBox, QDoubleSpinBox* yBox) const {
    double v1 = xBox->value();
    double v2 = yBox->value();
    if (m_coordSystem == CoordinateSystemType::Polar) {
        Point p; p.setPolar(v1, v2); return p;
    }
    return Point(v1, v2);
}

void Properties::setCoordinateSystem(CoordinateSystemType type) {
    m_coordSystem = type;
    updateLabels();
}

void Properties::updateLabels() {
    QString l1 = (m_coordSystem == CoordinateSystemType::Cartesian) ? "X:" : "R:";
    QString l2 = (m_coordSystem == CoordinateSystemType::Cartesian) ? "Y:" : "A:";

    auto updateList = [&](const std::vector<QLabel*>& list) {
        for(size_t i = 0; i < list.size(); i+=2) {
            if(i+1 < list.size()) {
                list[i]->setText(l1);
                list[i+1]->setText(l2);
            }
        }
    };

    updateList(m_coordLabels[PrimitiveType::Circle]);
    updateList(m_coordLabels[PrimitiveType::Arc]);
    updateList(m_coordLabels[PrimitiveType::Rectangle]);
    updateList(m_coordLabels[PrimitiveType::Ellipse]);
    updateList(m_coordLabels[PrimitiveType::Polygon]);
    updateList(m_coordLabels[PrimitiveType::Spline]);

    m_lblSegStartX->setText(l1); m_lblSegStartY->setText(l2);
    m_lblSegEndX->setText(l1);   m_lblSegEndY->setText(l2);
}

// ... Create Widgets Methods (createSegmentWidget etc) - assume same as before ...
// Вставлять сюда код создания виджетов из предыдущего Properties.cpp, он верен.
QWidget* Properties::createPlaceholder() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w);
    auto* lbl = new QLabel("Выберите инструмент\nили объект");
    lbl->setAlignment(Qt::AlignCenter); lbl->setObjectName("PlaceholderLabel");
    l->addWidget(lbl); return w;
}

QWidget* Properties::createSegmentWidget() {
    auto* gb = new QGroupBox("Отрезок");
    auto* gl = setupGridLayout(gb);
    m_segX1 = createSpin(); m_segY1 = createSpin();
    m_segX2 = createSpin(100); m_segY2 = createSpin(100);
    m_lblSegStartX = createLbl("X:"); m_lblSegStartY = createLbl("Y:");
    m_lblSegEndX = createLbl("X:");   m_lblSegEndY = createLbl("Y:");
    auto* titleStart = new QLabel("Начало:");
    titleStart->setStyleSheet("font-weight: bold; color: #F92672;");
    gl->addWidget(titleStart, 0, 0, 1, 4);
    addRow(gl, 1, m_lblSegStartX, m_segX1, m_lblSegStartY, m_segY1);
    auto* titleEnd = new QLabel("Конец:");
    titleEnd->setStyleSheet("font-weight: bold; color: #F92672;");
    gl->addWidget(titleEnd, 2, 0, 1, 4);
    addRow(gl, 3, m_lblSegEndX, m_segX2, m_lblSegEndY, m_segY2);
    return gb;
}

QWidget* Properties::createCircleWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w); l->setContentsMargins(0,0,0,0);
    auto* gb = new QGroupBox("Окружность"); auto* gl = new QVBoxLayout(gb); gl->setSpacing(10);
    m_circleMethodCombo = new QComboBox();
    m_circleMethodCombo->addItem("Центр и Радиус");
    m_circleMethodCombo->addItem("Центр и Диаметр");
    m_circleMethodCombo->addItem("Две точки (Диаметр)");
    m_circleMethodCombo->addItem("Три точки");
    auto* methodLayout = new QHBoxLayout();
    methodLayout->addWidget(createLbl("Метод:")); methodLayout->addWidget(m_circleMethodCombo);
    gl->addLayout(methodLayout);
    m_circleStack = new QStackedWidget();
    auto makePage = [&](QGridLayout*& grid) -> QWidget* {
        auto* p = new QWidget(); grid = new QGridLayout(p);
        grid->setContentsMargins(0,0,0,0); grid->setSpacing(10);
        grid->setColumnStretch(0,0); grid->setColumnStretch(1,1);
        grid->setColumnStretch(2,0); grid->setColumnStretch(3,1); return p;
    };
    QGridLayout *g0, *g1, *g2, *g3;
    m_circleStack->addWidget(makePage(g0));
    m_circCX = createSpin(); m_circCY = createSpin(); m_circR = createSpin(50, 0);
    auto l0x = createLbl("X:"); auto l0y = createLbl("Y:");
    addRow(g0, 0, l0x, m_circCX, l0y, m_circCY); addRow(g0, 1, createLbl("R:"), m_circR);
    m_coordLabels[PrimitiveType::Circle] = {l0x, l0y};
    m_circleStack->addWidget(makePage(g1));
    m_circCX_D = createSpin(); m_circCY_D = createSpin(); m_circD = createSpin(100, 0);
    auto l1x = createLbl("X:"); auto l1y = createLbl("Y:");
    addRow(g1, 0, l1x, m_circCX_D, l1y, m_circCY_D); addRow(g1, 1, createLbl("D:"), m_circD);
    m_coordLabels[PrimitiveType::Circle].insert(m_coordLabels[PrimitiveType::Circle].end(), {l1x, l1y});
    m_circleStack->addWidget(makePage(g2));
    m_circ2P1X = createSpin(); m_circ2P1Y = createSpin(); m_circ2P2X = createSpin(100); m_circ2P2Y = createSpin(100);
    auto l2p1x = createLbl("X:"); auto l2p1y = createLbl("Y:"); auto l2p2x = createLbl("X:"); auto l2p2y = createLbl("Y:");
    g2->addWidget(new QLabel("Точка 1:"), 0, 0, 1, 4); addRow(g2, 1, l2p1x, m_circ2P1X, l2p1y, m_circ2P1Y);
    g2->addWidget(new QLabel("Точка 2:"), 2, 0, 1, 4); addRow(g2, 3, l2p2x, m_circ2P2X, l2p2y, m_circ2P2Y);
    m_coordLabels[PrimitiveType::Circle].insert(m_coordLabels[PrimitiveType::Circle].end(), {l2p1x, l2p1y, l2p2x, l2p2y});
    m_circleStack->addWidget(makePage(g3));
    m_circ3P1X = createSpin(); m_circ3P1Y = createSpin(); m_circ3P2X = createSpin(50); m_circ3P2Y = createSpin(50); m_circ3P3X = createSpin(100); m_circ3P3Y = createSpin(0);
    auto l3p1x = createLbl("X:"); auto l3p1y = createLbl("Y:"); auto l3p2x = createLbl("X:"); auto l3p2y = createLbl("Y:"); auto l3p3x = createLbl("X:"); auto l3p3y = createLbl("Y:");
    g3->addWidget(new QLabel("Точка 1:"), 0, 0, 1, 4); addRow(g3, 1, l3p1x, m_circ3P1X, l3p1y, m_circ3P1Y);
    g3->addWidget(new QLabel("Точка 2:"), 2, 0, 1, 4); addRow(g3, 3, l3p2x, m_circ3P2X, l3p2y, m_circ3P2Y);
    g3->addWidget(new QLabel("Точка 3:"), 4, 0, 1, 4); addRow(g3, 5, l3p3x, m_circ3P3X, l3p3y, m_circ3P3Y);
    m_coordLabels[PrimitiveType::Circle].insert(m_coordLabels[PrimitiveType::Circle].end(), {l3p1x, l3p1y, l3p2x, l3p2y, l3p3x, l3p3y});
    gl->addWidget(m_circleStack);
    connect(m_circleMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_circleStack, &QStackedWidget::setCurrentIndex);
    l->addWidget(gb); return w;
}

QWidget* Properties::createArcWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w); l->setContentsMargins(0,0,0,0);
    auto* gb = new QGroupBox("Дуга"); auto* gl = new QVBoxLayout(gb); gl->setSpacing(10);
    m_arcMethodCombo = new QComboBox();
    m_arcMethodCombo->addItem("Центр, Угол"); m_arcMethodCombo->addItem("Три точки");
    auto* methodLayout = new QHBoxLayout(); methodLayout->addWidget(createLbl("Метод:")); methodLayout->addWidget(m_arcMethodCombo); gl->addLayout(methodLayout);
    m_arcStack = new QStackedWidget();
    auto makePage = [&](QGridLayout*& grid) -> QWidget* { auto* p = new QWidget(); grid = new QGridLayout(p); grid->setContentsMargins(0,0,0,0); grid->setSpacing(10); grid->setColumnStretch(0,0); grid->setColumnStretch(1,1); grid->setColumnStretch(2,0); grid->setColumnStretch(3,1); return p; };
    QGridLayout *g0, *g1;
    m_arcStack->addWidget(makePage(g0));
    m_arcCX = createSpin(); m_arcCY = createSpin(); m_arcR = createSpin(50, 0); m_arcStart = createSpin(0, -360, 360); m_arcSpan = createSpin(90, -360, 360);
    auto acx = createLbl("X:"); auto acy = createLbl("Y:");
    addRow(g0, 0, acx, m_arcCX, acy, m_arcCY); addRow(g0, 1, createLbl("R:"), m_arcR); addRow(g0, 2, createLbl("Нач:"), m_arcStart, createLbl("Угол:"), m_arcSpan);
    m_coordLabels[PrimitiveType::Arc] = {acx, acy};
    m_arcStack->addWidget(makePage(g1));
    m_arc3P1X = createSpin(); m_arc3P1Y = createSpin(); m_arc3P2X = createSpin(50); m_arc3P2Y = createSpin(25); m_arc3P3X = createSpin(100); m_arc3P3Y = createSpin(0);
    auto a3p1x = createLbl("X:"); auto a3p1y = createLbl("Y:"); auto a3p2x = createLbl("X:"); auto a3p2y = createLbl("Y:"); auto a3p3x = createLbl("X:"); auto a3p3y = createLbl("Y:");
    g1->addWidget(new QLabel("Начало:"), 0, 0, 1, 4); addRow(g1, 1, a3p1x, m_arc3P1X, a3p1y, m_arc3P1Y);
    g1->addWidget(new QLabel("Середина:"), 2, 0, 1, 4); addRow(g1, 3, a3p2x, m_arc3P2X, a3p2y, m_arc3P2Y);
    g1->addWidget(new QLabel("Конец:"), 4, 0, 1, 4); addRow(g1, 5, a3p3x, m_arc3P3X, a3p3y, m_arc3P3Y);
    m_coordLabels[PrimitiveType::Arc].insert(m_coordLabels[PrimitiveType::Arc].end(), {a3p1x, a3p1y, a3p2x, a3p2y, a3p3x, a3p3y});
    gl->addWidget(m_arcStack);
    connect(m_arcMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_arcStack, &QStackedWidget::setCurrentIndex);
    l->addWidget(gb); return w;
}

QWidget* Properties::createRectangleWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w); l->setContentsMargins(0,0,0,0);
    auto* gb = new QGroupBox("Прямоугольник"); auto* gl = new QVBoxLayout(gb); gl->setSpacing(10);
    m_rectMethodCombo = new QComboBox();
    m_rectMethodCombo->addItem("Две точки"); m_rectMethodCombo->addItem("Точка и Размер"); m_rectMethodCombo->addItem("Центр и Размер");
    auto* mh = new QHBoxLayout(); mh->addWidget(createLbl("Метод:")); mh->addWidget(m_rectMethodCombo); gl->addLayout(mh);
    m_rectStack = new QStackedWidget();
    auto makePage = [&](QGridLayout*& grid) -> QWidget* { auto* p = new QWidget(); grid = new QGridLayout(p); grid->setContentsMargins(0,0,0,0); grid->setSpacing(10); grid->setColumnStretch(0,0); grid->setColumnStretch(1,1); grid->setColumnStretch(2,0); grid->setColumnStretch(3,1); return p; };
    QGridLayout *g0, *g1, *g2;
    m_rectStack->addWidget(makePage(g0));
    m_rectP1X = createSpin(); m_rectP1Y = createSpin(); m_rectP2X = createSpin(100); m_rectP2Y = createSpin(100);
    auto r0x1 = createLbl("X:"); auto r0y1 = createLbl("Y:"); auto r0x2 = createLbl("X:"); auto r0y2 = createLbl("Y:");
    g0->addWidget(new QLabel("Точка 1:"), 0, 0, 1, 4); addRow(g0, 1, r0x1, m_rectP1X, r0y1, m_rectP1Y);
    g0->addWidget(new QLabel("Точка 2:"), 2, 0, 1, 4); addRow(g0, 3, r0x2, m_rectP2X, r0y2, m_rectP2Y);
    m_coordLabels[PrimitiveType::Rectangle] = {r0x1, r0y1, r0x2, r0y2};
    m_rectStack->addWidget(makePage(g1));
    m_rect1PX = createSpin(); m_rect1PY = createSpin(); m_rect1W = createSpin(100, 0); m_rect1H = createSpin(50, 0);
    auto r1x = createLbl("X:"); auto r1y = createLbl("Y:");
    g1->addWidget(new QLabel("Угол:"), 0, 0, 1, 4); addRow(g1, 1, r1x, m_rect1PX, r1y, m_rect1PY);
    g1->addWidget(new QLabel("Размер:"), 2, 0, 1, 4); addRow(g1, 3, createLbl("W:"), m_rect1W, createLbl("H:"), m_rect1H);
    m_coordLabels[PrimitiveType::Rectangle].insert(m_coordLabels[PrimitiveType::Rectangle].end(), {r1x, r1y});
    m_rectStack->addWidget(makePage(g2));
    m_rectCX = createSpin(); m_rectCY = createSpin(); m_rectCW = createSpin(100, 0); m_rectCH = createSpin(50, 0);
    auto r2x = createLbl("X:"); auto r2y = createLbl("Y:");
    g2->addWidget(new QLabel("Центр:"), 0, 0, 1, 4); addRow(g2, 1, r2x, m_rectCX, r2y, m_rectCY);
    g2->addWidget(new QLabel("Размер:"), 2, 0, 1, 4); addRow(g2, 3, createLbl("W:"), m_rectCW, createLbl("H:"), m_rectCH);
    m_coordLabels[PrimitiveType::Rectangle].insert(m_coordLabels[PrimitiveType::Rectangle].end(), {r2x, r2y});
    gl->addWidget(m_rectStack);
    connect(m_rectMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_rectStack, &QStackedWidget::setCurrentIndex);
    m_rectChamfer = createSpin(0, 0, 50);
    auto* commonLayout = new QHBoxLayout(); commonLayout->addWidget(createLbl("Скругление:")); commonLayout->addWidget(m_rectChamfer); gl->addLayout(commonLayout);
    l->addWidget(gb); return w;
}

QWidget* Properties::createEllipseWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w); l->setContentsMargins(0,0,0,0);
    auto* gb = new QGroupBox("Эллипс"); auto* gl = new QVBoxLayout(gb); gl->setSpacing(10);
    m_ellMethodCombo = new QComboBox();
    m_ellMethodCombo->addItem("Центр и Радиусы"); m_ellMethodCombo->addItem("Центр и Оси");
    auto* mh = new QHBoxLayout(); mh->addWidget(createLbl("Метод:")); mh->addWidget(m_ellMethodCombo); gl->addLayout(mh);
    m_ellStack = new QStackedWidget();
    auto makePage = [&](QGridLayout*& grid) -> QWidget* { auto* p = new QWidget(); grid = new QGridLayout(p); grid->setContentsMargins(0,0,0,0); grid->setSpacing(10); grid->setColumnStretch(0,0); grid->setColumnStretch(1,1); grid->setColumnStretch(2,0); grid->setColumnStretch(3,1); return p; };
    QGridLayout *g0, *g1;
    m_ellStack->addWidget(makePage(g0));
    m_ellCX = createSpin(); m_ellCY = createSpin(); m_ellRX = createSpin(60, 0); m_ellRY = createSpin(30, 0);
    auto e0x = createLbl("X:"); auto e0y = createLbl("Y:");
    addRow(g0, 0, e0x, m_ellCX, e0y, m_ellCY); addRow(g0, 1, createLbl("Rx:"), m_ellRX, createLbl("Ry:"), m_ellRY);
    m_coordLabels[PrimitiveType::Ellipse] = {e0x, e0y};
    m_ellStack->addWidget(makePage(g1));
    m_ell2CX = createSpin(); m_ell2CY = createSpin(); m_ell2P1X = createSpin(50); m_ell2P1Y = createSpin(0); m_ell2P2X = createSpin(0); m_ell2P2Y = createSpin(30);
    auto e1x = createLbl("X:"); auto e1y = createLbl("Y:"); auto e1p1x = createLbl("X:"); auto e1p1y = createLbl("Y:"); auto e1p2x = createLbl("X:"); auto e1p2y = createLbl("Y:");
    g1->addWidget(new QLabel("Центр:"), 0, 0, 1, 4); addRow(g1, 1, e1x, m_ell2CX, e1y, m_ell2CY);
    g1->addWidget(new QLabel("Ось 1:"), 2, 0, 1, 4); addRow(g1, 3, e1p1x, m_ell2P1X, e1p1y, m_ell2P1Y);
    g1->addWidget(new QLabel("Ось 2:"), 4, 0, 1, 4); addRow(g1, 5, e1p2x, m_ell2P2X, e1p2y, m_ell2P2Y);
    m_coordLabels[PrimitiveType::Ellipse].insert(m_coordLabels[PrimitiveType::Ellipse].end(), {e1x, e1y, e1p1x, e1p1y, e1p2x, e1p2y});
    gl->addWidget(m_ellStack);
    connect(m_ellMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), m_ellStack, &QStackedWidget::setCurrentIndex);
    l->addWidget(gb); return w;
}

QWidget* Properties::createPolygonWidget() {
    auto* gb = new QGroupBox("Многоугольник"); auto* gl = setupGridLayout(gb);
    m_polyCX = createSpin(); m_polyCY = createSpin(); m_polyR = createSpin(50, 0);
    m_polySides = new QSpinBox(); m_polySides->setRange(3, 100); m_polySides->setValue(5);
    m_polyInscribed = new QComboBox(); m_polyInscribed->addItem("Вписанный", true); m_polyInscribed->addItem("Описанный", false);
    auto px = createLbl("X:"); auto py = createLbl("Y:");
    addRow(gl, 0, px, m_polyCX, py, m_polyCY); addRow(gl, 1, createLbl("R:"), m_polyR);
    addRow(gl, 2, createLbl("Сторон:"), m_polySides, createLbl("Тип:"), m_polyInscribed);
    m_coordLabels[PrimitiveType::Polygon] = {px, py};
    return gb;
}

QWidget* Properties::createSplineWidget() {
    auto* w = new QWidget(); auto* l = new QVBoxLayout(w); l->setContentsMargins(0,0,0,0);
    auto* gb = new QGroupBox("Сплайн"); auto* gl = new QVBoxLayout(gb); gl->setSpacing(10);
    gl->addWidget(new QLabel("Контрольные точки:"));
    auto* pointsContainer = new QWidget();
    m_splinePointsLayout = new QGridLayout(pointsContainer);
    m_splinePointsLayout->setContentsMargins(0,0,0,0); m_splinePointsLayout->setSpacing(10);
    m_splinePointsLayout->setColumnStretch(0,0); m_splinePointsLayout->setColumnStretch(1,1); m_splinePointsLayout->setColumnStretch(2,0); m_splinePointsLayout->setColumnStretch(3,1);
    gl->addWidget(pointsContainer);
    auto* btnLayout = new QHBoxLayout();
    auto* addBtn = new QPushButton("+ Точка"); auto* remBtn = new QPushButton("-"); remBtn->setFixedWidth(30);
    connect(addBtn, &QPushButton::clicked, this, &Properties::onAddSplinePoint);
    connect(remBtn, &QPushButton::clicked, this, &Properties::onRemoveSplinePoint);
    btnLayout->addWidget(addBtn); btnLayout->addWidget(remBtn); gl->addLayout(btnLayout);
    l->addWidget(gb); return w;
}

void Properties::onAddSplinePoint() {
    int index = m_splineSpinBoxes.size();
    auto* lx = createLbl("X:"); auto* sx = createSpin();
    auto* ly = createLbl("Y:"); auto* sy = createSpin();
    m_splinePointsLayout->addWidget(lx, index, 0); m_splinePointsLayout->addWidget(sx, index, 1);
    m_splinePointsLayout->addWidget(ly, index, 2); m_splinePointsLayout->addWidget(sy, index, 3);
    m_splineSpinBoxes.push_back({sx, sy});
    m_coordLabels[PrimitiveType::Spline].push_back(lx); m_coordLabels[PrimitiveType::Spline].push_back(ly);
    if (index > 0) { sx->setValue(m_splineSpinBoxes[index-1].first->value() + 10); sy->setValue(m_splineSpinBoxes[index-1].second->value()); }
    updateLabels();
}

void Properties::onRemoveSplinePoint() {
    if (m_splineSpinBoxes.size() <= 2) return;
    auto pair = m_splineSpinBoxes.back();
    auto l1 = m_coordLabels[PrimitiveType::Spline][m_coordLabels[PrimitiveType::Spline].size()-2];
    auto l2 = m_coordLabels[PrimitiveType::Spline].back();
    m_splinePointsLayout->removeWidget(pair.first); delete pair.first; m_splinePointsLayout->removeWidget(pair.second); delete pair.second;
    m_splinePointsLayout->removeWidget(l1); delete l1; m_splinePointsLayout->removeWidget(l2); delete l2;
    m_splineSpinBoxes.pop_back(); m_coordLabels[PrimitiveType::Spline].pop_back(); m_coordLabels[PrimitiveType::Spline].pop_back();
}

QGroupBox* Properties::createStyleWidget() {
    auto* group = new QGroupBox("Стиль");
    auto* grid = setupGridLayout(group);
    m_stylePresetButton = new QPushButton("Сплошная основная");
    m_stylePresetButton->setObjectName("StylePresetButton");
    connect(m_stylePresetButton, &QPushButton::clicked, this, &Properties::showStyleMenu);
    m_lineWidthSpin = new QDoubleSpinBox(); m_lineWidthSpin->setRange(0.1, 20); m_lineWidthSpin->setValue(2.0);
    m_colorButton = new QPushButton(); m_colorButton->setFixedSize(40, 20); m_colorButton->setObjectName("ColorPickerButton");
    m_colorButton->setStyleSheet("background-color: white; border: 1px solid gray;");
    connect(m_colorButton, &QPushButton::clicked, this, &Properties::onColorButtonClicked);
    addRow(grid, 0, createLbl("Тип:"), m_stylePresetButton);
    addRow(grid, 1, createLbl("Толщина:"), m_lineWidthSpin, createLbl("Цвет:"), m_colorButton);
    return group;
}

void Properties::showCreationPropertiesFor(PrimitiveType type, int methodIndex) {
    m_isCreationMode = true;
    m_activeType = type;
    m_currentObjects.clear();
    m_applyButton->setText("Создать"); m_applyButton->setProperty("state", "create");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show();

    if (type == PrimitiveType::Generic) {
        m_stack->setCurrentWidget(m_placeholderWidget);
        m_styleGroup->hide(); m_applyButton->hide();
    } else {
        m_stack->setCurrentWidget(m_primitiveWidgets[type]);
        m_styleGroup->show();
        if (type == PrimitiveType::Circle) m_circleMethodCombo->setCurrentIndex(methodIndex);
        if (type == PrimitiveType::Arc) m_arcMethodCombo->setCurrentIndex(methodIndex);
        if (type == PrimitiveType::Rectangle) m_rectMethodCombo->setCurrentIndex(methodIndex);
        if (type == PrimitiveType::Ellipse) m_ellMethodCombo->setCurrentIndex(methodIndex);
        if (type == PrimitiveType::Spline) {
            while(!m_splineSpinBoxes.empty()) onRemoveSplinePoint();
            onAddSplinePoint(); onAddSplinePoint(); onAddSplinePoint();
            m_splineSpinBoxes[0].first->setValue(0); m_splineSpinBoxes[0].second->setValue(0);
            m_splineSpinBoxes[1].first->setValue(50); m_splineSpinBoxes[1].second->setValue(50);
            m_splineSpinBoxes[2].first->setValue(100); m_splineSpinBoxes[2].second->setValue(0);
        }
    }
}

void Properties::showEditingPropertiesFor(const std::vector<Object*>& objects) {
    if (objects.empty()) { showCreationPropertiesFor(m_activeType, 0); return; }
    m_isCreationMode = false;
    m_currentObjects = objects;
    m_applyButton->setText("Обновить"); m_applyButton->setProperty("state", "update");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show(); m_styleGroup->show();

    // При выделении нескольких объектов показываем только общие свойства (стиль)
    // Либо, если объекты одного типа, можно было бы показывать и геометрию, но для простоты - только стиль при множественном выборе
    if (objects.size() == 1) {
        PrimitiveType type = objects[0]->getType();
        if (m_primitiveWidgets.count(type)) {
            m_stack->setCurrentWidget(m_primitiveWidgets[type]);
            populateFields(objects[0]);
        } else {
            m_stack->setCurrentWidget(m_placeholderWidget);
        }
    } else {
        m_stack->setCurrentWidget(m_placeholderWidget);
    }
    populateStyleFields(objects);
}

void Properties::populateFields(Object* obj) {
    setCoordinateSystem(CoordinateSystemType::Cartesian);
    // ... Implementations same as before (copy from previous Properties.cpp) ...
    // Для краткости подразумевается, что код switch(obj->getType()) здесь есть
    switch(obj->getType()) {
    case PrimitiveType::Segment: { auto* s = static_cast<Segment*>(obj); m_segX1->setValue(s->getStart().getX()); m_segY1->setValue(s->getStart().getY()); m_segX2->setValue(s->getEnd().getX()); m_segY2->setValue(s->getEnd().getY()); break; }
    case PrimitiveType::Circle: { m_circleMethodCombo->setCurrentIndex(0); auto* c = static_cast<Circle*>(obj); m_circCX->setValue(c->getCenter().getX()); m_circCY->setValue(c->getCenter().getY()); m_circR->setValue(c->getRadius()); break; }
    case PrimitiveType::Arc: { m_arcMethodCombo->setCurrentIndex(0); auto* a = static_cast<Arc*>(obj); m_arcCX->setValue(a->getCenter().getX()); m_arcCY->setValue(a->getCenter().getY()); m_arcR->setValue(a->getRadius()); double s = a->getStartAngle(); double sp = a->getSpanAngle(); if (Point::getAngleUnit() == AngleUnit::Radians) { s = qDegreesToRadians(s); sp = qDegreesToRadians(sp); } m_arcStart->setValue(s); m_arcSpan->setValue(sp); break; }
    case PrimitiveType::Rectangle: { m_rectMethodCombo->setCurrentIndex(1); auto* r = static_cast<Rectangle*>(obj); m_rect1PX->setValue(r->getTopLeft().getX()); m_rect1PY->setValue(r->getTopLeft().getY()); m_rect1W->setValue(r->getWidth()); m_rect1H->setValue(r->getHeight()); m_rectChamfer->setValue(r->getCornerRadius()); break; }
    case PrimitiveType::Ellipse: { m_ellMethodCombo->setCurrentIndex(0); auto* e = static_cast<Ellipse*>(obj); m_ellCX->setValue(e->getCenter().getX()); m_ellCY->setValue(e->getCenter().getY()); m_ellRX->setValue(e->getRadiusX()); m_ellRY->setValue(e->getRadiusY()); break; }
    case PrimitiveType::Polygon: { auto* p = static_cast<PolygonObj*>(obj); m_polyCX->setValue(p->getCenter().getX()); m_polyCY->setValue(p->getCenter().getY()); m_polyR->setValue(p->getRadius()); m_polySides->setValue(p->getSides()); int idx = m_polyInscribed->findData(p->isInscribed()); if(idx >= 0) m_polyInscribed->setCurrentIndex(idx); break; }
    case PrimitiveType::Spline: { auto* sp = static_cast<Spline*>(obj); const auto& pts = sp->getPoints(); while(!m_splineSpinBoxes.empty()) onRemoveSplinePoint(); for(const auto& p : pts) { onAddSplinePoint(); m_splineSpinBoxes.back().first->setValue(p.getX()); m_splineSpinBoxes.back().second->setValue(p.getY()); } break; }
    default: break;
    }
}

void Properties::populateStyleFields(const std::vector<Object*>& objects) {
    if(objects.empty()) return;
    auto style = objects[0]->getLineStyle();
    m_lineWidthSpin->setValue(style.width);
    m_stylePresetButton->setText(style.name);
    m_selectedColor = objects[0]->getColor();
    m_colorButton->setStyleSheet(QString("background-color: %1").arg(m_selectedColor.name()));

    // Если стили разные, можно показать "Смешанный", но пока берем первый
    m_currentStyle = style;
}

void Properties::updateObjectGeometry(Object* obj) {
    switch (obj->getType()) {
    case PrimitiveType::Segment: { if (auto* s = dynamic_cast<Segment*>(obj)) { s->setStart(readPoint(m_segX1, m_segY1)); s->setEnd(readPoint(m_segX2, m_segY2)); } break; }
    case PrimitiveType::Circle: { int method = m_circleMethodCombo->currentIndex(); Point c; double r = 0; if (method == 0) { c = readPoint(m_circCX, m_circCY); r = m_circR->value(); } else if (method == 1) { c = readPoint(m_circCX_D, m_circCY_D); r = m_circD->value() / 2.0; } else if (method == 2) { Point p1 = readPoint(m_circ2P1X, m_circ2P1Y); Point p2 = readPoint(m_circ2P2X, m_circ2P2Y); c = Point((p1.getX()+p2.getX())/2, (p1.getY()+p2.getY())/2); r = MathUtils::dist(p1, p2) / 2.0; } else if (method == 3) { Point p1 = readPoint(m_circ3P1X, m_circ3P1Y); Point p2 = readPoint(m_circ3P2X, m_circ3P2Y); Point p3 = readPoint(m_circ3P3X, m_circ3P3Y); MathUtils::getCircleFrom3Points(p1, p2, p3, c, r); } if (auto* circle = dynamic_cast<Circle*>(obj)) { circle->setCenter(c); circle->setRadius(r); } break; }
    case PrimitiveType::Arc: { int method = m_arcMethodCombo->currentIndex(); Point c; double r=0, start=0, span=0; if (method == 0) { c = readPoint(m_arcCX, m_arcCY); r = m_arcR->value(); start = m_arcStart->value(); span = m_arcSpan->value(); if (Point::getAngleUnit() == AngleUnit::Radians) { start = qRadiansToDegrees(start); span = qRadiansToDegrees(span); } } else { Point p1 = readPoint(m_arc3P1X, m_arc3P1Y); Point p2 = readPoint(m_arc3P2X, m_arc3P2Y); Point p3 = readPoint(m_arc3P3X, m_arc3P3Y); if (MathUtils::getCircleFrom3Points(p1, p2, p3, c, r)) { double aStart = std::atan2(p1.getY() - c.getY(), p1.getX() - c.getX()) * 180 / M_PI; double aEnd = std::atan2(p3.getY() - c.getY(), p3.getX() - c.getX()) * 180 / M_PI; if (aStart < 0) aStart += 360; if (aEnd < 0) aEnd += 360; span = aEnd - aStart; if (span < 0) span += 360; start = aStart; } } if (auto* arc = dynamic_cast<Arc*>(obj)) { arc->setCenter(c); arc->setRadius(r); arc->setStartAngle(start); arc->setSpanAngle(span); } break; }
    case PrimitiveType::Rectangle: { int method = m_rectMethodCombo->currentIndex(); Point tl; double w=0, h=0, cr = m_rectChamfer->value(); if (method == 0) { Point p1 = readPoint(m_rectP1X, m_rectP1Y); Point p2 = readPoint(m_rectP2X, m_rectP2Y); tl = Point(std::min(p1.getX(), p2.getX()), std::min(p1.getY(), p2.getY())); w = std::abs(p1.getX() - p2.getX()); h = std::abs(p1.getY() - p2.getY()); } else if (method == 1) { tl = readPoint(m_rect1PX, m_rect1PY); w = m_rect1W->value(); h = m_rect1H->value(); } else { Point c = readPoint(m_rectCX, m_rectCY); w = m_rectCW->value(); h = m_rectCH->value(); tl = Point(c.getX() - w/2, c.getY() - h/2); } if (auto* rect = dynamic_cast<Rectangle*>(obj)) { rect->setTopLeft(tl); rect->setWidth(w); rect->setHeight(h); rect->setCornerRadius(cr); } break; }
    case PrimitiveType::Ellipse: { int method = m_ellMethodCombo->currentIndex(); Point c; double rx=0, ry=0; if (method == 0) { c = readPoint(m_ellCX, m_ellCY); rx = m_ellRX->value(); ry = m_ellRY->value(); } else { c = readPoint(m_ell2CX, m_ell2CY); Point p1 = readPoint(m_ell2P1X, m_ell2P1Y); Point p2 = readPoint(m_ell2P2X, m_ell2P2Y); rx = MathUtils::dist(c, p1); ry = MathUtils::dist(c, p2); } if (auto* ell = dynamic_cast<Ellipse*>(obj)) { ell->setCenter(c); ell->setRadiusX(rx); ell->setRadiusY(ry); } break; }
    case PrimitiveType::Polygon: { if (auto* poly = dynamic_cast<PolygonObj*>(obj)) { poly->setCenter(readPoint(m_polyCX, m_polyCY)); poly->setRadius(m_polyR->value()); poly->setSides(m_polySides->value()); poly->setInscribed(m_polyInscribed->currentData().toBool()); } break; }
    case PrimitiveType::Spline: { if (auto* sp = dynamic_cast<Spline*>(obj)) { std::vector<Point> pts; for(const auto& pair : m_splineSpinBoxes) { pts.push_back(readPoint(pair.first, pair.second)); } sp->setPoints(pts); } break; }
    default: break;
    }
}

void Properties::onApplyClicked() {
    LineStyle s = m_currentStyle; s.width = m_lineWidthSpin->value();

    if (m_isCreationMode) {
        std::unique_ptr<Object> newObj;
        switch (m_activeType) {
        case PrimitiveType::Segment: newObj = std::make_unique<Segment>(Point(), Point()); break;
        case PrimitiveType::Circle: newObj = std::make_unique<Circle>(Point(), 10); break;
        case PrimitiveType::Arc: newObj = std::make_unique<Arc>(Point(), 10, 0, 90); break;
        case PrimitiveType::Rectangle: newObj = std::make_unique<Rectangle>(Point(), 10, 10); break;
        case PrimitiveType::Ellipse: newObj = std::make_unique<Ellipse>(Point(), 10, 5); break;
        case PrimitiveType::Polygon: newObj = std::make_unique<PolygonObj>(Point(), 10, 5); break;
        case PrimitiveType::Spline: newObj = std::make_unique<Spline>(std::vector<Point>()); break;
        default: break;
        }

        if (newObj) {
            updateObjectGeometry(newObj.get());
            newObj->setLineStyle(s);
            newObj->setColor(m_selectedColor);
            emit objectCreateRequested(newObj.release());
        }
    }
    else {
        // При редактировании
        for (auto* obj : m_currentObjects) {
            obj->setLineStyle(s);
            obj->setColor(m_selectedColor);
            // Геометрию обновляем только если объект один, чтобы не схлопнуть все объекты в одну точку
            if (m_currentObjects.size() == 1) {
                updateObjectGeometry(obj);
            }
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
            m_lineWidthSpin->setValue(s.width); // Авто подстановка ширины
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
