#include "Viewport.h"
#include "Scene.h"
#include "Point.h"
#include "Draw.h"
#include "Camera.h"
#include "ContextMenu.h"
#include "Segment.h"
#include "Snapper.h"
#include "Tools.h"
#include "Circle.h"
#include "Arc.h"
#include "Rectangle.h"
#include "Polygon.h"
#include "Ellipse.h"
#include "Spline.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QGridLayout>
#include <QtMath>
#include <QRubberBand>

Viewport::Viewport(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_camera = new Camera(this);
    connect(m_camera, &Camera::updated, this, &Viewport::onCameraUpdated);

    m_contextMenu = new ContextMenu(this);
    connect(m_contextMenu, &ContextMenu::zoomInTriggered, this, &Viewport::zoomIn);
    connect(m_contextMenu, &ContextMenu::zoomOutTriggered, this, &Viewport::zoomOut);
    connect(m_contextMenu, &ContextMenu::zoomExtentsTriggered, this, &Viewport::zoomToExtents);
    connect(m_contextMenu, &ContextMenu::rotateLeftTriggered, this, &Viewport::rotateLeft);
    connect(m_contextMenu, &ContextMenu::rotateRightTriggered, this, &Viewport::rotateRight);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, &Viewport::showContextMenu);

    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);

    m_infoLabel = new QLabel(this);
    m_infoLabel->setObjectName("InfoLabel");
    m_infoLabel->setFixedSize(200, 80);
    m_infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->addWidget(m_infoLabel, 1, 1, Qt::AlignBottom | Qt::AlignRight);
    layout->setRowStretch(0, 1);
    layout->setColumnStretch(0, 1);

    updateInfoLabel();
}

Viewport::~Viewport() {}

void Viewport::setScene(Scene* scene) {
    m_scene = scene;
    m_snapper = std::make_unique<Snapper>(m_scene);
}

void Viewport::setGridSnap(bool enabled) { m_gridSnapEnabled = enabled; }
void Viewport::setObjectSnap(bool enabled) { m_objectSnapEnabled = enabled; }

void Viewport::setActiveTool(PrimitiveType type, int subMethod) {
    m_activeToolType = type;
    m_selectedObjects.clear();
    emit selectionChanged({});

    // Передаем subMethod в инструменты
    switch (type) {
    case PrimitiveType::Segment:
        m_currentTool = std::make_unique<CreateSegmentTool>();
        break;
    case PrimitiveType::Circle:
        m_currentTool = std::make_unique<CreateCircleTool>(subMethod);
        break;
    case PrimitiveType::Rectangle:
        m_currentTool = std::make_unique<CreateRectangleTool>(subMethod);
        break;
    case PrimitiveType::Arc:
        m_currentTool = std::make_unique<CreateArcTool>(subMethod);
        break;
    case PrimitiveType::Ellipse:
        m_currentTool = std::make_unique<CreateEllipseTool>();
        break;
    case PrimitiveType::Polygon:
        m_currentTool = std::make_unique<CreatePolygonTool>();
        break;
    case PrimitiveType::Spline:
        m_currentTool = std::make_unique<CreateSplineTool>();
        break;
    default: m_currentTool.reset(); break;
    }
    update();
}

void Viewport::resetTool() {
    m_activeToolType = PrimitiveType::Generic;
    m_currentTool.reset();
    update();
}

void Viewport::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor("#1A1B26"));

    QTransform worldToScreen = m_camera->getWorldToScreenTransform();
    drawGrid(painter, worldToScreen);
    drawGizmo(painter);

    if (m_scene && m_drawingStrategies) {
        painter.save();
        painter.setTransform(worldToScreen);
        for (const auto& primitive : m_scene->getPrimitives()) {
            auto it = m_drawingStrategies->find(primitive->getType());
            if (it != m_drawingStrategies->end()) {
                bool isSelected = false;
                for (auto* sel : m_selectedObjects) {
                    if (sel == primitive.get()) { isSelected = true; break; }
                }
                it->second->draw(painter, primitive.get(), isSelected);
            }
        }
        if (m_currentTool) {
            m_currentTool->draw(painter, m_camera->getZoomFactor());
        }
        painter.restore();
    }
}

