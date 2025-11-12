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

// Виджет для отрисовки 2D-сцены, сетки и навигации.
class Viewport : public QWidget
{
    Q_OBJECT

public:
    // Конструктор виджета Viewport.
    explicit Viewport(QWidget *parent = nullptr);

    // Устанавливает текущую сцену для отрисовки.
    void setScene(Scene* scene);

    // Устанавливает набор стратегий отрисовки для примитивов.
    void setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies);

    // Устанавливает базовый шаг координатной сетки.
    void setGridStep(int step);

    // Преобразует мировые координаты в экранные.
    QPointF worldToScreen(const QPointF& worldPos) const;

    // Преобразует экранные координаты в мировые.
    QPointF screenToWorld(const QPointF& screenPos) const;

public slots:
    // Запрашивает перерисовку виджета.
    void update();

    // Устанавливает систему координат для отображения на инфо-панели.
    void setCoordinateSystem(CoordinateSystemType type);

    // Устанавливает текущий выбранный объект для подсветки.
    void setSelectedObject(Object* obj);

protected:
    // Главный метод отрисовки виджета.
    void paintEvent(QPaintEvent *event) override;

    // Обработчики событий.
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // Отрисовывает координатную сетку.
    void drawGrid(QPainter& painter);

    // Отрисовывает гизмо (оси координат) в углу виджета.
    void drawGizmo(QPainter& painter);

    // Обновляет текст на информационной панели.
    void updateInfoLabel();

    // Рассчитывает динамический шаг сетки для текущего масштаба.
    double calculateDynamicGridStep() const;

    // Указатель на сцену.
    Scene* m_scene = nullptr;

    // Указатель на стратегии отрисовки.
    const std::map<PrimitiveType, std::unique_ptr<Draw>>* m_drawingStrategies = nullptr;

    // Указатель на выбранный объект (для подсветки).
    Object* m_selectedObject = nullptr;

    // Параметры навигации.
    int m_gridStep = 50;
    QPointF m_panOffset{0.0, 0.0};
    double m_zoomFactor = 1.0;
    QPoint m_lastPanPos;
    bool m_isPanning = false;

    // Поля для инфо-панели.
    QLabel* m_infoLabel;
    QPointF m_currentMouseWorldPos{0.0, 0.0};
    CoordinateSystemType m_coordSystemType = CoordinateSystemType::Cartesian;
};
