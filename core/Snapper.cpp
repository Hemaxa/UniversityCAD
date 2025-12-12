#include "Snapper.h"
#include "Scene.h"
#include "Segment.h"
#include <cmath>
#include <limits>

Snapper::Snapper(const Scene* scene) : m_scene(scene) {}

void Snapper::setGridSnap(bool enabled, int step) { m_gridSnapEnabled = enabled; m_gridStep = step; }
void Snapper::setObjectSnap(bool enabled) { m_objSnapEnabled = enabled; }

static double dist(const Point& a, const Point& b) {
    return std::sqrt(std::pow(a.getX() - b.getX(), 2) + std::pow(a.getY() - b.getY(), 2));
}

// Упрощенный расчет пересечения двух отрезков
std::optional<Point> getSegmentIntersection(const Point& p1, const Point& p2, const Point& p3, const Point& p4) {
    double det = (p2.getX() - p1.getX()) * (p4.getY() - p3.getY()) - (p4.getX() - p3.getX()) * (p2.getY() - p1.getY());
    if (std::abs(det) < 1e-9) return std::nullopt; // Параллельны

    double t = ((p3.getX() - p1.getX()) * (p4.getY() - p3.getY()) - (p4.getX() - p3.getX()) * (p3.getY() - p1.getY())) / det;
    double u = ((p3.getX() - p1.getX()) * (p2.getY() - p1.getY()) - (p2.getX() - p1.getX()) * (p3.getY() - p1.getY())) / det;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        return Point(p1.getX() + t * (p2.getX() - p1.getX()), p1.getY() + t * (p2.getY() - p1.getY()));
    }
    return std::nullopt;
}

SnapResult Snapper::snap(const Point& mouseWorldPos, double scaleFactor) const {
    SnapResult result;
    if (!m_scene) return result;

    double minInfoDist = SNAP_DISTANCE / scaleFactor;

    // 1. Привязка к объектам
    if (m_objSnapEnabled) {
        const auto& primitives = m_scene->getPrimitives();

        // А) Основные точки (делегируем объектам)
        for (const auto& obj : primitives) {
            auto snaps = obj->getSnapPoints();
            for (const auto& sp : snaps) {
                double d = dist(mouseWorldPos, sp.p);
                if (d < minInfoDist) {
                    minInfoDist = d;
                    result.snapped = true;
                    result.point = sp.p;
                    result.type = sp.type;
                }
            }
        }

        // Б) Пересечения (Intersection) - только для отрезков в рамках примера
        for (size_t i = 0; i < primitives.size(); ++i) {
            for (size_t j = i + 1; j < primitives.size(); ++j) {
                if (primitives[i]->getType() == PrimitiveType::Segment && primitives[j]->getType() == PrimitiveType::Segment) {
                    auto* s1 = static_cast<Segment*>(primitives[i].get());
                    auto* s2 = static_cast<Segment*>(primitives[j].get());
                    auto inter = getSegmentIntersection(s1->getStart(), s1->getEnd(), s2->getStart(), s2->getEnd());
                    if (inter) {
                        double d = dist(mouseWorldPos, *inter);
                        if (d < minInfoDist) {
                            minInfoDist = d;
                            result.snapped = true;
                            result.point = *inter;
                            result.type = SnapType::Intersection;
                        }
                    }
                }
            }
        }
    }

    // 2. Привязка к сетке (только если объектная не сработала лучше)
    if (!result.snapped && m_gridSnapEnabled && m_gridStep > 0) {
        double gs = static_cast<double>(m_gridStep);
        double x = std::round(mouseWorldPos.getX() / gs) * gs;
        double y = std::round(mouseWorldPos.getY() / gs) * gs;

        // Магнитим к сетке, если близко (или всегда, если так удобнее)
        if (dist(mouseWorldPos, Point(x, y)) < minInfoDist) {
            result.point = Point(x, y);
            result.snapped = true;
            result.type = SnapType::None;
        }
    }

    return result;
}
