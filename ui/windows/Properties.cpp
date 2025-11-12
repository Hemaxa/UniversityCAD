#include "Properties.h"
#include "Point.h"
#include "Segment.h"
#include "Object.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <cmath>

// Конструктор панели свойств.
Properties::Properties(QWidget *parent)
    : QWidget(parent),
    m_coordSystem(CoordinateSystemType::Cartesian),
    m_selectedColor(Qt::white),
    m_currentObject(nullptr) // Вначале ничего не редактируем
{
    this->setObjectName("PropertiesPanel");

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget(this);
    mainLayout->addWidget(m_stack);

    // Создаем и добавляем виджеты в стек
    m_placeholderWidget = createPlaceholderWidget();
    m_segmentWidget = createSegmentWidgets();
    m_stack->addWidget(m_placeholderWidget);
    m_stack->addWidget(m_segmentWidget);

    // По умолчанию показываем заглушку
    m_stack->setCurrentWidget(m_placeholderWidget);
}

// Создает виджет-заглушку
QWidget* Properties::createPlaceholderWidget()
{
    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignCenter);

    auto* label = new QLabel("Нажмите на кнопку\nдля создания объекта", this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #777; font-style: italic;");
    layout->addWidget(label);

    return container;
}

// Создает и компонует виджеты для ввода параметров отрезка.
QWidget* Properties::createSegmentWidgets()
{
    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignTop);
    auto* group = new QGroupBox("Параметры");
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
        connect(spin, &QDoubleSpinBox::valueChanged, this, &Properties::updateSegmentMetrics);
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
        connect(spin, &QDoubleSpinBox::valueChanged, this, &Properties::updateSegmentMetrics);
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

    // Длина и Угол
    m_segmentLengthLabel = new QLabel("0.00");
    m_segmentAngleLabel = new QLabel("0.00 °");
    m_segmentLengthLabel->setStyleSheet("font-weight: bold;");
    m_segmentAngleLabel->setStyleSheet("font-weight: bold;");
    formLayout->addRow("Длина:", m_segmentLengthLabel);
    formLayout->addRow("Угол:", m_segmentAngleLabel);

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

    // Кнопка "Создать" / "Применить".
    m_applyButton = new QPushButton("Создать");
    layout->addWidget(m_applyButton);

    // Соединения.
    connect(m_applyButton, &QPushButton::clicked, this, &Properties::onApplyClicked);
    connect(m_colorButton, &QPushButton::clicked, this, &Properties::onColorButtonClicked);

    updateSegmentMetrics();
    return container;
}

// Показывает панель для создания нового примитива.
void Properties::showCreationPropertiesFor(PrimitiveType type)
{
    m_currentObject = nullptr; // Мы в режиме создания, не редактирования

    if (type == PrimitiveType::Segment) {
        m_stack->setCurrentWidget(m_segmentWidget);
        m_applyButton->setText("Создать"); // Меняем текст кнопки
        // Здесь можно сбросить поля до значений по умолчанию, если нужно
    } else {
        m_stack->setCurrentWidget(m_placeholderWidget);
    }
}

// Показывает панель для редактирования существующего объекта.
void Properties::showEditingPropertiesFor(Object* obj)
{
    m_currentObject = obj; // Сохраняем указатель на редактируемый объект

    if (obj == nullptr) {
        // Если объект сброшен (nullptr), возвращаемся к заглушке
        m_stack->setCurrentWidget(m_placeholderWidget);
        return;
    }

    // Показываем нужную панель в зависимости от типа объекта
    if (obj->getType() == PrimitiveType::Segment) {
        m_stack->setCurrentWidget(m_segmentWidget);
        m_applyButton->setText("Применить"); // Меняем текст кнопки
        // Заполняем поля данными из объекта
        populateFields(static_cast<Segment*>(obj));
    }
    // Здесь можно будет добавить else if для других типов (Точка, Дуга...)
    else {
        m_stack->setCurrentWidget(m_placeholderWidget);
    }
}


// Переключает виджеты ввода между декартовыми и полярными координатами.
void Properties::setCoordinateSystem(CoordinateSystemType type)
{
    m_coordSystem = type;
    m_segmentParamsStack->setCurrentIndex((type == CoordinateSystemType::Cartesian) ? 0 : 1);

    // Если редактируем объект, нужно пересчитать и полярные/декартовы поля
    if(m_currentObject && m_currentObject->getType() == PrimitiveType::Segment) {
        populateFields(static_cast<Segment*>(m_currentObject));
    } else {
        updateSegmentMetrics(); // Обновляем метрики для полей создания
    }
}

// Обновляет текстовые метки единиц измерения углов.
void Properties::updateAngleLabels()
{
    const QString unit = (Point::getAngleUnit() == AngleUnit::Degrees) ? "°" : "rad";
    m_startAngleLabel->setText(unit);
    m_endAngleLabel->setText(unit);

    // Аналогично, пересчитываем поля, если в режиме редактирования
    if(m_currentObject && m_currentObject->getType() == PrimitiveType::Segment) {
        populateFields(static_cast<Segment*>(m_currentObject));
    } else {
        updateSegmentMetrics();
    }
}

