#include "Dimension.h"
#include "GlobalSettings.h"
#include "Segment.h"
#include "Rectangle.h"
#include "Circle.h"
#include "Arc.h"
#include "Ellipse.h"
#include "Polygon.h"

#include <QtMath>
#include <algorithm>

Point DimensionAnchor::resolve() const
{
    if (object && object->getType() == PrimitiveType::Dimension) {
        return fallback;
    }
    if (object && snapIndex >= 0) {
        auto snaps = object->getSnapPoints();
        if (snapIndex < static_cast<int>(snaps.size())) {
            return snaps[snapIndex].p;
        }
    }
    if (object) {
        return object->getClosestPoint(fallback);
    }
    return fallback;
}

Dimension::Dimension(DimensionType type, const DimensionAnchor& a, const DimensionAnchor& b, const Point& linePoint)
    : m_type(type), m_a(a), m_b(b), m_linePoint(linePoint)
{
    applyGlobalStyle();
}

void Dimension::applyGlobalStyle()
{
    const auto& s = GlobalSettings::instance().dimensionStyle;
    m_extensionColor = s.extensionColor;
    m_dimensionColor = s.dimensionColor;
    m_textColor = s.textColor;
    m_extensionLineStyle = s.extensionLineStyle;
    m_dimensionLineStyle = s.dimensionLineStyle;
    m_extensionOvershoot = s.extensionOvershoot;
    m_dimensionExtension = s.dimensionExtension;
    m_arrowType = s.arrowType;
    m_arrowPlacement = s.arrowPlacement;
    m_arrowSize = s.arrowSize;
    m_arrowFilled = s.arrowFilled;
    m_fontFamily = s.fontFamily;
    m_textHeight = s.textHeight;
    m_textOffset = s.textOffset;
    setColor(m_dimensionColor);
    setLineStyle(m_dimensionLineStyle);
}

double Dimension::measuredValue() const
{
    Point a = m_a.resolve();
    Point b = m_b.resolve();
    switch (m_type) {
    case DimensionType::Horizontal:
        return std::abs(b.getX() - a.getX());
    case DimensionType::Vertical:
        return std::abs(b.getY() - a.getY());
    case DimensionType::Radius:
        if (m_a.object && m_a.object->getType() == PrimitiveType::Circle) {
            return static_cast<const Circle*>(m_a.object)->getRadius();
        }
        if (m_a.object && m_a.object->getType() == PrimitiveType::Arc) {
            return static_cast<const Arc*>(m_a.object)->getRadius();
        }
        if (m_a.object && m_a.object->getType() == PrimitiveType::Ellipse) {
            return MathUtils::dist(a, b);
        }
        return MathUtils::dist(a, b);
    case DimensionType::Diameter:
        if (m_a.object && m_a.object->getType() == PrimitiveType::Circle) {
            return static_cast<const Circle*>(m_a.object)->getRadius() * 2.0;
        }
        if (m_a.object && m_a.object->getType() == PrimitiveType::Arc) {
            return static_cast<const Arc*>(m_a.object)->getRadius() * 2.0;
        }
        if (m_a.object && m_a.object->getType() == PrimitiveType::Ellipse) {
            return MathUtils::dist(a, b) * 2.0;
        }
        return MathUtils::dist(a, b) * 2.0;
    case DimensionType::Angular: {
        const double a1 = std::atan2(a.getY() - m_linePoint.getY(), a.getX() - m_linePoint.getX());
        const double a2 = std::atan2(b.getY() - m_linePoint.getY(), b.getX() - m_linePoint.getX());
        double deg = std::abs(qRadiansToDegrees(a2 - a1));
        if (deg > 180.0) deg = 360.0 - deg;
        return deg;
    }
    case DimensionType::Linear:
    default:
        return MathUtils::dist(a, b);
    }
}

QString Dimension::displayText() const
{
    if (hasTextOverride()) return m_textOverride;
    const QString suffix = (m_type == DimensionType::Angular) ? QString::fromUtf8("°") : QString();
    const QString prefix = (m_type == DimensionType::Radius) ? "R" : (m_type == DimensionType::Diameter ? QString::fromUtf8("Ø") : QString());
    return QString("%1%2%3").arg(prefix).arg(measuredValue(), 0, 'f', 2).arg(suffix);
}

