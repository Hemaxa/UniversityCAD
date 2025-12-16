#pragma once

#include "Enums.h"
#include <cmath>

// Класс для представления точки в 2D пространстве.
// Больше не наследует Object, чтобы избежать круговых зависимостей.
class Point
{
public:
    Point(double x = 0.0, double y = 0.0);

    // Устанавливает глобальную единицу измерения углов.
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
    double m_x, m_y;
    static AngleUnit s_angleUnit;
};
