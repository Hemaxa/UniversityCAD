#include "CadWindow.h"

#include <QApplication>
#include <QFile>

// Главная функция, точка входа в приложение.
int main(int argc, char *argv[])
{
    // Инициализация приложения Qt.
    QApplication a(argc, argv);

    // Загрузка и применение таблицы стилей QSS.
    QFile file(":/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(file.readAll());
        file.close();
    }

    // Создание и отображение главного окна в развернутом виде.
    CadWindow w;
    w.showMaximized();

    // Запуск цикла обработки событий приложения.
    return a.exec();
}
