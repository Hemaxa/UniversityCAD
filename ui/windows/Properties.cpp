#include "Properties.h"
#include "Point.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QColorDialog>
#include <QDoubleSpinBox>

// Конструктор панели свойств.
Properties::Properties(QWidget *parent)
    : QWidget(parent), m_coordSystem(CoordinateSystemType::Cartesian), m_selectedColor(Qt::white)
{
    this->setObjectName("PropertiesPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);
    mainLayout->addWidget(m_stack);

    m_segmentWidget = createSegmentWidgets();
    m_stack->addWidget(m_segmentWidget);
    m_stack->setCurrentWidget(m_segmentWidget);
}

// Создает и компонует виджеты для ввода параметров отрезка.
QWidget* Properties::createSegmentWidgets()
{
    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignTop);
    auto* group = new QGroupBox("Параметры отрезка");
    layout->addWidget(group);
    auto* formLayout = new QFormLayout(group);
    formLayout->setLabelAlignment(Qt::AlignLeft); // Выравнивание по левому краю.

    m_segmentParamsStack = new QStackedWidget();

    // Виджеты для декартовых координат.
    m_cartesianSegmentWidgets = new QWidget();
    auto* cartesianLayout = new QFormLayout(m_cartesianSegmentWidgets);
    cartesianLayout->setContentsMargins(0,0,0,0);
    m_startXSpin = new QDoubleSpinBox(); m_startYSpin = new QDoubleSpinBox();
    m_endXSpin = new QDoubleSpinBox(); m_endYSpin = new QDoubleSpinBox();
    for(auto* spin : {m_startXSpin, m_startYSpin, m_endXSpin, m_endYSpin}) {
        spin->setRange(-10000, 10000);
        spin->setDecimals(2);
    }
    cartesianLayout->addRow("Начало X:", m_startXSpin);
    cartesianLayout->addRow("Начало Y:", m_startYSpin);
    cartesianLayout->addRow("Конец X:", m_endXSpin);
    cartesianLayout->addRow("Конец Y:", m_endYSpin);

    // Виджеты для полярных координат.
    m_polarSegmentWidgets = new QWidget();
    auto* polarLayout = new QFormLayout(m_polarSegmentWidgets);
    polarLayout->setContentsMargins(0,0,0,0);
    m_startRadiusSpin = new QDoubleSpinBox(); m_startAngleSpin = new QDoubleSpinBox();
    m_endRadiusSpin = new QDoubleSpinBox(); m_endAngleSpin = new QDoubleSpinBox();
    m_startRadiusSpin->setRange(0, 10000); m_endRadiusSpin->setRange(0, 10000);
    m_startAngleSpin->setRange(-360, 360); m_endAngleSpin->setRange(-360, 360);
    for(auto* spin : {m_startRadiusSpin, m_startAngleSpin, m_endRadiusSpin, m_endAngleSpin}) {
        spin->setDecimals(2);
    }

    auto* startAngleLayout = new QHBoxLayout();
    startAngleLayout->addWidget(m_startAngleSpin);
    m_startAngleLabel = new QLabel("°");
    startAngleLayout->addWidget(m_startAngleLabel);
    auto* endAngleLayout = new QHBoxLayout();
    endAngleLayout->addWidget(m_endAngleSpin);
    m_endAngleLabel = new QLabel("°");
    endAngleLayout->addWidget(m_endAngleLabel);

    polarLayout->addRow("Начало R:", m_startRadiusSpin);
    polarLayout->addRow("Начало A:", startAngleLayout);
    polarLayout->addRow("Конец R:", m_endRadiusSpin);
    polarLayout->addRow("Конец A:", endAngleLayout);

    m_segmentParamsStack->addWidget(m_cartesianSegmentWidgets);
    m_segmentParamsStack->addWidget(m_polarSegmentWidgets);
    formLayout->addRow(m_segmentParamsStack);

    // Кнопка выбора цвета.
    m_colorButton = new QPushButton();
    m_colorButton->setObjectName("ColorPickerButton");
    updateColorButton(m_selectedColor);

    auto* colorContainer = new QWidget();
    auto* colorLayout = new QHBoxLayout(colorContainer);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->addWidget(m_colorButton);
    colorLayout->addStretch();
    formLayout->addRow("Цвет:", colorContainer);

    // Кнопка "Создать".
    m_applySegmentButton = new QPushButton("Создать");
    layout->addWidget(m_applySegmentButton);

    // Соединения.
    connect(m_applySegmentButton, &QPushButton::clicked, this, &Properties::onApplySegmentCreation);
    connect(m_colorButton, &QPushButton::clicked, this, &Properties::onColorButtonClicked);

    return container;
}

// Переключает виджеты ввода между декартовыми и полярными координатами.
void Properties::setCoordinateSystem(CoordinateSystemType type)
{
    m_coordSystem = type;
    m_segmentParamsStack->setCurrentIndex((type == CoordinateSystemType::Cartesian) ? 0 : 1);
}

// Обновляет текстовые метки единиц измерения углов.
void Properties::updateAngleLabels()
{
    const QString unit = (Point::getAngleUnit() == AngleUnit::Degrees) ? "°" : "rad";
    m_startAngleLabel->setText(unit);
    m_endAngleLabel->setText(unit);
}

// Собирает данные из полей ввода и испускает сигнал для создания отрезка.
void Properties::onApplySegmentCreation()
{
    Point start, end;
    if (m_coordSystem == CoordinateSystemType::Cartesian) {
        start.setX(m_startXSpin->value()); start.setY(m_startYSpin->value());
        end.setX(m_endXSpin->value()); end.setY(m_endYSpin->value());
    } else {
        start.setPolar(m_startRadiusSpin->value(), m_startAngleSpin->value());
        end.setPolar(m_endRadiusSpin->value(), m_endAngleSpin->value());
    }
    emit segmentCreateRequested(start, end, m_selectedColor);
}

// Открывает диалог выбора цвета и обновляет выбранный цвет.
void Properties::onColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_selectedColor, this, "Выберите цвет");
    if (color.isValid()) {
        m_selectedColor = color;
        updateColorButton(m_selectedColor);
    }
}

// Обновляет цвет фона кнопки выбора цвета.
void Properties::updateColorButton(const QColor& color)
{
    m_colorButton->setStyleSheet(QString("background-color: %1;").arg(color.name()));
}