void Dimension::setTextPositionFactor(double factor)
{
    m_textPositionFactor = std::clamp(factor, 0.0, 1.0);
}

static Point interpolate(const Point& a, const Point& b, double t)
{
    return Point(a.getX() + (b.getX() - a.getX()) * t,
                 a.getY() + (b.getY() - a.getY()) * t);
}

static Point offsetPoint(const Point& p, double nx, double ny, double amount)
{
    return Point(p.getX() + nx * amount, p.getY() + ny * amount);
}

static double projectFactorOnSegment(const Point& p, const Point& a, const Point& b)
{
    const double dx = b.getX() - a.getX();
    const double dy = b.getY() - a.getY();
    const double len2 = dx * dx + dy * dy;
    if (len2 < MathUtils::EPSILON) return 0.5;
    const double t = ((p.getX() - a.getX()) * dx + (p.getY() - a.getY()) * dy) / len2;
    return std::clamp(t, 0.0, 1.0);
}

static double shortestArcDelta(double a1, double a2)
{
    double delta = std::fmod(a2 - a1, 2.0 * M_PI);
    if (delta > M_PI) delta -= 2.0 * M_PI;
    if (delta < -M_PI) delta += 2.0 * M_PI;
    return delta;
}

Point Dimension::getTextPosition() const
{
    const double t = std::clamp(m_textPositionFactor, 0.0, 1.0);
    Point a = m_a.resolve();
    Point b = m_b.resolve();
    if (m_type == DimensionType::Horizontal) {
        return Point(a.getX() + (b.getX() - a.getX()) * t, m_linePoint.getY() + m_textOffset);
    }
    if (m_type == DimensionType::Vertical) {
        return Point(m_linePoint.getX() - std::abs(m_textOffset), a.getY() + (b.getY() - a.getY()) * t);
    }
    if (m_type == DimensionType::Radius || m_type == DimensionType::Diameter) {
        const double r = (m_type == DimensionType::Diameter) ? measuredValue() * 0.5 : measuredValue();
        double dx = m_linePoint.getX() - a.getX();
        double dy = m_linePoint.getY() - a.getY();
        double len = std::hypot(dx, dy);
        if (len < MathUtils::EPSILON) {
            dx = b.getX() - a.getX();
            dy = b.getY() - a.getY();
            len = std::hypot(dx, dy);
        }
        if (len < MathUtils::EPSILON) return m_linePoint;
        dx /= len;
        dy /= len;
        Point p1 = (m_type == DimensionType::Diameter)
            ? Point(a.getX() - dx * r, a.getY() - dy * r)
            : a;
        Point p2(a.getX() + dx * r, a.getY() + dy * r);
        Point base = interpolate(p1, p2, t);
        return offsetPoint(base, -dy, dx, m_textOffset);
    }
    if (m_type == DimensionType::Linear) {
        const double vx = b.getX() - a.getX();
        const double vy = b.getY() - a.getY();
        const double len = std::hypot(vx, vy);
        if (len > MathUtils::EPSILON) {
            const double nx = -vy / len;
            const double ny = vx / len;
            const double off = (m_linePoint.getX() - a.getX()) * nx + (m_linePoint.getY() - a.getY()) * ny;
            return Point(a.getX() + vx * t + nx * (off + m_textOffset),
                         a.getY() + vy * t + ny * (off + m_textOffset));
        }
    }
    if (m_type == DimensionType::Angular) {
        const double a1 = std::atan2(a.getY() - m_linePoint.getY(), a.getX() - m_linePoint.getX());
        const double a2 = std::atan2(b.getY() - m_linePoint.getY(), b.getX() - m_linePoint.getX());
        const double delta = shortestArcDelta(a1, a2);
        const double r = m_angularRadius > MathUtils::EPSILON
            ? m_angularRadius
            : std::max(15.0, std::min(MathUtils::dist(m_linePoint, a), MathUtils::dist(m_linePoint, b)) * 0.65);
        const double angle = a1 + delta * t;
        return Point(m_linePoint.getX() + std::cos(angle) * (r + m_textOffset),
                     m_linePoint.getY() + std::sin(angle) * (r + m_textOffset));
    }
    return interpolate(a, b, t);
}

