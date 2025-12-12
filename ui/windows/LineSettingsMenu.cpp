#include "LineSettingsMenu.h"
#include "GlobalSettings.h"
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGroupBox>

LineSettingsMenu::LineSettingsMenu(QWidget* parent) : QMenu(parent) {
    setTitle("Настройки линий");
    setupUi();
}

QDoubleSpinBox* LineSettingsMenu::createDoubleSpin(double val, double min, double step) {
    auto* sb = new QDoubleSpinBox();
    sb->setRange(min, 100.0);
    sb->setSingleStep(step);
    sb->setValue(val);
    sb->setMinimumWidth(70);
    // При изменении сразу обновляем глобальные настройки и шлем сигнал
    connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double){
        emit settingsChanged();
    });
    return sb;
}

void LineSettingsMenu::setupUi() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    // --- Глобальные множители ---
    auto* globalGroup = new QGroupBox("Общие множители");
    auto* globalForm = new QFormLayout(globalGroup);

    auto* wScale = createDoubleSpin(GlobalSettings::instance().globalWidthScale, 0.1, 0.1);
    connect(wScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ GlobalSettings::instance().globalWidthScale = v; });
    globalForm->addRow("Масштаб толщины:", wScale);

    auto* ltScale = createDoubleSpin(GlobalSettings::instance().globalLinetypeScale, 0.1, 0.1);
    connect(ltScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ GlobalSettings::instance().globalLinetypeScale = v; });
    globalForm->addRow("Масштаб штриха:", ltScale);

    layout->addWidget(globalGroup);

    // --- Параметры типов ---
    auto* paramsGroup = new QGroupBox("Параметры типов");
    auto* paramsLayout = new QFormLayout(paramsGroup);

    auto addParamRow = [&](const QString& name, LineStyleType type) {
        auto* dash = createDoubleSpin(GlobalSettings::instance().styleParams[type].dash);
        auto* gap = createDoubleSpin(GlobalSettings::instance().styleParams[type].gap);

        connect(dash, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [type](double v){ GlobalSettings::instance().styleParams[type].dash = v; });
        connect(gap, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [type](double v){ GlobalSettings::instance().styleParams[type].gap = v; });

        auto* rowWidget = new QWidget();
        auto* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0,0,0,0);
        rowLayout->addWidget(new QLabel("Штрих:")); rowLayout->addWidget(dash);
        rowLayout->addWidget(new QLabel("Пробел:")); rowLayout->addWidget(gap);

        paramsLayout->addRow(name, rowWidget);
    };

    addParamRow("Штриховая:", LineStyleType::Dashed);
    addParamRow("Штрихпунктир:", LineStyleType::DashDotThin);
    // Можно добавить остальные при необходимости

    layout->addWidget(paramsGroup);

    auto* action = new QWidgetAction(this);
    action->setDefaultWidget(widget);
    addAction(action);
}
