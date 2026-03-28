#include "LineSettingsMenu.h"
#include "GlobalSettings.h"
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QIcon>
#include <QPixmap>

// Статический метод для получения пути к иконке типа линии
QString LineSettingsMenu::getIconPath(LineStyleType type) {
    switch (type) {
        case LineStyleType::SolidMain:
            return ":/icons/line-solid-main.svg";
        case LineStyleType::SolidThin:
            return ":/icons/line-solid-thin.svg";
        case LineStyleType::SolidWavy:
            return ":/icons/line-wavy.svg";
        case LineStyleType::SolidZigZag:
            return ":/icons/line-zigzag.svg";
        case LineStyleType::Dashed:
            return ":/icons/line-dashed.svg";
        case LineStyleType::DashDotThin:
            return ":/icons/line-dashdot-thin.svg";
        case LineStyleType::DashDotThick:
            return ":/icons/line-dashdot-thick.svg";
        case LineStyleType::DashDotDot:
            return ":/icons/line-dashdotdot.svg";
        case LineStyleType::Custom:
            return ":/icons/line-custom.svg";
        default:
            return "";
    }
}

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

    // --- Типы линий (с иконками) ---
    auto* typesGroup = new QGroupBox("Типы линий");
    auto* typesGrid = new QGridLayout(typesGroup);
    typesGrid->setSpacing(8);
    
    auto addLineType = [&](int row, LineStyleType type, const QString& name) {
        QString iconPath = getIconPath(type);
        auto* iconLabel = new QLabel();
        iconLabel->setFixedSize(40, 20);
        iconLabel->setStyleSheet("background-color: transparent;");
        QIcon icon(iconPath);
        if (!icon.isNull()) {
            iconLabel->setPixmap(icon.pixmap(40, 20));
        }
        auto* nameLabel = new QLabel(name);
        nameLabel->setStyleSheet("font-weight: normal; color: #F0F0F0;");
        
        typesGrid->addWidget(iconLabel, row, 0);
        typesGrid->addWidget(nameLabel, row, 1);
    };
    
    addLineType(0, LineStyleType::SolidMain, "Сплошная основная");
    addLineType(1, LineStyleType::SolidThin, "Сплошная тонкая");
    addLineType(2, LineStyleType::Dashed, "Штриховая");
    addLineType(3, LineStyleType::DashDotThin, "Штрихпунктирная тонкая");
    addLineType(4, LineStyleType::DashDotThick, "Штрихпунктирная толстая");
    addLineType(5, LineStyleType::DashDotDot, "Штрихпунктирная с двумя точками");
    addLineType(6, LineStyleType::SolidWavy, "Волнистая");
    addLineType(7, LineStyleType::SolidZigZag, "С изломами");
    
    layout->addWidget(typesGroup);

    // --- Глобальные множители (ОБЩИЕ ДЛЯ ВСЕХ) ---
    auto* globalGroup = new QGroupBox("Общие настройки (для всех линий)");
    auto* globalForm = new QFormLayout(globalGroup);

    // Толщина основной линии (s): 0.5-2.0 мм, по умолчанию 1.2 мм
    auto* mainWidth = createDoubleSpin(GlobalSettings::instance().mainLineWidth, 0.5, 2.0, 0.05);
    mainWidth->setSuffix(" мм");
    connect(mainWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().mainLineWidth = v; 
    });
    globalForm->addRow("Толщина основной (s):", mainWidth);
    
    // Толщина тонкой линии (s/3..s/2): 0.1-0.7 мм, по умолчанию 0.3 мм
    auto* thinWidth = createDoubleSpin(GlobalSettings::instance().thinLineWidth, 0.1, 0.7, 0.05);
    thinWidth->setSuffix(" мм");
    connect(thinWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().thinLineWidth = v; 
    });
    globalForm->addRow("Толщина тонкой (s/2):", thinWidth);

    auto* wScale = createDoubleSpin(GlobalSettings::instance().globalWidthScale, 0.1, 10.0, 0.01);
    connect(wScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().globalWidthScale = v; 
    });
    globalForm->addRow("Масштаб толщины:", wScale);

    auto* ltScale = createDoubleSpin(GlobalSettings::instance().globalLinetypeScale, 0.1, 10.0, 0.01);
    connect(ltScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().globalLinetypeScale = v; 
    });
    globalForm->addRow("Масштаб штриха:", ltScale);

    layout->addWidget(globalGroup);

    // --- Параметры штриховых линий ---
    auto* paramsGroup = new QGroupBox("Параметры штриховых линий");
    auto* paramsLayout = new QFormLayout(paramsGroup);

    auto addParamRow = [&](const QString& name, LineStyleType type) {
        auto* dash = createDoubleSpin(GlobalSettings::instance().styleParams[type].dash, 0.5, 50.0, 0.01);
        auto* gap = createDoubleSpin(GlobalSettings::instance().styleParams[type].gap, 0.5, 50.0, 0.01);

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

        // Создаём метку с иконкой и текстом
        auto* labelWidget = new QWidget();
        auto* labelLayout = new QHBoxLayout(labelWidget);
        labelLayout->setContentsMargins(0,0,0,0);
        labelLayout->setSpacing(6);
        
        // Иконка (используем QIcon для корректного рендеринга SVG)
        QString iconPath = getIconPath(type);
        auto* iconLabel = new QLabel();
        iconLabel->setFixedSize(32, 16);
        QIcon icon(iconPath);
        if (!icon.isNull()) {
            iconLabel->setPixmap(icon.pixmap(32, 16));
        }
        labelLayout->addWidget(iconLabel);
        labelLayout->addWidget(new QLabel(name));
        
        paramsLayout->addRow(labelWidget, rowWidget);
    };

    addParamRow("Штриховая:", LineStyleType::Dashed);
    addParamRow("Штрихпунктир:", LineStyleType::DashDotThin);

    layout->addWidget(paramsGroup);

    // --- Параметры волнистой линии ---
    auto* wavyGroup = new QGroupBox("Волнистая линия");
    auto* wavyLayout = new QFormLayout(wavyGroup);

    auto* wavyAmp = createDoubleSpin(GlobalSettings::instance().wavyParams.amplitude, 0.5, 20.0, 0.01);
    connect(wavyAmp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().wavyParams.amplitude = v; 
    });
    wavyLayout->addRow("Амплитуда:", wavyAmp);

    auto* wavyPeriod = createDoubleSpin(GlobalSettings::instance().wavyParams.period, 5.0, 100.0, 0.01);
    connect(wavyPeriod, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().wavyParams.period = v; 
    });
    wavyLayout->addRow("Период:", wavyPeriod);

    layout->addWidget(wavyGroup);

    // --- Параметры линии с изломами ---
    auto* zigzagGroup = new QGroupBox("Линия с изломами");
    auto* zigzagLayout = new QFormLayout(zigzagGroup);

    auto* zigzagAmp = createDoubleSpin(GlobalSettings::instance().zigzagParams.amplitude, 0.5, 20.0, 0.01);
    connect(zigzagAmp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().zigzagParams.amplitude = v; 
    });
    zigzagLayout->addRow("Высота излома:", zigzagAmp);

    auto* zigzagStraight = createDoubleSpin(GlobalSettings::instance().zigzagParams.straightLength, 5.0, 100.0, 0.01);
    connect(zigzagStraight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().zigzagParams.straightLength = v; 
    });
    zigzagLayout->addRow("Прямой участок:", zigzagStraight);

    auto* zigzagBreak = createDoubleSpin(GlobalSettings::instance().zigzagParams.breakLength, 1.0, 30.0, 0.01);
    connect(zigzagBreak, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double v){ 
        GlobalSettings::instance().zigzagParams.breakLength = v; 
    });
    zigzagLayout->addRow("Длина излома:", zigzagBreak);

    layout->addWidget(zigzagGroup);

    auto* action = new QWidgetAction(this);
    action->setDefaultWidget(widget);
    addAction(action);
}
