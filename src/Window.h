#pragma once

#include <QMainWindow>

class QMenu;

class Window : public QMainWindow {
	Q_OBJECT;

public:
	explicit Window(QWidget* parent = nullptr);

	void loadGameConfig(const QString& path);

	void regenerateRecentConfigs();

private:
	QMenu* recent;

	QWidget* main;
};
