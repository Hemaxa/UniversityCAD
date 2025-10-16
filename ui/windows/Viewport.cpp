#include "Viewport.h"
#include "Scene.h"
#include "Point.h"
#include "Draw.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QGridLayout>

/**
 * @brief Конструктор виджета Viewport.
 */
Viewport::Viewport(QWidget *parent) : QWidget(parent)
{
    // Включаем отслеживание мыши, чтобы получать события mouseMoveEvent
    // даже без зажатых кнопок. Это нужно для обновления координат на инфо-панели.
    setMouseTracking(true);
    // Устанавливаем политику фокуса, чтобы виджет мог получать события колеса мыши.
    setFocusPolicy(Qt::StrongFocus);
    // Устанавливаем начальное смещение, чтобы центр координат был виден при запуске.
    m_panOffset = QPointF(width() / 2.0, height() / 2.0);

    // --- Создание и настройка инфо-панели ---
    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName("InfoLabel"); // Для стилизации через QSS
    m_infoLabel->setFixedSize(130, 60);
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Используем QGridLayout для простого и надежного позиционирования
    // виджета в правом нижнем углу.
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    // Добавляем виджет в последнюю ячейку сетки (1,1) и выравниваем по углам
    layout->addWidget(m_infoLabel, 1, 1, Qt::AlignBottom | Qt::AlignRight);
    // Растягиваем первую строку (0) и первый столбец (0), чтобы они заняли все
    // свободное пространство, прижимая инфо-панель к углу.
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);

    updateInfoLabel(); // Первоначальное отображение текста на панели
}

/**
 * @brief Главный метод отрисовки виджета. Вызывается системой при необходимости.
 */
void Viewport::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // Включаем сглаживание

    // Сначала рисуем фон
    painter.fillRect(rect(), QColor("#2D2D2D"));

    // Рисуем сетку и гизмо в экранных координатах
    drawGrid(painter);
    drawGizmo(painter);

    // Если нет сцены или отрисовщиков, дальше не рисуем.
    if (!m_scene || !m_drawingStrategies) return;

    // --- Настройка трансформации для отрисовки мировых координат ---
    painter.save(); // Сохраняем текущее состояние painter
    painter.translate(0, height()); // 1. Сдвигаем начало координат в левый нижний угол
    painter.scale(1, -1);           // 2. Инвертируем ось Y, чтобы она росла вверх
    painter.scale(m_zoomFactor, m_zoomFactor); // 3. Применяем зум
    painter.translate(m_panOffset.x(), m_panOffset.y()); // 4. Применяем панорамирование

    // Проходим по всем примитивам в сцене и отрисовываем каждый.
    for (const auto& primitive : m_scene->getPrimitives()) {
        auto it = m_drawingStrategies->find(primitive->getType());
        if (it != m_drawingStrategies->end()) {
            it->second->draw(painter, primitive.get()); // Вызываем нужную стратегию
        }
    }
    painter.restore(); // Восстанавливаем состояние painter
}

/**
 * @brief Обрабатывает нажатие кнопки мыши.
 * ИСПОЛЬЗУЕТСЯ ДЛЯ НАЧАЛА ПАНОРАМИРОВАНИЯ (перемещения по сцене).
 * Для этого необходимо зажать СРЕДНЮЮ КНОПКУ МЫШИ (колесико).
 */
void Viewport::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor); // Меняем курсор на "руку"
    }
}

/**
 * @brief Обрабатывает перемещение мыши.
 * Обновляет координаты на инфо-панели и выполняет панорамирование, если оно активно.
 */
void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    // Обновляем текущие мировые координаты курсора
    m_currentMouseWorldPos = screenToWorld(event->position());
    updateInfoLabel();

    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        // Смещение камеры зависит от текущего масштаба
        m_panOffset += QPointF(delta.x() / m_zoomFactor, -delta.y() / m_zoomFactor);
        update();
    }
}

/**
 * @brief Обрабатывает отпускание кнопки мыши.
 * Завершает режим панорамирования.
 */
void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor); // Возвращаем обычный курсор
    }
}

/**
 * @brief Обрабатывает вращение колеса мыши для масштабирования (зума).
 * Зум происходит относительно положения курсора.
 */
void Viewport::wheelEvent(QWheelEvent *event)
{
    double factor = 1.0 + (event->angleDelta().y() / 8.0) / 100.0; // Коэффициент зума
    QPointF worldPosBefore = screenToWorld(event->position());
    m_zoomFactor *= factor;
    m_zoomFactor = std::max(0.05, std::min(m_zoomFactor, 50.0)); // Ограничение зума
    QPointF worldPosAfter = screenToWorld(event->position());
    m_panOffset += worldPosBefore - worldPosAfter; // Коррекция для зума от курсора
    updateInfoLabel(); // Обновляем инфо (меняется шаг сетки)
    update();
}

