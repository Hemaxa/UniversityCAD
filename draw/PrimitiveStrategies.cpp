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

static void drawStyledEllipse(QPainter& painter, const QPointF& center, double rx, double ry, const Object* obj) {
    LineStyleType type = obj->getLineStyle().type;

    // Если стиль требует сложной геометрии (волна/зигзаг), делаем через путь
    if (type == LineStyleType::SolidWavy || type == LineStyleType::SolidZigZag) {
        QPainterPath path;
        path.addEllipse(center, rx, ry);

        // Тут сложнее: createWavyPath работает для отрезков.
        // Для окружности нужно семплировать точки или использовать упрощение.
        // Для простоты пока оставим стандартную отрисовку для волн на кругах,
        // так как "развернуть" волну по кругу математически затратно для этого примера.
        // НО, чтобы сработал dashed/dotted QPen, нужно убедиться что мы не используем drawPath(..., wavy).
        // Стандартный dashed работает в painter.drawEllipse.
        painter.drawEllipse(center, rx, ry);
    } else {
        // Обычные типы (Solid, Dashed, etc) отлично рисуются стандартным методом с настроенным Pen
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

void CircleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Circle*>(primitive);
    setupPen(painter, obj, isSelected); // setupPen настроит QPen (включая Dashed паттерны)
    // drawEllipse корректно использует QPen, проблема была возможно в том, что пользователь ожидал Wavy на круге,
    // либо в том, что setupPen неправильно обрабатывал типы.
    // Если проблема была в том, что "другие типы линий" (пунктир) не применялись, то drawEllipse это исправит,
    // при условии что setupPen вызывается.
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()),
                        obj->getRadius(), obj->getRadius());
}

void ArcDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Arc*>(primitive);
    setupPen(painter, obj, isSelected);
    double r = obj->getRadius();
    QRectF rect(obj->getCenter().getX() - r, obj->getCenter().getY() - r, r * 2, r * 2);
    // drawArc принимает углы в 1/16 градуса
    painter.drawArc(rect, int(obj->getStartAngle() * 16), int(obj->getSpanAngle() * 16));
}

void RectangleDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Rectangle*>(primitive);
    setupPen(painter, obj, isSelected);

    // Rectangle хранит TopLeft (Min X, Max Y).
    // QPainter drawRect(x,y,w,h) рисует от x,y вправо и ВНИЗ (в экранных координатах).
    // Если Camera инвертирует Y (scale 1, -1), то +Y экрана это -Y мира.
    // Чтобы нарисовать прямоугольник, который в мире имеет высоту H (вверх),
    // нам нужно знать, как QPainter обрабатывает высоту.

    // Надежнее всего нарисовать явно через координаты, чтобы не зависеть от знака высоты
    double minX = obj->getTopLeft().getX();
    double maxY = obj->getTopLeft().getY();
    double w = obj->getWidth();
    double h = obj->getHeight();

    // Прямоугольник от (minX, maxY) вниз на h (до maxY - h).
    // В мире: TopLeft = (minX, maxY). BottomRight = (minX+w, maxY-h).

    // Вариант 1: Использовать QRectF(TopLeft, Size) и надеяться на трансформацию.
    // Если scale(1, -1), то точка (0, 10) на экране (0, -10).
    // Если мы скажем drawRect(0, 10, 10, 10).
    // Это прямоугольник от (0,10) до (10, 20) в локальных координатах Pen.
    // После трансф: Y инвертируется.

    // Проще: задаем прямоугольник через верхний-левый и размеры так, чтобы он соответствовал математике.
    // Если Rectangle::m_topLeft это "Верхний Левый" в мире (Max Y), то
    // чтобы нарисовать его вниз (к Min Y), нужно использовать отрицательную высоту или сдвигать Y.

    // Исправление бага "появляется выше":
    // Мы рисуем от TopLeft.
    // Если мы используем drawRect(x, y, w, h) -> это рисует в сторону увеличения Y координат системы QPainter.
    // В мире (где Y вверх) увеличение Y - это вверх.
    // Значит drawRect(x,y,w,h) нарисует прямоугольник ВВЕРХ от точки x,y.
    // А наш Rectangle хранит ВЕРХНЮЮ точку. Значит нам надо рисовать ВНИЗ.

    painter.drawRoundedRect(QRectF(minX, maxY - h, w, h), // Рисуем от нижней точки вверх
                            obj->getCornerRadius(), obj->getCornerRadius());
}

void EllipseDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Ellipse*>(primitive);
    setupPen(painter, obj, isSelected);
    painter.drawEllipse(QPointF(obj->getCenter().getX(), obj->getCenter().getY()),
                        obj->getRadiusX(), obj->getRadiusY());
}

void PolygonDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<PolygonObj*>(primitive);
    setupPen(painter, obj, isSelected);

    QPolygonF poly;
    int sides = std::max(3, obj->getSides());
    double step = 2 * M_PI / sides;
    double startAngle = M_PI / 2;
    double r = obj->getRadius();

    // Если не вписанный, корректируем радиус, чтобы он был по грани
    if (!obj->isInscribed()) r = r / std::cos(M_PI / sides);

    for (int i = 0; i < sides; ++i) {
        double angle = startAngle + i * step;
        poly << QPointF(obj->getCenter().getX() + r * std::cos(angle),
                        obj->getCenter().getY() + r * std::sin(angle));
    }
    painter.drawPolygon(poly);
}

void SplineDraw::draw(QPainter& painter, Object* primitive, bool isSelected) const {
    auto* obj = static_cast<Spline*>(primitive);
    const auto& points = obj->getPoints();
    if (points.size() < 2) return;

    setupPen(painter, obj, isSelected);

    QPainterPath path;
    path.moveTo(points[0].getX(), points[0].getY());
    for (size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i].getX(), points[i].getY());
    }
    painter.drawPath(path);

    // Рисуем опорные точки, если объект выделен
    if (isSelected) {
        painter.setBrush(QColor("#F92672"));
        painter.setPen(Qt::NoPen);
        double s = 6.0 / getScale(painter); // Размер точек не зависит от зума
        for(const auto& p : points) {
            painter.drawEllipse(QPointF(p.getX(), p.getY()), s/2, s/2);
        }
    }
}
