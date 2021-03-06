#ifndef _CONSOLE_WIDGET_HPP_
#define _CONSOLE_WIDGET_HPP_

#include <QTextEdit>
#include <QProcess>

class ConsoleWidget : public QTextEdit
{
Q_OBJECT
public:
	ConsoleWidget(QWidget *parent = 0);
	~ConsoleWidget();
	
	void setProcess(QProcess *process);
	QProcess *process() const;
	
signals:
	void abortRequested();
	
protected:
	void keyPressEvent(QKeyEvent * event);

private slots:
	void readStandardOut();
	void readStandardErr();

private:
  QString _current;
  int _offset;
	QProcess *m_process;
};

#endif
