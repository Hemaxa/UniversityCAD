#include "Arc.h"

Arc::Arc(const Point& center, double radius, double startAngle, double spanAngle)
    : m_center(center), m_radius(radius), m_startAngle(startAngle), m_spanAngle(spanAngle)
{
}

const Point& Arc::getCenter() const { return m_center; }
void Arc::setCenter(const Point& p) { m_center = p; }

double Arc::getRadius() const { return m_radius; }
void Arc::setRadius(double r) { m_radius = r; }

double Arc::getStartAngle() const { return m_startAngle; }
void Arc::setStartAngle(double a) { m_startAngle = a; }

double Arc::getSpanAngle() const { return m_spanAngle; }
void Arc::setSpanAngle(double a) { m_spanAngle = a; }
