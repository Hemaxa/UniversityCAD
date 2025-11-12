#pragma once

#include <QMainWindow>
#include <map>
#include <memory>

#include "Enums.h"

// Прямые объявления для уменьшения зависимостей в заголовочных файлах.
class QSplitter;
class Viewport;
class Control;
class Properties;
class Scene;
class Draw;
class Point;
class QColor;
class Object;

// Главное окно приложения CAD.
class CadWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Конструктор главного окна.
    CadWindow(QWidget *parent = nullptr);

    // Деструктор главного окна.
    ~CadWindow();

private slots:
    // Слот для изменения шага сетки.
    void onGridStepChanged(int step);

    // Слот для изменения единиц измерения углов.
    void onAngleUnitChanged(AngleUnit unit);

    // Слот для выбора инструмента создания примитива.
    void onPrimitiveTypeSelected(PrimitiveType type);

    // Слот для создания нового отрезка на сцене.
    void createSegment(const Point& start, const Point& end, const QColor& color);

    // Слот для обработки запроса на удаление объекта.
    void onDeleteRequested();

    // Слот для обработки выбора объекта в списке.
    void onObjectSelected(Object* selectedObject);

    // Слот для обработки изменения данных объекта.
    void onObjectModified(Object* obj);

signals:
    // Сигнал, испускаемый при любом изменении в сцене.
    void sceneChanged(const Scene* scene);

private:
    // Настраивает пользовательский интерфейс окна.
    void setupUi();

    // Создает все необходимые сигнально-слотовые соединения.
    void createConnections();

    // Инициализирует стратегии отрисовки для разных типов примитивов.
    void setupDrawingStrategies();

    // UI компоненты.
    QSplitter* m_mainSplitter;
    QSplitter* m_rightColumnSplitter;
    Viewport* m_viewportPanel;
    Control* m_controlPanel;
    Properties* m_propertiesPanel;

    // Ядро.
    Scene* m_scene;
    std::map<PrimitiveType, std::unique_ptr<Draw>> m_drawingStrategies;
    Object* m_selectedObject = nullptr; // Указатель на выбранный объект.
    PrimitiveType m_activePrimitiveType = PrimitiveType::Generic; // Хранит активный инструмент
};
