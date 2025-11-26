#include "Viewport.h"
#include "Scene.h"
#include "Point.h"
#include "Draw.h"
#include "Camera.h"
#include "ContextMenu.h"
#include "Segment.h" // Нужен для получения BoundingBox (в будущем лучше вынести в интерфейс Object)

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QGridLayout>
#include <QtMath> // Для qSqrt

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
    m_infoLabel->setFixedSize(120, 80); // Чуть увеличил для доп. инфо
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_infoLabel->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 150); border-radius: 5px; padding: 5px;");

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->addWidget(m_infoLabel, 1, 1, Qt::AlignBottom | Qt::AlignRight);
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);

    updateInfoLabel();
}

Viewport::~Viewport()
{
    // m_camera и m_contextMenu удалятся автоматически, так как Viewport их родитель
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

    // 2. Рисуем сетку (передаем трансформацию, чтобы сетка знала границы мира)
    drawGrid(painter, worldToScreen);

    // 3. Рисуем Гизмо (он рисуется поверх, в углу)
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
    // Логика отрисовки сетки из ClarusCAD, адаптированная под UniversityCAD
    QPen gridPen(QColor(50, 52, 71), 1.0); // Просто линии, без точек для скорости
    QPen axisXPen(QColor("#F92672"), 1.5);
    QPen axisYPen(QColor("#66D9EF"), 1.5);

    painter.save();
    painter.setTransform(transform); // Работаем в мировых координатах

    // Определяем видимые границы мира
    QTransform screenToWorldTf = transform.inverted();
    QRectF visibleWorldRect = screenToWorldTf.mapRect(rect());

    double step = calculateDynamicGridStep();

    // Вычисляем границы для циклов
    double startX = std::floor(visibleWorldRect.left() / step) * step;
    double endX = std::ceil(visibleWorldRect.right() / step) * step;
    double startY = std::floor(visibleWorldRect.top() / step) * step;
    double endY = std::ceil(visibleWorldRect.bottom() / step) * step;

    // Вертикальные линии
    for (double x = startX; x <= endX; x += step) {
        if (std::abs(x) < 1e-9) painter.setPen(axisYPen); // Ось Y
        else painter.setPen(gridPen);
        painter.drawLine(QPointF(x, startY), QPointF(x, endY));
    }

    // Горизонтальные линии
    for (double y = startY; y <= endY; y += step) {
        if (std::abs(y) < 1e-9) painter.setPen(axisXPen); // Ось X
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
    // Гизмо теперь вращается вместе с камерой!
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    int size = 40;
    int padding = 50;
    QPoint origin(padding, height() - padding);

    // Создаем матрицу для вращения стрелок гизмо
    QTransform gizmoTransform;
    gizmoTransform.translate(origin.x(), origin.y());
    // Вращаем в обратную сторону от камеры, чтобы показать ориентацию осей
    gizmoTransform.rotate(-m_camera->getRotationAngle());
    // Учитываем, что Y на экране вниз
    gizmoTransform.scale(1, -1);

    // Настраиваем кисти
    QPen axisXPen(QColor("#F92672"), 2.0);
    QPen axisYPen(QColor("#66D9EF"), 2.0);

    // --- Ось X ---
    painter.setTransform(gizmoTransform);
    painter.setPen(axisXPen);
    painter.setBrush(QColor("#F92672"));
    painter.drawLine(0, 0, size, 0);

    // Стрелка X
    QPolygonF xArrow;
    xArrow << QPointF(size, 0) << QPointF(size - 8, 4) << QPointF(size - 8, -4);
    painter.drawPolygon(xArrow);

    // Текст X (отменяем трансформацию для текста, чтобы он не был перевернут)
    painter.resetTransform();
    QPointF xPos = gizmoTransform.map(QPointF(size + 10, 0));
    painter.setPen(Qt::white);
    painter.drawText(xPos, "X");

    // --- Ось Y ---
    painter.setTransform(gizmoTransform);
    painter.setPen(axisYPen);
    painter.setBrush(QColor("#66D9EF"));
    painter.drawLine(0, 0, 0, size);

    // Стрелка Y
    QPolygonF yArrow;
    yArrow << QPointF(0, size) << QPointF(-4, size - 8) << QPointF(4, size - 8);
    painter.drawPolygon(yArrow);

    // Текст Y
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
    // Панорамирование по средней кнопке
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    // Проверка клика по Гизмо для поворота (левый клик)
    else if (event->button() == Qt::LeftButton) {
        if (getGizmoRect().contains(event->pos())) {
            m_camera->rotateLeft(); // Быстрый поворот кликом по гизмо
        }
    }
    // Контекстное меню вызывается автоматически через customContextMenuRequested
}

void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    m_currentMouseWorldPos = screenToWorld(event->position());
    updateInfoLabel();

    // Панорамирование делегируем камере
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        m_camera->pan(delta); // Камера сама вызовет update() через сигнал
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
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void Viewport::wheelEvent(QWheelEvent *event)
{
    // Зум через камеру
    double factor = 1.0 + (event->angleDelta().y() / 8.0) / 100.0;
    m_camera->applyZoom(factor, event->position().toPoint());
}

void Viewport::resizeEvent(QResizeEvent *event)
{
    m_camera->setCanvasSize(event->size());
    QWidget::resizeEvent(event);
}

// Показать контекстное меню
void Viewport::showContextMenu(const QPoint& pos)
{
    m_contextMenu->exec(mapToGlobal(pos));
}

// Слот обновления от камеры
void Viewport::onCameraUpdated()
{
    updateInfoLabel();
    update(); // Перерисовка виджета
}

// Реализация слотов навигации
void Viewport::zoomIn() { m_camera->applyZoom(1.25, rect().center()); }
void Viewport::zoomOut() { m_camera->applyZoom(0.8, rect().center()); }
void Viewport::rotateLeft() { m_camera->rotateLeft(); }
void Viewport::rotateRight() { m_camera->rotateRight(); }

void Viewport::zoomToExtents()
{
    if (!m_scene) return;

    // Рассчитываем границы всех объектов
    QRectF worldBounds;
    bool first = true;

    // Для этого нам нужно пройтись по примитивам
    // Внимание: В UniversityCAD примитивы могут не иметь метода getBoundingBox() в базовом классе Object,
    // но в предоставленных файлах в Segment.h/cpp он тоже не реализован явно (там только getStart/getEnd).
    // Поэтому реализуем расчет здесь вручную для Segment.

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

    if (!first) { // Если были объекты
        m_camera->fitBounds(worldBounds);
    }
}

// Преобразования координат через камеру
QPointF Viewport::worldToScreen(const QPointF& worldPos) const {
    return m_camera->getWorldToScreenTransform().map(worldPos);
}

QPointF Viewport::screenToWorld(const QPointF& screenPos) const {
    return m_camera->getScreenToWorldTransform().map(screenPos);
}

QRect Viewport::getGizmoRect() const {
    // Область клика 60x60 в левом нижнем углу
    return QRect(0, height() - 70, 70, 70);
}

// Привязка к сетке теперь учитывает динамический шаг
QPointF Viewport::getSnappedPoint(const QPointF& mousePos) const
{
    // Преобразуем мышь в мир
    QPointF worldPos = screenToWorld(mousePos);

    // Считаем динамический шаг (как в ClarusCAD)
    double step = calculateDynamicGridStep();

    double x = std::round(worldPos.x() / step) * step;
    double y = std::round(worldPos.y() / step) * step;

    return QPointF(x, y);
}

double Viewport::calculateDynamicGridStep() const
{
    double zoom = m_camera->getZoomFactor();
    double step = m_gridStep;

    // Адаптация шага (алгоритм из ClarusCAD)
    while (step * zoom < 15) step *= 2;
    while (step * zoom > 150) step /= 2;

    return step;
}

// Обновление инфо-лейбла с учетом зума и угла
void Viewport::updateInfoLabel()
{
    QString infoText;
    // ... (логика формирования текста координат такая же) ...
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

    // Добавляем инфо о зуме и угле из камеры
    infoText += QString("\nZoom: %1%").arg(static_cast<int>(m_camera->getZoomFactor() * 100));
    infoText += QString("\nAngle: %1°").arg(static_cast<int>(m_camera->getRotationAngle()));

    m_infoLabel->setText(infoText);
}

// Остальные сеттеры
void Viewport::setScene(Scene* scene) { m_scene = scene; }
void Viewport::setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies) { m_drawingStrategies = strategies; }
void Viewport::setGridStep(int step) { if (step > 0) { m_gridStep = step; update(); } }
void Viewport::setCoordinateSystem(CoordinateSystemType type) { m_coordSystemType = type; updateInfoLabel(); }
void Viewport::setSelectedObject(Object* obj) { m_selectedObject = obj; update(); }
void Viewport::update() { QWidget::update(); }
