#include "Polygon.h"
#include "MathUtils.h"
#include <cmath>
#include <vector>

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
        Point p(m_center.getX() + r * std::cos(angle),
                m_center.getY() + r * std::sin(angle));
        snaps.push_back({p, SnapType::Endpoint});
    }
    return snaps;
}

Point PolygonObj::getClosestPoint(const Point& p) const {
    // Получаем вершины (повторяем логику getSnapPoints)
    int sides = std::max(3, m_sides);
    double step = 2 * M_PI / sides;
    double startAngle = M_PI / 2;
    double r = m_radius;
    if (!m_inscribed) r = r / std::cos(M_PI / sides);

    std::vector<Point> vertices;
    for (int i = 0; i < sides; ++i) {
        double angle = startAngle + i * step;
        vertices.emplace_back(m_center.getX() + r * std::cos(angle),
                              m_center.getY() + r * std::sin(angle));
    }

    // Ищем проекцию на ближайший сегмент
    Point closestPoint = vertices[0];
    double minDistSq = 1e15;

    for (int i = 0; i < sides; ++i) {
        Point p1 = vertices[i];
        Point p2 = vertices[(i + 1) % sides];
        Point proj = MathUtils::projectPointOnSegment(p, p1, p2);
        double d = MathUtils::distSq(p, proj);
        if (d < minDistSq) {
            minDistSq = d;
            closestPoint = proj;
        }
    }
    return closestPoint;
}
