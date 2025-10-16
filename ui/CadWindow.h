#pragma once
#include <QMainWindow>
#include <map>
#include <memory>
#include "Enums.h"

// Прямые объявления
class QSplitter;
class Viewport;
class Control;
class Properties;
class Scene;
class Draw;
class Point;
class QColor;
class Object; // Добавлено

class CadWindow : public QMainWindow
{
    Q_OBJECT

public:
    CadWindow(QWidget *parent = nullptr);
    ~CadWindow();

private slots:
    void onGridStepChanged(int step);
    void onAngleUnitChanged(AngleUnit unit);
    void createSegment(const Point& start, const Point& end, const QColor& color);

    // Новые слоты для обработки удаления и выбора
    void onDeleteRequested();
    void onObjectSelected(Object* selectedObject);


signals:
    /**
     * @brief Сигнал, испускаемый при любом изменении в сцене (добавлении/удалении).
     * @param scene Указатель на текущее состояние сцены.
     */
    void sceneChanged(const Scene* scene);

private:
    void setupUi();
    void createConnections();
    void setupDrawingStrategies();

    // UI компоненты
    QSplitter* m_mainSplitter;
    QSplitter* m_rightColumnSplitter;
    Viewport* m_viewportPanel;
    Control* m_controlPanel;
    Properties* m_propertiesPanel;

    // Ядро
    Scene* m_scene;
    std::map<PrimitiveType, std::unique_ptr<Draw>> m_drawingStrategies;
    Object* m_selectedObject = nullptr; // Указатель на выбранный в списке объект
};
