#pragma once

// Типы геометрических примитивов.
enum class PrimitiveType {
    Generic, // Общий тип
    Point,   // Точка
    Segment  // Отрезок
};

// Типы систем координат.
enum class CoordinateSystemType {
    Cartesian, // Декартова
    Polar      // Полярная
};

// Единицы измерения углов.
enum class AngleUnit {
    Degrees, // Градусы
    Radians  // Радианы
};
