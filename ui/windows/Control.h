#pragma once

#include <QWidget>
#include <QPushButton>
#include <vector>
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
    explicit Control(QWidget *parent = nullptr);

public slots:
    void updateObjectList(const Scene* scene);
    void clearSelection();

    // Сбрасывает состояние кнопок инструментов
    void resetTools();

    // Устанавливает выделение в списке (синхронизация с Viewport)
    void setSelectedObjects(const std::vector<Object*>& objects);

signals:
    void gridStepChanged(int step);
    void angleUnitChanged(AngleUnit unit);
    void coordinateSystemChanged(CoordinateSystemType type);
    void zoomStepChanged(double step);

    // ИСПРАВЛЕНО: Теперь передает список объектов
    void objectsSelected(const std::vector<Object*>& selectedObjects);

    void deleteRequested();
    void primitiveTypeSelected(PrimitiveType type);

private slots:
    void onCartesianClicked();
    void onPolarClicked();
    void onSelectionChanged();
    void onPrimitiveToolToggled(bool checked, PrimitiveType type);

private:
    QSpinBox* m_gridStepSpinBox;
    QDoubleSpinBox* m_zoomStepSpinBox;
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;
    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;
    QButtonGroup* m_primitiveToolsGroup;
    QToolButton* m_createSegmentBtn;

    // Флаг для предотвращения зацикливания сигналов выбора
    bool m_updatingSelection = false;
};
