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

QDoubleSpinBox* LineSettingsMenu::createDoubleSpin(double val, double min, double max, double step) {
    auto* sb = new QDoubleSpinBox();
    sb->setRange(min, max);
    sb->setSingleStep(step);
    sb->setValue(val);
    sb->setMinimumWidth(70);
    connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double){
        emit settingsChanged();
    });
    return sb;
}

void LineSettingsMenu::setupUi() {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);

    // --- Глобальные множители (ОБЩИЕ ДЛЯ ВСЕХ) ---
    auto* globalGroup = new QGroupBox("Общие настройки (для всех линий)");
    auto* globalForm = new QFormLayout(globalGroup);

    auto* wScale = createDoubleSpin(GlobalSettings::instance().globalWidthScale, 0.1, 10.0, 0.1);
    connect(wScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().globalWidthScale = v; 
    });
    globalForm->addRow("Масштаб толщины:", wScale);

    auto* ltScale = createDoubleSpin(GlobalSettings::instance().globalLinetypeScale, 0.1, 10.0, 0.1);
    connect(ltScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().globalLinetypeScale = v; 
    });
    globalForm->addRow("Масштаб штриха:", ltScale);

    layout->addWidget(globalGroup);

    // --- Параметры штриховых линий ---
    auto* paramsGroup = new QGroupBox("Параметры штриховых линий");
    auto* paramsLayout = new QFormLayout(paramsGroup);

    auto addParamRow = [&](const QString& name, LineStyleType type) {
        auto* dash = createDoubleSpin(GlobalSettings::instance().styleParams[type].dash, 0.5, 50.0, 0.5);
        auto* gap = createDoubleSpin(GlobalSettings::instance().styleParams[type].gap, 0.5, 50.0, 0.5);

        connect(dash, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [type](double v){ 
            GlobalSettings::instance().styleParams[type].dash = v; 
        });
        connect(gap, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [type](double v){ 
            GlobalSettings::instance().styleParams[type].gap = v; 
        });

        auto* rowWidget = new QWidget();
        auto* rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(0,0,0,0);
        rowLayout->addWidget(new QLabel("Штрих:")); rowLayout->addWidget(dash);
        rowLayout->addWidget(new QLabel("Пробел:")); rowLayout->addWidget(gap);

        paramsLayout->addRow(name, rowWidget);
    };

    addParamRow("Штриховая:", LineStyleType::Dashed);
    addParamRow("Штрихпунктир:", LineStyleType::DashDotThin);

    layout->addWidget(paramsGroup);

    // --- Параметры волнистой линии ---
    auto* wavyGroup = new QGroupBox("Волнистая линия");
    auto* wavyLayout = new QFormLayout(wavyGroup);

    auto* wavyAmp = createDoubleSpin(GlobalSettings::instance().wavyParams.amplitude, 0.5, 20.0, 0.5);
    connect(wavyAmp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().wavyParams.amplitude = v; 
    });
    wavyLayout->addRow("Амплитуда:", wavyAmp);

    auto* wavyPeriod = createDoubleSpin(GlobalSettings::instance().wavyParams.period, 5.0, 100.0, 1.0);
    connect(wavyPeriod, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().wavyParams.period = v; 
    });
    wavyLayout->addRow("Период:", wavyPeriod);

    layout->addWidget(wavyGroup);

    // --- Параметры линии с изломами ---
    auto* zigzagGroup = new QGroupBox("Линия с изломами");
    auto* zigzagLayout = new QFormLayout(zigzagGroup);

    auto* zigzagAmp = createDoubleSpin(GlobalSettings::instance().zigzagParams.amplitude, 0.5, 20.0, 0.5);
    connect(zigzagAmp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().zigzagParams.amplitude = v; 
    });
    zigzagLayout->addRow("Высота излома:", zigzagAmp);

    auto* zigzagStraight = createDoubleSpin(GlobalSettings::instance().zigzagParams.straightLength, 5.0, 100.0, 1.0);
    connect(zigzagStraight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().zigzagParams.straightLength = v; 
    });
    zigzagLayout->addRow("Прямой участок:", zigzagStraight);

    auto* zigzagBreak = createDoubleSpin(GlobalSettings::instance().zigzagParams.breakLength, 1.0, 30.0, 0.5);
    connect(zigzagBreak, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().zigzagParams.breakLength = v; 
    });
    zigzagLayout->addRow("Длина излома:", zigzagBreak);

    layout->addWidget(zigzagGroup);

    auto* action = new QWidgetAction(this);
    action->setDefaultWidget(widget);
    addAction(action);
}
