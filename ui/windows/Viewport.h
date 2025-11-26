#pragma once

#include <QWidget>
#include <map>
#include <memory>

#include "Enums.h"

// Прямые объявления.
class Scene;
class Draw;
class QPainter;
class QLabel;
class Object;
class Camera; // Новый класс камеры
class ContextMenu; // Новый класс меню

// Виджет для отрисовки 2D-сцены, сетки и навигации.
class Viewport : public QWidget
{
    Q_OBJECT

public:
    explicit Viewport(QWidget *parent = nullptr);
    ~Viewport(); // Деструктор нужен для удаления камеры

    void setScene(Scene* scene);
    void setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies);
    void setGridStep(int step);

    // Методы преобразования координат теперь используют камеру
    QPointF worldToScreen(const QPointF& worldPos) const;
    QPointF screenToWorld(const QPointF& screenPos) const;

    // Геттер точки привязки (для инструментов)
    QPointF getSnappedPoint(const QPointF& mousePos) const;

public slots:
    void update(); // Переопределенный update
    void setCoordinateSystem(CoordinateSystemType type);
    void setSelectedObject(Object* obj);

    // Новые слоты для навигации (вызываются из меню или горячих клавиш)
    void zoomIn();
    void zoomOut();
    void zoomToExtents();
    void rotateLeft();
    void rotateRight();

protected:
    void paintEvent(QPaintEvent *event) override;

    // События мыши и колеса
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // Нужно для обновления размера камеры

private slots:
    // Слот, вызываемый камерой при изменении её состояния
    void onCameraUpdated();
    // Слот вызова контекстного меню
    void showContextMenu(const QPoint& pos);

private:
    void drawGrid(QPainter& painter, const QTransform& transform); // Обновленная сетка
    void drawGizmo(QPainter& painter); // Обновленный гизмо
    void updateInfoLabel();
    double calculateDynamicGridStep() const;
    QRect getGizmoRect() const; // Область клика по гизмо

    Scene* m_scene = nullptr;
    const std::map<PrimitiveType, std::unique_ptr<Draw>>* m_drawingStrategies = nullptr;
    Object* m_selectedObject = nullptr;

    // --- Новые компоненты ---
    Camera* m_camera; // Экземпляр камеры
    ContextMenu* m_contextMenu; // Экземпляр меню

    // Параметры навигации (старые удалены, осталась логика взаимодействия)
    int m_gridStep = 50;
    QPoint m_lastPanPos;
    bool m_isPanning = false;

    QLabel* m_infoLabel;
    QPointF m_currentMouseWorldPos{0.0, 0.0};
    CoordinateSystemType m_coordSystemType = CoordinateSystemType::Cartesian;
};
