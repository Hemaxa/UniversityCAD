#include "DxfImporter.h"
#include "Scene.h"
#include "Segment.h"
#include "Circle.h"
#include "Arc.h"
#include "Rectangle.h"
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include "PointObject.h"
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <iostream>

// Простой парсинг файла DXF на пары
std::vector<DxfPair> DxfImporter::parseFile(const QString& filePath) {
    std::vector<DxfPair> pairs;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return pairs;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString codeStr = in.readLine().trimmed();
        if (codeStr.isEmpty()) continue;
        bool ok;
        int code = codeStr.toInt(&ok);
        if (!ok) continue; // Нарушен формат Dxf
        
        QString value = in.readLine().trimmed();
        pairs.push_back({code, value});
    }
    return pairs;
}

// Преобразование ACI в QColor
QColor DxfImporter::aciToColor(int aci) {
    switch (aci) {
        case 1: return QColor(255, 0, 0);     // Red
        case 2: return QColor(255, 255, 0);   // Yellow
        case 3: return QColor(0, 255, 0);     // Green
        case 4: return QColor(0, 255, 255);   // Cyan
        case 5: return QColor(0, 0, 255);     // Blue
        case 6: return QColor(255, 0, 255);   // Magenta
        case 7: return Qt::white;             // White/Black
        case 8: return QColor(128, 128, 128); // Dark Gray
        case 9: return QColor(192, 192, 192); // Light Gray
        default: return Qt::white;
    }
}

// Преобразование TrueColor в QColor
QColor DxfImporter::trueColorToColor(int tc) {
    int r = (tc >> 16) & 0xFF;
    int g = (tc >> 8) & 0xFF;
    int b = tc & 0xFF;
    return QColor(r, g, b);
}

// Извлечение общих свойств из вектора пар сущности
void DxfImporter::extractCommonProps(const std::vector<DxfPair>& pairs, DxfImporter::EntityProps& props) {
    bool hasTrueColor = false;
    bool hasClarusData = false;
    int clarusLType = 0;
    
    for (size_t i = 0; i < pairs.size(); ++i) {
        int code = pairs[i].code;
        const QString& val = pairs[i].value;
        
        if (code == 8) props.layer = val;
        else if (code == 370) props.lineWeight = val.toInt() / 100.0;
        else if (code == 62 && !hasTrueColor) {
            props.color = aciToColor(val.toInt());
        }
        else if (code == 420) { props.color = trueColorToColor(val.toInt()); hasTrueColor = true; }
        else if (code == 6) {
            // Стандартные DXF типы линий → наш LineStyleType
            QString upper = val.toUpper().trimmed();
            if (upper == "DASHED" || (upper.contains("DASH") && !upper.contains("DOT")))
                props.lineStyleType = static_cast<int>(LineStyleType::Dashed);
            else if (upper == "DASHDOT" || upper == "CENTER" || upper.contains("CENTER"))
                props.lineStyleType = static_cast<int>(LineStyleType::DashDotThin);
            else if (upper == "DIVIDE" || upper.contains("PHANTOM") || upper.contains("DIVIDE"))
                props.lineStyleType = static_cast<int>(LineStyleType::DashDotDot);
            // CONTINUOUS, BYLAYER, BYBLOCK — оставляем по умолчанию
        }
        else if (code == 999) {
            if (val.startsWith("CLARUSCAD_LTYPE:")) {
                hasClarusData = true;
                clarusLType = val.mid(16).toInt();
            }
        }
        else if (code == 10) props.pt10_x = val.toDouble();
        else if (code == 20) props.pt10_y = val.toDouble();
        else if (code == 11) props.pt11_x = val.toDouble();
        else if (code == 21) props.pt11_y = val.toDouble();
        else if (code == 40) props.pt40 = val.toDouble();
        else if (code == 50) props.pt50 = val.toDouble();
        else if (code == 51) props.pt51 = val.toDouble();
        else if (code == 70) props.isClosedX = (val.toInt() & 1);
        else if (code == 2) {
            if (props.type == "INSERT") props.blockName = val;
        }
    }
    
    // CLARUSCAD_LTYPE имеет приоритет над DXF code 6
    if (hasClarusData) {
        props.lineStyleType = clarusLType;
    }
    
    // Сбор точек для LWPOLYLINE
    if (props.type == "LWPOLYLINE") {
        double curX = 0, curY = 0;
        bool hasX = false;
        for (const auto& p : pairs) {
            if (p.code == 10) { curX = p.value.toDouble(); hasX = true; }
            else if (p.code == 20 && hasX) { 
                curY = p.value.toDouble(); 
                props.vertices.emplace_back(curX, curY);
                hasX = false;
            }
        }
    }
}

