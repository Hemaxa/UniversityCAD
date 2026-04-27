#include "Tools.h"
#include "Snapper.h"
#include "Segment.h"
#include "Circle.h"
#include "Rectangle.h"
#include "Arc.h"
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include "PointObject.h"
#include "Dimension.h"
#include "GlobalSettings.h"
#include "MathUtils.h"
#include "Properties.h"
#include <cmath>
#include <algorithm>
#include <QPen>
#include <QPainterPath>
#include <QFontMetricsF>

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
    m_tangentExtensionLine.reset();  // Сбрасываем при клике
}

void CreateSegmentTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    auto res = snapper.snap(worldPos, scale, m_startPoint);
    if (res.snapped) {
        m_endPoint = res.point;
        m_snapPoint = res.point;
        m_isSnapped = true;
        m_tangentExtensionLine = res.tangentExtensionLine;  // Сохраняем линию расширения
    } else {
        m_endPoint = worldPos;
        m_isSnapped = false;
        m_tangentExtensionLine.reset();
    }
}

void CreateSegmentTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    if (m_startPoint.has_value()) {
        QPen pen(Qt::white, 1.0, Qt::DashLine); pen.setCosmetic(true); painter.setPen(pen);
        painter.drawLine(QPointF(m_startPoint->getX(), m_startPoint->getY()), QPointF(m_endPoint.getX(), m_endPoint.getY()));
        
        // Рисуем пунктирное продолжение касательной
        if (m_tangentExtensionLine.has_value()) {
            auto [tangentPt, extEnd] = m_tangentExtensionLine.value();
            QPen extPen(Qt::cyan, 1.0, Qt::DotLine); 
            extPen.setCosmetic(true); 
            painter.setPen(extPen);
            // Линия от точки касания до конца расширения
            painter.drawLine(QPointF(tangentPt.getX(), tangentPt.getY()), 
                             QPointF(extEnd.getX(), extEnd.getY()));
            
            // Маркер точки касания
            double markerSize = 6.0 / scale;
            painter.setPen(QPen(Qt::magenta, 2.0 / scale));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(tangentPt.getX(), tangentPt.getY()), markerSize/2, markerSize/2);
        }
    }
}

