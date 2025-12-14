#include "Circle.h"
#include "MathUtils.h"

Circle::Circle(const Point& center, double radius) : m_center(center), m_radius(radius) {}

const Point& Circle::getCenter() const { return m_center; }
void Circle::setCenter(const Point& p) { m_center = p; }

double Circle::getRadius() const { return m_radius; }
void Circle::setRadius(double r) { m_radius = r; }

std::vector<SnapPoint> Circle::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    snaps.push_back({m_center, SnapType::Center});
    snaps.push_back({Point(m_center.getX() + m_radius, m_center.getY()), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX() - m_radius, m_center.getY()), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX(), m_center.getY() + m_radius), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX(), m_center.getY() - m_radius), SnapType::Quadrant});
    return snaps;
}

Point Circle::getClosestPoint(const Point& p) const {
    double dx = p.getX() - m_center.getX();
    double dy = p.getY() - m_center.getY();
    double len = std::sqrt(dx*dx + dy*dy);
    if (len < 1e-9) return Point(m_center.getX() + m_radius, m_center.getY()); // Degenerate case
    return Point(m_center.getX() + dx/len * m_radius, m_center.getY() + dy/len * m_radius);
}

std::vector<Point> Circle::getTangentPoints(const Point& p) const {
    return MathUtils::getTangentPoints(p, m_center, m_radius);
}
