#include "Properties.h"
#include "Point.h"
#include "Segment.h"
#include "Circle.h"
#include "Arc.h"
#include "Rectangle.h"
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include "PointObject.h"
#include "Dimension.h"
#include "StyleDialog.h"
#include "LineSettingsMenu.h"
#include "MathUtils.h"

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
#include <QLineEdit>
#include <QIcon>
#include <cmath>
#include <QtMath>

// Локальное определение MathUtils удалено

static QDoubleSpinBox* createSpin(double val = 0, double min = -100000, double max = 100000) {
    auto* s = new QDoubleSpinBox();
    s->setRange(min, max);
    s->setValue(val);
    s->setDecimals(2);
    s->setMinimumWidth(48);
    s->setFocusPolicy(Qt::ClickFocus);
    return s;
}

static QLabel* createLbl(const QString& text) {
    auto* l = new QLabel(text);
    l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return l;
}

static bool canConvertDimensionType(DimensionType from, DimensionType to) {
    auto isLinear = [](DimensionType t) {
        return t == DimensionType::Linear || t == DimensionType::Horizontal || t == DimensionType::Vertical;
    };
    auto isRadial = [](DimensionType t) {
        return t == DimensionType::Radius || t == DimensionType::Diameter;
    };
    return from == to || (isLinear(from) && isLinear(to)) || (isRadial(from) && isRadial(to));
}

Properties::Properties(QWidget *parent) : QWidget(parent), m_isCreationMode(true) {
    this->setObjectName("PropertiesPanel");
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
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
    m_primitiveWidgets[PrimitiveType::Point] = createPointWidget();
    m_primitiveWidgets[PrimitiveType::Dimension] = createDimensionWidget();

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

    // Стили (без width)
    m_availableStyles = {
        {LineStyleType::SolidMain, "Сплошная основная", 0, 0, true},
        {LineStyleType::SolidThin, "Сплошная тонкая", 0, 0, false},
        {LineStyleType::SolidWavy, "Сплошная волнистая", 0, 0, false},
        {LineStyleType::SolidZigZag, "Сплошная с изломами", 0, 0, false},
        {LineStyleType::Dashed, "Штриховая", 4.0, 2.0, false},
        {LineStyleType::DashDotThin, "Штрихпунктирная тонкая", 10.0, 3.0, false},
        {LineStyleType::DashDotThick, "Штрихпунктирная толстая", 10.0, 3.0, true},
        {LineStyleType::DashDotDot, "С двумя точками", 10.0, 3.0, false}
    };
    m_currentStyle = m_availableStyles[0];

    showCreationPropertiesFor(PrimitiveType::Generic);
}

QGridLayout* Properties::setupGridLayout(QGroupBox* group) {
    auto* grid = new QGridLayout(group);
    grid->setContentsMargins(8, 12, 8, 8);
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(8);
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

QWidget* Properties::createPlaceholder() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    auto* lbl = new QLabel("Выберите инструмент\nили объект");
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setObjectName("PlaceholderLabel");
    l->addWidget(lbl);
    return w;
}

QWidget* Properties::createSegmentWidget() {
    auto* gb = new QGroupBox("Отрезок");
    auto* gl = setupGridLayout(gb);
    m_segX1 = createSpin(); m_segY1 = createSpin();
    m_segX2 = createSpin(100); m_segY2 = createSpin(100);

    m_lblSegStartX = createLbl("X:"); m_lblSegStartY = createLbl("Y:");
    m_lblSegEndX = createLbl("X:"); m_lblSegEndY = createLbl("Y:");

    auto* ts = new QLabel("Начало:"); ts->setStyleSheet("font-weight: bold; color: #F92672;");
    gl->addWidget(ts, 0, 0, 1, 4);
    addRow(gl, 1, m_lblSegStartX, m_segX1, m_lblSegStartY, m_segY1);

    auto* te = new QLabel("Конец:"); te->setStyleSheet("font-weight: bold; color: #F92672;");
    gl->addWidget(te, 2, 0, 1, 4);
    addRow(gl, 3, m_lblSegEndX, m_segX2, m_lblSegEndY, m_segY2);

    return gb;
}

