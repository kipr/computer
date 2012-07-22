#ifndef _QCOLORBOX_H_
#define _QCOLORBOX_H_

#include <QLineEdit>

class QColorBox : public QLineEdit
{
Q_OBJECT
public:
	QColorBox(QWidget *parent = 0);
	~QColorBox();
	QColor getColor() const;
	void setColor(QColor);

private:
	QColor m_color;
	
	void mouseDoubleClickEvent(QMouseEvent *);
};

#endif
