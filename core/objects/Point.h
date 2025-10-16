#pragma once

#include "Object.h"
#include "Enums.h"

// Класс для представления точки в 2D пространстве.
class Point : public Object
{
public:
    // Конструктор, создающий точку с заданными координатами.
    Point(double x = 0.0, double y = 0.0);
    // Возвращает тип примитива (точка).
    PrimitiveType getType() const override { return PrimitiveType::Point; };

    // Устанавливает глобальную единицу измерения углов.
    static void setAngleUnit(AngleUnit unit);
    // Возвращает текущую глобальную единицу измерения углов.
    static AngleUnit getAngleUnit();

    // Возвращает координату X.
    double getX() const;
    // Устанавливает координату X.
    void setX(double x);
    // Возвращает координату Y.
    double getY() const;
    // Устанавливает координату Y.
    void setY(double y);

    // Возвращает полярный радиус.
    double getRadius() const;
    // Возвращает полярный угол в текущих единицах.
    double getAngle() const;
    // Устанавливает координаты точки через полярные координаты.
    void setPolar(double radius, double angle);

private:
    // Декартовы координаты точки.
    double m_x, m_y;
    // Глобальная статическая переменная для единиц измерения углов.
    static AngleUnit s_angleUnit;
};
