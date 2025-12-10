#include "Properties.h"
#include "Point.h"
#include "Segment.h"
#include "Object.h"
#include "StyleDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QMenu>
#include <QAction>
#include <QStyle> // Важно для обновления свойств стиля
#include <cmath>

Properties::Properties(QWidget *parent)
    : QWidget(parent),
    m_coordSystem(CoordinateSystemType::Cartesian),
    m_selectedColor(Qt::white),
    m_isCreationMode(true)
{
    this->setObjectName("PropertiesPanel");
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Инициализация стандартных стилей
    m_availableStyles = {
        {LineStyleType::Solid, "Сплошная", 0.8, 0, 0, true},
        {LineStyleType::Solid, "Сплошная тонкая", 0.4, 0, 0, false},
        {LineStyleType::SolidWavy, "Волнистая", 0.8, 0, 0, true},
        {LineStyleType::SolidZigZag, "С изломами", 0.8, 0, 0, true},
        {LineStyleType::Dashed, "Штриховая", 0.8, 4.0, 2.0, true},
        {LineStyleType::DashDot, "Штрихпунктирная", 0.8, 4.0, 2.0, true},
        {LineStyleType::DashDotDot, "Штрихпунктирная 2т", 0.8, 4.0, 2.0, true}
    };

    m_currentStyle = m_availableStyles[0];

    m_stack = new QStackedWidget(this);
    mainLayout->addWidget(m_stack);

    m_placeholderWidget = createPlaceholderWidget();
    m_segmentWidget = createSegmentWidgets();

    m_stack->addWidget(m_placeholderWidget);
    m_stack->addWidget(m_segmentWidget);
    m_stack->setCurrentWidget(m_placeholderWidget);
}

QWidget* Properties::createPlaceholderWidget() {
    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignCenter);
    auto* label = new QLabel("Выберите объекты\nили инструмент", this);
    label->setObjectName("PlaceholderLabel"); // Стиль в styles.qss
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return container;
}

QWidget* Properties::createStyleWidget() {
    auto* group = new QGroupBox("Стиль линии");
    auto* layout = new QFormLayout(group);
    layout->setLabelAlignment(Qt::AlignLeft);

    m_stylePresetButton = new QPushButton("Сплошная");
    m_stylePresetButton->setObjectName("StylePresetButton"); // Стиль в styles.qss
    connect(m_stylePresetButton, &QPushButton::clicked, this, &Properties::showStyleMenu);
    layout->addRow("Тип:", m_stylePresetButton);

    m_lineWidthSpin = new QDoubleSpinBox();
    m_lineWidthSpin->setRange(0.01, 50.0);
    m_lineWidthSpin->setSingleStep(0.1);
    m_lineWidthSpin->setSuffix(" мм");
    layout->addRow("Толщина:", m_lineWidthSpin);

    m_dashLengthSpin = new QDoubleSpinBox();
    m_dashLengthSpin->setRange(0.1, 100.0);
    layout->addRow("Штрих:", m_dashLengthSpin);

    m_gapLengthSpin = new QDoubleSpinBox();
    m_gapLengthSpin->setRange(0.1, 100.0);
    layout->addRow("Пробел:", m_gapLengthSpin);

    m_colorButton = new QPushButton();
    m_colorButton->setObjectName("ColorPickerButton");
    updateColorButton(m_selectedColor);
    auto* colorContainer = new QWidget();
    auto* colorLayout = new QHBoxLayout(colorContainer);
    colorLayout->setContentsMargins(0, 0, 0, 0);
    colorLayout->addWidget(m_colorButton);
    colorLayout->addStretch();
    layout->addRow("Цвет:", colorContainer);

    connect(m_colorButton, &QPushButton::clicked, this, &Properties::onColorButtonClicked);
    return group;
}

