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
class QDoubleSpinBox;
class QComboBox;
class QSpinBox;
class QGroupBox;
class QGridLayout;
class QVBoxLayout;
class QScrollArea;
class QLineEdit;

class Properties : public QWidget
{
    Q_OBJECT

public:
    explicit Properties(QWidget *parent = nullptr);

    LineStyle getCurrentStyle() const { return m_currentStyle; }
    QColor getCurrentColor() const { return m_selectedColor; }
    QString getCurrentLayer() const { return m_selectedLayer; }
    void applyCurrentStyleTo(Object* obj) const;

public slots:
    void setCoordinateSystem(CoordinateSystemType type);
    void showCreationPropertiesFor(PrimitiveType type, int methodIndex = 0);
    void showEditingPropertiesFor(const std::vector<Object*>& objects);

signals:
    void objectCreateRequested(Object* obj);
    void objectsModified(const std::vector<Object*>& objs);
    void dimensionSideToggleRequested();

private slots:
    void onApplyClicked();
    void onColorButtonClicked();
    void showStyleMenu();
    void onAddCustomStyle();
    void onEditCustomStyle(int index);
    void onDeleteCustomStyle(int index);
    void onAddSplinePoint();
    void onRemoveSplinePoint();
    void clearAllSplinePoints();

private:
    QWidget* createPlaceholder();
    QGridLayout* setupGridLayout(QGroupBox* group);
    void addRow(QGridLayout* layout, int row, QLabel* l1, QWidget* w1, QLabel* l2 = nullptr, QWidget* w2 = nullptr);

    QWidget* createSegmentWidget();
    QWidget* createCircleWidget();
    QWidget* createArcWidget();
    QWidget* createRectangleWidget();
    QWidget* createEllipseWidget();
    QWidget* createPolygonWidget();
    QWidget* createSplineWidget();
    QWidget* createPointWidget();
    QWidget* createDimensionWidget();
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
    std::map<PrimitiveType, std::vector<QLabel*>> m_coordLabels;

    QGroupBox* m_styleGroup;
    QPushButton* m_applyButton;

    // --- Поля ввода ---
    QDoubleSpinBox *m_segX1, *m_segY1, *m_segX2, *m_segY2;
    QLabel *m_lblSegStartX, *m_lblSegStartY, *m_lblSegEndX, *m_lblSegEndY;

    QComboBox* m_circleMethodCombo;
    QStackedWidget* m_circleStack;
    QDoubleSpinBox *m_circCX, *m_circCY, *m_circR, *m_circCX_D, *m_circCY_D, *m_circD, *m_circ2P1X, *m_circ2P1Y, *m_circ2P2X, *m_circ2P2Y, *m_circ3P1X, *m_circ3P1Y, *m_circ3P2X, *m_circ3P2Y, *m_circ3P3X, *m_circ3P3Y;

    QComboBox* m_arcMethodCombo;
    QStackedWidget* m_arcStack;
    QDoubleSpinBox *m_arcCX, *m_arcCY, *m_arcR, *m_arcStart, *m_arcSpan, *m_arc3P1X, *m_arc3P1Y, *m_arc3P2X, *m_arc3P2Y, *m_arc3P3X, *m_arc3P3Y;

    QComboBox* m_rectMethodCombo;
    QStackedWidget* m_rectStack;
    QDoubleSpinBox *m_rectP1X, *m_rectP1Y, *m_rectP2X, *m_rectP2Y, *m_rect1PX, *m_rect1PY, *m_rect1W, *m_rect1H, *m_rectCX, *m_rectCY, *m_rectCW, *m_rectCH, *m_rectChamfer;

    QComboBox* m_ellMethodCombo;
    QStackedWidget* m_ellStack;
    QDoubleSpinBox *m_ellCX, *m_ellCY, *m_ellRX, *m_ellRY, *m_ell2CX, *m_ell2CY, *m_ell2P1X, *m_ell2P1Y, *m_ell2P2X, *m_ell2P2Y;

    QDoubleSpinBox *m_polyCX, *m_polyCY, *m_polyR;
    QSpinBox* m_polySides;
    QComboBox* m_polyInscribed;

    QGridLayout* m_splinePointsLayout;
    std::vector<std::pair<QDoubleSpinBox*, QDoubleSpinBox*>> m_splineSpinBoxes;

    QPushButton* m_stylePresetButton;
    // Убран m_lineWidthSpin (теперь глобально)
    QPushButton* m_colorButton;
    QComboBox* m_layerCombo;

    // Поля для точки
    QDoubleSpinBox *m_ptX, *m_ptY;

    QComboBox* m_dimTypeCombo;
    QDoubleSpinBox *m_dimValue, *m_dimArrowSize, *m_dimTextHeight, *m_dimTextOffset;
    QComboBox* m_dimArrowCombo;
    QComboBox* m_dimArrowPlacementCombo;
    QComboBox* m_dimPrefixCombo;
    QPushButton* m_dimColorButton;
    QPushButton* m_dimCenterTextButton;
    QPushButton* m_dimToggleSideButton;
    QLineEdit* m_dimTextOverrideEdit;
    QString m_dimTextOverride;

    CoordinateSystemType m_coordSystem = CoordinateSystemType::Cartesian;
    QColor m_selectedColor = Qt::white;
    QString m_selectedLayer = "0";
    LineStyle m_currentStyle;
    std::vector<LineStyle> m_availableStyles;
    std::vector<Object*> m_currentObjects;
    bool m_isCreationMode = true;
    PrimitiveType m_activeType = PrimitiveType::Generic;
};
