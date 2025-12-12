#include "PrimitiveStrategies.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <vector>

// =========================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (Utils)
// =========================================================

// Настройка пера (Pen) на основе стиля объекта
static void setupPen(QPainter& painter, const Object* obj, bool isSelected) {
    const LineStyle& style = obj->getLineStyle();
    QPen pen;
    pen.setColor(obj->getColor());
    pen.setWidthF(style.width);
    pen.setCapStyle(Qt::RoundCap);

    QVector<qreal> dashes;
    double w = (style.width > 0.001) ? style.width : 1.0;

    switch (style.type) {
    case LineStyleType::Solid:
    case LineStyleType::SolidWavy:  // Волнистость рисуется геометрией, перо Solid
    case LineStyleType::SolidZigZag:
        pen.setStyle(Qt::SolidLine);
        break;
    case LineStyleType::Dashed:
    case LineStyleType::Custom:
        pen.setStyle(Qt::CustomDashLine);
        dashes << (style.dashLength / w) << (style.gapLength / w);
        pen.setDashPattern(dashes);
        break;
    case LineStyleType::DashDot:
        pen.setStyle(Qt::CustomDashLine);
        dashes << (style.dashLength / w) << (style.gapLength / w) << 1.0 << (style.gapLength / w);
        pen.setDashPattern(dashes);
        break;
    case LineStyleType::DashDotDot:
        pen.setStyle(Qt::CustomDashLine);
        dashes << (style.dashLength / w) << (style.gapLength / w)
               << 1.0 << (style.gapLength / w) << 1.0 << (style.gapLength / w);
        pen.setDashPattern(dashes);
        break;
    }

    // Подсветка при выделении
    if (isSelected) {
        pen.setColor(QColor("#F92672"));
        // Делаем линию чуть толще для визуала выделения
        pen.setWidthF(pen.widthF() + 1.0);
    }

    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
}

// Генерация пути для волнистой линии
static QPainterPath createWavyPath(const QPointF& start, const QPointF& end, double width) {
    QPainterPath path;
    path.moveTo(start);
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);
    if (length < 1e-6) return path;

    QTransform t;
    t.translate(start.x(), start.y());
    t.rotateRadians(angle);

    double amplitude = width * 1.5; if (amplitude < 2.0) amplitude = 2.0;
    double period = 10.0;
    int steps = static_cast<int>(length / (period / 8.0));
    if (steps < 2) steps = 2;

    for (int i = 0; i <= steps; ++i) {
        double x = (double)i / steps * length;
        double y = amplitude * std::sin(x * 2 * M_PI / period);
        path.lineTo(t.map(QPointF(x, y)));
    }
    return path;
}

// Генерация пути для зигзага
static QPainterPath createZigZagPath(const QPointF& start, const QPointF& end, double width) {
    QPainterPath path;
    path.moveTo(start);
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);
    if (length < 1e-6) return path;

    QTransform t;
    t.translate(start.x(), start.y());
    t.rotateRadians(angle);

    double amplitude = width * 1.5; if (amplitude < 2.0) amplitude = 2.0;
    double period = 10.0;
    double currentX = 0;
    bool up = true;
    while (currentX < length) {
        currentX += period / 2.0;
        double y = up ? amplitude : -amplitude;
        if (currentX > length) { currentX = length; y = 0; }
        path.lineTo(t.map(QPointF(currentX, y)));
        up = !up;
    }
    path.lineTo(end);
    return path;
}

// =========================================================
// РЕАЛИЗАЦИЯ СТРАТЕГИЙ
// =========================================================

// --- Segment ---
void SegmentDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* s = static_cast<Segment*>(primitive);
    setupPen(painter, s, isSelected);

    QPointF start(s->getStart().getX(), s->getStart().getY());
    QPointF end(s->getEnd().getX(), s->getEnd().getY());
    auto styleType = s->getLineStyle().type;

    if (styleType == LineStyleType::SolidWavy) {
        painter.drawPath(createWavyPath(start, end, s->getLineStyle().width));
    } else if (styleType == LineStyleType::SolidZigZag) {
        painter.drawPath(createZigZagPath(start, end, s->getLineStyle().width));
    } else {
        painter.drawLine(start, end);
    }
}

// --- Circle ---
void CircleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Circle*>(primitive);
    setupPen(painter, obj, isSelected);
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()),
                        obj->getRadius(), obj->getRadius());
}

// --- Arc ---
void ArcDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Arc*>(primitive);
    setupPen(painter, obj, isSelected);
    // Qt drawArc принимает прямоугольник, описывающий эллипс, и углы в 1/16 градуса
    double r = obj->getRadius();
    QRectF rect(obj->getCenter().getX() - r, obj->getCenter().getY() - r, r * 2, r * 2);
    // startAngle и spanAngle в градусах
    painter.drawArc(rect, int(obj->getStartAngle() * 16), int(obj->getSpanAngle() * 16));
}

// --- Rectangle ---
void RectangleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    // ИСПРАВЛЕНО: cast к Rectangle* (был RectanglePrim*)
    auto* obj = static_cast<Rectangle*>(primitive);
    setupPen(painter, obj, isSelected);
    // TopLeft в Qt - это верхний левый экранный
    painter.drawRoundedRect(QRectF(obj->getTopLeft().getX(), obj->getTopLeft().getY(),
                                   obj->getWidth(), obj->getHeight()),
                            obj->getCornerRadius(), obj->getCornerRadius());
}

// --- Ellipse ---
void EllipseDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Ellipse*>(primitive);
    setupPen(painter, obj, isSelected);
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()),
                        obj->getRadiusX(), obj->getRadiusY());
}

// --- Polygon ---
void PolygonDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<PolygonObj*>(primitive);
    setupPen(painter, obj, isSelected);

    QPolygonF poly;
    int sides = std::max(3, obj->getSides());
    double step = 2 * M_PI / sides;
    double startAngle = M_PI / 2; // Вершина сверху
    double r = obj->getRadius();

    // Если "Описанный" (inscribed = false), радиус окружности - это расстояние до грани,
    // значит расстояние до вершины будет r / cos(PI/N)
    if (!obj->isInscribed()) {
        r = r / std::cos(M_PI / sides);
    }

    for (int i = 0; i < sides; ++i) {
        double angle = startAngle + i * step;
        poly << QPointF(obj->getCenter().getX() + r * std::cos(angle),
                        obj->getCenter().getY() + r * std::sin(angle));
    }
    painter.drawPolygon(poly);
}

// --- Spline ---
void SplineDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Spline*>(primitive);
    const auto& points = obj->getPoints();
    if (points.size() < 2) return;

    setupPen(painter, obj, isSelected);
    QPainterPath path;
    path.moveTo(points[0].getX(), points[0].getY());

    // Простая отрисовка полилинией для наглядности, либо cubicTo
    // Для полноценного сплайна Catmull-Rom здесь нужен алгоритм интерполяции.
    // Используем упрощенный вариант: соединяем QuadTo через средние точки
    for (size_t i = 0; i < points.size() - 1; ++i) {
        QPointF p1(points[i].getX(), points[i].getY());
        QPointF p2(points[i+1].getX(), points[i+1].getY());

        // Линейная интерполяция (самый простой сплайн 1-го порядка)
        path.lineTo(p2);
    }
    painter.drawPath(path);

    // Рисуем контрольные точки при выделении
    if (isSelected) {
        painter.setBrush(QColor("#F92672"));
        painter.setPen(Qt::NoPen);
        for(const auto& p : points) {
            painter.drawEllipse(QPointF(p.getX(), p.getY()), 2.0, 2.0);
        }
    }
}
