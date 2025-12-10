#pragma once

#include <QWidget>
#include <vector>
#include "Enums.h"
#include "Object.h"

class QStackedWidget;
class QPushButton;
class QLabel;
class Point;
class QColor;
class QDoubleSpinBox;
class QComboBox;
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
    void segmentCreateRequested(const Point& start, const Point& end, const QColor& color, const LineStyle& style);
    void objectsModified(const std::vector<Object*>& objs);

private slots:
    void onApplyClicked(); // Обрабатывает и создание, и обновление
    void onColorButtonClicked();
    void updateSegmentMetrics();
    void showStyleMenu();
    void onAddCustomStyle();

private:
    QWidget* createPlaceholderWidget();
    QWidget* createSegmentWidgets();
    QWidget* createStyleWidget();

    void updateColorButton(const QColor& color);
    void populateFields(const std::vector<Object*>& objects);
    void populateStyleFields(const std::vector<Object*>& objects);
    void getPointsFromFields(Point& start, Point& end);

    // Возвращает путь к иконке ресурса для типа
    QString getIconPath(LineStyleType type);

    QStackedWidget* m_stack;
    QWidget* m_placeholderWidget;
    QWidget* m_segmentWidget;

    CoordinateSystemType m_coordSystem;
    QColor m_selectedColor;

    // Текущий выбранный стиль (используется для новых объектов)
    LineStyle m_currentStyle;

    // Список доступных пресетов (стандартные + пользовательские)
    std::vector<LineStyle> m_availableStyles;

    std::vector<Object*> m_currentObjects;
    bool m_isCreationMode = true;

    QStackedWidget* m_segmentParamsStack;
    QWidget* m_cartesianSegmentWidgets;
    QWidget* m_polarSegmentWidgets;

    // Спинбоксы геометрии
    QDoubleSpinBox *m_startXSpin, *m_startYSpin, *m_endXSpin, *m_endYSpin;
    QDoubleSpinBox *m_polarStartXSpin, *m_polarStartYSpin;
    QDoubleSpinBox *m_endRadiusSpin, *m_endAngleSpin;
    QLabel *m_endAngleLabel, *m_segmentLengthLabel, *m_segmentAngleLabel;

    // Виджеты стиля
    QWidget* m_styleContainer;
    QPushButton* m_stylePresetButton;
    QDoubleSpinBox* m_lineWidthSpin;
    QDoubleSpinBox* m_dashLengthSpin;
    QDoubleSpinBox* m_gapLengthSpin;
    QPushButton* m_colorButton;

    QPushButton* m_applyButton; // Кнопка "Создать" / "Обновить"

    bool m_mixedColor = false;
};
