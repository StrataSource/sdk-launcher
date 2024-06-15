#pragma once

#include <optional>
#include <QList>
#include <QString>

class GameConfig {
public:
	enum class ActionType : unsigned char {
		INVALID,
		COMMAND,
		LINK,
		DIRECTORY,
	};

	[[nodiscard]] static ActionType actionTypeFromString(const QString& string);

	enum class OS : unsigned char {
		NONE    = 0,
		WINDOWS = 1 << 0,
		LINUX   = 1 << 1,
		ALL     = WINDOWS | LINUX,
	};

	[[nodiscard]] static OS osFromString(const QString& string);

	struct Entry {
		QString name;
		ActionType type;
		QString action;
		QStringList arguments;
		QString iconOverride;
	};

	struct Section {
		QString name;
		QList<Entry> entries;
	};

	[[nodiscard]] static std::optional<GameConfig> parse(const QString& path);

	[[nodiscard]] const QList<Section>& getSections() const;

private:
	QList<Section> sections;

	GameConfig() = default;
};
