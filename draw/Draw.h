#pragma once

class QPainter;
class Object;

// Абстрактный базовый класс (интерфейс) для отрисовщиков объектов.
class Draw
{
public:
    // Виртуальный деструктор по умолчанию.
    virtual ~Draw() = default;
    // Чисто виртуальный метод для отрисовки объекта.
    virtual void draw(QPainter& painter, Object* primitive) const = 0;
};
