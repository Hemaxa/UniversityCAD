#pragma once

#include "Enums.h"
#include <QColor>
#include <QString>

// Структура, описывающая стиль линии.
struct LineStyle {
    LineStyleType type = LineStyleType::Solid; // Тип линии
    QString name = "Сплошная";                 // Название стиля

    double width = 0.8;       // Толщина линии
    double dashLength = 4.0;  // Длина штриха
    double gapLength = 2.0;   // Длина пробела

    bool isMain = true;       // Является ли линия основной (толстой)
};

// Абстрактный базовый класс для всех геометрических объектов.
class Object
{
public:
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

    // Устанавливает стиль линии.
    virtual void setLineStyle(const LineStyle& style) { m_style = style; }

    // Возвращает текущий стиль линии.
    virtual const LineStyle& getLineStyle() const { return m_style; }

private:
    QColor m_color = Qt::white; // Цвет объекта
    LineStyle m_style;          // Стиль линии
    unsigned int m_id = 0;      // ID объекта
};
