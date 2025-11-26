#include "Camera.h"
#include <QtMath>
#include <algorithm>

Camera::Camera(QObject* parent) : QObject(parent)
{
    // Начальная позиция камеры
    m_panOffset = QPointF(50.0, 50.0);

    // Настройка анимации вращения
    m_rotationAnimation = new QPropertyAnimation(this, "rotationAngle", this);
    m_rotationAnimation->setDuration(300);
    m_rotationAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_rotationAnimation, &QPropertyAnimation::valueChanged, this, &Camera::updated);
}

void Camera::setCanvasSize(const QSize& size)
{
    m_canvasSize = size;
}

QTransform Camera::getWorldToScreenTransform() const
{
    QTransform t;

    // Перенос начала координат в левый нижний угол экрана
    t.translate(0, m_canvasSize.height());

    // Инверсия оси Y так как в Qt Y вниз а в математике Y вверх
    t.scale(1, -1);

    // Масштабирование
    t.scale(m_zoomFactor, m_zoomFactor);

    // Панорамирование
    t.translate(m_panOffset.x(), m_panOffset.y());

    // Вращение
    t.rotate(m_rotationAngle);

    return t;
}

QTransform Camera::getScreenToWorldTransform() const
{
    bool invertible;
    return getWorldToScreenTransform().inverted(&invertible);
}

qreal Camera::getZoomFactor() const
{
    return m_zoomFactor;
}

void Camera::pan(const QPointF& screenDelta)
{
    QTransform screenToWorldTf = getScreenToWorldTransform();
    if (!screenToWorldTf.isInvertible()) return;

    // Вычисляем вектор смещения в мировых координатах
    QPointF worldP0 = screenToWorldTf.map(QPointF(0.0, 0.0));
    QPointF worldP1 = screenToWorldTf.map(screenDelta);
    QPointF worldDelta = worldP1 - worldP0;

    // Корректируем смещение с учетом текущего поворота камеры
    QTransform worldToPanOffsetTransform;
    worldToPanOffsetTransform.rotate(m_rotationAngle);
    QPointF panOffsetDelta = worldToPanOffsetTransform.map(worldDelta);

    m_panOffset += panOffsetDelta;
    emit updated();
}

void Camera::applyZoom(double factor, const QPoint& anchorPoint)
{
    // Запоминаем где была точка под курсором в мире до зума
    QPointF worldPosBeforeZoom = getScreenToWorldTransform().map(anchorPoint);

    // Применяем зум с ограничением
    m_zoomFactor *= factor;
    m_zoomFactor = std::max(0.05, std::min(m_zoomFactor, 50.0));

    // Смотрим где эта точка оказалась после зума
    QPointF worldPosAfterZoom = getScreenToWorldTransform().map(anchorPoint);

    // Вычисляем разницу чтобы компенсировать смещение
    QPointF worldDelta = worldPosBeforeZoom - worldPosAfterZoom;

    // Применяем компенсацию к смещению камеры
    QTransform worldToPanOffsetTransform;
    worldToPanOffsetTransform.rotate(m_rotationAngle);
    m_panOffset += worldToPanOffsetTransform.map(worldDelta);

    emit updated();
}

void Camera::rotateLeft()
{
    m_targetRotationStep++;
    qreal targetAngle = m_targetRotationStep * 90.0;

    m_rotationAnimation->stop();
    m_rotationAnimation->setEndValue(targetAngle);
    m_rotationAnimation->start();
}

void Camera::rotateRight()
{
    m_targetRotationStep--;
    qreal targetAngle = m_targetRotationStep * 90.0;

    m_rotationAnimation->stop();
    m_rotationAnimation->setEndValue(targetAngle);
    m_rotationAnimation->start();
}

void Camera::fitBounds(const QRectF& worldBounds)
{
    if (!worldBounds.isValid() || m_canvasSize.isEmpty()) return;

    // Добавляем небольшой отступ
    qreal padding = std::max(worldBounds.width(), worldBounds.height()) * 0.1;
    if (padding < 10) padding = 10;
    QRectF paddedBounds = worldBounds.adjusted(-padding, -padding, padding, padding);

    // Учитываем поворот при расчете границ
    QTransform rotation;
    rotation.rotate(m_rotationAngle);
    QRectF rotatedBounds = rotation.mapRect(paddedBounds);
    if (rotatedBounds.width() <= 1e-9 || rotatedBounds.height() <= 1e-9) return;

    // Вычисляем зум для ширины и высоты берем минимальный
    double xZoom = (double)m_canvasSize.width() / rotatedBounds.width();
    double yZoom = (double)m_canvasSize.height() / rotatedBounds.height();
    m_zoomFactor = std::min(xZoom, yZoom);

    // Ограничиваем зум разумными пределами
    m_zoomFactor = std::max(0.05, std::min(m_zoomFactor, 50.0));

    // Центр экрана в пространстве смещения камеры
    QPointF targetPanOffsetSpaceCenter = QPointF(
        (m_canvasSize.width() / 2.0) / m_zoomFactor,
        (m_canvasSize.height() / 2.0) / m_zoomFactor
        );

    m_panOffset = targetPanOffsetSpaceCenter - rotatedBounds.center();

    emit updated();
}

void Camera::setRotationAngle(qreal angle)
{
    m_rotationAngle = angle;
    emit updated();
}

qreal Camera::getRotationAngle() const
{
    return m_rotationAngle;
}
