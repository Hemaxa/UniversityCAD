#include "Point.h"
#include <cmath>

AngleUnit Point::s_angleUnit = AngleUnit::Degrees;

Point::Point(double x, double y) : m_x(x), m_y(y) {}
void Point::setAngleUnit(AngleUnit unit) { s_angleUnit = unit; }
AngleUnit Point::getAngleUnit() { return s_angleUnit; }
double Point::getX() const { return m_x; }
void Point::setX(double x) { m_x = x; }
double Point::getY() const { return m_y; }
void Point::setY(double y) { m_y = y; }

double Point::getRadius() const { return std::sqrt(m_x * m_x + m_y * m_y); }

double Point::getAngle() const {
    double angleRad = std::atan2(m_y, m_x);
    return (s_angleUnit == AngleUnit::Degrees) ? (angleRad * 180.0 / M_PI) : angleRad;
}

void Point::setPolar(double radius, double angle) {
    double angleRad = (s_angleUnit == AngleUnit::Degrees) ? (angle * M_PI / 180.0) : angle;
    m_x = radius * std::cos(angleRad);
    m_y = radius * std::sin(angleRad);
}