Point Dimension::getLineGripPosition() const
{
    Point a = m_a.resolve();
    Point b = m_b.resolve();
    if (m_type == DimensionType::Horizontal) {
        return Point((a.getX() + b.getX()) / 2.0, m_linePoint.getY());
    }
    if (m_type == DimensionType::Vertical) {
        return Point(m_linePoint.getX(), (a.getY() + b.getY()) / 2.0);
    }
    if (m_type == DimensionType::Linear) {
        const double vx = b.getX() - a.getX();
        const double vy = b.getY() - a.getY();
        const double len = std::hypot(vx, vy);
        if (len > MathUtils::EPSILON) {
            const double nx = -vy / len;
            const double ny = vx / len;
            const double off = (m_linePoint.getX() - a.getX()) * nx + (m_linePoint.getY() - a.getY()) * ny;
            return Point((a.getX() + b.getX()) / 2.0 + nx * off,
                         (a.getY() + b.getY()) / 2.0 + ny * off);
        }
    }
    if (m_type == DimensionType::Angular) {
        const double a1 = std::atan2(a.getY() - m_linePoint.getY(), a.getX() - m_linePoint.getX());
        const double a2 = std::atan2(b.getY() - m_linePoint.getY(), b.getX() - m_linePoint.getX());
        const double delta = shortestArcDelta(a1, a2);
        const double r = m_angularRadius > MathUtils::EPSILON
            ? m_angularRadius
            : std::max(15.0, std::min(MathUtils::dist(m_linePoint, a), MathUtils::dist(m_linePoint, b)) * 0.65);
        const double angle = a1 + delta * 0.5;
        return Point(m_linePoint.getX() + std::cos(angle) * r,
                     m_linePoint.getY() + std::sin(angle) * r);
    }
    return m_linePoint;
}

void Dimension::setTextPosition(const Point& p)
{
    Point a = m_a.resolve();
    Point b = m_b.resolve();

    if (m_type == DimensionType::Horizontal) {
        setTextPositionFactor(projectFactorOnSegment(p, Point(a.getX(), m_linePoint.getY()), Point(b.getX(), m_linePoint.getY())));
        return;
    }
    if (m_type == DimensionType::Vertical) {
        setTextPositionFactor(projectFactorOnSegment(p, Point(m_linePoint.getX(), a.getY()), Point(m_linePoint.getX(), b.getY())));
        return;
    }
    if (m_type == DimensionType::Linear) {
        const double vx = b.getX() - a.getX();
        const double vy = b.getY() - a.getY();
        const double len = std::hypot(vx, vy);
        if (len > MathUtils::EPSILON) {
            const double nx = -vy / len;
            const double ny = vx / len;
            const double off = (m_linePoint.getX() - a.getX()) * nx + (m_linePoint.getY() - a.getY()) * ny;
            Point da(a.getX() + nx * off, a.getY() + ny * off);
            Point db(b.getX() + nx * off, b.getY() + ny * off);
            setTextPositionFactor(projectFactorOnSegment(p, da, db));
        }
        return;
    }
    if (m_type == DimensionType::Radius || m_type == DimensionType::Diameter) {
        const double r = (m_type == DimensionType::Diameter) ? measuredValue() * 0.5 : measuredValue();
        double dx = m_linePoint.getX() - a.getX();
        double dy = m_linePoint.getY() - a.getY();
        double len = std::hypot(dx, dy);
        if (len < MathUtils::EPSILON) {
            dx = b.getX() - a.getX();
            dy = b.getY() - a.getY();
            len = std::hypot(dx, dy);
        }
        if (len > MathUtils::EPSILON) {
            dx /= len;
            dy /= len;
            Point p1 = (m_type == DimensionType::Diameter)
                ? Point(a.getX() - dx * r, a.getY() - dy * r)
                : a;
            Point p2(a.getX() + dx * r, a.getY() + dy * r);
            setTextPositionFactor(projectFactorOnSegment(p, p1, p2));
        }
        return;
    }
    if (m_type == DimensionType::Angular) {
        const double a1 = std::atan2(a.getY() - m_linePoint.getY(), a.getX() - m_linePoint.getX());
        const double delta = shortestArcDelta(a1, std::atan2(b.getY() - m_linePoint.getY(), b.getX() - m_linePoint.getX()));
        if (std::abs(delta) > MathUtils::EPSILON) {
            double angle = std::atan2(p.getY() - m_linePoint.getY(), p.getX() - m_linePoint.getX());
            double rel = shortestArcDelta(a1, angle) / delta;
            setTextPositionFactor(rel);
        }
    }
}

