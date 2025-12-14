#include "Segment.h"
#include "MathUtils.h"

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

Point Segment::getClosestPoint(const Point& p) const {
    return MathUtils::projectPointOnSegment(p, m_start, m_end);
}

std::optional<Point> Segment::getPerpendicularPoint(const Point& p) const {
    Point proj = MathUtils::projectPointOnLine(p, m_start, m_end);
    // Проверяем, падает ли перпендикуляр на сам отрезок
    // (в классическом CAD перпендикуляр часто работает и к продолжению линии,
    // но обычно привязка показывается только если попадаем в пределы отрезка)

    // Проверка попадания в пределы:
    double dSq = MathUtils::distSq(m_start, m_end);
    if (dSq < 1e-9) return std::nullopt; // Отрезок вырожден в точку

    double t = ((proj.getX() - m_start.getX()) * (m_end.getX() - m_start.getX()) +
                (proj.getY() - m_start.getY()) * (m_end.getY() - m_start.getY())) / dSq;

    if (t >= 0.0 && t <= 1.0) {
        return proj;
    }
    return std::nullopt;
}