QWidget* Properties::createCircleWidget() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);

    auto* gb = new QGroupBox("Окружность");
    auto* gl = new QVBoxLayout(gb);
    gl->setSpacing(10);

    m_circleMethodCombo = new QComboBox();
    m_circleMethodCombo->addItem("Центр и Радиус");
    m_circleMethodCombo->addItem("Центр и Диаметр");
    m_circleMethodCombo->addItem("Две точки (Диаметр)");
    m_circleMethodCombo->addItem("Три точки");

    auto* mh = new QHBoxLayout();
    mh->addWidget(createLbl("Метод:"));
    mh->addWidget(m_circleMethodCombo);
    gl->addLayout(mh);

    m_circleStack = new QStackedWidget();

    auto createPage = [&](QGridLayout*& g) {
        auto* p = new QWidget();
        g = new QGridLayout(p);
        g->setContentsMargins(0, 0, 0, 0);
        return p;
    };

    QGridLayout *g0, *g1, *g2, *g3;
    std::vector<QLabel*> labels;

    // --- Метод 0: Центр и Радиус ---
    m_circleStack->addWidget(createPage(g0));
    m_circCX = createSpin(); m_circCY = createSpin(); m_circR = createSpin(50, 0);
    auto* l0x = createLbl("X:"); auto* l0y = createLbl("Y:");
    addRow(g0, 0, l0x, m_circCX, l0y, m_circCY);
    addRow(g0, 1, createLbl("R:"), m_circR);
    labels.push_back(l0x); labels.push_back(l0y);

    // --- Метод 1: Центр и Диаметр ---
    m_circleStack->addWidget(createPage(g1));
    m_circCX_D = createSpin(); m_circCY_D = createSpin(); m_circD = createSpin(100, 0);
    auto* l1x = createLbl("X:"); auto* l1y = createLbl("Y:");
    addRow(g1, 0, l1x, m_circCX_D, l1y, m_circCY_D);
    addRow(g1, 1, createLbl("D:"), m_circD);
    labels.push_back(l1x); labels.push_back(l1y);

    // --- Метод 2: Две точки ---
    m_circleStack->addWidget(createPage(g2));
    m_circ2P1X = createSpin(); m_circ2P1Y = createSpin();
    m_circ2P2X = createSpin(); m_circ2P2Y = createSpin();

    g2->addWidget(new QLabel("Точка 1:"), 0, 0, 1, 4);
    auto* l2x1 = createLbl("X:"); auto* l2y1 = createLbl("Y:");
    addRow(g2, 1, l2x1, m_circ2P1X, l2y1, m_circ2P1Y);

    g2->addWidget(new QLabel("Точка 2:"), 2, 0, 1, 4);
    auto* l2x2 = createLbl("X:"); auto* l2y2 = createLbl("Y:");
    addRow(g2, 3, l2x2, m_circ2P2X, l2y2, m_circ2P2Y);

    labels.push_back(l2x1); labels.push_back(l2y1);
    labels.push_back(l2x2); labels.push_back(l2y2);

    // --- Метод 3: Три точки ---
    m_circleStack->addWidget(createPage(g3));
    m_circ3P1X = createSpin(); m_circ3P1Y = createSpin();
    m_circ3P2X = createSpin(); m_circ3P2Y = createSpin();
    m_circ3P3X = createSpin(); m_circ3P3Y = createSpin();

    g3->addWidget(new QLabel("Точка 1:"), 0, 0, 1, 4);
    auto* l3x1 = createLbl("X:"); auto* l3y1 = createLbl("Y:");
    addRow(g3, 1, l3x1, m_circ3P1X, l3y1, m_circ3P1Y);

    g3->addWidget(new QLabel("Точка 2:"), 2, 0, 1, 4);
    auto* l3x2 = createLbl("X:"); auto* l3y2 = createLbl("Y:");
    addRow(g3, 3, l3x2, m_circ3P2X, l3y2, m_circ3P2Y);

    g3->addWidget(new QLabel("Точка 3:"), 4, 0, 1, 4);
    auto* l3x3 = createLbl("X:"); auto* l3y3 = createLbl("Y:");
    addRow(g3, 5, l3x3, m_circ3P3X, l3y3, m_circ3P3Y);

    labels.push_back(l3x1); labels.push_back(l3y1);
    labels.push_back(l3x2); labels.push_back(l3y2);
    labels.push_back(l3x3); labels.push_back(l3y3);

    m_coordLabels[PrimitiveType::Circle] = labels;

    gl->addWidget(m_circleStack);
    connect(m_circleMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int next) {
        int old = m_circleStack->currentIndex();
        Point c;
        double r = 50.0;
        if (old == 0) { c = readPoint(m_circCX, m_circCY); r = m_circR->value(); }
        else if (old == 1) { c = readPoint(m_circCX_D, m_circCY_D); r = m_circD->value() / 2.0; }
        else if (old == 2) {
            Point p1 = readPoint(m_circ2P1X, m_circ2P1Y), p2 = readPoint(m_circ2P2X, m_circ2P2Y);
            c = Point((p1.getX() + p2.getX()) / 2.0, (p1.getY() + p2.getY()) / 2.0);
            r = MathUtils::dist(p1, p2) / 2.0;
        } else {
            Point p1 = readPoint(m_circ3P1X, m_circ3P1Y), p2 = readPoint(m_circ3P2X, m_circ3P2Y), p3 = readPoint(m_circ3P3X, m_circ3P3Y);
            MathUtils::getCircleFrom3Points(p1, p2, p3, c, r);
        }
        m_circCX->setValue(c.getX()); m_circCY->setValue(c.getY()); m_circR->setValue(r);
        m_circCX_D->setValue(c.getX()); m_circCY_D->setValue(c.getY()); m_circD->setValue(r * 2.0);
        m_circ2P1X->setValue(c.getX() - r); m_circ2P1Y->setValue(c.getY());
        m_circ2P2X->setValue(c.getX() + r); m_circ2P2Y->setValue(c.getY());
        m_circ3P1X->setValue(c.getX() + r); m_circ3P1Y->setValue(c.getY());
        m_circ3P2X->setValue(c.getX()); m_circ3P2Y->setValue(c.getY() + r);
        m_circ3P3X->setValue(c.getX() - r); m_circ3P3Y->setValue(c.getY());
        m_circleStack->setCurrentIndex(next);
    });
    l->addWidget(gb);
    return w;
}

QWidget* Properties::createArcWidget() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);

    auto* gb = new QGroupBox("Дуга");
    auto* gl = new QVBoxLayout(gb);

    m_arcMethodCombo = new QComboBox();
    m_arcMethodCombo->addItem("Центр, Угол");
    m_arcMethodCombo->addItem("Три точки");

    auto* methodLayout = new QHBoxLayout();
    methodLayout->addWidget(createLbl("Метод:"));
    methodLayout->addWidget(m_arcMethodCombo);
    gl->addLayout(methodLayout);

    m_arcStack = new QStackedWidget();

    auto createPage = [&](QGridLayout*& g) {
        auto* p = new QWidget();
        g = new QGridLayout(p);
        g->setContentsMargins(0, 0, 0, 0);
        return p;
    };

    QGridLayout *g0, *g1;
    std::vector<QLabel*> labels;

    // --- Метод 0: Центр, Угол ---
    m_arcStack->addWidget(createPage(g0));
    m_arcCX = createSpin(); m_arcCY = createSpin();
    m_arcR = createSpin(50);
    m_arcStart = createSpin(); m_arcSpan = createSpin(90);

    auto* acx = createLbl("X:"); auto* acy = createLbl("Y:");
    addRow(g0, 0, acx, m_arcCX, acy, m_arcCY);
    addRow(g0, 1, createLbl("R:"), m_arcR);
    addRow(g0, 2, createLbl("Нач:"), m_arcStart, createLbl("Угол:"), m_arcSpan);
    labels.push_back(acx); labels.push_back(acy);

    // --- Метод 1: Три точки ---
    m_arcStack->addWidget(createPage(g1));
    m_arc3P1X = createSpin(); m_arc3P1Y = createSpin();
    m_arc3P2X = createSpin(); m_arc3P2Y = createSpin();
    m_arc3P3X = createSpin(); m_arc3P3Y = createSpin();

    auto* a1x = createLbl("X1:"); auto* a1y = createLbl("Y1:");
    addRow(g1, 0, a1x, m_arc3P1X, a1y, m_arc3P1Y);

    auto* a2x = createLbl("X2:"); auto* a2y = createLbl("Y2:");
    addRow(g1, 1, a2x, m_arc3P2X, a2y, m_arc3P2Y);

    auto* a3x = createLbl("X3:"); auto* a3y = createLbl("Y3:");
    addRow(g1, 2, a3x, m_arc3P3X, a3y, m_arc3P3Y);

    labels.push_back(a1x); labels.push_back(a1y);
    labels.push_back(a2x); labels.push_back(a2y);
    labels.push_back(a3x); labels.push_back(a3y);

    m_coordLabels[PrimitiveType::Arc] = labels;

    gl->addWidget(m_arcStack);
    connect(m_arcMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int next) {
        int old = m_arcStack->currentIndex();
        Point c;
        double r = 50.0, start = 0.0, span = 90.0;
        if (old == 0) {
            c = readPoint(m_arcCX, m_arcCY);
            r = m_arcR->value();
            start = m_arcStart->value();
            span = m_arcSpan->value();
        } else {
            Point p1 = readPoint(m_arc3P1X, m_arc3P1Y), p2 = readPoint(m_arc3P2X, m_arc3P2Y), p3 = readPoint(m_arc3P3X, m_arc3P3Y);
            if (MathUtils::getCircleFrom3Points(p1, p2, p3, c, r)) {
                start = qRadiansToDegrees(std::atan2(p1.getY() - c.getY(), p1.getX() - c.getX()));
                double end = qRadiansToDegrees(std::atan2(p3.getY() - c.getY(), p3.getX() - c.getX()));
                span = end - start;
            }
        }
        m_arcCX->setValue(c.getX()); m_arcCY->setValue(c.getY()); m_arcR->setValue(r); m_arcStart->setValue(start); m_arcSpan->setValue(span);
        double a1 = qDegreesToRadians(start), am = qDegreesToRadians(start + span / 2.0), a2 = qDegreesToRadians(start + span);
        m_arc3P1X->setValue(c.getX() + r * std::cos(a1)); m_arc3P1Y->setValue(c.getY() + r * std::sin(a1));
        m_arc3P2X->setValue(c.getX() + r * std::cos(am)); m_arc3P2Y->setValue(c.getY() + r * std::sin(am));
        m_arc3P3X->setValue(c.getX() + r * std::cos(a2)); m_arc3P3Y->setValue(c.getY() + r * std::sin(a2));
        m_arcStack->setCurrentIndex(next);
    });
    l->addWidget(gb);
    return w;
}

