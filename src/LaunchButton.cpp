#include "LaunchButton.h"

#include "Options.h"

LaunchButton::LaunchButton(QWidget* parent)
		: QToolButton(parent) {
	QObject::connect(this, &LaunchButton::clicked, this, [this] {
		if (Options::get<bool>(BOOL_SINGLE_CLICK_TO_RUN, BOOL_SINGLE_CLICK_TO_RUN_DEFAULT)) {
			emit this->launch();
		}
	});
}

void LaunchButton::mouseDoubleClickEvent(QMouseEvent* event) {
	if (!Options::get<bool>(BOOL_SINGLE_CLICK_TO_RUN, BOOL_SINGLE_CLICK_TO_RUN_DEFAULT)) {
		emit this->launch();
	}
	QToolButton::mouseDoubleClickEvent(event);
}
