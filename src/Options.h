#pragma once

#include <QSettings>

[[nodiscard]] QSettings& options();

constexpr std::string_view STR_RECENT_CONFIGS = "str_recent_configs";
constexpr std::string_view STR_GAME_OVERRIDE = "str_game_override";

constexpr std::string_view BOOL_SINGLE_CLICK_TO_RUN = "opt_single_click_to_run";
constexpr bool BOOL_SINGLE_CLICK_TO_RUN_DEFAULT = true;
