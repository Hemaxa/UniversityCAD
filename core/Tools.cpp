#include "Tools.h"
#include "Snapper.h"
#include "Segment.h"
#include "Circle.h"
#include "Rectangle.h"
#include "Arc.h"
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include <cmath>
#include <QPen>

// Helper
void updateSnap(const Point& worldPos, const Snapper& snapper, double scale,
                Point& outPos, Point& outSnapPt, bool& outIsSnapped) {
    auto res = snapper.snap(worldPos, scale);
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
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    if (!m_startPoint.has_value()) { m_startPoint = target; m_endPoint = target; }
    else { m_endPoint = target; m_result = std::make_unique<Segment>(m_startPoint.value(), m_endPoint); m_finished = true; }
}
void CreateSegmentTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    updateSnap(worldPos, snapper, scale, m_endPoint, m_snapPoint, m_isSnapped);
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
void CreateCircleTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    if (!m_center.has_value()) { m_center = target; m_radius = 0; }
    else { m_radius = std::hypot(target.getX() - m_center->getX(), target.getY() - m_center->getY()); m_result = std::make_unique<Circle>(m_center.value(), m_radius); m_finished = true; }
}
void CreateCircleTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    if (m_center.has_value()) m_radius = std::hypot(target.getX() - m_center->getX(), target.getY() - m_center->getY());
}
void CreateCircleTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    if (m_center.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
        painter.drawEllipse(QPointF(m_center->getX(), m_center->getY()), m_radius, m_radius);
    }
}
std::unique_ptr<Object> CreateCircleTool::takeObject() { return std::move(m_result); }
void CreateCircleTool::reset() { m_finished = false; m_center.reset(); }

// --- Rectangle ---
void CreateRectangleTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    if (!m_start.has_value()) { m_start = target; m_end = target; }
    else {
        m_end = target;
        double w = std::abs(m_end.getX() - m_start->getX()); double h = std::abs(m_end.getY() - m_start->getY());
        Point tl(std::min(m_start->getX(), m_end.getX()), std::max(m_start->getY(), m_end.getY()));
        m_result = std::make_unique<Rectangle>(tl, w, h); m_finished = true;
    }
}
void CreateRectangleTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    updateSnap(worldPos, snapper, scale, m_end, m_snapPoint, m_isSnapped);
}
void CreateRectangleTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    if (m_start.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
        painter.drawRect(QRectF(QPointF(m_start->getX(), m_start->getY()), QPointF(m_end.getX(), m_end.getY())));
    }
}
std::unique_ptr<Object> CreateRectangleTool::takeObject() { return std::move(m_result); }
void CreateRectangleTool::reset() { m_finished = false; m_start.reset(); }

// --- Arc ---
void CreateArcTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    if (m_step == 0) {
        m_center = target; m_step = 1;
    } else if (m_step == 1) {
        m_radius = std::hypot(target.getX() - m_center->getX(), target.getY() - m_center->getY());
        m_startAngle = std::atan2(target.getY() - m_center->getY(), target.getX() - m_center->getX()) * 180.0 / M_PI;
        m_step = 2;
    } else if (m_step == 2) {
        double endAngle = std::atan2(target.getY() - m_center->getY(), target.getX() - m_center->getX()) * 180.0 / M_PI;
        m_spanAngle = endAngle - m_startAngle;
        if (m_spanAngle < -180) m_spanAngle += 360; if (m_spanAngle > 180) m_spanAngle -= 360;
        m_result = std::make_unique<Arc>(m_center.value(), m_radius, m_startAngle, m_spanAngle);
        m_finished = true;
    }
}
void CreateArcTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    updateSnap(worldPos, snapper, scale, m_cursorPos, m_snapPoint, m_isSnapped);
    if (m_step == 1 && m_center) {
        m_radius = std::hypot(m_cursorPos.getX() - m_center->getX(), m_cursorPos.getY() - m_center->getY());
        m_startAngle = std::atan2(m_cursorPos.getY() - m_center->getY(), m_cursorPos.getX() - m_center->getX()) * 180.0 / M_PI;
    } else if (m_step == 2 && m_center) {
        double endAngle = std::atan2(m_cursorPos.getY() - m_center->getY(), m_cursorPos.getX() - m_center->getX()) * 180.0 / M_PI;
        m_spanAngle = endAngle - m_startAngle;
    }
}
void CreateArcTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
    if (m_step >= 1 && m_center) {
        painter.drawEllipse(QPointF(m_center->getX(), m_center->getY()), 2, 2);
        painter.drawLine(QPointF(m_center->getX(), m_center->getY()), QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    }
    if (m_step == 2 && m_center) {
        QRectF rect(m_center->getX() - m_radius, m_center->getY() - m_radius, m_radius*2, m_radius*2);
        painter.drawArc(rect, int(m_startAngle*16), int(m_spanAngle*16));
    }
}
std::unique_ptr<Object> CreateArcTool::takeObject() { return std::move(m_result); }
void CreateArcTool::reset() { m_finished = false; m_center.reset(); m_step = 0; }

// --- Ellipse ---
void CreateEllipseTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
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
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
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
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    if (!m_center.has_value()) { m_center = target; }
    else {
        m_radius = std::hypot(target.getX() - m_center->getX(), target.getY() - m_center->getY());
        m_result = std::make_unique<PolygonObj>(m_center.value(), m_radius, 5); // Default 5 sides
        m_finished = true;
    }
}
void CreatePolygonTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    if (m_center.has_value()) m_radius = std::hypot(target.getX() - m_center->getX(), target.getY() - m_center->getY());
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
    Point target; updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);
    m_points.push_back(target);
}
void CreateSplineTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    updateSnap(worldPos, snapper, scale, m_currentPos, m_snapPoint, m_isSnapped);
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
