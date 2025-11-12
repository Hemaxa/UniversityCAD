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
class Object;
class Segment;

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

    // Показывает панель создания.
    void showCreationPropertiesFor(PrimitiveType type);

    // Показывает панель редактирования для выбранного объекта.
    void showEditingPropertiesFor(Object* obj);

signals:
    // Сигнал, запрашивающий создание отрезка с заданными параметрами.
    void segmentCreateRequested(const Point& start, const Point& end, const QColor& color);

    // Сигнал, что данные объекта были изменены.
    void objectModified(Object* obj);

private slots:
    // Слот обрабатывает и "Создать", и "Применить".
    void onApplyClicked();

    // Слот, вызываемый при нажатии на кнопку выбора цвета.
    void onColorButtonClicked();

    // Слот для обновления вычисляемых метрик (длина, угол).
    void updateSegmentMetrics();

private:
    // Создает виджет-заглушку (когда не выбран инструмент).
    QWidget* createPlaceholderWidget();

    // Создает виджеты для ввода параметров отрезка.
    QWidget* createSegmentWidgets();

    // Обновляет цвет фона кнопки выбора цвета.
    void updateColorButton(const QColor& color);

    // Заполняет поля данными из выбранного отрезка.
    void populateFields(Segment* segment);

    // Считывает данные из полей и обновляет выбранный объект.
    void updateSelectedObject();

    // Считывает точки из полей (используется и для создания, и для обновления).
    void getPointsFromFields(Point& start, Point& end);

    // Элементы UI.
    QStackedWidget* m_stack;
    QWidget* m_placeholderWidget;
    QWidget* m_segmentWidget;
    CoordinateSystemType m_coordSystem;
    QColor m_selectedColor;

    // Указатель на объект, который сейчас редактируется.
    // Если nullptr, панель находится в режиме "Создание".
    Object* m_currentObject = nullptr;

    // Элементы для отрезка
    QStackedWidget* m_segmentParamsStack;
    QWidget* m_cartesianSegmentWidgets;
    QWidget* m_polarSegmentWidgets;
    QDoubleSpinBox *m_startXSpin, *m_startYSpin, *m_endXSpin, *m_endYSpin;
    QDoubleSpinBox *m_startRadiusSpin, *m_startAngleSpin, *m_endRadiusSpin, *m_endAngleSpin;
    QLabel *m_startAngleLabel, *m_endAngleLabel;
    QLabel *m_segmentLengthLabel, *m_segmentAngleLabel;
    QPushButton* m_colorButton;
    QPushButton* m_applyButton;
};
