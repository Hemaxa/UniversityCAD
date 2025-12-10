#pragma once

#include <QDialog>
#include "Object.h"

class QLineEdit;
class QDoubleSpinBox;

class StyleDialog : public QDialog
{
    Q_OBJECT

public:
    // Конструктор диалога.
    explicit StyleDialog(QWidget *parent = nullptr);

    // Возвращает созданный пользователем стиль.
    LineStyle getStyle() const;

private:
    QLineEdit* m_nameEdit;       // Поле ввода названия
    QDoubleSpinBox* m_dashSpin;  // Поле длины штриха
    QDoubleSpinBox* m_gapSpin;   // Поле длины пробела
    QDoubleSpinBox* m_widthSpin; // Поле толщины
};
