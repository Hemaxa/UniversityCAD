#pragma once
#include <QMainWindow>
#include <map>
#include <memory>
#include <vector>
#include "Enums.h"
#include "Object.h"

class QSplitter;
class Viewport;
class Control;
class Properties;
class Scene;
class Draw;
class Point;
class QColor;
class Object;

class CadWindow : public QMainWindow
{
    Q_OBJECT

public:
    CadWindow(QWidget *parent = nullptr);
    ~CadWindow();

private slots:
    void onGridStepChanged(int step);
    void onAngleUnitChanged(AngleUnit unit);
    void onPrimitiveTypeSelected(PrimitiveType type);

    void onDeleteRequested();

    // Слот при выборе во Viewport
    void onObjectsSelected(const std::vector<Object*>& selectedObjects);

    // Слот при выборе в Control (Списке)
    void onObjectsSelectedFromList(const std::vector<Object*>& selectedObjects);

    void onObjectsModified(const std::vector<Object*>& objs);

    // ИСПРАВЛЕНО: Принимаем сырой указатель (Object*), чтобы обойти ограничения MOC на unique_ptr
    void onObjectCreateRequested(Object* obj);

    // Слот для кнопки Escape
    void onEscapePressed();

signals:
    void sceneChanged(const Scene* scene);

private:
    void setupUi();
    void createConnections();
    void setupDrawingStrategies();

    QSplitter* m_mainSplitter;
    QSplitter* m_rightColumnSplitter;
    Viewport* m_viewportPanel;
    Control* m_controlPanel;
    Properties* m_propertiesPanel;

    Scene* m_scene;
    std::map<PrimitiveType, std::unique_ptr<Draw>> m_drawingStrategies;

    std::vector<Object*> m_selectedObjects;
    PrimitiveType m_activePrimitiveType = PrimitiveType::Generic;
};
