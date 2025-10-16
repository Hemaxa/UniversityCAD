#pragma once
#include "Enums.h"
#include <QColor>

/**
 * @class Object
 * @brief Абстрактный базовый класс для всех геометрических объектов.
 */
class Object
{
public:
    virtual ~Object() = default;
    virtual PrimitiveType getType() const { return PrimitiveType::Generic; }
    virtual void setColor(const QColor& color) { m_color = color; }
    virtual QColor getColor() const { return m_color; }

private:
    QColor m_color = Qt::white; // Цвет объекта по умолчанию
};
