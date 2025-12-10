#pragma once

#include <QWidget>
#include <map>
#include <memory>
#include <vector>
#include "Enums.h"

class Scene;
class Draw;
class QPainter;
class QLabel;
class Object;
class Camera;
class ContextMenu;
class QRubberBand;

// Виджет, отвечающий за отрисовку 2D-сцены, сетки, навигацию и обработку мыши.
class Viewport : public QWidget
{
    Q_OBJECT

public:
    explicit Viewport(QWidget *parent = nullptr);
    ~Viewport();

    void setScene(Scene* scene);
    void setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies);
    void setGridStep(int step);
    void setZoomStep(double step);

    QPointF worldToScreen(const QPointF& worldPos) const;
    QPointF screenToWorld(const QPointF& screenPos) const;
    QPointF getSnappedPoint(const QPointF& mousePos) const;

public slots:
    void update();
    void setCoordinateSystem(CoordinateSystemType type);

    // Теперь принимает список, или сбрасывает выделение
    void setSelectedObjects(const std::vector<Object*>& objs);

    void zoomIn();
    void zoomOut();
    void zoomToExtents();
    void rotateLeft();
    void rotateRight();

signals:
    // Сигнал, что выделение изменилось (передаем список)
    void selectionChanged(const std::vector<Object*>& selectedObjects);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onCameraUpdated();
    void showContextMenu(const QPoint& pos);

private:
    void drawGrid(QPainter& painter, const QTransform& transform);
    void drawGizmo(QPainter& painter);
    void updateInfoLabel();
    double calculateDynamicGridStep() const;
    QRect getGizmoRect() const;

    // Находит объекты внутри экранного прямоугольника
    std::vector<Object*> pickObjects(const QRect& screenRect);

    Scene* m_scene = nullptr;
    const std::map<PrimitiveType, std::unique_ptr<Draw>>* m_drawingStrategies = nullptr;

    // Список выделенных объектов
    std::vector<Object*> m_selectedObjects;

    Camera* m_camera;
    ContextMenu* m_contextMenu;

    int m_gridStep = 50;
    double m_zoomStep = 1.25;

    // Навигация
    QPoint m_lastPanPos;
    bool m_isPanning = false;

    // Выделение рамкой
    QRubberBand* m_rubberBand;
    QPoint m_rubberBandOrigin;
    bool m_isSelecting = false;

    QLabel* m_infoLabel;
    QPointF m_currentMouseWorldPos{0.0, 0.0};
    CoordinateSystemType m_coordSystemType = CoordinateSystemType::Cartesian;
};
