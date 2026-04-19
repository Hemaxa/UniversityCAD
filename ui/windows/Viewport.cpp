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
#include "PointObject.h"
#include "Dimension.h"
#include "MathUtils.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QGridLayout>
#include <QtMath>
#include <QRubberBand>
#include <QInputDialog>
#include <limits>

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
    connect(m_contextMenu, &ContextMenu::dimensionToolTriggered, this, &Viewport::setActiveDimensionTool);

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
    m_activeSubMethod = subMethod;
    m_selectedObjects.clear();
    emit selectionChanged({});

    // Передаем subMethod в инструменты для всех типов
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
        m_currentTool = std::make_unique<CreateEllipseTool>(subMethod);
        break;
    case PrimitiveType::Polygon:
        m_currentTool = std::make_unique<CreatePolygonTool>(subMethod);
        break;
    case PrimitiveType::Spline:
        m_currentTool = std::make_unique<CreateSplineTool>();
        break;
    case PrimitiveType::Point:
        m_currentTool = std::make_unique<CreatePointTool>();
        break;
    case PrimitiveType::Dimension:
        m_currentTool = std::make_unique<CreateDimensionTool>(static_cast<DimensionType>(subMethod));
        break;
    default: m_currentTool.reset(); break;
    }
    update();
}

void Viewport::setActiveDimensionTool(DimensionType type)
{
    setActiveTool(PrimitiveType::Dimension, static_cast<int>(type));
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
                // Безопасно: takeObject() возвращает unique_ptr, release() передает владение
                // в onObjectCreateRequested, где объект сразу оборачивается в unique_ptr
                emit objectCreated(m_currentTool->takeObject().release());
                m_currentTool->reset();
            }
            update();
        } else if (event->button() == Qt::RightButton) {
            m_currentTool->finish();
            if (m_currentTool->isFinished()) {
                // Безопасно: takeObject() возвращает unique_ptr, release() передает владение
                // в onObjectCreateRequested, где объект сразу оборачивается в unique_ptr
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

            if (m_scene) {
                const auto& primitives = m_scene->getPrimitives();
                for (auto it = primitives.rbegin(); it != primitives.rend(); ++it) {
                    if ((*it)->getType() != PrimitiveType::Dimension) continue;
                    auto* dim = static_cast<Dimension*>(it->get());
                    if (beginDimensionGripDrag(dim, worldP)) {
                        m_selectedObjects = {dim};
                        emit selectionChanged(m_selectedObjects);
                        update();
                        return;
                    }
                }
            }
            
            // Проверяем, попал ли клик непосредственно по объекту
            Object* clickedObject = pickObjectAtPoint(event->pos());
            if (clickedObject) {
                if (clickedObject->getType() == PrimitiveType::Dimension) {
                    auto* dim = static_cast<Dimension*>(clickedObject);
                    Point tp = dim->getTextPosition();
                    const double textHit = 28.0 / m_camera->getZoomFactor();
                    if (MathUtils::dist(worldP, tp) <= textHit) {
                        m_draggingDimensionText = dim;
                    }
                }
                // Выделяем объект для редактирования только если клик попал по нему
                m_selectedObjects = {clickedObject};
                emit selectionChanged(m_selectedObjects);
                update();
                return;
            }
            
            // Если не попали по объекту, начинаем выделение рамкой
            // Выделение рамкой активируется при зажатии кнопки мыши
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
    else if (m_draggingDimensionText) {
        Point target = worldP;
        if (m_snapper) {
            m_snapper->setGridSnap(m_gridSnapEnabled, m_gridStep);
            m_snapper->setObjectSnap(false);
            auto res = m_snapper->snap(worldP, m_camera->getZoomFactor());
            if (res.snapped) target = res.point;
            m_snapper->setObjectSnap(m_objectSnapEnabled);
        }
        m_draggingDimensionText->setTextPosition(target);
        emit selectionChanged(m_selectedObjects);
        update();
    }
    else if (m_draggingDimensionGrip) {
        Point target = worldP;
        DimensionAnchor anchor;
        if (m_snapper) {
            m_snapper->setGridSnap(m_gridSnapEnabled, m_gridStep);
            m_snapper->setObjectSnap(m_objectSnapEnabled);
            auto res = m_snapper->snap(worldP, m_camera->getZoomFactor());
            if (res.snapped) {
                target = res.point;
                if (res.object && res.object->getType() != PrimitiveType::Dimension) {
                    anchor.object = res.object;
                    anchor.snapIndex = res.snapIndex;
                }
            }
        }
        anchor.fallback = target;
        if (m_dimensionGripIndex == 0) {
            m_draggingDimensionGrip->setFirstAnchor(anchor);
        } else if (m_dimensionGripIndex == 1) {
            m_draggingDimensionGrip->setSecondAnchor(anchor);
        } else {
            m_draggingDimensionGrip->setLinePoint(target);
        }
        emit selectionChanged(m_selectedObjects);
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
    } else if (event->button() == Qt::LeftButton && m_draggingDimensionText) {
        m_draggingDimensionText = nullptr;
    } else if (event->button() == Qt::LeftButton && m_draggingDimensionGrip) {
        m_draggingDimensionGrip = nullptr;
        m_dimensionGripIndex = -1;
    }
}

void Viewport::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) return;
    QPointF worldF = screenToWorld(event->position());
    Point worldP(worldF.x(), worldF.y());
    Object* clickedObject = pickObjectAtPoint(event->pos());
    if (!clickedObject || clickedObject->getType() != PrimitiveType::Dimension) return;

    auto* dim = static_cast<Dimension*>(clickedObject);
    const double textHit = 22.0 / m_camera->getZoomFactor();
    if (MathUtils::dist(worldP, dim->getTextPosition()) > textHit) return;

    bool ok = false;
    double value = QInputDialog::getDouble(this, "Изменить размер", "Значение:", dim->measuredValue(), 0.001, 1000000.0, 2, &ok);
    if (!ok) return;
    dim->setTextOverride(QString());
    dim->applyMeasuredValue(value);
    m_selectedObjects = {dim};
    emit selectionChanged(m_selectedObjects);
    update();
}

