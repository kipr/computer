#include "console_widget.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

ConsoleWidget::ConsoleWidget(QWidget *parent)
	: QTextEdit(parent)
  , _offset(0)
  , m_process(0)
{
	setProcess(0);
}

ConsoleWidget::~ConsoleWidget()
{
	setProcess(0);
}

void ConsoleWidget::setProcess(QProcess *process)
{
	if(m_process) m_process->disconnect(this);
	m_process = process;
	setReadOnly(!m_process);
	if(!m_process) return;
	clear();
	m_process->setProcessChannelMode(QProcess::MergedChannels);
	connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(readStandardOut()));
	connect(m_process, SIGNAL(readyReadStandardError()), this, SLOT(readStandardErr()));
}

QProcess *ConsoleWidget::process() const
{
	return m_process;
}

void ConsoleWidget::keyPressEvent(QKeyEvent *event)
{
  if(!m_process) return;
  QTextEdit::keyPressEvent(event);
  if(event->modifiers() != Qt::NoModifier && event->modifiers() != Qt::ShiftModifier) return;
  QString text = event->text();
  // if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) text = "\n";
  
  _current.insert(_current.size() + _offset, text);
  m_process->write(_current.toAscii(), _current.length());
  _current = QString();
}

void ConsoleWidget::readStandardOut()
{
	insertPlainText(m_process->readAllStandardOutput());
	moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
}

void ConsoleWidget::readStandardErr()
{
	insertPlainText(m_process->readAllStandardError());
	moveCursor(QTextCursor::End, QTextCursor::KeepAnchor);
}
