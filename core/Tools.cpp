#include "Tools.h"
#include "Snapper.h"
#include "Segment.h"
#include "Circle.h"
#include <QPen>

// --- Base Tool Helper ---
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
    painter.setPen(QPen(Qt::green, 2.0 / scale)); // Косметически не скейлим тут, ручками
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(p.getX() - s/2, p.getY() - s/2, s, s));
}

// --- Create Segment Tool ---
void CreateSegmentTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);

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
    updateSnap(worldPos, snapper, scale, m_endPoint, m_snapPoint, m_isSnapped);
}

void CreateSegmentTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);

    if (m_startPoint.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawLine(QPointF(m_startPoint->getX(), m_startPoint->getY()),
                         QPointF(m_endPoint.getX(), m_endPoint.getY()));
    }
}

std::unique_ptr<Object> CreateSegmentTool::takeObject() { return std::move(m_result); }
void CreateSegmentTool::reset() { m_finished = false; m_startPoint.reset(); }


// --- Create Circle Tool ---
void CreateCircleTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);

    if (!m_center.has_value()) {
        m_center = target;
        m_radius = 0;
    } else {
        m_radius = std::sqrt(std::pow(target.getX() - m_center->getX(), 2) +
                             std::pow(target.getY() - m_center->getY(), 2));
        m_result = std::make_unique<Circle>(m_center.value(), m_radius);
        m_finished = true;
    }
}

void CreateCircleTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    updateSnap(worldPos, snapper, scale, target, m_snapPoint, m_isSnapped);

    if (m_center.has_value()) {
        m_radius = std::sqrt(std::pow(target.getX() - m_center->getX(), 2) +
                             std::pow(target.getY() - m_center->getY(), 2));
    }
}

void CreateCircleTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);

    if (m_center.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawEllipse(QPointF(m_center->getX(), m_center->getY()), m_radius, m_radius);
        painter.drawLine(QPointF(m_center->getX(), m_center->getY()),
                         QPointF(m_center->getX() + m_radius, m_center->getY()));
    }
}

std::unique_ptr<Object> CreateCircleTool::takeObject() { return std::move(m_result); }
void CreateCircleTool::reset() { m_finished = false; m_center.reset(); }
