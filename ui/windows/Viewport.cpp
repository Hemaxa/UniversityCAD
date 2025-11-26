#include "Viewport.h"
#include "Scene.h"
#include "Point.h"
#include "Draw.h"
#include "Camera.h"
#include "ContextMenu.h"
#include "Segment.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QGridLayout>
#include <QtMath>

Viewport::Viewport(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // --- Инициализация камеры ---
    m_camera = new Camera(this);
    connect(m_camera, &Camera::updated, this, &Viewport::onCameraUpdated);

    // --- Инициализация контекстного меню ---
    m_contextMenu = new ContextMenu(this);

    // Связываем сигналы меню со слотами Viewport
    connect(m_contextMenu, &ContextMenu::zoomInTriggered, this, &Viewport::zoomIn);
    connect(m_contextMenu, &ContextMenu::zoomOutTriggered, this, &Viewport::zoomOut);
    connect(m_contextMenu, &ContextMenu::zoomExtentsTriggered, this, &Viewport::zoomToExtents);
    connect(m_contextMenu, &ContextMenu::rotateLeftTriggered, this, &Viewport::rotateLeft);
    connect(m_contextMenu, &ContextMenu::rotateRightTriggered, this, &Viewport::rotateRight);

    // Установка политики контекстного меню
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &Viewport::showContextMenu);

    // UI Инфо-панели
    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName("InfoLabel");
    m_infoLabel->setFixedSize(120, 80);
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Цвет текста #A6E22E (салатовый)
    m_infoLabel->setStyleSheet("color: #A6E22E; background-color: rgba(18, 18, 18, 0.85); border-radius: 5px; padding: 5px;");

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->addWidget(m_infoLabel, 1, 1, Qt::AlignBottom | Qt::AlignRight);
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);

    updateInfoLabel();
}

Viewport::~Viewport()
{
}

void Viewport::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Заливка фона
    painter.fillRect(rect(), QColor("#1A1B26"));

    // 1. Получаем матрицу трансформации от камеры
    QTransform worldToScreen = m_camera->getWorldToScreenTransform();

    // 2. Рисуем сетку
    drawGrid(painter, worldToScreen);

    // 3. Рисуем Гизмо
    drawGizmo(painter);

    if (!m_scene || !m_drawingStrategies) return;

    // 4. Применяем трансформацию камеры к painter для отрисовки объектов
    painter.save();
    painter.setTransform(worldToScreen);

    // Отрисовка примитивов
    for (const auto& primitive : m_scene->getPrimitives()) {
        auto it = m_drawingStrategies->find(primitive->getType());
        if (it != m_drawingStrategies->end()) {
            bool isSelected = (primitive.get() == m_selectedObject);
            it->second->draw(painter, primitive.get(), isSelected);
        }
    }
    painter.restore();
}

void Viewport::drawGrid(QPainter& painter, const QTransform& transform)
{
    QPen gridPen(QColor(50, 52, 71), 1.0);
    QPen axisXPen(QColor("#F92672"), 1.5);
    QPen axisYPen(QColor("#66D9EF"), 1.5);

    painter.save();
    painter.setTransform(transform);

    QTransform screenToWorldTf = transform.inverted();
    QRectF visibleWorldRect = screenToWorldTf.mapRect(rect());

    double step = calculateDynamicGridStep();

    double startX = std::floor(visibleWorldRect.left() / step) * step;
    double endX = std::ceil(visibleWorldRect.right() / step) * step;
    double startY = std::floor(visibleWorldRect.top() / step) * step;
    double endY = std::ceil(visibleWorldRect.bottom() / step) * step;

    // Вертикальные линии
    for (double x = startX; x <= endX; x += step) {
        if (std::abs(x) < 1e-9) painter.setPen(axisYPen);
        else painter.setPen(gridPen);
        painter.drawLine(QPointF(x, startY), QPointF(x, endY));
    }

    // Горизонтальные линии
    for (double y = startY; y <= endY; y += step) {
        if (std::abs(y) < 1e-9) painter.setPen(axisXPen);
        else painter.setPen(gridPen);
        painter.drawLine(QPointF(startX, y), QPointF(endX, y));
    }

    // Рисуем точку начала координат
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(QPointF(0,0), 3.0/m_camera->getZoomFactor(), 3.0/m_camera->getZoomFactor());

    painter.restore();
}

