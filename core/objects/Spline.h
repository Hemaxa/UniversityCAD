#pragma once

#include "Object.h"
#include "Point.h"
#include <vector>

// Класс сплайна - кривая, проходящая через контрольные точки.
class Spline : public Object {
public:
    // Конструктор: создает сплайн по набору контрольных точек.
    Spline(const std::vector<Point>& points);

    // Возвращает тип примитива.
    PrimitiveType getType() const override { return PrimitiveType::Spline; }

    // Возвращает контрольные точки сплайна.
    const std::vector<Point>& getPoints() const;
    // Устанавливает контрольные точки сплайна.
    void setPoints(const std::vector<Point>& points);
    // Добавляет контрольную точку в конец.
    void addPoint(const Point& p);

    // Вычисляет сглаженные точки для отрисовки и DXF экспорта.
    std::vector<Point> getSmoothPoints(int resolution = 20) const;

    // Возвращает точки привязки (контрольные точки и середины).
    std::vector<SnapPoint> getSnapPoints() const override;
    // Возвращает ближайшую точку на сплайне к заданной точке p.
    Point getClosestPoint(const Point& p) const override;

private:
    std::vector<Point> m_points;
};
