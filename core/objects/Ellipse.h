#pragma once

#include "Object.h"
#include "Point.h"

// Класс эллипса - определяется центром и радиусами по осям X и Y.
class Ellipse : public Object {
public:
    // Конструктор: создает эллипс с заданным центром и радиусами.
    Ellipse(const Point& center, double radX, double radY);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Ellipse; }

    // Возвращает центр эллипса.
    const Point& getCenter() const;
    // Устанавливает центр эллипса.
    void setCenter(const Point& p);

    // Возвращает радиус по оси X.
    double getRadiusX() const;
    // Устанавливает радиус по оси X.
    void setRadiusX(double r);

    // Возвращает радиус по оси Y.
    double getRadiusY() const;
    // Устанавливает радиус по оси Y.
    void setRadiusY(double r);

    // Возвращает точки привязки (центр и квадранты осей).
    std::vector<SnapPoint> getSnapPoints() const override;
    // Возвращает ближайшую точку на эллипсе к заданной точке p.
    Point getClosestPoint(const Point& p) const override;
    // Возвращает точки касания из внешней точки p.
    std::vector<Point> getTangentPoints(const Point& p) const override;
    // Возвращает проекцию на касательную линию.
    std::optional<std::pair<Point, Point>> getTangentSnapPoint(
        const Point& prevPoint, const Point& mousePos) const override;

private:
    Point m_center;
    double m_radiusX, m_radiusY;
};
