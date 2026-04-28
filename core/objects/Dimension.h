#pragma once

#include "Object.h"
#include "MathUtils.h"

#include <QString>

struct DimensionAnchor {
    const Object* object = nullptr;
    int snapIndex = -1;
    Point fallback;

    Point resolve() const;
};

class Dimension : public Object {
public:
    Dimension(DimensionType type, const DimensionAnchor& a, const DimensionAnchor& b, const Point& linePoint);

    PrimitiveType getType() const override { return PrimitiveType::Dimension; }

    DimensionType getDimensionType() const { return m_type; }
    void setDimensionType(DimensionType type) { m_type = type; }

    const DimensionAnchor& firstAnchor() const { return m_a; }
    const DimensionAnchor& secondAnchor() const { return m_b; }
    void setFirstAnchor(const DimensionAnchor& anchor) { m_a = anchor; }
    void setSecondAnchor(const DimensionAnchor& anchor) { m_b = anchor; }

    Point getLinePoint() const { return m_linePoint; }
    void setLinePoint(const Point& p) { m_linePoint = p; }

    Point getTextPosition() const;
    Point getLineGripPosition() const;
    void setTextPosition(const Point& p);
    double textPositionFactor() const { return m_textPositionFactor; }
    void setTextPositionFactor(double factor);
    void centerText() { m_textPositionFactor = 0.5; }

    QString getTextOverride() const { return m_textOverride; }
    void setTextOverride(const QString& text) { m_textOverride = text; }
    bool hasTextOverride() const { return !m_textOverride.trimmed().isEmpty(); }

    double measuredValue() const;
    QString displayText() const;

    QColor extensionColor() const { return m_extensionColor; }
    void setExtensionColor(const QColor& color) { m_extensionColor = color; }
    QColor dimensionColor() const { return m_dimensionColor; }
    void setDimensionColor(const QColor& color) { m_dimensionColor = color; }
    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& color) { m_textColor = color; }

    LineStyle extensionLineStyle() const { return m_extensionLineStyle; }
    void setExtensionLineStyle(const LineStyle& style) { m_extensionLineStyle = style; }
    LineStyle dimensionLineStyle() const { return m_dimensionLineStyle; }
    void setDimensionLineStyle(const LineStyle& style) { m_dimensionLineStyle = style; }

    double extensionOvershoot() const { return m_extensionOvershoot; }
    void setExtensionOvershoot(double v) { m_extensionOvershoot = v; }
    double dimensionExtension() const { return m_dimensionExtension; }
    void setDimensionExtension(double v) { m_dimensionExtension = v; }
    ArrowType arrowType() const { return m_arrowType; }
    void setArrowType(ArrowType type) { m_arrowType = type; }
    ArrowPlacement arrowPlacement() const { return m_arrowPlacement; }
    void setArrowPlacement(ArrowPlacement placement) { m_arrowPlacement = placement; }
    double arrowSize() const { return m_arrowSize; }
    void setArrowSize(double v) { m_arrowSize = v; }
    bool arrowFilled() const { return m_arrowFilled; }
    void setArrowFilled(bool v) { m_arrowFilled = v; }
    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& f) { m_fontFamily = f; }
    double textHeight() const { return m_textHeight; }
    void setTextHeight(double v) { m_textHeight = v; }
    double textOffset() const { return m_textOffset; }
    void setTextOffset(double v) { m_textOffset = v; }
    DimensionValuePrefix valuePrefix() const { return m_valuePrefix; }
    void setValuePrefix(DimensionValuePrefix prefix) { m_valuePrefix = prefix; }
    double angularRadius() const { return m_angularRadius; }
    void setAngularRadius(double r) { m_angularRadius = r; }
    bool useSupplementaryAngle() const { return m_useSupplementaryAngle; }
    void setUseSupplementaryAngle(bool v) { m_useSupplementaryAngle = v; }
    void toggleAngleSide() { m_useSupplementaryAngle = !m_useSupplementaryAngle; }

    void applyGlobalStyle();
    bool applyMeasuredValue(double newValue);

    std::vector<SnapPoint> getSnapPoints() const override;
    Point getClosestPoint(const Point& p) const override;

private:
    DimensionType m_type;
    DimensionAnchor m_a;
    DimensionAnchor m_b;
    Point m_linePoint;
    double m_textPositionFactor = 0.5;
    QString m_textOverride;

    QColor m_extensionColor = Qt::white;
    QColor m_dimensionColor = Qt::white;
    QColor m_textColor = Qt::white;
    LineStyle m_extensionLineStyle = {LineStyleType::SolidThin, "Сплошная тонкая", 0, 0, false};
    LineStyle m_dimensionLineStyle = {LineStyleType::SolidThin, "Сплошная тонкая", 0, 0, false};
    double m_extensionOvershoot = 8.0;
    double m_dimensionExtension = 0.0;
    ArrowType m_arrowType = ArrowType::Closed;
    ArrowPlacement m_arrowPlacement = ArrowPlacement::Inside;
    double m_arrowSize = 12.0;
    bool m_arrowFilled = true;
    QString m_fontFamily = "Courier New";
    double m_textHeight = 16.0;
    double m_textOffset = 10.0;
    DimensionValuePrefix m_valuePrefix = DimensionValuePrefix::None;
    double m_angularRadius = 0.0;
    bool m_useSupplementaryAngle = false;
};
