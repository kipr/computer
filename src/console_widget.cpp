#include "console_widget.hpp"

#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

ConsoleWidget::ConsoleWidget(QWidget *parent)
	: QTextEdit(parent), m_process(0)
{
	cmdStr = "";
	curCursorLoc = this->textCursor();
	inputCharCount = 0;
	histLocation = -1;
	tempCmd = "";
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
	int key = event->key();
	Qt::KeyboardModifiers modifiers = event->modifiers();
	setTextCursor(curCursorLoc);
	
	if(key == Qt::Key_Backspace) {
		if(inputCharCount) {
			--inputCharCount;
			QTextEdit::keyPressEvent(event);
			cmdStr.remove(inputCharCount, 1);
		} else QApplication::beep();
	} else if(key == Qt::Key_Delete) {
		if(!curCursorLoc.atBlockEnd()) {
			QTextEdit::keyPressEvent(event);
			cmdStr.remove(inputCharCount, 1);
		} else QApplication::beep();
	} else if(key == Qt::Key_Return || key == Qt::Key_Enter) {
		inputCharCount = 0;
	} else if(key == Qt::Key_Left) {
		if (inputCharCount) {
			--inputCharCount;
			QTextEdit::keyPressEvent(event);
		} else QApplication::beep();
	} else if(key == Qt::Key_Right) {
		QTextCursor cursor = this->textCursor();
		if(cursor.movePosition(QTextCursor::Right)) {
			++inputCharCount;
			this->setTextCursor(cursor);
		} else QApplication::beep();
	} else if(modifiers == Qt::ControlModifier && key == Qt::Key_C) emit abortRequested();
	else if((modifiers == Qt::ControlModifier && key == Qt::Key_A) || key == Qt::Key_Home) {
		inputCharCount = 0;
		moveCursor(QTextCursor::StartOfLine);
	} else if ((modifiers == Qt::ControlModifier && key == Qt::Key_E) || key == Qt::Key_End) {
		inputCharCount = cmdStr.length();
		moveCursor(QTextCursor::EndOfLine);
	} else if (modifiers == Qt::ControlModifier && key == Qt::Key_K) {
		moveCursor(QTextCursor::EndOfLine);
		for(int i = inputCharCount; i < cmdStr.length(); ++i) {
			QTextEdit::keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier));
		}
		cmdStr.remove(inputCharCount, cmdStr.length() - inputCharCount);
	} else if (modifiers == Qt::ControlModifier && key == Qt::Key_U) {
		for(int i = 0; i < inputCharCount; ++i) {
			QTextEdit::keyPressEvent(new QKeyEvent(QEvent::KeyPress, Qt::Key_Backspace, Qt::NoModifier));
		}
		cmdStr.remove(0, inputCharCount);
		inputCharCount = 0;
	} else {
		QString text = event->text();
		for(int i = 0; i < text.length(); ++i) {
			if (text.at(i).isPrint()) ++inputCharCount;
		}
		QTextEdit::keyPressEvent(event);
	}

	if((key == Qt::Key_Return || key == Qt::Key_Enter) && m_process) {
		moveCursor(QTextCursor::End);
		m_process->write(cmdStr.toAscii().data(), cmdStr.length());
		m_process->write("\r\n", 2);
		QTextEdit::keyPressEvent(event);
		cmdHistory.push_back(cmdStr);
		histLocation = -1;
		cmdStr = "";
		tempCmd = "";
	} else {
		QString input = event->text();
		for(int i = 0; i < input.length(); ++i) {
			if(input.at(i).isPrint()) cmdStr.insert(inputCharCount - 1, input.at(i));
		}
	}
	curCursorLoc = textCursor();
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