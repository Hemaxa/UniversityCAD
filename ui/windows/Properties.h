#pragma once

#include <QWidget>
#include <vector>
#include <map>           // Добавлено для std::map
#include <memory>        // Добавлено для std::unique_ptr
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
class QGroupBox; // Важное предварительное объявление
class Segment;
class QToolButton;

// Панель для ввода параметров и стилей объектов.
class Properties : public QWidget
{
    Q_OBJECT

public:
    explicit Properties(QWidget *parent = nullptr);

public slots:
    void setCoordinateSystem(CoordinateSystemType type);
    void updateAngleLabels();

    void showCreationPropertiesFor(PrimitiveType type);
    void showEditingPropertiesFor(const std::vector<Object*>& objects);

signals:
    // Передаем сырой указатель (владение заберет Scene)
    void objectCreateRequested(Object* obj);
    void objectsModified(const std::vector<Object*>& objs);

private slots:
    void onApplyClicked();
    void onColorButtonClicked();
    void showStyleMenu();
    void onAddCustomStyle();

private:
    QWidget* createPlaceholder();
    QWidget* createSegmentWidget();
    QWidget* createCircleWidget();
    QWidget* createArcWidget();
    QWidget* createRectangleWidget();
    QWidget* createEllipseWidget();
    QWidget* createPolygonWidget();
    QWidget* createSplineWidget();

    // ИСПРАВЛЕНО: Возвращаем QGroupBox*, чтобы совпадало с типом m_styleGroup
    QGroupBox* createStyleWidget();

    void populateFields(Object* obj);
    void populateStyleFields(const std::vector<Object*>& objects);

    // Указатели на виджеты
    QStackedWidget* m_stack;
    QWidget* m_placeholderWidget;
    std::map<PrimitiveType, QWidget*> m_primitiveWidgets;

    // Группа стиля
    QGroupBox* m_styleGroup;
    QPushButton* m_applyButton;

    // Поля ввода
    QDoubleSpinBox *m_segX1, *m_segY1, *m_segX2, *m_segY2;
    QComboBox* m_circleMethodCombo;
    QDoubleSpinBox *m_circCX, *m_circCY, *m_circR, *m_circP1X, *m_circP1Y, *m_circP2X, *m_circP2Y;
    QDoubleSpinBox *m_arcCX, *m_arcCY, *m_arcR, *m_arcStart, *m_arcSpan;
    QComboBox* m_rectMethodCombo;
    QDoubleSpinBox *m_rectP1X, *m_rectP1Y, *m_rectP2X, *m_rectP2Y;
    QDoubleSpinBox *m_rectCX, *m_rectCY, *m_rectW, *m_rectH, *m_rectChamfer;
    QDoubleSpinBox *m_ellCX, *m_ellCY, *m_ellRX, *m_ellRY;
    QDoubleSpinBox *m_polyCX, *m_polyCY, *m_polyR;
    QSpinBox* m_polySides;
    QComboBox* m_polyInscribed;

    // Стили
    QPushButton* m_stylePresetButton;
    QDoubleSpinBox* m_lineWidthSpin;
    QPushButton* m_colorButton;

    CoordinateSystemType m_coordSystem;
    QColor m_selectedColor = Qt::white;

    LineStyle m_currentStyle;
    std::vector<LineStyle> m_availableStyles;

    std::vector<Object*> m_currentObjects;
    bool m_isCreationMode = true;
    PrimitiveType m_activeType = PrimitiveType::Generic;
};
