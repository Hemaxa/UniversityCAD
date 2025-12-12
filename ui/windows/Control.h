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
    void setPopupWidget(QWidget* popup);

signals:
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

    // ИЗМЕНЕНО: Добавлен index для выбора конкретного способа построения
    void primitiveTypeSelected(PrimitiveType type, int methodIndex);

private slots:
    void onCartesianClicked();
    void onPolarClicked();
    void onSelectionChanged();

    // ИЗМЕНЕНО: Слот теперь принимает и индекс
    void onPrimitiveToolClicked(int id);

private:
    QWidget* createVariantPopup(LongPressButton* mainBtn,
                                const std::vector<std::pair<QString, int>>& variants, // int - это subMethodIndex
                                PrimitiveType type);

    QSpinBox* m_gridStepSpinBox;
    QDoubleSpinBox* m_zoomStepSpinBox;
    QComboBox* m_angleUnitComboBox;
    QToolButton* m_cartesianBtn;
    QToolButton* m_polarBtn;
    QListWidget* m_objectListWidget;
    QPushButton* m_deleteBtn;

    QButtonGroup* m_primitiveToolsGroup;
    bool m_updatingSelection = false;

    // Храним текущий выбранный под-метод для каждого типа, чтобы при повторном клике восстанавливать его
    std::map<PrimitiveType, int> m_activeSubMethods;
};
