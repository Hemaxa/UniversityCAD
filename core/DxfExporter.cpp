#include "DxfExporter.h"
#include "Scene.h"
#include "Object.h"
#include "Point.h"
#include "Segment.h"
#include "Circle.h"
#include "Arc.h"
#include "Rectangle.h"
#include "Ellipse.h"
#include "Polygon.h"
#include "Spline.h"
#include "PointObject.h"
#include "GlobalSettings.h"
#include "Enums.h"

#include <QFile>
#include <QTextStream>
#include <QColor>
#include <QLocale>
#include <QSet>
#include <cmath>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// Цветовая палитра AutoCAD
// ============================================================
static int getAutoCadColorIndex(const QColor& color) {
    if (color == Qt::white) return 7;
    if (color == Qt::red) return 1;
    if (color == Qt::yellow) return 2;
    if (color == Qt::green) return 3;
    if (color == Qt::cyan) return 4;
    if (color == Qt::blue) return 5;
    if (color == Qt::magenta) return 6;
    if (color == Qt::black) return 0;

    struct AcColor { int r, g, b, index; };
    static const AcColor acPalette[] = {
        {255, 0, 0, 1}, {255, 255, 0, 2}, {0, 255, 0, 3}, {0, 255, 255, 4},
        {0, 0, 255, 5}, {255, 0, 255, 6}, {255, 255, 255, 7}, {128, 128, 128, 8},
        {192, 192, 192, 9}, {255, 0, 0, 10}, {255, 127, 127, 11},
        {165, 0, 0, 12}, {165, 82, 82, 13}, {127, 0, 0, 14}, {127, 63, 63, 15},
        {255, 63, 0, 20}, {255, 159, 127, 21}, {255, 127, 0, 30}, {255, 191, 127, 31},
        {255, 191, 0, 40}, {255, 223, 127, 41}, {255, 255, 0, 50}, {255, 255, 127, 51},
        {127, 255, 0, 60}, {191, 255, 127, 61}, {0, 255, 0, 80}, {127, 255, 127, 81},
        {0, 255, 127, 100}, {127, 255, 191, 101}, {0, 255, 255, 140}, {127, 255, 255, 141},
        {0, 127, 255, 160}, {127, 191, 255, 161}, {0, 0, 255, 180}, {127, 127, 255, 181},
        {127, 0, 255, 200}, {191, 127, 255, 201}, {255, 0, 255, 220}, {255, 127, 255, 221},
        {255, 0, 127, 240}, {255, 127, 191, 241}, {51, 51, 51, 250}, {91, 91, 91, 251},
        {132, 132, 132, 252}, {173, 173, 173, 253}, {214, 214, 214, 254}, {255, 255, 255, 255}
    };

    int bestIndex = 7;
    int minDist = 255 * 255 * 3;
    for (const auto& c : acPalette) {
        int dr = color.red() - c.r;
        int dg = color.green() - c.g;
        int db = color.blue() - c.b;
        int dist = dr*dr + dg*dg + db*db;
        if (dist < minDist) {
            minDist = dist;
            bestIndex = c.index;
        }
    }
    return bestIndex;
}

static int getTrueColor24Bit(const QColor& color) {
    return (color.red() << 16) | (color.green() << 8) | color.blue();
}

// ============================================================
// Определение толщины линии в DXF (код 370, в сотых долях мм)
// Использует реальные настройки из GlobalSettings для точного экспорта.
// ============================================================
static int getDxfLineWeight(const Object* obj) {
    const auto& style = obj->getLineStyle();
    const auto& global = GlobalSettings::instance();
    
    double widthMm;
    if (style.customWidth > 0) {
        widthMm = style.customWidth;
    } else {
        bool isThick = (style.type == LineStyleType::SolidMain || style.type == LineStyleType::DashDotThick);
        widthMm = isThick ? global.mainLineWidth : global.thinLineWidth;
    }
    
    // Применяем глобальный масштаб
    widthMm *= global.globalWidthScale;
    
    // DXF код 370 — в сотых долях мм, округляем к ближайшему стандартному значению
    int raw = static_cast<int>(std::round(widthMm * 100));
    
    // Стандартные значения lineweight в DXF: 0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40, 50, 53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211
    static const int standard[] = {0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40, 50, 53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211};
    int best = standard[0];
    int bestDist = std::abs(raw - best);
    for (int s : standard) {
        int d = std::abs(raw - s);
        if (d < bestDist) { bestDist = d; best = s; }
    }
    return best;
}

