#pragma once
#include "Object.h"
#include "Point.h"

class Ellipse : public Object {
public:
    Ellipse(const Point& center, double radX, double radY);
    PrimitiveType getType() const override { return PrimitiveType::Ellipse; }

    const Point& getCenter() const;
    void setCenter(const Point& p);

    double getRadiusX() const;
    void setRadiusX(double r);

    double getRadiusY() const;
    void setRadiusY(double r);

    std::vector<SnapPoint> getSnapPoints() const override;

private:
    Point m_center;
    double m_radiusX, m_radiusY;
};