QWidget* Properties::createRectangleWidget() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);

    auto* gb = new QGroupBox("Прямоугольник");
    auto* gl = new QVBoxLayout(gb);

    m_rectMethodCombo = new QComboBox();
    m_rectMethodCombo->addItem("Две точки");
    m_rectMethodCombo->addItem("Точка и Размер");
    m_rectMethodCombo->addItem("Центр и Размер");

    auto* mh = new QHBoxLayout();
    mh->addWidget(createLbl("Метод:"));
    mh->addWidget(m_rectMethodCombo);
    gl->addLayout(mh);

    m_rectStack = new QStackedWidget();

    auto createPage = [&](QGridLayout*& g) {
        auto* p = new QWidget();
        g = new QGridLayout(p);
        g->setContentsMargins(0, 0, 0, 0);
        return p;
    };

    QGridLayout *g0, *g1, *g2;
    std::vector<QLabel*> labels;

    // --- Метод 0: Две точки ---
    m_rectStack->addWidget(createPage(g0));
    m_rectP1X = createSpin(); m_rectP1Y = createSpin();
    m_rectP2X = createSpin(100); m_rectP2Y = createSpin(100);

    auto* r0x1 = createLbl("X1:"); auto* r0y1 = createLbl("Y1:");
    addRow(g0, 0, r0x1, m_rectP1X, r0y1, m_rectP1Y);

    auto* r0x2 = createLbl("X2:"); auto* r0y2 = createLbl("Y2:");
    addRow(g0, 1, r0x2, m_rectP2X, r0y2, m_rectP2Y);

    labels.push_back(r0x1); labels.push_back(r0y1);
    labels.push_back(r0x2); labels.push_back(r0y2);

    // --- Метод 1: Точка и Размер ---
    m_rectStack->addWidget(createPage(g1));
    m_rect1PX = createSpin(); m_rect1PY = createSpin();
    m_rect1W = createSpin(100); m_rect1H = createSpin(50);

    auto* r1x = createLbl("X:"); auto* r1y = createLbl("Y:");
    addRow(g1, 0, r1x, m_rect1PX, r1y, m_rect1PY);
    addRow(g1, 1, createLbl("W:"), m_rect1W, createLbl("H:"), m_rect1H);
    labels.push_back(r1x); labels.push_back(r1y);

    // --- Метод 2: Центр и Размер ---
    m_rectStack->addWidget(createPage(g2));
    m_rectCX = createSpin(); m_rectCY = createSpin();
    m_rectCW = createSpin(100); m_rectCH = createSpin(50);

    auto* r2x = createLbl("X:"); auto* r2y = createLbl("Y:");
    addRow(g2, 0, r2x, m_rectCX, r2y, m_rectCY);
    addRow(g2, 1, createLbl("W:"), m_rectCW, createLbl("H:"), m_rectCH);
    labels.push_back(r2x); labels.push_back(r2y);

    m_coordLabels[PrimitiveType::Rectangle] = labels;

    gl->addWidget(m_rectStack);
    connect(m_rectMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int next) {
        int old = m_rectStack->currentIndex();
        Point tl;
        double w = 100.0, h = 50.0;
        if (old == 0) {
            Point p1 = readPoint(m_rectP1X, m_rectP1Y), p2 = readPoint(m_rectP2X, m_rectP2Y);
            tl = Point(std::min(p1.getX(), p2.getX()), std::max(p1.getY(), p2.getY()));
            w = std::abs(p1.getX() - p2.getX());
            h = std::abs(p1.getY() - p2.getY());
        } else if (old == 1) {
            tl = readPoint(m_rect1PX, m_rect1PY); w = m_rect1W->value(); h = m_rect1H->value();
        } else {
            Point c = readPoint(m_rectCX, m_rectCY); w = m_rectCW->value(); h = m_rectCH->value();
            tl = Point(c.getX() - w / 2.0, c.getY() + h / 2.0);
        }
        m_rectP1X->setValue(tl.getX()); m_rectP1Y->setValue(tl.getY());
        m_rectP2X->setValue(tl.getX() + w); m_rectP2Y->setValue(tl.getY() - h);
        m_rect1PX->setValue(tl.getX()); m_rect1PY->setValue(tl.getY()); m_rect1W->setValue(w); m_rect1H->setValue(h);
        m_rectCX->setValue(tl.getX() + w / 2.0); m_rectCY->setValue(tl.getY() - h / 2.0); m_rectCW->setValue(w); m_rectCH->setValue(h);
        m_rectStack->setCurrentIndex(next);
    });

    m_rectChamfer = createSpin();
    auto* cl = new QHBoxLayout();
    cl->addWidget(createLbl("Скругление:"));
    cl->addWidget(m_rectChamfer);
    gl->addLayout(cl);

    l->addWidget(gb);
    return w;
}

