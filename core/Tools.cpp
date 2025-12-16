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
#include <algorithm>
#include <QPen>
#include <QPainterPath>

// Хелпер для обновления привязки с учетом контекста (предыдущей точки)
void updateSnap(const Point& worldPos, const Snapper& snapper, double scale,
                const std::optional<Point>& prevPoint,
                Point& outPos, Point& outSnapPt, bool& outIsSnapped) {
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

// Вспомогательная функция для отрисовки маркера точки
void drawPointMarker(QPainter& painter, const Point& p, double scale, const QColor& color = Qt::cyan) {
    double s = 6.0 / scale;
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(p.getX(), p.getY()), s/2, s/2);
}

// --- Segment ---
void CreateSegmentTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
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
        // Вспомогательная линия от центра к курсору (радиус)
        painter.drawLine(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), 
                        QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    }
    else if (m_method == 1) { // Center + D
        double d_dist = MathUtils::dist(m_clicks[0], m_cursorPos);
        painter.drawEllipse(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), d_dist, d_dist);
        // Линия диаметра
        painter.drawLine(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), 
                        QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    }
    else if (m_method == 2) { // 2 Points (Diameter)
        Point center((m_clicks[0].getX() + m_cursorPos.getX())/2, (m_clicks[0].getY() + m_cursorPos.getY())/2);
        double r = MathUtils::dist(m_clicks[0], m_cursorPos) / 2.0;
        painter.drawEllipse(QPointF(center.getX(), center.getY()), r, r);
        // Линия диаметра между точками
        painter.drawLine(QPointF(m_clicks[0].getX(), m_clicks[0].getY()), 
                        QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    }
    else if (m_method == 3) {
        // Отрисовываем линии между точками
        for (size_t i = 0; i < m_clicks.size(); ++i) {
            Point next = (i + 1 < m_clicks.size()) ? m_clicks[i+1] : m_cursorPos;
            painter.drawLine(QPointF(m_clicks[i].getX(), m_clicks[i].getY()), 
                            QPointF(next.getX(), next.getY()));
        }
        
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
            double minX = std::min(m_start->getX(), m_end.getX());
            double maxY = std::max(m_start->getY(), m_end.getY());
            double w = std::abs(m_end.getX() - m_start->getX());
            double h = std::abs(m_end.getY() - m_start->getY());

            Point tl(minX, maxY);
            m_result = std::make_unique<Rectangle>(tl, w, h);
            m_finished = true;
        }
    }
    // 1: Corner + Width/Height (первая точка - угол, вторая - противоположный по диагонали)
    else if (m_method == 1) {
        if (!m_start.has_value()) {
            m_start = target;
            m_end = target;
        }
        else {
            m_end = target;
            double minX = std::min(m_start->getX(), m_end.getX());
            double maxY = std::max(m_start->getY(), m_end.getY());
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

        if (m_method == 0 || m_method == 1) {
            double minX = std::min(m_start->getX(), m_end.getX());
            double maxY = std::max(m_start->getY(), m_end.getY());
            double w = std::abs(m_end.getX() - m_start->getX());
            double h = std::abs(m_end.getY() - m_start->getY());
            painter.drawRect(QRectF(minX, maxY - h, w, h));
        } else if (m_method == 2) {
            double halfW = std::abs(m_end.getX() - m_start->getX());
            double halfH = std::abs(m_end.getY() - m_start->getY());
            painter.drawRect(QRectF(m_start->getX() - halfW, m_start->getY() - halfH, halfW*2, halfH*2));
            // Крестик в центре
            double cs = 5.0 / scale;
            painter.drawLine(QPointF(m_start->getX() - cs, m_start->getY()), 
                            QPointF(m_start->getX() + cs, m_start->getY()));
            painter.drawLine(QPointF(m_start->getX(), m_start->getY() - cs), 
                            QPointF(m_start->getX(), m_start->getY() + cs));
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

    // 0: Center, Start (radius + start angle), End angle
    if (m_method == 0) {
        if (m_clicks.size() == 3) {
            Point center = m_clicks[0];
            double r = MathUtils::dist(center, m_clicks[1]);
            double startAngle = std::atan2(m_clicks[1].getY() - center.getY(), m_clicks[1].getX() - center.getX()) * 180.0 / M_PI;
            double endAngle = std::atan2(m_clicks[2].getY() - center.getY(), m_clicks[2].getX() - center.getX()) * 180.0 / M_PI;
            
            // Вычисляем span (размах дуги) - разность между конечным и начальным углом
            double span = endAngle - startAngle;
            
            // Нормализуем span в диапазон [-180, 180] для корректного отображения
            // Если span > 180, идем в обратную сторону (отрицательный span)
            if (span > 180.0) {
                span = span - 360.0;
            } else if (span < -180.0) {
                span = span + 360.0;
            }
            
            m_result = std::make_unique<Arc>(center, r, startAngle, span);
            m_finished = true;
        }
    }
    // 1: 3 Points (Start, End, Radius) - первые две точки ограничивают дугу, третья задает радиус окружности
    else if (m_method == 1) {
        if (m_clicks.size() == 3) {
            // Первая и вторая точки - начало и конец дуги
            Point start = m_clicks[0];
            Point end = m_clicks[1];
            // Третья точка задает радиус окружности (расстояние от первой точки до третьей = радиус)
            Point radiusPoint = m_clicks[2];
            
            // Радиус = расстояние от первой точки до третьей точки
            double radius = MathUtils::dist(start, radiusPoint);
            
            // Находим центр окружности, которая проходит через первую и вторую точку с заданным радиусом
            Point center;
            if (MathUtils::getCircleCenterFrom2PointsAndRadius(start, end, radius, radiusPoint, center)) {
                // Вычисляем углы для начала и конца дуги
                double startAngle = std::atan2(start.getY() - center.getY(), start.getX() - center.getX()) * 180.0 / M_PI;
                double endAngle = std::atan2(end.getY() - center.getY(), end.getX() - center.getX()) * 180.0 / M_PI;
                
                // Нормализуем углы в диапазон [0, 360)
                auto norm = [](double a){ 
                    a = std::fmod(a, 360.0);
                    if (a < 0) a += 360.0;
                    return a;
                };
                double s = norm(startAngle);
                double e = norm(endAngle);
                
                // Вычисляем span (размах дуги)
                double span = e - s;
                if (span < 0) span += 360.0;
                
                // Проверяем направление: выбираем дугу, которая проходит ближе к третьей точке
                double radiusAngle = norm(std::atan2(radiusPoint.getY() - center.getY(), 
                                                     radiusPoint.getX() - center.getX()) * 180.0 / M_PI);
                
                // Если третья точка не попадает в диапазон дуги, используем обратную дугу
                double radiusDiff = radiusAngle - s;
                if (radiusDiff < 0) radiusDiff += 360.0;
                
                if (radiusDiff > span && radiusDiff < 360.0 - span) {
                    span = span - 360.0;
                }
                
                m_result = std::make_unique<Arc>(center, radius, startAngle, span);
                m_finished = true;
            } else {
                // Если решение невозможно (расстояние между точками > 2*radius), удаляем последний клик
                m_clicks.pop_back();
            }
        }
    }
}

void CreateArcTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    std::optional<Point> prev = m_clicks.empty() ? std::nullopt : std::make_optional(m_clicks.back());
    updateSnap(worldPos, snapper, scale, prev, m_cursorPos, m_snapPoint, m_isSnapped);
}

void CreateArcTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    QPen dashPen(Qt::white, 1.0, Qt::DashLine); dashPen.setCosmetic(true);
    QPen solidPen(QColor("#66D9EF"), 1.5, Qt::SolidLine); solidPen.setCosmetic(true);

    if (m_method == 0) { // Center, Start, End
        if (m_clicks.size() >= 1) {
            Point center = m_clicks[0];
            
            // Пунктирная линия от центра к курсору
            painter.setPen(dashPen);
            painter.drawLine(QPointF(center.getX(), center.getY()), 
                            QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
            
            if (m_clicks.size() >= 2) {
                double r = MathUtils::dist(center, m_clicks[1]);
                
                // Пунктирная окружность (базовая)
                QPen circPen(Qt::gray, 0.5, Qt::DotLine); circPen.setCosmetic(true);
                painter.setPen(circPen);
                painter.drawEllipse(QPointF(center.getX(), center.getY()), r, r);
                
                // Пунктирная линия от центра к начальной точке
                painter.setPen(dashPen);
                painter.drawLine(QPointF(center.getX(), center.getY()), 
                                QPointF(m_clicks[1].getX(), m_clicks[1].getY()));
                
                // Вычисляем и рисуем дугу
                double startAngle = std::atan2(m_clicks[1].getY() - center.getY(), 
                                               m_clicks[1].getX() - center.getX()) * 180.0 / M_PI;
                double endAngle = std::atan2(m_cursorPos.getY() - center.getY(), 
                                             m_cursorPos.getX() - center.getX()) * 180.0 / M_PI;
                double span = endAngle - startAngle;
                
                // Нормализуем span в диапазон [-180, 180]
                if (span > 180.0) {
                    span = span - 360.0;
                } else if (span < -180.0) {
                    span = span + 360.0;
                }
                
                // Рисуем саму дугу
                // Qt использует углы в 1/16 градуса, и начало отсчета - 3 часа (90 градусов)
                // Конвертируем: Qt angle = (90 - angle) * 16 для правильного отображения
                painter.setPen(solidPen);
                QRectF rect(center.getX() - r, center.getY() - r, r * 2, r * 2);
                int qtStartAngle = int((90.0 - startAngle) * 16);
                int qtSpanAngle = int(span * 16);
                painter.drawArc(rect, qtStartAngle, qtSpanAngle);
            }
        }
    }
    else if (m_method == 1) { // 3 Points
        painter.setPen(dashPen);
        
        // Линии между точками
        for (size_t i = 0; i < m_clicks.size(); ++i) {
            Point next = (i + 1 < m_clicks.size()) ? m_clicks[i+1] : m_cursorPos;
            painter.drawLine(QPointF(m_clicks[i].getX(), m_clicks[i].getY()), 
                            QPointF(next.getX(), next.getY()));
        }
        
        if (m_clicks.size() == 2) {
            // Первая и вторая точки - начало и конец дуги
            Point start = m_clicks[0];
            Point end = m_clicks[1];
            // Курсор задает радиус окружности
            Point radiusPoint = m_cursorPos;
            
            // Радиус = расстояние от первой точки до курсора
            double radius = MathUtils::dist(start, radiusPoint);
            
            // Находим центр окружности, которая проходит через первую и вторую точку с заданным радиусом
            Point center;
            if (MathUtils::getCircleCenterFrom2PointsAndRadius(start, end, radius, radiusPoint, center)) {
                // Пунктирная окружность
                QPen circPen(Qt::gray, 0.5, Qt::DotLine); circPen.setCosmetic(true);
                painter.setPen(circPen);
                painter.drawEllipse(QPointF(center.getX(), center.getY()), radius, radius);
                
                // Вычисляем углы для начала и конца дуги
                double startAngle = std::atan2(start.getY() - center.getY(), 
                                               start.getX() - center.getX()) * 180.0 / M_PI;
                double endAngle = std::atan2(end.getY() - center.getY(), 
                                             end.getX() - center.getX()) * 180.0 / M_PI;
                
                // Нормализуем углы
                auto norm = [](double a){ 
                    a = std::fmod(a, 360.0);
                    if (a < 0) a += 360.0;
                    return a;
                };
                double s = norm(startAngle);
                double e = norm(endAngle);
                
                // Вычисляем span
                double span = e - s;
                if (span < 0) span += 360.0;
                
                // Проверяем направление через третью точку (курсор)
                double radiusAngle = norm(std::atan2(radiusPoint.getY() - center.getY(), 
                                                      radiusPoint.getX() - center.getX()) * 180.0 / M_PI);
                double radiusDiff = radiusAngle - s;
                if (radiusDiff < 0) radiusDiff += 360.0;
                
                // Если третья точка не попадает в диапазон дуги, идем в обратную сторону
                if (radiusDiff > span && radiusDiff < 360.0 - span) {
                    span = span - 360.0;
                }
                
                // Рисуем дугу
                painter.setPen(solidPen);
                QRectF rect(center.getX() - radius, center.getY() - radius, radius * 2, radius * 2);
                int qtStartAngle = int((90.0 - startAngle) * 16);
                int qtSpanAngle = int(span * 16);
                painter.drawArc(rect, qtStartAngle, qtSpanAngle);
            }
        }
    }
}

std::unique_ptr<Object> CreateArcTool::takeObject() { return std::move(m_result); }
void CreateArcTool::reset() { m_finished = false; m_clicks.clear(); }

// --- Ellipse ---
// Методы: 0 - Центр+Радиусы (одним кликом задаём центр, вторым - угол эллипса)
//         1 - Центр+2 оси (центр, затем точка на оси X, затем точка на оси Y)
CreateEllipseTool::CreateEllipseTool(int method) : m_method(method) {}

void CreateEllipseTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    std::optional<Point> prev = m_clicks.empty() ? std::nullopt : std::make_optional(m_clicks.back());
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);

    m_clicks.push_back(target);

    // Метод 0: Центр + угловая точка (определяет оба радиуса)
    if (m_method == 0) {
        if (m_clicks.size() == 2) {
            Point center = m_clicks[0];
            m_rx = std::abs(m_clicks[1].getX() - center.getX());
            m_ry = std::abs(m_clicks[1].getY() - center.getY());
            if (m_rx < 1e-3) m_rx = 1.0;
            if (m_ry < 1e-3) m_ry = 1.0;
            m_result = std::make_unique<Ellipse>(center, m_rx, m_ry);
            m_finished = true;
        }
    }
    // Метод 1: Центр + 2 оси (3 клика: центр, точка на оси X, точка на оси Y)
    else if (m_method == 1) {
        if (m_clicks.size() == 3) {
            Point center = m_clicks[0];
            m_rx = MathUtils::dist(center, m_clicks[1]);
            m_ry = MathUtils::dist(center, m_clicks[2]);
            if (m_rx < 1e-3) m_rx = 1.0;
            if (m_ry < 1e-3) m_ry = 1.0;
            m_result = std::make_unique<Ellipse>(center, m_rx, m_ry);
            m_finished = true;
        }
    }
}

void CreateEllipseTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    std::optional<Point> prev = m_clicks.empty() ? std::nullopt : std::make_optional(m_clicks.back());
    updateSnap(worldPos, snapper, scale, prev, m_cursorPos, m_snapPoint, m_isSnapped);

    if (!m_clicks.empty()) {
        Point center = m_clicks[0];
        if (m_method == 0) {
            m_rx = std::abs(m_cursorPos.getX() - center.getX());
            m_ry = std::abs(m_cursorPos.getY() - center.getY());
        } else if (m_method == 1) {
            if (m_clicks.size() == 1) {
                m_rx = MathUtils::dist(center, m_cursorPos);
                m_ry = 0;
            } else if (m_clicks.size() == 2) {
                m_rx = MathUtils::dist(center, m_clicks[1]);
                m_ry = MathUtils::dist(center, m_cursorPos);
            }
        }
    }
}

void CreateEllipseTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);

    if (m_clicks.empty()) return;
    Point center = m_clicks[0];

    if (m_method == 0) {
        // Рисуем эллипс
        painter.drawEllipse(QPointF(center.getX(), center.getY()), m_rx, m_ry);
        // Оси пунктиром
        painter.drawLine(QPointF(center.getX() - m_rx, center.getY()), 
                        QPointF(center.getX() + m_rx, center.getY()));
        painter.drawLine(QPointF(center.getX(), center.getY() - m_ry), 
                        QPointF(center.getX(), center.getY() + m_ry));
    }
    else if (m_method == 1) {
        // Рисуем линии осей
        if (m_clicks.size() >= 1) {
            painter.drawLine(QPointF(center.getX(), center.getY()), 
                            QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
        }
        if (m_clicks.size() >= 2) {
            // Ось X зафиксирована
            painter.drawLine(QPointF(center.getX(), center.getY()), 
                            QPointF(m_clicks[1].getX(), m_clicks[1].getY()));
            // Ось Y к курсору
            painter.drawLine(QPointF(center.getX(), center.getY()), 
                            QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
        }
        // Рисуем предварительный эллипс
        double rx = m_rx > 0 ? m_rx : 1;
        double ry = m_ry > 0 ? m_ry : 1;
        painter.drawEllipse(QPointF(center.getX(), center.getY()), rx, ry);
    }
}

std::unique_ptr<Object> CreateEllipseTool::takeObject() { return std::move(m_result); }
void CreateEllipseTool::reset() { m_finished = false; m_clicks.clear(); m_rx = 0; m_ry = 0; }

// --- Polygon ---
// Методы: 0 - Вписанный (вершины на окружности), 1 - Описанный (стороны касаются окружности)
CreatePolygonTool::CreatePolygonTool(int method) : m_method(method) {}

void CreatePolygonTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    std::optional<Point> prev = m_center;
    updateSnap(worldPos, snapper, scale, prev, target, m_snapPoint, m_isSnapped);

    if (!m_center.has_value()) { 
        m_center = target; 
    }
    else {
        m_radius = MathUtils::dist(target, m_center.value());
        bool inscribed = (m_method == 0); // 0 = вписанный, 1 = описанный
        m_result = std::make_unique<PolygonObj>(m_center.value(), m_radius, 5, inscribed);
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
        
        int sides = 5;
        double step = 2 * M_PI / sides;
        double startAngle = M_PI / 2;
        
        // Радиус для отрисовки (зависит от метода)
        double drawRadius = m_radius;
        if (m_method == 1) { // Описанный - радиус до середины стороны
            drawRadius = m_radius / std::cos(M_PI / sides);
        }
        
        // Окружность
        painter.drawEllipse(QPointF(m_center->getX(), m_center->getY()), m_radius, m_radius);
        
        // Сам многоугольник
        QPolygonF poly;
        for (int i = 0; i < sides; ++i) {
            double angle = startAngle + i * step;
            poly << QPointF(m_center->getX() + drawRadius * std::cos(angle),
                           m_center->getY() + drawRadius * std::sin(angle));
        }
        painter.drawPolygon(poly);
        
        // Линия от центра к курсору
        painter.drawLine(QPointF(m_center->getX(), m_center->getY()), 
                        QPointF(m_center->getX() + m_radius, m_center->getY()));
    }
}

