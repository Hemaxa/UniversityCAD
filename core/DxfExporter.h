#pragma once

#include "Point.h"
#include <QString>
#include <QColor>
#include <vector>

class Scene;
class Object;
class QTextStream;

// Модуль для экспорта сцены в формат DXF (AC1015)
class DxfExporter {
public:
    // Экспортирует все объекты сцены в указанный файл.
    static bool exportScene(const Scene* scene, const QString& filePath);

    // Преобразует RGB цвет QColor в ближайший индекс ACI (AutoCAD Color Index, 1-255).
    static int colorToACI(const QColor& color);
    
    // Преобразует RGB цвет в 24-битное значение TrueColor для кода 420.
    static int colorToTrueColor(const QColor& color);

    // Генерирует строку имени типа линии на основе LineStyleType.
    static QString getDxfLinetype(int lineStyleTypeEnum);
};