QWidget* Properties::createEllipseWidget() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);

    auto* gb = new QGroupBox("Эллипс");
    auto* gl = new QVBoxLayout(gb);

    m_ellMethodCombo = new QComboBox();
    m_ellMethodCombo->addItem("Центр и Радиусы");
    m_ellMethodCombo->addItem("Центр и Оси");

    auto* mh = new QHBoxLayout();
    mh->addWidget(createLbl("Метод:"));
    mh->addWidget(m_ellMethodCombo);
    gl->addLayout(mh);

    m_ellStack = new QStackedWidget();

    auto createPage = [&](QGridLayout*& g) {
        auto* p = new QWidget();
        g = new QGridLayout(p);
        g->setContentsMargins(0, 0, 0, 0);
        return p;
    };

    QGridLayout *g0, *g1;
    std::vector<QLabel*> labels;

    // --- Метод 0: Центр и Радиусы ---
    m_ellStack->addWidget(createPage(g0));
    m_ellCX = createSpin(); m_ellCY = createSpin();
    m_ellRX = createSpin(60); m_ellRY = createSpin(30);

    auto* e0x = createLbl("X:"); auto* e0y = createLbl("Y:");
    addRow(g0, 0, e0x, m_ellCX, e0y, m_ellCY);
    addRow(g0, 1, createLbl("Rx:"), m_ellRX, createLbl("Ry:"), m_ellRY);
    labels.push_back(e0x); labels.push_back(e0y);

    // --- Метод 1: Центр и Оси (Точки) ---
    m_ellStack->addWidget(createPage(g1));
    m_ell2CX = createSpin(); m_ell2CY = createSpin();
    m_ell2P1X = createSpin(50); m_ell2P1Y = createSpin();
    m_ell2P2X = createSpin(0); m_ell2P2Y = createSpin(30);

    auto* e1x = createLbl("X:"); auto* e1y = createLbl("Y:");
    addRow(g1, 0, e1x, m_ell2CX, e1y, m_ell2CY);

    auto* e1p1x = createLbl("P1X:"); auto* e1p1y = createLbl("P1Y:");
    addRow(g1, 1, e1p1x, m_ell2P1X, e1p1y, m_ell2P1Y);

    auto* e1p2x = createLbl("P2X:"); auto* e1p2y = createLbl("P2Y:");
    addRow(g1, 2, e1p2x, m_ell2P2X, e1p2y, m_ell2P2Y);

    labels.push_back(e1x); labels.push_back(e1y);
    labels.push_back(e1p1x); labels.push_back(e1p1y);
    labels.push_back(e1p2x); labels.push_back(e1p2y);

    m_coordLabels[PrimitiveType::Ellipse] = labels;

    gl->addWidget(m_ellStack);
    connect(m_ellMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int next) {
        int old = m_ellStack->currentIndex();
        Point c;
        double rx = 60.0, ry = 30.0;
        if (old == 0) {
            c = readPoint(m_ellCX, m_ellCY); rx = m_ellRX->value(); ry = m_ellRY->value();
        } else {
            c = readPoint(m_ell2CX, m_ell2CY);
            rx = MathUtils::dist(c, readPoint(m_ell2P1X, m_ell2P1Y));
            ry = MathUtils::dist(c, readPoint(m_ell2P2X, m_ell2P2Y));
        }
        m_ellCX->setValue(c.getX()); m_ellCY->setValue(c.getY()); m_ellRX->setValue(rx); m_ellRY->setValue(ry);
        m_ell2CX->setValue(c.getX()); m_ell2CY->setValue(c.getY());
        m_ell2P1X->setValue(c.getX() + rx); m_ell2P1Y->setValue(c.getY());
        m_ell2P2X->setValue(c.getX()); m_ell2P2Y->setValue(c.getY() + ry);
        m_ellStack->setCurrentIndex(next);
    });
    l->addWidget(gb);
    return w;
}

QWidget* Properties::createPolygonWidget() {
    auto* gb = new QGroupBox("Многоугольник");
    auto* gl = setupGridLayout(gb);

    m_polyCX = createSpin(); m_polyCY = createSpin();
    m_polyR = createSpin(50);
    m_polySides = new QSpinBox(); m_polySides->setValue(5);
    m_polyInscribed = new QComboBox();
    m_polyInscribed->addItem("Вписанный", true);
    m_polyInscribed->addItem("Описанный", false);

    auto* px = createLbl("X:"); auto* py = createLbl("Y:");
    addRow(gl, 0, px, m_polyCX, py, m_polyCY);
    addRow(gl, 1, createLbl("R:"), m_polyR);
    addRow(gl, 2, createLbl("Сторон:"), m_polySides, createLbl("Тип:"), m_polyInscribed);

    m_coordLabels[PrimitiveType::Polygon] = { px, py };

    return gb;
}

QWidget* Properties::createSplineWidget() {
    auto* w = new QWidget();
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);

    auto* gb = new QGroupBox("Сплайн");
    auto* gl = new QVBoxLayout(gb);

    auto* pc = new QWidget();
    m_splinePointsLayout = new QGridLayout(pc);
    gl->addWidget(pc);

    auto* bl = new QHBoxLayout();
    auto* ab = new QPushButton("+");
    auto* rb = new QPushButton("-");
    connect(ab, &QPushButton::clicked, this, &Properties::onAddSplinePoint);
    connect(rb, &QPushButton::clicked, this, &Properties::onRemoveSplinePoint);
    bl->addWidget(ab);
    bl->addWidget(rb);
    gl->addLayout(bl);

    l->addWidget(gb);
    return w;
}

QWidget* Properties::createPointWidget() {
    auto* gb = new QGroupBox("Точка");
    auto* gl = setupGridLayout(gb);
    m_ptX = createSpin(); m_ptY = createSpin();
    auto* lx = createLbl("X:"); auto* ly = createLbl("Y:");
    addRow(gl, 0, lx, m_ptX, ly, m_ptY);
    m_coordLabels[PrimitiveType::Point] = { lx, ly };
    return gb;
}

QWidget* Properties::createDimensionWidget() {
    auto* gb = new QGroupBox("Размер");
    auto* gl = setupGridLayout(gb);

    m_dimTypeCombo = new QComboBox();
    m_dimTypeCombo->addItem(QIcon(":/icons/dimension-linear.svg"), "Линейный", static_cast<int>(DimensionType::Linear));
    m_dimTypeCombo->addItem(QIcon(":/icons/dimension-horizontal.svg"), "Горизонтальный", static_cast<int>(DimensionType::Horizontal));
    m_dimTypeCombo->addItem(QIcon(":/icons/dimension-vertical.svg"), "Вертикальный", static_cast<int>(DimensionType::Vertical));
    m_dimTypeCombo->addItem(QIcon(":/icons/dimension-radius.svg"), "Радиус", static_cast<int>(DimensionType::Radius));
    m_dimTypeCombo->addItem(QIcon(":/icons/dimension-diameter.svg"), "Диаметр", static_cast<int>(DimensionType::Diameter));
    m_dimTypeCombo->addItem(QIcon(":/icons/dimension-angular.svg"), "Угловой", static_cast<int>(DimensionType::Angular));
    m_dimTypeCombo->setEnabled(true);

    m_dimValue = createSpin(0, 0, 100000);
    m_dimTextOverrideEdit = new QLineEdit();
    m_dimCenterTextButton = new QPushButton("Центрировать");
    m_dimToggleSideButton = new QPushButton("Сменить сторону");
    m_dimPrefixCombo = new QComboBox();
    m_dimPrefixCombo->addItem("Нет", static_cast<int>(DimensionValuePrefix::None));
    m_dimPrefixCombo->addItem("R", static_cast<int>(DimensionValuePrefix::Radius));
    m_dimPrefixCombo->addItem(QString::fromUtf8("Ø"), static_cast<int>(DimensionValuePrefix::Diameter));
    m_dimArrowCombo = new QComboBox();
    m_dimArrowCombo->addItem(QIcon(":/icons/arrow-open.svg"), "Закрытая", static_cast<int>(ArrowType::Closed));
    m_dimArrowCombo->addItem(QIcon(":/icons/arrow-closed.svg"), "Открытая", static_cast<int>(ArrowType::Open));
    m_dimArrowCombo->addItem(QIcon(":/icons/arrow-tick.svg"), "Засечка", static_cast<int>(ArrowType::Tick));
    m_dimArrowCombo->addItem(QIcon(":/icons/arrow-dot.svg"), "Точка", static_cast<int>(ArrowType::Dot));
    m_dimArrowPlacementCombo = new QComboBox();
    m_dimArrowPlacementCombo->addItem("Внутри", static_cast<int>(ArrowPlacement::Inside));
    m_dimArrowPlacementCombo->addItem("Снаружи", static_cast<int>(ArrowPlacement::Outside));
    m_dimArrowSize = createSpin(10, 1, 100);
    m_dimTextHeight = createSpin(16, 1, 100);
    m_dimTextOffset = createSpin(8, -100, 100);

    addRow(gl, 0, createLbl("Тип:"), m_dimTypeCombo);
    addRow(gl, 1, createLbl("Значение:"), m_dimValue);
    addRow(gl, 2, createLbl("Текст:"), m_dimTextOverrideEdit);
    addRow(gl, 3, createLbl("Тип стрелки:"), m_dimArrowCombo);
    addRow(gl, 4, createLbl("Положение стрелок:"), m_dimArrowPlacementCombo);
    addRow(gl, 5, createLbl("Размер стрелки:"), m_dimArrowSize);
    addRow(gl, 6, createLbl("Размер шрифта:"), m_dimTextHeight);
    addRow(gl, 7, createLbl("Отступ текста:"), m_dimTextOffset);
    addRow(gl, 8, createLbl("Префикс:"), m_dimPrefixCombo);
    addRow(gl, 9, createLbl("Позиция текста:"), m_dimCenterTextButton);
    addRow(gl, 10, createLbl("Сторона угла:"), m_dimToggleSideButton);

    connect(m_dimCenterTextButton, &QPushButton::clicked, this, [this]() {
        for (auto* obj : m_currentObjects) {
            if (auto* d = dynamic_cast<Dimension*>(obj)) {
                d->centerText();
            }
        }
        emit objectsModified(m_currentObjects);
        if (!m_currentObjects.empty()) populateFields(m_currentObjects.front());
    });

    connect(m_dimToggleSideButton, &QPushButton::clicked, this, [this]() {
        if (m_isCreationMode) {
            emit dimensionSideToggleRequested();
            return;
        }
        for (auto* obj : m_currentObjects) {
            if (auto* d = dynamic_cast<Dimension*>(obj)) {
                d->toggleAngleSide();
            }
        }
        emit objectsModified(m_currentObjects);
        if (!m_currentObjects.empty()) populateFields(m_currentObjects.front());
    });

    connect(m_dimPrefixCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        if (!m_isCreationMode || m_activeType != PrimitiveType::Dimension) {
            return;
        }
        GlobalSettings::instance().dimensionStyle.linearPrefix =
            static_cast<DimensionValuePrefix>(m_dimPrefixCombo->currentData().toInt());
    });

    return gb;
}

