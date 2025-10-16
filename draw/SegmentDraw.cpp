#include "SegmentDraw.h"
#include "Segment.h"

#include <QPainter>
#include <QPen>

//Метод отрисовки отрезка
void SegmentDraw::draw(QPainter& painter, Object* primitive) const
{
    auto* segment = static_cast<Segment*>(primitive);
    if (!segment) return;

    painter.setPen(QPen(segment->getColor(), 1.5));
    painter.drawLine(
        QPointF(segment->getStart().getX(), segment->getStart().getY()),
        QPointF(segment->getEnd().getX(), segment->getEnd().getY())
        );
}