QWidget* Properties::createSegmentWidgets()
{
    auto* container = new QWidget();
    auto* layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignTop);

    auto* geoGroup = new QGroupBox("Геометрия");
    auto* formLayout = new QFormLayout(geoGroup);
    formLayout->setLabelAlignment(Qt::AlignLeft);

    m_segmentParamsStack = new QStackedWidget();

    m_cartesianSegmentWidgets = new QWidget();
    auto* cl = new QFormLayout(m_cartesianSegmentWidgets); cl->setContentsMargins(0,0,0,0);
    m_startXSpin = new QDoubleSpinBox(); m_startYSpin = new QDoubleSpinBox();
    m_endXSpin = new QDoubleSpinBox(); m_endYSpin = new QDoubleSpinBox();
    for(auto* s : {m_startXSpin, m_startYSpin, m_endXSpin, m_endYSpin}) {
        s->setRange(-10000, 10000); s->setDecimals(2);
        connect(s, &QDoubleSpinBox::valueChanged, this, &Properties::updateSegmentMetrics);
    }
    cl->addRow("Начало X:", m_startXSpin); cl->addRow("Начало Y:", m_startYSpin);
    cl->addRow("Конец X:", m_endXSpin); cl->addRow("Конец Y:", m_endYSpin);

    m_polarSegmentWidgets = new QWidget();
    auto* pl = new QFormLayout(m_polarSegmentWidgets); pl->setContentsMargins(0,0,0,0);
    m_polarStartXSpin = new QDoubleSpinBox(); m_polarStartYSpin = new QDoubleSpinBox();
    m_endRadiusSpin = new QDoubleSpinBox(); m_endAngleSpin = new QDoubleSpinBox();
    m_polarStartXSpin->setRange(-10000,10000); m_polarStartYSpin->setRange(-10000,10000);
    m_endRadiusSpin->setRange(0,10000); m_endAngleSpin->setRange(-360,360);
    for(auto* s : {m_polarStartXSpin, m_polarStartYSpin, m_endRadiusSpin, m_endAngleSpin}) {
        s->setDecimals(2);
        connect(s, &QDoubleSpinBox::valueChanged, this, &Properties::updateSegmentMetrics);
    }
    auto* al = new QHBoxLayout(); al->addWidget(m_endAngleSpin); m_endAngleLabel = new QLabel("°"); al->addWidget(m_endAngleLabel);
    pl->addRow("Начало X:", m_polarStartXSpin); pl->addRow("Начало Y:", m_polarStartYSpin);
    pl->addRow("Конец R:", m_endRadiusSpin); pl->addRow("Конец A:", al);

    m_segmentParamsStack->addWidget(m_cartesianSegmentWidgets);
    m_segmentParamsStack->addWidget(m_polarSegmentWidgets);
    formLayout->addRow(m_segmentParamsStack);

    m_segmentLengthLabel = new QLabel("0.00");
    m_segmentAngleLabel = new QLabel("0.00 °");
    formLayout->addRow("Длина:", m_segmentLengthLabel);
    formLayout->addRow("Угол:", m_segmentAngleLabel);

    layout->addWidget(geoGroup);

    m_styleContainer = createStyleWidget();
    layout->addWidget(m_styleContainer);

    m_applyButton = new QPushButton("Создать");
    m_applyButton->setObjectName("ApplyButton");
    layout->addWidget(m_applyButton);
    connect(m_applyButton, &QPushButton::clicked, this, &Properties::onApplyClicked);

    updateSegmentMetrics();
    return container;
}

void Properties::showCreationPropertiesFor(PrimitiveType type)
{
    m_isCreationMode = true;
    m_currentObjects.clear();

    if (type == PrimitiveType::Segment) {
        m_stack->setCurrentWidget(m_segmentWidget);
        // Показываем группу геометрии (она родитель стека)
        m_segmentParamsStack->parentWidget()->show();

        m_applyButton->show();
        m_applyButton->setText("Создать");

        // Обновляем состояние для стилизации
        m_applyButton->setProperty("state", "create");
        m_applyButton->style()->unpolish(m_applyButton);
        m_applyButton->style()->polish(m_applyButton);

    } else {
        m_stack->setCurrentWidget(m_placeholderWidget);
    }
}

