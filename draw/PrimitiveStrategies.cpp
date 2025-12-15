#include "PrimitiveStrategies.h"
#include "GlobalSettings.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <vector>

// =========================================================
// Вспомогательные функции для создания путей (Волна, Зигзаг)
// =========================================================

static QPainterPath createWavyPath(const QPointF& start, const QPointF& end, double zoomFactor) {
    QPainterPath path; path.moveTo(start);
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);

    if (length < 1e-6) return path;

    QTransform t;
    t.translate(start.x(), start.y());
    t.rotateRadians(angle);

    double screenAmp = 3.0;
    double screenPeriod = 10.0;

    // Амплитуда и период в мировых координатах, чтобы визуально размер сохранялся
    double worldAmp = screenAmp / zoomFactor;
    double worldPeriod = screenPeriod / zoomFactor;

    int steps = static_cast<int>(length / (worldPeriod / 8.0));
    if (steps < 2) steps = 2;

    for (int i = 0; i <= steps; ++i) {
        double x = (double)i / steps * length;
        double y = worldAmp * std::sin(x * 2 * M_PI / worldPeriod);
        path.lineTo(t.map(QPointF(x, y)));
    }
    return path;
}

static QPainterPath createZigZagPath(const QPointF& start, const QPointF& end, double zoomFactor) {
    QPainterPath path; path.moveTo(start);
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);

    if (length < 1e-6) return path;

    QTransform t;
    t.translate(start.x(), start.y());
    t.rotateRadians(angle);

    double worldAmp = 4.0 / zoomFactor;
    double worldPeriod = 10.0 / zoomFactor; // Период зигзага

    double currentX = 0;
    bool up = true;
    while (currentX < length) {
        currentX += worldPeriod / 2.0;
        double y = up ? worldAmp : -worldAmp;
        if (currentX > length) { currentX = length; y = 0; }
        path.lineTo(t.map(QPointF(currentX, y)));
        up = !up;
    }
    path.lineTo(end);
    return path;
}

static double getScale(const QPainter& p) {
    return p.transform().m11();
}

// Helper to get points on an ellipse
static std::vector<QPointF> getEllipsePoints(const QPointF& center, double rx, double ry, int steps = 100) {
    std::vector<QPointF> points;
    double step = 2 * M_PI / steps;
    for (int i = 0; i <= steps; ++i) {
        double angle = i * step;
        points.emplace_back(center.x() + rx * std::cos(angle), center.y() + ry * std::sin(angle));
    }
    return points;
}

static void drawStyledEllipse(QPainter& painter, const QPointF& center, double rx, double ry, const Object* obj) {
    LineStyleType type = obj->getLineStyle().type;

    if (type == LineStyleType::SolidWavy) {
        // Approximate ellipse with segments and use wavy generator
        auto points = getEllipsePoints(center, rx, ry, std::max(20, int(std::max(rx, ry) / 2)));
        QPainterPath path;
        double scale = getScale(painter);
        if(!points.empty()) {
            path.moveTo(points[0]);
            for(size_t i = 0; i < points.size() - 1; ++i) {
                // Generate wavy segment between points
                // Note: creating individual wavy paths for short segments might look disjointed,
                // but for a "mini CAD" this is a reasonable approximation without implementing a complex "wave along curve" shader/algo.
                // Better approach: interpolate the wave along the perimeter distance.
                
                // Let's use the createWavyPath for each segment, but we need to ensure continuity.
                // The current createWavyPath starts at 'start' and ends at 'end'.
                path.connectPath(createWavyPath(points[i], points[i+1], scale));
            }
        }
        painter.drawPath(path);
    } else if (type == LineStyleType::SolidZigZag) {
         auto points = getEllipsePoints(center, rx, ry, std::max(20, int(std::max(rx, ry) / 2)));
        QPainterPath path;
        double scale = getScale(painter);
        if(!points.empty()) {
            path.moveTo(points[0]);
            for(size_t i = 0; i < points.size() - 1; ++i) {
                path.connectPath(createZigZagPath(points[i], points[i+1], scale));
            }
        }
        painter.drawPath(path);
    } else {
        // Standard (including Dashed via QPen)
        painter.drawEllipse(center, rx, ry);
    }
}