bool DxfImporter::importScene(Scene* scene, const QString& filePath) {
    if (!scene) return false;
    
    auto pairs = parseFile(filePath);
    if (pairs.empty()) return false;
    
    std::map<QString, DxfBlock> blocksMap;
    std::vector<std::vector<DxfPair>> mainEntities;
    
    bool inBlocks = false;
    bool inEntities = false;
    
    DxfBlock currentBlock;
    bool inBlockDef = false;
    
    std::vector<DxfPair> currentEntity;
    bool buildingPolyline = false;
    EntityProps currentPolylineProps;
    
    // Функция для завершения текущей базовой сущности (LINE, CIRCLE, INSERT, etc)
    auto finishEntity = [&]() {
        if (!currentEntity.empty()) {
            // Если это VERTEX или SEQEND, они относятся к POLYLINE
            QString type;
            for (const auto& p : currentEntity) { if (p.code == 0) { type = p.value; break; } }
            
            if (type == "VERTEX" && buildingPolyline) {
                EntityProps vp;
                extractCommonProps(currentEntity, vp);
                currentPolylineProps.vertices.emplace_back(vp.pt10_x, vp.pt10_y);
            } 
            else if (type == "SEQEND" && buildingPolyline) {
                // Завершение POLYLINE
                if (inBlockDef) {
                    // Копируем свойства POLYLINE обратно в виде набора пар (упрощенно: просто сохраняем точки в 10/20)
                    std::vector<DxfPair> fakeLwPoly;
                    fakeLwPoly.push_back({0, "LWPOLYLINE"});
                    fakeLwPoly.push_back({8, currentPolylineProps.layer});
                    if (currentPolylineProps.lineStyleType != -1)
                        fakeLwPoly.push_back({999, "CLARUSCAD_LTYPE:" + QString::number(currentPolylineProps.lineStyleType)});
                    fakeLwPoly.push_back({70, currentPolylineProps.isClosedX ? "1" : "0"});
                    for (const auto& v : currentPolylineProps.vertices) {
                        fakeLwPoly.push_back({10, QString::number(v.getX())});
                        fakeLwPoly.push_back({20, QString::number(v.getY())});
                    }
                    currentBlock.entities.push_back(fakeLwPoly);
                } else {
                    // Создаем объект напрямую
                    auto obj = createEntity(currentPolylineProps);
                    if (obj) scene->addPrimitive(std::move(obj));
                }
                buildingPolyline = false;
            }
            else if (type == "POLYLINE") {
                buildingPolyline = true;
                currentPolylineProps = EntityProps();
                currentPolylineProps.type = "POLYLINE";
                extractCommonProps(currentEntity, currentPolylineProps);
            }
            else if (type != "ENDBLK" && type != "BLOCK") {
                if (inBlockDef) {
                    currentBlock.entities.push_back(currentEntity);
                } else if (inEntities) {
                    mainEntities.push_back(currentEntity);
                }
            }
            currentEntity.clear();
        }
    };
    
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (pairs[i].code == 0 && pairs[i].value == "SECTION") {
            if (i + 1 < pairs.size() && pairs[i+1].code == 2) {
                if (pairs[i+1].value == "BLOCKS") inBlocks = true;
                else if (pairs[i+1].value == "ENTITIES") {
                    inEntities = true;
                    inBlocks = false;
                }
            }
        } 
        else if (pairs[i].code == 0 && pairs[i].value == "ENDSEC") {
            finishEntity();
            inBlocks = false;
            inEntities = false;
        }
        else if (pairs[i].code == 0) {
            finishEntity();
            currentEntity.push_back(pairs[i]);
            
            if (pairs[i].value == "BLOCK" && inBlocks) {
                inBlockDef = true;
                currentBlock = DxfBlock();
            } else if (pairs[i].value == "ENDBLK" && inBlocks) {
                inBlockDef = false;
                // Игнорируем системные блоки (начинающиеся с *)
                if (!currentBlock.name.startsWith("*")) {
                    blocksMap[currentBlock.name] = currentBlock;
                }
            }
        } 
        else {
            if (!currentEntity.empty()) {
                currentEntity.push_back(pairs[i]);
            }
            // Считываем имя блока и базовую точку
            if (inBlockDef && pairs[i].code == 2 && currentBlock.name.isEmpty()) {
                currentBlock.name = pairs[i].value;
            }
            if (inBlockDef && pairs[i].code == 10 && currentEntity.size() < 3) currentBlock.basePoint.setX(pairs[i].value.toDouble());
            if (inBlockDef && pairs[i].code == 20 && currentEntity.size() < 4) currentBlock.basePoint.setY(pairs[i].value.toDouble());
        }
    }
    finishEntity();
    
    // Инстанцируем примитивы из mainEntities
    for (const auto& entityPairs : mainEntities) {
        QString type;
        for (const auto& p : entityPairs) { if (p.code == 0) { type = p.value; break; } }
        
        EntityProps props;
        props.type = type;
        extractCommonProps(entityPairs, props);
        
        if (type == "INSERT") {
            instantiateBlock(props.blockName, props.pt10_x, props.pt10_y, blocksMap, scene, 0);
        } else if (type == "LWPOLYLINE") {
            auto obj = createEntity(props);
            if (obj) scene->addPrimitive(std::move(obj));
        } else {
            auto obj = createEntity(props);
            if (obj) scene->addPrimitive(std::move(obj));
        }
    }
    
    return true;
}

