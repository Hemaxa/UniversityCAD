#include "Tools.h"
#include "Snapper.h"
#include "Segment.h"
#include "Circle.h"
#include "Rectangle.h"
#include "Arc.h"
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include "MathUtils.h"
#include "Properties.h"
#include <cmath>
#include <algorithm> // Для std::min, std::max
#include <QPen>

// Хелпер для обновления привязки с учетом контекста (предыдущей точки)
void updateSnap(const Point& worldPos, const Snapper& snapper, double scale,
                const std::optional<Point>& prevPoint,
                Point& outPos, Point& outSnapPt, bool& outIsSnapped) {
    // Передаем prevPoint в метод snap для поиска касательных и перпендикуляров
    auto res = snapper.snap(worldPos, scale, prevPoint);
    if (res.snapped) {
        outPos = res.point;
        outSnapPt = res.point;
        outIsSnapped = true;
    } else {
        outPos = worldPos;
        outIsSnapped = false;
    }
}

void drawSnapMarker(QPainter& painter, const Point& p, double scale) {
    double s = 10.0 / scale;
    painter.setPen(QPen(Qt::green, 2.0 / scale));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(p.getX() - s/2, p.getY() - s/2, s, s));
}

// --- Segment ---
void CreateSegmentTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    // Передаем m_startPoint как контекст, если он есть
    updateSnap(worldPos, snapper, scale, m_startPoint, target, m_snapPoint, m_isSnapped);

    if (!m_startPoint.has_value()) {
        m_startPoint = target;
        m_endPoint = target;
    } else {
        m_endPoint = target;
        m_result = std::make_unique<Segment>(m_startPoint.value(), m_endPoint);
        m_finished = true;
    }
}

void CreateSegmentTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    updateSnap(worldPos, snapper, scale, m_startPoint, m_endPoint, m_snapPoint, m_isSnapped);
}

void CreateSegmentTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    if (m_startPoint.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
        painter.drawLine(QPointF(m_startPoint->getX(), m_startPoint->getY()), QPointF(m_endPoint.getX(), m_endPoint.getY()));
    }
}

std::unique_ptr<Object> CreateSegmentTool::takeObject() { return std::move(m_result); }
void CreateSegmentTool::reset() { m_finished = false; m_startPoint.reset(); }

// --- Circle ---
CreateCircleTool::CreateCircleTool(int method) : m_method(method) {}

void CreateCircleTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    // Контекст - последняя нажатая точка, если есть
    std::optional<Point> prev = m_clicks.empty() ? std::nullopt : std::make_optional(m_clicks.back());
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);

    m_clicks.push_back(target);

    // 0: Center + Radius
    if (m_method == 0) {
        if (m_clicks.size() == 2) {
            double r = MathUtils::dist(m_clicks[0], m_clicks[1]);
            m_result = std::make_unique<Circle>(m_clicks[0], r);
            m_finished = true;
        }
    }
    // 1: Center + Diameter
    else if (m_method == 1) {
        if (m_clicks.size() == 2) {
            double d = MathUtils::dist(m_clicks[0], m_clicks[1]);
            m_result = std::make_unique<Circle>(m_clicks[0], d / 2.0);
            m_finished = true;
        }
    }
    // 2: 2 Points (Diameter)
    else if (m_method == 2) {
        if (m_clicks.size() == 2) {
            Point center((m_clicks[0].getX() + m_clicks[1].getX())/2.0, (m_clicks[0].getY() + m_clicks[1].getY())/2.0);
            double r = MathUtils::dist(m_clicks[0], m_clicks[1]) / 2.0;
            m_result = std::make_unique<Circle>(center, r);
            m_finished = true;
        }
    }
    // 3: 3 Points
    else if (m_method == 3) {
        if (m_clicks.size() == 3) {
            Point center; double r;
            if (MathUtils::getCircleFrom3Points(m_clicks[0], m_clicks[1], m_clicks[2], center, r)) {
                m_result = std::make_unique<Circle>(center, r);
                m_finished = true;
            } else {
                m_clicks.pop_back();
            }
        }
    }
}

void CreateCircleTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    std::optional<Point> prev = m_clicks.empty() ? std::nullopt : std::make_optional(m_clicks.back());
    updateSnap(worldPos, snapper, scale, prev, m_cursorPos, m_snapPoint, m_isSnapped);
}

void CreateCircleTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);

    if (m_clicks.empty()) return;

    if (m_method == 0) { // Center + R
        double r = MathUtils::dist(m_clicks[0], m_cursorPos);
        painter.drawEllipse(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), r, r);
    }
    else if (m_method == 1) { // Center + D
        double d_dist = MathUtils::dist(m_clicks[0], m_cursorPos);
        painter.drawEllipse(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), d_dist, d_dist);
    }
    else if (m_method == 2) { // 2 Points (Diameter)
        Point center((m_clicks[0].getX() + m_cursorPos.getX())/2, (m_clicks[0].getY() + m_cursorPos.getY())/2);
        double r = MathUtils::dist(m_clicks[0], m_cursorPos) / 2.0;
        painter.drawEllipse(QPointF(center.getX(), center.getY()), r, r);
    }
    else if (m_method == 3) {
        if (m_clicks.size() == 2) {
            Point center; double r;
            if (MathUtils::getCircleFrom3Points(m_clicks[0], m_clicks[1], m_cursorPos, center, r)) {
                painter.drawEllipse(QPointF(center.getX(), center.getY()), r, r);
            }
        }
    }
}

std::unique_ptr<Object> CreateCircleTool::takeObject() { return std::move(m_result); }
void CreateCircleTool::reset() { m_finished = false; m_clicks.clear(); }

// --- Rectangle ---
CreateRectangleTool::CreateRectangleTool(int method) : m_method(method) {}

void CreateRectangleTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    std::optional<Point> prev = m_start;
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);

    // 0: 2 points (Corner to Corner)
    if (m_method == 0) {
        if (!m_start.has_value()) {
            m_start = target;
            m_end = target;
        }
        else {
            m_end = target;
            // ИСПРАВЛЕНИЕ: Прямоугольник между двумя углами
            double minX = std::min(m_start->getX(), m_end.getX());
            double maxY = std::max(m_start->getY(), m_end.getY()); // В мире Y вверх
            double w = std::abs(m_end.getX() - m_start->getX());
            double h = std::abs(m_end.getY() - m_start->getY());

            Point tl(minX, maxY);
            m_result = std::make_unique<Rectangle>(tl, w, h);
            m_finished = true;
        }
    }
    // 2: Center + Size
    else if (m_method == 2) {
        if (!m_start.has_value()) {
            m_start = target; // Center
            m_end = target;
        }
        else {
            m_end = target;
            double halfW = std::abs(m_end.getX() - m_start->getX());
            double halfH = std::abs(m_end.getY() - m_start->getY());
            Point tl(m_start->getX() - halfW, m_start->getY() + halfH);
            m_result = std::make_unique<Rectangle>(tl, halfW*2, halfH*2);
            m_finished = true;
        }
    }
}

void CreateRectangleTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    updateSnap(worldPos, snapper, scale, m_start, m_end, m_snapPoint, m_isSnapped);
}

void CreateRectangleTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    if (m_start.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);

        if (m_method == 0) {
            // Рисуем рамку
            double minX = std::min(m_start->getX(), m_end.getX());
            double maxY = std::max(m_start->getY(), m_end.getY());
            double w = std::abs(m_end.getX() - m_start->getX());
            double h = std::abs(m_end.getY() - m_start->getY());
            // Рисуем от верхнего левого вниз
            painter.drawRect(QRectF(minX, maxY - h, w, h));
        } else if (m_method == 2) {
            double halfW = std::abs(m_end.getX() - m_start->getX());
            double halfH = std::abs(m_end.getY() - m_start->getY());
            painter.drawRect(QRectF(m_start->getX() - halfW, m_start->getY() - halfH, halfW*2, halfH*2));
        }
    }
}

std::unique_ptr<Object> CreateRectangleTool::takeObject() { return std::move(m_result); }
void CreateRectangleTool::reset() { m_finished = false; m_start.reset(); }

// --- Arc ---
CreateArcTool::CreateArcTool(int method) : m_method(method) {}

void CreateArcTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    std::optional<Point> prev = m_clicks.empty() ? std::nullopt : std::make_optional(m_clicks.back());
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);

    m_clicks.push_back(target);

    // 0: Center, Start (radius), Span
    if (m_method == 0) {
        if (m_clicks.size() == 2) {
            // Center -> Start point (определяет R и start angle)
        } else if (m_clicks.size() == 3) {
            Point center = m_clicks[0];
            double r = MathUtils::dist(center, m_clicks[1]);
            double start = std::atan2(m_clicks[1].getY() - center.getY(), m_clicks[1].getX() - center.getX()) * 180.0 / M_PI;
            double end = std::atan2(m_clicks[2].getY() - center.getY(), m_clicks[2].getX() - center.getX()) * 180.0 / M_PI;
            double span = end - start;
            if (span < -180) span += 360; if (span > 180) span -= 360;
            m_result = std::make_unique<Arc>(center, r, start, span);
            m_finished = true;
        }
    }
    // 1: 3 Points
    else if (m_method == 1) {
        if (m_clicks.size() == 3) {
            Point c; double r;
            if (MathUtils::getCircleFrom3Points(m_clicks[0], m_clicks[1], m_clicks[2], c, r)) {
                double start = std::atan2(m_clicks[0].getY() - c.getY(), m_clicks[0].getX() - c.getX()) * 180.0 / M_PI;
                double end = std::atan2(m_clicks[2].getY() - c.getY(), m_clicks[2].getX() - c.getX()) * 180.0 / M_PI;
                double span = end - start;
                // Проверка ориентации (простая версия)
                double mid = std::atan2(m_clicks[1].getY() - c.getY(), m_clicks[1].getX() - c.getX()) * 180.0 / M_PI;
                auto norm = [](double a){ while(a<0) a+=360; while(a>=360) a-=360; return a; };
                double s = norm(start), e = norm(end), m = norm(mid);
                bool normal = (s<e) ? (m>s && m<e) : (m>s || m<e);
                if (!normal) { span = span - 360; if(span < -360) span += 360; }
                if (span == 0) span = 360;
                m_result = std::make_unique<Arc>(c, r, start, span);
                m_finished = true;
            } else m_clicks.pop_back();
        }
    }
}

void CreateArcTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    std::optional<Point> prev = m_clicks.empty() ? std::nullopt : std::make_optional(m_clicks.back());
    updateSnap(worldPos, snapper, scale, prev, m_cursorPos, m_snapPoint, m_isSnapped);
}

void CreateArcTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);

    if (m_method == 0) {
        if (m_clicks.size() >= 1) {
            painter.drawLine(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
            if (m_clicks.size() == 2) {
                double r = MathUtils::dist(m_clicks[0], m_clicks[1]);
                painter.drawEllipse(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), r, r);
            }
        }
    }
}

std::unique_ptr<Object> CreateArcTool::takeObject() { return std::move(m_result); }
void CreateArcTool::reset() { m_finished = false; m_clicks.clear(); }

// --- Ellipse ---
void CreateEllipseTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    std::optional<Point> prev = m_center;
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);

    if (!m_center.has_value()) { m_center = target; }
    else {
        m_rx = std::abs(target.getX() - m_center->getX());
        m_ry = std::abs(target.getY() - m_center->getY());
        if(m_rx < 1e-3) m_rx = 1.0; if(m_ry < 1e-3) m_ry = 1.0;
        m_result = std::make_unique<Ellipse>(m_center.value(), m_rx, m_ry);
        m_finished = true;
    }
}

void CreateEllipseTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    updateSnap(worldPos, snapper, scale, m_center, target, m_snapPoint, m_isSnapped);

    if (m_center.has_value()) {
        m_rx = std::abs(target.getX() - m_center->getX());
        m_ry = std::abs(target.getY() - m_center->getY());
    }
}

void CreateEllipseTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    if (m_center.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
        painter.drawEllipse(QPointF(m_center->getX(), m_center->getY()), m_rx, m_ry);
    }
}

std::unique_ptr<Object> CreateEllipseTool::takeObject() { return std::move(m_result); }
void CreateEllipseTool::reset() { m_finished = false; m_center.reset(); }

// --- Polygon ---
void CreatePolygonTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    std::optional<Point> prev = m_center;
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);

    if (!m_center.has_value()) { m_center = target; }
    else {
        m_radius = MathUtils::dist(target, m_center.value());
        m_result = std::make_unique<PolygonObj>(m_center.value(), m_radius, 5);
        m_finished = true;
    }
}

void CreatePolygonTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    updateSnap(worldPos, snapper, scale, m_center, target, m_snapPoint, m_isSnapped);
    if (m_center.has_value()) m_radius = MathUtils::dist(target, m_center.value());
}

void CreatePolygonTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    if (m_center.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
        painter.drawEllipse(QPointF(m_center->getX(), m_center->getY()), m_radius, m_radius);
    }
}

std::unique_ptr<Object> CreatePolygonTool::takeObject() { return std::move(m_result); }
void CreatePolygonTool::reset() { m_finished = false; m_center.reset(); }

// --- Spline ---
void CreateSplineTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    std::optional<Point> prev = m_points.empty() ? std::nullopt : std::make_optional(m_points.back());
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);
    m_points.push_back(target);
}

void CreateSplineTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    std::optional<Point> prev = m_points.empty() ? std::nullopt : std::make_optional(m_points.back());
    updateSnap(worldPos, snapper, scale, prev, m_currentPos, m_snapPoint, m_isSnapped);
}

void CreateSplineTool::finish() {
    if (m_points.size() >= 2) {
        m_result = std::make_unique<Spline>(m_points);
        m_finished = true;
    } else {
        reset();
    }
}

void CreateSplineTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
    if (!m_points.empty()) {
        for(size_t i=0; i<m_points.size()-1; ++i)
            painter.drawLine(QPointF(m_points[i].getX(), m_points[i].getY()), QPointF(m_points[i+1].getX(), m_points[i+1].getY()));
        painter.drawLine(QPointF(m_points.back().getX(), m_points.back().getY()), QPointF(m_currentPos.getX(), m_currentPos.getY()));
    }
}

std::unique_ptr<Object> CreateSplineTool::takeObject() { return std::move(m_result); }
void CreateSplineTool::reset() { m_finished = false; m_points.clear(); }
