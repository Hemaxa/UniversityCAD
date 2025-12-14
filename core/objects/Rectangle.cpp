#include "Rectangle.h"
#include "MathUtils.h"
#include <algorithm>
#include <cmath>

// Нормализуем прямоугольник при создании: topLeft должен быть "верхним левым" геометрически
Rectangle::Rectangle(const Point& p1, double width, double height, double cornerRadius)
    : m_topLeft(p1), m_width(width), m_height(height), m_cornerRadius(cornerRadius)
{
    // В декартовой системе (Y вверх): TopLeft это (MinX, MaxY).
    // Но часто в коде использовалось смешанное представление.
    // Договоримся: m_topLeft хранит точку с Min X и Max Y (левый верхний угол).
    // m_height идет "вниз".
}

const Point& Rectangle::getTopLeft() const { return m_topLeft; }
void Rectangle::setTopLeft(const Point& p) { m_topLeft = p; }

double Rectangle::getWidth() const { return m_width; }
void Rectangle::setWidth(double w) { m_width = w; }

double Rectangle::getHeight() const { return m_height; }
void Rectangle::setHeight(double h) { m_height = h; }

double Rectangle::getCornerRadius() const { return m_cornerRadius; }
void Rectangle::setCornerRadius(double r) { m_cornerRadius = r; }

std::vector<SnapPoint> Rectangle::getSnapPoints() const {
    std::vector<SnapPoint> snaps;
    // TopLeft (X, Y)
    double x = m_topLeft.getX();
    double y = m_topLeft.getY();
    double w = m_width;
    double h = m_height;

    // Угловые точки
    Point tl(x, y);
    Point tr(x + w, y);
    Point bl(x, y - h);
    Point br(x + w, y - h);

    snaps.push_back({tl, SnapType::Endpoint});
    snaps.push_back({tr, SnapType::Endpoint});
    snaps.push_back({bl, SnapType::Endpoint});
    snaps.push_back({br, SnapType::Endpoint});

    // Середины сторон
    snaps.push_back({Point(x + w/2, y), SnapType::Midpoint});     // Top
    snaps.push_back({Point(x + w/2, y - h), SnapType::Midpoint}); // Bottom
    snaps.push_back({Point(x, y - h/2), SnapType::Midpoint});     // Left
    snaps.push_back({Point(x + w, y - h/2), SnapType::Midpoint}); // Right

    // Центр
    snaps.push_back({Point(x + w/2, y - h/2), SnapType::Center});

    return snaps;
}

Point Rectangle::getClosestPoint(const Point& p) const {
    double x = m_topLeft.getX();
    double y = m_topLeft.getY();
    double w = m_width;
    double h = m_height;

    // Проекция на 4 отрезка, выбираем ближайшую
    Point edges[4][2] = {
        {Point(x, y), Point(x + w, y)},       // Top
        {Point(x + w, y), Point(x + w, y - h)}, // Right
        {Point(x + w, y - h), Point(x, y - h)}, // Bottom
        {Point(x, y - h), Point(x, y)}        // Left
    };

    Point closest;
    double minDist = 1e15;

    for (int i=0; i<4; ++i) {
        Point proj = MathUtils::projectPointOnSegment(p, edges[i][0], edges[i][1]);
        double d = MathUtils::distSq(p, proj);
        if (d < minDist) {
            minDist = d;
            closest = proj;
        }
    }
    return closest;
}
