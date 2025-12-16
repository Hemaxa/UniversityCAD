#pragma once

#include "Object.h"
#include "Point.h"

// Класс отрезка - линия между двумя точками.
class Segment : public Object
{
public:
    // Конструктор: создает отрезок от start до end.
    Segment(const Point& start, const Point& end);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Segment; };

    // Возвращает начальную точку отрезка.
    const Point& getStart() const;
    // Устанавливает начальную точку отрезка.
    void setStart(const Point& point);

    // Возвращает конечную точку отрезка.
    const Point& getEnd() const;
    // Устанавливает конечную точку отрезка.
    void setEnd(const Point& point);

    // Возвращает точки привязки (концы и середина).
    std::vector<SnapPoint> getSnapPoints() const override;

    // Возвращает ближайшую точку на отрезке к заданной точке p.
    Point getClosestPoint(const Point& p) const override;
    // Возвращает точку перпендикуляра из точки p на отрезок.
    std::optional<Point> getPerpendicularPoint(const Point& p) const override;

private:
    Point m_start;
    Point m_end;
};
