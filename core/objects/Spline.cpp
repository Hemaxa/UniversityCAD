#include "Spline.h"

Spline::Spline(const std::vector<Point>& points) : m_points(points) {}

const std::vector<Point>& Spline::getPoints() const { return m_points; }
void Spline::setPoints(const std::vector<Point>& points) { m_points = points; }
void Spline::addPoint(const Point& p) { m_points.push_back(p); }

std::vector<SnapPoint> Spline::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    for(const auto& p : m_points) {
        snaps.push_back({p, SnapType::Endpoint}); // Привязка к опорным точкам
    }
    return snaps;
}
