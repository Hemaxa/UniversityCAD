#pragma once
#include "Object.h"
#include "Point.h"
#include <vector>

class Spline : public Object {
public:
    Spline(const std::vector<Point>& points);

    PrimitiveType getType() const override { return PrimitiveType::Spline; }

    const std::vector<Point>& getPoints() const;
    void setPoints(const std::vector<Point>& points);

    void addPoint(const Point& p);

private:
    std::vector<Point> m_points;
};
