#pragma once

// Типы геометрических примитивов
enum class PrimitiveType {
    Generic,
    Point,
    Segment
};

// Типы систем координат
enum class CoordinateSystemType {
    Cartesian, // Декартова
    Polar      // Полярная
};

// Единицы измерения углов
enum class AngleUnit {
    Degrees, // Градусы
    Radians  // Радианы
};
