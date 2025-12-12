#pragma once
#include "Enums.h"
#include <map>

struct StyleParams {
    double dash = 4.0;
    double gap = 2.0;
    double dash2 = 0.0; // Для сложных линий (второй штрих/точка)
    double gap2 = 0.0;
};

class GlobalSettings {
public:
    static GlobalSettings& instance() {
        static GlobalSettings s;
        return s;
    }

    // Глобальные множители
    double globalWidthScale = 1.0;
    double globalLinetypeScale = 1.0; // Влияет на размер штрихов

    // Параметры стандартных типов
    std::map<LineStyleType, StyleParams> styleParams;

private:
    GlobalSettings() {
        // Дефолтные настройки по ГОСТ (условно)
        styleParams[LineStyleType::Dashed] = {8.0, 3.0};
        styleParams[LineStyleType::DashDotThin] = {10.0, 3.0, 1.0, 3.0}; // Длинный, пробел, точка(короткий), пробел
        styleParams[LineStyleType::DashDotThick] = {8.0, 3.0, 1.0, 3.0};
        styleParams[LineStyleType::DashDotDot] = {10.0, 3.0, 1.0, 2.0};
    }
};
