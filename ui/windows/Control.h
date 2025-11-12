#pragma once

#include <QWidget>
#include <QPushButton>

#include "Enums.h"

// Прямые объявления.
class QSpinBox;
class QComboBox;
class QToolButton;
class QButtonGroup;
class QListWidget;
class Scene;
class Object;

// Панель с настройками, списком объектов и инструментами.
class Control : public QWidget
{
    Q_OBJECT

public:
    // Конструктор панели управления.
    explicit Control(QWidget *parent = nullptr);

public slots:
    // Обновляет список объектов на панели данными из сцены.
    void updateObjectList(const Scene* scene);

signals:
    // Сигналы об изменении настроек.
    void gridStepChanged(int step);
    void angleUnitChanged(AngleUnit unit);
    void coordinateSystemChanged(CoordinateSystemType type);

    // Сигнал о том, что пользователь выбрал объект в списке.
    void objectSelected(Object* selectedObject);

    // Сигнал о нажатии кнопки "Удалить".
    void deleteRequested();

    // Сигнал о выборе инструмента для создания примитива.
    void primitiveTypeSelected(PrimitiveType type);

private slots:
    // Слот при выборе декартовой системы координат.
    void onCartesianClicked();

    // Слот при выборе полярной системы координат.
    void onPolarClicked();

    // Слот, срабатывающий при изменении выбора в списке объектов.
    void onSelectionChanged();

    // Слот, срабатывающий при выборе инструмента (напр. Отрезок).
    void onPrimitiveToolToggled(bool checked, PrimitiveType type);

private:
    // Элементы UI.
    QSpinBox* m_gridStepSpinBox;
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;
    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;

    // Группа для кнопок-инструментов.
    QButtonGroup* m_primitiveToolsGroup;
    QToolButton* m_createSegmentBtn;
};
