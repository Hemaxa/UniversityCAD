#pragma once

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QTimer>
#include <vector>
#include "Enums.h"

class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QButtonGroup;
class QListWidget;
class Scene;
class Object;

// =================================================================
// Кастомная кнопка с поддержкой долгого нажатия
// =================================================================
class LongPressButton : public QToolButton {
    Q_OBJECT
public:
    explicit LongPressButton(QWidget* parent = nullptr);

    // Устанавливает виджет, который будет всплывать (контейнер с кнопками)
    void setPopupWidget(QWidget* popup);

signals:
    // Сигнал долгого нажатия (для открытия меню)
    void longPressActivated();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

    // ДОБАВЛЕНО: Для отрисовки треугольника
    void paintEvent(QPaintEvent* e) override;

private slots:
    void onTimerTimeout();

private:
    QTimer m_longPressTimer;
    QWidget* m_popupWidget = nullptr;
    bool m_isLongPressHandled = false;
};

// ... (Остальная часть класса Control без изменений)
class Control : public QWidget
{
    Q_OBJECT

public:
    explicit Control(QWidget *parent = nullptr);

public slots:
    void updateObjectList(const Scene* scene);
    void clearSelection();
    void resetTools();
    void setSelectedObjects(const std::vector<Object*>& objects);

signals:
    void gridStepChanged(int step);
    void angleUnitChanged(AngleUnit unit);
    void coordinateSystemChanged(CoordinateSystemType type);
    void zoomStepChanged(double step);
    void objectsSelected(const std::vector<Object*>& selectedObjects);
    void deleteRequested();
    void primitiveTypeSelected(PrimitiveType type);

private slots:
    void onCartesianClicked();
    void onPolarClicked();
    void onSelectionChanged();
    void onPrimitiveToolToggled(bool checked, PrimitiveType type);

private:
    QWidget* createVariantPopup(LongPressButton* mainBtn,
                                const std::vector<std::pair<QString, PrimitiveType>>& variants);

    QSpinBox* m_gridStepSpinBox;
    QDoubleSpinBox* m_zoomStepSpinBox;
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;
    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;

    QButtonGroup* m_primitiveToolsGroup;
    bool m_updatingSelection = false;
};
