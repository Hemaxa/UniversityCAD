#pragma once

#include "Object.h"
#include "Point.h"

// Класс для представления точки как геометрического примитива на сцене.
// Отображается как крестик (×) фиксированного экранного размера.
class PointObject : public Object {
public:
    // Конструктор: создает точку в заданной позиции.
    explicit PointObject(const Point& position);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Point; }

    // Возвращает позицию точки.
    const Point& getPosition() const;
    // Устанавливает позицию точки.
    void setPosition(const Point& p);

    // Возвращает точки привязки (Endpoint на позиции).
    std::vector<SnapPoint> getSnapPoints() const override;

    // Возвращает ближайшую точку на объекте (сама позиция).
    Point getClosestPoint(const Point& p) const override;

private:
    Point m_position;
};
