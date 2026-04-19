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
    Spline,
    Dimension
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

// Типы линий (ГОСТ 2.303-68).
enum class LineStyleType {
    SolidMain,          // Сплошная толстая (основная)
    SolidThin,          // Сплошная тонкая
    SolidWavy,          // Сплошная волнистая
    SolidZigZag,        // Сплошная с изломами
    Dashed,             // Штриховая
    DashDotThin,        // Штрихпунктирная тонкая
    DashDotThick,       // Штрихпунктирная утолщенная
    DashDotDot,         // Штрихпунктирная с двумя точками
    Custom              // Пользовательская
};

// Типы привязок.
enum class SnapType {
    None,
    Endpoint,       // Конечная точка
    Midpoint,       // Середина
    Center,         // Центр (окружности, дуги, многоугольника, прямоугольника)
    Intersection,   // Пересечение
    Perpendicular,  // Перпендикуляр (требует предыдущей точки)
    Tangent,        // Касательная (требует предыдущей точки)
    Quadrant,       // Квадрант (0, 90, 180, 270 градусов)
    Nearest         // Ближайшая точка на объекте
};

enum class DimensionType {
    Linear,
    Horizontal,
    Vertical,
    Radius,
    Diameter,
    Angular
};

enum class ArrowType {
    Closed,
    Open,
    Tick,
    Dot
};

enum class ArrowPlacement {
    Inside,
    Outside
};