static bool isSpecialLineType(LineStyleType type) {
    return type == LineStyleType::SolidWavy || type == LineStyleType::SolidZigZag;
}

// ============================================================
// Генерация точек волны/зигзага для отрезков
// ============================================================
static std::vector<Point> generateWavePoints(const Point& start, const Point& end) {
    const auto& wavy = GlobalSettings::instance().wavyParams;
    double amplitude = wavy.amplitude;
    double period = wavy.period;

    std::vector<Point> pts;
    double dx = end.getX() - start.getX();
    double dy = end.getY() - start.getY();
    double length = std::sqrt(dx*dx + dy*dy);
    if (length < 1e-6) { pts.push_back(start); pts.push_back(end); return pts; }

    double px = -dy / length;
    double py =  dx / length;

    double step = std::max(1.0, period / 10.0);
    for (double i = 0; i <= length; i += step) {
        double t = i / length;
        double offset = amplitude * std::sin(i * 2.0 * M_PI / period);
        double x = start.getX() + dx * t + px * offset;
        double y = start.getY() + dy * t + py * offset;
        pts.emplace_back(x, y);
    }
    if (pts.empty() || std::abs(pts.back().getX() - end.getX()) > 1e-6 || std::abs(pts.back().getY() - end.getY()) > 1e-6)
        pts.push_back(end);
    return pts;
}

static std::vector<Point> generateZigzagPoints(const Point& start, const Point& end) {
    const auto& zigzag = GlobalSettings::instance().zigzagParams;
    double amplitude = zigzag.amplitude;
    double kinkLen = zigzag.breakLength;
    double straightLen = zigzag.straightLength;
    double period = 2 * kinkLen + straightLen;

    std::vector<Point> pts;
    double dx = end.getX() - start.getX();
    double dy = end.getY() - start.getY();
    double length = std::sqrt(dx*dx + dy*dy);
    if (length < 1e-6) { pts.push_back(start); pts.push_back(end); return pts; }

    double px = -dy / length;
    double py =  dx / length;

    pts.push_back(start);
    double currentPos = 0;
    while (currentPos < length) {
        double t1 = (currentPos + kinkLen / 2.0) / length;
        if (t1 > 1.0) t1 = 1.0;
        pts.emplace_back(start.getX() + dx * t1 - px * amplitude,
                         start.getY() + dy * t1 - py * amplitude);
        double t2 = (currentPos + kinkLen) / length;
        if (t2 > 1.0) { pts.push_back(end); break; }
        pts.emplace_back(start.getX() + dx * t2 + px * amplitude,
                         start.getY() + dy * t2 + py * amplitude);
        double t3 = (currentPos + kinkLen + kinkLen / 2.0) / length;
        if (t3 > 1.0) t3 = 1.0;
        pts.emplace_back(start.getX() + dx * t3, start.getY() + dy * t3);
        currentPos += period;
        double t4 = currentPos / length;
        if (t4 > 1.0) t4 = 1.0;
        pts.emplace_back(start.getX() + dx * t4, start.getY() + dy * t4);
    }
    if (std::abs(pts.back().getX() - end.getX()) > 1e-6 || std::abs(pts.back().getY() - end.getY()) > 1e-6)
        pts.push_back(end);
    return pts;
}

