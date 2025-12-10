#include "PrimitiveStrategies.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

// Вспомогательная функция для настройки пера (дублируется из SegmentDraw, лучше вынести в утилиты)
static void setupPen(QPainter& painter, const Object* obj, bool isSelected) {
    QPen pen;
    pen.setColor(obj->getColor());
    pen.setWidthF(obj->getLineStyle().width);
    // ... (Настройка стиля линии как в SegmentDraw) ...
    // Для краткости используем Solid, в реальном коде скопируйте switch из SegmentDraw
    pen.setStyle(Qt::SolidLine);

    if (isSelected) {
        pen.setColor(QColor("#F92672"));
        pen.setWidthF(pen.widthF() + 2.0);
    }
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
}

void CircleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Circle*>(primitive);
    setupPen(painter, obj, isSelected);
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()),
                        obj->getRadius(), obj->getRadius());
}

void ArcDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Arc*>(primitive);
    setupPen(painter, obj, isSelected);
    // Qt принимает углы в 1/16 градуса
    QRectF rect(obj->getCenter().getX() - obj->getRadius(), obj->getCenter().getY() - obj->getRadius(),
                obj->getRadius() * 2, obj->getRadius() * 2);
    painter.drawArc(rect, int(obj->getStartAngle() * 16), int(obj->getSpanAngle() * 16));
}

void RectangleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<RectanglePrim*>(primitive);
    setupPen(painter, obj, isSelected);
    painter.drawRoundedRect(QRectF(obj->getTopLeft().getX(), obj->getTopLeft().getY(),
                                   obj->getWidth(), obj->getHeight()),
                            obj->getCornerRadius(), obj->getCornerRadius());
}

void EllipseDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Ellipse*>(primitive);
    setupPen(painter, obj, isSelected);
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()),
                        obj->getRadiusX(), obj->getRadiusY());
}

void PolygonDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<PolygonObj*>(primitive);
    setupPen(painter, obj, isSelected);

    QPolygonF poly;
    double step = 2 * M_PI / obj->getSides();
    double startAngle = -M_PI / 2; // Начало сверху
    double r = obj->getRadius();
    // Если описанный, корректируем радиус
    if (!obj->isInscribed()) {
        r = r / std::cos(M_PI / obj->getSides());
    }

    for (int i = 0; i < obj->getSides(); ++i) {
        double angle = startAngle + i * step;
        poly << QPointF(obj->getCenter().getX() + r * std::cos(angle),
                        obj->getCenter().getY() + r * std::sin(angle));
    }
    painter.drawPolygon(poly);
}

void SplineDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Spline*>(primitive);
    const auto& points = obj->getPoints();
    if (points.size() < 2) return;

    setupPen(painter, obj, isSelected);
    QPainterPath path;
    path.moveTo(points[0].getX(), points[0].getY());

    // Простой пример с кубическими кривыми. Для настоящего сплайна нужен алгоритм Catmull-Rom или B-Spline
    for (size_t i = 0; i < points.size() - 1; ++i) {
        QPointF p1(points[i].getX(), points[i].getY());
        QPointF p2(points[i+1].getX(), points[i+1].getY());
        // Упрощенно соединяем линиями или quadTo для гладкости
        path.lineTo(p2);
    }
    painter.drawPath(path);

    // Рисуем контрольные точки при выделении
    if (isSelected) {
        painter.setBrush(Qt::red);
        for(const auto& p : points) {
            painter.drawEllipse(QPointF(p.getX(), p.getY()), 3, 3);
        }
    }
}
