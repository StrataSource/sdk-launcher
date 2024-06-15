#include "GameFinder.h"

#include <filesystem>

#include <QFile>
#include <QTextStream>
#include <SAPP/SAPP.h>

#include "Config.h"

namespace {

[[nodiscard]] unsigned int getDefaultGameAppIdLocal(const std::filesystem::path& path) {
	// Make sure directory structure looks right
	std::filesystem::path steamAppIdTxtPath;
	if ((path.filename() == "win64" || path.filename() == "linux64") && path.parent_path().filename() == "bin") {
		steamAppIdTxtPath = path.parent_path().parent_path() / "steam_appid.txt";
	} else if (path.filename() == "bin") {
		steamAppIdTxtPath = path.parent_path() / "steam_appid.txt";
	}
	if (!std::filesystem::exists(steamAppIdTxtPath)) {
		return 0;
	}

	// Use steam_appid.txt
	QString appId;
	if (QFile file(steamAppIdTxtPath.string().c_str()); file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		appId += in.readAll();
		file.close();
	} else {
		return 0;
	}
	return appId.trimmed().toUInt();
}

[[nodiscard]] SAPP& getSAPP() {
	static SAPP sapp;
	return sapp;
}

[[nodiscard]] unsigned int getDefaultGameAppIdSteam(const std::filesystem::path& path) {
	auto& sapp = ::getSAPP();
	if (!sapp) {
		return 0;
	}

	for (auto appId : sapp.getInstalledApps()) {
		if (QString(path.string().c_str()).startsWith(sapp.getAppInstallDir(appId).c_str())) {
			return appId;
		}
	}
	return 0;
}

} // namespace

unsigned int GameFinder::getDefaultGameAppId() {
	auto path = std::filesystem::current_path();

	// If we're in a bin directory, find the game that we're inside and use that
	if (auto localAppId = ::getDefaultGameAppIdLocal(path)) {
		return localAppId;
	}

	// If we're in a Steam game, find the game that we're inside and use that
	if (auto steamAppId = ::getDefaultGameAppIdSteam(path)) {
		return steamAppId;
	}

	// Just use the default
	return PROJECT_DEFAULT_APPID;
}

QString GameFinder::getGameInstallPath(unsigned int appId) {
	auto& sapp = ::getSAPP();
	if (!sapp) {
		return "";
	}
	return sapp.getAppInstallDir(appId).c_str();
}

QIcon GameFinder::getGameIcon(unsigned int appId) {
	auto& sapp = ::getSAPP();
	if (!sapp) {
		return QIcon{};
	}
	return QIcon{sapp.getAppIconPath(appId).c_str()};
}
