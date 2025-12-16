#include "Spline.h"
#include "MathUtils.h"

Spline::Spline(const std::vector<Point>& points) : m_points(points) {}

const std::vector<Point>& Spline::getPoints() const { return m_points; }
void Spline::setPoints(const std::vector<Point>& points) { m_points = points; }
void Spline::addPoint(const Point& p) { m_points.push_back(p); }

std::vector<SnapPoint> Spline::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    
    // Привязка к контрольным точкам (концы)
    for(size_t i = 0; i < m_points.size(); ++i) {
        // Первая и последняя точки - Endpoint, остальные тоже Endpoint (контрольные)
        snaps.push_back({m_points[i], SnapType::Endpoint});
    }
    
    // Середины между контрольными точками (приблизительные)
    for(size_t i = 0; i + 1 < m_points.size(); ++i) {
        Point mid((m_points[i].getX() + m_points[i+1].getX()) / 2.0,
                  (m_points[i].getY() + m_points[i+1].getY()) / 2.0);
        snaps.push_back({mid, SnapType::Midpoint});
    }
    
    return snaps;
}

Point Spline::getClosestPoint(const Point& p) const {
    if (m_points.empty()) return p;
    if (m_points.size() == 1) return m_points[0];
    
    // Аппроксимируем сплайн отрезками между контрольными точками для упрощения
    // В реальном CAD нужно было бы вычислять точки на кривой Безье
    Point closest = m_points[0];
    double minDistSq = MathUtils::distSq(p, closest);
    
    for(size_t i = 0; i + 1 < m_points.size(); ++i) {
        Point proj = MathUtils::projectPointOnSegment(p, m_points[i], m_points[i+1]);
        double d = MathUtils::distSq(p, proj);
        if (d < minDistSq) {
            minDistSq = d;
            closest = proj;
        }
    }
    
    return closest;
}