// Слот для обновления вычисляемых метрик (длина, угол).
void Properties::updateSegmentMetrics()
{
    Point start, end;
    // Считываем текущие значения из полей
    getPointsFromFields(start, end);

    double dx = end.getX() - start.getX();
    double dy = end.getY() - start.getY();
    double length = std::sqrt(dx * dx + dy * dy);
    double angleRad = std::atan2(dy, dx);
    double angle = (Point::getAngleUnit() == AngleUnit::Degrees)
                       ? (angleRad * 180.0 / M_PI)
                       : angleRad;
    const QString unit = (Point::getAngleUnit() == AngleUnit::Degrees) ? "°" : "rad";

    m_segmentLengthLabel->setText(QString::number(length, 'f', 2));
    m_segmentAngleLabel->setText(QString("%1 %2").arg(angle, 0, 'f', 2).arg(unit));
}

// Обрабатывает нажатие кнопки "Создать" или "Применить".
void Properties::onApplyClicked()
{
    if (m_currentObject) {
        // Режим Редактирования - обновляем существующий объект
        updateSelectedObject();
        emit objectModified(m_currentObject); // Сообщаем, что объект изменен
    } else {
        // Режим Создания - создаем новый объект
        Point start, end;
        getPointsFromFields(start, end);
        emit segmentCreateRequested(start, end, m_selectedColor);
    }
}

// Открывает диалог выбора цвета и обновляет выбранный цвет.
void Properties::onColorButtonClicked()
{
    QColor color = QColorDialog::getColor(m_selectedColor, this, "Выберите цвет");
    if (color.isValid()) {
        m_selectedColor = color;
        updateColorButton(m_selectedColor);

        // Если мы в режиме редактирования, сразу применяем цвет
        if (m_currentObject) {
            m_currentObject->setColor(m_selectedColor);
            emit objectModified(m_currentObject); // Сообщаем об изменении
        }
    }
}

// Обновляет цвет фона кнопки выбора цвета.
void Properties::updateColorButton(const QColor& color)
{
    m_colorButton->setStyleSheet(QString("background-color: %1;").arg(color.name()));
}

// Заполняет поля ввода данными из объекта Segment.
void Properties::populateFields(Segment* segment)
{
    if (!segment) return;

    // Блокируем сигналы, чтобы не вызывать updateSegmentMetrics 10 раз
    for(auto* spin : {m_startXSpin, m_startYSpin, m_endXSpin, m_endYSpin,
                       m_startRadiusSpin, m_startAngleSpin, m_endRadiusSpin, m_endAngleSpin}) {
        spin->blockSignals(true);
    }

    const Point& start = segment->getStart();
    const Point& end = segment->getEnd();

    // Заполняем декартовы поля
    m_startXSpin->setValue(start.getX());
    m_startYSpin->setValue(start.getY());
    m_endXSpin->setValue(end.getX());
    m_endYSpin->setValue(end.getY());

    // Заполняем полярные поля
    m_startRadiusSpin->setValue(start.getRadius());
    m_startAngleSpin->setValue(start.getAngle());
    m_endRadiusSpin->setValue(end.getRadius());
    m_endAngleSpin->setValue(end.getAngle());

    // Заполняем цвет
    m_selectedColor = segment->getColor();
    updateColorButton(m_selectedColor);

    // Разблокируем сигналы
    for(auto* spin : {m_startXSpin, m_startYSpin, m_endXSpin, m_endYSpin,
                       m_startRadiusSpin, m_startAngleSpin, m_endRadiusSpin, m_endAngleSpin}) {
        spin->blockSignals(false);
    }

    // Теперь, когда все поля обновлены, один раз вызываем обновление метрик
    updateSegmentMetrics();
}

// Обновляет m_currentObject данными из полей ввода.
void Properties::updateSelectedObject()
{
    if (!m_currentObject || m_currentObject->getType() != PrimitiveType::Segment) {
        return;
    }

    auto* segment = static_cast<Segment*>(m_currentObject);
    Point start, end;
    getPointsFromFields(start, end); // Получаем точки из полей

    segment->setStart(start);
    segment->setEnd(end);
    segment->setColor(m_selectedColor);
}

// Вспомогательный метод для получения точек из полей.
void Properties::getPointsFromFields(Point& start, Point& end)
{
    if (m_coordSystem == CoordinateSystemType::Cartesian) {
        start.setX(m_startXSpin->value()); start.setY(m_startYSpin->value());
        end.setX(m_endXSpin->value()); end.setY(m_endYSpin->value());
    } else {
        start.setPolar(m_startRadiusSpin->value(), m_startAngleSpin->value());
        end.setPolar(m_endRadiusSpin->value(), m_endAngleSpin->value());
    }
}
