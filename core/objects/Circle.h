#pragma once
#include "Object.h"
#include "Point.h"

class Circle : public Object {
public:
    Circle(const Point& center, double radius);

    PrimitiveType getType() const override { return PrimitiveType::Circle; }

    const Point& getCenter() const;
    void setCenter(const Point& p);

    double getRadius() const;
    void setRadius(double r);

private:
    Point m_center;
    double m_radius;
};
