#include "color_box.hpp"

#include <QColorDialog>

ColorBox::ColorBox(QWidget *parent)
	: QLineEdit(parent)
{
	setReadOnly(true);
	m_color = palette().color(QPalette::Base);
}

ColorBox::~ColorBox()
{
}

QColor ColorBox::getColor() const
{
	return m_color;
}

void ColorBox::setColor(QColor color)
{
	QPalette pal = palette();
	pal.setColor(QPalette::Base, color);
	setPalette(pal);
	m_color = color;
}

void ColorBox::mouseDoubleClickEvent(QMouseEvent *event)
{
	QColorDialog diag;
	if(!diag.exec()) return;
	setColor(diag.selectedColor());
}