Point getSnappedPoint(const Point& rawWorldP, Snapper* snapper, double scale, bool objSnap, bool gridSnap, int gridStep) {
    Point result = rawWorldP;
    if (objSnap && snapper) {
        auto res = snapper->snap(rawWorldP, scale);
        if (res.snapped) return res.point;
    }
    if (gridSnap) {
        double gs = (double)gridStep;
        double x = std::round(rawWorldP.getX() / gs) * gs;
        double y = std::round(rawWorldP.getY() / gs) * gs;
        result = Point(x, y);
    }
    return result;
}

void Viewport::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true; m_lastPanPos = event->pos(); setCursor(Qt::ClosedHandCursor); return;
    }

    QPointF worldF = screenToWorld(event->position());
    Point worldP(worldF.x(), worldF.y());

    if (m_snapper) {
        m_snapper->setGridSnap(m_gridSnapEnabled, m_gridStep);
        m_snapper->setObjectSnap(m_objectSnapEnabled);
    }

    if (m_currentTool) {
        if (event->button() == Qt::LeftButton) {
            m_currentTool->onMousePress(worldP, *m_snapper, m_camera->getZoomFactor());
            if (m_currentTool->isFinished()) {
                emit objectCreated(m_currentTool->takeObject().release());
                m_currentTool->reset();
            }
            update();
        } else if (event->button() == Qt::RightButton) {
            m_currentTool->finish();
            if (m_currentTool->isFinished()) {
                emit objectCreated(m_currentTool->takeObject().release());
                m_currentTool->reset();
            } else {
                m_currentTool->reset();
            }
            update();
        }
    } else {
        if (event->button() == Qt::LeftButton) {
            if (getGizmoRect().contains(event->pos())) { m_camera->rotateLeft(); return; }
            m_isSelecting = true;
            m_rubberBandOrigin = event->pos();
            m_rubberBand->setGeometry(QRect(m_rubberBandOrigin, QSize()));
            m_rubberBand->show();
        }
    }
}

void Viewport::mouseMoveEvent(QMouseEvent *event) {
    m_currentMouseWorldPos = screenToWorld(event->position());
    Point worldP(m_currentMouseWorldPos.x(), m_currentMouseWorldPos.y());
    updateInfoLabel();

    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();
        m_camera->pan(delta);
    }
    else if (m_currentTool) {
        if (m_snapper) {
            m_snapper->setGridSnap(m_gridSnapEnabled, m_gridStep);
            m_snapper->setObjectSnap(m_objectSnapEnabled);
        }
        m_currentTool->onMouseMove(worldP, *m_snapper, m_camera->getZoomFactor());
        update();
    }
    else if (m_isSelecting) {
        m_rubberBand->setGeometry(QRect(m_rubberBandOrigin, event->pos()).normalized());
    }
}

void Viewport::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_isSelecting && !m_currentTool) {
        m_isSelecting = false; m_rubberBand->hide();
        QRect selectionRect = m_rubberBand->geometry();
        std::vector<Object*> picked = pickObjects(selectionRect);
        m_selectedObjects = picked;
        emit selectionChanged(m_selectedObjects);
        update();
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = false; setCursor(Qt::ArrowCursor);
    }
}

