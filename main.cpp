#include "CadWindow.h"
#include <QApplication>
#include <QString>

/**
 * @brief Главная функция, точка входа в приложение.
 * @param argc Количество аргументов командной строки.
 * @param argv Массив строк аргументов командной строки.
 * @return int Код выхода приложения.
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Обновленная строка QSS для более темной темы с акцентным голубым цветом
    QString styleSheet = R"(
        /* Общий стиль в темно-серых тонах */
        QWidget {
            background-color: #2D2D2D; /* Очень темный серый фон */
            color: #EAEAEA;            /* Светло-серый цвет текста */
            font-family: "Segoe UI", Arial, sans-serif;
            font-size: 12px;
        }

        /* Стиль для полей ввода */
        QLineEdit, QDoubleSpinBox {
            background-color: #3D3D3D;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 5px;
        }
        QLineEdit:focus, QDoubleSpinBox:focus {
            border: 1px solid #007ACC; /* Акцентный голубой при фокусе */
        }

        /* Акцентный голубой для основных кнопок */
        QPushButton {
            background-color: #007ACC;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #005A9E;
        }
        QPushButton#deleteButton { /* Особый стиль для кнопки удаления */
            background-color: #C75450; /* Красный цвет */
        }
        QPushButton#deleteButton:hover {
            background-color: #A54541;
        }


        /* Кнопки-переключатели */
        QToolButton {
            background-color: #3D3D3D;
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 5px;
        }
        QToolButton:checked {
            background-color: #007ACC;
            border: 1px solid #007ACC;
        }
        QToolButton:hover {
            border: 1px solid #777777;
        }

        /* Группы и метки */
        QGroupBox {
            font-weight: bold;
            font-size: 14px;
            margin-top: 10px;
            border: 1px solid #555555;
            border-radius: 4px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top center;
            padding: 0 5px;
        }

        /* Список объектов */
        QListWidget {
             border: 1px solid #555555;
             border-radius: 4px;
        }
        QListWidget::item:selected {
            background-color: #007ACC;
        }

        /* НОВЫЙ СТИЛЬ: Информационная панель в окне просмотра */
        #InfoLabel {
            background-color: rgba(45, 45, 45, 0.85); /* Полупрозрачный фон */
            border: 1px solid #555555;
            border-radius: 4px;
            padding: 5px;
            font-size: 11px;
        }

        /* Разделители панелей */
        QSplitter::handle {
            background-color: #555555;
        }
        QSplitter::handle:horizontal { width: 1px; }
        QSplitter::handle:vertical { height: 1px; }
    )";

    a.setStyleSheet(styleSheet);

    CadWindow w;
    w.showMaximized();

    return a.exec();
}
