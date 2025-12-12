#pragma once

#include "Point.h"
#include "Enums.h"
#include <vector>
#include <memory>
#include <optional>

class Object;
class Scene;

struct SnapResult {
    bool snapped = false;
    Point point;
    SnapType type = SnapType::None;
};

class Snapper {
public:
    static const int SNAP_DISTANCE = 15;

    Snapper(const Scene* scene);

    void setGridSnap(bool enabled, int step);
    void setObjectSnap(bool enabled);

    SnapResult snap(const Point& mouseWorldPos, double scaleFactor) const;

private:
    const Scene* m_scene;
    bool m_gridSnapEnabled = false;
    int m_gridStep = 50;
    bool m_objSnapEnabled = true;
};