// Генерация точек волны/зигзага для эллипса (замкнутого)
static std::vector<Point> generateWaveEllipsePoints(const Point& center, double rx, double ry, bool isWave) {
    const auto& global = GlobalSettings::instance();
    double amplitude = isWave ? global.wavyParams.amplitude : global.zigzagParams.amplitude;
    double period = isWave ? global.wavyParams.period : (2.0 * global.zigzagParams.breakLength + global.zigzagParams.straightLength);

    double perimeter = 2.0 * M_PI * std::sqrt((rx*rx + ry*ry) / 2.0);
    double cycles = std::round(perimeter / period);
    if (cycles < 1.0) cycles = 1.0;

    int totalSteps = static_cast<int>(cycles * 40);
    std::vector<Point> pts;
    pts.reserve(totalSteps + 2);

    for (int i = 0; i <= totalSteps; ++i) {
        double t = static_cast<double>(i) / totalSteps;
        double angle = t * 2.0 * M_PI;
        double offset = 0.0;

        if (isWave) {
            offset = amplitude * std::sin(angle * cycles);
        } else {
            double phase = t * cycles;
            phase -= std::floor(phase);
            if (phase < 0.25) offset = amplitude * (phase / 0.25);
            else if (phase < 0.75) offset = amplitude * (1.0 - (phase - 0.25) / 0.25);
            else offset = -amplitude * (1.0 - (phase - 0.75) / 0.25);
        }

        double rX = rx + offset;
        double rY = ry + offset;
        pts.emplace_back(center.getX() + rX * std::cos(angle), center.getY() + rY * std::sin(angle));
    }
    return pts;
}

// Генерация точек волны/зигзага для дуги
static std::vector<Point> generateWaveArcPoints(const Point& center, double radius,
                                                 double startAngleDeg, double spanAngleDeg, bool isWave) {
    const auto& global = GlobalSettings::instance();
    double amplitude = isWave ? global.wavyParams.amplitude : global.zigzagParams.amplitude;
    double period = isWave ? global.wavyParams.period : (2.0 * global.zigzagParams.breakLength + global.zigzagParams.straightLength);

    double startRad = startAngleDeg * M_PI / 180.0;
    double spanRad = spanAngleDeg * M_PI / 180.0;
    double arcLen = std::abs(radius * spanRad);
    double cycles = arcLen / period;

    int totalSteps = std::max(10, static_cast<int>(cycles * 40));
    std::vector<Point> pts;
    pts.reserve(totalSteps + 2);

    for (int i = 0; i <= totalSteps; ++i) {
        double t = static_cast<double>(i) / totalSteps;
        double currentAngleRad = startRad + t * spanRad;
        double phase = t * cycles;
        double offset = 0.0;

        if (isWave) {
            offset = amplitude * std::sin(phase * 2.0 * M_PI);
        } else {
            double p = phase - std::floor(phase);
            if (p < 0.25) offset = amplitude * (p / 0.25);
            else if (p < 0.75) offset = amplitude * (1.0 - (p - 0.25) / 0.25);
            else offset = -amplitude * (1.0 - (p - 0.75) / 0.25);
        }

        double r = radius + offset;
        pts.emplace_back(center.getX() + r * std::cos(currentAngleRad),
                         center.getY() + r * std::sin(currentAngleRad));
    }
    return pts;
}

// Стилизация набора точек для замкнутых/открытых фигур
static std::vector<Point> stylizeEdges(const std::vector<Point>& verts, const Object* obj, bool closed) {
    LineStyleType type = obj->getLineStyle().type;
    bool isWave = (type == LineStyleType::SolidWavy);

    std::vector<Point> allPts;
    int n = (int)verts.size();
    int edges = closed ? n : n - 1;
    for (int i = 0; i < edges; ++i) {
        const Point& p1 = verts[i];
        const Point& p2 = verts[(i + 1) % n];
        auto edgePts = isWave ? generateWavePoints(p1, p2) : generateZigzagPoints(p1, p2);
        if (!allPts.empty() && !edgePts.empty())
            edgePts.erase(edgePts.begin());
        allPts.insert(allPts.end(), edgePts.begin(), edgePts.end());
    }
    return allPts;
}

