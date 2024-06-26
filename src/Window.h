#pragma once

#include <QMainWindow>

class QAction;
class QMenu;

class Window : public QMainWindow {
	Q_OBJECT;

public:
	explicit Window(QWidget* parent = nullptr);

	[[nodiscard]] static QIcon getStrataIcon();

	void loadGameConfig(const QString& path);

	void regenerateRecentConfigs();

private:
	QMenu* recent;
	QAction* loadDefault;

	QWidget* main;
};
