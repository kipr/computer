#ifndef _COLOR_BOX_HPP_
#define _COLOR_BOX_HPP_

#include <QLineEdit>

class ColorBox : public QLineEdit
{
Q_OBJECT
public:
	ColorBox(QWidget *parent = 0);
	~ColorBox();
	QColor getColor() const;
	void setColor(QColor);

private:
	QColor m_color;
	
	void mouseDoubleClickEvent(QMouseEvent *);
};

#endif
