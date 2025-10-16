#include "Viewport.h"
#include "Scene.h"
#include "Point.h"
#include "Draw.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QGridLayout>
#include <cmath>

// Конструктор виджета Viewport.
Viewport::Viewport(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    m_panOffset = QPointF(width() / 2.0, height() / 2.0);

    // Создание и настройка инфо-панели.
    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName("InfoLabel");
    m_infoLabel->setFixedSize(100, 70);
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Размещение инфо-панели в правом нижнем углу.
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->addWidget(m_infoLabel, 1, 1, Qt::AlignBottom | Qt::AlignRight);
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);

    updateInfoLabel();
}

// Главный метод отрисовки виджета.
void Viewport::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect(), QColor("#1A1B26"));

    drawGrid(painter);
    drawGizmo(painter);

    if (!m_scene || !m_drawingStrategies) return;

    // Настройка трансформации для отрисовки объектов сцены.
    painter.save();
    painter.translate(0, height());
    painter.scale(1, -1);
    painter.scale(m_zoomFactor, m_zoomFactor);
    painter.translate(m_panOffset.x(), m_panOffset.y());

    // Отрисовка каждого примитива на сцене.
    for (const auto& primitive : m_scene->getPrimitives()) {
        auto it = m_drawingStrategies->find(primitive->getType());
        if (it != m_drawingStrategies->end()) {
            it->second->draw(painter, primitive.get());
        }
    }
    painter.restore();
}

// Обрабатывает нажатие кнопки мыши для начала панорамирования.
void Viewport::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

// Обновляет координаты на инфо-панели и выполняет панорамирование.
void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    m_currentMouseWorldPos = screenToWorld(event->position());
    updateInfoLabel();

    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        m_panOffset += QPointF(delta.x() / m_zoomFactor, -delta.y() / m_zoomFactor);
        update();
    }
}

// Завершает режим панорамирования.
void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

// Обрабатывает события колеса мыши для масштабирования.
void Viewport::wheelEvent(QWheelEvent *event)
{
    double factor = 1.0 + (event->angleDelta().y() / 8.0) / 100.0;
    QPointF worldPosBefore = screenToWorld(event->position());
    m_zoomFactor *= factor;
    m_zoomFactor = std::max(0.05, std::min(m_zoomFactor, 50.0));
    QPointF worldPosAfter = screenToWorld(event->position());
    m_panOffset += worldPosBefore - worldPosAfter;
    updateInfoLabel();
    update();
}

// Отрисовка координатной сетки.
void Viewport::drawGrid(QPainter& painter)
{
    QPen gridPen(QColor(50, 52, 71), 1.0, Qt::DotLine);
    QPen axisXPen(QColor("#F92672"), 1.5);
    QPen axisYPen(QColor("#66D9EF"), 1.5);
    double dynamicGridStep = calculateDynamicGridStep();

    QPointF topLeft = screenToWorld({0,0});
    QPointF bottomRight = screenToWorld({(double)width(), (double)height()});

    // Вертикальные линии.
    for (double x = std::floor(topLeft.x() / dynamicGridStep) * dynamicGridStep; x < bottomRight.x(); x += dynamicGridStep) {
        QLineF line(worldToScreen({x, topLeft.y()}), worldToScreen({x, bottomRight.y()}));
        painter.setPen(std::abs(x) < 1e-9 ? axisYPen : gridPen);
        painter.drawLine(line);
    }
    // Горизонтальные линии.
    for (double y = std::floor(bottomRight.y() / dynamicGridStep) * dynamicGridStep; y < topLeft.y(); y += dynamicGridStep) {
        QLineF line(worldToScreen({topLeft.x(), y}), worldToScreen({bottomRight.x(), y}));
        painter.setPen(std::abs(y) < 1e-9 ? axisXPen : gridPen);
        painter.drawLine(line);
    }

    // Рисуем белую точку в начале координат.
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::white);
    QPointF originScreen = worldToScreen({0.0, 0.0});
    painter.drawEllipse(originScreen, 3, 3);
    painter.restore();
}

// Отрисовка гизмо (осей координат) в левом нижнем углу.
void Viewport::drawGizmo(QPainter& painter)
{
    painter.save();
    int size = 35, padding = 15;
    QPoint origin(padding + 10, height() - padding - 10);

    QPen axisXPen(QColor("#F92672"), 2.0);
    QPen axisYPen(QColor("#66D9EF"), 2.0);

    // Ось X.
    painter.setPen(axisXPen);
    painter.drawLine(origin, origin + QPoint(size, 0));
    painter.drawText(origin + QPoint(size + 5, 5), "X");

    // Ось Y.
    painter.setPen(axisYPen);
    painter.drawLine(origin, origin - QPoint(0, size));
    painter.drawText(origin - QPoint(10, size + 5), "Y");

    painter.restore();
}

// Устанавливает сцену для отрисовки.
void Viewport::setScene(Scene* scene) { m_scene = scene; }
// Устанавливает стратегии отрисовки.
void Viewport::setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies) { m_drawingStrategies = strategies; }

// Устанавливает базовый шаг сетки и обновляет виджет.
void Viewport::setGridStep(int step)
{
    if (step > 0) {
        m_gridStep = step;
        updateInfoLabel();
        update();
    }
}

// Запрашивает перерисовку виджета.
void Viewport::update() { QWidget::update(); }

// Преобразует мировые координаты в экранные.
QPointF Viewport::worldToScreen(const QPointF& worldPos) const {
    double screenX = (worldPos.x() + m_panOffset.x()) * m_zoomFactor;
    double screenY = (worldPos.y() + m_panOffset.y()) * m_zoomFactor;
    return QPointF(screenX, height() - screenY);
}

// Преобразует экранные координаты в мировые.
QPointF Viewport::screenToWorld(const QPointF& screenPos) const {
    double worldX = (screenPos.x() / m_zoomFactor) - m_panOffset.x();
    double worldY = ((height() - screenPos.y()) / m_zoomFactor) - m_panOffset.y();
    return QPointF(worldX, worldY);
}

// Слот для смены системы координат на инфо-панели.
void Viewport::setCoordinateSystem(CoordinateSystemType type)
{
    m_coordSystemType = type;
    updateInfoLabel();
}

// Рассчитывает шаг сетки, видимый на экране, для отображения на инфо-панели.
double Viewport::calculateDynamicGridStep() const
{
    double dynamicGridStep = m_gridStep;
    while (dynamicGridStep * m_zoomFactor < 25) {
        dynamicGridStep *= 5;
    }
    while (dynamicGridStep * m_zoomFactor > 125) {
        dynamicGridStep /= 5;
    }
    return dynamicGridStep;
}

// Обновляет текст на информационной панели.
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

    infoText += QString("\nGrid: %1 px").arg(calculateDynamicGridStep());
    m_infoLabel->setText(infoText);
}