void Properties::showEditingPropertiesFor(const std::vector<Object*>& objects)
{
    m_isCreationMode = false;
    m_currentObjects = objects;

    if (objects.empty()) {
        m_stack->setCurrentWidget(m_placeholderWidget);
        return;
    }

    m_stack->setCurrentWidget(m_segmentWidget);

    m_applyButton->show();
    m_applyButton->setText("Обновить");

    // Обновляем состояние для стилизации
    m_applyButton->setProperty("state", "update");
    m_applyButton->style()->unpolish(m_applyButton);
    m_applyButton->style()->polish(m_applyButton);

    // ИСПРАВЛЕНИЕ: Скрываем только GroupBox (родитель стека), а не весь контейнер
    if (objects.size() > 1) {
        m_segmentParamsStack->parentWidget()->hide();
    } else {
        m_segmentParamsStack->parentWidget()->show();
        if(objects[0]->getType() == PrimitiveType::Segment)
            populateFields(objects);
    }

    populateStyleFields(objects);
}

// ... populateFields, populateStyleFields, onApplyClicked, getIconPath ...
// (Они остаются без изменений логики, а стилизация там отсутствовала или была минимальна)
// ВАЖНО: Вставьте сюда соответствующие методы из предыдущего кода Properties.cpp

void Properties::populateFields(const std::vector<Object*>& objects) {
    if(objects.empty() || objects.size() > 1) return;
    Segment* s = dynamic_cast<Segment*>(objects[0]);
    if(!s) return;
    for(auto* spin : {m_startXSpin, m_startYSpin, m_endXSpin, m_endYSpin,
                       m_polarStartXSpin, m_polarStartYSpin, m_endRadiusSpin, m_endAngleSpin}) {
        spin->blockSignals(true);
    }
    const Point& start = s->getStart(); const Point& end = s->getEnd();
    m_startXSpin->setValue(start.getX()); m_startYSpin->setValue(start.getY());
    m_endXSpin->setValue(end.getX()); m_endYSpin->setValue(end.getY());
    m_polarStartXSpin->setValue(start.getX()); m_polarStartYSpin->setValue(start.getY());
    m_endRadiusSpin->setValue(end.getRadius()); m_endAngleSpin->setValue(end.getAngle());

    for(auto* spin : {m_startXSpin, m_startYSpin, m_endXSpin, m_endYSpin,
                       m_polarStartXSpin, m_polarStartYSpin, m_endRadiusSpin, m_endAngleSpin}) {
        spin->blockSignals(false);
    }
    updateSegmentMetrics();
}

void Properties::populateStyleFields(const std::vector<Object*>& objects) {
    if(objects.empty()) return;

    LineStyle refStyle = objects[0]->getLineStyle();
    QColor refColor = objects[0]->getColor();

    bool diffType = false, diffWidth = false, diffColor = false;
    bool diffDash = false, diffGap = false;

    for (size_t i = 1; i < objects.size(); ++i) {
        const auto& s = objects[i]->getLineStyle();
        if (s.type != refStyle.type) diffType = true;
        if (std::abs(s.width - refStyle.width) > 0.001) diffWidth = true;
        if (std::abs(s.dashLength - refStyle.dashLength) > 0.001) diffDash = true;
        if (std::abs(s.gapLength - refStyle.gapLength) > 0.001) diffGap = true;
        if (objects[i]->getColor() != refColor) diffColor = true;
    }

    m_mixedColor = diffColor;

    m_lineWidthSpin->blockSignals(true);
    m_dashLengthSpin->blockSignals(true);
    m_gapLengthSpin->blockSignals(true);

    if (diffType) {
        m_stylePresetButton->setText("Разные");
        m_stylePresetButton->setIcon(QIcon());
    } else {
        m_stylePresetButton->setText(refStyle.name);
        m_stylePresetButton->setIcon(QIcon(getIconPath(refStyle.type)));
    }

    if (diffWidth) m_lineWidthSpin->setValue(refStyle.width);
    else m_lineWidthSpin->setValue(refStyle.width);

    m_dashLengthSpin->setValue(diffDash ? 0 : refStyle.dashLength);
    m_gapLengthSpin->setValue(diffGap ? 0 : refStyle.gapLength);

    m_selectedColor = refColor;
    updateColorButton(refColor);

    m_lineWidthSpin->blockSignals(false);
    m_dashLengthSpin->blockSignals(false);
    m_gapLengthSpin->blockSignals(false);
}