void Viewport::drawGizmo(QPainter& painter)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    int size = 40;
    int padding = 50;
    QPoint origin(padding, height() - padding);

    QTransform gizmoTransform;
    gizmoTransform.translate(origin.x(), origin.y());
    gizmoTransform.rotate(-m_camera->getRotationAngle());
    gizmoTransform.scale(1, -1);

    QPen axisXPen(QColor("#F92672"), 2.0);
    QPen axisYPen(QColor("#66D9EF"), 2.0);

    // --- Ось X ---
    painter.setTransform(gizmoTransform);
    painter.setPen(axisXPen);
    painter.setBrush(QColor("#F92672"));
    painter.drawLine(0, 0, size, 0);

    QPolygonF xArrow;
    xArrow << QPointF(size, 0) << QPointF(size - 8, 4) << QPointF(size - 8, -4);
    painter.drawPolygon(xArrow);

    painter.resetTransform();
    QPointF xPos = gizmoTransform.map(QPointF(size + 10, 0));
    painter.setPen(Qt::white);
    painter.drawText(xPos, "X");

    // --- Ось Y ---
    painter.setTransform(gizmoTransform);
    painter.setPen(axisYPen);
    painter.setBrush(QColor("#66D9EF"));
    painter.drawLine(0, 0, 0, size);

    QPolygonF yArrow;
    yArrow << QPointF(0, size) << QPointF(-4, size - 8) << QPointF(4, size - 8);
    painter.drawPolygon(yArrow);

    painter.resetTransform();
    QPointF yPos = gizmoTransform.map(QPointF(0, size + 10));
    painter.setPen(Qt::white);
    painter.drawText(yPos, "Y");

    // Центр
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    painter.drawEllipse(origin, 3, 3);

    painter.restore();
}

