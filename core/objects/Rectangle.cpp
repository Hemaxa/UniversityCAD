#pragma once
#include "Object.h"
#include "Point.h"

class Rectangle : public Object {
public:
    Rectangle(const Point& topLeft, double width, double height, double cornerRadius = 0.0)
        : m_topLeft(topLeft), m_width(width), m_height(height), m_cornerRadius(cornerRadius) {}

    PrimitiveType getType() const override { return PrimitiveType::Rectangle; }

    const Point& getTopLeft() const { return m_topLeft; }
    void setTopLeft(const Point& p) { m_topLeft = p; }

    double getWidth() const { return m_width; }
    void setWidth(double w) { m_width = w; }

    double getHeight() const { return m_height; }
    void setHeight(double h) { m_height = h; }

    double getCornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(double r) { m_cornerRadius = r; }

private:
    Point m_topLeft;
    double m_width, m_height;
    double m_cornerRadius; // Для фасок/скруглений
};
