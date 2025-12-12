#include "Segment.h"

Segment::Segment(const Point& start, const Point& end) : m_start(start), m_end(end) {}

const Point& Segment::getStart() const { return m_start; }
void Segment::setStart(const Point& point) { m_start = point; }

const Point& Segment::getEnd() const { return m_end; }
void Segment::setEnd(const Point& point) { m_end = point; }

std::vector<SnapPoint> Segment::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    snaps.push_back({m_start, SnapType::Endpoint});
    snaps.push_back({m_end, SnapType::Endpoint});
    snaps.push_back({Point((m_start.getX() + m_end.getX()) / 2,
                           (m_start.getY() + m_end.getY()) / 2), SnapType::Midpoint});
    return snaps;
}
