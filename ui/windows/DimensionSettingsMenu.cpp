#include "DimensionSettingsMenu.h"
#include "GlobalSettings.h"
#include "LineSettingsMenu.h"

#include <QWidgetAction>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QColorDialog>
#include <QFontComboBox>
#include <QIcon>
#include <QLabel>

static LineStyle makeDimensionLineStyle(LineStyleType type)
{
    switch (type) {
    case LineStyleType::Dashed:
        return {type, "Штриховая", 8.0, 3.0, false};
    case LineStyleType::DashDotThin:
        return {type, "Штрихпунктирная тонкая", 10.0, 3.0, false};
    case LineStyleType::DashDotThick:
        return {type, "Штрихпунктирная толстая", 10.0, 3.0, true};
    case LineStyleType::DashDotDot:
        return {type, "С двумя точками", 10.0, 3.0, false};
    case LineStyleType::SolidMain:
        return {type, "Сплошная основная", 0, 0, true};
    case LineStyleType::SolidThin:
    default:
        return {LineStyleType::SolidThin, "Сплошная тонкая", 0, 0, false};
    }
}

DimensionSettingsMenu::DimensionSettingsMenu(QWidget* parent) : QMenu(parent)
{
    setTitle("Настройки размеров");
    setupUi();
    connect(this, &QMenu::aboutToHide, this, &DimensionSettingsMenu::applyToExistingRequested);
}

QDoubleSpinBox* DimensionSettingsMenu::createDoubleSpin(double val, double min, double max, double step)
{
    auto* sb = new QDoubleSpinBox();
    sb->setRange(min, max);
    sb->setSingleStep(step);
    sb->setValue(val);
    sb->setMinimumWidth(80);
    connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
        emit settingsChanged();
    });
    return sb;
}

QPushButton* DimensionSettingsMenu::createColorButton(const QColor& color, const std::function<void(const QColor&)>& setter)
{
    auto* btn = new QPushButton();
    btn->setFixedSize(42, 22);
    btn->setStyleSheet(QString("background-color: %1; border: 1px solid gray;").arg(color.name()));
    connect(btn, &QPushButton::clicked, this, [this, btn, setter]() {
        QColor c = QColorDialog::getColor(btn->palette().button().color(), this);
        if (!c.isValid()) return;
        setter(c);
        btn->setStyleSheet(QString("background-color: %1; border: 1px solid gray;").arg(c.name()));
        emit settingsChanged();
    });
    return btn;
}

