#pragma once

#include <QWidget>
#include <QPushButton>

#include "Enums.h"

// Прямые объявления.
class QSpinBox;
class QDoubleSpinBox; // Добавили класс
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
    explicit Control(QWidget *parent = nullptr);

public slots:
    void updateObjectList(const Scene* scene);

signals:
    void gridStepChanged(int step);
    void angleUnitChanged(AngleUnit unit);
    void coordinateSystemChanged(CoordinateSystemType type);

    // Сигнал об изменении шага зума
    void zoomStepChanged(double step);

    void objectSelected(Object* selectedObject);
    void deleteRequested();
    void primitiveTypeSelected(PrimitiveType type);

private slots:
    void onCartesianClicked();
    void onPolarClicked();
    void onSelectionChanged();
    void onPrimitiveToolToggled(bool checked, PrimitiveType type);

private:
    // Элементы UI.
    QSpinBox* m_gridStepSpinBox;
    QDoubleSpinBox* m_zoomStepSpinBox; // Новый спинбокс для зума
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;
    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;

    QButtonGroup* m_primitiveToolsGroup;
    QToolButton* m_createSegmentBtn;
};
