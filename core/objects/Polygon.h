#pragma once

#include "Object.h"
#include "Point.h"

// Класс правильного многоугольника - определяется центром, радиусом и количеством сторон.
class PolygonObj : public Object {
public:
    // Конструктор: создает многоугольник (inscribed = вписанный в окружность).
    PolygonObj(const Point& center, double radius, int sides, bool inscribed = true);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Polygon; }

    // Возвращает центр многоугольника.
    const Point& getCenter() const;
    // Устанавливает центр многоугольника.
    void setCenter(const Point& p);

    // Возвращает радиус описанной/вписанной окружности.
    double getRadius() const;
    // Устанавливает радиус.
    void setRadius(double r);

    // Возвращает количество сторон многоугольника.
    int getSides() const;
    // Устанавливает количество сторон.
    void setSides(int s);

    // Возвращает true, если многоугольник вписан в окружность.
    bool isInscribed() const;
    // Устанавливает режим вписывания.
    void setInscribed(bool i);

    // Возвращает точки привязки (центр, вершины, середины сторон).
    std::vector<SnapPoint> getSnapPoints() const override;
    // Возвращает ближайшую точку на периметре к заданной точке p.
    Point getClosestPoint(const Point& p) const override;
    // Возвращает точку перпендикуляра из точки p на ближайшую сторону.
    std::optional<Point> getPerpendicularPoint(const Point& p) const override;

private:
    // Вычисляет вершины многоугольника.
    std::vector<Point> getVertices() const;

    Point m_center;
    double m_radius;
    int m_sides;
    bool m_inscribed;
};
