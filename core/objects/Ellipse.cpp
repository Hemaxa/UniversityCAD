#include "Ellipse.h"

Ellipse::Ellipse(const Point& center, double radX, double radY)
    : m_center(center), m_radiusX(radX), m_radiusY(radY) {}

const Point& Ellipse::getCenter() const { return m_center; }
void Ellipse::setCenter(const Point& p) { m_center = p; }

double Ellipse::getRadiusX() const { return m_radiusX; }
void Ellipse::setRadiusX(double r) { m_radiusX = r; }

double Ellipse::getRadiusY() const { return m_radiusY; }
void Ellipse::setRadiusY(double r) { m_radiusY = r; }

std::vector<SnapPoint> Ellipse::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    snaps.push_back({m_center, SnapType::Center});
    snaps.push_back({Point(m_center.getX() + m_radiusX, m_center.getY()), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX() - m_radiusX, m_center.getY()), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX(), m_center.getY() + m_radiusY), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX(), m_center.getY() - m_radiusY), SnapType::Quadrant});
    return snaps;
}
