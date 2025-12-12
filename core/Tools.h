#pragma once

#include "Enums.h"
#include "Point.h"
#include <QPainter>
#include <memory>
#include <optional>
#include <vector>

class Scene;
class Snapper;
class Object; // <--- ДОБАВЛЕНО: Сообщаем компилятору, что класс Object существует

class Tool {
public:
    virtual ~Tool() = default;
    virtual void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {}
    virtual void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {}
    virtual void onMouseRelease(const Point& worldPos) {}
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

// --- Circle ---
class CreateCircleTool : public Tool {
public:
    explicit CreateCircleTool(int method); // 0:R, 1:D, 2:2P, 3:3P
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    int m_method;
    std::vector<Point> m_clicks;
    Point m_cursorPos;
    std::unique_ptr<Object> m_result;
};

// --- Rectangle ---
class CreateRectangleTool : public Tool {
public:
    explicit CreateRectangleTool(int method); // 0:2P, 1:P+Size(не реализован в этом коде), 2:Center+Size
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    int m_method;
    std::optional<Point> m_start;
    Point m_end;
    std::unique_ptr<Object> m_result;
};

// --- Arc ---
class CreateArcTool : public Tool {
public:
    explicit CreateArcTool(int method); // 0:Center, 1:3P
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    int m_method;
    std::vector<Point> m_clicks;
    Point m_cursorPos;
    std::unique_ptr<Object> m_result;
};

// --- Ellipse ---
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

// --- Polygon ---
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

// --- Spline ---
class CreateSplineTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void finish() override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::vector<Point> m_points;
    Point m_currentPos;
    std::unique_ptr<Object> m_result;
};
