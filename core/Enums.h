#pragma once

// Типы геометрических примитивов.
enum class PrimitiveType {
    Generic,
    Point,
    Segment,
    Circle,
    Arc,
    Rectangle,
    Ellipse,
    Polygon,
    Spline
};

// Типы систем координат.
enum class CoordinateSystemType {
    Cartesian,
    Polar
};

// Единицы измерения углов.
enum class AngleUnit {
    Degrees,
    Radians
};

// Типы линий.
enum class LineStyleType {
    Solid,
    SolidWavy,
    SolidZigZag,
    Dashed,
    DashDot,
    DashDotDot,
    Custom
};