static bool setSegmentLength(const DimensionAnchor& a, const DimensionAnchor& b, double value)
{
    if (!a.object || a.object != b.object || a.object->getType() != PrimitiveType::Segment) return false;
    if ((a.snapIndex != 0 && a.snapIndex != 1) || (b.snapIndex != 0 && b.snapIndex != 1)) return false;
    auto* s = const_cast<Segment*>(static_cast<const Segment*>(a.object));
    Point p1 = a.snapIndex == 0 ? s->getStart() : s->getEnd();
    Point p2 = a.snapIndex == 0 ? s->getEnd() : s->getStart();
    const double len = MathUtils::dist(p1, p2);
    if (len < MathUtils::EPSILON) return false;
    Point moved(p1.getX() + (p2.getX() - p1.getX()) / len * value,
                p1.getY() + (p2.getY() - p1.getY()) / len * value);
    if (a.snapIndex == 0) s->setEnd(moved);
    else s->setStart(moved);
    return true;
}

static void updateRectangleAnchorFallback(DimensionAnchor& anchor,
                                          double oldX, double oldY, double oldW, double oldH,
                                          double newW, double newH)
{
    const double eps = 1e-6;
    double x = anchor.fallback.getX();
    double y = anchor.fallback.getY();
    if (std::abs(x - (oldX + oldW)) < eps) x = oldX + newW;
    if (std::abs(y - (oldY - oldH)) < eps) y = oldY - newH;
    anchor.fallback = Point(x, y);
}

static bool sameObject(const DimensionAnchor& a, const DimensionAnchor& b, PrimitiveType type)
{
    return a.object && a.object == b.object && a.object->getType() == type;
}

static int signedDirection(double delta)
{
    return delta < 0.0 ? -1 : 1;
}

static bool setProjectedSegmentSize(const DimensionAnchor& a, const DimensionAnchor& b, double value, bool horizontal)
{
    if (!sameObject(a, b, PrimitiveType::Segment)) return false;
    if ((a.snapIndex != 0 && a.snapIndex != 1) || (b.snapIndex != 0 && b.snapIndex != 1)) return false;
    auto* s = const_cast<Segment*>(static_cast<const Segment*>(a.object));
    Point fixed = a.snapIndex == 0 ? s->getStart() : s->getEnd();
    Point moved = a.snapIndex == 0 ? s->getEnd() : s->getStart();
    if (horizontal) {
        const int sign = signedDirection(moved.getX() - fixed.getX());
        moved.setX(fixed.getX() + sign * value);
    } else {
        const int sign = signedDirection(moved.getY() - fixed.getY());
        moved.setY(fixed.getY() + sign * value);
    }
    if (a.snapIndex == 0) s->setEnd(moved);
    else s->setStart(moved);
    return true;
}

static bool pointsFormAxisDiameter(const Point& a, const Point& b, const Point& center, bool horizontal)
{
    const double eps = 1e-4;
    if (horizontal) {
        return std::abs(a.getY() - center.getY()) < eps
            && std::abs(b.getY() - center.getY()) < eps
            && (a.getX() - center.getX()) * (b.getX() - center.getX()) <= 0.0;
    }
    return std::abs(a.getX() - center.getX()) < eps
        && std::abs(b.getX() - center.getX()) < eps
        && (a.getY() - center.getY()) * (b.getY() - center.getY()) <= 0.0;
}

