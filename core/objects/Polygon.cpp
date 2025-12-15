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

// Вспомогательная функция для получения вершин многоугольника
std::vector<Point> PolygonObj::getVertices() const {
    std::vector<Point> vertices;
    int sides = std::max(3, m_sides);
    double step = 2 * M_PI / sides;
    double startAngle = M_PI / 2;
    double r = m_radius;
    if (!m_inscribed) r = r / std::cos(M_PI / sides);

    for (int i = 0; i < sides; ++i) {
        double angle = startAngle + i * step;
        vertices.emplace_back(m_center.getX() + r * std::cos(angle),
                              m_center.getY() + r * std::sin(angle));
    }
    return vertices;
}

std::vector<SnapPoint> PolygonObj::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    
    // Центр
    snaps.push_back({m_center, SnapType::Center});

    auto vertices = getVertices();
    int sides = static_cast<int>(vertices.size());

    // Вершины (концы)
    for (const auto& v : vertices) {
        snaps.push_back({v, SnapType::Endpoint});
    }

    // Середины сторон
    for (int i = 0; i < sides; ++i) {
        Point p1 = vertices[i];
        Point p2 = vertices[(i + 1) % sides];
        Point mid((p1.getX() + p2.getX()) / 2.0, (p1.getY() + p2.getY()) / 2.0);
        snaps.push_back({mid, SnapType::Midpoint});
    }

    return snaps;
}

Point PolygonObj::getClosestPoint(const Point& p) const {
    auto vertices = getVertices();
    int sides = static_cast<int>(vertices.size());

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

std::optional<Point> PolygonObj::getPerpendicularPoint(const Point& p) const {
    auto vertices = getVertices();
    int sides = static_cast<int>(vertices.size());

    Point bestPerp;
    double minDist = 1e15;
    bool found = false;

    for (int i = 0; i < sides; ++i) {
        Point p1 = vertices[i];
        Point p2 = vertices[(i + 1) % sides];
        
        Point proj = MathUtils::projectPointOnLine(p, p1, p2);
        
        // Проверяем, что проекция попадает на сторону
        double dSq = MathUtils::distSq(p1, p2);
        if (dSq < 1e-9) continue;
        
        double t = ((proj.getX() - p1.getX()) * (p2.getX() - p1.getX()) +
                    (proj.getY() - p1.getY()) * (p2.getY() - p1.getY())) / dSq;
        
        if (t >= 0.0 && t <= 1.0) {
            double d = MathUtils::dist(p, proj);
            if (d < minDist) {
                minDist = d;
                bestPerp = proj;
                found = true;
            }
        }
    }

    if (found) return bestPerp;
    return std::nullopt;
}
