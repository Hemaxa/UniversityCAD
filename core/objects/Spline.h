#pragma once
#include "Object.h"
#include "Point.h"
#include <vector>

class Spline : public Object {
public:
    Spline(const std::vector<Point>& points) : m_points(points) {}

    PrimitiveType getType() const override { return PrimitiveType::Spline; }

    const std::vector<Point>& getPoints() const { return m_points; }
    void setPoints(const std::vector<Point>& points) { m_points = points; }

    void addPoint(const Point& p) { m_points.push_back(p); }

private:
    std::vector<Point> m_points;
};
