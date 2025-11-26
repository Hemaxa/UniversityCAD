#pragma once

#include <QObject>
#include <QPointF>
#include <QSize>
#include <QTransform>
#include <QPropertyAnimation>

class Camera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal rotationAngle READ getRotationAngle WRITE setRotationAngle)

public:
    explicit Camera(QObject* parent = nullptr);

    // Установка размера области рисования
    void setCanvasSize(const QSize& size);

    // Панорамирование в экранных координатах
    void pan(const QPointF& screenDelta);

    // Зум относительно точки
    void applyZoom(double factor, const QPoint& anchorPoint);

    // Поворот на 90 градусов влево
    void rotateLeft();

    // Поворот на 90 градусов вправо
    void rotateRight();

    // Вписать прямоугольник в экран
    void fitBounds(const QRectF& worldBounds);

    // Матрица Мир в Экран
    QTransform getWorldToScreenTransform() const;

    // Матрица Экран в Мир
    QTransform getScreenToWorldTransform() const;

    // Текущий зум
    qreal getZoomFactor() const;

    // Сеттер угла для анимации
    void setRotationAngle(qreal angle);

    // Геттер угла для анимации
    qreal getRotationAngle() const;

signals:
    // Сигнал сообщающий что параметры камеры изменились
    void updated();

private:
    QPointF m_panOffset{0.0, 0.0};
    double m_zoomFactor = 1.0;
    qreal m_rotationAngle = 0.0;

    int m_targetRotationStep = 0;
    QPropertyAnimation* m_rotationAnimation;
    QSize m_canvasSize;
};
