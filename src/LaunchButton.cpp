#include "LaunchButton.h"

#include "Options.h"

LaunchButton::LaunchButton(QWidget* parent)
		: QToolButton(parent) {
	QObject::connect(this, &LaunchButton::clicked, this, [this] {
		if (::options().value(BOOL_SINGLE_CLICK_TO_RUN, BOOL_SINGLE_CLICK_TO_RUN_DEFAULT).toBool()) {
			emit this->launch();
		}
	});
}

void LaunchButton::mouseDoubleClickEvent(QMouseEvent* event) {
	if (!::options().value(BOOL_SINGLE_CLICK_TO_RUN, BOOL_SINGLE_CLICK_TO_RUN_DEFAULT).toBool()) {
		emit this->launch();
	}
	QToolButton::mouseDoubleClickEvent(event);
}
