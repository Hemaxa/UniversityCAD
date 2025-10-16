#pragma once
#include <QWidget>
#include "Enums.h"

// Прямые объявления
class QSpinBox;
class QComboBox;
class QPushButton;
class QToolButton;
class QListWidget;
class Scene;
class Object;

/**
 * @class Control
 * @brief Панель с настройками, списком объектов и инструментами.
 */
class Control : public QWidget
{
    Q_OBJECT

public:
    explicit Control(QWidget *parent = nullptr);

public slots:
    /**
     * @brief Слот для обновления списка объектов при изменении сцены.
     * @param scene Указатель на текущую сцену.
     */
    void updateObjectList(const Scene* scene);

signals:
    // Сигналы об изменении настроек
    void gridStepChanged(int step);
    void angleUnitChanged(AngleUnit unit);
    void coordinateSystemChanged(CoordinateSystemType type);

    // Сигнал о том, что пользователь выбрал объект в списке
    void objectSelected(Object* selectedObject);
    // Сигнал о нажатии кнопки "Удалить"
    void deleteRequested();


private slots:
    void onCartesianClicked();
    void onPolarClicked();
    /**
     * @brief Слот, который срабатывает при выборе элемента в списке.
     */
    void onSelectionChanged();


private:
    // Элементы UI
    QSpinBox* m_gridStepSpinBox;
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;
    QPushButton* m_createSegmentBtn;

    // Новые элементы для списка объектов и удаления
    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;
};
