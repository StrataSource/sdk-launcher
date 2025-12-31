#pragma once

#include <QMainWindow>

class QAction;
class QMenu;

class Window : public QMainWindow {
	Q_OBJECT;

public:
	explicit Window(QWidget* parent = nullptr);

	[[nodiscard]] static QString getStrataIconPath();

	[[nodiscard]] static QString getSDKLauncherIconPath();

	void loadMostRecentGameConfig();

	void loadDefaultGameConfig();

	void loadGameConfig(const QString& path);

	void regenerateRecentConfigs();

private:
	QString gameDefault;
	bool configUsingLegacyBinDir;
	QString configModTemplateURL;

	QMenu* recent;
	QAction* config_loadDefault;
	QAction* game_overrideGame;
	QAction* game_resetToDefault;
	QAction* utilities_createNewMod;
	QAction* utilities_createNewAddon;

	QWidget* main;
};
