#pragma once
#include "Object.h"
#include "Point.h"

class Arc : public Object {
public:
    // Angles in degrees
    Arc(const Point& center, double radius, double startAngle, double spanAngle)
        : m_center(center), m_radius(radius), m_startAngle(startAngle), m_spanAngle(spanAngle) {}

    PrimitiveType getType() const override { return PrimitiveType::Arc; }

    const Point& getCenter() const { return m_center; }
    void setCenter(const Point& p) { m_center = p; }

    double getRadius() const { return m_radius; }
    void setRadius(double r) { m_radius = r; }

    double getStartAngle() const { return m_startAngle; }
    void setStartAngle(double a) { m_startAngle = a; }

    double getSpanAngle() const { return m_spanAngle; }
    void setSpanAngle(double a) { m_spanAngle = a; }

private:
    Point m_center;
    double m_radius;
    double m_startAngle;
    double m_spanAngle;
};