bool Viewport::beginDimensionGripDrag(Dimension* dim, const Point& worldPoint)
{
    if (!dim) return false;
    const double hit = 20.0 / m_camera->getZoomFactor();
    std::vector<Point> grips;
    if (dim->getDimensionType() == DimensionType::Radius || dim->getDimensionType() == DimensionType::Diameter) {
        grips = {dim->getLinePoint(), dim->firstAnchor().resolve(), dim->secondAnchor().resolve()};
    } else {
        grips = {dim->firstAnchor().resolve(), dim->secondAnchor().resolve(), dim->getLineGripPosition(), dim->getLinePoint()};
    }
    double best = hit;
    int bestIndex = -1;
    for (int i = 0; i < static_cast<int>(grips.size()); ++i) {
        double d = MathUtils::dist(worldPoint, grips[i]);
        if (d <= best) {
            best = d;
            bestIndex = i;
        }
    }
    if (bestIndex < 0) return false;
    m_draggingDimensionGrip = dim;
    if (dim->getDimensionType() == DimensionType::Radius || dim->getDimensionType() == DimensionType::Diameter) {
        m_dimensionGripIndex = bestIndex == 0 ? 2 : bestIndex - 1;
    } else {
        m_dimensionGripIndex = bestIndex;
    }
    return true;
}

Object* Viewport::pickObjectAtPoint(const QPoint& screenPoint) {
    if (!m_scene) return nullptr;
    
    QPointF worldF = screenToWorld(screenPoint);
    Point worldP(worldF.x(), worldF.y());
    
    // Порог для попадания (в экранных координатах, конвертируем в мировые)
    // Уменьшаем порог для более точного попадания
    double threshold = 3.0 / m_camera->getZoomFactor(); // 3 пикселя в мировых координатах
    
    Object* closestObject = nullptr;
    double minDist = std::numeric_limits<double>::max(); // Начинаем с максимального значения
    
    // Проверяем объекты в обратном порядке (последние нарисованные - сверху)
    const auto& primitives = m_scene->getPrimitives();
    for (auto it = primitives.rbegin(); it != primitives.rend(); ++it) {
        const auto& obj = *it;
        
        // Получаем ближайшую точку на объекте
        Point closest = obj->getClosestPoint(worldP);
        double dist = MathUtils::dist(worldP, closest);
        
        // Сохраняем ближайший объект, если он ближе предыдущего
        if (dist < minDist) {
            minDist = dist;
            closestObject = obj.get();
        }
    }
    
    // Возвращаем объект только если расстояние меньше порога (клик попал по объекту)
    return (minDist <= threshold) ? closestObject : nullptr;
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
        case PrimitiveType::Arc: {
            auto* a = static_cast<Arc*>(obj.get());
            Point center = a->getCenter();
            double r = a->getRadius();
            // Проверяем bounding box дуги
            if (check(Point(center.getX()-r, center.getY()-r)) && 
                check(Point(center.getX()+r, center.getY()+r))) inside = true;
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
        case PrimitiveType::Ellipse: {
            auto* e = static_cast<Ellipse*>(obj.get());
            Point center = e->getCenter();
            double rx = e->getRadiusX();
            double ry = e->getRadiusY();
            if (check(Point(center.getX()-rx, center.getY()-ry)) && 
                check(Point(center.getX()+rx, center.getY()+ry))) inside = true;
            break;
        }
        case PrimitiveType::Polygon: {
            auto* p = static_cast<PolygonObj*>(obj.get());
            Point c = p->getCenter(); double r = p->getRadius();
            if (check(Point(c.getX()-r, c.getY()-r)) && check(Point(c.getX()+r, c.getY()+r))) inside = true;
            break;
        }
        case PrimitiveType::Spline: {
            auto* sp = static_cast<Spline*>(obj.get());
            const auto& pts = sp->getPoints();
            if (!pts.empty()) {
                // Проверяем все контрольные точки
                bool allInside = true;
                for (const auto& p : pts) {
                    if (!check(p)) { allInside = false; break; }
                }
                if (allInside) inside = true;
            }
            break;
        }
        case PrimitiveType::Point: {
            auto* pt = static_cast<PointObject*>(obj.get());
            if (check(pt->getPosition())) inside = true;
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
