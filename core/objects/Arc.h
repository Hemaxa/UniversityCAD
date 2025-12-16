#pragma once

#include "Object.h"
#include "Point.h"

// Класс дуги - часть окружности, определяемая центром, радиусом и углами.
class Arc : public Object {
public:
    // Конструктор: создает дугу с центром, радиусом, начальным и охватывающим углами.
    Arc(const Point& center, double radius, double startAngle, double spanAngle);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Arc; }

    // Возвращает центр дуги.
    const Point& getCenter() const;
    // Устанавливает центр дуги.
    void setCenter(const Point& p);

    // Возвращает радиус дуги.
    double getRadius() const;
    // Устанавливает радиус дуги.
    void setRadius(double r);

    // Возвращает начальный угол дуги (в градусах).
    double getStartAngle() const;
    // Устанавливает начальный угол дуги.
    void setStartAngle(double a);

    // Возвращает угол охвата дуги (в градусах).
    double getSpanAngle() const;
    // Устанавливает угол охвата дуги.
    void setSpanAngle(double a);

    // Возвращает точки привязки (центр, концы, середина).
    std::vector<SnapPoint> getSnapPoints() const override;

    // Возвращает ближайшую точку на дуге к заданной точке p.
    Point getClosestPoint(const Point& p) const override;
    // Возвращает точки касания из внешней точки p.
    std::vector<Point> getTangentPoints(const Point& p) const override;

private:
    Point m_center;
    double m_radius;
    double m_startAngle;
    double m_spanAngle;
};
