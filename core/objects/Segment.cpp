#include "Segment.h"

// Конструктор класса Segment.
Segment::Segment(const Point& start, const Point& end) : m_start(start), m_end(end) {}
// Возвращает начальную точку отрезка.
const Point& Segment::getStart() const { return m_start; }
// Устанавливает начальную точку отрезка.
void Segment::setStart(const Point& point) { m_start = point; }
// Возвращает конечную точку отрезка.
const Point& Segment::getEnd() const { return m_end; }
// Устанавливает конечную точку отрезка.
void Segment::setEnd(const Point& point) { m_end = point; }
