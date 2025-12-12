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
class QFormLayout;

// Панель для ввода параметров и стилей объектов.
class Properties : public QWidget
{
    Q_OBJECT

public:
    explicit Properties(QWidget *parent = nullptr);

public slots:
    void setCoordinateSystem(CoordinateSystemType type);

    // ИЗМЕНЕНО: Добавлен метод индекса
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

private:
    QWidget* createPlaceholder();
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

    // Обновление подписей лейблов (X->R, Y->A)
    void updateLabels();

    // Хелпер для считывания координат с учетом полярной системы
    Point readPoint(QDoubleSpinBox* xBox, QDoubleSpinBox* yBox) const;

    QStackedWidget* m_stack;
    QWidget* m_placeholderWidget;
    std::map<PrimitiveType, QWidget*> m_primitiveWidgets;

    // Храним пары лейблов для обновления X/Y -> R/Angle
    // Ключ: примитив. Значение: список пар лейблов (например, "X1:", "Y1:")
    std::map<PrimitiveType, std::vector<std::pair<QLabel*, QLabel*>>> m_coordLabels;

    QGroupBox* m_styleGroup;
    QPushButton* m_applyButton;

    // --- Поля ввода ---
    // Отрезок
    QDoubleSpinBox *m_segX1, *m_segY1, *m_segX2, *m_segY2;
    QLabel *m_lblSeg1X, *m_lblSeg1Y, *m_lblSeg2X, *m_lblSeg2Y; // Лейблы

    // Окружность
    QComboBox* m_circleMethodCombo;
    QStackedWidget* m_circleStack;
    QDoubleSpinBox *m_circCX, *m_circCY, *m_circR; // Центр-Радиус
    QDoubleSpinBox *m_circCX_D, *m_circCY_D, *m_circD; // Центр-Диаметр
    QDoubleSpinBox *m_circ2P1X, *m_circ2P1Y, *m_circ2P2X, *m_circ2P2Y; // 2 точки
    QDoubleSpinBox *m_circ3P1X, *m_circ3P1Y, *m_circ3P2X, *m_circ3P2Y, *m_circ3P3X, *m_circ3P3Y; // 3 точки
    // Лейблы окружности для хранения ссылок (чтобы менять текст)
    std::vector<QLabel*> m_circleLabels;

    // Дуга
    QComboBox* m_arcMethodCombo;
    QStackedWidget* m_arcStack;
    QDoubleSpinBox *m_arcCX, *m_arcCY, *m_arcR, *m_arcStart, *m_arcSpan; // Центр-Угол
    QDoubleSpinBox *m_arc3P1X, *m_arc3P1Y, *m_arc3P2X, *m_arc3P2Y, *m_arc3P3X, *m_arc3P3Y; // 3 Точки
    std::vector<QLabel*> m_arcLabels;

    // Прямоугольник
    QComboBox* m_rectMethodCombo;
    QStackedWidget* m_rectStack;
    QDoubleSpinBox *m_rectP1X, *m_rectP1Y, *m_rectP2X, *m_rectP2Y; // 2 точки
    QDoubleSpinBox *m_rect1PX, *m_rect1PY, *m_rect1W, *m_rect1H; // Точка-Размер
    QDoubleSpinBox *m_rectCX, *m_rectCY, *m_rectCW, *m_rectCH; // Центр-Размер
    QDoubleSpinBox *m_rectChamfer;
    std::vector<QLabel*> m_rectLabels;

    // Эллипс
    QComboBox* m_ellMethodCombo;
    QStackedWidget* m_ellStack;
    QDoubleSpinBox *m_ellCX, *m_ellCY, *m_ellRX, *m_ellRY; // Центр-Радиусы
    QDoubleSpinBox *m_ell2CX, *m_ell2CY, *m_ell2P1X, *m_ell2P1Y, *m_ell2P2X, *m_ell2P2Y; // Центр-Точки осей
    std::vector<QLabel*> m_ellLabels;

    // Полигон
    QDoubleSpinBox *m_polyCX, *m_polyCY, *m_polyR;
    QSpinBox* m_polySides;
    QComboBox* m_polyInscribed;
    std::vector<QLabel*> m_polyLabels;

    // Сплайн
    // ... пока заглушка

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