std::vector<Object*> Viewport::pickObjects(const QRect& screenRect) {
    std::vector<Object*> result;
    if (!m_scene) return result;

    for (const auto& obj : m_scene->getPrimitives()) {
        bool inside = false;
        auto check = [&](const Point& p) {
            return screenRect.contains(worldToScreen(QPointF(p.getX(), p.getY())).toPoint());
        };

        switch (obj->getType()) {
        case PrimitiveType::Segment: {
            auto* s = static_cast<Segment*>(obj.get());
            if (check(s->getStart()) && check(s->getEnd())) inside = true;
            break;
        }
        case PrimitiveType::Circle: {
            auto* c = static_cast<Circle*>(obj.get());
            Point center = c->getCenter();
            double r = c->getRadius();
            if (check(center) &&
                check(Point(center.getX()+r, center.getY())) &&
                check(Point(center.getX()-r, center.getY())) &&
                check(Point(center.getX(), center.getY()+r)) &&
                check(Point(center.getX(), center.getY()-r))) inside = true;
            break;
        }
        case PrimitiveType::Rectangle: {
            auto* r = static_cast<Rectangle*>(obj.get());
            Point tl = r->getTopLeft();
            if (check(tl) &&
                check(Point(tl.getX()+r->getWidth(), tl.getY())) &&
                check(Point(tl.getX(), tl.getY()-r->getHeight())) &&
                check(Point(tl.getX()+r->getWidth(), tl.getY()-r->getHeight()))) inside = true;
            break;
        }
        case PrimitiveType::Polygon: {
            auto* p = static_cast<PolygonObj*>(obj.get());
            Point c = p->getCenter(); double r = p->getRadius();
            if (check(Point(c.getX()-r, c.getY()-r)) && check(Point(c.getX()+r, c.getY()+r))) inside = true;
            break;
        }
        default: break;
        }

        if (inside) {
            result.push_back(obj.get());
        }
    }
    return result;
}

void Viewport::updateInfoLabel() {
    QString coordText;
    if (m_coordSystemType == CoordinateSystemType::Cartesian) {
        coordText = QString("X: %1\nY: %2").arg(m_currentMouseWorldPos.x(), 0, 'f', 2).arg(m_currentMouseWorldPos.y(), 0, 'f', 2);
    } else {
        Point p(m_currentMouseWorldPos.x(), m_currentMouseWorldPos.y());
        Point::setAngleUnit(AngleUnit::Degrees);
        coordText = QString("R: %1\nA: %2°").arg(p.getRadius(), 0, 'f', 2).arg(p.getAngle(), 0, 'f', 2);
    }
    QString zoomText = QString("Zoom: %1%").arg((int)(m_camera->getZoomFactor() * 100));
    QString gridText = QString("Grid: %1").arg(m_gridStep);
    m_infoLabel->setText(QString("%1\n%2\n%3").arg(coordText, zoomText, gridText));
}

