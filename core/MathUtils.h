#pragma once
#include "Point.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace MathUtils {

const double PI = 3.14159265358979323846;
const double EPSILON = 1e-9;

inline double distSq(const Point& p1, const Point& p2) {
    return std::pow(p1.getX() - p2.getX(), 2) + std::pow(p1.getY() - p2.getY(), 2);
}

inline double dist(const Point& p1, const Point& p2) {
    return std::sqrt(distSq(p1, p2));
}

// Нормализация угла в диапазон [0, 360)
inline double normalizeAngle(double angleDeg) {
    angleDeg = std::fmod(angleDeg, 360.0);
    if (angleDeg < 0) angleDeg += 360.0;
    return angleDeg;
}

inline bool getCircleFrom3Points(const Point& p1, const Point& p2, const Point& p3, Point& center, double& radius) {
    double x1 = p1.getX(), y1 = p1.getY();
    double x2 = p2.getX(), y2 = p2.getY();
    double x3 = p3.getX(), y3 = p3.getY();
    double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (std::abs(D) < EPSILON) return false;
    double Ux = ((x1*x1 + y1*y1) * (y2 - y3) + (x2*x2 + y2*y2) * (y3 - y1) + (x3*x3 + y3*y3) * (y1 - y2)) / D;
    double Uy = ((x1*x1 + y1*y1) * (x3 - x2) + (x2*x2 + y2*y2) * (x1 - x3) + (x3*x3 + y3*y3) * (x2 - x1)) / D;
    center = Point(Ux, Uy);
    radius = std::sqrt(std::pow(Ux - x1, 2) + std::pow(Uy - y1, 2));
    return true;
}

// Важно для привязок: Касательные
inline std::vector<Point> getTangentPoints(const Point& external, const Point& center, double r) {
    std::vector<Point> res;
    double d2 = distSq(external, center);
    double r2 = r * r;
    if (d2 < r2) return res; // Точка внутри

    double dx = external.getX() - center.getX();
    double dy = external.getY() - center.getY();
    double d = std::sqrt(d2); // Расстояние

    // Угол до центра окружности
    double beta = std::atan2(dy, dx);
    // Угол отклонения касательной
    double alpha = std::asin(r / d);

    double t1 = beta + alpha;
    double t2 = beta - alpha;

    res.push_back(Point(center.getX() + r * std::cos(t1), center.getY() + r * std::sin(t1)));
    res.push_back(Point(center.getX() + r * std::cos(t2), center.getY() + r * std::sin(t2)));
    return res;
}

// Важно для привязок: Проекция точки (для Nearest и Perpendicular)
inline Point projectPointOnSegment(const Point& p, const Point& a, const Point& b) {
    double l2 = distSq(a, b);
    if (l2 < EPSILON) return a;
    double t = ((p.getX() - a.getX()) * (b.getX() - a.getX()) + (p.getY() - a.getY()) * (b.getY() - a.getY())) / l2;
    t = std::max(0.0, std::min(1.0, t));
    return Point(a.getX() + t * (b.getX() - a.getX()), a.getY() + t * (b.getY() - a.getY()));
}

inline Point projectPointOnLine(const Point& p, const Point& a, const Point& b) {
    double l2 = distSq(a, b);
    if (l2 < EPSILON) return a;
    double t = ((p.getX() - a.getX()) * (b.getX() - a.getX()) + (p.getY() - a.getY()) * (b.getY() - a.getY())) / l2;
    return Point(a.getX() + t * (b.getX() - a.getX()), a.getY() + t * (b.getY() - a.getY()));
}

}
