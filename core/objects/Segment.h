#pragma once
#include "Object.h"
#include "Point.h"

class Segment : public Object
{
public:
    Segment(const Point& start, const Point& end);
    PrimitiveType getType() const override { return PrimitiveType::Segment; };

    const Point& getStart() const;
    void setStart(const Point& point);

    const Point& getEnd() const;
    void setEnd(const Point& point);

    std::vector<SnapPoint> getSnapPoints() const override;

private:
    Point m_start;
    Point m_end;
};
