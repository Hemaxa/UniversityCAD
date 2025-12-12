#include "Arc.h"
#include <cmath>

Arc::Arc(const Point& center, double radius, double startAngle, double spanAngle)
    : m_center(center), m_radius(radius), m_startAngle(startAngle), m_spanAngle(spanAngle) {}

const Point& Arc::getCenter() const { return m_center; }
void Arc::setCenter(const Point& p) { m_center = p; }

double Arc::getRadius() const { return m_radius; }
void Arc::setRadius(double r) { m_radius = r; }

double Arc::getStartAngle() const { return m_startAngle; }
void Arc::setStartAngle(double a) { m_startAngle = a; }

double Arc::getSpanAngle() const { return m_spanAngle; }
void Arc::setSpanAngle(double a) { m_spanAngle = a; }

std::vector<SnapPoint> Arc::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    snaps.push_back({m_center, SnapType::Center});

    double startRad = m_startAngle * M_PI / 180.0;
    double endRad = (m_startAngle + m_spanAngle) * M_PI / 180.0;

    snaps.push_back({Point(m_center.getX() + m_radius * std::cos(startRad),
                           m_center.getY() + m_radius * std::sin(startRad)), SnapType::Endpoint});

    snaps.push_back({Point(m_center.getX() + m_radius * std::cos(endRad),
                           m_center.getY() + m_radius * std::sin(endRad)), SnapType::Endpoint});

    double midRad = (m_startAngle + m_spanAngle/2.0) * M_PI / 180.0;
    snaps.push_back({Point(m_center.getX() + m_radius * std::cos(midRad),
                           m_center.getY() + m_radius * std::sin(midRad)), SnapType::Midpoint});

    return snaps;
}
