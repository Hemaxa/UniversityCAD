#include "Point.h"

#include <cmath>

// Инициализация статической переменной для единиц измерения углов.
AngleUnit Point::s_angleUnit = AngleUnit::Degrees;

// Конструктор класса Point.
Point::Point(double x, double y) : m_x(x), m_y(y) {}
// Устанавливает глобальную единицу измерения углов (градусы или радианы).
void Point::setAngleUnit(AngleUnit unit) { s_angleUnit = unit; }
// Возвращает текущую глобальную единицу измерения углов.
AngleUnit Point::getAngleUnit() { return s_angleUnit; }
// Возвращает координату X точки.
double Point::getX() const { return m_x; }
// Устанавливает координату X точки.
void Point::setX(double x) { m_x = x; }
// Возвращает координату Y точки.
double Point::getY() const { return m_y; }
// Устанавливает координату Y точки.
void Point::setY(double y) { m_y = y; }

// Вычисляет и возвращает полярный радиус точки.
double Point::getRadius() const { return std::sqrt(m_x * m_x + m_y * m_y); }

// Вычисляет и возвращает полярный угол в установленных единицах.
double Point::getAngle() const {
    double angleRad = std::atan2(m_y, m_x);
    return (s_angleUnit == AngleUnit::Degrees) ? (angleRad * 180.0 / M_PI) : angleRad;
}

// Устанавливает декартовы координаты точки на основе полярных.
void Point::setPolar(double radius, double angle) {
    double angleRad = (s_angleUnit == AngleUnit::Degrees) ? (angle * M_PI / 180.0) : angle;
    m_x = radius * std::cos(angleRad);
    m_y = radius * std::sin(angleRad);
}