void Properties::onApplyClicked()
{
    LineStyle uiStyle;
    uiStyle.type = m_currentStyle.type;
    uiStyle.name = m_stylePresetButton->text();
    uiStyle.width = m_lineWidthSpin->value();
    uiStyle.dashLength = m_dashLengthSpin->value();
    uiStyle.gapLength = m_gapLengthSpin->value();
    uiStyle.isMain = (uiStyle.width >= 0.5);

    if (m_isCreationMode) {
        Point start, end;
        getPointsFromFields(start, end);
        emit segmentCreateRequested(start, end, m_selectedColor, uiStyle);
    } else {
        // Режим обновления
        for (auto* obj : m_currentObjects) {
            LineStyle newStyle = obj->getLineStyle();

            // Если не "Разные", применяем тип стиля
            if (m_stylePresetButton->text() != "Разные") {
                newStyle.type = m_currentStyle.type;
                newStyle.name = m_currentStyle.name;
            }

            // Применяем параметры из UI
            newStyle.width = m_lineWidthSpin->value();
            newStyle.dashLength = m_dashLengthSpin->value();
            newStyle.gapLength = m_gapLengthSpin->value();
            newStyle.isMain = (newStyle.width >= 0.5);

            obj->setLineStyle(newStyle);

            // Если цвет mixed, но мы нажали обновить - применяем текущий m_selectedColor
            obj->setColor(m_selectedColor);

            if (m_currentObjects.size() == 1 && obj->getType() == PrimitiveType::Segment) {
                Point start, end;
                getPointsFromFields(start, end);
                Segment* s = static_cast<Segment*>(obj);
                s->setStart(start);
                s->setEnd(end);
            }
        }
        emit objectsModified(m_currentObjects);
    }
}

void Properties::showStyleMenu() {
    QMenu menu(this);
    // Стиль меню теперь полностью в styles.qss (селектор QMenu)

    for (const auto& s : m_availableStyles) {
        QAction* action = menu.addAction(QIcon(getIconPath(s.type)), s.name);
        connect(action, &QAction::triggered, this, [this, s]() {
            m_currentStyle = s;
            m_stylePresetButton->setText(s.name);
            m_stylePresetButton->setIcon(QIcon(getIconPath(s.type)));

            m_lineWidthSpin->setValue(s.width);
            m_dashLengthSpin->setValue(s.dashLength);
            m_gapLengthSpin->setValue(s.gapLength);
        });
    }

    menu.addSeparator();
    QAction* addAction = menu.addAction("Добавить свой стиль...");
    connect(addAction, &QAction::triggered, this, &Properties::onAddCustomStyle);

    menu.exec(m_stylePresetButton->mapToGlobal(QPoint(0, m_stylePresetButton->height())));
}

void Properties::onAddCustomStyle() {
    StyleDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        LineStyle newStyle = dlg.getStyle();
        m_availableStyles.push_back(newStyle);
        m_currentStyle = newStyle;
        m_stylePresetButton->setText(newStyle.name);
        m_stylePresetButton->setIcon(QIcon(getIconPath(LineStyleType::Custom)));
        m_lineWidthSpin->setValue(newStyle.width);
        m_dashLengthSpin->setValue(newStyle.dashLength);
        m_gapLengthSpin->setValue(newStyle.gapLength);
    }
}