void Viewport::mousePressEvent(QMouseEvent *event)
{
    // ЛКМ используется и для Гизмо, и для Панорамирования
    if (event->button() == Qt::LeftButton) {
        if (getGizmoRect().contains(event->pos())) {
            m_camera->rotateLeft(); // Быстрый поворот кликом по гизмо
            return;
        }

        // Панорамирование
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    m_currentMouseWorldPos = screenToWorld(event->position());
    updateInfoLabel();

    // Панорамирование
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        m_camera->pan(delta);
    }

    // Смена курсора над гизмо
    if (!m_isPanning && getGizmoRect().contains(event->pos())) {
        setCursor(Qt::PointingHandCursor);
    } else if (!m_isPanning) {
        setCursor(Qt::ArrowCursor);
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    // Отпускание ЛКМ завершает панорамирование
    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void Viewport::wheelEvent(QWheelEvent *event)
{
    double factor = 1.0 + (event->angleDelta().y() / 8.0) / 100.0;
    m_camera->applyZoom(factor, event->position().toPoint());
}

void Viewport::resizeEvent(QResizeEvent *event)
{
    m_camera->setCanvasSize(event->size());
    QWidget::resizeEvent(event);
}

void Viewport::showContextMenu(const QPoint& pos)
{
    m_contextMenu->exec(mapToGlobal(pos));
}

void Viewport::onCameraUpdated()
{
    updateInfoLabel();
    update();
}

// Логика аддитивного зума
void Viewport::zoomIn()
{
    double increment = m_zoomStep - 1.0;
    double currentZoom = m_camera->getZoomFactor();
    double targetZoom = currentZoom + increment;
    double factor = targetZoom / currentZoom;
    m_camera->applyZoom(factor, rect().center());
}

void Viewport::zoomOut()
{
    double decrement = m_zoomStep - 1.0;
    double currentZoom = m_camera->getZoomFactor();
    double targetZoom = currentZoom - decrement;
    if (targetZoom < 0.05) targetZoom = 0.05;
    double factor = targetZoom / currentZoom;
    m_camera->applyZoom(factor, rect().center());
}

void Viewport::rotateLeft() { m_camera->rotateLeft(); }
void Viewport::rotateRight() { m_camera->rotateRight(); }

void Viewport::zoomToExtents()
{
    if (!m_scene) return;

    QRectF worldBounds;
    bool first = true;

    for (const auto& primitive : m_scene->getPrimitives()) {
        if (primitive->getType() == PrimitiveType::Segment) {
            auto* s = static_cast<Segment*>(primitive.get());
            qreal minX = std::min(s->getStart().getX(), s->getEnd().getX());
            qreal maxX = std::max(s->getStart().getX(), s->getEnd().getX());
            qreal minY = std::min(s->getStart().getY(), s->getEnd().getY());
            qreal maxY = std::max(s->getStart().getY(), s->getEnd().getY());

            QRectF itemRect(QPointF(minX, minY), QPointF(maxX, maxY));

            if (first) { worldBounds = itemRect; first = false; }
            else { worldBounds = worldBounds.united(itemRect); }
        }
    }

    if (!first) {
        m_camera->fitBounds(worldBounds);
    }
}

QPointF Viewport::worldToScreen(const QPointF& worldPos) const {
    return m_camera->getWorldToScreenTransform().map(worldPos);
}

QPointF Viewport::screenToWorld(const QPointF& screenPos) const {
    return m_camera->getScreenToWorldTransform().map(screenPos);
}

QRect Viewport::getGizmoRect() const {
    return QRect(0, height() - 70, 70, 70);
}

QPointF Viewport::getSnappedPoint(const QPointF& mousePos) const
{
    QPointF worldPos = screenToWorld(mousePos);
    double step = calculateDynamicGridStep();
    double x = std::round(worldPos.x() / step) * step;
    double y = std::round(worldPos.y() / step) * step;
    return QPointF(x, y);
}

double Viewport::calculateDynamicGridStep() const
{
    double zoom = m_camera->getZoomFactor();
    double step = m_gridStep;
    while (step * zoom < 15) step *= 2;
    while (step * zoom > 150) step /= 2;
    return step;
}

void Viewport::updateInfoLabel()
{
    QString infoText;
    if (m_coordSystemType == CoordinateSystemType::Cartesian) {
        infoText = QString("X: %1\nY: %2")
        .arg(m_currentMouseWorldPos.x(), 0, 'f', 2)
            .arg(m_currentMouseWorldPos.y(), 0, 'f', 2);
    } else {
        Point p(m_currentMouseWorldPos.x(), m_currentMouseWorldPos.y());
        QString angleUnit = (Point::getAngleUnit() == AngleUnit::Degrees) ? "°" : " rad";
        infoText = QString("R: %1\nA: %2%3")
                       .arg(p.getRadius(), 0, 'f', 2)
                       .arg(p.getAngle(), 0, 'f', 2)
                       .arg(angleUnit);
    }

    // Округление процентов зума
    infoText += QString("\nZoom: %1%").arg(qRound(m_camera->getZoomFactor() * 100));

    // НОРМАЛИЗАЦИЯ УГЛА: Приводим к диапазону [0, 360)
    int angle = qRound(m_camera->getRotationAngle()) % 360;
    if (angle < 0) angle += 360;
    infoText += QString("\nAngle: %1°").arg(angle);

    m_infoLabel->setText(infoText);
}

void Viewport::setScene(Scene* scene) { m_scene = scene; }
void Viewport::setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies) { m_drawingStrategies = strategies; }
void Viewport::setGridStep(int step) { if (step > 0) { m_gridStep = step; update(); } }
void Viewport::setZoomStep(double step) { if (step > 0) { m_zoomStep = step; } }
void Viewport::setCoordinateSystem(CoordinateSystemType type) { m_coordSystemType = type; updateInfoLabel(); }
void Viewport::setSelectedObject(Object* obj) { m_selectedObject = obj; update(); }
void Viewport::update() { QWidget::update(); }
