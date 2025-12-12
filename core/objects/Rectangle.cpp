#include "Rectangle.h"

Rectangle::Rectangle(const Point& topLeft, double width, double height, double cornerRadius)
    : m_topLeft(topLeft), m_width(width), m_height(height), m_cornerRadius(cornerRadius) {}

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
    Point tl = m_topLeft;
    Point tr(tl.getX() + m_width, tl.getY());
    Point bl(tl.getX(), tl.getY() - m_height);
    Point br(tl.getX() + m_width, tl.getY() - m_height);

    snaps.push_back({tl, SnapType::Endpoint});
    snaps.push_back({tr, SnapType::Endpoint});
    snaps.push_back({bl, SnapType::Endpoint});
    snaps.push_back({br, SnapType::Endpoint});

    snaps.push_back({Point(tl.getX() + m_width/2, tl.getY()), SnapType::Midpoint});
    snaps.push_back({Point(tl.getX() + m_width/2, tl.getY() - m_height), SnapType::Midpoint});
    snaps.push_back({Point(tl.getX(), tl.getY() - m_height/2), SnapType::Midpoint});
    snaps.push_back({Point(tl.getX() + m_width, tl.getY() - m_height/2), SnapType::Midpoint});

    snaps.push_back({Point(tl.getX() + m_width/2, tl.getY() - m_height/2), SnapType::Center});
    return snaps;
}