void Properties::onColorButtonClicked() {
    QColor color = QColorDialog::getColor(m_selectedColor, this, "Выберите цвет");
    if (color.isValid()) {
        m_selectedColor = color;
        m_mixedColor = false;
        updateColorButton(m_selectedColor);
    }
}

QString Properties::getIconPath(LineStyleType type) {
    switch(type) {
    case LineStyleType::Solid: return ":/icons/linestyle-solid.svg";
    case LineStyleType::SolidWavy: return ":/icons/linestyle-wavy.svg";
    case LineStyleType::SolidZigZag: return ":/icons/linestyle-zigzag.svg";
    case LineStyleType::Dashed: return ":/icons/linestyle-dashed.svg";
    case LineStyleType::DashDot: return ":/icons/linestyle-dashdot.svg";
    case LineStyleType::DashDotDot: return ":/icons/linestyle-dashdotdot.svg";
    case LineStyleType::Custom: return ":/icons/linestyle-dashed.svg";
    default: return ":/icons/linestyle-solid.svg";
    }
}

void Properties::updateColorButton(const QColor& color)
{
    // Динамический стиль (цвет) оставляем в C++, но ставим свойство "mixed"
    if (!m_isCreationMode && m_mixedColor) {
        m_colorButton->setProperty("mixed", true);
        // Градиент слишком сложен для чистого QSS без хаков, оставляем inline
        // Но можно было бы перенести, если бы QSS поддерживал сложные условия
        // Пока оставим gradient здесь, но уберем простую заливку
        m_colorButton->setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 red, stop:0.5 green, stop:1 blue);");
    } else {
        m_colorButton->setProperty("mixed", false);
        m_colorButton->setStyleSheet(QString("background-color: %1;").arg(color.name()));
    }
    // Обновляем стиль, если вдруг background-color не применился бы
    m_colorButton->style()->unpolish(m_colorButton);
    m_colorButton->style()->polish(m_colorButton);
}

void Properties::updateSegmentMetrics() {
    Point start, end; getPointsFromFields(start, end);
    double dx = end.getX() - start.getX(); double dy = end.getY() - start.getY();
    double length = std::sqrt(dx * dx + dy * dy);
    double angleRad = std::atan2(dy, dx);
    double angle = (Point::getAngleUnit() == AngleUnit::Degrees) ? (angleRad * 180.0 / M_PI) : angleRad;
    const QString unit = (Point::getAngleUnit() == AngleUnit::Degrees) ? "°" : "rad";
    m_segmentLengthLabel->setText(QString::number(length, 'f', 2));
    m_segmentAngleLabel->setText(QString("%1 %2").arg(angle, 0, 'f', 2).arg(unit));
}
void Properties::getPointsFromFields(Point& start, Point& end) {
    if (m_coordSystem == CoordinateSystemType::Cartesian) {
        start.setX(m_startXSpin->value()); start.setY(m_startYSpin->value());
        end.setX(m_endXSpin->value()); end.setY(m_endYSpin->value());
    } else {
        start.setX(m_polarStartXSpin->value()); start.setY(m_polarStartYSpin->value());
        end.setPolar(m_endRadiusSpin->value(), m_endAngleSpin->value());
    }
}
void Properties::setCoordinateSystem(CoordinateSystemType type) {
    m_coordSystem = type; m_segmentParamsStack->setCurrentIndex((type == CoordinateSystemType::Cartesian) ? 0 : 1);
    if(m_currentObjects.size() == 1 && m_currentObjects[0]->getType() == PrimitiveType::Segment) populateFields(m_currentObjects);
    else updateSegmentMetrics();
}
void Properties::updateAngleLabels() {
    const QString unit = (Point::getAngleUnit() == AngleUnit::Degrees) ? "°" : "rad";
    m_endAngleLabel->setText(unit);
}
