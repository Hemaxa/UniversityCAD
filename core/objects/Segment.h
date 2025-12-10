#pragma once

#include "Object.h"
#include "Point.h"

// Класс для представления отрезка.
class Segment : public Object
{
public:
    // Конструктор по двум точкам.
    Segment(const Point& start, const Point& end);

    // Возвращает тип примитива (Segment).
    PrimitiveType getType() const override { return PrimitiveType::Segment; };

    // Возвращает начальную точку.
    const Point& getStart() const;

    // Устанавливает начальную точку.
    void setStart(const Point& point);

    // Возвращает конечную точку.
    const Point& getEnd() const;

    // Устанавливает конечную точку.
    void setEnd(const Point& point);

private:
    Point m_start; // Начало отрезка
    Point m_end;   // Конец отрезка
};
