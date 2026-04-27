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
#include "Dimension.h"
#include <QFile>
#include <QTextStream>
#include <QUrl>
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
            if (val.startsWith("UNIVERSITYCAD_LTYPE:")) {
                hasClarusData = true;
                clarusLType = val.mid(val.indexOf(':') + 1).toInt();
            }
            else if (val.startsWith("UNIVERSITYCAD_ORIG:")) {
                QString data = val.mid(val.indexOf(':') + 1);
                QStringList parts = data.split('|');
                if (!parts.isEmpty()) {
                    props.origType = parts[0];
                    if (props.origType == "Circle" && parts.size() >= 4) {
                        props.origCX = parts[1].toDouble();
                        props.origCY = parts[2].toDouble();
                        props.origR = parts[3].toDouble();
                    } else if (props.origType == "Ellipse" && parts.size() >= 5) {
                        props.origCX = parts[1].toDouble();
                        props.origCY = parts[2].toDouble();
                        props.origRX = parts[3].toDouble();
                        props.origRY = parts[4].toDouble();
                    } else if (props.origType == "Arc" && parts.size() >= 6) {
                        props.origCX = parts[1].toDouble();
                        props.origCY = parts[2].toDouble();
                        props.origR = parts[3].toDouble();
                        props.origStartAngle = parts[4].toDouble();
                        props.origSpanAngle = parts[5].toDouble();
                    }
                }
            }
            else if (val.startsWith("UNIVERSITYCAD_DIM:")) {
                props.dimensionData = val.mid(val.indexOf(':') + 1);
            }
            else if (val == "UNIVERSITYCAD_DIM_GEOM") {
                props.dimensionVisualEntity = true;
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
    
    // UNIVERSITYCAD_LTYPE имеет приоритет над DXF code 6
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
    
    // Сбор контрольных точек для DXF SPLINE
    if (props.type == "SPLINE") {
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

static void rebindImportedDimensions(Scene* scene);

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
                        fakeLwPoly.push_back({999, "UNIVERSITYCAD_LTYPE:" + QString::number(currentPolylineProps.lineStyleType)});
                    // Передаём xdata оригинального типа
                    if (!currentPolylineProps.origType.isEmpty()) {
                        QString origData;
                        if (currentPolylineProps.origType == "Circle") {
                            origData = QString("UNIVERSITYCAD_ORIG:Circle|%1|%2|%3")
                                .arg(currentPolylineProps.origCX)
                                .arg(currentPolylineProps.origCY)
                                .arg(currentPolylineProps.origR);
                        } else if (currentPolylineProps.origType == "Ellipse") {
                            origData = QString("UNIVERSITYCAD_ORIG:Ellipse|%1|%2|%3|%4")
                                .arg(currentPolylineProps.origCX)
                                .arg(currentPolylineProps.origCY)
                                .arg(currentPolylineProps.origRX)
                                .arg(currentPolylineProps.origRY);
                        } else if (currentPolylineProps.origType == "Arc") {
                            origData = QString("UNIVERSITYCAD_ORIG:Arc|%1|%2|%3|%4|%5")
                                .arg(currentPolylineProps.origCX)
                                .arg(currentPolylineProps.origCY)
                                .arg(currentPolylineProps.origR)
                                .arg(currentPolylineProps.origStartAngle)
                                .arg(currentPolylineProps.origSpanAngle);
                        }
                        if (!origData.isEmpty())
                            fakeLwPoly.push_back({999, origData});
                    }
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

    rebindImportedDimensions(scene);
    
    return true;
}

static QString decodeDimString(const QString& value)
{
    return QUrl::fromPercentEncoding(value.toLatin1());
}

static QColor dimTokenToColor(const QString& value, const QColor& fallback = Qt::white)
{
    QColor color(value);
    return color.isValid() ? color : fallback;
}

static std::unique_ptr<Object> createDimensionFromData(const QString& packed)
{
    const QStringList parts = packed.split('|');
    if (parts.size() < 25 || parts[0] != "1") return nullptr;

    auto toDouble = [&](int index, double fallback = 0.0) {
        bool ok = false;
        const double value = parts[index].toDouble(&ok);
        return ok ? value : fallback;
    };
    auto toInt = [&](int index, int fallback = 0) {
        bool ok = false;
        const int value = parts[index].toInt(&ok);
        return ok ? value : fallback;
    };

    DimensionAnchor a;
    DimensionAnchor b;
    a.fallback = Point(toDouble(2), toDouble(3));
    b.fallback = Point(toDouble(4), toDouble(5));

    const auto type = static_cast<DimensionType>(toInt(1));
    auto dim = std::make_unique<Dimension>(type, a, b, Point(toDouble(6), toDouble(7)));
    dim->setTextPositionFactor(toDouble(8, 0.5));
    dim->setTextOverride(decodeDimString(parts[9]));
    dim->setExtensionColor(dimTokenToColor(parts[10]));
    dim->setDimensionColor(dimTokenToColor(parts[11]));
    dim->setTextColor(dimTokenToColor(parts[12]));

    LineStyle extensionStyle = dim->extensionLineStyle();
    extensionStyle.type = static_cast<LineStyleType>(toInt(13, static_cast<int>(extensionStyle.type)));
    dim->setExtensionLineStyle(extensionStyle);

    LineStyle dimensionStyle = dim->dimensionLineStyle();
    dimensionStyle.type = static_cast<LineStyleType>(toInt(14, static_cast<int>(dimensionStyle.type)));
    dim->setDimensionLineStyle(dimensionStyle);
    dim->setLineStyle(dimensionStyle);

    dim->setExtensionOvershoot(toDouble(15, dim->extensionOvershoot()));
    dim->setDimensionExtension(toDouble(16, dim->dimensionExtension()));
    dim->setArrowType(static_cast<ArrowType>(toInt(17, static_cast<int>(dim->arrowType()))));
    dim->setArrowPlacement(static_cast<ArrowPlacement>(toInt(18, static_cast<int>(dim->arrowPlacement()))));
    dim->setArrowSize(toDouble(19, dim->arrowSize()));
    dim->setArrowFilled(toInt(20, dim->arrowFilled() ? 1 : 0) != 0);
    dim->setFontFamily(decodeDimString(parts[21]));
    dim->setTextHeight(toDouble(22, dim->textHeight()));
    dim->setTextOffset(toDouble(23, dim->textOffset()));
    dim->setAngularRadius(toDouble(24, dim->angularRadius()));
    dim->setUseSupplementaryAngle(toInt(25, dim->useSupplementaryAngle() ? 1 : 0) != 0);
    dim->setColor(dim->dimensionColor());
    return dim;
}

static double pointDistance(const Point& a, const Point& b)
{
    return std::hypot(a.getX() - b.getX(), a.getY() - b.getY());
}

static DimensionAnchor rebindDimensionAnchor(const DimensionAnchor& source, const Scene* scene)
{
    constexpr double tolerance = 1e-3;
    DimensionAnchor best = source;
    double bestDistance = tolerance;

    for (const auto& candidatePtr : scene->getPrimitives()) {
        const Object* candidate = candidatePtr.get();
        if (!candidate || candidate->getType() == PrimitiveType::Dimension) continue;

        const auto snaps = candidate->getSnapPoints();
        for (int i = 0; i < static_cast<int>(snaps.size()); ++i) {
            const double d = pointDistance(source.fallback, snaps[i].p);
            if (d < bestDistance) {
                bestDistance = d;
                best.object = candidate;
                best.snapIndex = snaps[i].index >= 0 ? snaps[i].index : i;
                best.fallback = snaps[i].p;
            }
        }

        const Point closest = candidate->getClosestPoint(source.fallback);
        const double d = pointDistance(source.fallback, closest);
        if (d < bestDistance) {
            bestDistance = d;
            best.object = candidate;
            best.snapIndex = -1;
            best.fallback = closest;
        }
    }

    return best;
}

static void rebindImportedDimensions(Scene* scene)
{
    for (const auto& objPtr : scene->getPrimitives()) {
        if (!objPtr || objPtr->getType() != PrimitiveType::Dimension) continue;
        auto* dimension = static_cast<Dimension*>(objPtr.get());
        dimension->setFirstAnchor(rebindDimensionAnchor(dimension->firstAnchor(), scene));
        dimension->setSecondAnchor(rebindDimensionAnchor(dimension->secondAnchor(), scene));
    }
}

std::unique_ptr<Object> DxfImporter::createEntity(const EntityProps& props) {
    std::unique_ptr<Object> obj;

    if (props.dimensionVisualEntity) {
        return nullptr;
    }

    if (!props.dimensionData.isEmpty()) {
        obj = createDimensionFromData(props.dimensionData);
    }
    
    if (!obj) {
    if (props.type == "LINE") {
        obj = std::make_unique<Segment>(Point(props.pt10_x, props.pt10_y), Point(props.pt11_x, props.pt11_y));
    } else if (props.type == "CIRCLE") {
        obj = std::make_unique<Circle>(Point(props.pt10_x, props.pt10_y), props.pt40);
    } else if (props.type == "ARC") {
        if (!props.origType.isEmpty() && props.origType == "Arc" && props.origR > 0) {
            obj = std::make_unique<Arc>(Point(props.origCX, props.origCY), props.origR, props.origStartAngle, props.origSpanAngle);
        } else {
            // Стандартный импорт
            double dxfStart = props.pt50;
            double dxfEnd = props.pt51;
            
            double dxfSpan = dxfEnd - dxfStart;
            if (dxfSpan <= 0) dxfSpan += 360.0;
            
            double qt_start = std::fmod(360.0 - dxfEnd, 360.0);
            if (qt_start < 0) qt_start += 360.0;
            double qt_span = dxfSpan;
            
            obj = std::make_unique<Arc>(Point(props.pt10_x, props.pt10_y), props.pt40, qt_start, qt_span);
        }
    } else if (props.type == "ELLIPSE") {
        double majorLen = std::sqrt(props.pt11_x * props.pt11_x + props.pt11_y * props.pt11_y);
        double minorLen = majorLen * props.pt40;
        
        // В DXF pt11 указывает направление большой полуоси.
        // Если вектор больше вытянут по Y, значит это вертикальный эллипс.
        if (std::abs(props.pt11_x) >= std::abs(props.pt11_y)) {
            obj = std::make_unique<Ellipse>(Point(props.pt10_x, props.pt10_y), majorLen, minorLen);
        } else {
            obj = std::make_unique<Ellipse>(Point(props.pt10_x, props.pt10_y), minorLen, majorLen);
        }
    } else if (props.type == "POLYLINE" || props.type == "LWPOLYLINE") {
        if (props.vertices.size() < 2) return nullptr;
        
        // 1. Проверяем xdata оригинального типа (round-trip из нашего экспортёра)
        if (!props.origType.isEmpty()) {
            if (props.origType == "Circle" && props.origR > 0) {
                obj = std::make_unique<Circle>(Point(props.origCX, props.origCY), props.origR);
            } else if (props.origType == "Ellipse" && props.origRX > 0 && props.origRY > 0) {
                obj = std::make_unique<Ellipse>(Point(props.origCX, props.origCY), props.origRX, props.origRY);
            } else if (props.origType == "Arc" && props.origR > 0) {
                obj = std::make_unique<Arc>(Point(props.origCX, props.origCY), props.origR, props.origStartAngle, props.origSpanAngle);
            }
        }
        
        // 2. Если xdata не помогла, используем геометрический анализ
        if (!obj) {
            if (props.vertices.size() == 2) {
                obj = std::make_unique<Segment>(props.vertices[0], props.vertices[1]);
            } else if (props.vertices.size() >= 4 && props.isClosedX) {
                // Попытка определить прямоугольник (4 вершины, прямые углы)
                if (props.vertices.size() == 4) {
                    double minX = props.vertices[0].getX(), maxX = props.vertices[0].getX();
                    double minY = props.vertices[0].getY(), maxY = props.vertices[0].getY();
                    for (const auto& p : props.vertices) {
                        minX = std::min(minX, p.getX()); maxX = std::max(maxX, p.getX());
                        minY = std::min(minY, p.getY()); maxY = std::max(maxY, p.getY());
                    }
                    obj = std::make_unique<Rectangle>(Point(minX, maxY), maxX - minX, maxY - minY);
                } else {
                    // Эвристика: проверяем, является ли замкнутая полилиния окружностью или эллипсом
                    // Вычисляем центр масс
                    double sumX = 0, sumY = 0;
                    for (const auto& p : props.vertices) { sumX += p.getX(); sumY += p.getY(); }
                    Point center(sumX / props.vertices.size(), sumY / props.vertices.size());
                    
                    // Проверяем, лежат ли все точки на одной окружности
                    double dx0 = props.vertices[0].getX() - center.getX();
                    double dy0 = props.vertices[0].getY() - center.getY();
                    double r0 = std::sqrt(dx0*dx0 + dy0*dy0);
                    
                    if (r0 > 1e-6 && props.vertices.size() >= 8) {
                        // Проверка на окружность: все расстояния от центра одинаковы
                        bool isCircle = true;
                        double maxDeviation = 0;
                        for (const auto& p : props.vertices) {
                            double dx = p.getX() - center.getX();
                            double dy = p.getY() - center.getY();
                            double r = std::sqrt(dx*dx + dy*dy);
                            double dev = std::abs(r - r0) / r0;
                            maxDeviation = std::max(maxDeviation, dev);
                        }
                        
                        if (maxDeviation < 0.02) { // 2% допуск
                            obj = std::make_unique<Circle>(center, r0);
                        } else {
                            // Проверка на эллипс: вычисляем min/max расстояния по осям
                            double maxDX = 0, maxDY = 0;
                            for (const auto& p : props.vertices) {
                                maxDX = std::max(maxDX, std::abs(p.getX() - center.getX()));
                                maxDY = std::max(maxDY, std::abs(p.getY() - center.getY()));
                            }
                            
                            if (maxDX > 1e-6 && maxDY > 1e-6) {
                                // Проверяем, лежат ли точки на эллипсе (x/rx)^2 + (y/ry)^2 ≈ 1
                                bool isEllipse = true;
                                double maxEllDev = 0;
                                for (const auto& p : props.vertices) {
                                    double nx = (p.getX() - center.getX()) / maxDX;
                                    double ny = (p.getY() - center.getY()) / maxDY;
                                    double dev = std::abs(nx*nx + ny*ny - 1.0);
                                    maxEllDev = std::max(maxEllDev, dev);
                                }
                                
                                if (maxEllDev < 0.05) { // 5% допуск для эллипса
                                    obj = std::make_unique<Ellipse>(center, maxDX, maxDY);
                                }
                            }
                        }
                    }
                    
                    // Если не окружность и не эллипс — полигон
                    if (!obj) {
                        double radius = r0;
                        obj = std::make_unique<PolygonObj>(center, radius, (int)props.vertices.size(), true);
                    }
                }
            } else {
                obj = std::make_unique<Spline>(props.vertices);
            }
        }
    } else if (props.type == "SPLINE") {
        // DXF SPLINE → Spline объект
        // Используем контрольные точки как опорные точки сплайна.
        // Для составного Bezier (4 точки на сегмент) извлекаем точки на кривой:
        // P0, (skip CP1, CP2), P1, (skip CP1, CP2), P2, ...
        if (props.vertices.size() >= 2) {
            // Проверяем, является ли это составной Bezier кривой (наш экспорт)
            // Количество CP = 3*k + 1 для k сегментов
            int numCPs = (int)props.vertices.size();
            if (numCPs >= 4 && (numCPs - 1) % 3 == 0) {
                // Это составной Bezier — извлекаем точки на кривой
                std::vector<Point> onCurvePoints;
                onCurvePoints.push_back(props.vertices[0]);
                for (int i = 3; i < numCPs; i += 3) {
                    onCurvePoints.push_back(props.vertices[i]);
                }
                obj = std::make_unique<Spline>(onCurvePoints);
            } else {
                // Общий случай — используем все точки
                obj = std::make_unique<Spline>(props.vertices);
            }
        }
    } else if (props.type == "POINT") {
        obj = std::make_unique<PointObject>(Point(props.pt10_x, props.pt10_y));
    }
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
