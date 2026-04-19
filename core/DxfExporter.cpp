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
#include "Dimension.h"
#include "GlobalSettings.h"
#include "Enums.h"

#include <QFile>
#include <QTextStream>
#include <QColor>
#include <QLocale>
#include <QPointF>
#include <QSet>
#include <QStringList>
#include <QUrl>
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

static QString encodeDimString(const QString& value) {
    return QString::fromLatin1(QUrl::toPercentEncoding(value));
}

static QString colorToDimToken(const QColor& color) {
    return color.name(QColor::HexRgb);
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
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(10);

    int handleCounter = 1;
    auto nextHandle = [&handleCounter]() -> QString {
        return QString::number(handleCounter++, 16).toUpper();
    };

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
    writeCode(1, "AC1015");
    writeCode(9, "$HANDSEED");
    writeCode(5, "FFFF");
    writeCode(9, "$LTSCALE");
    writeCode(40, 1.0);
    writeCode(0, "ENDSEC");

    // ================= TABLES SECTION =================
    writeCode(0, "SECTION");
    writeCode(2, "TABLES");

    // --- VPORT TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "VPORT");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTable");
    writeCode(70, 1);
    writeCode(0, "VPORT");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTableRecord");
    writeCode(100, "AcDbViewportTableRecord");
    writeCode(2, "*ACTIVE");
    writeCode(70, 0);
    writeCode(10, 0.0); writeCode(20, 0.0);
    writeCode(11, 1.0); writeCode(21, 1.0);
    writeCode(12, 0.0); writeCode(22, 0.0);
    writeCode(13, 0.0); writeCode(23, 0.0);
    writeCode(14, 10.0); writeCode(24, 10.0);
    writeCode(15, 10.0); writeCode(25, 10.0);
    writeCode(16, 0.0); writeCode(26, 0.0); writeCode(36, 1.0);
    writeCode(17, 0.0); writeCode(27, 0.0); writeCode(37, 0.0);
    writeCode(40, 1000.0);
    writeCode(41, 1.0);
    writeCode(42, 50.0);
    writeCode(43, 0.0);
    writeCode(44, 0.0);
    writeCode(50, 0.0);
    writeCode(51, 0.0);
    writeCode(71, 0);
    writeCode(72, 100);
    writeCode(73, 1);
    writeCode(74, 3);
    writeCode(75, 0);
    writeCode(76, 1);
    writeCode(77, 0);
    writeCode(78, 0);
    writeCode(0, "ENDTAB");

    // --- LTYPE TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "LTYPE");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTable");
    writeCode(70, 4);

    auto writeLType = [&](const QString& name, const QString& desc) {
        writeCode(0, "LTYPE");
        writeCode(5, nextHandle());
        writeCode(100, "AcDbSymbolTableRecord");
        writeCode(100, "AcDbLinetypeTableRecord");
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
        writeCode(74, 0);
        writeCode(49, -s);
        writeCode(74, 0);
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
        writeCode(74, 0);
        writeCode(49, -s);
        writeCode(74, 0);
        writeCode(49, 0.0);
        writeCode(74, 0);
        writeCode(49, -s);
        writeCode(74, 0);
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
        writeCode(74, 0);
        writeCode(49, -s);
        writeCode(74, 0);
        writeCode(49, 0.0);
        writeCode(74, 0);
        writeCode(49, -s);
        writeCode(74, 0);
        writeCode(49, 0.0);
        writeCode(74, 0);
        writeCode(49, -s);
        writeCode(74, 0);
    }

    writeCode(0, "ENDTAB");

    // --- LAYER TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "LAYER");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTable");
    writeCode(70, layerNames.size());
    for (const QString& layerName : layerNames) {
        writeCode(0, "LAYER");
        writeCode(5, nextHandle());
        writeCode(100, "AcDbSymbolTableRecord");
        writeCode(100, "AcDbLayerTableRecord");
        writeCode(2, layerName);
        writeCode(70, 0);
        writeCode(62, 7);
        writeCode(6, "CONTINUOUS");
        writeCode(370, 25);
    }
    writeCode(0, "ENDTAB");

    // --- STYLE TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "STYLE");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTable");
    writeCode(70, 1);
    {
        writeCode(0, "STYLE");
        writeCode(5, nextHandle());
        writeCode(100, "AcDbSymbolTableRecord");
        writeCode(100, "AcDbTextStyleTableRecord");
        writeCode(2, "STANDARD");
        writeCode(70, 0);
        writeCode(40, 0.0);
        writeCode(41, 1.0);
        writeCode(50, 0.0);
        writeCode(71, 0);
        writeCode(42, 2.5);
        writeCode(3, "txt");
        writeCode(4, "");
    }
    writeCode(0, "ENDTAB");

    // --- APPID TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "APPID");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTable");
    writeCode(70, 1);
    {
        writeCode(0, "APPID");
        writeCode(5, nextHandle());
        writeCode(100, "AcDbSymbolTableRecord");
        writeCode(100, "AcDbRegAppTableRecord");
        writeCode(2, "ACAD");
        writeCode(70, 0);
    }
    writeCode(0, "ENDTAB");

    // --- DIMSTYLE TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "DIMSTYLE");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTable");
    writeCode(70, 1);
    writeCode(100, "AcDbDimStyleTable");
    {
        writeCode(0, "DIMSTYLE");
        writeCode(5, nextHandle());
        writeCode(100, "AcDbSymbolTableRecord");
        writeCode(100, "AcDbDimStyleTableRecord");
        writeCode(2, "STANDARD");
        writeCode(70, 0);
    }
    writeCode(0, "ENDTAB");

    // --- BLOCK_RECORD TABLE ---
    writeCode(0, "TABLE");
    writeCode(2, "BLOCK_RECORD");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTable");
    writeCode(70, 2);

    writeCode(0, "BLOCK_RECORD");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTableRecord");
    writeCode(100, "AcDbBlockTableRecord");
    writeCode(2, "*Model_Space");

    writeCode(0, "BLOCK_RECORD");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbSymbolTableRecord");
    writeCode(100, "AcDbBlockTableRecord");
    writeCode(2, "*Paper_Space");

    writeCode(0, "ENDTAB");

    writeCode(0, "ENDSEC");

    // ================= BLOCKS SECTION =================
    writeCode(0, "SECTION");
    writeCode(2, "BLOCKS");

    // *Model_Space
    writeCode(0, "BLOCK");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbEntity");
    writeCode(8, "0");
    writeCode(100, "AcDbBlockBegin");
    writeCode(2, "*Model_Space");
    writeCode(70, 0);
    writeCode(10, 0.0); writeCode(20, 0.0); writeCode(30, 0.0);
    writeCode(3, "*Model_Space");
    writeCode(1, "");
    writeCode(0, "ENDBLK");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbEntity");
    writeCode(8, "0");
    writeCode(100, "AcDbBlockEnd");

    // *Paper_Space
    writeCode(0, "BLOCK");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbEntity");
    writeCode(8, "0");
    writeCode(100, "AcDbBlockBegin");
    writeCode(2, "*Paper_Space");
    writeCode(70, 0);
    writeCode(10, 0.0); writeCode(20, 0.0); writeCode(30, 0.0);
    writeCode(3, "*Paper_Space");
    writeCode(1, "");
    writeCode(0, "ENDBLK");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbEntity");
    writeCode(8, "0");
    writeCode(100, "AcDbBlockEnd");

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
        auto writeCommonProperties = [&](const QString& subclassMarker) {
            writeCode(5, nextHandle());
            writeCode(100, "AcDbEntity");
            writeCode(8, obj->getLayer());
            writeCode(6, getDxfLinetype(static_cast<int>(st)));
            writeCode(62, getAutoCadColorIndex(obj->getColor()));
            writeCode(420, getTrueColor24Bit(obj->getColor()));
            writeCode(370, getDxfLineWeight(obj));
            writeCode(100, subclassMarker);
        };

        // Лямбда для записи xdata с нашим типом линии
        QString origTypeData; // Дополнительные xdata для оригинального типа
        auto writeXData = [&]() {
            writeCode(999, QString("UNIVERSITYCAD_LTYPE:%1").arg(static_cast<int>(st)));
            if (!origTypeData.isEmpty()) {
                writeCode(999, origTypeData);
            }
        };

        // Лямбда для записи POLYLINE из набора точек
        auto writePolyline = [&](const std::vector<Point>& pts, bool closed) {
            writeCode(0, "POLYLINE");
            writeCode(5, nextHandle());
            writeCode(100, "AcDbEntity");
            writeCode(8, obj->getLayer());
            writeCode(6, getDxfLinetype(static_cast<int>(st)));
            writeCode(62, getAutoCadColorIndex(obj->getColor()));
            writeCode(420, getTrueColor24Bit(obj->getColor()));
            writeCode(370, getDxfLineWeight(obj));
            writeCode(100, "AcDb2dPolyline");
            writeCode(66, 1);
            writeCode(70, closed ? 1 : 0);
            writeCode(10, 0.0);
            writeCode(20, 0.0);
            writeCode(30, 0.0);
            writeXData();
            for (const auto& pt : pts) {
                writeCode(0, "VERTEX");
                writeCode(5, nextHandle());
                writeCode(100, "AcDbEntity");
                writeCode(8, obj->getLayer());
                writeCode(100, "AcDbVertex");
                writeCode(100, "AcDb2dVertex");
                writeCode(10, pt.getX());
                writeCode(20, pt.getY());
                writeCode(30, 0.0);
            }
            writeCode(0, "SEQEND");
            writeCode(5, nextHandle());
            writeCode(100, "AcDbEntity");
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
                    writeCommonProperties("AcDbLine");
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
                    // Устанавливаем xdata оригинального типа для round-trip
                    origTypeData = QString("UNIVERSITYCAD_ORIG:Circle|%1|%2|%3")
                        .arg(circ->getCenter().getX())
                        .arg(circ->getCenter().getY())
                        .arg(circ->getRadius());
                    writePolyline(pts, true);
                    origTypeData.clear();
                } else {
                    writeCode(0, "CIRCLE");
                    writeCommonProperties("AcDbCircle");
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
                origTypeData = QString("UNIVERSITYCAD_ORIG:Arc|%1|%2|%3|%4|%5")
                    .arg(arc->getCenter().getX())
                    .arg(arc->getCenter().getY())
                    .arg(arc->getRadius())
                    .arg(arc->getStartAngle())
                    .arg(arc->getSpanAngle());

                if (isSpecial) {
                    auto pts = generateWaveArcPoints(arc->getCenter(), arc->getRadius(),
                                                     arc->getStartAngle(), arc->getSpanAngle(), isWave);
                    writePolyline(pts, false);
                    origTypeData.clear();
                } else {
                    writeCode(0, "ARC");
                    writeCommonProperties("AcDbCircle");
                    writeCode(10, arc->getCenter().getX());
                    writeCode(20, arc->getCenter().getY());
                    writeCode(30, 0.0);
                    writeCode(40, arc->getRadius());
                    writeCode(100, "AcDbArc");
                    
                    // Углы внутри приложения хранятся в Y-up системе,
                    // но пользователь видит инвертированный Y на экране.
                    // DXF использует Y-up, но углы должны соответствовать визуальному
                    // отображению, поэтому отражаем по оси X: dxfAngle = -internalAngle
                    auto normAngle = [](double a) -> double {
                        a = std::fmod(a, 360.0);
                        if (a < 0) a += 360.0;
                        return a;
                    };
                    
                    double internalStart = arc->getStartAngle();
                    double internalEnd = internalStart + arc->getSpanAngle();
                    
                    // Отражение по X-оси
                    double dxfStart = normAngle(-internalStart);
                    double dxfEnd = normAngle(-internalEnd);
                    
                    // DXF ARC рисуется CCW от code 50 к code 51.
                    // При отражении направление обхода инвертируется,
                    // поэтому start и end меняются местами.
                    double span = arc->getSpanAngle();
                    if (span >= 0) {
                        writeCode(50, dxfEnd);
                        writeCode(51, dxfStart);
                    } else {
                        writeCode(50, dxfStart);
                        writeCode(51, dxfEnd);
                    }
                    writeXData();
                    origTypeData.clear();
                }
                break;
            }
            case PrimitiveType::Ellipse: {
                auto* ell = static_cast<Ellipse*>(obj);
                if (isSpecial) {
                    auto pts = generateWaveEllipsePoints(ell->getCenter(), ell->getRadiusX(), ell->getRadiusY(), isWave);
                    // Устанавливаем xdata оригинального типа для round-trip
                    origTypeData = QString("UNIVERSITYCAD_ORIG:Ellipse|%1|%2|%3|%4")
                        .arg(ell->getCenter().getX())
                        .arg(ell->getCenter().getY())
                        .arg(ell->getRadiusX())
                        .arg(ell->getRadiusY());
                    writePolyline(pts, true);
                    origTypeData.clear();
                } else {
                    double rX = ell->getRadiusX();
                    double rY = ell->getRadiusY();
                    double majorR = std::max(rX, rY);
                    double minorR = std::min(rX, rY);
                    double ratio = minorR / majorR;

                    writeCode(0, "ELLIPSE");
                    writeCommonProperties("AcDbEllipse");
                    writeCode(10, ell->getCenter().getX());
                    writeCode(20, ell->getCenter().getY());
                    writeCode(30, 0.0);
                    if (rX >= rY) {
                        writeCode(11, rX);
                        writeCode(21, 0.0);
                    } else {
                        writeCode(11, 0.0);
                        writeCode(21, rY);
                    }
                    writeCode(31, 0.0);
                    writeCode(40, ratio);
                    writeCode(41, 0.0);
                    writeCode(42, 2.0 * M_PI);
                    writeXData();
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
                if (isSpecial) {
                    const auto& splPoints = spl->getSmoothPoints();
                    auto pts = stylizeEdges(splPoints, obj, false);
                    writePolyline(pts, false);
                } else {
                    // Экспорт как POLYLINE с высоким разрешением для гладкости
                    auto smoothPts = spl->getSmoothPoints(200);
                    writePolyline(smoothPts, false);
                }
                break;
            }
            case PrimitiveType::Point: {
                auto* pt = static_cast<PointObject*>(obj);
                writeCode(0, "POINT");
                writeCommonProperties("AcDbPoint");
                writeCode(10, pt->getPosition().getX());
                writeCode(20, pt->getPosition().getY());
                writeCode(30, 0.0);
                writeXData();
                break;
            }
            case PrimitiveType::Dimension: {
                auto* dim = static_cast<Dimension*>(obj);
                const Point a = dim->firstAnchor().resolve();
                const Point b = dim->secondAnchor().resolve();
                const Point line = dim->getLinePoint();

                auto normalizedPoint = [](const QPointF& v) {
                    const double len = std::hypot(v.x(), v.y());
                    if (len < 1e-9) return QPointF(1.0, 0.0);
                    return QPointF(v.x() / len, v.y() / len);
                };

                auto writeDimEntityCommon = [&](const QString& subclassMarker, const QColor& color, const LineStyle& style) {
                    writeCode(5, nextHandle());
                    writeCode(100, "AcDbEntity");
                    writeCode(8, obj->getLayer());
                    writeCode(6, getDxfLinetype(static_cast<int>(style.type)));
                    writeCode(62, getAutoCadColorIndex(color));
                    writeCode(420, getTrueColor24Bit(color));
                    writeCode(370, 13);
                    writeCode(999, "UNIVERSITYCAD_DIM_GEOM");
                    writeCode(100, subclassMarker);
                };

                auto writeDimLine = [&](const QPointF& p1, const QPointF& p2, const QColor& color, const LineStyle& style) {
                    writeCode(0, "LINE");
                    writeDimEntityCommon("AcDbLine", color, style);
                    writeCode(10, p1.x());
                    writeCode(20, p1.y());
                    writeCode(30, 0.0);
                    writeCode(11, p2.x());
                    writeCode(21, p2.y());
                    writeCode(31, 0.0);
                };

                auto writeDimCircle = [&](const QPointF& center, double radius, const QColor& color, const LineStyle& style) {
                    writeCode(0, "CIRCLE");
                    writeDimEntityCommon("AcDbCircle", color, style);
                    writeCode(10, center.x());
                    writeCode(20, center.y());
                    writeCode(30, 0.0);
                    writeCode(40, radius);
                };

                auto writeDimArc = [&](const QPointF& center, double radius, double startRad, double deltaRad, const QColor& color, const LineStyle& style) {
                    double startDeg = qRadiansToDegrees(startRad);
                    double endDeg = qRadiansToDegrees(startRad + deltaRad);
                    if (deltaRad < 0.0) std::swap(startDeg, endDeg);
                    writeCode(0, "ARC");
                    writeDimEntityCommon("AcDbCircle", color, style);
                    writeCode(10, center.x());
                    writeCode(20, center.y());
                    writeCode(30, 0.0);
                    writeCode(40, radius);
                    writeCode(100, "AcDbArc");
                    writeCode(50, startDeg);
                    writeCode(51, endDeg);
                };

                auto writeDimText = [&](const QPointF& pos, double angleDeg) {
                    while (angleDeg > 180.0) angleDeg -= 360.0;
                    while (angleDeg < -180.0) angleDeg += 360.0;
                    if (angleDeg > 90.0) angleDeg -= 180.0;
                    if (angleDeg < -90.0) angleDeg += 180.0;
                    writeCode(0, "TEXT");
                    writeDimEntityCommon("AcDbText", dim->textColor(), dim->dimensionLineStyle());
                    writeCode(10, pos.x());
                    writeCode(20, pos.y());
                    writeCode(30, 0.0);
                    writeCode(40, dim->textHeight());
                    writeCode(1, dim->displayText());
                    writeCode(50, angleDeg);
                    writeCode(7, "STANDARD");
                    writeCode(72, 1);
                    writeCode(11, pos.x());
                    writeCode(21, pos.y());
                    writeCode(31, 0.0);
                    writeCode(73, 2);
                };

                auto writeArrow = [&](const QPointF& tip, double angle) {
                    const double size = std::max(1.0, dim->arrowSize());
                    const QPointF back(tip.x() - std::cos(angle) * size, tip.y() - std::sin(angle) * size);
                    const QPointF left(back.x() + std::cos(angle + M_PI / 2.0) * size * 0.35,
                                       back.y() + std::sin(angle + M_PI / 2.0) * size * 0.35);
                    const QPointF right(back.x() + std::cos(angle - M_PI / 2.0) * size * 0.35,
                                        back.y() + std::sin(angle - M_PI / 2.0) * size * 0.35);
                    if (dim->arrowType() == ArrowType::Dot) {
                        writeDimCircle(tip, size * 0.25, dim->dimensionColor(), dim->dimensionLineStyle());
                    } else if (dim->arrowType() == ArrowType::Tick) {
                        writeDimLine(QPointF(tip.x() - size * 0.35, tip.y() - size * 0.35),
                                     QPointF(tip.x() + size * 0.35, tip.y() + size * 0.35),
                                     dim->dimensionColor(), dim->dimensionLineStyle());
                    } else {
                        writeDimLine(tip, left, dim->dimensionColor(), dim->dimensionLineStyle());
                        writeDimLine(tip, right, dim->dimensionColor(), dim->dimensionLineStyle());
                        if (dim->arrowType() == ArrowType::Closed) {
                            writeDimLine(left, right, dim->dimensionColor(), dim->dimensionLineStyle());
                        }
                    }
                };

                auto writeVisibleDimension = [&]() {
                    const QPointF ap(a.getX(), a.getY());
                    const QPointF bp(b.getX(), b.getY());
                    const QPointF lp(line.getX(), line.getY());
                    const QPointF text(dim->getTextPosition().getX(), dim->getTextPosition().getY());

                    if (dim->getDimensionType() == DimensionType::Radius || dim->getDimensionType() == DimensionType::Diameter) {
                        double r = dim->measuredValue();
                        if (dim->getDimensionType() == DimensionType::Diameter) r *= 0.5;
                        QPointF dir = normalizedPoint(lp - ap);
                        if (std::hypot(lp.x() - ap.x(), lp.y() - ap.y()) < 1e-9) {
                            dir = normalizedPoint(bp - ap);
                        }
                        const QPointF edge1 = ap + dir * r;
                        const QPointF edge2 = ap - dir * r;
                        if (dim->getDimensionType() == DimensionType::Radius) {
                            QPointF leaderEnd = lp;
                            if (std::hypot(leaderEnd.x() - ap.x(), leaderEnd.y() - ap.y()) < r) leaderEnd = edge1;
                            writeDimLine(ap, leaderEnd, dim->dimensionColor(), dim->dimensionLineStyle());
                            double arrowAngle = std::atan2(ap.y() - edge1.y(), ap.x() - edge1.x());
                            if (dim->arrowPlacement() == ArrowPlacement::Inside) arrowAngle += M_PI;
                            writeArrow(edge1, arrowAngle);
                            writeDimText(text, qRadiansToDegrees(std::atan2(leaderEnd.y() - ap.y(), leaderEnd.x() - ap.x())));
                        } else {
                            writeDimLine(edge2, edge1, dim->dimensionColor(), dim->dimensionLineStyle());
                            const bool outside = dim->arrowPlacement() == ArrowPlacement::Inside;
                            writeArrow(edge2, std::atan2(edge1.y() - edge2.y(), edge1.x() - edge2.x()) + (outside ? M_PI : 0.0));
                            writeArrow(edge1, std::atan2(edge2.y() - edge1.y(), edge2.x() - edge1.x()) + (outside ? M_PI : 0.0));
                            writeDimText(text, qRadiansToDegrees(std::atan2(edge1.y() - edge2.y(), edge1.x() - edge2.x())));
                        }
                        return;
                    }

                    if (dim->getDimensionType() == DimensionType::Angular) {
                        const double a1 = std::atan2(ap.y() - lp.y(), ap.x() - lp.x());
                        const double a2 = std::atan2(bp.y() - lp.y(), bp.x() - lp.x());
                        double delta = std::fmod(a2 - a1, 2.0 * M_PI);
                        if (delta > M_PI) delta -= 2.0 * M_PI;
                        if (delta < -M_PI) delta += 2.0 * M_PI;
                        const double r = dim->angularRadius() > 1e-9
                            ? dim->angularRadius()
                            : std::max(15.0, std::min(std::hypot(ap.x() - lp.x(), ap.y() - lp.y()),
                                                      std::hypot(bp.x() - lp.x(), bp.y() - lp.y())) * 0.65);
                        const QPointF arcA(lp.x() + std::cos(a1) * r, lp.y() + std::sin(a1) * r);
                        const QPointF arcB(lp.x() + std::cos(a1 + delta) * r, lp.y() + std::sin(a1 + delta) * r);
                        auto writeAngularExtension = [&](double angle, const QPointF& source) {
                            QPointF dir(std::cos(angle), std::sin(angle));
                            double sourceRadius = std::hypot(source.x() - lp.x(), source.y() - lp.y());
                            double endRadius = r + dim->extensionOvershoot();
                            if (endRadius < sourceRadius) std::swap(sourceRadius, endRadius);
                            writeDimLine(QPointF(lp.x() + dir.x() * sourceRadius, lp.y() + dir.y() * sourceRadius),
                                         QPointF(lp.x() + dir.x() * endRadius, lp.y() + dir.y() * endRadius),
                                         dim->extensionColor(), dim->extensionLineStyle());
                        };
                        writeAngularExtension(a1, ap);
                        writeAngularExtension(a1 + delta, bp);
                        writeDimArc(lp, r, a1, delta, dim->dimensionColor(), dim->dimensionLineStyle());
                        const double tangentSign = delta >= 0.0 ? 1.0 : -1.0;
                        writeArrow(arcA, a1 + tangentSign * M_PI / 2.0);
                        writeArrow(arcB, a1 + delta - tangentSign * M_PI / 2.0);
                        writeDimText(text, qRadiansToDegrees(a1 + delta * dim->textPositionFactor() + tangentSign * M_PI / 2.0));
                        return;
                    }

                    QPointF da = ap;
                    QPointF db = bp;
                    double textAngle = 0.0;
                    if (dim->getDimensionType() == DimensionType::Horizontal) {
                        da = QPointF(ap.x(), lp.y());
                        db = QPointF(bp.x(), lp.y());
                    } else if (dim->getDimensionType() == DimensionType::Vertical) {
                        da = QPointF(lp.x(), ap.y());
                        db = QPointF(lp.x(), bp.y());
                        textAngle = 90.0;
                    } else {
                        const double vx = bp.x() - ap.x();
                        const double vy = bp.y() - ap.y();
                        const double len = std::hypot(vx, vy);
                        if (len > 1e-9) {
                            const double nx = -vy / len;
                            const double ny = vx / len;
                            const double off = (lp.x() - ap.x()) * nx + (lp.y() - ap.y()) * ny;
                            da = QPointF(ap.x() + nx * off, ap.y() + ny * off);
                            db = QPointF(bp.x() + nx * off, bp.y() + ny * off);
                        }
                    }
                    QPointF u = normalizedPoint(db - da);
                    QPointF n(-u.y(), u.x());
                    const QPointF dimStart = da - u * dim->dimensionExtension();
                    const QPointF dimEnd = db + u * dim->dimensionExtension();
                    auto writeExtension = [&](const QPointF& source, const QPointF& target) {
                        const QPointF normal = normalizedPoint(n);
                        const double sign = QPointF::dotProduct(target - source, normal) >= 0.0 ? 1.0 : -1.0;
                        writeDimLine(source, target + normal * (sign * dim->extensionOvershoot()),
                                     dim->extensionColor(), dim->extensionLineStyle());
                    };
                    writeExtension(ap, da);
                    writeExtension(bp, db);
                    writeDimLine(dimStart, dimEnd, dim->dimensionColor(), dim->dimensionLineStyle());
                    const double angle = std::atan2(db.y() - da.y(), db.x() - da.x());
                    const bool outside = dim->arrowPlacement() == ArrowPlacement::Inside;
                    writeArrow(da, angle + (outside ? M_PI : 0.0));
                    writeArrow(db, angle + M_PI + (outside ? M_PI : 0.0));
                    if (dim->getDimensionType() != DimensionType::Vertical) {
                        textAngle = qRadiansToDegrees(angle);
                    }
                    writeDimText(text, textAngle);
                };

                QStringList data;
                data << "1"
                     << QString::number(static_cast<int>(dim->getDimensionType()))
                     << QString::number(a.getX(), 'f', 10)
                     << QString::number(a.getY(), 'f', 10)
                     << QString::number(b.getX(), 'f', 10)
                     << QString::number(b.getY(), 'f', 10)
                     << QString::number(line.getX(), 'f', 10)
                     << QString::number(line.getY(), 'f', 10)
                     << QString::number(dim->textPositionFactor(), 'f', 10)
                     << encodeDimString(dim->getTextOverride())
                     << colorToDimToken(dim->extensionColor())
                     << colorToDimToken(dim->dimensionColor())
                     << colorToDimToken(dim->textColor())
                     << QString::number(static_cast<int>(dim->extensionLineStyle().type))
                     << QString::number(static_cast<int>(dim->dimensionLineStyle().type))
                     << QString::number(dim->extensionOvershoot(), 'f', 10)
                     << QString::number(dim->dimensionExtension(), 'f', 10)
                     << QString::number(static_cast<int>(dim->arrowType()))
                     << QString::number(static_cast<int>(dim->arrowPlacement()))
                     << QString::number(dim->arrowSize(), 'f', 10)
                     << QString::number(dim->arrowFilled() ? 1 : 0)
                     << encodeDimString(dim->fontFamily())
                     << QString::number(dim->textHeight(), 'f', 10)
                     << QString::number(dim->textOffset(), 'f', 10)
                     << QString::number(dim->angularRadius(), 'f', 10);

                writeCode(0, "POINT");
                writeCommonProperties("AcDbPoint");
                writeCode(999, "UNIVERSITYCAD_DIM:" + data.join('|'));
                writeCode(10, line.getX());
                writeCode(20, line.getY());
                writeCode(30, 0.0);
                writeXData();
                writeVisibleDimension();
                break;
            }
            default:
                break;
        }
    }

    writeCode(0, "ENDSEC");

    // ================= OBJECTS SECTION =================
    writeCode(0, "SECTION");
    writeCode(2, "OBJECTS");
    writeCode(0, "DICTIONARY");
    writeCode(5, nextHandle());
    writeCode(100, "AcDbDictionary");
    writeCode(281, 1);
    writeCode(0, "ENDSEC");

    // ================= EOF =================
    writeCode(0, "EOF");

    file.close();
    return true;
}
