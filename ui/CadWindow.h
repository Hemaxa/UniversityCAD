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

// Главное окно CAD-приложения.
// Управляет сценой, инструментами рисования и панелями интерфейса.
class CadWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Конструктор главного окна.
    CadWindow(QWidget *parent = nullptr);
    // Деструктор.
    ~CadWindow();

private slots:
    // Обработчик изменения шага сетки.
    void onGridStepChanged(int step);
    // Обработчик изменения единиц углов (градусы/радианы).
    void onAngleUnitChanged(AngleUnit unit);
    // Обработчик выбора типа примитива и метода построения.
    void onPrimitiveTypeSelected(PrimitiveType type, int methodIndex);
    // Обработчик запроса на удаление выбранных объектов.
    void onDeleteRequested();
    // Обработчик выбора объектов на виджете просмотра.
    void onObjectsSelected(const std::vector<Object*>& selectedObjects);
    // Обработчик выбора объектов из списка.
    void onObjectsSelectedFromList(const std::vector<Object*>& selectedObjects);
    // Обработчик изменения свойств объектов.
    void onObjectsModified(const std::vector<Object*>& objs);
    // Обработчик запроса на создание объекта из панели свойств.
    void onObjectCreateRequested(Object* obj);
    // Обработчик нажатия Escape (сброс выделения/инструмента).
    void onEscapePressed();
    // Обработчик импорта DXF.
    void onImportDxf();
    // Обработчик экспорта DXF.
    void onExportDxf();
    void onApplyDimensionGlobalStyle();

signals:
    // Сигнал об изменении сцены.
    void sceneChanged(const Scene* scene);

private:
    // Настройка пользовательского интерфейса.
    void setupUi();
    // Создание связей сигнал-слот.
    void createConnections();
    // Инициализация стратегий отрисовки примитивов.
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