std::unique_ptr<Object> CreatePolygonTool::takeObject() { return std::move(m_result); }
void CreatePolygonTool::reset() { m_finished = false; m_center.reset(); m_radius = 0; }

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
    
    if (m_points.size() >= 2) {
        // Строим сплайн через все точки + текущую позицию курсора
        std::vector<Point> allPoints = m_points;
        allPoints.push_back(m_currentPos);
        
        QPainterPath path;
        path.moveTo(allPoints[0].getX(), allPoints[0].getY());
        
        // Smooth spline using cubic beziers (Catmull-Rom spline conversion)
        for (size_t i = 0; i < allPoints.size() - 1; ++i) {
            Point p0 = (i == 0) ? allPoints[0] : allPoints[i-1];
            Point p1 = allPoints[i];
            Point p2 = allPoints[i+1];
            Point p3 = (i + 2 < allPoints.size()) ? allPoints[i+2] : p2;
            
            double cp1x = p1.getX() + (p2.getX() - p0.getX()) / 6.0;
            double cp1y = p1.getY() + (p2.getY() - p0.getY()) / 6.0;
            
            double cp2x = p2.getX() - (p3.getX() - p1.getX()) / 6.0;
            double cp2y = p2.getY() - (p3.getY() - p1.getY()) / 6.0;
            
            path.cubicTo(cp1x, cp1y, cp2x, cp2y, p2.getX(), p2.getY());
        }
        
        painter.drawPath(path);
    } else if (m_points.size() == 1) {
        // Если только одна точка, рисуем линию к курсору
        painter.drawLine(QPointF(m_points[0].getX(), m_points[0].getY()), 
                        QPointF(m_currentPos.getX(), m_currentPos.getY()));
    }
}

std::unique_ptr<Object> CreateSplineTool::takeObject() { return std::move(m_result); }
void CreateSplineTool::reset() { m_finished = false; m_points.clear(); }
