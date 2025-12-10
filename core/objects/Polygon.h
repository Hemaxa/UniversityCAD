#pragma once
#include "Object.h"
#include "Point.h"

class PolygonObj : public Object {
public:
    PolygonObj(const Point& center, double radius, int sides, bool inscribed = true)
        : m_center(center), m_radius(radius), m_sides(sides), m_inscribed(inscribed) {}

    PrimitiveType getType() const override { return PrimitiveType::Polygon; }

    const Point& getCenter() const { return m_center; }
    void setCenter(const Point& p) { m_center = p; }

    double getRadius() const { return m_radius; }
    void setRadius(double r) { m_radius = r; }

    int getSides() const { return m_sides; }
    void setSides(int s) { m_sides = s; }

    bool isInscribed() const { return m_inscribed; }
    void setInscribed(bool i) { m_inscribed = i; }

private:
    Point m_center;
    double m_radius;
    int m_sides;
    bool m_inscribed;
};