/**
 * @brief Отрисовка координатной сетки.
 * Шаг сетки динамически изменяется в зависимости от масштаба.
 */
void Viewport::drawGrid(QPainter& painter)
{
    QPen gridPen(QColor(60, 60, 60), 1.0);
    QPen axisXPen(Qt::red, 1.5);
    QPen axisYPen(Qt::green, 1.5);
    double dynamicGridStep = calculateDynamicGridStep();

    QPointF topLeft = screenToWorld({0,0});
    QPointF bottomRight = screenToWorld({(double)width(), (double)height()});

    for (double x = std::floor(topLeft.x() / dynamicGridStep) * dynamicGridStep; x < bottomRight.x(); x += dynamicGridStep) {
        painter.setPen(std::abs(x) < 1e-9 ? axisYPen : gridPen);
        painter.drawLine(worldToScreen({x, topLeft.y()}), worldToScreen({x, bottomRight.y()}));
    }
    for (double y = std::floor(bottomRight.y() / dynamicGridStep) * dynamicGridStep; y < topLeft.y(); y += dynamicGridStep) {
        painter.setPen(std::abs(y) < 1e-9 ? axisXPen : gridPen);
        painter.drawLine(worldToScreen({topLeft.x(), y}), worldToScreen({bottomRight.x(), y}));
    }
}

/**
 * @brief Отрисовка гизмо (осей координат) в левом нижнем углу.
 */
void Viewport::drawGizmo(QPainter& painter)
{
    painter.save();
    int size = 40, padding = 15;
    QPoint origin(padding + 10, height() - padding - 10);
    painter.setPen(QPen(Qt::red, 2.0));
    painter.drawLine(origin, origin + QPoint(size, 0));
    painter.drawText(origin + QPoint(size + 5, 5), "X");
    painter.setPen(QPen(Qt::green, 2.0));
    painter.drawLine(origin, origin - QPoint(0, size));
    painter.drawText(origin - QPoint(10, size + 5), "Y");
    painter.restore();
}

void Viewport::setScene(Scene* scene) { m_scene = scene; }
void Viewport::setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies) { m_drawingStrategies = strategies; }

void Viewport::setGridStep(int step)
{
    if (step > 0) {
        m_gridStep = step;
        updateInfoLabel(); // Обновляем инфо (меняется шаг сетки)
        update();
    }
}

void Viewport::update() { QWidget::update(); }

QPointF Viewport::worldToScreen(const QPointF& worldPos) const {
    double screenX = (worldPos.x() + m_panOffset.x()) * m_zoomFactor;
    double screenY = (worldPos.y() + m_panOffset.y()) * m_zoomFactor;
    return QPointF(screenX, height() - screenY);
}

QPointF Viewport::screenToWorld(const QPointF& screenPos) const {
    double worldX = (screenPos.x() / m_zoomFactor) - m_panOffset.x();
    double worldY = ((height() - screenPos.y()) / m_zoomFactor) - m_panOffset.y();
    return QPointF(worldX, worldY);
}

/**
 * @brief Слот для смены системы координат.
 * Вызывается из CadWindow, когда пользователь нажимает кнопку на панели Control.
 */
void Viewport::setCoordinateSystem(CoordinateSystemType type)
{
    m_coordSystemType = type;
    updateInfoLabel(); // Обновляем текст координат
}

/**
 * @brief Рассчитывает шаг сетки, видимый на экране, для отображения на инфо-панели.
 */
double Viewport::calculateDynamicGridStep() const
{
    double dynamicGridStep = m_gridStep;
    while (dynamicGridStep * m_zoomFactor < 20) dynamicGridStep *= 5;
    while (dynamicGridStep * m_zoomFactor > 100) dynamicGridStep /= 5;
    return dynamicGridStep;
}

/**
 * @brief Обновляет текст на информационной панели с текущими координатами и шагом сетки.
 */
void Viewport::updateInfoLabel()
{
    QString infoText;
    if (m_coordSystemType == CoordinateSystemType::Cartesian) {
        infoText = QString("X: %1\nY: %2")
        .arg(m_currentMouseWorldPos.x(), 0, 'f', 2)
            .arg(m_currentMouseWorldPos.y(), 0, 'f', 2);
    } else {
        Point p(m_currentMouseWorldPos.x(), m_currentMouseWorldPos.y());
        QString angleUnit = (Point::getAngleUnit() == AngleUnit::Degrees) ? "°" : "rad";
        infoText = QString("R: %1\nA: %2%3")
                       .arg(p.getRadius(), 0, 'f', 2)
                       .arg(p.getAngle(), 0, 'f', 2)
                       .arg(angleUnit);
    }

    infoText += QString("\nGrid: %1 px").arg(calculateDynamicGridStep());
    m_infoLabel->setText(infoText);
}