bool Dimension::applyMeasuredValue(double newValue)
{
    if (newValue <= 0) return false;
    if (m_type == DimensionType::Linear && m_a.object == m_b.object && m_a.object && m_a.object->getType() == PrimitiveType::Segment) {
        return setSegmentLength(m_a, m_b, newValue);
    }
    if ((m_type == DimensionType::Horizontal || m_type == DimensionType::Vertical) && sameObject(m_a, m_b, PrimitiveType::Segment)) {
        return setProjectedSegmentSize(m_a, m_b, newValue, m_type == DimensionType::Horizontal);
    }
    if (m_type == DimensionType::Linear && m_a.object == m_b.object && m_a.object && m_a.object->getType() == PrimitiveType::Rectangle) {
        auto* r = const_cast<Rectangle*>(static_cast<const Rectangle*>(m_a.object));
        Point a = m_a.resolve();
        Point b = m_b.resolve();
        const double oldX = r->getTopLeft().getX();
        const double oldY = r->getTopLeft().getY();
        const double oldW = r->getWidth();
        const double oldH = r->getHeight();
        if (std::abs(a.getY() - b.getY()) <= std::abs(a.getX() - b.getX())) {
            r->setWidth(newValue);
        } else {
            r->setHeight(newValue);
        }
        updateRectangleAnchorFallback(m_a, oldX, oldY, oldW, oldH, r->getWidth(), r->getHeight());
        updateRectangleAnchorFallback(m_b, oldX, oldY, oldW, oldH, r->getWidth(), r->getHeight());
        return true;
    }
    if ((m_type == DimensionType::Horizontal || m_type == DimensionType::Vertical) && m_a.object == m_b.object && m_a.object && m_a.object->getType() == PrimitiveType::Rectangle) {
        auto* r = const_cast<Rectangle*>(static_cast<const Rectangle*>(m_a.object));
        const double oldX = r->getTopLeft().getX();
        const double oldY = r->getTopLeft().getY();
        const double oldW = r->getWidth();
        const double oldH = r->getHeight();
        if (m_type == DimensionType::Horizontal) r->setWidth(newValue);
        else r->setHeight(newValue);
        updateRectangleAnchorFallback(m_a, oldX, oldY, oldW, oldH, r->getWidth(), r->getHeight());
        updateRectangleAnchorFallback(m_b, oldX, oldY, oldW, oldH, r->getWidth(), r->getHeight());
        return true;
    }
    if ((m_type == DimensionType::Horizontal || m_type == DimensionType::Vertical) && sameObject(m_a, m_b, PrimitiveType::Circle)) {
        auto* c = const_cast<Circle*>(static_cast<const Circle*>(m_a.object));
        c->setRadius(newValue / 2.0);
        return true;
    }
    if ((m_type == DimensionType::Horizontal || m_type == DimensionType::Vertical) && sameObject(m_a, m_b, PrimitiveType::Ellipse)) {
        auto* e = const_cast<Ellipse*>(static_cast<const Ellipse*>(m_a.object));
        if (m_type == DimensionType::Horizontal) e->setRadiusX(newValue / 2.0);
        else e->setRadiusY(newValue / 2.0);
        return true;
    }
    if (m_type == DimensionType::Linear && sameObject(m_a, m_b, PrimitiveType::Circle)) {
        auto* c = const_cast<Circle*>(static_cast<const Circle*>(m_a.object));
        Point a = m_a.resolve();
        Point b = m_b.resolve();
        const Point center = c->getCenter();
        const bool horizontalDiameter = pointsFormAxisDiameter(a, b, center, true);
        const bool verticalDiameter = pointsFormAxisDiameter(a, b, center, false);
        if (horizontalDiameter || verticalDiameter) {
            c->setRadius(newValue / 2.0);
            return true;
        }
    }
    if (m_type == DimensionType::Linear && sameObject(m_a, m_b, PrimitiveType::Ellipse)) {
        auto* e = const_cast<Ellipse*>(static_cast<const Ellipse*>(m_a.object));
        Point a = m_a.resolve();
        Point b = m_b.resolve();
        const Point center = e->getCenter();
        if (pointsFormAxisDiameter(a, b, center, true)) {
            e->setRadiusX(newValue / 2.0);
            return true;
        }
        if (pointsFormAxisDiameter(a, b, center, false)) {
            e->setRadiusY(newValue / 2.0);
            return true;
        }
    }
    if ((m_type == DimensionType::Radius || m_type == DimensionType::Diameter) && m_a.object && m_a.object->getType() == PrimitiveType::Circle) {
        auto* c = const_cast<Circle*>(static_cast<const Circle*>(m_a.object));
        c->setRadius(m_type == DimensionType::Diameter ? newValue / 2.0 : newValue);
        return true;
    }
    if ((m_type == DimensionType::Radius || m_type == DimensionType::Diameter) && m_a.object && m_a.object->getType() == PrimitiveType::Arc) {
        auto* a = const_cast<Arc*>(static_cast<const Arc*>(m_a.object));
        a->setRadius(m_type == DimensionType::Diameter ? newValue / 2.0 : newValue);
        return true;
    }
    if ((m_type == DimensionType::Radius || m_type == DimensionType::Diameter) && m_a.object && m_a.object->getType() == PrimitiveType::Ellipse) {
        auto* e = const_cast<Ellipse*>(static_cast<const Ellipse*>(m_a.object));
        Point center = e->getCenter();
        Point edge = m_b.resolve();
        const double dx = std::abs(edge.getX() - center.getX());
        const double dy = std::abs(edge.getY() - center.getY());
        double r = m_type == DimensionType::Diameter ? newValue / 2.0 : newValue;
        if (dx >= dy) e->setRadiusX(r);
        else e->setRadiusY(r);
        return true;
    }
    if ((m_type == DimensionType::Radius || m_type == DimensionType::Diameter) && m_a.object && m_a.object->getType() == PrimitiveType::Polygon) {
        auto* p = const_cast<PolygonObj*>(static_cast<const PolygonObj*>(m_a.object));
        p->setRadius(m_type == DimensionType::Diameter ? newValue / 2.0 : newValue);
        return true;
    }
    if ((m_type == DimensionType::Linear || m_type == DimensionType::Horizontal || m_type == DimensionType::Vertical)
        && sameObject(m_a, m_b, PrimitiveType::Polygon)) {
        auto* p = const_cast<PolygonObj*>(static_cast<const PolygonObj*>(m_a.object));
        const double current = measuredValue();
        if (current > MathUtils::EPSILON) {
            p->setRadius(p->getRadius() * (newValue / current));
            return true;
        }
    }
    if (m_type == DimensionType::Angular) {
        const Object* base = nullptr;
        if (m_a.object && m_a.object == m_b.object) base = m_a.object;
        else if (m_a.object && m_a.object->getType() == PrimitiveType::Arc) base = m_a.object;
        else if (m_b.object && m_b.object->getType() == PrimitiveType::Arc) base = m_b.object;

        if (base && base->getType() == PrimitiveType::Arc) {
            auto* arc = const_cast<Arc*>(static_cast<const Arc*>(base));
            const double sign = arc->getSpanAngle() < 0.0 ? -1.0 : 1.0;
            arc->setSpanAngle(sign * std::min(newValue, 360.0));
            return true;
        }
    }
    return false;
}

