#pragma once
#include "Object.h"
#include "Point.h"

class PolygonObj : public Object {
public:
    PolygonObj(const Point& center, double radius, int sides, bool inscribed = true);
    PrimitiveType getType() const override { return PrimitiveType::Polygon; }

    const Point& getCenter() const;
    void setCenter(const Point& p);

    double getRadius() const;
    void setRadius(double r);

    int getSides() const;
    void setSides(int s);

    bool isInscribed() const;
    void setInscribed(bool i);

    std::vector<SnapPoint> getSnapPoints() const override;
    Point getClosestPoint(const Point& p) const override;
    std::optional<Point> getPerpendicularPoint(const Point& p) const override;

private:
    std::vector<Point> getVertices() const;

    Point m_center;
    double m_radius;
    int m_sides;
    bool m_inscribed;
};
