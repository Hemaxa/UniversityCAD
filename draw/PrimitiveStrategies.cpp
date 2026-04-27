#include "PrimitiveStrategies.h"
#include "GlobalSettings.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QtMath>
#include <vector>

// =========================================================
// Вспомогательные функции для создания путей (Волна, Зигзаг)
// =========================================================

// Создание волнистой линии по ГОСТ - амплитуда и период в мировых координатах
static QPainterPath createWavyPath(const QPointF& start, const QPointF& end) {
    QPainterPath path;
    path.moveTo(start);
    
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);

    if (length < 1e-6) return path;

    QTransform t;
    t.translate(start.x(), start.y());
    t.rotateRadians(angle);

    // Используем глобальные настройки для волнистой линии
    const auto& wavy = GlobalSettings::instance().wavyParams;
    double worldAmp = wavy.amplitude;
    double worldPeriod = wavy.period;

    // Количество точек для плавной синусоиды
    int steps = std::max(2, static_cast<int>(length / (worldPeriod / 16.0)));
    
    for (int i = 1; i <= steps; ++i) {
        double x = (double)i / steps * length;
        double y = worldAmp * std::sin(x * 2.0 * M_PI / worldPeriod);
        path.lineTo(t.map(QPointF(x, y)));
    }
    
    return path;
}

