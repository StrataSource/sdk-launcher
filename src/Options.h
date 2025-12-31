#pragma once

#include <QSettings>

constexpr std::string_view STR_RECENT_CONFIGS = "str_recent_configs";
constexpr std::string_view STR_GAME_OVERRIDE = "str_game_override";

constexpr std::string_view BOOL_SINGLE_CLICK_TO_RUN = "opt_single_click_to_run";
constexpr bool BOOL_SINGLE_CLICK_TO_RUN_DEFAULT = true;

namespace Options {

[[nodiscard]] QSettings& get();

[[nodiscard]] bool contains(std::string_view key);

template<typename T>
[[nodiscard]] T get(std::string_view key, T value = {}) {
	return qvariant_cast<T>(get().value(key, value));
}

void set(std::string_view key, auto value) {
	get().setValue(key, value);
}

void remove(std::string_view key);

} // namespace Options
