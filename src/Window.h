#pragma once

#include <QMainWindow>

class QAction;
class QMenu;

class Window : public QMainWindow {
	Q_OBJECT;

public:
	explicit Window(QWidget* parent = nullptr);

	[[nodiscard]] static QString getStrataIconPath();

	void loadGameConfig(const QString& path);

	void regenerateRecentConfigs();

private:
	bool configUsingLegacyBinDir;
	QString configModTemplateURL;

	QMenu* recent;
	QAction* config_loadDefault;
	QAction* game_resetToDefault;
	QAction* game_overrideGame;
	QAction* utilities_createNewMod;

	QWidget* main;
};
