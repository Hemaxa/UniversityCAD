#pragma once

#include <QString>
#include <QColor>
#include <map>
#include <vector>
#include <memory>
#include "Object.h"

class Scene;

// Пара код-значение DXF
struct DxfPair {
    int code;
    QString value;
};

// Блок DXF, содержит список распарсенных пар для сущностей внутри блока
struct DxfBlock {
    QString name;
    Point basePoint;
    std::vector<std::vector<DxfPair>> entities; // Каждая сущность - это список своих пар
};

// Модуль для импорта DXF файлов в сцену.
class DxfImporter {
public:
    // Импортирует объекты из DXF файла в переданную сцену.
    // Возвращает true при успешном импорте, иначе false.
    static bool importScene(Scene* scene, const QString& filePath);

    // Читает файл Dxf и возвращает плоский список всех пар код-значение
    static std::vector<DxfPair> parseFile(const QString& filePath);
    
    // Преобразует индекс ACI в QColor (с учетом ACI 7 = Black/White)
    static QColor aciToColor(int aci);
    // Преобразует 24-битный TrueColor код 420 в QColor
    static QColor trueColorToColor(int tc);
    
    // Вспомогательная структура для свойств объекта при парсинге
    struct EntityProps {
        QString type;
        QString layer = "0";
        QColor color = Qt::white;
        int lineStyleType = -1; // -1 означает что не найдено (брать умолчание)
        double lineWeight = 0;
        
        // Для POLYLINE..VERTEX
        std::vector<Point> vertices;
        bool isClosedX = false;
        
        // Координаты (например, для LINE/ARC и пр)
        double pt10_x = 0, pt10_y = 0; // x1, y1 / center.x, center.y
        double pt11_x = 0, pt11_y = 0; // x2, y2 / majorAxis.x, majorAxis.y
        double pt40 = 0; // radius / ratio
        double pt50 = 0, pt51 = 0; // startAngle, endAngle
        
        // Для INSERT
        QString blockName;
        
        // XData: оригинальный тип объекта (для round-trip wavy/zigzag)
        QString origType; // "Circle", "Ellipse", etc.
        double origCX = 0, origCY = 0; // Центр оригинального объекта
        double origR = 0;              // Радиус (для Circle)
        double origRX = 0, origRY = 0; // Радиусы (для Ellipse)
        double origStartAngle = 0, origSpanAngle = 0; // Углы (для Arc)
    };
    
    // Извлечение общих свойств из набора пар
    static void extractCommonProps(const std::vector<DxfPair>& pairs, EntityProps& props);
    
    // Создание объекта нужного класса по распарсенным свойствам (возвращает std::unique_ptr)
    // Если объект состоит только из вершин (LWPOLYLINE / аппроксимированные сложные примитивы), 
    // алгоритм постарается создать наиболее подходящий объект (Spline или просто набор Segments / Polygon)
    static std::unique_ptr<Object> createEntity(const EntityProps& props);
    
    // Рекурсивная вставка блока
    static void instantiateBlock(const QString& blockName, 
                                 double x, double y, 
                                 const std::map<QString, DxfBlock>& blocksMap, 
                                 Scene* scene, 
                                 int recursionDepth);
};
