#pragma once
#include "Object.h"
#include "Point.h"

class Arc : public Object {
public:
    Arc(const Point& center, double radius, double startAngle, double spanAngle);
    PrimitiveType getType() const override { return PrimitiveType::Arc; }

    const Point& getCenter() const;
    void setCenter(const Point& p);

    double getRadius() const;
    void setRadius(double r);

    double getStartAngle() const;
    void setStartAngle(double a);

    double getSpanAngle() const;
    void setSpanAngle(double a);

    std::vector<SnapPoint> getSnapPoints() const override;

private:
    Point m_center;
    double m_radius;
    double m_startAngle;
    double m_spanAngle;
};