void DimensionSettingsMenu::setupUi()
{
    auto* widget = new QWidget();
    widget->setMinimumWidth(420);
    auto* layout = new QVBoxLayout(widget);
    auto& style = GlobalSettings::instance().dimensionStyle;

    auto* lineGroup = new QGroupBox("Линии");
    auto* lineForm = new QFormLayout(lineGroup);
    lineForm->addRow("Выносные:", createColorButton(style.extensionColor, [](const QColor& c){ GlobalSettings::instance().dimensionStyle.extensionColor = c; }));
    lineForm->addRow("Размерные:", createColorButton(style.dimensionColor, [](const QColor& c){ GlobalSettings::instance().dimensionStyle.dimensionColor = c; }));

    auto* extLineType = new QComboBox();
    auto* dimLineType = new QComboBox();
    auto addLineType = [](QComboBox* combo, const QString& text, LineStyleType type) {
        combo->addItem(QIcon(LineSettingsMenu::getIconPath(type)), text, static_cast<int>(type));
    };
    for (auto* combo : {extLineType, dimLineType}) {
        addLineType(combo, "Сплошная тонкая", LineStyleType::SolidThin);
        addLineType(combo, "Сплошная основная", LineStyleType::SolidMain);
        addLineType(combo, "Штриховая", LineStyleType::Dashed);
        addLineType(combo, "Штрихпунктирная", LineStyleType::DashDotThin);
        addLineType(combo, "С двумя точками", LineStyleType::DashDotDot);
    }
    extLineType->setCurrentIndex(extLineType->findData(static_cast<int>(style.extensionLineStyle.type)));
    dimLineType->setCurrentIndex(dimLineType->findData(static_cast<int>(style.dimensionLineStyle.type)));
    connect(extLineType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [extLineType, this](int) {
        GlobalSettings::instance().dimensionStyle.extensionLineStyle = makeDimensionLineStyle(static_cast<LineStyleType>(extLineType->currentData().toInt()));
        emit settingsChanged();
    });
    connect(dimLineType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [dimLineType, this](int) {
        GlobalSettings::instance().dimensionStyle.dimensionLineStyle = makeDimensionLineStyle(static_cast<LineStyleType>(dimLineType->currentData().toInt()));
        emit settingsChanged();
    });
    lineForm->addRow("Тип выносных:", extLineType);
    lineForm->addRow("Тип размерной:", dimLineType);

    auto* overshoot = createDoubleSpin(style.extensionOvershoot, 0.0, 100.0, 0.5);
    connect(overshoot, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ GlobalSettings::instance().dimensionStyle.extensionOvershoot = v; });
    lineForm->addRow("Выход выносных:", overshoot);

    auto* ext = createDoubleSpin(style.dimensionExtension, 0.0, 100.0, 0.5);
    connect(ext, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ GlobalSettings::instance().dimensionStyle.dimensionExtension = v; });
    lineForm->addRow("Расширение размерной:", ext);
    layout->addWidget(lineGroup);

    auto* arrowGroup = new QGroupBox("Стрелки");
    auto* arrowForm = new QFormLayout(arrowGroup);
    auto* arrowType = new QComboBox();
    arrowType->addItem(QIcon(":/icons/arrow-closed.svg"), "Закрытая", static_cast<int>(ArrowType::Closed));
    arrowType->addItem(QIcon(":/icons/arrow-open.svg"), "Открытая", static_cast<int>(ArrowType::Open));
    arrowType->addItem(QIcon(":/icons/arrow-tick.svg"), "Засечка", static_cast<int>(ArrowType::Tick));
    arrowType->addItem(QIcon(":/icons/arrow-dot.svg"), "Точка", static_cast<int>(ArrowType::Dot));
    arrowType->setCurrentIndex(arrowType->findData(static_cast<int>(style.arrowType)));
    connect(arrowType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [arrowType, this](int){
        GlobalSettings::instance().dimensionStyle.arrowType = static_cast<ArrowType>(arrowType->currentData().toInt());
        emit settingsChanged();
    });
    arrowForm->addRow("Тип:", arrowType);

    auto* arrowPlacement = new QComboBox();
    arrowPlacement->addItem("Внутри", static_cast<int>(ArrowPlacement::Inside));
    arrowPlacement->addItem("Снаружи", static_cast<int>(ArrowPlacement::Outside));
    arrowPlacement->setCurrentIndex(arrowPlacement->findData(static_cast<int>(style.arrowPlacement)));
    connect(arrowPlacement, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [arrowPlacement, this](int){
        GlobalSettings::instance().dimensionStyle.arrowPlacement = static_cast<ArrowPlacement>(arrowPlacement->currentData().toInt());
        emit settingsChanged();
    });
    arrowForm->addRow("Положение:", arrowPlacement);

    auto* arrowSize = createDoubleSpin(style.arrowSize, 1.0, 100.0, 0.5);
    connect(arrowSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ GlobalSettings::instance().dimensionStyle.arrowSize = v; });
    arrowForm->addRow("Размер:", arrowSize);
    layout->addWidget(arrowGroup);

    auto* textGroup = new QGroupBox("Текст");
    auto* textForm = new QFormLayout(textGroup);
    textForm->addRow("Цвет:", createColorButton(style.textColor, [](const QColor& c){ GlobalSettings::instance().dimensionStyle.textColor = c; }));

    auto* fontCombo = new QFontComboBox();
    fontCombo->setCurrentFont(QFont(style.fontFamily));
    connect(fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont& font) {
        GlobalSettings::instance().dimensionStyle.fontFamily = font.family();
        emit settingsChanged();
    });
    textForm->addRow("Шрифт:", fontCombo);

    auto* textHeight = createDoubleSpin(style.textHeight, 1.0, 100.0, 0.5);
    connect(textHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ GlobalSettings::instance().dimensionStyle.textHeight = v; });
    textForm->addRow("Высота:", textHeight);

    auto* textOffset = createDoubleSpin(style.textOffset, -100.0, 100.0, 0.5);
    connect(textOffset, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ GlobalSettings::instance().dimensionStyle.textOffset = v; });
    textForm->addRow("Смещение:", textOffset);
    layout->addWidget(textGroup);

    auto* action = new QWidgetAction(this);
    action->setDefaultWidget(widget);
    addAction(action);
}
