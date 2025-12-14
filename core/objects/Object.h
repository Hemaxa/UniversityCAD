#pragma once

#include "Enums.h"
#include "Point.h"
#include <QColor>
#include <QString>
#include <vector>
#include <optional>

struct SnapPoint {
    Point p;
    SnapType type;
};

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

    // Основные точки привязки (End, Mid, Center, Quadrant)
    virtual std::vector<SnapPoint> getSnapPoints() const { return {}; }

    // Ближайшая точка на объекте (для привязки Nearest)
    virtual Point getClosestPoint(const Point& p) const { return p; }

    // Точки, образующие перпендикуляр из точки p к объекту
    virtual std::optional<Point> getPerpendicularPoint(const Point& p) const { return std::nullopt; }

    // Точки касания из точки p к объекту
    virtual std::vector<Point> getTangentPoints(const Point& p) const { return {}; }

private:
    QColor m_color = Qt::white;
    LineStyle m_style;
    unsigned int m_id = 0;
};
