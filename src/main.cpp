#include "main_window.hpp"

#include <QApplication>
#include <time.h>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	srand(time(NULL));
		
#ifdef Q_OS_MAC
	QDir::setCurrent(QApplication::applicationDirPath() + "/../");
#else
	QDir::setCurrent(QApplication::applicationDirPath());
#endif
	
	MainWindow mainWindow;
	mainWindow.show();
	mainWindow.raise();
	
	return app.exec();
}
