#include "Ellipse.h"
#include "MathUtils.h"
#include <cmath>

Ellipse::Ellipse(const Point& center, double radX, double radY)
    : m_center(center), m_radiusX(radX), m_radiusY(radY) {}

const Point& Ellipse::getCenter() const { return m_center; }
void Ellipse::setCenter(const Point& p) { m_center = p; }

double Ellipse::getRadiusX() const { return m_radiusX; }
void Ellipse::setRadiusX(double r) { m_radiusX = r; }

double Ellipse::getRadiusY() const { return m_radiusY; }
void Ellipse::setRadiusY(double r) { m_radiusY = r; }

std::vector<SnapPoint> Ellipse::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    snaps.push_back({m_center, SnapType::Center});
    // Квадрантные точки (концы осей)
    snaps.push_back({Point(m_center.getX() + m_radiusX, m_center.getY()), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX() - m_radiusX, m_center.getY()), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX(), m_center.getY() + m_radiusY), SnapType::Quadrant});
    snaps.push_back({Point(m_center.getX(), m_center.getY() - m_radiusY), SnapType::Quadrant});
    return snaps;
}

Point Ellipse::getClosestPoint(const Point& p) const {
    // Приблизительный метод: нормализуем к единичной окружности, находим ближайшую точку, 
    // затем обратно масштабируем
    double dx = p.getX() - m_center.getX();
    double dy = p.getY() - m_center.getY();
    
    // Нормализуем к окружности
    double normX = (m_radiusX > 1e-9) ? dx / m_radiusX : 0;
    double normY = (m_radiusY > 1e-9) ? dy / m_radiusY : 0;
    
    double len = std::sqrt(normX * normX + normY * normY);
    if (len < 1e-9) {
        // Точка в центре - возвращаем точку на большой оси
        return Point(m_center.getX() + m_radiusX, m_center.getY());
    }
    
    // Нормализуем на единичную окружность
    normX /= len;
    normY /= len;
    
    // Обратное масштабирование
    return Point(m_center.getX() + normX * m_radiusX,
                 m_center.getY() + normY * m_radiusY);
}

std::vector<Point> Ellipse::getTangentPoints(const Point& p) const {
    // Для эллипса касательные сложнее чем для окружности.
    // Используем приближённый метод через нормализацию к окружности
    std::vector<Point> result;
    
    // Трансформируем в пространство единичной окружности
    double dx = p.getX() - m_center.getX();
    double dy = p.getY() - m_center.getY();
    
    double normX = (m_radiusX > 1e-9) ? dx / m_radiusX : 0;
    double normY = (m_radiusY > 1e-9) ? dy / m_radiusY : 0;
    
    // Вычисляем касательные к единичной окружности
    double d2 = normX * normX + normY * normY;
    if (d2 < 1.0) return result; // Точка внутри эллипса
    
    double d = std::sqrt(d2);
    double beta = std::atan2(normY, normX);
    double alpha = std::asin(1.0 / d);
    
    double t1 = beta + alpha;
    double t2 = beta - alpha;
    
    // Обратная трансформация
    result.push_back(Point(m_center.getX() + m_radiusX * std::cos(t1),
                           m_center.getY() + m_radiusY * std::sin(t1)));
    result.push_back(Point(m_center.getX() + m_radiusX * std::cos(t2),
                           m_center.getY() + m_radiusY * std::sin(t2)));
    
    return result;
}

std::optional<std::pair<Point, Point>> Ellipse::getTangentSnapPoint(
    const Point& prevPoint, const Point& mousePos) const {
    // Получаем точки касания к эллипсу
    auto tangentPts = getTangentPoints(prevPoint);
    if (tangentPts.empty()) return std::nullopt;
    
    // Выбираем ближайшую касательную к позиции мыши
    Point bestTangentPt;
    Point bestProjection;
    double bestDist = 1e15;
    
    for (const auto& tangentPt : tangentPts) {
        // Линия касательной: от prevPoint через tangentPt
        // Проецируем mousePos на эту линию (бесконечную)
        Point proj = MathUtils::projectPointOnLine(mousePos, prevPoint, tangentPt);
        double d = MathUtils::distSq(mousePos, proj);
        
        if (d < bestDist) {
            bestDist = d;
            bestTangentPt = tangentPt;
            bestProjection = proj;
        }
    }
    
    return std::make_pair(bestTangentPt, bestProjection);
}
