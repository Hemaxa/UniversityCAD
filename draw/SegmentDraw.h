#pragma once

#include "Draw.h"

// Класс, отвечающий за отрисовку примитива "Отрезок".
class SegmentDraw : public Draw
{

public:
    // Реализует метод отрисовки для отрезка.
    void draw(QPainter& painter, Object* primitive, bool isSelected = false) const override;
};
