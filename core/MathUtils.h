#pragma once
#include "Point.h"
#include <cmath>

namespace MathUtils {

inline double dist(const Point& p1, const Point& p2) {
    return std::sqrt(std::pow(p1.getX() - p2.getX(), 2) + std::pow(p1.getY() - p2.getY(), 2));
}

inline bool getCircleFrom3Points(const Point& p1, const Point& p2, const Point& p3, Point& center, double& radius) {
    double x1 = p1.getX(), y1 = p1.getY();
    double x2 = p2.getX(), y2 = p2.getY();
    double x3 = p3.getX(), y3 = p3.getY();
    double D = 2 * (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
    if (std::abs(D) < 1e-9) return false;
    double Ux = ((x1*x1 + y1*y1) * (y2 - y3) + (x2*x2 + y2*y2) * (y3 - y1) + (x3*x3 + y3*y3) * (y1 - y2)) / D;
    double Uy = ((x1*x1 + y1*y1) * (x3 - x2) + (x2*x2 + y2*y2) * (x1 - x3) + (x3*x3 + y3*y3) * (x2 - x1)) / D;
    center = Point(Ux, Uy);
    radius = std::sqrt(std::pow(Ux - x1, 2) + std::pow(Uy - y1, 2));
    return true;
}

}
