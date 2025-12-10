#pragma once
#include "Draw.h"
#include "Circle.h"
#include "Arc.h"
#include "RectanglePrim.h"
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"

// --- Circle ---
class CircleDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};

// --- Arc ---
class ArcDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};

// --- Rectangle ---
class RectangleDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};

// --- Ellipse ---
class EllipseDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};

// --- Polygon ---
class PolygonDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};

// --- Spline ---
class SplineDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};
