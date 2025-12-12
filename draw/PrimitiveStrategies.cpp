#include "PrimitiveStrategies.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <vector>

// =========================================================
// Настройка пера по ГОСТ 2.303-68
// =========================================================
static void setupPen(QPainter& painter, const Object* obj, bool isSelected) {
    const LineStyle& style = obj->getLineStyle();
    QPen pen;
    pen.setColor(obj->getColor());
    pen.setCapStyle(Qt::FlatCap); // ГОСТ обычно требует ровные концы
    pen.setCosmetic(true);        // ВАЖНО: Толщина в пикселях не зависит от зума

    // Базовые толщины (S - основная)
    double s = (style.width > 0) ? style.width : 2.0;
    double s_thin = s / 2.0;
    if (s_thin < 1.0) s_thin = 1.0;

    QVector<qreal> dashes;

    switch (style.type) {
    case LineStyleType::SolidMain:
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(s);
        break;

    case LineStyleType::SolidThin:
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(s_thin);
        break;

    case LineStyleType::SolidWavy:
    case LineStyleType::SolidZigZag:
        // Эти линии рисуются геометрией (path), перо нужно тонкое
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(s_thin);
        break;

    case LineStyleType::Dashed: // Штриховая
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s_thin);
        // Штрихи ~4-6 мм, пробелы 1-2 мм. В пикселях возьмем пропорцию
        dashes << 8 << 3;
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::DashDotThin: // Штрихпунктирная тонкая
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s_thin);
        dashes << 10 << 3 << 1 << 3;
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::DashDotThick: // Штрихпунктирная утолщенная
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s); // Толстая
        dashes << 8 << 3 << 1 << 3;
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::DashDotDot: // С двумя точками
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s_thin);
        dashes << 10 << 3 << 1 << 2 << 1 << 3;
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::Custom:
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(style.width);
        dashes << style.dashLength << style.gapLength;
        pen.setDashPattern(dashes);
        break;
    }

    if (isSelected) {
        pen.setColor(QColor("#F92672"));
        // pen.setWidthF(pen.widthF() + 1.0); // Можно не утолщать, цвет уже выделяет
    }

    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
}

// Функции путей для волны и зигзага нужно адаптировать под косметический масштаб
static QPainterPath createWavyPath(const QPointF& start, const QPointF& end, double zoomFactor) {
    QPainterPath path;
    path.moveTo(start);
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);
    if (length < 1e-6) return path;

    QTransform t; t.translate(start.x(), start.y()); t.rotateRadians(angle);

    // Амплитуда и период должны быть константными на экране, значит в мире они зависят от 1/zoom
    double screenAmp = 3.0; // px
    double screenPeriod = 10.0; // px

    double worldAmp = screenAmp / zoomFactor;
    double worldPeriod = screenPeriod / zoomFactor;

    int steps = static_cast<int>(length / (worldPeriod / 8.0));
    if (steps < 2) steps = 2;

    for (int i = 0; i <= steps; ++i) {
        double x = (double)i / steps * length;
        double y = worldAmp * std::sin(x * 2 * M_PI / worldPeriod);
        path.lineTo(t.map(QPointF(x, y)));
    }
    return path;
}

static QPainterPath createZigZagPath(const QPointF& start, const QPointF& end, double zoomFactor) {
    QPainterPath path; path.moveTo(start);
    double dx = end.x() - start.x(); double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy); double angle = std::atan2(dy, dx);
    if (length < 1e-6) return path;

    QTransform t; t.translate(start.x(), start.y()); t.rotateRadians(angle);
    double worldAmp = 4.0 / zoomFactor;
    double worldPeriod = 10.0 / zoomFactor;

    double currentX = 0; bool up = true;
    while (currentX < length) {
        currentX += worldPeriod / 2.0;
        double y = up ? worldAmp : -worldAmp;
        if (currentX > length) { currentX = length; y = 0; }
        path.lineTo(t.map(QPointF(currentX, y)));
        up = !up;
    }
    path.lineTo(end);
    return path;
}

// Получение зум-фактора из трансформации
static double getScale(const QPainter& p) {
    return p.transform().m11(); // Упрощенно, предполагая равномерное масштабирование
}

// --- Segment ---
void SegmentDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* s = static_cast<Segment*>(primitive);
    setupPen(painter, s, isSelected);
    QPointF start(s->getStart().getX(), s->getStart().getY());
    QPointF end(s->getEnd().getX(), s->getEnd().getY());
    auto styleType = s->getLineStyle().type;

    if (styleType == LineStyleType::SolidWavy) {
        painter.drawPath(createWavyPath(start, end, getScale(painter)));
    } else if (styleType == LineStyleType::SolidZigZag) {
        painter.drawPath(createZigZagPath(start, end, getScale(painter)));
    } else {
        painter.drawLine(start, end);
    }
}

// ... Остальные методы отрисовки аналогично обновляют setupPen ...
void CircleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Circle*>(primitive); setupPen(painter, obj, isSelected);
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()), obj->getRadius(), obj->getRadius());
}
void ArcDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Arc*>(primitive); setupPen(painter, obj, isSelected);
    double r = obj->getRadius();
    QRectF rect(obj->getCenter().getX() - r, obj->getCenter().getY() - r, r * 2, r * 2);
    painter.drawArc(rect, int(obj->getStartAngle() * 16), int(obj->getSpanAngle() * 16));
}
void RectangleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Rectangle*>(primitive); setupPen(painter, obj, isSelected);
    painter.drawRoundedRect(QRectF(obj->getTopLeft().getX(), obj->getTopLeft().getY(), obj->getWidth(), obj->getHeight()), obj->getCornerRadius(), obj->getCornerRadius());
}
void EllipseDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Ellipse*>(primitive); setupPen(painter, obj, isSelected);
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()), obj->getRadiusX(), obj->getRadiusY());
}
void PolygonDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<PolygonObj*>(primitive); setupPen(painter, obj, isSelected);
    QPolygonF poly;
    int sides = std::max(3, obj->getSides()); double step = 2 * M_PI / sides; double startAngle = M_PI / 2; double r = obj->getRadius();
    if (!obj->isInscribed()) r = r / std::cos(M_PI / sides);
    for (int i = 0; i < sides; ++i) { double angle = startAngle + i * step; poly << QPointF(obj->getCenter().getX() + r * std::cos(angle), obj->getCenter().getY() + r * std::sin(angle)); }
    painter.drawPolygon(poly);
}
void SplineDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Spline*>(primitive);
    const auto& points = obj->getPoints(); if (points.size() < 2) return;
    setupPen(painter, obj, isSelected);
    QPainterPath path; path.moveTo(points[0].getX(), points[0].getY());
    for (size_t i = 0; i < points.size() - 1; ++i) { QPointF p2(points[i+1].getX(), points[i+1].getY()); path.lineTo(p2); }
    painter.drawPath(path);
    if (isSelected) {
        painter.setBrush(QColor("#F92672")); painter.setPen(Qt::NoPen);
        double s = 4.0 / getScale(painter);
        for(const auto& p : points) painter.drawEllipse(QPointF(p.getX(), p.getY()), s/2, s/2);
    }
}
