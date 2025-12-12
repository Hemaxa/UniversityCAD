#pragma once

#include "Enums.h"
#include "Point.h" // Теперь мы можем включить Point.h, так как он не зависит от Object
#include <QColor>
#include <QString>
#include <vector>

// Точка привязки
struct SnapPoint {
    Point p;
    SnapType type;
};

// Структура, описывающая стиль линии.
struct LineStyle {
    LineStyleType type = LineStyleType::SolidMain;
    QString name = "Сплошная основная";

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

    // Виртуальный метод получения точек привязки
    virtual std::vector<SnapPoint> getSnapPoints() const { return {}; }

private:
    QColor m_color = Qt::white;
    LineStyle m_style;
    unsigned int m_id = 0;
};
