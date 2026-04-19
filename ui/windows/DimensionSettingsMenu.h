#pragma once

#include <QMenu>
#include <QColor>
#include <functional>

class QDoubleSpinBox;
class QComboBox;
class QPushButton;

class DimensionSettingsMenu : public QMenu {
    Q_OBJECT
public:
    explicit DimensionSettingsMenu(QWidget* parent = nullptr);

signals:
    void settingsChanged();
    void applyToExistingRequested();

private:
    QDoubleSpinBox* createDoubleSpin(double val, double min, double max, double step);
    QPushButton* createColorButton(const QColor& color, const std::function<void(const QColor&)>& setter);
    void setupUi();
};
