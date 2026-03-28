#include "PointObject.h"
#include "MathUtils.h"

PointObject::PointObject(const Point& position) : m_position(position) {}

const Point& PointObject::getPosition() const { return m_position; }
void PointObject::setPosition(const Point& p) { m_position = p; }

std::vector<SnapPoint> PointObject::getSnapPoints() const {
    return {{m_position, SnapType::Endpoint}};
}

Point PointObject::getClosestPoint(const Point& p) const {
    // Для точки ближайшая точка — всегда сама позиция
    return m_position;
}
