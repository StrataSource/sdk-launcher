#include "Steam.h"

#include <filesystem>
#ifdef _WIN32
	#include <memory>
	#include <Windows.h>
#else
	#include <cstdlib>
#endif

namespace {

/// Copied from sourcepp
[[nodiscard]] QString getSourceModsDirHelper() {
	std::filesystem::path steamLocation;
	std::error_code ec;

#ifdef _WIN32
	{
		// 16384 being the maximum length of a null-terminated path
		static constexpr DWORD STEAM_LOCATION_MAX_SIZE = 16384;
		std::unique_ptr<wchar_t[]> steamLocationData{new wchar_t[STEAM_LOCATION_MAX_SIZE] {}};

		HKEY steam;
		if (
			RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &steam) != ERROR_SUCCESS &&
			RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &steam) != ERROR_SUCCESS
		) {
			return "";
		}

		DWORD steamLocationSize = STEAM_LOCATION_MAX_SIZE * sizeof(wchar_t);
		if (RegQueryValueExW(steam, L"InstallPath", nullptr, nullptr, reinterpret_cast<LPBYTE>(steamLocationData.get()), &steamLocationSize) != ERROR_SUCCESS) {
			RegCloseKey(steam);
			return "";
		}
		RegCloseKey(steam);

		steamLocation = steamLocationSize > 0 ? std::filesystem::path{steamLocationData.get()} : std::filesystem::path{};
	}
#else
	{
		std::filesystem::path HOME{"~"};
		if (const auto* homeEnv = std::getenv("HOME")) {
			HOME = homeEnv;
		}
#ifdef __APPLE__
		steamLocation = HOME / "Library" / "Application Support" / "Steam";
#else
		std::filesystem::path XDG_DATA_HOME{HOME / ".local" / "share"};
		if (const auto* xdgDataHomeEnv = std::getenv("XDG_DATA_HOME")) {
			XDG_DATA_HOME = xdgDataHomeEnv;
		}

		const std::array locations{
			HOME / "snap" / "steam" / "common" / ".local" / "share" / "Steam", // snap install
			HOME / "snap" / "steam" / "common" / ".steam" / "steam", // snap symlink
			HOME / ".var" / "app" / "com.valvesoftware.Steam" / ".local" / "share" / "Steam", // flatpak install
			HOME / ".var" / "app" / "com.valvesoftware.Steam" / ".steam" / "steam", // flatpak symlink
			XDG_DATA_HOME / "Steam", // expected install (XDG_DATA_HOME)
			HOME / ".local" / "share" / "Steam", // expected install (HOME)
			HOME / ".steam" / "steam", // expected symlink
		};

		for (const auto& location : locations) {
			if (std::filesystem::exists(location, ec)) {
				steamLocation = location;
				break;
			}
		}
		if (steamLocation.empty()) {
			// Find where the Steam process is running from
			std::filesystem::path location;
			const std::filesystem::path d{"cwd/steamclient64.dll"};
			for (const auto& entry : std::filesystem::directory_iterator{"/proc/"}) {
				if (std::filesystem::exists(entry / d, ec)) {
					ec.clear();
					location = std::filesystem::read_symlink(entry.path() / "cwd", ec);
					if (ec) {
						continue;
					}
					break;
				}
			}
			if (location.empty()) {
				return "";
			}
			steamLocation = location;
		}
#endif
	}
#endif

	if (const auto sourceModPath = (steamLocation / "steamapps" / "sourcemods").string(); std::filesystem::exists(sourceModPath, ec)) {
		return sourceModPath.c_str();
	}
	return "";
}

} // namespace

const QString& getSourceModsDir() {
	static QString location = ::getSourceModsDirHelper();
	return location;
}
