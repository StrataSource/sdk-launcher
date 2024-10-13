#pragma once

#include <QToolButton>

class QMouseEvent;

class LaunchButton : public QToolButton {
	Q_OBJECT;

public:
	using QToolButton::QToolButton;

protected:
	// Actually incredible Qt doesn't expose this already
	void mouseDoubleClickEvent(QMouseEvent*) override {
		emit this->doubleClicked();
	}

signals:
	void doubleClicked();
};
