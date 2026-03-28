#include "PrimitiveStrategies.h"
#include "GlobalSettings.h"
#include <QPainter>
#include <QPainterPath>
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

    QPainterPath path;
    path.moveTo(points[0].getX(), points[0].getY());
    
    // Smooth spline using cubic beziers (Catmull-Rom spline conversion)
    if (points.size() >= 2) {
        for (size_t i = 0; i < points.size() - 1; ++i) {
            Point p0 = (i == 0) ? points[0] : points[i-1];
            Point p1 = points[i];
            Point p2 = points[i+1];
            Point p3 = (i + 2 < points.size()) ? points[i+2] : p2;

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
