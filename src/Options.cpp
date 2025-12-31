#include "Options.h"

#include "Config.h"

QSettings& options() {
	static QSettings s{QString{"%1.ini"}.arg(PROJECT_TARGET_NAME.data()), QSettings::Format::IniFormat};
	return s;
}
