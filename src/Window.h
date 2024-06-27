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
	QMenu* recent;
	QAction* config_loadDefault;
	QAction* game_resetToDefault;
	QAction* game_overrideGame;

	QWidget* main;
};
