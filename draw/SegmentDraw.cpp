#include "SegmentDraw.h"
#include "Segment.h"

#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QtMath>

// Генерирует путь для волнистой линии (синусоиды).
static QPainterPath createWavyPath(const QPointF& start, const QPointF& end, double amplitude, double period) {
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

    int steps = static_cast<int>(length / (period / 8.0));
    if (steps < 2) steps = 2;

    for (int i = 0; i <= steps; ++i) {
        double x = (double)i / steps * length;
        double y = amplitude * std::sin(x * 2 * M_PI / period);
        path.lineTo(t.map(QPointF(x, y)));
    }
    return path;
}

// Генерирует путь для линии с изломами (зигзаг).
static QPainterPath createZigZagPath(const QPointF& start, const QPointF& end, double amplitude, double period) {
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

// Основной метод отрисовки отрезка.
void SegmentDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const
{
    auto* segment = static_cast<Segment*>(primitive);
    if (!segment) return;

    const QPointF start(segment->getStart().getX(), segment->getStart().getY());
    const QPointF end(segment->getEnd().getX(), segment->getEnd().getY());
    const LineStyle& style = segment->getLineStyle();

    QPen pen;
    pen.setColor(segment->getColor());
    pen.setWidthF(style.width);
    pen.setCapStyle(Qt::FlatCap);

    QVector<qreal> dashes;
    double w = (style.width > 0.001) ? style.width : 1.0;

    // Параметры для сложных линий
    double amplitude = style.width * 1.5;
    if (amplitude < 1.0) amplitude = 1.0;
    double period = 10.0;

    switch (style.type) {
    case LineStyleType::Solid:
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawLine(start, end);
        break;

    case LineStyleType::Dashed:
    case LineStyleType::Custom:
        pen.setStyle(Qt::CustomDashLine);
        dashes << (style.dashLength / w) << (style.gapLength / w);
        pen.setDashPattern(dashes);
        painter.setPen(pen);
        painter.drawLine(start, end);
        break;

    case LineStyleType::DashDot:
        pen.setStyle(Qt::CustomDashLine);
        dashes << (style.dashLength / w) << (style.gapLength / w) << 1.0 << (style.gapLength / w);
        pen.setDashPattern(dashes);
        painter.setPen(pen);
        painter.drawLine(start, end);
        break;

    case LineStyleType::DashDotDot:
        pen.setStyle(Qt::CustomDashLine);
        dashes << (style.dashLength / w) << (style.gapLength / w)
               << 1.0 << (style.gapLength / w) << 1.0 << (style.gapLength / w);
        pen.setDashPattern(dashes);
        painter.setPen(pen);
        painter.drawLine(start, end);
        break;

    case LineStyleType::SolidWavy:
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawPath(createWavyPath(start, end, amplitude, period));
        break;

    case LineStyleType::SolidZigZag:
        pen.setStyle(Qt::SolidLine);
        painter.setPen(pen);
        painter.drawPath(createZigZagPath(start, end, amplitude, period));
        break;
    }

    // Отрисовка подсветки при выделении
    if (isSelected) {
        QColor highlightColor = QColor("#F92672");
        highlightColor.setAlpha(80);
        QPen highlightPen(highlightColor, style.width + 4.0, Qt::SolidLine, Qt::RoundCap);
        highlightPen.setCosmetic(true);
        painter.setPen(highlightPen);
        painter.drawLine(start, end);
    }
}
