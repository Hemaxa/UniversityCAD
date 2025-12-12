#pragma once
#include "Object.h"
#include "Point.h"

class Rectangle : public Object {
public:
    // Конструктор: верхний левый угол, ширина, высота и радиус скругления углов
    Rectangle(const Point& topLeft, double width, double height, double cornerRadius = 0.0);

    PrimitiveType getType() const override { return PrimitiveType::Rectangle; }

    const Point& getTopLeft() const;
    void setTopLeft(const Point& p);

    double getWidth() const;
    void setWidth(double w);

    double getHeight() const;
    void setHeight(double h);

    double getCornerRadius() const;
    void setCornerRadius(double r);

private:
    Point m_topLeft;
    double m_width;
    double m_height;
    double m_cornerRadius; // Для скругленных углов
};
