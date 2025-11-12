#include "SegmentDraw.h"
#include "Segment.h"

#include <QPainter>
#include <QPen>

//Метод отрисовки отрезка
void SegmentDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const
{
    auto* segment = static_cast<Segment*>(primitive);
    if (!segment) return;

    // Координаты для удобства
    const QPointF start(segment->getStart().getX(), segment->getStart().getY());
    const QPointF end(segment->getEnd().getX(), segment->getEnd().getY());

    // 1. Отрисовка стандартной линии
    painter.setPen(QPen(segment->getColor(), 1.5));
    painter.drawLine(start, end);

    // 2. Отрисовка подсветки, если объект выбран
    if (isSelected) {
        QColor highlightColor = segment->getColor();
        highlightColor.setAlpha(100); // Задаем прозрачность (0-255)

        QPen highlightPen(highlightColor, 6.0, Qt::SolidLine, Qt::RoundCap);
        painter.setPen(highlightPen);
        painter.drawLine(start, end);
    }
}
