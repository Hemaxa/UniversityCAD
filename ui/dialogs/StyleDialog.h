#pragma once

#include <QDialog>
#include "Object.h"

class QLineEdit;
class QDoubleSpinBox;

class StyleDialog : public QDialog
{
    Q_OBJECT

public:
    // Конструктор для создания нового стиля
    explicit StyleDialog(QWidget *parent = nullptr);
    
    // Конструктор для редактирования существующего стиля
    explicit StyleDialog(const LineStyle& existingStyle, QWidget *parent = nullptr);

    // Возвращает созданный/отредактированный стиль
    LineStyle getStyle() const;

private:
    void setupUi(bool isEditing = false);
    
    QLineEdit* m_nameEdit;       // Поле ввода названия
    QDoubleSpinBox* m_dashSpin;  // Поле длины штриха
    QDoubleSpinBox* m_gapSpin;   // Поле длины пробела
    QDoubleSpinBox* m_widthSpin; // Поле толщины
    
    LineStyle m_existingStyle;   // Существующий стиль (для редактирования)
};
