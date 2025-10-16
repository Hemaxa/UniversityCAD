#pragma once
#include <QWidget>
#include "Enums.h"

class QStackedWidget;
class QPushButton;
class QLabel;
class Point;
class QColor;
class QDoubleSpinBox;

/**
 * @class Properties
 * @brief Панель для ввода параметров создаваемого объекта.
 */
class Properties : public QWidget
{
    Q_OBJECT

public:
    explicit Properties(QWidget *parent = nullptr);

public slots:
    void setCoordinateSystem(CoordinateSystemType type);
    void updateAngleLabels();

signals:
    void segmentCreateRequested(const Point& start, const Point& end, const QColor& color);

private slots:
    void onApplySegmentCreation();
    void onColorButtonClicked();

private:
    QWidget* createSegmentWidgets();
    void updateColorButton(const QColor& color);

    QStackedWidget* m_stack;
    QWidget* m_segmentWidget;
    CoordinateSystemType m_coordSystem;
    QColor m_selectedColor;

    QStackedWidget* m_segmentParamsStack;
    QWidget* m_cartesianSegmentWidgets;
    QWidget* m_polarSegmentWidgets;
    QDoubleSpinBox *m_startXSpin, *m_startYSpin, *m_endXSpin, *m_endYSpin;
    QDoubleSpinBox *m_startRadiusSpin, *m_startAngleSpin, *m_endRadiusSpin, *m_endAngleSpin;
    QLabel *m_startAngleLabel, *m_endAngleLabel;
    QPushButton* m_colorButton;
    QPushButton* m_applySegmentButton;
};
