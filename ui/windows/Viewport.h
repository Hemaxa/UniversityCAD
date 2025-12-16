#pragma once

#include <QWidget>
#include <map>
#include <memory>
#include <vector>
#include "Enums.h"
#include "Tools.h"

class Scene;
class Draw;
class QPainter;
class QLabel;
class Object;
class Camera;
class ContextMenu;
class QRubberBand;
class Snapper;

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

    void setActiveTool(PrimitiveType type, int subMethod);
    void resetTool();

    QPointF worldToScreen(const QPointF& worldPos) const;
    QPointF screenToWorld(const QPointF& screenPos) const;

public slots:
    void update();
    void setCoordinateSystem(CoordinateSystemType type);
    void setSelectedObjects(const std::vector<Object*>& objs);

    // Слоты для привязок
    void setGridSnap(bool enabled);
    void setObjectSnap(bool enabled);

    void zoomIn();
    void zoomOut();
    void zoomToExtents();
    void rotateLeft();
    void rotateRight();

signals:
    void selectionChanged(const std::vector<Object*>& selectedObjects);
    void objectCreated(Object* obj);

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

    // Реализация выделения рамкой
    std::vector<Object*> pickObjects(const QRect& screenRect);
    
    // Поиск объекта по точке (для клика)
    Object* pickObjectAtPoint(const QPoint& screenPoint);

    Scene* m_scene = nullptr;
    const std::map<PrimitiveType, std::unique_ptr<Draw>>* m_drawingStrategies = nullptr;
    std::vector<Object*> m_selectedObjects;

    Camera* m_camera;
    ContextMenu* m_contextMenu;
    std::unique_ptr<Snapper> m_snapper;

    std::unique_ptr<Tool> m_currentTool;
    PrimitiveType m_activeToolType = PrimitiveType::Generic;
    int m_activeSubMethod = 0;

    int m_gridStep = 50;
    double m_zoomStep = 1.25;

    // Состояние привязок
    bool m_gridSnapEnabled = false;
    bool m_objectSnapEnabled = true;

    QPoint m_lastPanPos;
    bool m_isPanning = false;

    QRubberBand* m_rubberBand;
    QPoint m_rubberBandOrigin;
    bool m_isSelecting = false;

    QLabel* m_infoLabel;
    QPointF m_currentMouseWorldPos{0.0, 0.0};
    CoordinateSystemType m_coordSystemType = CoordinateSystemType::Cartesian;
};
