#include "QColorBox.h"
#include <QDebug>
#include <QColorDialog>

QColorBox::QColorBox(QWidget *parent)
	: QLineEdit(parent)
{
	setReadOnly(true);
	m_color = palette().color(QPalette::Base);
}

QColorBox::~QColorBox()
{
}

QColor QColorBox::getColor() const
{
	return m_color;
}

void QColorBox::setColor(QColor color)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Base, color);
	setPalette(pal);
	m_color = color;
}

void QColorBox::mouseDoubleClickEvent(QMouseEvent *event)
{
	QColorDialog diag;
	if(diag.exec()) setColor(diag.selectedColor());
}
