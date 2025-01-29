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
		// 16383 being the maximum length of a path
		static constexpr DWORD STEAM_LOCATION_MAX_SIZE = 16383;
		std::unique_ptr<char[]> steamLocationData{new char[STEAM_LOCATION_MAX_SIZE]};

		HKEY steam;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Valve\Steam)", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &steam) != ERROR_SUCCESS) {
			return "";
		}

		DWORD steamLocationSize = STEAM_LOCATION_MAX_SIZE;
		if (RegQueryValueExA(steam, "InstallPath", nullptr, nullptr, reinterpret_cast<LPBYTE>(steamLocationData.get()), &steamLocationSize) != ERROR_SUCCESS) {
			return "";
		}

		RegCloseKey(steam);
		steamLocation = steamLocationSize > 0 ? std::string(steamLocationData.get(), steamLocationSize - 1) : "";
	}
#else
	{
		std::filesystem::path home{std::getenv("HOME")};
#ifdef __APPLE__
		steamLocation = home / "Library" / "Application Support" / "Steam";
#else
		// Snap install takes priority, the .steam symlink may exist simultaneously with Snap installs
		steamLocation = home / "snap" / "steam" / "common" / ".steam" / "steam";

		if (!std::filesystem::exists(steamLocation, ec)) {
			// Use the regular install path
			steamLocation = home / ".steam" / "steam";
		}
#endif
	}

	if (!std::filesystem::exists(steamLocation, ec)) {
		std::string location;
		std::filesystem::path d{"cwd/steamclient64.dll"};
		for (const auto& entry : std::filesystem::directory_iterator{"/proc/"}) {
			if (std::filesystem::exists(entry / d, ec)) {
				ec.clear();
				const auto s = std::filesystem::read_symlink(entry.path() / "cwd", ec);
				if (ec) {
					continue;
				}
				location = s.string();
				break;
			}
		}
		if (location.empty()) {
			return "";
		} else {
			steamLocation = location;
		}
	}
#endif

	if (auto sourceModPath = (steamLocation / "steamapps" / "sourcemods").string(); std::filesystem::exists(sourceModPath, ec)) {
		return sourceModPath.c_str();
	}
	return "";
}

} // namespace

const QString& getSourceModsDir() {
	static QString location = ::getSourceModsDirHelper();
	return location;
}
