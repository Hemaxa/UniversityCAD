#pragma once

// Типы геометрических примитивов.
enum class PrimitiveType {
    Generic, // Общий тип (курсор)
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

// Типы линий.
enum class LineStyleType {
    Solid,          // Сплошная
    SolidWavy,      // Сплошная волнистая
    SolidZigZag,    // Сплошная с изломами
    Dashed,         // Штриховая
    DashDot,        // Штрихпунктирная
    DashDotDot,     // Штрихпунктирная с двумя точками
    Custom          // Пользовательская
};
