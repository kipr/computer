#ifndef _INPUT_DIALOG_HPP_
#define _INPUT_DIALOG_HPP_

#include <QDialog>

namespace Ui
{
	class InputDialog;
}

class InputDialog : public QDialog
{
Q_OBJECT
public:
	InputDialog(QWidget *parent = 0);
	
	void setKey(const QString &key);
	QString key() const;
	
	void setValue(const QString &value);
	QString value() const;
	
	void setEmptyAllowed(const bool emptyAllowed);
	bool emptyAllowed() const;
	
private Q_SLOTS:
	void on_value_textChanged(const QString &text);
	
private:
	Ui::InputDialog *ui;
	bool m_emptyAllowed;
};

#endif
