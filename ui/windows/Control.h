#pragma once

#include <QWidget>

#include "Enums.h"

// Прямые объявления.
class QSpinBox;
class QComboBox;
class QPushButton;
class QToolButton;
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

private slots:
    // Слот при выборе декартовой системы координат.
    void onCartesianClicked();
    // Слот при выборе полярной системы координат.
    void onPolarClicked();
    // Слот, срабатывающий при изменении выбора в списке объектов.
    void onSelectionChanged();

private:
    // Элементы UI.
    QSpinBox* m_gridStepSpinBox;
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;
    QPushButton* m_createSegmentBtn;
    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;
};
