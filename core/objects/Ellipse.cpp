#include "Ellipse.h"

Ellipse::Ellipse(const Point& center, double radX, double radY)
    : m_center(center), m_radiusX(radX), m_radiusY(radY)
{
}

const Point& Ellipse::getCenter() const { return m_center; }
void Ellipse::setCenter(const Point& p) { m_center = p; }

double Ellipse::getRadiusX() const { return m_radiusX; }
void Ellipse::setRadiusX(double r) { m_radiusX = r; }

double Ellipse::getRadiusY() const { return m_radiusY; }
void Ellipse::setRadiusY(double r) { m_radiusY = r; }
