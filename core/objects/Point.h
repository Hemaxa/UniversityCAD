#pragma once
#include "Object.h"
#include "Enums.h"

class Point : public Object
{
public:
    Point(double x = 0.0, double y = 0.0);
    PrimitiveType getType() const override { return PrimitiveType::Point; };

    static void setAngleUnit(AngleUnit unit);
    static AngleUnit getAngleUnit();

    double getX() const;
    void setX(double x);
    double getY() const;
    void setY(double y);

    double getRadius() const;
    double getAngle() const;
    void setPolar(double radius, double angle);

private:
    double m_x, m_y; // Декартовы координаты
    static AngleUnit s_angleUnit; // Глобальная единица измерения углов
};