QGroupBox* Properties::createStyleWidget() {
    auto* group = new QGroupBox("Стиль");
    auto* grid = setupGridLayout(group);

    m_stylePresetButton = new QPushButton("Сплошная основная");
    m_stylePresetButton->setObjectName("StylePresetButton");
    connect(m_stylePresetButton, &QPushButton::clicked, this, &Properties::showStyleMenu);

    m_colorButton = new QPushButton();
    m_colorButton->setFixedSize(40, 20);
    m_colorButton->setObjectName("ColorPickerButton");
    m_colorButton->setStyleSheet("background-color: white; border: 1px solid gray;");
    connect(m_colorButton, &QPushButton::clicked, this, &Properties::onColorButtonClicked);

    m_layerCombo = new QComboBox();
    m_layerCombo->setEditable(true);
    m_layerCombo->addItem("0");
    m_layerCombo->setCurrentText("0");
    connect(m_layerCombo, &QComboBox::currentTextChanged, this, [this](const QString& text){
        m_selectedLayer = text;
    });

    addRow(grid, 0, createLbl("Тип:"), m_stylePresetButton);
    addRow(grid, 1, createLbl("Цвет:"), m_colorButton);
    addRow(grid, 2, createLbl("Слой:"), m_layerCombo);
    return group;
}