// Создание линии с изломами по ГОСТ 2.303-68
// Паттерн: прямой участок → излом вниз → пересечение линии → излом вверх → обратно на линию
// Изломы в обе стороны (сначала вниз, затем вверх)
static QPainterPath createZigZagPath(const QPointF& start, const QPointF& end) {
    QPainterPath path;
    path.moveTo(start);
    
    double dx = end.x() - start.x();
    double dy = end.y() - start.y();
    double length = std::sqrt(dx * dx + dy * dy);
    double angle = std::atan2(dy, dx);

    if (length < 1e-6) return path;

    QTransform t;
    t.translate(start.x(), start.y());
    t.rotateRadians(angle);

    // Используем глобальные настройки
    const auto& zigzag = GlobalSettings::instance().zigzagParams;
    double amp = zigzag.amplitude;           // Высота излома
    double straightLen = zigzag.straightLength; // Длина прямого участка
    double breakLen = zigzag.breakLength;    // Длина наклонного участка излома
    
    double currentX = 0;
    
    while (currentX < length) {
        // 1. Прямой участок
        double nextX = currentX + straightLen;
        if (nextX >= length) {
            path.lineTo(end);
            break;
        }
        path.lineTo(t.map(QPointF(nextX, 0)));
        currentX = nextX;
        
        // 2. Излом вниз (первая половина)
        nextX = currentX + breakLen / 2.0;
        if (nextX >= length) {
            path.lineTo(end);
            break;
        }
        path.lineTo(t.map(QPointF(nextX, -amp)));
        currentX = nextX;
        
        // 3. Проход через линию к верхней точке
        nextX = currentX + breakLen;
        if (nextX >= length) {
            path.lineTo(end);
            break;
        }
        path.lineTo(t.map(QPointF(nextX, amp)));
        currentX = nextX;
        
        // 4. Возврат обратно на линию
        nextX = currentX + breakLen / 2.0;
        if (nextX >= length) {
            path.lineTo(end);
            break;
        }
        path.lineTo(t.map(QPointF(nextX, 0)));
        currentX = nextX;
    }
    
    // Убедимся, что линия доходит до конца
    if (path.currentPosition() != end) {
        path.lineTo(end);
    }
    
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
        // Создаем волнистую линию вдоль всей окружности/эллипса
        // Используем параметризацию по углу и применяем синусоидальное смещение
        const auto& wavy = GlobalSettings::instance().wavyParams;
        double amplitude = wavy.amplitude;
        double period = wavy.period;
        
        // Приблизительная длина окружности/эллипса для расчета количества волн
        double circumference = M_PI * (3.0 * (rx + ry) - std::sqrt((3.0 * rx + ry) * (rx + 3.0 * ry)));
        
        // Количество точек для плавной волны - больше точек для более плавной линии
        int numPoints = std::max(180, int(circumference / 2.0));
        
        QPainterPath path;
        for (int i = 0; i <= numPoints; ++i) {
            double t = (double)i / numPoints;
            double angle = t * 2.0 * M_PI;
            
            // Базовая точка на эллипсе
            double baseX = center.x() + rx * std::cos(angle);
            double baseY = center.y() + ry * std::sin(angle);
            
            // Направление нормали (наружу от эллипса)
            // Для эллипса нормаль не совпадает с радиальным направлением
            double nx = ry * std::cos(angle);  // Компонента нормали X
            double ny = rx * std::sin(angle);  // Компонента нормали Y
            double nLen = std::sqrt(nx * nx + ny * ny);
            if (nLen > 1e-9) {
                nx /= nLen;
                ny /= nLen;
            }
            
            // Длина вдоль кривой (приблизительно)
            double arcLen = t * circumference;
            
            // Синусоидальное смещение вдоль нормали
            double offset = amplitude * std::sin(arcLen * 2.0 * M_PI / period);
            
            double px = baseX + offset * nx;
            double py = baseY + offset * ny;
            
            if (i == 0) {
                path.moveTo(px, py);
            } else {
                path.lineTo(px, py);
            }
        }
        painter.drawPath(path);
        
    } else if (type == LineStyleType::SolidZigZag) {
        // Создаем зигзаг линию вдоль всей окружности/эллипса
        const auto& zigzag = GlobalSettings::instance().zigzagParams;
        double amplitude = zigzag.amplitude;
        double straightLen = zigzag.straightLength;
        double breakLen = zigzag.breakLength;
        
        // Приблизительная длина окружности/эллипса
        double circumference = M_PI * (3.0 * (rx + ry) - std::sqrt((3.0 * rx + ry) * (rx + 3.0 * ry)));
        
        // Один полный паттерн: прямой участок + излом
        double patternLen = straightLen + 2.0 * breakLen;
        
        QPainterPath path;
        double currentArcLen = 0.0;
        double totalLen = circumference;
        bool firstPoint = true;
        int direction = 1; // Чередуем направление излома
        
        while (currentArcLen < totalLen) {
            // Вычисляем угол по длине дуги (приблизительно)
            double t = currentArcLen / circumference;
            double angle = t * 2.0 * M_PI;
            
            // Базовая точка на эллипсе
            double baseX = center.x() + rx * std::cos(angle);
            double baseY = center.y() + ry * std::sin(angle);
            
            // Направление нормали
            double nx = ry * std::cos(angle);
            double ny = rx * std::sin(angle);
            double nLen = std::sqrt(nx * nx + ny * ny);
            if (nLen > 1e-9) {
                nx /= nLen;
                ny /= nLen;
            }
            
            if (firstPoint) {
                path.moveTo(baseX, baseY);
                firstPoint = false;
            }
            
            // Прямой участок
            double nextArcLen = currentArcLen + straightLen;
            if (nextArcLen >= totalLen) {
                // Замыкаем на начальную точку
                path.lineTo(center.x() + rx, center.y());
                break;
            }
            
            double t2 = nextArcLen / circumference;
            double angle2 = t2 * 2.0 * M_PI;
            double x2 = center.x() + rx * std::cos(angle2);
            double y2 = center.y() + ry * std::sin(angle2);
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            // Излом вниз (первая половина)
            nextArcLen = currentArcLen + breakLen / 2.0;
            if (nextArcLen >= totalLen) {
                path.lineTo(center.x() + rx, center.y());
                break;
            }
            t2 = nextArcLen / circumference;
            angle2 = t2 * 2.0 * M_PI;
            double nx2 = ry * std::cos(angle2);
            double ny2 = rx * std::sin(angle2);
            nLen = std::sqrt(nx2 * nx2 + ny2 * ny2);
            if (nLen > 1e-9) { nx2 /= nLen; ny2 /= nLen; }
            x2 = center.x() + rx * std::cos(angle2) - direction * amplitude * nx2;
            y2 = center.y() + ry * std::sin(angle2) - direction * amplitude * ny2;
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            // Проход через линию к верхней точке
            nextArcLen = currentArcLen + breakLen;
            if (nextArcLen >= totalLen) {
                path.lineTo(center.x() + rx, center.y());
                break;
            }
            t2 = nextArcLen / circumference;
            angle2 = t2 * 2.0 * M_PI;
            nx2 = ry * std::cos(angle2);
            ny2 = rx * std::sin(angle2);
            nLen = std::sqrt(nx2 * nx2 + ny2 * ny2);
            if (nLen > 1e-9) { nx2 /= nLen; ny2 /= nLen; }
            x2 = center.x() + rx * std::cos(angle2) + direction * amplitude * nx2;
            y2 = center.y() + ry * std::sin(angle2) + direction * amplitude * ny2;
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            // Возврат на линию
            nextArcLen = currentArcLen + breakLen / 2.0;
            if (nextArcLen >= totalLen) {
                path.lineTo(center.x() + rx, center.y());
                break;
            }
            t2 = nextArcLen / circumference;
            angle2 = t2 * 2.0 * M_PI;
            x2 = center.x() + rx * std::cos(angle2);
            y2 = center.y() + ry * std::sin(angle2);
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            direction = -direction; // Чередуем направление
        }
        painter.drawPath(path);
        
    } else {
        // For dashed lines and other styles, approximate ellipse with segments
        // to ensure proper dash pattern rendering
        bool needsSegmentation = (type == LineStyleType::Dashed || 
                                 type == LineStyleType::DashDotThin || 
                                 type == LineStyleType::DashDotThick || 
                                 type == LineStyleType::DashDotDot || 
                                 type == LineStyleType::Custom);
        
        if (needsSegmentation) {
            int numSegments = std::max(40, int(std::max(rx, ry) / 2));
            auto points = getEllipsePoints(center, rx, ry, numSegments);
            QPainterPath path;
            if(!points.empty()) {
                path.moveTo(points[0]);
                for(size_t i = 1; i < points.size(); ++i) {
                    path.lineTo(points[i]);
                }
                path.closeSubpath();
            }
            painter.drawPath(path);
        } else {
            // Standard solid lines
            painter.drawEllipse(center, rx, ry);
        }
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

    // Определяем базовую толщину (в мм)
    double baseWidth;
    
    // Если стиль имеет кастомную толщину (> 0), используем её
    if (style.customWidth > 0) {
        baseWidth = style.customWidth;
    } else {
        // Толстые типы: SolidMain и DashDotThick
        // Все остальные — тонкие (SolidThin, Dashed, DashDotThin, DashDotDot, Wavy, ZigZag)
        bool isThick = (style.type == LineStyleType::SolidMain || style.type == LineStyleType::DashDotThick);
        baseWidth = isThick ? global.mainLineWidth : global.thinLineWidth;
    }

    // Применяем ОБЩИЙ глобальный масштаб толщины
    double s = baseWidth * global.globalWidthScale;
    
    // Cosmetic pen использует пиксели, а не мм. Переводим мм → пиксели.
    // Стандартный экран ~ 96 DPI → 1 мм ≈ 3.78 px.
    // Используем коэффициент, чтобы толщина линий на экране соответствовала реальным мм.
    constexpr double mmToPx = 96.0 / 25.4; // ≈ 3.78 px/mm
    s *= mmToPx;
    
    if (s < 1.0) s = 1.0; // Минимальная видимая толщина — 1 пиксель

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
        // Для Custom используем индивидуальные параметры объекта, но масштабируем
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

void PointDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<PointObject*>(primitive);
    double scale = getScale(painter);
    if (scale < 1e-9) scale = 1.0;
    double s = 4.0 / scale; // Фиксированный экранный размер крестика
    
    QPen pen;
    pen.setColor(isSelected ? QColor("#F92672") : obj->getColor());
    pen.setWidthF(isSelected ? 2.0 / scale : 1.5 / scale);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    
    double x = obj->getPosition().getX();
    double y = obj->getPosition().getY();
    
    // Рисуем × (крестик)
    painter.drawLine(QPointF(x - s, y - s), QPointF(x + s, y + s));
    painter.drawLine(QPointF(x + s, y - s), QPointF(x - s, y + s));
}

void SegmentDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* s = static_cast<Segment*>(primitive);
    setupPen(painter, s, isSelected);

    QPointF start(s->getStart().getX(), s->getStart().getY());
    QPointF end(s->getEnd().getX(), s->getEnd().getY());

    auto styleType = s->getLineStyle().type;
    if (styleType == LineStyleType::SolidWavy) {
        painter.drawPath(createWavyPath(start, end));
    } else if (styleType == LineStyleType::SolidZigZag) {
        painter.drawPath(createZigZagPath(start, end));
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
                 styledPath.connectPath(createWavyPath(QPointF(e1.x, e1.y), QPointF(e2.x, e2.y)));
             } else {
                 styledPath.connectPath(createZigZagPath(QPointF(e1.x, e1.y), QPointF(e2.x, e2.y)));
             }
        }
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
    
    LineStyleType type = obj->getLineStyle().type;
    double r = obj->getRadius();
    Point c = obj->getCenter();
    double startAngle = obj->getStartAngle();
    double spanAngle = obj->getSpanAngle();
    
    if (type == LineStyleType::SolidWavy) {
        // Создаем волнистую линию вдоль всей дуги
        const auto& wavy = GlobalSettings::instance().wavyParams;
        double amplitude = wavy.amplitude;
        double period = wavy.period;
        
        // Длина дуги
        double arcLength = std::abs(spanAngle) * M_PI / 180.0 * r;
        
        // Количество точек для плавной волны
        int numPoints = std::max(90, int(arcLength / 2.0));
        
        QPainterPath path;
        for (int i = 0; i <= numPoints; ++i) {
            double t = (double)i / numPoints;
            double angle = (startAngle + t * spanAngle) * M_PI / 180.0;
            
            // Базовая точка на дуге
            double baseX = c.getX() + r * std::cos(angle);
            double baseY = c.getY() + r * std::sin(angle);
            
            // Направление нормали (радиальное направление для окружности)
            double nx = std::cos(angle);
            double ny = std::sin(angle);
            
            // Длина вдоль дуги
            double arcLen = t * arcLength;
            
            // Синусоидальное смещение вдоль нормали
            double offset = amplitude * std::sin(arcLen * 2.0 * M_PI / period);
            
            double px = baseX + offset * nx;
            double py = baseY + offset * ny;
            
            if (i == 0) {
                path.moveTo(px, py);
            } else {
                path.lineTo(px, py);
            }
        }
        painter.drawPath(path);
        
    } else if (type == LineStyleType::SolidZigZag) {
        // Создаем зигзаг линию вдоль всей дуги
        const auto& zigzag = GlobalSettings::instance().zigzagParams;
        double amplitude = zigzag.amplitude;
        double straightLen = zigzag.straightLength;
        double breakLen = zigzag.breakLength;
        
        // Длина дуги
        double arcLength = std::abs(spanAngle) * M_PI / 180.0 * r;
        
        QPainterPath path;
        double currentArcLen = 0.0;
        double totalLen = arcLength;
        bool firstPoint = true;
        int direction = 1; // Чередуем направление излома
        
        auto getPointOnArc = [&](double arcLen) -> std::pair<double, double> {
            double t = arcLen / arcLength;
            double angle = (startAngle + t * spanAngle) * M_PI / 180.0;
            return {c.getX() + r * std::cos(angle), c.getY() + r * std::sin(angle)};
        };
        
        auto getNormalOnArc = [&](double arcLen) -> std::pair<double, double> {
            double t = arcLen / arcLength;
            double angle = (startAngle + t * spanAngle) * M_PI / 180.0;
            return {std::cos(angle), std::sin(angle)};
        };
        
        while (currentArcLen < totalLen) {
            auto [baseX, baseY] = getPointOnArc(currentArcLen);
            
            if (firstPoint) {
                path.moveTo(baseX, baseY);
                firstPoint = false;
            }
            
            // Прямой участок
            double nextArcLen = currentArcLen + straightLen;
            if (nextArcLen >= totalLen) {
                auto [endX, endY] = getPointOnArc(totalLen);
                path.lineTo(endX, endY);
                break;
            }
            
            auto [x2, y2] = getPointOnArc(nextArcLen);
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            // Излом вниз (первая половина)
            nextArcLen = currentArcLen + breakLen / 2.0;
            if (nextArcLen >= totalLen) {
                auto [endX, endY] = getPointOnArc(totalLen);
                path.lineTo(endX, endY);
                break;
            }
            auto [bx, by] = getPointOnArc(nextArcLen);
            auto [nx, ny] = getNormalOnArc(nextArcLen);
            x2 = bx - direction * amplitude * nx;
            y2 = by - direction * amplitude * ny;
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            // Проход через линию к верхней точке
            nextArcLen = currentArcLen + breakLen;
            if (nextArcLen >= totalLen) {
                auto [endX, endY] = getPointOnArc(totalLen);
                path.lineTo(endX, endY);
                break;
            }
            std::tie(bx, by) = getPointOnArc(nextArcLen);
            std::tie(nx, ny) = getNormalOnArc(nextArcLen);
            x2 = bx + direction * amplitude * nx;
            y2 = by + direction * amplitude * ny;
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            // Возврат на линию
            nextArcLen = currentArcLen + breakLen / 2.0;
            if (nextArcLen >= totalLen) {
                auto [endX, endY] = getPointOnArc(totalLen);
                path.lineTo(endX, endY);
                break;
            }
            std::tie(x2, y2) = getPointOnArc(nextArcLen);
            path.lineTo(x2, y2);
            currentArcLen = nextArcLen;
            
            direction = -direction; // Чередуем направление
        }
        painter.drawPath(path);
        
    } else {
        // For dashed lines and other styles, approximate arc with segments
        // to ensure proper dash pattern rendering
        bool needsSegmentation = (type == LineStyleType::Dashed || 
                                 type == LineStyleType::DashDotThin || 
                                 type == LineStyleType::DashDotThick || 
                                 type == LineStyleType::DashDotDot || 
                                 type == LineStyleType::Custom);
        
        if (needsSegmentation) {
            QPainterPath path;
            int steps = std::max(40, int(std::abs(spanAngle) / 2));
            double step = spanAngle / steps;
            
            QPointF first(c.getX() + r * std::cos(startAngle * M_PI/180), c.getY() + r * std::sin(startAngle * M_PI/180));
            path.moveTo(first);
            
            for(int i = 1; i <= steps; ++i) {
                double a = startAngle + i * step;
                QPointF cur(c.getX() + r * std::cos(a * M_PI/180), c.getY() + r * std::sin(a * M_PI/180));
                path.lineTo(cur);
            }
            painter.drawPath(path);
        } else {
            // Standard solid lines
            // Qt использует углы в 1/16 градуса
            // Инвертируем углы для корректного отображения в мировых координатах
            QRectF rect(c.getX() - r, c.getY() - r, r * 2, r * 2);
            int qtStartAngle = int(-startAngle * 16);
            int qtSpanAngle = int(-spanAngle * 16);
            painter.drawArc(rect, qtStartAngle, qtSpanAngle);
        }
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

    auto smoothPoints = obj->getSmoothPoints();
    if (smoothPoints.empty()) return;
    
    LineStyleType type = obj->getLineStyle().type;
    
    if (type == LineStyleType::SolidWavy) {
        // Волнистая линия вдоль всего сплайна
        const auto& wavy = GlobalSettings::instance().wavyParams;
        double amplitude = wavy.amplitude;
        double period = wavy.period;
        
        // Используем высокое разрешение для гладкости
        auto hiRes = obj->getSmoothPoints(100);
        if (hiRes.size() < 2) return;
        
        // Вычисляем кумулятивные длины дуги
        std::vector<double> arcLengths(hiRes.size(), 0.0);
        for (size_t i = 1; i < hiRes.size(); ++i) {
            double dx = hiRes[i].getX() - hiRes[i-1].getX();
            double dy = hiRes[i].getY() - hiRes[i-1].getY();
            arcLengths[i] = arcLengths[i-1] + std::sqrt(dx*dx + dy*dy);
        }
        double totalLen = arcLengths.back();
        if (totalLen < 1e-9) return;
        
        // Ресемплируем: ~10 точек на период волны, минимум 200 точек
        int numSamples = std::max(200, (int)(totalLen / period * 10));
        double step = totalLen / numSamples;
        
        // Хелпер: интерполяция позиции и касательной по длине дуги
        auto sampleAt = [&](double s) -> std::tuple<double, double, double, double> {
            // Бинарный поиск
            size_t lo = 0, hi = arcLengths.size() - 1;
            while (lo + 1 < hi) {
                size_t mid = (lo + hi) / 2;
                if (arcLengths[mid] <= s) lo = mid;
                else hi = mid;
            }
            double segLen = arcLengths[hi] - arcLengths[lo];
            double t = (segLen > 1e-9) ? (s - arcLengths[lo]) / segLen : 0.0;
            double bx = hiRes[lo].getX() + t * (hiRes[hi].getX() - hiRes[lo].getX());
            double by = hiRes[lo].getY() + t * (hiRes[hi].getY() - hiRes[lo].getY());
            // Касательная (усредненная)
            double tx = hiRes[hi].getX() - hiRes[lo].getX();
            double ty = hiRes[hi].getY() - hiRes[lo].getY();
            double tl = std::sqrt(tx*tx + ty*ty);
            if (tl < 1e-9) tl = 1.0;
            double nx = -ty / tl;
            double ny =  tx / tl;
            return {bx, by, nx, ny};
        };
        
        QPainterPath path;
        for (int i = 0; i <= numSamples; ++i) {
            double s = i * step;
            auto [bx, by, nx, ny] = sampleAt(s);
            double offset = amplitude * std::sin(s * 2.0 * M_PI / period);
            double px = bx + offset * nx;
            double py = by + offset * ny;
            if (i == 0) path.moveTo(px, py);
            else path.lineTo(px, py);
        }
        painter.drawPath(path);
        
    } else if (type == LineStyleType::SolidZigZag) {
        // Зигзаг вдоль всего сплайна
        const auto& zigzag = GlobalSettings::instance().zigzagParams;
        double amplitude = zigzag.amplitude;
        double straightLen = zigzag.straightLength;
        double breakLen = zigzag.breakLength;
        
        // Используем высокое разрешение для гладкости
        auto hiRes = obj->getSmoothPoints(100);
        if (hiRes.size() < 2) return;
        
        // Вычисляем кумулятивные длины дуги
        std::vector<double> arcLengths(hiRes.size(), 0.0);
        for (size_t i = 1; i < hiRes.size(); ++i) {
            double dx = hiRes[i].getX() - hiRes[i-1].getX();
            double dy = hiRes[i].getY() - hiRes[i-1].getY();
            arcLengths[i] = arcLengths[i-1] + std::sqrt(dx*dx + dy*dy);
        }
        double totalLen = arcLengths.back();
        
        // Хелпер: найти точку и нормаль по длине дуги
        auto getPointAndNormal = [&](double s) -> std::tuple<double, double, double, double> {
            size_t lo = 0, hi = arcLengths.size() - 1;
            while (lo + 1 < hi) {
                size_t mid = (lo + hi) / 2;
                if (arcLengths[mid] <= s) lo = mid;
                else hi = mid;
            }
            double segLen = arcLengths[hi] - arcLengths[lo];
            double t = (segLen > 1e-9) ? (s - arcLengths[lo]) / segLen : 0.0;
            double bx = hiRes[lo].getX() + t * (hiRes[hi].getX() - hiRes[lo].getX());
            double by = hiRes[lo].getY() + t * (hiRes[hi].getY() - hiRes[lo].getY());
            double tx = hiRes[hi].getX() - hiRes[lo].getX();
            double ty = hiRes[hi].getY() - hiRes[lo].getY();
            double tl = std::sqrt(tx*tx + ty*ty);
            if (tl < 1e-9) tl = 1.0;
            double nx = -ty / tl;
            double ny =  tx / tl;
            return {bx, by, nx, ny};
        };
        
        QPainterPath path;
        double patternLen = straightLen + 2.0 * breakLen;
        double currentS = 0.0;
        int direction = 1;
        bool first = true;
        
        while (currentS < totalLen) {
            auto [bx, by, nx, ny] = getPointAndNormal(std::min(currentS, totalLen));
            if (first) { path.moveTo(bx, by); first = false; }
            
            // Прямой участок
            double nextS = currentS + straightLen;
            if (nextS >= totalLen) {
                auto [ex, ey, enx, eny] = getPointAndNormal(totalLen);
                path.lineTo(ex, ey); break;
            }
            auto [sx, sy, snx, sny] = getPointAndNormal(nextS);
            path.lineTo(sx, sy);
            currentS = nextS;
            
            // Излом вниз
            nextS = currentS + breakLen / 2.0;
            if (nextS >= totalLen) {
                auto [ex, ey, enx, eny] = getPointAndNormal(totalLen);
                path.lineTo(ex, ey); break;
            }
            auto [d1x, d1y, d1nx, d1ny] = getPointAndNormal(nextS);
            path.lineTo(d1x - direction * amplitude * d1nx, d1y - direction * amplitude * d1ny);
            currentS = nextS;
            
            // Проход через линию вверх
            nextS = currentS + breakLen;
            if (nextS >= totalLen) {
                auto [ex, ey, enx, eny] = getPointAndNormal(totalLen);
                path.lineTo(ex, ey); break;
            }
            auto [u1x, u1y, u1nx, u1ny] = getPointAndNormal(nextS);
            path.lineTo(u1x + direction * amplitude * u1nx, u1y + direction * amplitude * u1ny);
            currentS = nextS;
            
            // Возврат на линию
            nextS = currentS + breakLen / 2.0;
            if (nextS >= totalLen) {
                auto [ex, ey, enx, eny] = getPointAndNormal(totalLen);
                path.lineTo(ex, ey); break;
            }
            auto [rx, ry, rnx, rny] = getPointAndNormal(nextS);
            path.lineTo(rx, ry);
            currentS = nextS;
            
            direction = -direction;
        }
        painter.drawPath(path);
        
    } else {
        // Для всех остальных типов — стандартный сглаженный путь
        QPainterPath path;
        path.moveTo(smoothPoints[0].getX(), smoothPoints[0].getY());
        for (size_t i = 1; i < smoothPoints.size(); ++i) {
            path.lineTo(smoothPoints[i].getX(), smoothPoints[i].getY());
        }
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

static void drawArrowHead(QPainter& painter, const QPointF& tip, double angle, const Dimension* d, double invScale)
{
    const double size = d->arrowSize() * invScale;
    const QPointF back(tip.x() - std::cos(angle) * size, tip.y() - std::sin(angle) * size);
    const QPointF left(back.x() + std::cos(angle + M_PI / 2.0) * size * 0.35,
                       back.y() + std::sin(angle + M_PI / 2.0) * size * 0.35);
    const QPointF right(back.x() + std::cos(angle - M_PI / 2.0) * size * 0.35,
                        back.y() + std::sin(angle - M_PI / 2.0) * size * 0.35);

    if (d->arrowType() == ArrowType::Dot) {
        painter.setBrush(d->arrowFilled() ? QBrush(d->dimensionColor()) : Qt::NoBrush);
        painter.drawEllipse(tip, size * 0.35, size * 0.35);
    } else if (d->arrowType() == ArrowType::Tick) {
        painter.drawLine(QPointF(tip.x() - size * 0.35, tip.y() - size * 0.35),
                         QPointF(tip.x() + size * 0.35, tip.y() + size * 0.35));
    } else if (d->arrowType() == ArrowType::Open) {
        painter.drawLine(tip, left);
        painter.drawLine(tip, right);
    } else {
        QPolygonF poly;
        poly << tip << left << right;
        painter.setBrush(d->arrowFilled() ? QBrush(d->dimensionColor()) : Qt::NoBrush);
        painter.drawPolygon(poly);
    }
}

static QPen makeDimensionPen(const LineStyle& style, const QColor& color, bool isSelected)
{
    QPen pen(isSelected ? QColor("#F92672") : color, 1.0);
    pen.setCosmetic(true);
    switch (style.type) {
    case LineStyleType::Dashed:
        pen.setStyle(Qt::DashLine);
        break;
    case LineStyleType::DashDotThin:
    case LineStyleType::DashDotThick:
        pen.setStyle(Qt::DashDotLine);
        break;
    case LineStyleType::DashDotDot:
        pen.setStyle(Qt::DashDotDotLine);
        break;
    default:
        pen.setStyle(Qt::SolidLine);
        break;
    }
    return pen;
}

static std::pair<double, double> extendedFactorRange(double factor)
{
    return {std::min(0.0, factor), std::max(1.0, factor)};
}

static void drawTextAlongArc(QPainter& painter,
                             const QString& text,
                             const QFont& font,
                             const QColor& color,
                             const QPointF& centerScreen,
                             double radiusScreen,
                             double middleAngleDeg,
                             double tangentSign)
{
    if (text.isEmpty() || radiusScreen < 1.0) {
        return;
    }

    QFontMetricsF metrics(font);
    double totalAdvance = 0.0;
    for (const QChar ch : text) {
        totalAdvance += metrics.horizontalAdvance(ch);
    }
    if (totalAdvance <= 0.0) {
        return;
    }

    const double anglePerPixel = 180.0 / (M_PI * radiusScreen);
    double angleCursor = middleAngleDeg - tangentSign * totalAdvance * anglePerPixel * 0.5;

    painter.save();
    painter.setFont(font);
    painter.setPen(color);
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
        const QRectF rect(-advance * 0.5, -metrics.height() * 0.5, advance, metrics.height());
        painter.drawText(rect, Qt::AlignCenter, glyph);
        painter.restore();

        angleCursor += tangentSign * advance * anglePerPixel;
    }
    painter.restore();
}

void DimensionDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const
{
    auto* d = static_cast<Dimension*>(primitive);
    const double scale = std::abs(painter.transform().m11()) < 1e-9 ? 1.0 : std::abs(painter.transform().m11());
    const double inv = 1.0 / scale;
    QTransform worldTransform = painter.transform();
    Point ap = d->firstAnchor().resolve();
    Point bp = d->secondAnchor().resolve();
    QPointF a(ap.getX(), ap.getY());
    QPointF b(bp.getX(), bp.getY());
    QPointF linePoint(d->getLinePoint().getX(), d->getLinePoint().getY());

    QPen dimPen = makeDimensionPen(d->dimensionLineStyle(), d->dimensionColor(), isSelected);
    QPen extPen = makeDimensionPen(d->extensionLineStyle(), d->extensionColor(), isSelected);

    QPointF da = a;
    QPointF db = b;
    QPointF dimStart;
    QPointF dimEnd;
    double textAngleDeg = 0.0;
    const QString dimensionText = d->displayText();
    QFont textFont(d->fontFamily(), std::max(6, static_cast<int>(d->textHeight())));
    QFontMetricsF textMetrics(textFont);

    auto normalized = [](const QPointF& v) {
        const double len = std::hypot(v.x(), v.y());
        if (len < 1e-9) return QPointF(1.0, 0.0);
        return QPointF(v.x() / len, v.y() / len);
    };

    auto screenAngle = [&](const QPointF& worldVector) {
        QPointF s0 = worldTransform.map(QPointF(0, 0));
        QPointF s1 = worldTransform.map(worldVector);
        QPointF sv = s1 - s0;
        return qRadiansToDegrees(std::atan2(sv.y(), sv.x()));
    };

    auto drawGrip = [&](const QPointF& worldPoint, bool square) {
        if (!isSelected) return;
        QPointF sp = worldTransform.map(worldPoint);
        painter.save();
        painter.resetTransform();
        painter.setPen(QPen(QColor("#66D9EF"), 1.2));
        painter.setBrush(QColor("#1A1B26"));
        QRectF r(sp.x() - 4.0, sp.y() - 4.0, 8.0, 8.0);
        if (square) painter.drawRect(r);
        else painter.drawEllipse(r);
        painter.restore();
    };

    auto drawExtension = [&](const QPointF& source, const QPointF& target, const QPointF& normal) {
        QPointF n = normalized(normal);
        double sign = QPointF::dotProduct(target - source, n) >= 0 ? 1.0 : -1.0;
        QPointF overshoot = n * (sign * d->extensionOvershoot());
        painter.drawLine(source, target + overshoot);
    };

    bool drawCurvedAngularText = false;
    QPointF arcTextCenterScreen;
    double arcTextRadiusScreen = 0.0;
    double arcTextAngleDeg = 0.0;
    double arcTextTangentSign = 1.0;

    if (d->getDimensionType() == DimensionType::Radius || d->getDimensionType() == DimensionType::Diameter) {
        double r = d->measuredValue();
        if (d->getDimensionType() == DimensionType::Diameter) r *= 0.5;
        QPointF dir = normalized(linePoint - a);
        if (std::hypot(linePoint.x() - a.x(), linePoint.y() - a.y()) < 1e-9) {
            dir = normalized(b - a);
        }
        QPointF edge1 = a + dir * r;
        QPointF edge2 = a - dir * r;
        const auto [startFactor, endFactor] = extendedFactorRange(d->textPositionFactor());
        const QPointF radialStart = (d->getDimensionType() == DimensionType::Diameter) ? edge2 : a;
        const QPointF radialEnd = edge1;
        QPointF lineVec = radialEnd - radialStart;
        const double lineLen = std::hypot(lineVec.x(), lineVec.y());
        double previewStartFactor = startFactor;
        double previewEndFactor = endFactor;
        if (lineLen > 1e-9) {
            const double padFactor = ((textMetrics.horizontalAdvance(dimensionText) * 0.5) + 4.0) * inv / lineLen;
            if (d->textPositionFactor() < 0.0) previewStartFactor -= padFactor;
            if (d->textPositionFactor() > 1.0) previewEndFactor += padFactor;
        }
        const QPointF lineStart = radialStart + lineVec * previewStartFactor;
        const QPointF lineEnd = radialStart + lineVec * previewEndFactor;

        painter.setPen(dimPen);
        if (d->getDimensionType() == DimensionType::Radius) {
            painter.drawLine(lineStart, lineEnd);
            double arrowAngle = std::atan2(a.y() - edge1.y(), a.x() - edge1.x());
            if (d->arrowPlacement() == ArrowPlacement::Inside) arrowAngle += M_PI;
            drawArrowHead(painter, edge1, arrowAngle, d, inv);
            textAngleDeg = screenAngle(lineEnd - lineStart);
        } else {
            painter.drawLine(lineStart, lineEnd);
            const bool outside = d->arrowPlacement() == ArrowPlacement::Inside;
            drawArrowHead(painter, edge2, std::atan2(edge1.y() - edge2.y(), edge1.x() - edge2.x()) + (outside ? M_PI : 0.0), d, inv);
            drawArrowHead(painter, edge1, std::atan2(edge2.y() - edge1.y(), edge2.x() - edge1.x()) + (outside ? M_PI : 0.0), d, inv);
            textAngleDeg = screenAngle(lineEnd - lineStart);
        }
        drawGrip(a, false);
        drawGrip(linePoint, true);
    } else if (d->getDimensionType() == DimensionType::Angular) {
        const double a1 = std::atan2(a.y() - linePoint.y(), a.x() - linePoint.x());
        const double a2 = std::atan2(b.y() - linePoint.y(), b.x() - linePoint.x());
        double delta = std::fmod(a2 - a1, 2.0 * M_PI);
        if (delta > M_PI) delta -= 2.0 * M_PI;
        if (delta < -M_PI) delta += 2.0 * M_PI;
        if (d->useSupplementaryAngle() && std::abs(delta) > 1e-9) {
            delta = delta > 0.0 ? delta - 2.0 * M_PI : delta + 2.0 * M_PI;
        }
        const double r = d->angularRadius() > 1e-9
            ? d->angularRadius()
            : std::max(15.0, std::min(std::hypot(a.x() - linePoint.x(), a.y() - linePoint.y()),
                                      std::hypot(b.x() - linePoint.x(), b.y() - linePoint.y())) * 0.65);
        QPointF arcA(linePoint.x() + std::cos(a1) * r, linePoint.y() + std::sin(a1) * r);
        QPointF arcB(linePoint.x() + std::cos(a1 + delta) * r, linePoint.y() + std::sin(a1 + delta) * r);

        painter.setPen(extPen);
        auto drawAngularExtension = [&](double angle, const QPointF& source) {
            QPointF dir(std::cos(angle), std::sin(angle));
            double sourceRadius = std::hypot(source.x() - linePoint.x(), source.y() - linePoint.y());
            double endRadius = r + d->extensionOvershoot();
            if (endRadius < sourceRadius) {
                std::swap(sourceRadius, endRadius);
            }
            QPointF from(linePoint.x() + dir.x() * sourceRadius, linePoint.y() + dir.y() * sourceRadius);
            QPointF to(linePoint.x() + dir.x() * endRadius, linePoint.y() + dir.y() * endRadius);
            painter.drawLine(from, to);
        };
        drawAngularExtension(a1, a);
        drawAngularExtension(a1 + delta, b);

        painter.setPen(dimPen);
        const auto [startFactor, endFactor] = extendedFactorRange(d->textPositionFactor());
        double arcStart = a1 + delta * startFactor;
        double arcSpan = delta * (endFactor - startFactor);
        const QPointF textPosWorld(d->getTextPosition().getX(), d->getTextPosition().getY());
        arcTextCenterScreen = worldTransform.map(linePoint);
        QPointF textPosScreen = worldTransform.map(textPosWorld);
        arcTextRadiusScreen = std::hypot(textPosScreen.x() - arcTextCenterScreen.x(), textPosScreen.y() - arcTextCenterScreen.y());
        if (arcTextRadiusScreen > 1e-9) {
            const double padAngle = (textMetrics.horizontalAdvance(dimensionText) * 0.5 + 4.0) / arcTextRadiusScreen;
            if (d->textPositionFactor() < 0.0) {
                arcStart -= delta >= 0.0 ? padAngle : -padAngle;
                arcSpan += delta >= 0.0 ? padAngle : -padAngle;
            }
            if (d->textPositionFactor() > 1.0) {
                arcSpan += delta >= 0.0 ? padAngle : -padAngle;
            }
        }
        QRectF rect(linePoint.x() - r, linePoint.y() - r, r * 2.0, r * 2.0);
        painter.drawArc(rect, int(-qRadiansToDegrees(arcStart) * 16.0), int(-qRadiansToDegrees(arcSpan) * 16.0));
        const double tangentSign = delta >= 0 ? 1.0 : -1.0;
        drawArrowHead(painter, arcA, a1 + tangentSign * M_PI / 2.0, d, inv);
        drawArrowHead(painter, arcB, a1 + delta - tangentSign * M_PI / 2.0, d, inv);
        const double textAngle = a1 + delta * d->textPositionFactor();
        arcTextAngleDeg = qRadiansToDegrees(textAngle);
        arcTextTangentSign = tangentSign;
        drawCurvedAngularText = true;
        textAngleDeg = screenAngle(QPointF(std::cos(textAngle + tangentSign * M_PI / 2.0), std::sin(textAngle + tangentSign * M_PI / 2.0)));
        drawGrip(linePoint, false);
        drawGrip(a, true);
        drawGrip(b, true);
    } else {
    if (d->getDimensionType() == DimensionType::Horizontal) {
        da = QPointF(a.x(), linePoint.y());
        db = QPointF(b.x(), linePoint.y());
        textAngleDeg = screenAngle(db - da);
    } else if (d->getDimensionType() == DimensionType::Vertical) {
        da = QPointF(linePoint.x(), a.y());
        db = QPointF(linePoint.x(), b.y());
        textAngleDeg = -90.0;
    } else {
        const double vx = b.x() - a.x();
        const double vy = b.y() - a.y();
        const double len = std::hypot(vx, vy);
        if (len > 1e-9) {
            const double nx = -vy / len;
            const double ny = vx / len;
            const double off = (linePoint.x() - a.x()) * nx + (linePoint.y() - a.y()) * ny;
            da = QPointF(a.x() + nx * off, a.y() + ny * off);
            db = QPointF(b.x() + nx * off, b.y() + ny * off);
        }
        textAngleDeg = screenAngle(db - da);
    }

    QPointF u = normalized(db - da);
    QPointF n(-u.y(), u.x());
    const auto [startFactor, endFactor] = extendedFactorRange(d->textPositionFactor());
    double adjustedStartFactor = startFactor;
    double adjustedEndFactor = endFactor;
    const double baseLen = std::hypot(db.x() - da.x(), db.y() - da.y());
    if (baseLen > 1e-9) {
        const double padFactor = ((textMetrics.horizontalAdvance(dimensionText) * 0.5) + 4.0) * inv / baseLen;
        if (d->textPositionFactor() < 0.0) adjustedStartFactor -= padFactor;
        if (d->textPositionFactor() > 1.0) adjustedEndFactor += padFactor;
    }
    dimStart = da + (db - da) * adjustedStartFactor - u * d->dimensionExtension();
    dimEnd = da + (db - da) * adjustedEndFactor + u * d->dimensionExtension();

    painter.setPen(extPen);
    drawExtension(a, da, n);
    drawExtension(b, db, n);

    painter.setPen(dimPen);
    painter.drawLine(dimStart, dimEnd);

    const double angle = std::atan2(db.y() - da.y(), db.x() - da.x());
    const bool outside = d->arrowPlacement() == ArrowPlacement::Inside;
    drawArrowHead(painter, da, angle + (outside ? M_PI : 0.0), d, inv);
    drawArrowHead(painter, db, angle + M_PI + (outside ? M_PI : 0.0), d, inv);
    drawGrip(a, true);
    drawGrip(b, true);
    Point gp = d->getLineGripPosition();
    drawGrip(QPointF(gp.getX(), gp.getY()), false);
    }

    QPointF textPos(d->getTextPosition().getX(), d->getTextPosition().getY());
    while (textAngleDeg > 180.0) textAngleDeg -= 360.0;
    while (textAngleDeg < -180.0) textAngleDeg += 360.0;
    if (textAngleDeg > 90.0) textAngleDeg -= 180.0;
    if (textAngleDeg < -90.0) textAngleDeg += 180.0;

    QPointF sp = worldTransform.map(textPos);
    painter.save();
    painter.resetTransform();
    QFont font = textFont;
    if (drawCurvedAngularText) {
        drawTextAlongArc(painter,
                         dimensionText,
                         font,
                         isSelected ? QColor("#F92672") : d->textColor(),
                         arcTextCenterScreen,
                         arcTextRadiusScreen,
                         arcTextAngleDeg,
                         arcTextTangentSign);
    } else {
        painter.setFont(font);
        painter.setPen(isSelected ? QColor("#F92672") : d->textColor());
        painter.translate(sp);
        painter.rotate(textAngleDeg);
        QRectF textRect(-70, -14, 140, 28);
        painter.drawText(textRect, Qt::AlignCenter, dimensionText);
    }
    painter.restore();
}
