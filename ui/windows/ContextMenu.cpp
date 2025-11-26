#include "ContextMenu.h"
#include <QAction>
#include <QIcon>
#include <QWidget>

ContextMenu::ContextMenu(QWidget* parent) : QMenu(parent)
{
    setTitle("Вид");
    setupActions();
}

void ContextMenu::setupActions()
{
    QWidget* parent = parentWidget(); // Родительский виджет (Viewport)

    // --- Приблизить ---
    QAction* zoomInAction = new QAction(QIcon(":/icons/zoom-in.svg"), "Приблизить", this);
    zoomInAction->setShortcuts({QKeySequence::ZoomIn, QKeySequence(Qt::Key_Equal)});
    zoomInAction->setShortcutContext(Qt::WindowShortcut);
    connect(zoomInAction, &QAction::triggered, this, &ContextMenu::zoomInTriggered);
    addAction(zoomInAction);
    if (parent) parent->addAction(zoomInAction);

    // --- Отдалить ---
    QAction* zoomOutAction = new QAction(QIcon(":/icons/zoom-out.svg"), "Отдалить", this);
    zoomOutAction->setShortcuts({QKeySequence::ZoomOut, QKeySequence(Qt::Key_Minus)});
    zoomOutAction->setShortcutContext(Qt::WindowShortcut);
    connect(zoomOutAction, &QAction::triggered, this, &ContextMenu::zoomOutTriggered);
    addAction(zoomOutAction);
    if (parent) parent->addAction(zoomOutAction);

    // --- Показать все (Fit) ---
    QAction* zoomExtAction = new QAction(QIcon(":/icons/zoom-extents.svg"), "Показать все", this);
    zoomExtAction->setShortcut(Qt::Key_F);
    zoomExtAction->setShortcutContext(Qt::WindowShortcut);
    connect(zoomExtAction, &QAction::triggered, this, &ContextMenu::zoomExtentsTriggered);
    addAction(zoomExtAction);
    if (parent) parent->addAction(zoomExtAction);

    addSeparator();

    // --- Вращение влево ---
    QAction* rotateLeftAction = new QAction(QIcon(":/icons/rotate-left.svg"), "Повернуть влево", this);
    rotateLeftAction->setShortcut(Qt::Key_BracketLeft);
    rotateLeftAction->setShortcutContext(Qt::WindowShortcut);
    connect(rotateLeftAction, &QAction::triggered, this, &ContextMenu::rotateLeftTriggered);
    addAction(rotateLeftAction);
    if (parent) parent->addAction(rotateLeftAction);

    // --- Вращение вправо ---
    QAction* rotateRightAction = new QAction(QIcon(":/icons/rotate-right.svg"), "Повернуть вправо", this);
    rotateRightAction->setShortcut(Qt::Key_BracketRight);
    rotateRightAction->setShortcutContext(Qt::WindowShortcut);
    connect(rotateRightAction, &QAction::triggered, this, &ContextMenu::rotateRightTriggered);
    addAction(rotateRightAction);
    if (parent) parent->addAction(rotateRightAction);
}
