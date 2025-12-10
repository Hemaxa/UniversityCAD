#include "Segment.h"

// Конструктор инициализирует начальную и конечную точки.
Segment::Segment(const Point& start, const Point& end) : m_start(start), m_end(end) {}

// Возвращает ссылку на начальную точку.
const Point& Segment::getStart() const
{
    return m_start;
}

// Задает новую начальную точку.
void Segment::setStart(const Point& point)
{
    m_start = point;
}

// Возвращает ссылку на конечную точку.
const Point& Segment::getEnd() const
{
    return m_end;
}

// Задает новую конечную точку.
void Segment::setEnd(const Point& point)
{
    m_end = point;
}
