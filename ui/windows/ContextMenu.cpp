#include "ContextMenu.h"
#include <QAction>

ContextMenu::ContextMenu(QWidget* parent) : QMenu(parent)
{
    setTitle("Вид");
    setupActions();
}

void ContextMenu::setupActions()
{
    // Создание действий меню
    QAction* zoomInAction = new QAction("Приблизить (+)", this);
    connect(zoomInAction, &QAction::triggered, this, &ContextMenu::zoomInTriggered);
    addAction(zoomInAction);

    QAction* zoomOutAction = new QAction("Отдалить (-)", this);
    connect(zoomOutAction, &QAction::triggered, this, &ContextMenu::zoomOutTriggered);
    addAction(zoomOutAction);

    QAction* zoomExtAction = new QAction("Показать все", this);
    connect(zoomExtAction, &QAction::triggered, this, &ContextMenu::zoomExtentsTriggered);
    addAction(zoomExtAction);

    addSeparator();

    QAction* rotateLeftAction = new QAction("Повернуть влево", this);
    connect(rotateLeftAction, &QAction::triggered, this, &ContextMenu::rotateLeftTriggered);
    addAction(rotateLeftAction);

    QAction* rotateRightAction = new QAction("Повернуть вправо", this);
    connect(rotateRightAction, &QAction::triggered, this, &ContextMenu::rotateRightTriggered);
    addAction(rotateRightAction);
}