std::unique_ptr<Object> CreateSegmentTool::takeObject() { return std::move(m_result); }
void CreateSegmentTool::reset() { m_finished = false; m_startPoint.reset(); m_tangentExtensionLine.reset(); }

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
    // 1: 3 Points - все три точки лежат на дуге (начало, конец, точка на дуге)
    else if (m_method == 1) {
        if (m_clicks.size() == 3) {
            // Все три точки лежат на дуге
            Point p1 = m_clicks[0]; // Начало дуги
            Point p2 = m_clicks[1]; // Конец дуги
            Point p3 = m_clicks[2]; // Точка на дуге (определяет окружность)
            
            // Находим окружность через три точки
            Point center;
            double radius;
            if (MathUtils::getCircleFrom3Points(p1, p2, p3, center, radius)) {
                // Вычисляем углы для всех трёх точек
                double angle1 = std::atan2(p1.getY() - center.getY(), p1.getX() - center.getX()) * 180.0 / M_PI;
                double angle2 = std::atan2(p2.getY() - center.getY(), p2.getX() - center.getX()) * 180.0 / M_PI;
                double angle3 = std::atan2(p3.getY() - center.getY(), p3.getX() - center.getX()) * 180.0 / M_PI;
                
                // Нормализуем углы в диапазон [0, 360)
                auto norm = [](double a) { 
                    a = std::fmod(a, 360.0);
                    if (a < 0) a += 360.0;
                    return a;
                };
                double a1 = norm(angle1);
                double a2 = norm(angle2);
                double a3 = norm(angle3);
                
                // Определяем направление дуги: от p1 к p2 через p3
                // Проверяем, лежит ли p3 между p1 и p2 при движении против часовой стрелки
                double spanCCW = a2 - a1;
                if (spanCCW < 0) spanCCW += 360.0;
                
                double diff3 = a3 - a1;
                if (diff3 < 0) diff3 += 360.0;
                
                double span;
                if (diff3 < spanCCW) {
                    // p3 лежит на дуге против часовой стрелки от p1 к p2
                    span = spanCCW;
                } else {
                    // p3 лежит на дуге по часовой стрелке от p1 к p2
                    span = spanCCW - 360.0;
                }
                
                m_result = std::make_unique<Arc>(center, radius, angle1, span);
                m_finished = true;
            } else {
                // Три точки коллинеарны, невозможно построить окружность
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
                // Qt использует углы в 1/16 градуса
                // Поскольку Y-ось инвертирована в мировых координатах (вверх = положительный Y),
                // а Qt рисует с инвертированной Y-осью, нужно инвертировать углы
                painter.setPen(solidPen);
                QRectF rect(center.getX() - r, center.getY() - r, r * 2, r * 2);
                int qtStartAngle = int(-startAngle * 16);
                int qtSpanAngle = int(-span * 16);
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
            // Все три точки лежат на дуге: начало, конец, точка на дуге (курсор)
            Point p1 = m_clicks[0]; // Начало дуги
            Point p2 = m_clicks[1]; // Конец дуги
            Point p3 = m_cursorPos; // Точка на дуге
            
            // Находим окружность через три точки
            Point center;
            double radius;
            if (MathUtils::getCircleFrom3Points(p1, p2, p3, center, radius)) {
                // Пунктирная окружность
                QPen circPen(Qt::gray, 0.5, Qt::DotLine); circPen.setCosmetic(true);
                painter.setPen(circPen);
                painter.drawEllipse(QPointF(center.getX(), center.getY()), radius, radius);
                
                // Вычисляем углы для всех трёх точек
                double angle1 = std::atan2(p1.getY() - center.getY(), p1.getX() - center.getX()) * 180.0 / M_PI;
                double angle2 = std::atan2(p2.getY() - center.getY(), p2.getX() - center.getX()) * 180.0 / M_PI;
                double angle3 = std::atan2(p3.getY() - center.getY(), p3.getX() - center.getX()) * 180.0 / M_PI;
                
                // Нормализуем углы в диапазон [0, 360)
                auto norm = [](double a) { 
                    a = std::fmod(a, 360.0);
                    if (a < 0) a += 360.0;
                    return a;
                };
                double a1 = norm(angle1);
                double a2 = norm(angle2);
                double a3 = norm(angle3);
                
                // Определяем направление дуги: от p1 к p2 через p3
                double spanCCW = a2 - a1;
                if (spanCCW < 0) spanCCW += 360.0;
                
                double diff3 = a3 - a1;
                if (diff3 < 0) diff3 += 360.0;
                
                double span;
                if (diff3 < spanCCW) {
                    span = spanCCW;
                } else {
                    span = spanCCW - 360.0;
                }
                
                // Рисуем дугу
                painter.setPen(solidPen);
                QRectF rect(center.getX() - radius, center.getY() - radius, radius * 2, radius * 2);
                int qtStartAngle = int(-angle1 * 16);
                int qtSpanAngle = int(-span * 16);
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

// --- Point ---
void CreatePointTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {
    Point target;
    updateSnap(worldPos, snapper, scale, std::nullopt, target, m_snapPoint, m_isSnapped);
    m_result = std::make_unique<PointObject>(target);
    m_finished = true;
}

void CreatePointTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {
    updateSnap(worldPos, snapper, scale, std::nullopt, m_cursorPos, m_snapPoint, m_isSnapped);
}

void CreatePointTool::draw(QPainter& painter, double scale) {
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    // Рисуем крестик-превью в позиции курсора
    double s = 4.0 / scale;
    QPen pen(Qt::white, 1.5 / scale);
    painter.setPen(pen);
    painter.drawLine(QPointF(m_cursorPos.getX() - s, m_cursorPos.getY() - s),
                     QPointF(m_cursorPos.getX() + s, m_cursorPos.getY() + s));
    painter.drawLine(QPointF(m_cursorPos.getX() + s, m_cursorPos.getY() - s),
                     QPointF(m_cursorPos.getX() - s, m_cursorPos.getY() + s));
}

std::unique_ptr<Object> CreatePointTool::takeObject() { return std::move(m_result); }
void CreatePointTool::reset() { m_finished = false; }

// --- Dimension ---
CreateDimensionTool::CreateDimensionTool(DimensionType type) : m_type(type) {}

DimensionAnchor CreateDimensionTool::makeAnchor(const Point& raw, const Snapper& snapper, double scale) const
{
    auto res = snapper.snap(raw, scale);
    DimensionAnchor anchor;
    anchor.fallback = res.snapped ? res.point : raw;
    if (res.object && res.object->getType() != PrimitiveType::Dimension) {
        anchor.object = res.object;
        anchor.snapIndex = res.snapIndex;
    }
    return anchor;
}

bool CreateDimensionTool::tryPickRadialAnchors(const Point& worldPos, const Snapper& snapper, double scale)
{
    auto picked = makeAnchor(worldPos, snapper, scale);
    if (!picked.object) {
        return false;
    }

    DimensionAnchor center = picked;
    DimensionAnchor edge = picked;
    if (picked.object->getType() == PrimitiveType::Circle) {
        auto* c = static_cast<const Circle*>(picked.object);
        center.fallback = c->getCenter();
        center.snapIndex = 0;
    } else if (picked.object->getType() == PrimitiveType::Arc) {
        auto* a = static_cast<const Arc*>(picked.object);
        center.fallback = a->getCenter();
        center.snapIndex = 0;
    } else if (picked.object->getType() == PrimitiveType::Ellipse) {
        auto* e = static_cast<const Ellipse*>(picked.object);
        center.fallback = e->getCenter();
        center.snapIndex = 0;
    } else {
        return false;
    }

    edge.fallback = picked.fallback;
    edge.snapIndex = -1;
    m_anchors = {center, edge};
    return true;
}

void CreateDimensionTool::finishDimensionWithText(const Point& textPos)
{
    if (m_anchors.size() < 2 || !m_pendingLinePoint.has_value()) {
        return;
    }

    auto dim = std::make_unique<Dimension>(m_type, m_anchors[0], m_anchors[1], *m_pendingLinePoint);
    if (m_type == DimensionType::Angular) {
        dim->setAngularRadius(m_pendingAngularRadius.value_or(5.0));
        dim->setUseSupplementaryAngle(m_pendingSupplementaryAngle);
    }
    dim->setTextPosition(textPos);
    m_result = std::move(dim);
    m_finished = true;
}

std::unique_ptr<Dimension> CreateDimensionTool::buildPreviewDimension(const Point& textPos) const
{
    if (m_anchors.size() < 2 || !m_pendingLinePoint.has_value()) {
        return nullptr;
    }

    auto dim = std::make_unique<Dimension>(m_type, m_anchors[0], m_anchors[1], *m_pendingLinePoint);
    if (m_type == DimensionType::Angular) {
        dim->setAngularRadius(m_pendingAngularRadius.value_or(5.0));
        dim->setUseSupplementaryAngle(m_pendingSupplementaryAngle);
    }
    dim->setTextPosition(textPos);
    return dim;
}

void CreateDimensionTool::toggleAngularSide()
{
    if (m_type != DimensionType::Angular) {
        return;
    }
    m_pendingSupplementaryAngle = !m_pendingSupplementaryAngle;
}

static std::optional<DimensionEdgeRef> nearestDimensionEdge(const Object* obj, const Point& p)
{
    if (!obj || obj->getType() == PrimitiveType::Dimension) return std::nullopt;

    std::vector<DimensionEdgeRef> edges;
    if (obj->getType() == PrimitiveType::Segment) {
        auto* s = static_cast<const Segment*>(obj);
        edges.push_back({s->getStart(), s->getEnd(), obj, 0, 1});
    } else if (obj->getType() == PrimitiveType::Rectangle) {
        auto* r = static_cast<const Rectangle*>(obj);
        double x = r->getTopLeft().getX();
        double y = r->getTopLeft().getY();
        double w = r->getWidth();
        double h = r->getHeight();
        Point tl(x, y), tr(x + w, y), br(x + w, y - h), bl(x, y - h);
        edges = {{tl, tr}, {tr, br}, {br, bl}, {bl, tl}};
    } else if (obj->getType() == PrimitiveType::Polygon) {
        auto* poly = static_cast<const PolygonObj*>(obj);
        auto vertices = poly->getVertices();
        for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
            edges.push_back({vertices[i], vertices[(i + 1) % vertices.size()]});
        }
    }

    if (edges.empty()) return std::nullopt;
    double best = std::numeric_limits<double>::max();
    DimensionEdgeRef bestEdge = edges.front();
    for (const auto& edge : edges) {
        Point proj = MathUtils::projectPointOnSegment(p, edge.first, edge.second);
        double d = MathUtils::distSq(p, proj);
        if (d < best) {
            best = d;
            bestEdge = edge;
        }
    }
    return bestEdge;
}

static bool intersectInfiniteLines(const Point& a1, const Point& a2, const Point& b1, const Point& b2, Point& out)
{
    const double x1 = a1.getX(), y1 = a1.getY();
    const double x2 = a2.getX(), y2 = a2.getY();
    const double x3 = b1.getX(), y3 = b1.getY();
    const double x4 = b2.getX(), y4 = b2.getY();
    const double den = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::abs(den) < 1e-9) return false;
    const double px = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / den;
    const double py = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / den;
    out = Point(px, py);
    return true;
}

static Point fartherFrom(const Point& origin, const DimensionEdgeRef& edge)
{
    return MathUtils::distSq(origin, edge.first) > MathUtils::distSq(origin, edge.second) ? edge.first : edge.second;
}

static DimensionAnchor fartherAnchorFrom(const Point& origin, const DimensionEdgeRef& edge)
{
    DimensionAnchor anchor;
    const bool chooseFirst = MathUtils::distSq(origin, edge.first) > MathUtils::distSq(origin, edge.second);
    anchor.object = edge.object;
    anchor.snapIndex = chooseFirst ? edge.firstSnapIndex : edge.secondSnapIndex;
    anchor.fallback = chooseFirst ? edge.first : edge.second;
    return anchor;
}

static void drawPreviewDimensionText(QPainter& painter, const Dimension& dim, double scale)
{
    const QPointF textPos(dim.getTextPosition().getX(), dim.getTextPosition().getY());
    const QTransform worldTransform = painter.transform();
    painter.save();
    painter.resetTransform();
    const QPointF screenPos = worldTransform.map(textPos);
    QFont font(dim.fontFamily(), std::max(6, static_cast<int>(dim.textHeight())));
    QFontMetricsF metrics(font);
    painter.setFont(font);
    painter.setPen(QPen(QColor("#A6E22E")));

    if (dim.getDimensionType() == DimensionType::Angular) {
        const QString text = dim.displayText();
        const QPointF centerScreen = worldTransform.map(QPointF(dim.getLinePoint().getX(), dim.getLinePoint().getY()));
        const double radiusScreen = std::hypot(screenPos.x() - centerScreen.x(), screenPos.y() - centerScreen.y());
        if (radiusScreen > 1.0) {
            Point a = dim.firstAnchor().resolve();
            Point b = dim.secondAnchor().resolve();
            const double a1 = std::atan2(a.getY() - dim.getLinePoint().getY(), a.getX() - dim.getLinePoint().getX());
            const double a2 = std::atan2(b.getY() - dim.getLinePoint().getY(), b.getX() - dim.getLinePoint().getX());
            double delta = std::fmod(a2 - a1, 2.0 * M_PI);
            if (delta > M_PI) delta -= 2.0 * M_PI;
            if (delta < -M_PI) delta += 2.0 * M_PI;
            if (dim.useSupplementaryAngle() && std::abs(delta) > MathUtils::EPSILON) {
                delta = delta > 0.0 ? delta - 2.0 * M_PI : delta + 2.0 * M_PI;
            }
            const double tangentSign = delta >= 0.0 ? 1.0 : -1.0;
            const double middleAngleDeg = qRadiansToDegrees(a1 + delta * dim.textPositionFactor());
            const double totalAdvance = metrics.horizontalAdvance(text);
            const double anglePerPixel = 180.0 / (M_PI * radiusScreen);
            double angleCursor = middleAngleDeg - tangentSign * totalAdvance * anglePerPixel * 0.5;
            for (const QChar ch : text) {
                const QString glyph(ch);
                const double advance = metrics.horizontalAdvance(glyph);
                const double glyphAngle = angleCursor + tangentSign * advance * anglePerPixel * 0.5;
                const double glyphRad = qDegreesToRadians(glyphAngle);
                const QPointF pos(centerScreen.x() + std::cos(glyphRad) * radiusScreen,
                                  centerScreen.y() - std::sin(glyphRad) * radiusScreen);
                painter.save();
                painter.translate(pos);
                painter.rotate(-glyphAngle + (tangentSign >= 0.0 ? -90.0 : 90.0));
                painter.drawText(QRectF(-advance * 0.5, -metrics.height() * 0.5, advance, metrics.height()), Qt::AlignCenter, glyph);
                painter.restore();
                angleCursor += tangentSign * advance * anglePerPixel;
            }
            painter.restore();
            return;
        }
    }

    auto screenAngle = [&](const QPointF& worldVector) {
        QPointF s0 = worldTransform.map(QPointF(0, 0));
        QPointF s1 = worldTransform.map(worldVector);
        QPointF sv = s1 - s0;
        return qRadiansToDegrees(std::atan2(sv.y(), sv.x()));
    };

    double textAngleDeg = 0.0;
    if (dim.getDimensionType() == DimensionType::Vertical) {
        textAngleDeg = -90.0;
    } else if (dim.getDimensionType() == DimensionType::Horizontal) {
        textAngleDeg = 0.0;
    } else if (dim.getDimensionType() == DimensionType::Linear
               || dim.getDimensionType() == DimensionType::Radius
               || dim.getDimensionType() == DimensionType::Diameter) {
        Point a = dim.firstAnchor().resolve();
        Point b = dim.secondAnchor().resolve();
        QPointF dir;
        if (dim.getDimensionType() == DimensionType::Linear) {
            dir = QPointF(b.getX() - a.getX(), b.getY() - a.getY());
        } else {
            dir = QPointF(dim.getLinePoint().getX() - a.getX(), dim.getLinePoint().getY() - a.getY());
        }
        textAngleDeg = screenAngle(dir);
    }

    while (textAngleDeg > 180.0) textAngleDeg -= 360.0;
    while (textAngleDeg < -180.0) textAngleDeg += 360.0;
    if (textAngleDeg > 90.0) textAngleDeg -= 180.0;
    if (textAngleDeg < -90.0) textAngleDeg += 180.0;

    painter.translate(screenPos);
    painter.rotate(textAngleDeg);
    painter.drawText(QRectF(-80, -16, 160, 32), Qt::AlignCenter, dim.displayText());
    painter.restore();
}

void CreateDimensionTool::onMousePress(const Point& worldPos, const Snapper& snapper, double scale)
{
    if (m_type == DimensionType::Angular) {
        if (m_pendingLinePoint.has_value() && m_pendingAngularRadius.has_value()) {
            finishDimensionWithText(makeAnchor(worldPos, snapper, scale).fallback);
            return;
        }

        auto picked = makeAnchor(worldPos, snapper, scale);
        if (m_anchors.empty() && m_dimensionEdges.empty() && picked.object && picked.object->getType() == PrimitiveType::Arc) {
            auto* arc = static_cast<const Arc*>(picked.object);
            const double start = arc->getStartAngle() * M_PI / 180.0;
            const double end = (arc->getStartAngle() + arc->getSpanAngle()) * M_PI / 180.0;
            DimensionAnchor a;
            a.object = picked.object;
            a.snapIndex = 1;
            a.fallback = Point(arc->getCenter().getX() + arc->getRadius() * std::cos(start),
                               arc->getCenter().getY() + arc->getRadius() * std::sin(start));
            DimensionAnchor b;
            b.object = picked.object;
            b.snapIndex = 2;
            b.fallback = Point(arc->getCenter().getX() + arc->getRadius() * std::cos(end),
                               arc->getCenter().getY() + arc->getRadius() * std::sin(end));
            m_anchors = {a, b};
            m_pendingLinePoint = arc->getCenter();
            return;
        }

        if (m_hoverEdge && m_dimensionEdges.size() < 2 && !m_pendingLinePoint.has_value()) {
            m_dimensionEdges.push_back(*m_hoverEdge);
            return;
        }

        if (m_dimensionEdges.size() == 2 && !m_pendingLinePoint.has_value()) {
            Point vertex;
            if (!intersectInfiniteLines(m_dimensionEdges[0].first, m_dimensionEdges[0].second,
                                        m_dimensionEdges[1].first, m_dimensionEdges[1].second,
                                        vertex)) {
                reset();
                return;
            }
            DimensionAnchor a = fartherAnchorFrom(vertex, m_dimensionEdges[0]);
            DimensionAnchor b = fartherAnchorFrom(vertex, m_dimensionEdges[1]);
            m_anchors = {a, b};
            m_pendingLinePoint = vertex;
            return;
        }

        if (m_anchors.size() == 2 && m_pendingLinePoint.has_value() && !m_pendingAngularRadius.has_value()) {
            m_pendingAngularRadius = std::max(5.0, MathUtils::dist(*m_pendingLinePoint, makeAnchor(worldPos, snapper, scale).fallback));
            return;
        }

        if (m_anchors.size() < 2) {
            m_anchors.push_back(picked);
            return;
        }

        if (m_anchors.size() == 2) {
            auto third = makeAnchor(worldPos, snapper, scale);
            DimensionAnchor firstRay = m_anchors[1];
            DimensionAnchor secondRay = third;
            m_anchors = {firstRay, secondRay};
            m_pendingLinePoint = m_anchors[0].resolve();
        }
        return;
    }

    if (m_type == DimensionType::Radius || m_type == DimensionType::Diameter) {
        if (m_pendingLinePoint.has_value()) {
            finishDimensionWithText(makeAnchor(worldPos, snapper, scale).fallback);
            return;
        }

        if (m_anchors.empty()) {
            tryPickRadialAnchors(worldPos, snapper, scale);
            return;
        }

        Point center = m_anchors[0].resolve();
        Point edge = m_anchors[1].resolve();
        Point dir(worldPos.getX() - center.getX(), worldPos.getY() - center.getY());
        double len = std::max(MathUtils::dist(center, worldPos), MathUtils::EPSILON);
        double baseLen = MathUtils::dist(center, edge);
        const double leaderLen = std::max(baseLen + GlobalSettings::instance().dimensionStyle.dimensionExtension, baseLen * 1.25);
        m_pendingLinePoint = Point(center.getX() + dir.getX() / len * leaderLen,
                                   center.getY() + dir.getY() / len * leaderLen);
        return;
    }

    if (m_pendingLinePoint.has_value()) {
        finishDimensionWithText(makeAnchor(worldPos, snapper, scale).fallback);
        return;
    }

    if (m_anchors.size() < 2) {
        m_anchors.push_back(makeAnchor(worldPos, snapper, scale));
        return;
    }

    m_pendingLinePoint = makeAnchor(worldPos, snapper, scale).fallback;
}

void CreateDimensionTool::onMouseMove(const Point& worldPos, const Snapper& snapper, double scale)
{
    auto res = snapper.snap(worldPos, scale);
    m_cursorPos = res.snapped ? res.point : worldPos;
    m_snapPoint = m_cursorPos;
    m_isSnapped = res.snapped;
    m_hoverEdge.reset();
    m_hoverCurveObject = nullptr;
    if ((m_type == DimensionType::Horizontal || m_type == DimensionType::Vertical || m_type == DimensionType::Linear || m_type == DimensionType::Angular)
        && res.object && res.object->getType() != PrimitiveType::Dimension) {
        m_hoverEdge = nearestDimensionEdge(res.object, m_cursorPos);
    }
    if ((m_type == DimensionType::Radius || m_type == DimensionType::Diameter || m_type == DimensionType::Angular)
        && res.object && (res.object->getType() == PrimitiveType::Circle || res.object->getType() == PrimitiveType::Arc || res.object->getType() == PrimitiveType::Ellipse)) {
        m_hoverCurveObject = res.object;
    }
}

void CreateDimensionTool::draw(QPainter& painter, double scale)
{
    if (m_isSnapped) drawSnapMarker(painter, m_snapPoint, scale);
    QPen pen(QColor("#A6E22E"), 1.0 / scale, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    if (m_hoverEdge) {
        QPen edgePen(QColor("#66D9EF"), 2.0 / scale);
        painter.setPen(edgePen);
        painter.drawLine(QPointF(m_hoverEdge->first.getX(), m_hoverEdge->first.getY()),
                         QPointF(m_hoverEdge->second.getX(), m_hoverEdge->second.getY()));
        painter.setPen(pen);
    }
    if (m_hoverCurveObject) {
        QPen curvePen(QColor("#66D9EF"), 2.0 / scale);
        painter.setPen(curvePen);
        if (m_hoverCurveObject->getType() == PrimitiveType::Circle) {
            auto* c = static_cast<const Circle*>(m_hoverCurveObject);
            painter.drawEllipse(QPointF(c->getCenter().getX(), c->getCenter().getY()), c->getRadius(), c->getRadius());
        } else if (m_hoverCurveObject->getType() == PrimitiveType::Ellipse) {
            auto* e = static_cast<const Ellipse*>(m_hoverCurveObject);
            painter.drawEllipse(QPointF(e->getCenter().getX(), e->getCenter().getY()), e->getRadiusX(), e->getRadiusY());
        } else if (m_hoverCurveObject->getType() == PrimitiveType::Arc) {
            auto* a = static_cast<const Arc*>(m_hoverCurveObject);
            QRectF rect(a->getCenter().getX() - a->getRadius(), a->getCenter().getY() - a->getRadius(), a->getRadius() * 2.0, a->getRadius() * 2.0);
            painter.drawArc(rect, int(-a->getStartAngle() * 16.0), int(-a->getSpanAngle() * 16.0));
        }
        painter.setPen(pen);
    }

    auto drawPreviewDimension = [&](const Point& pa, const Point& pb, const Point& linePoint, double textFactor, double textHalfWorld) {
        QPointF a(pa.getX(), pa.getY());
        QPointF b(pb.getX(), pb.getY());
        QPointF lp(linePoint.getX(), linePoint.getY());
        QPointF da = a;
        QPointF db = b;
        if (m_type == DimensionType::Horizontal) {
            da = QPointF(a.x(), lp.y());
            db = QPointF(b.x(), lp.y());
        } else if (m_type == DimensionType::Vertical) {
            da = QPointF(lp.x(), a.y());
            db = QPointF(lp.x(), b.y());
        }
        else {
            double vx = b.x() - a.x();
            double vy = b.y() - a.y();
            double len = std::hypot(vx, vy);
            if (len > 1e-9) {
                double nx = -vy / len;
                double ny = vx / len;
                double off = (lp.x() - a.x()) * nx + (lp.y() - a.y()) * ny;
                da = QPointF(a.x() + nx * off, a.y() + ny * off);
                db = QPointF(b.x() + nx * off, b.y() + ny * off);
            }
        }
        painter.drawLine(a, da);
        painter.drawLine(b, db);
        QPointF lineVec = db - da;
        const double len = std::hypot(lineVec.x(), lineVec.y());
        QPointF dimStart = da;
        QPointF dimEnd = db;
        if (len > 1e-9) {
            const double padFactor = textHalfWorld / len;
            double startFactor = std::min(0.0, textFactor);
            double endFactor = std::max(1.0, textFactor);
            if (textFactor < 0.0) startFactor -= padFactor;
            if (textFactor > 1.0) endFactor += padFactor;
            dimStart = da + lineVec * startFactor;
            dimEnd = da + lineVec * endFactor;
        }
        painter.drawLine(dimStart, dimEnd);
    };

    const QFont previewFont(GlobalSettings::instance().dimensionStyle.fontFamily,
                            std::max(6, static_cast<int>(GlobalSettings::instance().dimensionStyle.textHeight)));
    const QFontMetricsF previewMetrics(previewFont);

    if (m_type == DimensionType::Angular && m_pendingLinePoint.has_value() && m_pendingAngularRadius.has_value() && m_anchors.size() >= 2) {
        Point vertex = *m_pendingLinePoint;
        Point a = m_anchors[0].resolve();
        Point b = m_anchors[1].resolve();
        auto previewDim = buildPreviewDimension(m_cursorPos);
        painter.drawLine(QPointF(vertex.getX(), vertex.getY()), QPointF(a.getX(), a.getY()));
        painter.drawLine(QPointF(vertex.getX(), vertex.getY()), QPointF(b.getX(), b.getY()));
        double a1 = std::atan2(a.getY() - vertex.getY(), a.getX() - vertex.getX());
        double a2 = std::atan2(b.getY() - vertex.getY(), b.getX() - vertex.getX());
        double delta = std::fmod(a2 - a1, 2.0 * M_PI);
        if (delta > M_PI) delta -= 2.0 * M_PI;
        if (delta < -M_PI) delta += 2.0 * M_PI;
        if (m_pendingSupplementaryAngle && std::abs(delta) > MathUtils::EPSILON) {
            delta = delta > 0.0 ? delta - 2.0 * M_PI : delta + 2.0 * M_PI;
        }
        double start = a1;
        double span = delta;
        if (previewDim) {
            const double textWidthScreen = previewMetrics.horizontalAdvance(previewDim->displayText()) + 8.0;
            const double textPosRadius = std::hypot(previewDim->getTextPosition().getX() - vertex.getX(),
                                                    previewDim->getTextPosition().getY() - vertex.getY());
            const double radiusScreen = textPosRadius * scale;
            if (radiusScreen > 1e-9) {
                const double padAngle = textWidthScreen / radiusScreen;
                if (previewDim->textPositionFactor() < 0.0) {
                    start += delta * previewDim->textPositionFactor() - (delta >= 0.0 ? padAngle : -padAngle);
                    span = (a1 + delta) - start;
                } else if (previewDim->textPositionFactor() > 1.0) {
                    span = delta * previewDim->textPositionFactor() + (delta >= 0.0 ? padAngle : -padAngle);
                }
            }
        }
        double r = *m_pendingAngularRadius;
        QRectF arcRect(vertex.getX() - r, vertex.getY() - r, r * 2.0, r * 2.0);
        painter.drawArc(arcRect, int(-qRadiansToDegrees(start) * 16.0), int(-qRadiansToDegrees(span) * 16.0));
        if (previewDim) drawPreviewDimensionText(painter, *previewDim, scale);
    } else if (m_type == DimensionType::Angular && m_pendingLinePoint.has_value() && m_anchors.size() >= 2) {
        Point vertex = *m_pendingLinePoint;
        Point a = m_anchors[0].resolve();
        Point b = m_anchors[1].resolve();
        painter.drawLine(QPointF(vertex.getX(), vertex.getY()), QPointF(a.getX(), a.getY()));
        painter.drawLine(QPointF(vertex.getX(), vertex.getY()), QPointF(b.getX(), b.getY()));
        double r = std::max(5.0, MathUtils::dist(vertex, m_cursorPos));
        QRectF arcRect(vertex.getX() - r, vertex.getY() - r, r * 2.0, r * 2.0);
        double a1 = std::atan2(a.getY() - vertex.getY(), a.getX() - vertex.getX()) * 180.0 / M_PI;
        double a2 = std::atan2(b.getY() - vertex.getY(), b.getX() - vertex.getX()) * 180.0 / M_PI;
        double span = a2 - a1;
        if (span > 180.0) span -= 360.0;
        if (span < -180.0) span += 360.0;
        if (m_pendingSupplementaryAngle && std::abs(span) > 1e-9) {
            span = span > 0.0 ? span - 360.0 : span + 360.0;
        }
        painter.drawArc(arcRect, int(-a1 * 16.0), int(-span * 16.0));
    } else if (m_type == DimensionType::Angular && m_anchors.size() == 1) {
        Point v = m_anchors[0].resolve();
        painter.drawLine(QPointF(v.getX(), v.getY()), QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    } else if (m_type == DimensionType::Angular && m_anchors.size() >= 2 && !m_pendingLinePoint.has_value()) {
        Point v = m_anchors[0].resolve();
        Point a = m_anchors[1].resolve();
        painter.drawLine(QPointF(v.getX(), v.getY()), QPointF(a.getX(), a.getY()));
        painter.drawLine(QPointF(v.getX(), v.getY()), QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    } else if (m_type == DimensionType::Angular && m_dimensionEdges.size() == 1) {
        painter.drawLine(QPointF(m_dimensionEdges[0].first.getX(), m_dimensionEdges[0].first.getY()),
                         QPointF(m_dimensionEdges[0].second.getX(), m_dimensionEdges[0].second.getY()));
    } else if (m_type == DimensionType::Angular && m_dimensionEdges.size() == 2 && !m_pendingLinePoint.has_value()) {
        Point vertex;
        if (intersectInfiniteLines(m_dimensionEdges[0].first, m_dimensionEdges[0].second,
                                   m_dimensionEdges[1].first, m_dimensionEdges[1].second,
                                   vertex)) {
            Point a = fartherFrom(vertex, m_dimensionEdges[0]);
            Point b = fartherFrom(vertex, m_dimensionEdges[1]);
            painter.drawLine(QPointF(vertex.getX(), vertex.getY()), QPointF(a.getX(), a.getY()));
            painter.drawLine(QPointF(vertex.getX(), vertex.getY()), QPointF(b.getX(), b.getY()));
            double r = std::max(5.0, MathUtils::dist(vertex, m_cursorPos));
            QRectF arcRect(vertex.getX() - r, vertex.getY() - r, r * 2.0, r * 2.0);
            double a1 = std::atan2(a.getY() - vertex.getY(), a.getX() - vertex.getX()) * 180.0 / M_PI;
            double a2 = std::atan2(b.getY() - vertex.getY(), b.getX() - vertex.getX()) * 180.0 / M_PI;
            double span = a2 - a1;
            if (span > 180.0) span -= 360.0;
            if (span < -180.0) span += 360.0;
            painter.drawArc(arcRect, int(-a1 * 16.0), int(-span * 16.0));
        }
    } else if ((m_type == DimensionType::Radius || m_type == DimensionType::Diameter) && m_pendingLinePoint.has_value() && m_anchors.size() >= 2) {
        auto previewDim = buildPreviewDimension(m_cursorPos);
        Point c = m_anchors[0].resolve();
        Point e = m_anchors[1].resolve();
        Point lp = *m_pendingLinePoint;
        if (previewDim) {
            double r = previewDim->measuredValue();
            if (previewDim->getDimensionType() == DimensionType::Diameter) r *= 0.5;
            QPointF center(c.getX(), c.getY());
            QPointF dir(lp.getX() - c.getX(), lp.getY() - c.getY());
            const double len = std::hypot(dir.x(), dir.y());
            if (len > 1e-9) {
                dir /= len;
                QPointF edge1 = center + dir * r;
                QPointF edge2 = center - dir * r;
                QPointF start = previewDim->getDimensionType() == DimensionType::Diameter ? edge2 : center;
                QPointF end = edge1;
                QPointF vec = end - start;
                const double baseLen = std::hypot(vec.x(), vec.y());
                double t = previewDim->textPositionFactor();
                double startFactor = std::min(0.0, t);
                double endFactor = std::max(1.0, t);
                const double padFactor = baseLen > 1e-9 ? ((previewMetrics.horizontalAdvance(previewDim->displayText()) * 0.5 + 4.0) / scale) / baseLen : 0.0;
                if (t < 0.0) startFactor -= padFactor;
                if (t > 1.0) endFactor += padFactor;
                painter.drawLine(start + vec * startFactor, start + vec * endFactor);
                drawPreviewDimensionText(painter, *previewDim, scale);
            }
        } else {
            painter.drawLine(QPointF(c.getX(), c.getY()), QPointF(e.getX(), e.getY()));
            painter.drawLine(QPointF(c.getX(), c.getY()), QPointF(m_pendingLinePoint->getX(), m_pendingLinePoint->getY()));
        }
    } else if ((m_type == DimensionType::Radius || m_type == DimensionType::Diameter) && m_anchors.size() >= 2) {
        Point c = m_anchors[0].resolve();
        painter.drawLine(QPointF(c.getX(), c.getY()), QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    } else if (m_pendingLinePoint.has_value() && m_anchors.size() >= 2) {
        Point a = m_anchors[0].resolve();
        Point b = m_anchors[1].resolve();
        auto previewDim = buildPreviewDimension(m_cursorPos);
        const double textFactor = previewDim ? previewDim->textPositionFactor() : 0.5;
        const double textHalfWorld = previewDim ? ((previewMetrics.horizontalAdvance(previewDim->displayText()) * 0.5 + 4.0) / scale) : 0.0;
        drawPreviewDimension(a, b, *m_pendingLinePoint, textFactor, textHalfWorld);
        if (previewDim) drawPreviewDimensionText(painter, *previewDim, scale);
    } else if (m_anchors.size() == 1) {
        Point a = m_anchors[0].resolve();
        painter.drawLine(QPointF(a.getX(), a.getY()), QPointF(m_cursorPos.getX(), m_cursorPos.getY()));
    } else if (m_anchors.size() >= 2) {
        Point a = m_anchors[0].resolve();
        Point b = m_anchors[1].resolve();
        drawPreviewDimension(a, b, m_cursorPos, 0.5, 0.0);
    }
}

std::unique_ptr<Object> CreateDimensionTool::takeObject() { return std::move(m_result); }
void CreateDimensionTool::reset() {
    m_finished = false;
    m_anchors.clear();
    m_pendingLinePoint.reset();
    m_pendingAngularRadius.reset();
    m_pendingSupplementaryAngle = false;
    m_dimensionEdges.clear();
    m_hoverEdge.reset();
    m_hoverCurveObject = nullptr;
    m_result.reset();
}
