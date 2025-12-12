#pragma once

#include <QMenu>
#include "GlobalSettings.h"

class QDoubleSpinBox;

class LineSettingsMenu : public QMenu {
    Q_OBJECT
public:
    explicit LineSettingsMenu(QWidget* parent = nullptr);

signals:
    void settingsChanged();

private:
    QDoubleSpinBox* createDoubleSpin(double val, double min = 0.1, double step = 0.5);
    void setupUi();
};
