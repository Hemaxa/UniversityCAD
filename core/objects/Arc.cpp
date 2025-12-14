#include "Arc.h"
#include <cmath>
#include "MathUtils.h"

Arc::Arc(const Point& center, double radius, double startAngle, double spanAngle) : m_center(center), m_radius(radius), m_startAngle(startAngle), m_spanAngle(spanAngle) {}

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

    // Углы в радианах
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

Point Arc::getClosestPoint(const Point& p) const {
    // 1. Проецируем на полную окружность
    double dx = p.getX() - m_center.getX();
    double dy = p.getY() - m_center.getY();
    double angle = std::atan2(dy, dx) * 180.0 / M_PI; // -180..180
    if (angle < 0) angle += 360.0; // 0..360

    // Проверка попадания в диапазон углов дуги
    double s = MathUtils::normalizeAngle(m_startAngle);
    double e = MathUtils::normalizeAngle(m_startAngle + m_spanAngle);

    bool inside = false;
    if (s < e) inside = (angle >= s && angle <= e);
    else inside = (angle >= s || angle <= e); // Переход через 0

    if (inside) {
        return Point(m_center.getX() + m_radius * std::cos(angle * M_PI/180),
                     m_center.getY() + m_radius * std::sin(angle * M_PI/180));
    }

    // Если не внутри, возвращаем ближайший конец
    auto snaps = getSnapPoints();
    // snaps[1] и snaps[2] это концы (см. getSnapPoints)
    double d1 = MathUtils::distSq(p, snaps[1].p);
    double d2 = MathUtils::distSq(p, snaps[2].p);
    return (d1 < d2) ? snaps[1].p : snaps[2].p;
}

std::vector<Point> Arc::getTangentPoints(const Point& p) const {
    // Получаем касательные к полной окружности
    auto candidates = MathUtils::getTangentPoints(p, m_center, m_radius);
    std::vector<Point> result;

    // Фильтруем те, что не лежат на дуге
    for (const auto& pt : candidates) {
        double dx = pt.getX() - m_center.getX();
        double dy = pt.getY() - m_center.getY();
        double angle = std::atan2(dy, dx) * 180.0 / M_PI;
        if (angle < 0) angle += 360.0;

        double s = MathUtils::normalizeAngle(m_startAngle);
        double e = MathUtils::normalizeAngle(m_startAngle + m_spanAngle);

        bool inside = false;
        if (s < e) inside = (angle >= s - 0.1 && angle <= e + 0.1);
        else inside = (angle >= s - 0.1 || angle <= e + 0.1);

        if (inside) result.push_back(pt);
    }
    return result;
}
