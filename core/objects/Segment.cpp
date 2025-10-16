#include "Segment.h"

Segment::Segment(const Point& start, const Point& end) : m_start(start), m_end(end) {}
const Point& Segment::getStart() const { return m_start; }
void Segment::setStart(const Point& point) { m_start = point; }
const Point& Segment::getEnd() const { return m_end; }
void Segment::setEnd(const Point& point) { m_end = point; }
