#include "Polygon.h"

PolygonObj::PolygonObj(const Point& center, double radius, int sides, bool inscribed)
    : m_center(center), m_radius(radius), m_sides(sides), m_inscribed(inscribed)
{
}

const Point& PolygonObj::getCenter() const { return m_center; }
void PolygonObj::setCenter(const Point& p) { m_center = p; }

double PolygonObj::getRadius() const { return m_radius; }
void PolygonObj::setRadius(double r) { m_radius = r; }

int PolygonObj::getSides() const { return m_sides; }
void PolygonObj::setSides(int s) { m_sides = s; }

bool PolygonObj::isInscribed() const { return m_inscribed; }
void PolygonObj::setInscribed(bool i) { m_inscribed = i; }
