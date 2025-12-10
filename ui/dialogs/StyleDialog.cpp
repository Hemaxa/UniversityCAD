#include "StyleDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>

// Инициализация виджетов диалога.
StyleDialog::StyleDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Новый стиль линии");
    setModal(true);
    resize(300, 200);

    // Стили применяются автоматически из styles.qss по селектору QDialog и его детям

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    m_nameEdit = new QLineEdit("Мой стиль");

    m_widthSpin = new QDoubleSpinBox();
    m_widthSpin->setRange(0.1, 20.0);
    m_widthSpin->setValue(0.8);
    m_widthSpin->setSuffix(" мм");

    m_dashSpin = new QDoubleSpinBox();
    m_dashSpin->setRange(0.1, 100.0);
    m_dashSpin->setValue(5.0);

    m_gapSpin = new QDoubleSpinBox();
    m_gapSpin->setRange(0.1, 100.0);
    m_gapSpin->setValue(2.0);

    form->addRow("Название:", m_nameEdit);
    form->addRow("Толщина:", m_widthSpin);
    form->addRow("Длина штриха:", m_dashSpin);
    form->addRow("Пробел:", m_gapSpin);

    layout->addLayout(form);

    auto* btnLayout = new QHBoxLayout();
    auto* okBtn = new QPushButton("Создать");
    auto* cancelBtn = new QPushButton("Отмена");

    connect(okBtn, &QPushButton::clicked, this, &StyleDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &StyleDialog::reject);

    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
}

// Формирует структуру LineStyle на основе введенных данных.
LineStyle StyleDialog::getStyle() const
{
    LineStyle style;
    style.type = LineStyleType::Custom;
    style.name = m_nameEdit->text();
    style.width = m_widthSpin->value();
    style.dashLength = m_dashSpin->value();
    style.gapLength = m_gapSpin->value();
    style.isMain = (style.width >= 0.5);
    return style;
}