std::vector<SnapPoint> Dimension::getSnapPoints() const
{
    return {{m_a.resolve(), SnapType::Endpoint, this, 0}, {m_b.resolve(), SnapType::Endpoint, this, 1}, {getTextPosition(), SnapType::Center, this, 2}};
}

Point Dimension::getClosestPoint(const Point& p) const
{
    Point a = m_a.resolve();
    Point b = m_b.resolve();
    Point da = a;
    Point db = b;
    if (m_type == DimensionType::Horizontal) {
        da = Point(a.getX(), m_linePoint.getY());
        db = Point(b.getX(), m_linePoint.getY());
    } else if (m_type == DimensionType::Vertical) {
        da = Point(m_linePoint.getX(), a.getY());
        db = Point(m_linePoint.getX(), b.getY());
    } else if (m_type == DimensionType::Linear) {
        const double vx = b.getX() - a.getX();
        const double vy = b.getY() - a.getY();
        const double len = std::hypot(vx, vy);
        if (len > MathUtils::EPSILON) {
            const double nx = -vy / len;
            const double ny = vx / len;
            const double off = (m_linePoint.getX() - a.getX()) * nx + (m_linePoint.getY() - a.getY()) * ny;
            da = Point(a.getX() + nx * off, a.getY() + ny * off);
            db = Point(b.getX() + nx * off, b.getY() + ny * off);
        }
    } else if (m_type == DimensionType::Radius || m_type == DimensionType::Diameter) {
        const double r = (m_type == DimensionType::Diameter) ? measuredValue() * 0.5 : measuredValue();
        double dx = m_linePoint.getX() - a.getX();
        double dy = m_linePoint.getY() - a.getY();
        double len = std::hypot(dx, dy);
        if (len < MathUtils::EPSILON) return getTextPosition();
        dx /= len;
        dy /= len;
        da = (m_type == DimensionType::Diameter) ? Point(a.getX() - dx * r, a.getY() - dy * r) : a;
        db = Point(a.getX() + dx * r, a.getY() + dy * r);
    }
    Point onLine = MathUtils::projectPointOnSegment(p, da, db);
    Point text = getTextPosition();
    return MathUtils::dist(p, text) < MathUtils::dist(p, onLine) ? text : onLine;
}
