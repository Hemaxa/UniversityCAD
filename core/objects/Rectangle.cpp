#include "Rectangle.h"

Rectangle::Rectangle(const Point& topLeft, double width, double height, double cornerRadius)
    : m_topLeft(topLeft), m_width(width), m_height(height), m_cornerRadius(cornerRadius)
{
}

const Point& Rectangle::getTopLeft() const {
    return m_topLeft;
}

void Rectangle::setTopLeft(const Point& p) {
    m_topLeft = p;
}

double Rectangle::getWidth() const {
    return m_width;
}

void Rectangle::setWidth(double w) {
    m_width = w;
}

double Rectangle::getHeight() const {
    return m_height;
}

void Rectangle::setHeight(double h) {
    m_height = h;
}

double Rectangle::getCornerRadius() const {
    return m_cornerRadius;
}

void Rectangle::setCornerRadius(double r) {
    m_cornerRadius = r;
}
