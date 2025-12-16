#pragma once

#include <QMenu>
#include <QString>
#include "GlobalSettings.h"

class QDoubleSpinBox;

class LineSettingsMenu : public QMenu {
    Q_OBJECT
public:
    explicit LineSettingsMenu(QWidget* parent = nullptr);
    
    // Возвращает путь к иконке для типа линии (для использования в QIcon)
    static QString getIconPath(LineStyleType type);

signals:
    void settingsChanged();

private:
    QDoubleSpinBox* createDoubleSpin(double val, double min = 0.1, double max = 100.0, double step = 0.5);
    void setupUi();
};
