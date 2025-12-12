#pragma once

#include "Enums.h"
#include "Point.h"
#include <QPainter>
#include <memory>
#include <optional>
#include <vector>

class Scene;
class Snapper;

class Tool {
public:
    virtual ~Tool() = default;
    virtual void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {}
    virtual void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {}
    virtual void onMouseRelease(const Point& worldPos) {}
    // Новый метод для принудительного завершения (например, правой кнопкой)
    virtual void finish() {}

    virtual void draw(QPainter& painter, double scale) {}
    virtual bool isFinished() const { return m_finished; }
    virtual std::unique_ptr<Object> takeObject() { return nullptr; }
    virtual void reset() { m_finished = false; }

protected:
    bool m_finished = false;
    Point m_snapPoint;
    bool m_isSnapped = false;
};

// --- Segment ---
class CreateSegmentTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::optional<Point> m_startPoint;
    Point m_endPoint;
    std::unique_ptr<Object> m_result;
};

// --- Circle (Center + Edge) ---
class CreateCircleTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::optional<Point> m_center;
    double m_radius = 0;
    std::unique_ptr<Object> m_result;
};

// --- Rectangle (Corner + Corner) ---
class CreateRectangleTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::optional<Point> m_start;
    Point m_end;
    std::unique_ptr<Object> m_result;
};

// --- Arc (Center + Radius/Start + Span) ---
class CreateArcTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::optional<Point> m_center;
    double m_radius = 0;
    double m_startAngle = 0;
    double m_spanAngle = 0;
    int m_step = 0; // 0: Center, 1: Start/Radius, 2: Span
    Point m_cursorPos;
    std::unique_ptr<Object> m_result;
};

// --- Ellipse (Center + Corner defining Rx/Ry) ---
class CreateEllipseTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::optional<Point> m_center;
    double m_rx = 0, m_ry = 0;
    std::unique_ptr<Object> m_result;
};

// --- Polygon (Center + Radius, fixed 5 sides) ---
class CreatePolygonTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::optional<Point> m_center;
    double m_radius = 0;
    std::unique_ptr<Object> m_result;
};

// --- Spline (Points...) ---
class CreateSplineTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void finish() override; // Вызывается по ПКМ
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::vector<Point> m_points;
    Point m_currentPos;
    std::unique_ptr<Object> m_result;
};
