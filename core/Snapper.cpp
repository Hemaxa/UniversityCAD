#include "Snapper.h"
#include "Scene.h"
#include "Segment.h"
#include "Circle.h"
#include "Rectangle.h"
#include <cmath>
#include <limits>

Snapper::Snapper(const Scene* scene) : m_scene(scene) {}

void Snapper::setGridSnap(bool enabled, int step) { m_gridSnapEnabled = enabled; m_gridStep = step; }
void Snapper::setObjectSnap(bool enabled) { m_objSnapEnabled = enabled; }

static double dist(const Point& a, const Point& b) {
    return std::sqrt(std::pow(a.getX() - b.getX(), 2) + std::pow(a.getY() - b.getY(), 2));
}

SnapResult Snapper::snap(const Point& mouseWorldPos, double scaleFactor) const {
    SnapResult result;

    // 1. Привязка к объектам (если включена)
    if (m_objSnapEnabled && m_scene) {
        double minInfoDist = std::numeric_limits<double>::max();
        double tolerance = SNAP_DISTANCE / scaleFactor;

        auto checkPoint = [&](const Point& p, SnapType type) {
            double d = dist(mouseWorldPos, p);
            if (d < tolerance && d < minInfoDist) {
                minInfoDist = d;
                result.snapped = true;
                result.point = p;
                result.type = type;
            }
        };

        for (const auto& obj : m_scene->getPrimitives()) {
            switch (obj->getType()) {
            case PrimitiveType::Segment: {
                auto* s = static_cast<Segment*>(obj.get());
                checkPoint(s->getStart(), SnapType::Endpoint);
                checkPoint(s->getEnd(), SnapType::Endpoint);
                checkPoint(Point((s->getStart().getX() + s->getEnd().getX()) / 2,
                                 (s->getStart().getY() + s->getEnd().getY()) / 2),
                           SnapType::Midpoint);
                break;
            }
            case PrimitiveType::Circle: {
                auto* c = static_cast<Circle*>(obj.get());
                checkPoint(c->getCenter(), SnapType::Center);
                checkPoint(Point(c->getCenter().getX() + c->getRadius(), c->getCenter().getY()), SnapType::Quadrant);
                checkPoint(Point(c->getCenter().getX() - c->getRadius(), c->getCenter().getY()), SnapType::Quadrant);
                checkPoint(Point(c->getCenter().getX(), c->getCenter().getY() + c->getRadius()), SnapType::Quadrant);
                checkPoint(Point(c->getCenter().getX(), c->getCenter().getY() - c->getRadius()), SnapType::Quadrant);
                break;
            }
            case PrimitiveType::Rectangle: {
                auto* r = static_cast<Rectangle*>(obj.get());
                Point tl = r->getTopLeft();
                Point tr(tl.getX() + r->getWidth(), tl.getY());
                Point bl(tl.getX(), tl.getY() - r->getHeight());
                Point br(tl.getX() + r->getWidth(), tl.getY() - r->getHeight());
                checkPoint(tl, SnapType::Endpoint); checkPoint(tr, SnapType::Endpoint);
                checkPoint(bl, SnapType::Endpoint); checkPoint(br, SnapType::Endpoint);
                break;
            }
            default: break;
            }
        }
    }

    // 2. Привязка к сетке (если объектная не сработала)
    if (!result.snapped && m_gridSnapEnabled && m_gridStep > 0) {
        double gs = static_cast<double>(m_gridStep);
        double x = std::round(mouseWorldPos.getX() / gs) * gs;
        double y = std::round(mouseWorldPos.getY() / gs) * gs;

        // Проверяем дистанцию (опционально, можно магнитить всегда)
        // Для удобства магнитим всегда, если курсор "близко" к узлу, или просто всегда округляем координату
        result.point = Point(x, y);
        result.snapped = true;
        result.type = SnapType::None; // Тип None, но snapped = true - значит координаты модифицированы
    }

    return result;
}
