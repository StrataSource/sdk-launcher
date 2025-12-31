#pragma once

#include <QToolButton>

class QMouseEvent;

class LaunchButton : public QToolButton {
	Q_OBJECT;

public:
	explicit LaunchButton(QWidget* parent = nullptr);

protected:
	void mouseDoubleClickEvent(QMouseEvent* event) override;

signals:
	void launch();
};
