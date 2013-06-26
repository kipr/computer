#include "input_dialog.hpp"
#include "ui_input_dialog.h"

#include <QPushButton>

InputDialog::InputDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::InputDialog)
{
	ui->setupUi(this);
}

void InputDialog::setKey(const QString &key)
{
	ui->key->setText(key + ":");
}

QString InputDialog::key() const
{
	return ui->key->text();
}

void InputDialog::setValue(const QString &value)
{
	ui->value->setText(value);
}

QString InputDialog::value() const
{
	return ui->value->text();
}

void InputDialog::setEmptyAllowed(const bool emptyAllowed)
{
	m_emptyAllowed = emptyAllowed;
	on_value_textChanged(ui->value->text());
}

bool InputDialog::emptyAllowed() const
{
	return m_emptyAllowed;
}

void InputDialog::on_value_textChanged(const QString &text)
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_emptyAllowed || !text.isEmpty());
}