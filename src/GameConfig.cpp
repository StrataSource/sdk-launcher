#include "GameConfig.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "GameFinder.h"

GameConfig::ActionType GameConfig::actionTypeFromString(const QString& string) {
	using enum ActionType;
	if (string == "command") {
		return COMMAND;
	} else if (string == "link") {
		return LINK;
	} else if (string == "directory") {
		return DIRECTORY;
	}
	return INVALID;
}

GameConfig::OS GameConfig::osFromString(const QString& string) {
	using enum OS;
	auto out = static_cast<unsigned char>(NONE);
	if (string.contains("windows")) {
		out |= static_cast<unsigned char>(WINDOWS);
	}
	if (string.contains("linux")) {
		out |= static_cast<unsigned char>(LINUX);
	}
	if (out == static_cast<unsigned char>(NONE)) {
		out = static_cast<unsigned char>(ALL);
	}
	return static_cast<OS>(out);
}

std::optional<GameConfig> GameConfig::parse(const QString& path) {
	QString config;
	if (QFile file(path); file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);
		config += in.readAll();
		file.close();
	} else {
		return std::nullopt;
	}

	QJsonDocument configJson = QJsonDocument::fromJson(config.toUtf8());
	if (!configJson.isObject()) {
		return std::nullopt;
	}
	QJsonObject configObject = configJson.object();

	GameConfig gameConfig;

	if (!configObject.contains("type") || !configObject["type"].isString()) {
		return std::nullopt;
	}
	auto configType = configObject["type"].toString();

	if (configType == "steam") {
		if (!configObject.contains("appid") || !configObject["appid"].isString()) {
			return std::nullopt;
		}
		gameConfig.appId = configObject["appid"].toString().toUInt();
		gameConfig.root = GameFinder::getGameInstallPath(gameConfig.appId);
	} else if (configType == "custom") {
		if (!configObject.contains("root") || !configObject["root"].isString()) {
			return std::nullopt;
		}
		gameConfig.root = configObject["root"].toString();
	} else {
		return std::nullopt;
	}

	if (!configObject.contains("sections") || !configObject["sections"].isArray()) {
		return std::nullopt;
	}
	QJsonArray sections = configObject["sections"].toArray();

	for (const auto& sectionValue : sections) {
		if (!sectionValue.isObject()) {
			continue;
		}

		QJsonObject sectionObject = sectionValue.toObject();
		if (!sectionObject.contains("name") || !sectionObject["name"].isString() || !sectionObject.contains("entries") || !sectionObject["entries"].isArray()) {
			continue;
		}

		auto& gameConfigSection = gameConfig.sections.emplace_back();
		gameConfigSection.name = sectionObject["name"].toString();

		auto entries = sectionObject["entries"].toArray();
		for (const auto& entryValue : entries) {
			if (!entryValue.isObject()) {
				continue;
			}

			QJsonObject entryObject = entryValue.toObject();
			if (!entryObject.contains("name") || !entryObject["name"].isString() || !entryObject.contains("type") || !entryObject["type"].isString() || !entryObject.contains("action") || !entryObject["action"].isString()) {
				continue;
			}

			auto& gameConfigSectionEntry = gameConfigSection.entries.emplace_back();
			gameConfigSectionEntry.name = entryObject["name"].toString();
			gameConfigSectionEntry.type = actionTypeFromString(entryObject["type"].toString());
			gameConfigSectionEntry.action = entryObject["action"].toString();

			if (entryObject.contains("arguments") && entryObject["arguments"].isArray()) {
				for (const auto& argument : entryObject["arguments"].toArray()) {
					if (!argument.isString()) {
						continue;
					}
					gameConfigSectionEntry.arguments.push_back(argument.toString());
				}
			}

			if (entryObject.contains("icon_override") && entryObject["icon_override"].isString()) {
				gameConfigSectionEntry.iconOverride = entryObject["icon_override"].toString();
			}

			auto os = static_cast<unsigned char>((!entryObject.contains("os") || !entryObject["os"].isString()) ? OS::ALL : osFromString(entryObject["os"].toString()));
#if defined(_WIN32)
			if (!(os & static_cast<unsigned char>(OS::WINDOWS))) {
				gameConfigSection.entries.pop_back();
			}
#elif defined(__linux__)
			if (!(os & static_cast<unsigned char>(OS::LINUX))) {
				gameConfigSection.entries.pop_back();
			}
#endif
		}

		if (gameConfigSection.entries.empty()) {
			gameConfig.sections.pop_back();
		}
	}
	return gameConfig;
}
