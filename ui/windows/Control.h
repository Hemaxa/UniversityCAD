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
class QCheckBox;
class Scene;
class Object;

// Кнопка с поддержкой долгого нажатия для отображения подменю.
class LongPressButton : public QToolButton {
    Q_OBJECT
public:
    // Конструктор кнопки.
    explicit LongPressButton(QWidget* parent = nullptr);
    // Устанавливает всплывающий виджет для долгого нажатия.
    void setPopupWidget(QWidget* popup);

signals:
    // Сигнал активации долгого нажатия.
    void longPressActivated();

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void paintEvent(QPaintEvent* e) override;

private slots:
    void onTimerTimeout();

private:
    QTimer m_longPressTimer;
    QWidget* m_popupWidget = nullptr;
    bool m_isLongPressHandled = false;
};

// Панель управления - содержит настройки сетки, выбор инструментов и список объектов.
class Control : public QWidget
{
    Q_OBJECT

public:
    // Конструктор панели управления.
    explicit Control(QWidget *parent = nullptr);

public slots:
    // Обновляет список объектов из сцены.
    void updateObjectList(const Scene* scene);
    // Сбрасывает выделение в списке.
    void clearSelection();
    // Сбрасывает выбранные инструменты.
    void resetTools();
    // Устанавливает выбранные объекты в списке.
    void setSelectedObjects(const std::vector<Object*>& objects);

signals:
    // Сигнал изменения шага сетки.
    void gridStepChanged(int step);
    // Сигнал изменения единиц углов.
    void angleUnitChanged(AngleUnit unit);
    // Сигнал изменения системы координат.
    void coordinateSystemChanged(CoordinateSystemType type);
    // Сигнал изменения шага зума.
    void zoomStepChanged(double step);
    // Сигнал выбора объектов.
    void objectsSelected(const std::vector<Object*>& selectedObjects);
    // Сигнал запроса на удаление.
    void deleteRequested();
    // Сигнал переключения привязки к сетке.
    void gridSnapToggled(bool enabled);
    // Сигнал переключения привязки к объектам.
    void objectSnapToggled(bool enabled);
    // Сигнал выбора типа примитива и метода.
    void primitiveTypeSelected(PrimitiveType type, int methodIndex);

private slots:
    void onCartesianClicked();
    void onPolarClicked();
    void onSelectionChanged();
    void onPrimitiveToolClicked(int id);

private:
    // Создает всплывающее меню вариантов для кнопки.
    QWidget* createVariantPopup(LongPressButton* mainBtn,
                                const std::vector<std::pair<QString, int>>& variants,
                                PrimitiveType type);

    QSpinBox* m_gridStepSpinBox;
    QDoubleSpinBox* m_zoomStepSpinBox;
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;

    QCheckBox* m_gridSnapCheck;
    QCheckBox* m_objSnapCheck;

    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;

    QButtonGroup* m_primitiveToolsGroup;
    bool m_updatingSelection = false;
    std::map<PrimitiveType, int> m_activeSubMethods;
};
