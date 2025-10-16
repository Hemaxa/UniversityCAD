#pragma once

class QPainter;
class Object;

/**
 * @class Draw
 * @brief Абстрактный базовый класс (интерфейс) для отрисовщиков объектов.
 */
class Draw
{
public:
    virtual ~Draw() = default;
    virtual void draw(QPainter& painter, Object* primitive) const = 0;
};
