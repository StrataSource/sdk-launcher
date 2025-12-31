#include <QApplication>
#include <QStyleHints>

#include "Config.h"
#include "Window.h"

int main(int argc, char** argv) {
	QCoreApplication::setOrganizationName(PROJECT_ORGANIZATION.data());
	QCoreApplication::setApplicationName(PROJECT_NAME.data());
	QCoreApplication::setApplicationVersion(PROJECT_VERSION.data());

#if !defined(__APPLE__) && !defined(_WIN32)
	QGuiApplication::setDesktopFileName(PROJECT_NAME.data());
#endif

	QApplication app(argc, argv);

	if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
		QApplication::setStyle("fusion");
	}

	auto* window = new Window;
	window->show();

	return QApplication::exec();
}
