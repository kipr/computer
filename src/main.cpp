#include "MainWindow.h"

#include <QApplication>
#include "time.h"
#include <kiss-compiler/CompilerPluginManager.h>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	srand(time(NULL));
	
#ifdef Q_OS_MAC
	QDir::setCurrent(QApplication::applicationDirPath() + "/../");
#else
	QDir::setCurrent(QApplication::applicationDirPath());
#endif
	
	CompilerPluginManager::ref().loadAll(); // Load all available compilers
	
	MainWindow mainWindow;
	mainWindow.show();
	mainWindow.raise();
	
	return app.exec();
}
