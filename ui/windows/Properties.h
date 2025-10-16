#pragma once

#include <QWidget>

#include "Enums.h"

// Прямые объявления.
class QStackedWidget;
class QPushButton;
class QLabel;
class Point;
class QColor;
class QDoubleSpinBox;

// Панель для ввода параметров создаваемого объекта.
class Properties : public QWidget
{
    Q_OBJECT

public:
    // Конструктор панели свойств.
    explicit Properties(QWidget *parent = nullptr);

public slots:
    // Устанавливает текущую систему координат (декартову или полярную).
    void setCoordinateSystem(CoordinateSystemType type);
    // Обновляет суффиксы для полей ввода углов (° или rad).
    void updateAngleLabels();

signals:
    // Сигнал, запрашивающий создание отрезка с заданными параметрами.
    void segmentCreateRequested(const Point& start, const Point& end, const QColor& color);

private slots:
    // Слот, вызываемый при нажатии кнопки "Создать" для отрезка.
    void onApplySegmentCreation();
    // Слот, вызываемый при нажатии на кнопку выбора цвета.
    void onColorButtonClicked();

private:
    // Создает виджеты для ввода параметров отрезка.
    QWidget* createSegmentWidgets();
    // Обновляет цвет фона кнопки выбора цвета.
    void updateColorButton(const QColor& color);

    // Элементы UI.
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
