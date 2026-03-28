#pragma once

#include "Enums.h"
#include "Point.h"
#include <QPainter>
#include <memory>
#include <optional>
#include <vector>

class Scene;
class Snapper;
class Object;

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
    
    // Для отрисовки пунктирного продолжения касательной
    std::optional<std::pair<Point, Point>> m_tangentExtensionLine;
};

// --- Circle ---
// Методы: 0 - Центр+Радиус, 1 - Центр+Диаметр, 2 - 2 точки (диаметр), 3 - 3 точки
class CreateCircleTool : public Tool {
public:
    explicit CreateCircleTool(int method);
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
// Методы: 0 - 2 точки, 1 - Точка+Размер, 2 - Центр+Размер
class CreateRectangleTool : public Tool {
public:
    explicit CreateRectangleTool(int method);
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
// Методы: 0 - Центр+Углы, 1 - 3 точки
class CreateArcTool : public Tool {
public:
    explicit CreateArcTool(int method);
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
// Методы: 0 - Центр+Радиусы, 1 - Центр+2 оси (точки)
class CreateEllipseTool : public Tool {
public:
    explicit CreateEllipseTool(int method = 0);
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    int m_method;
    std::vector<Point> m_clicks;
    Point m_cursorPos;
    double m_rx = 0, m_ry = 0;
    std::unique_ptr<Object> m_result;
};

// --- Polygon ---
// Методы: 0 - Центр+Радиус (вписанный), 1 - Центр+Радиус (описанный)
class CreatePolygonTool : public Tool {
public:
    explicit CreatePolygonTool(int method = 0);
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    int m_method;
    std::optional<Point> m_center;
    double m_radius = 0;
    std::unique_ptr<Object> m_result;
};

// --- Point ---
class CreatePointTool : public Tool {
public:
    void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) override;
    void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) override;
    void draw(QPainter& painter, double scale) override;
    std::unique_ptr<Object> takeObject() override;
    void reset() override;
private:
    std::unique_ptr<Object> m_result;
    Point m_cursorPos;
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
