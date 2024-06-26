#pragma once

#include <QMainWindow>

class Window : public QMainWindow {
	Q_OBJECT;

public:
	explicit Window(QWidget* parent = nullptr);

	void loadGameConfig(const QString& path);

private:
	QWidget* main;
};
