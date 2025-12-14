#include "Snapper.h"
#include "Scene.h"
#include "Segment.h"
#include "MathUtils.h"
#include <cmath>
#include <limits>
#include <algorithm>

Snapper::Snapper(const Scene* scene) : m_scene(scene) {}

void Snapper::setGridSnap(bool enabled, int step) { m_gridSnapEnabled = enabled; m_gridStep = step; }
void Snapper::setObjectSnap(bool enabled) { m_objSnapEnabled = enabled; }

// Пересечение двух отрезков (утилита)
std::optional<Point> getSegmentIntersection(const Point& p1, const Point& p2, const Point& p3, const Point& p4) {
    double det = (p2.getX() - p1.getX()) * (p4.getY() - p3.getY()) - (p4.getX() - p3.getX()) * (p2.getY() - p1.getY());
    if (std::abs(det) < 1e-9) return std::nullopt;

    double t = ((p3.getX() - p1.getX()) * (p4.getY() - p3.getY()) - (p4.getX() - p3.getX()) * (p3.getY() - p1.getY())) / det;
    double u = ((p3.getX() - p1.getX()) * (p2.getY() - p1.getY()) - (p2.getX() - p1.getX()) * (p3.getY() - p1.getY())) / det;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        return Point(p1.getX() + t * (p2.getX() - p1.getX()), p1.getY() + t * (p2.getY() - p1.getY()));
    }
    return std::nullopt;
}

SnapResult Snapper::snap(const Point& mouseWorldPos, double scaleFactor, const std::optional<Point>& prevPoint) const {
    SnapResult result;
    if (!m_scene) return result;

    double minInfoDist = SNAP_DISTANCE / scaleFactor;
    double bestDist = minInfoDist;

    if (m_objSnapEnabled) {
        const auto& primitives = m_scene->getPrimitives();

        // 1. Стандартные точки (Endpoint, Midpoint, Center, Quadrant)
        for (const auto& obj : primitives) {
            auto snaps = obj->getSnapPoints();
            for (const auto& sp : snaps) {
                double d = MathUtils::dist(mouseWorldPos, sp.p);
                if (d < bestDist) {
                    bestDist = d;
                    result.snapped = true;
                    result.point = sp.p;
                    result.type = sp.type;
                }
            }
        }

        // 2. Пересечения (Intersection)
        // Для примера только отрезки, можно расширить
        for (size_t i = 0; i < primitives.size(); ++i) {
            for (size_t j = i + 1; j < primitives.size(); ++j) {
                if (primitives[i]->getType() == PrimitiveType::Segment && primitives[j]->getType() == PrimitiveType::Segment) {
                    auto* s1 = static_cast<Segment*>(primitives[i].get());
                    auto* s2 = static_cast<Segment*>(primitives[j].get());
                    auto inter = getSegmentIntersection(s1->getStart(), s1->getEnd(), s2->getStart(), s2->getEnd());
                    if (inter) {
                        double d = MathUtils::dist(mouseWorldPos, *inter);
                        if (d < bestDist) {
                            bestDist = d;
                            result.snapped = true;
                            result.point = *inter;
                            result.type = SnapType::Intersection;
                        }
                    }
                }
            }
        }

        // Если нашли жесткую привязку, возвращаем её.
        if (result.snapped) return result;

        // 3. Динамические привязки (Tangent, Perpendicular) - требуют начальной точки
        if (prevPoint.has_value()) {
            for (const auto& obj : primitives) {
                // Perpendicular
                auto perp = obj->getPerpendicularPoint(prevPoint.value());
                if (perp.has_value()) {
                    double d = MathUtils::dist(mouseWorldPos, *perp);
                    if (d < bestDist) {
                        bestDist = d;
                        result.snapped = true;
                        result.point = *perp;
                        result.type = SnapType::Perpendicular;
                    }
                }
                // Tangent
                auto tans = obj->getTangentPoints(prevPoint.value());
                for (const auto& p : tans) {
                    double d = MathUtils::dist(mouseWorldPos, p);
                    if (d < bestDist) {
                        bestDist = d;
                        result.snapped = true;
                        result.point = p;
                        result.type = SnapType::Tangent;
                    }
                }
            }
        }

        if (result.snapped) return result;

        // 4. Nearest (Ближайшая на объекте)
        for (const auto& obj : primitives) {
            Point p = obj->getClosestPoint(mouseWorldPos);
            double d = MathUtils::dist(mouseWorldPos, p);
            if (d < bestDist) {
                bestDist = d;
                result.snapped = true;
                result.point = p;
                result.type = SnapType::Nearest;
            }
        }
    }

    // 5. Grid (самый низкий приоритет)
    if (!result.snapped && m_gridSnapEnabled && m_gridStep > 0) {
        double gs = static_cast<double>(m_gridStep);
        double x = std::round(mouseWorldPos.getX() / gs) * gs;
        double y = std::round(mouseWorldPos.getY() / gs) * gs;

        if (MathUtils::dist(mouseWorldPos, Point(x, y)) < minInfoDist) {
            result.point = Point(x, y);
            result.snapped = true;
            result.type = SnapType::None; // Сетка без иконки
        }
    }

    return result;
}
