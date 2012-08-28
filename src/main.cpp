#include "MainWindow.h"

#include <QApplication>
#include "time.h"
#include <kiss-compiler/CompilerPluginManager.h>
#include <kiss-compiler/PlatformHintsManager.h>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	srand(time(NULL));
	
	
#ifdef Q_OS_MAC
	QDir::setCurrent(QApplication::applicationDirPath() + "/../");
#else
	QDir::setCurrent(QApplication::applicationDirPath());
#endif
	
	QString prefix = QDir::currentPath() + "/prefix";
	FlagMap flags = PlatformHintsManager::loadFrom(":/platform/platform.hints");
	foreach(const QString& key, flags.keys()) {
		flags[key] = flags[key].replace("${PREFIX}", prefix);
	}
	PlatformHintsManager::ref().setPlatformHints("computer", flags);
	
	CompilerPluginManager::ref().loadAll(); // Load all available compilers
	
	MainWindow mainWindow;
	mainWindow.show();
	mainWindow.raise();
	
	return app.exec();
}