Point Properties::readPoint(QDoubleSpinBox* xBox, QDoubleSpinBox* yBox) const {
    double v1 = xBox->value();
    double v2 = yBox->value();
    if (m_coordSystem == CoordinateSystemType::Polar) {
        Point p;
        p.setPolar(v1, v2);
        return p;
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
        for(size_t i = 0; i < list.size(); i += 2) {
            if(i + 1 < list.size()) {
                if(list[i]) list[i]->setText(l1);
                if(list[i+1]) list[i+1]->setText(l2);
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
    m_lblSegEndX->setText(l1); m_lblSegEndY->setText(l2);
}

void Properties::onAddSplinePoint() {
    int index = m_splineSpinBoxes.size();
    auto* lx = createLbl("X:");
    auto* sx = createSpin();
    auto* ly = createLbl("Y:");
    auto* sy = createSpin();

    m_splinePointsLayout->addWidget(lx, index, 0);
    m_splinePointsLayout->addWidget(sx, index, 1);
    m_splinePointsLayout->addWidget(ly, index, 2);
    m_splinePointsLayout->addWidget(sy, index, 3);

    m_splineSpinBoxes.push_back({sx, sy});
    m_coordLabels[PrimitiveType::Spline].push_back(lx);
    m_coordLabels[PrimitiveType::Spline].push_back(ly);

    if (index > 0) {
        sx->setValue(m_splineSpinBoxes[index - 1].first->value() + 10);
        sy->setValue(m_splineSpinBoxes[index - 1].second->value());
    }
    updateLabels();
}

void Properties::onRemoveSplinePoint() {
    // Не удаляем если осталось 2 или меньше точек (минимум для сплайна)
    if (m_splineSpinBoxes.size() <= 2) return;
    
    // Проверяем наличие элементов в m_coordLabels
    auto& labels = m_coordLabels[PrimitiveType::Spline];
    if (labels.size() < 2) return;
    
    auto pair = m_splineSpinBoxes.back();
    auto* l1 = labels[labels.size() - 2];
    auto* l2 = labels.back();

    m_splinePointsLayout->removeWidget(pair.first); delete pair.first;
    m_splinePointsLayout->removeWidget(pair.second); delete pair.second;
    m_splinePointsLayout->removeWidget(l1); delete l1;
    m_splinePointsLayout->removeWidget(l2); delete l2;

    m_splineSpinBoxes.pop_back();
    labels.pop_back();
    labels.pop_back();
}

// Вспомогательная функция для очистки всех точек сплайна
void Properties::clearAllSplinePoints() {
    auto& labels = m_coordLabels[PrimitiveType::Spline];
    
    // Удаляем все виджеты точек
    for (auto& pair : m_splineSpinBoxes) {
        m_splinePointsLayout->removeWidget(pair.first); delete pair.first;
        m_splinePointsLayout->removeWidget(pair.second); delete pair.second;
    }
    m_splineSpinBoxes.clear();
    
    // Удаляем все метки
    for (auto* lbl : labels) {
        m_splinePointsLayout->removeWidget(lbl); delete lbl;
    }
    labels.clear();
}

void Properties::applyCurrentStyleTo(Object* obj) const {
    if(!obj) return;
    obj->setLineStyle(m_currentStyle);
    obj->setColor(m_selectedColor);
    obj->setLayer(m_selectedLayer);
}

void Properties::showCreationPropertiesFor(PrimitiveType type, int methodIndex) {
    m_isCreationMode = true; m_activeType = type; m_currentObjects.clear();
    m_applyButton->setText("Создать"); m_applyButton->setProperty("state", "create");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show();

    if (type == PrimitiveType::Generic) {
        m_stack->setCurrentWidget(m_placeholderWidget);
        m_styleGroup->hide();
        m_applyButton->hide();
    }
    else {
        m_stack->setCurrentWidget(m_primitiveWidgets[type]);

        // СИНХРОНИЗАЦИЯ С МЕТОДОМ
        if (type == PrimitiveType::Circle) {
            m_circleMethodCombo->blockSignals(true);
            m_circleMethodCombo->setCurrentIndex(methodIndex);
            m_circleStack->setCurrentIndex(methodIndex);
            m_circleMethodCombo->blockSignals(false);
        }
        else if (type == PrimitiveType::Rectangle) {
            m_rectMethodCombo->blockSignals(true);
            m_rectMethodCombo->setCurrentIndex(methodIndex);
            m_rectStack->setCurrentIndex(methodIndex);
            m_rectMethodCombo->blockSignals(false);
        }
        else if (type == PrimitiveType::Arc) {
            m_arcMethodCombo->blockSignals(true);
            m_arcMethodCombo->setCurrentIndex(methodIndex);
            m_arcStack->setCurrentIndex(methodIndex);
            m_arcMethodCombo->blockSignals(false);
        }
        else if (type == PrimitiveType::Ellipse) {
            m_ellMethodCombo->blockSignals(true);
            m_ellMethodCombo->setCurrentIndex(methodIndex);
            m_ellStack->setCurrentIndex(methodIndex);
            m_ellMethodCombo->blockSignals(false);
        }

        m_styleGroup->show();
    }

    if (m_dimToggleSideButton) {
        m_dimToggleSideButton->setVisible(type == PrimitiveType::Dimension
                                          && methodIndex == static_cast<int>(DimensionType::Angular));
    }
    if (type == PrimitiveType::Dimension && m_dimPrefixCombo) {
        const int idx = m_dimPrefixCombo->findData(static_cast<int>(GlobalSettings::instance().dimensionStyle.linearPrefix));
        if (idx >= 0) m_dimPrefixCombo->setCurrentIndex(idx);
    }

    m_stylePresetButton->setText(m_currentStyle.name);
    m_colorButton->setStyleSheet(QString("background-color: %1").arg(m_selectedColor.name()));
    m_layerCombo->setCurrentText(m_selectedLayer);
}

void Properties::showEditingPropertiesFor(const std::vector<Object*>& objects) {
    if (objects.empty()) {
        showCreationPropertiesFor(m_activeType, 0);
        return;
    }
    m_isCreationMode = false; m_currentObjects = objects;
    m_applyButton->setText("Обновить"); m_applyButton->setProperty("state", "update");
    m_applyButton->style()->unpolish(m_applyButton); m_applyButton->style()->polish(m_applyButton);
    m_applyButton->show(); m_styleGroup->show();

    if (objects.size() == 1) {
        PrimitiveType type = objects[0]->getType();
        if (m_primitiveWidgets.count(type)) {
            m_stack->setCurrentWidget(m_primitiveWidgets[type]);
            populateFields(objects[0]);
        }
        else m_stack->setCurrentWidget(m_placeholderWidget);
    } else {
        m_stack->setCurrentWidget(m_placeholderWidget);
    }
    if (m_dimToggleSideButton) {
        const bool showToggle = objects.size() == 1
            && objects[0]->getType() == PrimitiveType::Dimension
            && static_cast<Dimension*>(objects[0])->getDimensionType() == DimensionType::Angular;
        m_dimToggleSideButton->setVisible(showToggle);
    }
    populateStyleFields(objects);
}

void Properties::populateFields(Object* obj) {
    setCoordinateSystem(CoordinateSystemType::Cartesian);
    switch(obj->getType()) {
    case PrimitiveType::Segment: {
        auto* s = static_cast<Segment*>(obj);
        m_segX1->setValue(s->getStart().getX());
        m_segY1->setValue(s->getStart().getY());
        m_segX2->setValue(s->getEnd().getX());
        m_segY2->setValue(s->getEnd().getY());
        break;
    }
    case PrimitiveType::Circle: {
        m_circleMethodCombo->setCurrentIndex(0);
        auto* c = static_cast<Circle*>(obj);
        m_circCX->setValue(c->getCenter().getX());
        m_circCY->setValue(c->getCenter().getY());
        m_circR->setValue(c->getRadius());
        break;
    }
    case PrimitiveType::Arc: {
        m_arcMethodCombo->setCurrentIndex(0);
        auto* a = static_cast<Arc*>(obj);
        m_arcCX->setValue(a->getCenter().getX());
        m_arcCY->setValue(a->getCenter().getY());
        m_arcR->setValue(a->getRadius());
        double s = a->getStartAngle();
        double sp = a->getSpanAngle();
        if (Point::getAngleUnit() == AngleUnit::Radians) {
            s = qDegreesToRadians(s);
            sp = qDegreesToRadians(sp);
        }
        m_arcStart->setValue(s);
        m_arcSpan->setValue(sp);
        break;
    }
    case PrimitiveType::Rectangle: {
        m_rectMethodCombo->setCurrentIndex(1);
        auto* r = static_cast<Rectangle*>(obj);
        m_rect1PX->setValue(r->getTopLeft().getX());
        m_rect1PY->setValue(r->getTopLeft().getY());
        m_rect1W->setValue(r->getWidth());
        m_rect1H->setValue(r->getHeight());
        m_rectChamfer->setValue(r->getCornerRadius());
        break;
    }
    case PrimitiveType::Ellipse: {
        m_ellMethodCombo->setCurrentIndex(0);
        auto* e = static_cast<Ellipse*>(obj);
        m_ellCX->setValue(e->getCenter().getX());
        m_ellCY->setValue(e->getCenter().getY());
        m_ellRX->setValue(e->getRadiusX());
        m_ellRY->setValue(e->getRadiusY());
        break;
    }
    case PrimitiveType::Polygon: {
        auto* p = static_cast<PolygonObj*>(obj);
        m_polyCX->setValue(p->getCenter().getX());
        m_polyCY->setValue(p->getCenter().getY());
        m_polyR->setValue(p->getRadius());
        m_polySides->setValue(p->getSides());
        int idx = m_polyInscribed->findData(p->isInscribed());
        if(idx >= 0) m_polyInscribed->setCurrentIndex(idx);
        break;
    }
    case PrimitiveType::Spline: {
        auto* sp = static_cast<Spline*>(obj);
        const auto& pts = sp->getPoints();
        clearAllSplinePoints();
        for(const auto& p : pts) {
            onAddSplinePoint();
            m_splineSpinBoxes.back().first->setValue(p.getX());
            m_splineSpinBoxes.back().second->setValue(p.getY());
        }
        break;
    }
    case PrimitiveType::Point: {
        auto* pt = static_cast<PointObject*>(obj);
        m_ptX->setValue(pt->getPosition().getX());
        m_ptY->setValue(pt->getPosition().getY());
        break;
    }
    case PrimitiveType::Dimension: {
        auto* d = static_cast<Dimension*>(obj);
        int typeIdx = m_dimTypeCombo->findData(static_cast<int>(d->getDimensionType()));
        if (typeIdx >= 0) m_dimTypeCombo->setCurrentIndex(typeIdx);
        m_dimValue->setValue(d->measuredValue());
        m_dimTextOverrideEdit->setText(d->getTextOverride());
        int arrowIdx = m_dimArrowCombo->findData(static_cast<int>(d->arrowType()));
        if (arrowIdx >= 0) m_dimArrowCombo->setCurrentIndex(arrowIdx);
        int placementIdx = m_dimArrowPlacementCombo->findData(static_cast<int>(d->arrowPlacement()));
        if (placementIdx >= 0) m_dimArrowPlacementCombo->setCurrentIndex(placementIdx);
        int prefixIdx = m_dimPrefixCombo->findData(static_cast<int>(d->valuePrefix()));
        if (prefixIdx >= 0) m_dimPrefixCombo->setCurrentIndex(prefixIdx);
        m_dimArrowSize->setValue(d->arrowSize());
        m_dimTextHeight->setValue(d->textHeight());
        m_dimTextOffset->setValue(d->textOffset());
        break;
    }
    default: break;
    }
}

void Properties::populateStyleFields(const std::vector<Object*>& objects) {
    if(objects.empty()) return;
    bool sameColor = true;
    bool sameStyle = true;

    QColor firstColor = objects[0]->getColor();
    LineStyle firstStyle = objects[0]->getLineStyle();

    for(auto* obj : objects) {
        if(obj->getColor() != firstColor) sameColor = false;
        if(obj->getLineStyle().type != firstStyle.type) sameStyle = false;
    }

    if (sameColor) {
        m_selectedColor = firstColor;
        m_colorButton->setStyleSheet(QString("background-color: %1").arg(m_selectedColor.name()));
    } else {
        m_colorButton->setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 red, stop:0.5 green, stop:1 blue);");
    }

    if (sameStyle) {
        m_currentStyle = firstStyle;
        m_stylePresetButton->setText(firstStyle.name);
    } else {
        m_stylePresetButton->setText("Разные типы");
    }

    // Слой
    bool sameLayer = true;
    QString firstLayer = objects[0]->getLayer();
    for(auto* obj : objects) {
        if(obj->getLayer() != firstLayer) { sameLayer = false; break; }
    }
    if (sameLayer) {
        m_selectedLayer = firstLayer;
        m_layerCombo->setCurrentText(firstLayer);
        // Добавляем слой в список если его нет
        if (m_layerCombo->findText(firstLayer) == -1)
            m_layerCombo->addItem(firstLayer);
    } else {
        m_layerCombo->setCurrentText("");
    }
}

void Properties::updateObjectGeometry(Object* obj) {
    switch (obj->getType()) {
    case PrimitiveType::Segment: {
        if (auto* s = dynamic_cast<Segment*>(obj)) {
            s->setStart(readPoint(m_segX1, m_segY1));
            s->setEnd(readPoint(m_segX2, m_segY2));
        }
        break;
    }
    case PrimitiveType::Circle: {
        int method = m_circleMethodCombo->currentIndex();
        Point c; double r = 0;
        if (method == 0) {
            c = readPoint(m_circCX, m_circCY);
            r = m_circR->value();
        } else if (method == 1) {
            c = readPoint(m_circCX_D, m_circCY_D);
            r = m_circD->value() / 2.0;
        } else if (method == 2) {
            Point p1 = readPoint(m_circ2P1X, m_circ2P1Y);
            Point p2 = readPoint(m_circ2P2X, m_circ2P2Y);
            c = Point((p1.getX()+p2.getX())/2, (p1.getY()+p2.getY())/2);
            r = MathUtils::dist(p1, p2) / 2.0;
        } else if (method == 3) {
            Point p1 = readPoint(m_circ3P1X, m_circ3P1Y);
            Point p2 = readPoint(m_circ3P2X, m_circ3P2Y);
            Point p3 = readPoint(m_circ3P3X, m_circ3P3Y);
            MathUtils::getCircleFrom3Points(p1, p2, p3, c, r);
        }
        if (auto* circle = dynamic_cast<Circle*>(obj)) {
            circle->setCenter(c);
            circle->setRadius(r);
        }
        break;
    }
    case PrimitiveType::Arc: {
        int method = m_arcMethodCombo->currentIndex();
        Point c; double r=0, start=0, span=0;
        if (method == 0) {
            c = readPoint(m_arcCX, m_arcCY);
            r = m_arcR->value();
            start = m_arcStart->value();
            span = m_arcSpan->value();
            if (Point::getAngleUnit() == AngleUnit::Radians) {
                start = qRadiansToDegrees(start);
                span = qRadiansToDegrees(span);
            }
        } else {
            Point p1 = readPoint(m_arc3P1X, m_arc3P1Y);
            Point p2 = readPoint(m_arc3P2X, m_arc3P2Y);
            Point p3 = readPoint(m_arc3P3X, m_arc3P3Y);
            if (MathUtils::getCircleFrom3Points(p1, p2, p3, c, r)) {
                double aStart = std::atan2(p1.getY() - c.getY(), p1.getX() - c.getX()) * 180 / M_PI;
                double aEnd = std::atan2(p3.getY() - c.getY(), p3.getX() - c.getX()) * 180 / M_PI;
                if (aStart < 0) aStart += 360;
                if (aEnd < 0) aEnd += 360;
                span = aEnd - aStart;
                if (span < 0) span += 360;
                start = aStart;
            }
        }
        if (auto* arc = dynamic_cast<Arc*>(obj)) {
            arc->setCenter(c); arc->setRadius(r);
            arc->setStartAngle(start); arc->setSpanAngle(span);
        }
        break;
    }
    case PrimitiveType::Rectangle: {
        int method = m_rectMethodCombo->currentIndex();
        Point tl; double w=0, h=0, cr = m_rectChamfer->value();
        if (method == 0) {
            Point p1 = readPoint(m_rectP1X, m_rectP1Y);
            Point p2 = readPoint(m_rectP2X, m_rectP2Y);
            tl = Point(std::min(p1.getX(), p2.getX()), std::min(p1.getY(), p2.getY()));
            w = std::abs(p1.getX() - p2.getX());
            h = std::abs(p1.getY() - p2.getY());
        } else if (method == 1) {
            tl = readPoint(m_rect1PX, m_rect1PY);
            w = m_rect1W->value(); h = m_rect1H->value();
        } else {
            Point c = readPoint(m_rectCX, m_rectCY);
            w = m_rectCW->value(); h = m_rectCH->value();
            tl = Point(c.getX() - w/2, c.getY() - h/2);
        }
        if (auto* rect = dynamic_cast<Rectangle*>(obj)) {
            rect->setTopLeft(tl); rect->setWidth(w);
            rect->setHeight(h); rect->setCornerRadius(cr);
        }
        break;
    }
    case PrimitiveType::Ellipse: {
        int method = m_ellMethodCombo->currentIndex();
        Point c; double rx=0, ry=0;
        if (method == 0) {
            c = readPoint(m_ellCX, m_ellCY);
            rx = m_ellRX->value();
            ry = m_ellRY->value();
        } else {
            c = readPoint(m_ell2CX, m_ell2CY);
            Point p1 = readPoint(m_ell2P1X, m_ell2P1Y);
            Point p2 = readPoint(m_ell2P2X, m_ell2P2Y);
            rx = MathUtils::dist(c, p1);
            ry = MathUtils::dist(c, p2);
        }
        if (auto* ell = dynamic_cast<Ellipse*>(obj)) {
            ell->setCenter(c); ell->setRadiusX(rx); ell->setRadiusY(ry);
        }
        break;
    }
    case PrimitiveType::Polygon: {
        if (auto* poly = dynamic_cast<PolygonObj*>(obj)) {
            poly->setCenter(readPoint(m_polyCX, m_polyCY));
            poly->setRadius(m_polyR->value());
            poly->setSides(m_polySides->value());
            poly->setInscribed(m_polyInscribed->currentData().toBool());
        }
        break;
    }
    case PrimitiveType::Spline: {
        if (auto* sp = dynamic_cast<Spline*>(obj)) {
            std::vector<Point> pts;
            for(const auto& pair : m_splineSpinBoxes) {
                pts.push_back(readPoint(pair.first, pair.second));
            }
            sp->setPoints(pts);
        }
        break;
    }
    case PrimitiveType::Point: {
        if (auto* pt = dynamic_cast<PointObject*>(obj)) {
            pt->setPosition(readPoint(m_ptX, m_ptY));
        }
        break;
    }
    case PrimitiveType::Dimension: {
        if (auto* d = dynamic_cast<Dimension*>(obj)) {
            DimensionType requestedType = static_cast<DimensionType>(m_dimTypeCombo->currentData().toInt());
            if (canConvertDimensionType(d->getDimensionType(), requestedType)) {
                d->setDimensionType(requestedType);
            }
            d->setTextOverride(m_dimTextOverrideEdit->text());
            d->setArrowType(static_cast<ArrowType>(m_dimArrowCombo->currentData().toInt()));
            d->setArrowPlacement(static_cast<ArrowPlacement>(m_dimArrowPlacementCombo->currentData().toInt()));
            d->setArrowSize(m_dimArrowSize->value());
            d->setTextHeight(m_dimTextHeight->value());
            d->setTextOffset(m_dimTextOffset->value());
            d->setValuePrefix(static_cast<DimensionValuePrefix>(m_dimPrefixCombo->currentData().toInt()));
            d->setDimensionColor(m_selectedColor);
            d->setTextColor(m_selectedColor);
            d->setExtensionColor(m_selectedColor);
            d->setDimensionLineStyle(m_currentStyle);
            d->setExtensionLineStyle(m_currentStyle);
            d->applyMeasuredValue(m_dimValue->value());
        }
        break;
    }
    default: break;
    }
}

void Properties::onApplyClicked() {
    if (m_isCreationMode) {
        std::unique_ptr<Object> newObj;
        switch(m_activeType) {
        // Создаем объекты по параметрам из полей (нажатие кнопки "Создать")
        case PrimitiveType::Segment: newObj=std::make_unique<Segment>(Point(), Point()); break;
        case PrimitiveType::Circle: newObj = std::make_unique<Circle>(Point(), 10); break;
        case PrimitiveType::Arc: newObj = std::make_unique<Arc>(Point(), 10, 0, 90); break;
        case PrimitiveType::Rectangle: newObj = std::make_unique<Rectangle>(Point(), 10, 10); break;
        case PrimitiveType::Ellipse: newObj = std::make_unique<Ellipse>(Point(), 10, 5); break;
        case PrimitiveType::Polygon: newObj = std::make_unique<PolygonObj>(Point(), 10, 5); break;
        case PrimitiveType::Spline: newObj = std::make_unique<Spline>(std::vector<Point>()); break;
        case PrimitiveType::Point: newObj = std::make_unique<PointObject>(Point()); break;
        default: break;
        }

        if (newObj) {
            updateObjectGeometry(newObj.get());
            applyCurrentStyleTo(newObj.get());
            emit objectCreateRequested(newObj.release());
        }
    } else {
        for (auto* obj : m_currentObjects) {
            if (m_stylePresetButton->text() != "Разные типы") {
                auto st = obj->getLineStyle();
                st.type = m_currentStyle.type; st.name = m_currentStyle.name;
                st.dashLength = m_currentStyle.dashLength; st.gapLength = m_currentStyle.gapLength;
                obj->setLineStyle(st);
            }
            if (m_selectedColor.isValid()) {
                obj->setColor(m_selectedColor);
            }
            // Применяем слой
            QString layerText = m_layerCombo->currentText().trimmed();
            if (!layerText.isEmpty()) {
                obj->setLayer(layerText);
            }
            if (m_currentObjects.size() == 1) updateObjectGeometry(obj);
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
    
    // Индекс для пользовательских стилей
    int customIndex = 0;
    
    for(size_t i = 0; i < m_availableStyles.size(); ++i) {
        const auto& s = m_availableStyles[i];
        
        if (s.type == LineStyleType::Custom) {
            // Для пользовательских стилей создаём подменю с действиями
            QMenu* subMenu = menu.addMenu(s.name);
            
            // Выбрать стиль
            subMenu->addAction("Выбрать", this, [this, s](){
                m_currentStyle = s;
                m_stylePresetButton->setText(s.name);
            });
            
            subMenu->addSeparator();
            
            // Редактировать стиль
            int idx = i;
            subMenu->addAction("Редактировать", this, [this, idx](){
                onEditCustomStyle(idx);
            });
            
            // Удалить стиль
            subMenu->addAction("Удалить", this, [this, idx](){
                onDeleteCustomStyle(idx);
            });
            
            customIndex++;
        } else {
            // Для стандартных стилей — действие с иконкой
            QString iconPath = LineSettingsMenu::getIconPath(s.type);
            QAction* action = menu.addAction(QIcon(iconPath), s.name, this, [this, s](){
                m_currentStyle = s;
                m_stylePresetButton->setText(s.name);
            });
        }
    }
    
    menu.addSeparator();
    menu.addAction("Добавить...", this, &Properties::onAddCustomStyle);
    menu.exec(QCursor::pos());
}

void Properties::onAddCustomStyle() {
    StyleDialog dlg(this);
    if(dlg.exec()) m_availableStyles.push_back(dlg.getStyle());
}

void Properties::onEditCustomStyle(int index) {
    if (index < 0 || index >= static_cast<int>(m_availableStyles.size())) return;
    
    StyleDialog dlg(m_availableStyles[index], this);
    if (dlg.exec()) {
        m_availableStyles[index] = dlg.getStyle();
        // Если текущий стиль был изменён, обновляем кнопку
        if (m_currentStyle.name == m_availableStyles[index].name) {
            m_currentStyle = m_availableStyles[index];
            m_stylePresetButton->setText(m_currentStyle.name);
        }
    }
}

void Properties::onDeleteCustomStyle(int index) {
    if (index < 0 || index >= static_cast<int>(m_availableStyles.size())) return;
    
    // Если удаляемый стиль — текущий, переключаемся на первый стандартный
    if (m_currentStyle.name == m_availableStyles[index].name) {
        if (!m_availableStyles.empty()) {
            m_currentStyle = m_availableStyles[0];
            m_stylePresetButton->setText(m_currentStyle.name);
        }
    }
    
    m_availableStyles.erase(m_availableStyles.begin() + index);
}
