#pragma once

#include "Enums.h"

#include <QColor>

// Абстрактный базовый класс для всех геометрических объектов.
class Object
{
public:
    // Виртуальный деструктор по умолчанию.
    virtual ~Object() = default;

    // Возвращает тип примитива.
    virtual PrimitiveType getType() const { return PrimitiveType::Generic; }

    // Устанавливает уникальный идентификатор объекта.
    void setID(unsigned int id) { m_id = id; }

    // Возвращает уникальный идентификатор объекта.
    unsigned int getID() const { return m_id; }

    // Устанавливает цвет объекта.
    virtual void setColor(const QColor& color) { m_color = color; }

    // Возвращает текущий цвет объекта.
    virtual QColor getColor() const { return m_color; }

private:
    // Цвет объекта по умолчанию (белый).
    QColor m_color = Qt::white;

    // Уникальный идентификатор объекта (управляется Сценой).
    unsigned int m_id = 0;
};