// =========================================================
// Настройка пера по ГОСТ 2.303-68 (С учетом глобальных настроек)
// =========================================================
static void setupPen(QPainter& painter, const Object* obj, bool isSelected) {
    const LineStyle& style = obj->getLineStyle();
    const auto& global = GlobalSettings::instance();

    QPen pen;
    pen.setColor(obj->getColor());
    pen.setCapStyle(Qt::FlatCap);
    pen.setCosmetic(true);

    // Определяем базовую толщину: 2.0 для основных/толстых, 1.0 для тонких
    double baseWidth = 1.0;
    if (style.isMain || style.type == LineStyleType::SolidMain || style.type == LineStyleType::DashDotThick) {
        baseWidth = 2.0;
    }

    // Применяем глобальный масштаб толщины
    double s = baseWidth * global.globalWidthScale;
    if (s < 0.5) s = 0.5; // Минимальная видимая толщина

    QVector<qreal> dashes;
    // Глобальный масштаб штрихов (LTSCALE)
    double lsc = global.globalLinetypeScale;

    auto applyPattern = [&](LineStyleType t) {
        if (global.styleParams.count(t)) {
            const auto& p = global.styleParams.at(t);
            dashes << p.dash * lsc << p.gap * lsc;
            if (p.dash2 > 0 || p.gap2 > 0) {
                dashes << p.dash2 * lsc << p.gap2 * lsc;
            }
        }
    };

    switch (style.type) {
    case LineStyleType::SolidMain:
    case LineStyleType::SolidThin:
    case LineStyleType::SolidWavy:
    case LineStyleType::SolidZigZag:
        pen.setStyle(Qt::SolidLine);
        pen.setWidthF(s);
        break;

    case LineStyleType::Dashed:
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s);
        applyPattern(LineStyleType::Dashed);
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::DashDotThin:
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s);
        applyPattern(LineStyleType::DashDotThin);
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::DashDotThick:
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s);
        applyPattern(LineStyleType::DashDotThick);
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::DashDotDot:
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s);
        applyPattern(LineStyleType::DashDotDot);
        pen.setDashPattern(dashes);
        break;

    case LineStyleType::Custom:
        pen.setStyle(Qt::CustomDashLine);
        pen.setWidthF(s);
        dashes << style.dashLength * lsc << style.gapLength * lsc;
        pen.setDashPattern(dashes);
        break;
    }

    if (isSelected) {
        // Цвет выделения (розовый/мажента)
        pen.setColor(QColor("#F92672"));
    }

    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
}

// =========================================================
// Реализация отрисовки примитивов
// =========================================================

void SegmentDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* s = static_cast<Segment*>(primitive);
    setupPen(painter, s, isSelected);

    QPointF start(s->getStart().getX(), s->getStart().getY());
    QPointF end(s->getEnd().getX(), s->getEnd().getY());

    auto styleType = s->getLineStyle().type;
    if (styleType == LineStyleType::SolidWavy) {
        painter.drawPath(createWavyPath(start, end, getScale(painter)));
    } else if (styleType == LineStyleType::SolidZigZag) {
        painter.drawPath(createZigZagPath(start, end, getScale(painter)));
    } else {
        painter.drawLine(start, end);
    }
}

