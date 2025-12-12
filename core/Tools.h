#pragma once

#include "Enums.h"
#include "Point.h"
#include <QPainter>
#include <memory>
#include <optional>

class Scene;
class Snapper;

// Базовый класс инструмента
class Tool {
public:
    virtual ~Tool() = default;

    virtual void onMousePress(const Point& worldPos, const Snapper& snapper, double scale) {}
    virtual void onMouseMove(const Point& worldPos, const Snapper& snapper, double scale) {}
    virtual void onMouseRelease(const Point& worldPos) {}

    // Отрисовка превью (резиновая нить, привязки)
    virtual void draw(QPainter& painter, double scale) {}

    // Возвращает true, если инструмент завершил создание объекта
    virtual bool isFinished() const { return m_finished; }

    // Забирает созданный объект (передача владения)
    virtual std::unique_ptr<Object> takeObject() { return nullptr; }

    // Сброс состояния инструмента
    virtual void reset() { m_finished = false; }

protected:
    bool m_finished = false;
    Point m_currentMousePos;
    Point m_snapPoint;
    bool m_isSnapped = false;
};

// Инструмент создания отрезка
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

// Инструмент создания окружности (Центр + Радиус)
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
