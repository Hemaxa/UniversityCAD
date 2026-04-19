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
            for (int i = 0; i < static_cast<int>(snaps.size()); ++i) {
                const auto& sp = snaps[i];
                double d = MathUtils::dist(mouseWorldPos, sp.p);
                if (d < bestDist) {
                    bestDist = d;
                    result.snapped = true;
                    result.point = sp.p;
                    result.type = sp.type;
                    result.object = sp.object ? sp.object : obj.get();
                    result.snapIndex = sp.index >= 0 ? sp.index : i;
                }
            }
        }

        // 2. Пересечения (Intersection)
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

        // NO EARLY RETURN HERE. Let Dynamic snaps compete.

        // 3. Динамические привязки (Tangent, Perpendicular) - требуют начальной точки
        if (prevPoint.has_value()) {
            for (const auto& obj : primitives) {
                // Perpendicular
                auto perp = obj->getPerpendicularPoint(prevPoint.value());
                if (perp.has_value()) {
                    double d = MathUtils::dist(mouseWorldPos, *perp);
                    // Give priority to perpendicular if it's close enough (maybe slightly larger radius or equal)
                    if (d < bestDist) {
                        bestDist = d;
                        result.snapped = true;
                        result.point = *perp;
                        result.type = SnapType::Perpendicular;
                    }
                }
                
                // Tangent - ИСПРАВЛЕННАЯ ЛОГИКА:
                // Срабатывает когда курсор близко к КАСАТЕЛЬНОЙ ЛИНИИ (не к кривой!)
                // Это гарантирует что отрезок будет касаться окружности, а не проходить через неё
                auto tangentSnap = obj->getTangentSnapPoint(prevPoint.value(), mouseWorldPos);
                if (tangentSnap.has_value()) {
                    auto [tangentPoint, projectedPoint] = tangentSnap.value();
                    
                    // Расстояние от курсора до касательной линии = расстояние до проекции
                    double distToTangentLine = MathUtils::dist(mouseWorldPos, projectedPoint);
                    
                    // Срабатываем только если курсор близко к касательной ЛИНИИ
                    if (distToTangentLine < minInfoDist && distToTangentLine < bestDist) {
                        bestDist = distToTangentLine;
                        result.snapped = true;
                        result.point = projectedPoint;  // Проекция курсора на касательную линию
                        result.type = SnapType::Tangent;
                        
                        // Вычисляем точку расширения (продолжение линии от tangentPoint)
                        double dx = tangentPoint.getX() - prevPoint.value().getX();
                        double dy = tangentPoint.getY() - prevPoint.value().getY();
                        double len = std::sqrt(dx * dx + dy * dy);
                        if (len > 1e-9) {
                            // Продлеваем линию на 500 единиц за точку касания
                            double extLength = 500.0;
                            Point extEnd(
                                tangentPoint.getX() + (dx / len) * extLength,
                                tangentPoint.getY() + (dy / len) * extLength
                            );
                            result.tangentExtensionLine = std::make_pair(tangentPoint, extEnd);
                        }
                    }
                }
            }
        }
        if (result.snapped && result.type != SnapType::Nearest && result.type != SnapType::Tangent) return result;

        // 4. Nearest (Ближайшая на объекте)
        // Calculating nearest independent of Tangent to compare
        SnapResult nearestRes;
        double nearestDist = minInfoDist;
        
        for (const auto& obj : primitives) {
            Point p = obj->getClosestPoint(mouseWorldPos);
            double d = MathUtils::dist(mouseWorldPos, p);
            if (d < nearestDist) {
                nearestDist = d;
                nearestRes.snapped = true;
                nearestRes.point = p;
                nearestRes.type = SnapType::Nearest;
                nearestRes.object = obj.get();
                nearestRes.snapIndex = -1;
            }
        }

        // If we have a Tangent candidate from step 3 (stored in result)
        if (result.snapped && result.type == SnapType::Tangent) {
            // If we also have a Nearest candidate
            if (nearestRes.snapped) {
                 // Prioritize Tangent unless Nearest is SIGNIFICANTLY closer (e.g. user moved away from tangent point to another part of circle)
                 // But typically if we are finding a tangent, we want it "sticky".
                 // If the mouse is within checking distance of Tangent, we kept it.
                 // So if we have tangent, keep it unless nearest is way better?
                 // Actually, if we have Tangent, we usually prefer it over generic "nearest point on line".
                 // So do nothing, keep result. 
            }
        } else {
            // No tangent found (or other hard snap), use nearest if found
            if (nearestRes.snapped) {
                result = nearestRes;
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
