#include "StyleDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>

StyleDialog::StyleDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Новый стиль линии");
    setModal(true);
    resize(300, 200);

    auto* layout = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    m_nameEdit = new QLineEdit("Мой стиль");

    // m_widthSpin оставляем для логики isMain, хотя самой ширины в LineStyle нет
    m_widthSpin = new QDoubleSpinBox();
    m_widthSpin->setRange(0.1, 20.0);
    m_widthSpin->setValue(0.8);
    m_widthSpin->setSuffix(" мм (влияет на тип)");

    m_dashSpin = new QDoubleSpinBox();
    m_dashSpin->setRange(0.1, 100.0);
    m_dashSpin->setValue(5.0);

    m_gapSpin = new QDoubleSpinBox();
    m_gapSpin->setRange(0.1, 100.0);
    m_gapSpin->setValue(2.0);

    form->addRow("Название:", m_nameEdit);
    form->addRow("Толщина (условно):", m_widthSpin);
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

LineStyle StyleDialog::getStyle() const
{
    LineStyle style;
    style.type = LineStyleType::Custom;
    style.name = m_nameEdit->text();
    // style.width больше нет, не присваиваем его
    style.dashLength = m_dashSpin->value();
    style.gapLength = m_gapSpin->value();

    // Используем значение спинбокса только для определения, является ли линия "основной"
    style.isMain = (m_widthSpin->value() >= 0.5);

    return style;
}
