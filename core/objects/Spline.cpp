#include "Spline.h"
#include "MathUtils.h"
#include <cmath>

Spline::Spline(const std::vector<Point>& points) : m_points(points) {}

const std::vector<Point>& Spline::getPoints() const { return m_points; }
void Spline::setPoints(const std::vector<Point>& points) { m_points = points; }
void Spline::addPoint(const Point& p) { m_points.push_back(p); }

std::vector<Point> Spline::getSmoothPoints(int resolution) const {
    std::vector<Point> smoothPts;
    if (m_points.empty()) return smoothPts;
    if (m_points.size() == 1) {
        smoothPts.push_back(m_points[0]);
        return smoothPts;
    }
    if (m_points.size() == 2) {
        smoothPts.push_back(m_points[0]);
        smoothPts.push_back(m_points[1]);
        return smoothPts;
    }

    smoothPts.push_back(m_points[0]);
    for (size_t i = 0; i < m_points.size() - 1; ++i) {
        Point p0 = (i == 0) ? m_points[0] : m_points[i - 1];
        Point p1 = m_points[i];
        Point p2 = m_points[i + 1];
        Point p3 = (i + 2 < m_points.size()) ? m_points[i + 2] : p2;

        double cp1x = p1.getX() + (p2.getX() - p0.getX()) / 6.0;
        double cp1y = p1.getY() + (p2.getY() - p0.getY()) / 6.0;

        double cp2x = p2.getX() - (p3.getX() - p1.getX()) / 6.0;
        double cp2y = p2.getY() - (p3.getY() - p1.getY()) / 6.0;

        for (int j = 1; j <= resolution; ++j) {
            double t = (double)j / resolution;
            double t2 = t * t;
            double t3 = t2 * t;

            double u = 1.0 - t;
            double u2 = u * u;
            double u3 = u2 * u;

            double x = u3 * p1.getX() + 3 * u2 * t * cp1x + 3 * u * t2 * cp2x + t3 * p2.getX();
            double y = u3 * p1.getY() + 3 * u2 * t * cp1y + 3 * u * t2 * cp2y + t3 * p2.getY();

            smoothPts.emplace_back(x, y);
        }
    }
    return smoothPts;
}

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
    
    auto smoothPts = getSmoothPoints();
    Point closest = smoothPts[0];
    double minDistSq = MathUtils::distSq(p, closest);
    
    for(size_t i = 0; i + 1 < smoothPts.size(); ++i) {
        Point proj = MathUtils::projectPointOnSegment(p, smoothPts[i], smoothPts[i+1]);
        double d = MathUtils::distSq(p, proj);
        if (d < minDistSq) {
            minDistSq = d;
            closest = proj;
        }
    }
    
    return closest;
}
