#pragma once
#include "Object.h"
#include "Point.h"

class Ellipse : public Object {
public:
    Ellipse(const Point& center, double radX, double radY)
        : m_center(center), m_radiusX(radX), m_radiusY(radY) {}

    PrimitiveType getType() const override { return PrimitiveType::Ellipse; }

    const Point& getCenter() const { return m_center; }
    void setCenter(const Point& p) { m_center = p; }

    double getRadiusX() const { return m_radiusX; }
    void setRadiusX(double r) { m_radiusX = r; }

    double getRadiusY() const { return m_radiusY; }
    void setRadiusY(double r) { m_radiusY = r; }

private:
    Point m_center;
    double m_radiusX, m_radiusY;
};