std::unique_ptr<Object> DxfImporter::createEntity(const EntityProps& props) {
    std::unique_ptr<Object> obj;
    
    if (props.type == "LINE") {
        obj = std::make_unique<Segment>(Point(props.pt10_x, props.pt10_y), Point(props.pt11_x, props.pt11_y));
    } else if (props.type == "CIRCLE") {
        obj = std::make_unique<Circle>(Point(props.pt10_x, props.pt10_y), props.pt40);
    } else if (props.type == "ARC") {
        double span = props.pt51 - props.pt50;
        if (span < 0) span += 360.0;
        obj = std::make_unique<Arc>(Point(props.pt10_x, props.pt10_y), props.pt40, props.pt50, span);
    } else if (props.type == "ELLIPSE") {
        double majorLen = std::sqrt(props.pt11_x * props.pt11_x + props.pt11_y * props.pt11_y);
        obj = std::make_unique<Ellipse>(Point(props.pt10_x, props.pt10_y), majorLen, majorLen * props.pt40);
    } else if (props.type == "POLYLINE" || props.type == "LWPOLYLINE") {
        if (props.vertices.size() < 2) return nullptr;
        
        if (props.vertices.size() == 2) {
            obj = std::make_unique<Segment>(props.vertices[0], props.vertices[1]);
        } else if (props.vertices.size() >= 4 && props.isClosedX) {
            if (props.vertices.size() == 4) {
                double minX = props.vertices[0].getX(), maxX = props.vertices[0].getX();
                double minY = props.vertices[0].getY(), maxY = props.vertices[0].getY();
                for (const auto& p : props.vertices) {
                    minX = std::min(minX, p.getX()); maxX = std::max(maxX, p.getX());
                    minY = std::min(minY, p.getY()); maxY = std::max(maxY, p.getY());
                }
                obj = std::make_unique<Rectangle>(Point(minX, maxY), maxX - minX, maxY - minY);
            } else {
                double sumX = 0, sumY = 0;
                for (const auto& p : props.vertices) { sumX += p.getX(); sumY += p.getY(); }
                Point center(sumX / props.vertices.size(), sumY / props.vertices.size());
                double dx = props.vertices[0].getX() - center.getX();
                double dy = props.vertices[0].getY() - center.getY();
                double radius = std::sqrt(dx*dx + dy*dy);
                obj = std::make_unique<PolygonObj>(center, radius, (int)props.vertices.size(), true);
            }
        } else {
            obj = std::make_unique<Spline>(props.vertices);
        }
    } else if (props.type == "POINT") {
        obj = std::make_unique<PointObject>(Point(props.pt10_x, props.pt10_y));
    }
    
    if (obj) {
        obj->setLayer(props.layer);
        obj->setColor(props.color);
        LineStyle style = obj->getLineStyle();
        if (props.lineStyleType != -1) {
            style.type = static_cast<LineStyleType>(props.lineStyleType);
        }
        if (props.lineWeight > 0) {
            style.customWidth = props.lineWeight;
        }
        obj->setLineStyle(style);
    }
    
    return obj;
}

void DxfImporter::instantiateBlock(const QString& blockName, 
                                   double x, double y, 
                                   const std::map<QString, DxfBlock>& blocksMap, 
                                   Scene* scene, 
                                   int recursionDepth) 
{
    if (recursionDepth >= 10) return; // Защита от потенциального зацикливания
    
    auto it = blocksMap.find(blockName);
    if (it == blocksMap.end()) return;
    
    const DxfBlock& block = it->second;
    double dx = x - block.basePoint.getX();
    double dy = y - block.basePoint.getY();
    
    for (const auto& entityPairs : block.entities) {
        QString type;
        for (const auto& p : entityPairs) { if (p.code == 0) { type = p.value; break; } }
        
        EntityProps props;
        props.type = type;
        extractCommonProps(entityPairs, props);
        
        if (type == "INSERT") {
            instantiateBlock(props.blockName, props.pt10_x + dx, props.pt10_y + dy, blocksMap, scene, recursionDepth + 1);
        } else {
            // Применяем смещение ко всем координатам
            props.pt10_x += dx; props.pt10_y += dy;
            props.pt11_x += dx; props.pt11_y += dy;
            for (auto& v : props.vertices) {
                v.setX(v.getX() + dx);
                v.setY(v.getY() + dy);
            }
            
            auto obj = createEntity(props);
            if (obj) scene->addPrimitive(std::move(obj));
        }
    }
}
