#include "Circle.h"

Circle::Circle(const Point& center, double radius)
    : m_center(center), m_radius(radius)
{
}

const Point& Circle::getCenter() const {
    return m_center;
}

void Circle::setCenter(const Point& p) {
    m_center = p;
}

double Circle::getRadius() const {
    return m_radius;
}

void Circle::setRadius(double r) {
    m_radius = r;
}
