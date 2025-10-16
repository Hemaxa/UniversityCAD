#pragma once

#include "Object.h"
#include "Point.h"

// Класс для представления отрезка, определенного двумя точками.
class Segment : public Object
{
public:
    // Конструктор, создающий отрезок по начальной и конечной точкам.
    Segment(const Point& start, const Point& end);
    // Возвращает тип примитива (отрезок).
    PrimitiveType getType() const override { return PrimitiveType::Segment; };

    // Возвращает константную ссылку на начальную точку отрезка.
    const Point& getStart() const;
    // Устанавливает начальную точку отрезка.
    void setStart(const Point& point);
    // Возвращает константную ссылку на конечную точку отрезка.
    const Point& getEnd() const;
    // Устанавливает конечную точку отрезка.
    void setEnd(const Point& point);

private:
    // Начальная точка отрезка.
    Point m_start;
    // Конечная точка отрезка.
    Point m_end;
};
