#pragma once

#include <QMenu>

class ContextMenu : public QMenu
{
    Q_OBJECT

public:
    explicit ContextMenu(QWidget* parent = nullptr);

signals:
    void zoomInTriggered();
    void zoomOutTriggered();
    void zoomExtentsTriggered();
    void rotateLeftTriggered();
    void rotateRightTriggered();

private:
    void setupActions();
};
