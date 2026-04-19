#pragma once
#include "Draw.h"

// Включаем классы объектов, чтобы стратегии знали о них
#include "Segment.h"
#include "Circle.h"
#include "Arc.h"
#include "Rectangle.h" // Используем RectanglePrim во избежание конфликтов
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include "PointObject.h"
#include "Dimension.h"

// --- Point ---
class PointDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};

// --- Segment (Логика перенесена сюда) ---
class SegmentDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};

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

// --- Dimension ---
class DimensionDraw : public Draw {
public:
    void draw(QPainter& painter, Object* primitive, bool isSelected) const override;
};
