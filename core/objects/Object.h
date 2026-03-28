#pragma once

#include "Enums.h"
#include "Point.h"
#include <QColor>
#include <QString>
#include <vector>
#include <optional>
#include <utility>

// Точка привязки с информацией о типе.
struct SnapPoint {
    Point p;
    SnapType type;
};

// Стиль линии по ГОСТ.
struct LineStyle {
    LineStyleType type = LineStyleType::SolidMain;
    QString name = "Сплошная основная";
    double dashLength = 4.0;
    double gapLength = 2.0;
    bool isMain = true;
    double customWidth = 0.0;  // Если > 0, переопределяет глобальную толщину
};

// Базовый класс для всех геометрических объектов.
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
    // Возвращает цвет объекта.
    virtual QColor getColor() const { return m_color; }

    // Устанавливает стиль линии объекта.
    virtual void setLineStyle(const LineStyle& style) { m_style = style; }
    // Возвращает стиль линии объекта.
    virtual const LineStyle& getLineStyle() const { return m_style; }

    // Устанавливает слой объекта.
    virtual void setLayer(const QString& layer) { m_layer = layer; }
    // Возвращает слой объекта.
    virtual QString getLayer() const { return m_layer; }

    // Возвращает основные точки привязки (End, Mid, Center, Quadrant).
    virtual std::vector<SnapPoint> getSnapPoints() const { return {}; }

    // Возвращает ближайшую точку на объекте (для привязки Nearest).
    virtual Point getClosestPoint(const Point& p) const { return p; }

    // Возвращает точку перпендикуляра из точки p к объекту.
    virtual std::optional<Point> getPerpendicularPoint(const Point& p) const { return std::nullopt; }

    // Возвращает точки касания из точки p к объекту.
    virtual std::vector<Point> getTangentPoints(const Point& p) const { return {}; }

    // Возвращает проекцию на касательную линию: (точка касания, проекция mousePos на линию касательной).
    // prevPoint - начальная точка линии, mousePos - текущая позиция курсора.
    virtual std::optional<std::pair<Point, Point>> getTangentSnapPoint(
        const Point& prevPoint, const Point& mousePos) const { return std::nullopt; }

private:
    QColor m_color = Qt::white;
    LineStyle m_style;
    QString m_layer = "0";
    unsigned int m_id = 0;
};

