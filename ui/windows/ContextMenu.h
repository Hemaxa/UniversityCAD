#pragma once

#include <QMenu>

// Класс контекстного меню, предоставляющий быстрый доступ к командам навигации.
class ContextMenu : public QMenu
{
    Q_OBJECT

public:
    // Конструктор контекстного меню.
    explicit ContextMenu(QWidget* parent = nullptr);

signals:
    // Сигнал выбора пункта "Приблизить".
    void zoomInTriggered();

    // Сигнал выбора пункта "Отдалить".
    void zoomOutTriggered();

    // Сигнал выбора пункта "Показать все".
    void zoomExtentsTriggered();

    // Сигнал выбора пункта "Повернуть влево".
    void rotateLeftTriggered();

    // Сигнал выбора пункта "Повернуть вправо".
    void rotateRightTriggered();

private:
    // Создает и настраивает QAction для меню.
    void setupActions();
};