// Helper for Rect/Poly lines
static void drawStyledPath(QPainter& painter, const QPainterPath& path, const Object* obj) {
    LineStyleType type = obj->getLineStyle().type;
    if (type == LineStyleType::SolidWavy || type == LineStyleType::SolidZigZag) {
        // Iterate elements and apply style to each segment
        QPainterPath styledPath;
        double scale = getScale(painter);
        
        for (int i = 0; i < path.elementCount() - 1; ++i) {
             QPainterPath::Element e1 = path.elementAt(i);
             QPainterPath::Element e2 = path.elementAt(i+1);
             // Skip move to if purely moving (handled by loop logic: we connect segments)
             if (e2.type == QPainterPath::MoveToElement) continue; 
             
             // If we have a gap (MoveTo), restart
             if (e1.type == QPainterPath::MoveToElement) {
                 styledPath.moveTo(e1.x, e1.y);
             }

             if (type == LineStyleType::SolidWavy) {
                 styledPath.connectPath(createWavyPath(QPointF(e1.x, e1.y), QPointF(e2.x, e2.y), scale));
             } else {
                 styledPath.connectPath(createZigZagPath(QPointF(e1.x, e1.y), QPointF(e2.x, e2.y), scale));
             }
        }
        // Handle closing if needed (not automatic for path elements unless we check isClosed)
        // For simple rects/polygons, we might want to ensure closure.
        // Assuming path is just a sequence of lines for now.
        painter.drawPath(styledPath);
    } else {
        painter.drawPath(path);
    }
}

void CircleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Circle*>(primitive);
    setupPen(painter, obj, isSelected); 
    // Use the styled ellipse helper which supports Wavy/ZigZag
    drawStyledEllipse(painter, QPointF(obj->getCenter().getX(), obj->getCenter().getY()), obj->getRadius(), obj->getRadius(), obj);
}

void ArcDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Arc*>(primitive);
    setupPen(painter, obj, isSelected);
    // For Arcs, Wavy/ZigZag is complex. Let's approximate if needed, or fallback.
    // For now, standard arc rendering. If user needs Wavy Arc, we'd need getArcPoints.
    // Let's implement basic support via path approximation if custom style.
    LineStyleType type = obj->getLineStyle().type;
    if (type == LineStyleType::SolidWavy || type == LineStyleType::SolidZigZag) {
        QPainterPath path;
        double r = obj->getRadius();
        QRectF rect(obj->getCenter().getX() - r, obj->getCenter().getY() - r, r * 2, r * 2);
        path.arcMoveTo(rect, obj->getStartAngle());
        path.arcTo(rect, obj->getStartAngle(), obj->getSpanAngle());
        
        // Convert arc path to flattened subpaths for styling
        QPainterPath styledPath;
        double scale = getScale(painter);
        // Flatten to small lines
        QPainterPath flatParams = path; // Default flattening is usually fine or we can use toSubpathPolygons
        // Simple manual approximation:
        int steps = std::max(10, int(std::abs(obj->getSpanAngle()) / 5));
        double step = obj->getSpanAngle() / steps;
        double start = obj->getStartAngle();
        Point c = obj->getCenter();
        QPointF prev(c.getX() + r * std::cos(start * M_PI/180), c.getY() + r * std::sin(start * M_PI/180));
        styledPath.moveTo(prev);
        
        for(int i=1; i<=steps; ++i) {
            double a = start + i*step;
            QPointF cur(c.getX() + r * std::cos(a * M_PI/180), c.getY() + r * std::sin(a * M_PI/180));
             if (type == LineStyleType::SolidWavy)
                 styledPath.connectPath(createWavyPath(prev, cur, scale));
             else 
                 styledPath.connectPath(createZigZagPath(prev, cur, scale));
            prev = cur;
        }
        painter.drawPath(styledPath);
    } else {
        double r = obj->getRadius();
        QRectF rect(obj->getCenter().getX() - r, obj->getCenter().getY() - r, r * 2, r * 2);
        painter.drawArc(rect, int(obj->getStartAngle() * 16), int(obj->getSpanAngle() * 16));
    }
}

void RectangleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Rectangle*>(primitive);
    setupPen(painter, obj, isSelected);

    double minX = obj->getTopLeft().getX();
    double maxY = obj->getTopLeft().getY();
    double w = obj->getWidth();
    double h = obj->getHeight();
    
    LineStyleType type = obj->getLineStyle().type;
    if (type == LineStyleType::SolidWavy || type == LineStyleType::SolidZigZag) {
         QPainterPath path;
         path.moveTo(minX, maxY); // TL
         path.lineTo(minX + w, maxY); // TR
         path.lineTo(minX + w, maxY - h); // BR
         path.lineTo(minX, maxY - h); // BL
         path.lineTo(minX, maxY); // Close
         drawStyledPath(painter, path, obj);
    } else {
        painter.drawRoundedRect(QRectF(minX, maxY - h, w, h),
                                obj->getCornerRadius(), obj->getCornerRadius());
    }
}

void EllipseDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Ellipse*>(primitive);
    setupPen(painter, obj, isSelected);
    drawStyledEllipse(painter, QPointF(obj->getCenter().getX(), obj->getCenter().getY()),
                        obj->getRadiusX(), obj->getRadiusY(), obj);
}

void PolygonDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<PolygonObj*>(primitive);
    setupPen(painter, obj, isSelected);

    QPolygonF poly;
    int sides = std::max(3, obj->getSides());
    double step = 2 * M_PI / sides;
    double startAngle = M_PI / 2;
    double r = obj->getRadius();

    if (!obj->isInscribed()) r = r / std::cos(M_PI / sides);

    for (int i = 0; i < sides; ++i) {
        double angle = startAngle + i * step;
        poly << QPointF(obj->getCenter().getX() + r * std::cos(angle),
                        obj->getCenter().getY() + r * std::sin(angle));
    }
    
    LineStyleType type = obj->getLineStyle().type;
     if (type == LineStyleType::SolidWavy || type == LineStyleType::SolidZigZag) {
        QPainterPath path;
        path.addPolygon(poly);
        path.closeSubpath(); // Ensure closed
        drawStyledPath(painter, path, obj);
     } else {
        painter.drawPolygon(poly);
     }
}

void SplineDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Spline*>(primitive);
    const auto& points = obj->getPoints();
    if (points.size() < 2) return;

    setupPen(painter, obj, isSelected);

    QPainterPath path;
    path.moveTo(points[0].getX(), points[0].getY());
    
    // Smooth spline using cubic beziers (Catmull-Rom or simple cubic between points)
    // Simple approach: Cubic to next point using control points based on neighbors
    // Or just QPainterPath::cubicTo() with estimated control points.
    // Let's implementation a basic Catmull-Rom spline conversion to Bezier:
    
    // Safe bounds check for loop
    if (points.size() >= 2) {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            Point p0 = (i == 0) ? points[0] : points[i-1];
            Point p1 = points[i];
            Point p2 = points[i+1];
            // Safe access for p3
            Point p3 = (i + 2 < points.size()) ? points[i+2] : p2;

            double alpha = 0.5;

            double d1 = std::hypot(p1.getX()-p0.getX(), p1.getY()-p0.getY());
            double d2 = std::hypot(p2.getX()-p1.getX(), p2.getY()-p1.getY());
            double d3 = std::hypot(p3.getX()-p2.getX(), p3.getY()-p2.getY());
            
            if (d1 < 1e-6) d1 = 1.0; if (d2 < 1e-6) d2 = 1.0; if (d3 < 1e-6) d3 = 1.0;
            
            double cp1x = p1.getX() + (p2.getX() - p0.getX()) / 6.0;
            double cp1y = p1.getY() + (p2.getY() - p0.getY()) / 6.0;

            double cp2x = p2.getX() - (p3.getX() - p1.getX()) / 6.0;
            double cp2y = p2.getY() - (p3.getY() - p1.getY()) / 6.0;

            path.cubicTo(cp1x, cp1y, cp2x, cp2y, p2.getX(), p2.getY());
        }
    }
    
    // Apply style to Spline
    LineStyleType type = obj->getLineStyle().type;
    if (type == LineStyleType::SolidWavy || type == LineStyleType::SolidZigZag) {
         drawStyledPath(painter, path, obj);
    } else {
         painter.drawPath(path);
    }

    // Рисуем опорные точки, если объект выделен
    if (isSelected) {
        painter.setBrush(QColor("#F92672"));
        painter.setPen(Qt::NoPen);
        double s = 6.0 / getScale(painter); 
        for(const auto& p : points) {
            painter.drawEllipse(QPointF(p.getX(), p.getY()), s/2, s/2);
        }
    }
}
