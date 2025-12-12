#include "Polygon.h"
#include <cmath>

PolygonObj::PolygonObj(const Point& center, double radius, int sides, bool inscribed)
    : m_center(center), m_radius(radius), m_sides(sides), m_inscribed(inscribed) {}

const Point& PolygonObj::getCenter() const { return m_center; }
void PolygonObj::setCenter(const Point& p) { m_center = p; }

double PolygonObj::getRadius() const { return m_radius; }
void PolygonObj::setRadius(double r) { m_radius = r; }

int PolygonObj::getSides() const { return m_sides; }
void PolygonObj::setSides(int s) { m_sides = s; }

bool PolygonObj::isInscribed() const { return m_inscribed; }
void PolygonObj::setInscribed(bool i) { m_inscribed = i; }

std::vector<SnapPoint> PolygonObj::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    snaps.push_back({m_center, SnapType::Center});

    int sides = std::max(3, m_sides);
    double step = 2 * M_PI / sides;
    double startAngle = M_PI / 2;
    double r = m_radius;
    if (!m_inscribed) r = r / std::cos(M_PI / sides);

    for (int i = 0; i < sides; ++i) {
        double angle = startAngle + i * step;
        snaps.push_back({Point(m_center.getX() + r * std::cos(angle),
                               m_center.getY() + r * std::sin(angle)), SnapType::Endpoint});
    }
    return snaps;
}
