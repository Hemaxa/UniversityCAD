#pragma once
#include "Object.h"
#include "Point.h"

class Circle : public Object {
public:
    Circle(const Point& center, double radius) : m_center(center), m_radius(radius) {}
    PrimitiveType getType() const override { return PrimitiveType::Circle; }

    const Point& getCenter() const { return m_center; }
    void setCenter(const Point& p) { m_center = p; }

    double getRadius() const { return m_radius; }
    void setRadius(double r) { m_radius = r; }

private:
    Point m_center;
    double m_radius;
};
