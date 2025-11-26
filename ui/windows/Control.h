#pragma once

#include <QWidget>
#include <QPushButton>
#include "Enums.h"

class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QToolButton;
class QButtonGroup;
class QListWidget;
class Scene;
class Object;

// Панель управления: содержит настройки сцены, список объектов и инструменты.
class Control : public QWidget
{
    Q_OBJECT

public:
    // Конструктор: инициализирует элементы управления.
    explicit Control(QWidget *parent = nullptr);

public slots:
    // Обновляет виджет списка объектов на основе данных сцены.
    void updateObjectList(const Scene* scene);

    // Обработчик снятия выделения с объекта.
    void clearSelection();

signals:
    // Сигнал об изменении шага сетки.
    void gridStepChanged(int step);

    // Сигнал об изменении единиц измерения (градусы/радианы).
    void angleUnitChanged(AngleUnit unit);

    // Сигнал об изменении системы координат (Декартова/Полярная).
    void coordinateSystemChanged(CoordinateSystemType type);

    // Сигнал об изменении шага зумирования.
    void zoomStepChanged(double step);

    // Сигнал о выборе объекта в списке.
    void objectSelected(Object* selectedObject);

    // Сигнал о нажатии кнопки удаления.
    void deleteRequested();

    // Сигнал о выборе инструмента создания примитива.
    void primitiveTypeSelected(PrimitiveType type);

private slots:
    // Обработчик нажатия кнопки "XYZ" (Декартова система).
    void onCartesianClicked();

    // Обработчик нажатия кнопки "Pol" (Полярная система).
    void onPolarClicked();

    // Обработчик изменения выделения в списке объектов.
    void onSelectionChanged();

    // Обработчик переключения кнопок инструментов.
    void onPrimitiveToolToggled(bool checked, PrimitiveType type);

private:
    // Спинбокс для настройки шага сетки.
    QSpinBox* m_gridStepSpinBox;

    // Спинбокс для настройки коэффициента зума.
    QDoubleSpinBox* m_zoomStepSpinBox;

    // Выпадающий список для выбора единиц измерения углов.
    QComboBox* m_angleUnitComboBox;

    // Кнопка переключения в декартову систему координат.
    QToolButton* m_cartesianBtn;

    // Кнопка переключения в полярную систему координат.
    QToolButton* m_polarBtn;

    // Виджет списка для отображения объектов сцены.
    QListWidget* m_objectListWidget;

    // Кнопка для удаления выбранного объекта.
    QPushButton* m_deleteBtn;

    // Группа кнопок инструментов (для взаимоисключающего выбора).
    QButtonGroup* m_primitiveToolsGroup;

    // Кнопка инструмента "Отрезок".
    QToolButton* m_createSegmentBtn;
};
