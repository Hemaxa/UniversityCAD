#pragma once
#include <QWidget>
#include <map>
#include <memory>
#include "Enums.h"

class Scene;
class Draw;
class QPainter;
class QLabel;

/**
 * @class Viewport
 * @brief Виджет для отрисовки 2D-сцены, сетки и навигации.
 */
class Viewport : public QWidget
{
    Q_OBJECT

public:
    explicit Viewport(QWidget *parent = nullptr);

    void setScene(Scene* scene);
    void setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies);
    void setGridStep(int step);

    QPointF worldToScreen(const QPointF& worldPos) const;
    QPointF screenToWorld(const QPointF& screenPos) const;

public slots:
    void update();
    void setCoordinateSystem(CoordinateSystemType type);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void drawGrid(QPainter& painter);
    void drawGizmo(QPainter& painter);
    /**
     * @brief Обновляет текст на информационной панели.
     */
    void updateInfoLabel();
    /**
     * @brief Рассчитывает динамический шаг сетки для отображения.
     */
    double calculateDynamicGridStep() const;

    Scene* m_scene = nullptr;
    const std::map<PrimitiveType, std::unique_ptr<Draw>>* m_drawingStrategies = nullptr;

    // Параметры навигации
    int m_gridStep = 50;
    QPointF m_panOffset{0.0, 0.0};
    double m_zoomFactor = 1.0;
    QPoint m_lastPanPos;
    bool m_isPanning = false;

    // Новые поля для инфо-панели
    QLabel* m_infoLabel;
    QPointF m_currentMouseWorldPos{0.0, 0.0};
    CoordinateSystemType m_coordSystemType = CoordinateSystemType::Cartesian;
};