void Viewport::drawGrid(QPainter& painter, const QTransform& transform) {
    QPen gridPen(QColor(50, 52, 71), 1.0); QPen axisXPen(QColor("#F92672"), 1.5); QPen axisYPen(QColor("#66D9EF"), 1.5);
    painter.save(); painter.setTransform(transform);
    QTransform screenToWorldTf = transform.inverted(); QRectF visibleWorldRect = screenToWorldTf.mapRect(rect());
    double step = calculateDynamicGridStep();
    double startX = std::floor(visibleWorldRect.left() / step) * step; double endX = std::ceil(visibleWorldRect.right() / step) * step;
    double startY = std::floor(visibleWorldRect.top() / step) * step; double endY = std::ceil(visibleWorldRect.bottom() / step) * step;
    for (double x = startX; x <= endX; x += step) { if (std::abs(x) < 1e-9) painter.setPen(axisYPen); else painter.setPen(gridPen); painter.drawLine(QPointF(x, startY), QPointF(x, endY)); }
    for (double y = startY; y <= endY; y += step) { if (std::abs(y) < 1e-9) painter.setPen(axisXPen); else painter.setPen(gridPen); painter.drawLine(QPointF(startX, y), QPointF(endX, y)); }
    painter.restore();
}
void Viewport::drawGizmo(QPainter& painter) {
    painter.save(); painter.setRenderHint(QPainter::Antialiasing);
    int size = 40; int padding = 50; QPoint origin(padding, height() - padding);
    QTransform gizmoTransform; gizmoTransform.translate(origin.x(), origin.y());
    gizmoTransform.rotate(-m_camera->getRotationAngle()); gizmoTransform.scale(1, -1);
    QPen axisXPen(QColor("#F92672"), 2.0); QPen axisYPen(QColor("#66D9EF"), 2.0);
    painter.setTransform(gizmoTransform); painter.setPen(axisXPen); painter.setBrush(QColor("#F92672")); painter.drawLine(0, 0, size, 0);
    QPolygonF xArrow; xArrow << QPointF(size, 0) << QPointF(size - 8, 4) << QPointF(size - 8, -4); painter.drawPolygon(xArrow); painter.resetTransform();
    QPointF xPos = gizmoTransform.map(QPointF(size + 10, 0)); painter.setPen(Qt::white); painter.drawText(xPos, "X");
    painter.setTransform(gizmoTransform); painter.setPen(axisYPen); painter.setBrush(QColor("#66D9EF")); painter.drawLine(0, 0, 0, size);
    QPolygonF yArrow; yArrow << QPointF(0, size) << QPointF(-4, size - 8) << QPointF(4, size - 8); painter.drawPolygon(yArrow); painter.resetTransform();
    QPointF yPos = gizmoTransform.map(QPointF(0, size + 10)); painter.setPen(Qt::white); painter.drawText(yPos, "Y");
    painter.setPen(Qt::NoPen); painter.setBrush(Qt::white); painter.drawEllipse(origin, 3, 3); painter.restore();
}
QPointF Viewport::worldToScreen(const QPointF& worldPos) const { return m_camera->getWorldToScreenTransform().map(worldPos); }
QPointF Viewport::screenToWorld(const QPointF& screenPos) const { return m_camera->getScreenToWorldTransform().map(screenPos); }
QRect Viewport::getGizmoRect() const { return QRect(0, height() - 70, 70, 70); }
double Viewport::calculateDynamicGridStep() const { double zoom = m_camera->getZoomFactor(); double step = m_gridStep; while (step * zoom < 15) step *= 2; while (step * zoom > 150) step /= 2; return step; }
void Viewport::setDrawingStrategies(const std::map<PrimitiveType, std::unique_ptr<Draw>>* strategies) { m_drawingStrategies = strategies; }
void Viewport::setGridStep(int step) { if (step > 0) { m_gridStep = step; update(); } }
void Viewport::setZoomStep(double step) { if (step > 0) { m_zoomStep = step; } }
void Viewport::setCoordinateSystem(CoordinateSystemType type) { m_coordSystemType = type; updateInfoLabel(); }
void Viewport::setSelectedObjects(const std::vector<Object*>& objs) { m_selectedObjects = objs; update(); }
void Viewport::update() { QWidget::update(); }
void Viewport::onCameraUpdated() { updateInfoLabel(); update(); }
void Viewport::showContextMenu(const QPoint& pos) { m_contextMenu->exec(mapToGlobal(pos)); }
void Viewport::zoomIn() { m_camera->applyZoom(m_zoomStep, rect().center()); }
void Viewport::zoomOut() { m_camera->applyZoom(1.0/m_zoomStep, rect().center()); }
void Viewport::rotateLeft() { m_camera->rotateLeft(); }
void Viewport::rotateRight() { m_camera->rotateRight(); }
void Viewport::wheelEvent(QWheelEvent *event) { double factor = 1.0 + (event->angleDelta().y() / 8.0) / 100.0; m_camera->applyZoom(factor, event->position().toPoint()); }
void Viewport::resizeEvent(QResizeEvent *event) { m_camera->setCanvasSize(event->size()); }
void Viewport::zoomToExtents() { }
