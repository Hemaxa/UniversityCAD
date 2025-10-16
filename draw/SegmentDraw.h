#pragma once
#include "Draw.h"

class SegmentDraw : public Draw
{
public:
    void draw(QPainter& painter, Object* primitive) const override;
};
