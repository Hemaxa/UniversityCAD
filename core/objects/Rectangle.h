#pragma once

#include "Object.h"
#include "Point.h"

// Класс прямоугольника - определяется верхним левым углом, шириной и высотой.
class Rectangle : public Object {
public:
    // Конструктор: создает прямоугольник с опциональным скруглением углов.
    Rectangle(const Point& topLeft, double width, double height, double cornerRadius = 0.0);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Rectangle; }

    // Возвращает верхний левый угол прямоугольника.
    const Point& getTopLeft() const;
    // Устанавливает верхний левый угол.
    void setTopLeft(const Point& p);

    // Возвращает ширину прямоугольника.
    double getWidth() const;
    // Устанавливает ширину.
    void setWidth(double w);

    // Возвращает высоту прямоугольника.
    double getHeight() const;
    // Устанавливает высоту.
    void setHeight(double h);

    // Возвращает радиус скругления углов.
    double getCornerRadius() const;
    // Устанавливает радиус скругления.
    void setCornerRadius(double r);

    // Возвращает точки привязки (углы, середины сторон, центр).
    std::vector<SnapPoint> getSnapPoints() const override;

    // Возвращает ближайшую точку на периметре к заданной точке p.
    Point getClosestPoint(const Point& p) const override;

private:
    Point m_topLeft;
    double m_width;
    double m_height;
    double m_cornerRadius;
};
