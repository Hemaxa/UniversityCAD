#pragma once

#include "Enums.h"
#include <QColor>
#include <QString>

// Структура, описывающая стиль линии.
struct LineStyle {
    LineStyleType type = LineStyleType::SolidMain;
    QString name = "Сплошная основная";

    // Для ГОСТ стилей width задает логическую толщину (пиксели на экране)
    double width = 2.0;
    double dashLength = 4.0;
    double gapLength = 2.0;

    bool isMain = true;
};

class Object
{
public:
    virtual ~Object() = default;

    virtual PrimitiveType getType() const { return PrimitiveType::Generic; }
    void setID(unsigned int id) { m_id = id; }
    unsigned int getID() const { return m_id; }
    virtual void setColor(const QColor& color) { m_color = color; }
    virtual QColor getColor() const { return m_color; }
    virtual void setLineStyle(const LineStyle& style) { m_style = style; }
    virtual const LineStyle& getLineStyle() const { return m_style; }

private:
    QColor m_color = Qt::white;
    LineStyle m_style;
    unsigned int m_id = 0;
};
