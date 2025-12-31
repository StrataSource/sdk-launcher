#include "Options.h"

#include "Config.h"

QSettings& Options::get() {
	static QSettings s{QString{"%1.ini"}.arg(PROJECT_TARGET_NAME.data()), QSettings::Format::IniFormat};
	return s;
}

bool Options::contains(std::string_view key) {
	return get().contains(key);
}

void Options::remove(std::string_view key) {
	get().remove(key);
}
