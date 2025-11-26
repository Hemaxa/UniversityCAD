#pragma once

#include <QWidget>
#include <map>
#include <memory>
#include "Enums.h"

class Scene;
class Draw;
class QPainter;
class QLabel;
class Object;
class Camera;
class ContextMenu;

// Виджет, отвечающий за отрисовку 2D-сцены, сетки, навигацию и обработку мыши.
class Viewport : public QWidget
{
    Q_OBJECT

public:
    // Конструктор: настраивает камеру, меню и инфо-панель.
    explicit Viewport(QWidget *parent = nullptr);

    // Деструктор.
    ~Viewport();

    // Устанавливает указатель на сцену для отрисовки.
    void setScene(Scene* scene);

    // Устанавливает набор стратегий для отрисовки разных типов объектов.
    void setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies);

    // Задает шаг сетки (в мировых единицах).
    void setGridStep(int step);

    // Задает шаг зумирования (множитель масштаба).
    void setZoomStep(double step);

    // Преобразует координаты из мировых (сцена) в экранные (пиксели).
    QPointF worldToScreen(const QPointF& worldPos) const;

    // Преобразует координаты из экранных (пиксели) в мировые (сцена).
    QPointF screenToWorld(const QPointF& screenPos) const;

    // Возвращает точку в мировых координатах, привязанную к узлу сетки.
    QPointF getSnappedPoint(const QPointF& mousePos) const;

public slots:
    // Принудительно перерисовывает виджет.
    void update();

    // Меняет отображаемую систему координат в инфо-панели.
    void setCoordinateSystem(CoordinateSystemType type);

    // Устанавливает текущий выделенный объект (для подсветки).
    void setSelectedObject(Object* obj);

    // Приближает камеру.
    void zoomIn();

    // Отдаляет камеру.
    void zoomOut();

    // Масштабирует вид так, чтобы все объекты поместились на экране.
    void zoomToExtents();

    // Поворачивает камеру влево.
    void rotateLeft();

    // Поворачивает камеру вправо.
    void rotateRight();

protected:
    // Событие отрисовки: рисует фон, сетку, объекты и интерфейсные элементы.
    void paintEvent(QPaintEvent *event) override;

    // Событие нажатия кнопки мыши: начало панорамирования или клик по гизмо.
    void mousePressEvent(QMouseEvent *event) override;

    // Событие перемещения мыши: панорамирование и обновление координат.
    void mouseMoveEvent(QMouseEvent *event) override;

    // Событие отпускания кнопки мыши: завершение панорамирования.
    void mouseReleaseEvent(QMouseEvent *event) override;

    // Событие прокрутки колеса мыши: зумирование.
    void wheelEvent(QWheelEvent *event) override;

    // Событие изменения размера окна: обновление параметров камеры.
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // Вызывается камерой при изменении её параметров (для перерисовки).
    void onCameraUpdated();

    // Показывает контекстное меню в позиции курсора.
    void showContextMenu(const QPoint& pos);

private:
    // Рисует сетку координат.
    void drawGrid(QPainter& painter, const QTransform& transform);

    // Рисует навигационный куб/оси (Гизмо).
    void drawGizmo(QPainter& painter);

    // Обновляет текст в информационной панели (координаты, зум).
    void updateInfoLabel();

    // Вычисляет шаг сетки, адаптированный под текущий зум.
    double calculateDynamicGridStep() const;

    // Возвращает область экрана, занимаемую гизмо (для кликов).
    QRect getGizmoRect() const;

    // Указатель на сцену с данными.
    Scene* m_scene = nullptr;

    // Указатель на карту стратегий отрисовки.
    const std::map<PrimitiveType, std::unique_ptr<Draw>>* m_drawingStrategies = nullptr;

    // Указатель на выделенный объект.
    Object* m_selectedObject = nullptr;

    // Объект камеры, управляющий трансформациями.
    Camera* m_camera;

    // Контекстное меню вида.
    ContextMenu* m_contextMenu;

    // Текущий шаг сетки.
    int m_gridStep = 50;

    // Текущий коэффициент изменения зума.
    double m_zoomStep = 1.25;

    // Последняя позиция мыши при панорамировании.
    QPoint m_lastPanPos;

    // Флаг, указывающий, происходит ли сейчас панорамирование.
    bool m_isPanning = false;

    // Виджет для отображения информации о состоянии.
    QLabel* m_infoLabel;

    // Текущие координаты мыши в мире.
    QPointF m_currentMouseWorldPos{0.0, 0.0};

    // Тип системы координат для отображения в инфо-панели.
    CoordinateSystemType m_coordSystemType = CoordinateSystemType::Cartesian;
};
