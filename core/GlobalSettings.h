#pragma once
#include "Object.h"
#include <map>

struct StyleParams {
    double dash = 4.0;
    double gap = 2.0;
    double dash2 = 0.0; // Для сложных линий (второй штрих/точка)
    double gap2 = 0.0;
};

// Параметры для волнистой линии
struct WavyParams {
    double amplitude = 2.0;   // Амплитуда волны (меньше по ГОСТ)
    double period = 15.0;     // Период волны (больше по ГОСТ для плавности)
};

// Параметры для линии с изломами (по ГОСТ 2.303-68)
// Паттерн: прямой участок → излом вверх → прямой участок → излом вниз → ...
struct ZigZagParams {
    double amplitude = 3.0;       // Высота излома
    double straightLength = 15.0; // Длина прямого участка между изломами
    double breakLength = 5.0;     // Длина самого излома (наклонный участок)
};

struct DimensionStyle {
    QColor extensionColor = Qt::white;
    QColor dimensionColor = Qt::white;
    QColor textColor = Qt::white;
    LineStyle extensionLineStyle = {LineStyleType::SolidThin, "Сплошная тонкая", 0, 0, false};
    LineStyle dimensionLineStyle = {LineStyleType::SolidThin, "Сплошная тонкая", 0, 0, false};
    double extensionOvershoot = 8.0;
    double dimensionExtension = 0.0;
    ArrowType arrowType = ArrowType::Closed;
    ArrowPlacement arrowPlacement = ArrowPlacement::Inside;
    double arrowSize = 12.0;
    bool arrowFilled = true;
    QString fontFamily = "Courier New";
    double textHeight = 16.0;
    double textOffset = 10.0;
    DimensionValuePrefix linearPrefix = DimensionValuePrefix::None;
};

class GlobalSettings {
public:
    static GlobalSettings& instance() {
        static GlobalSettings s;
        return s;
    }

    // Глобальные множители
    double globalWidthScale = 1.0;       // Общий масштаб толщины для ВСЕХ линий
    double globalLinetypeScale = 1.0;    // Влияет на размер штрихов
    
    // Толщина линий по ГОСТ 2.303-68
    // Основная толстая линия (s): 0.5-1.4 мм, по умолчанию 1.2 мм
    double mainLineWidth = 1.2;
    // Тонкие линии (s/3 .. s/2): 0.2-0.5 мм, по умолчанию 0.3 мм
    double thinLineWidth = 0.3;

    // Параметры стандартных типов
    std::map<LineStyleType, StyleParams> styleParams;

    // Параметры волнистой линии (общие)
    WavyParams wavyParams;
    
    // Параметры линии с изломами (общие)
    ZigZagParams zigzagParams;

    DimensionStyle dimensionStyle;

private:
    GlobalSettings() {
        // Дефолтные настройки по ГОСТ 2.303-68
        styleParams[LineStyleType::Dashed] = {8.0, 3.0};
        styleParams[LineStyleType::DashDotThin] = {10.0, 3.0, 1.0, 3.0};
        styleParams[LineStyleType::DashDotThick] = {8.0, 3.0, 1.0, 3.0};
        styleParams[LineStyleType::DashDotDot] = {10.0, 3.0, 1.0, 2.0};
        
        // По ГОСТ волнистая линия должна быть плавной
        wavyParams = {2.0, 15.0};
        
        // Линия с изломами: прямой участок 15, излом 5, амплитуда 3
        zigzagParams = {3.0, 15.0, 5.0};
    }
};
