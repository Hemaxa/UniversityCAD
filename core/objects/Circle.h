#pragma once

#include "Object.h"
#include "Point.h"

// Класс окружности - определяется центром и радиусом.
class Circle : public Object {
public:
    // Конструктор: создает окружность с заданным центром и радиусом.
    Circle(const Point& center, double radius);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Circle; }

    // Возвращает центр окружности.
    const Point& getCenter() const;
    // Устанавливает центр окружности.
    void setCenter(const Point& p);

    // Возвращает радиус окружности.
    double getRadius() const;
    // Устанавливает радиус окружности.
    void setRadius(double r);

    // Возвращает точки привязки (центр и квадранты).
    std::vector<SnapPoint> getSnapPoints() const override;

    // Возвращает ближайшую точку на окружности к заданной точке p.
    Point getClosestPoint(const Point& p) const override;
    // Возвращает точки касания из внешней точки p.
    std::vector<Point> getTangentPoints(const Point& p) const override;
    // Возвращает проекцию на касательную линию.
    std::optional<std::pair<Point, Point>> getTangentSnapPoint(
        const Point& prevPoint, const Point& mousePos) const override;

private:
    Point m_center;
    double m_radius;
};