// ============================================================
// Публичные статические методы
// ============================================================

int DxfExporter::colorToACI(const QColor& color) {
    return getAutoCadColorIndex(color);
}

int DxfExporter::colorToTrueColor(const QColor& color) {
    return getTrueColor24Bit(color);
}

QString DxfExporter::getDxfLinetype(int lineStyleTypeEnum) {
    LineStyleType type = static_cast<LineStyleType>(lineStyleTypeEnum);
    switch (type) {
        case LineStyleType::Dashed: return "DASHED";
        case LineStyleType::DashDotThin: return "DASHDOT";
        case LineStyleType::DashDotThick: return "DASHDOT";
        case LineStyleType::DashDotDot: return "DIVIDE";
        case LineStyleType::SolidMain:
        case LineStyleType::SolidThin:
        case LineStyleType::SolidWavy:
        case LineStyleType::SolidZigZag:
        case LineStyleType::Custom:
        default: return "CONTINUOUS";
    }
}

// ============================================================
// Главный метод экспорта — формат AC1009 (DXF R12)
// ============================================================
bool DxfExporter::exportScene(const Scene* scene, const QString& filePath) {
    if (!scene) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setLocale(QLocale::c());

    // Лямбда для записи DXF-пары (код, значение)
    auto writeCode = [&out](int code, const auto& value) {
        out << code << "\n" << value << "\n";
    };

    // Параметры штрихов из GlobalSettings
    const auto& global = GlobalSettings::instance();
    double dashLen = 10.0; // длина штриха по умолчанию
    double dashSpace = 5.0; // длина пробела по умолчанию
    // Пытаемся взять из глобальных настроек если есть
    if (global.styleParams.count(LineStyleType::Dashed)) {
        dashLen = global.styleParams.at(LineStyleType::Dashed).dash;
        dashSpace = global.styleParams.at(LineStyleType::Dashed).gap;
    }

    // Собираем слои
    QSet<QString> layerNames;
    layerNames.insert("0");
    for (const auto& prim : scene->getPrimitives()) {
        layerNames.insert(prim->getLayer());
    }

    // ================= HEADER SECTION =================
    writeCode(0, "SECTION");
    writeCode(2, "HEADER");
    writeCode(9, "$ACADVER");
    writeCode(1, "AC1009");
    writeCode(9, "$HANDLING");
    writeCode(70, 0);
    writeCode(9, "$LTSCALE");
    writeCode(40, 1.0);
    writeCode(0, "ENDSEC");

    // ================= TABLES SECTION =================
    writeCode(0, "SECTION");
    writeCode(2, "TABLES");

    // --- LTYPE TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "LTYPE");
    writeCode(70, 4);

    auto writeLType = [&writeCode](const QString& name, const QString& desc) {
        writeCode(0, "LTYPE");
        writeCode(2, name);
        writeCode(70, 0);
        writeCode(3, desc);
        writeCode(72, 65);
    };

    // CONTINUOUS
    writeLType("CONTINUOUS", "Solid line");
    writeCode(73, 0); writeCode(40, 0.0);

    // DASHED
    {
        double d = dashLen;
        double s = dashSpace;
        double total = d + s;
        writeLType("DASHED", "Dashed __ __ __ __");
        writeCode(73, 2);
        writeCode(40, total);
        writeCode(49, d);
        writeCode(49, -s);
    }

    // DASHDOT
    {
        double d = dashLen;
        double s = dashSpace / 2.0;
        double total = d + s + 0.0 + s;
        writeLType("DASHDOT", "Dash dot __ . __ . __");
        writeCode(73, 4);
        writeCode(40, total);
        writeCode(49, d);
        writeCode(49, -s);
        writeCode(49, 0.0);
        writeCode(49, -s);
    }

    // DIVIDE
    {
        double d = dashLen;
        double s = dashSpace / 2.0;
        double total = d + s + 0.0 + s + 0.0 + s;
        writeLType("DIVIDE", "Dash dot dot __ . . __ . . __");
        writeCode(73, 6);
        writeCode(40, total);
        writeCode(49, d);
        writeCode(49, -s);
        writeCode(49, 0.0);
        writeCode(49, -s);
        writeCode(49, 0.0);
        writeCode(49, -s);
    }

    writeCode(0, "ENDTAB");

    // --- LAYER TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "LAYER");
    writeCode(70, layerNames.size());
    for (const QString& layerName : layerNames) {
        writeCode(0, "LAYER");
        writeCode(2, layerName);
        writeCode(70, 0);
        writeCode(62, 7);
        writeCode(6, "CONTINUOUS");
    }
    writeCode(0, "ENDTAB");
    writeCode(0, "ENDSEC");

    // ================= BLOCKS SECTION =================
    writeCode(0, "SECTION");
    writeCode(2, "BLOCKS");
    writeCode(0, "ENDSEC");

    // ================= ENTITIES SECTION =================
    writeCode(0, "SECTION");
    writeCode(2, "ENTITIES");

    for (const auto& objPtr : scene->getPrimitives()) {
        Object* obj = objPtr.get();
        PrimitiveType type = obj->getType();
        LineStyleType st = obj->getLineStyle().type;
        bool isSpecial = isSpecialLineType(st);
        bool isWave = (st == LineStyleType::SolidWavy);

        // Лямбда для записи общих свойств сущности
        auto writeCommonProperties = [&]() {
            writeCode(8, obj->getLayer());
            writeCode(6, getDxfLinetype(static_cast<int>(st)));
            writeCode(62, getAutoCadColorIndex(obj->getColor()));
            writeCode(420, getTrueColor24Bit(obj->getColor()));
            writeCode(370, getDxfLineWeight(obj));
        };

        // Лямбда для записи xdata с нашим типом линии
        auto writeXData = [&]() {
            writeCode(999, QString("CLARUSCAD_LTYPE:%1").arg(static_cast<int>(st)));
        };

        // Лямбда для записи POLYLINE из набора точек
        auto writePolyline = [&](const std::vector<Point>& pts, bool closed) {
            writeCode(0, "POLYLINE");
            writeCommonProperties();
            writeCode(66, 1);
            writeCode(70, closed ? 1 : 0);
            writeCode(10, 0.0);
            writeCode(20, 0.0);
            writeCode(30, 0.0);
            writeXData();
            for (const auto& pt : pts) {
                writeCode(0, "VERTEX");
                writeCode(8, obj->getLayer());
                writeCode(10, pt.getX());
                writeCode(20, pt.getY());
                writeCode(30, 0.0);
            }
            writeCode(0, "SEQEND");
            writeCode(8, obj->getLayer());
        };

        switch (type) {
            case PrimitiveType::Segment: {
                auto* seg = static_cast<Segment*>(obj);
                if (isSpecial) {
                    auto pts = isWave ? generateWavePoints(seg->getStart(), seg->getEnd())
                                      : generateZigzagPoints(seg->getStart(), seg->getEnd());
                    writePolyline(pts, false);
                } else {
                    writeCode(0, "LINE");
                    writeCommonProperties();
                    writeCode(10, seg->getStart().getX());
                    writeCode(20, seg->getStart().getY());
                    writeCode(30, 0.0);
                    writeCode(11, seg->getEnd().getX());
                    writeCode(21, seg->getEnd().getY());
                    writeCode(31, 0.0);
                    writeXData();
                }
                break;
            }
            case PrimitiveType::Circle: {
                auto* circ = static_cast<Circle*>(obj);
                if (isSpecial) {
                    auto pts = generateWaveEllipsePoints(circ->getCenter(), circ->getRadius(), circ->getRadius(), isWave);
                    writePolyline(pts, true);
                } else {
                    writeCode(0, "CIRCLE");
                    writeCommonProperties();
                    writeCode(10, circ->getCenter().getX());
                    writeCode(20, circ->getCenter().getY());
                    writeCode(30, 0.0);
                    writeCode(40, circ->getRadius());
                    writeXData();
                }
                break;
            }
            case PrimitiveType::Arc: {
                auto* arc = static_cast<Arc*>(obj);
                if (isSpecial) {
                    auto pts = generateWaveArcPoints(arc->getCenter(), arc->getRadius(),
                                                     arc->getStartAngle(), arc->getSpanAngle(), isWave);
                    writePolyline(pts, false);
                } else {
                    writeCode(0, "ARC");
                    writeCommonProperties();
                    writeCode(10, arc->getCenter().getX());
                    writeCode(20, arc->getCenter().getY());
                    writeCode(30, 0.0);
                    writeCode(40, arc->getRadius());
                    writeCode(50, arc->getStartAngle());
                    double endAngle = arc->getStartAngle() + arc->getSpanAngle();
                    writeCode(51, endAngle);
                    writeXData();
                }
                break;
            }
            case PrimitiveType::Ellipse: {
                auto* ell = static_cast<Ellipse*>(obj);
                if (isSpecial) {
                    auto pts = generateWaveEllipsePoints(ell->getCenter(), ell->getRadiusX(), ell->getRadiusY(), isWave);
                    writePolyline(pts, true);
                } else {
                    // Экспортируем как POLYLINE (эллипс не поддерживается в AC1009 R12)
                    std::vector<Point> pts;
                    int steps = 128;
                    for (int i = 0; i < steps; ++i) {
                        double angle = (double)i / steps * 2.0 * M_PI;
                        pts.emplace_back(ell->getCenter().getX() + ell->getRadiusX() * std::cos(angle),
                                         ell->getCenter().getY() + ell->getRadiusY() * std::sin(angle));
                    }
                    writePolyline(pts, true);
                }
                break;
            }
            case PrimitiveType::Rectangle: {
                auto* rect = static_cast<Rectangle*>(obj);
                double minX = rect->getTopLeft().getX();
                double maxY = rect->getTopLeft().getY();
                double w = rect->getWidth();
                double h = rect->getHeight();
                std::vector<Point> corners = {
                    Point(minX, maxY), Point(minX + w, maxY),
                    Point(minX + w, maxY - h), Point(minX, maxY - h)
                };
                if (isSpecial) {
                    auto pts = stylizeEdges(corners, obj, true);
                    writePolyline(pts, false); // замкнутость через точки
                } else {
                    writePolyline(corners, true);
                }
                break;
            }
            case PrimitiveType::Polygon: {
                auto* poly = static_cast<PolygonObj*>(obj);
                auto verts = poly->getVertices();
                if (isSpecial) {
                    auto pts = stylizeEdges(verts, obj, true);
                    writePolyline(pts, false);
                } else {
                    writePolyline(verts, true);
                }
                break;
            }
            case PrimitiveType::Spline: {
                auto* spl = static_cast<Spline*>(obj);
                const auto& splPoints = spl->getPoints();
                if (isSpecial) {
                    auto pts = stylizeEdges(splPoints, obj, false);
                    writePolyline(pts, false);
                } else {
                    writePolyline(splPoints, false);
                }
                break;
            }
            case PrimitiveType::Point: {
                auto* pt = static_cast<PointObject*>(obj);
                writeCode(0, "POINT");
                writeCommonProperties();
                writeCode(10, pt->getPosition().getX());
                writeCode(20, pt->getPosition().getY());
                writeCode(30, 0.0);
                writeXData();
                break;
            }
            default:
                break;
        }
    }

    writeCode(0, "ENDSEC");

    // ================= EOF =================
    writeCode(0, "EOF");

    file.close();
    return true;
}
