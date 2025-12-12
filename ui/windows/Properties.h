#pragma once

#include <QWidget>
#include <vector>
#include <map>
#include <memory>
#include "Enums.h"
#include "Object.h"

class QStackedWidget;
class QPushButton;
class QLabel;
class Point;
class QColor;
class QDoubleSpinBox;
class QComboBox;
class QSpinBox;
class QGroupBox;
class QGridLayout;
class QVBoxLayout;
class QScrollArea;

// Панель для ввода параметров и стилей объектов.
class Properties : public QWidget
{
    Q_OBJECT

public:
    explicit Properties(QWidget *parent = nullptr);

public slots:
    void setCoordinateSystem(CoordinateSystemType type);
    void showCreationPropertiesFor(PrimitiveType type, int methodIndex = 0);
    void showEditingPropertiesFor(const std::vector<Object*>& objects);

signals:
    void objectCreateRequested(Object* obj);
    void objectsModified(const std::vector<Object*>& objs);

private slots:
    void onApplyClicked();
    void onColorButtonClicked();
    void showStyleMenu();
    void onAddCustomStyle();

    void onAddSplinePoint();
    void onRemoveSplinePoint();

private:
    QWidget* createPlaceholder();

    // Хелпер для создания стандартного Grid Layout
    QGridLayout* setupGridLayout(QGroupBox* group);

    // Хелпер для добавления строки из двух пар "Лейбл-Значение"
    void addRow(QGridLayout* layout, int row,
                QLabel* l1, QWidget* w1,
                QLabel* l2 = nullptr, QWidget* w2 = nullptr);

    // Методы создания интерфейсов для примитивов
    QWidget* createSegmentWidget();
    QWidget* createCircleWidget();
    QWidget* createArcWidget();
    QWidget* createRectangleWidget();
    QWidget* createEllipseWidget();
    QWidget* createPolygonWidget();
    QWidget* createSplineWidget();

    QGroupBox* createStyleWidget();

    void populateFields(Object* obj);
    void populateStyleFields(const std::vector<Object*>& objects);
    void updateObjectGeometry(Object* obj);
    void updateLabels();

    Point readPoint(QDoubleSpinBox* xBox, QDoubleSpinBox* yBox) const;

    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;

    QStackedWidget* m_stack;
    QWidget* m_placeholderWidget;
    std::map<PrimitiveType, QWidget*> m_primitiveWidgets;

    // Лейблы для динамического обновления текста (X/Y <-> R/A)
    std::map<PrimitiveType, std::vector<QLabel*>> m_coordLabels;

    QGroupBox* m_styleGroup;
    QPushButton* m_applyButton;

    // --- Поля ввода ---
    // Отрезок
    QDoubleSpinBox *m_segX1, *m_segY1, *m_segX2, *m_segY2;
    QLabel *m_lblSegStartX, *m_lblSegStartY, *m_lblSegEndX, *m_lblSegEndY;

    // Окружность
    QComboBox* m_circleMethodCombo;
    QStackedWidget* m_circleStack;
    QDoubleSpinBox *m_circCX, *m_circCY, *m_circR;
    QDoubleSpinBox *m_circCX_D, *m_circCY_D, *m_circD;
    QDoubleSpinBox *m_circ2P1X, *m_circ2P1Y, *m_circ2P2X, *m_circ2P2Y;
    QDoubleSpinBox *m_circ3P1X, *m_circ3P1Y, *m_circ3P2X, *m_circ3P2Y, *m_circ3P3X, *m_circ3P3Y;

    // Дуга
    QComboBox* m_arcMethodCombo;
    QStackedWidget* m_arcStack;
    QDoubleSpinBox *m_arcCX, *m_arcCY, *m_arcR, *m_arcStart, *m_arcSpan;
    QDoubleSpinBox *m_arc3P1X, *m_arc3P1Y, *m_arc3P2X, *m_arc3P2Y, *m_arc3P3X, *m_arc3P3Y;

    // Прямоугольник
    QComboBox* m_rectMethodCombo;
    QStackedWidget* m_rectStack;
    QDoubleSpinBox *m_rectP1X, *m_rectP1Y, *m_rectP2X, *m_rectP2Y;
    QDoubleSpinBox *m_rect1PX, *m_rect1PY, *m_rect1W, *m_rect1H;
    QDoubleSpinBox *m_rectCX, *m_rectCY, *m_rectCW, *m_rectCH;
    QDoubleSpinBox *m_rectChamfer;

    // Эллипс
    QComboBox* m_ellMethodCombo;
    QStackedWidget* m_ellStack;
    QDoubleSpinBox *m_ellCX, *m_ellCY, *m_ellRX, *m_ellRY;
    QDoubleSpinBox *m_ell2CX, *m_ell2CY, *m_ell2P1X, *m_ell2P1Y, *m_ell2P2X, *m_ell2P2Y;

    // Полигон
    QDoubleSpinBox *m_polyCX, *m_polyCY, *m_polyR;
    QSpinBox* m_polySides;
    QComboBox* m_polyInscribed;

    // Сплайн
    QGridLayout* m_splinePointsLayout; // Используем Grid для сплайна тоже
    std::vector<std::pair<QDoubleSpinBox*, QDoubleSpinBox*>> m_splineSpinBoxes;

    // Стили
    QPushButton* m_stylePresetButton;
    QDoubleSpinBox* m_lineWidthSpin;
    QPushButton* m_colorButton;

    CoordinateSystemType m_coordSystem = CoordinateSystemType::Cartesian;
    QColor m_selectedColor = Qt::white;

    LineStyle m_currentStyle;
    std::vector<LineStyle> m_availableStyles;

    std::vector<Object*> m_currentObjects;
    bool m_isCreationMode = true;
    PrimitiveType m_activeType = PrimitiveType::Generic;
};
