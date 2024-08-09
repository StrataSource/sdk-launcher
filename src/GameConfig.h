#pragma once

#include <optional>
#include <QList>
#include <QString>

constexpr int DEFAULT_WINDOW_WIDTH = 256;
constexpr int DEFAULT_WINDOW_HEIGHT = 450;

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

	[[nodiscard]] const QString& getGameDefault() const { return this->gameDefault; }

	[[nodiscard]] const QString& getGameIcon() const { return this->gameIcon; }

	[[nodiscard]] bool getUsesLegacyBinDir() const { return this->usesLegacyBinDir; }

	[[nodiscard]] int getWindowWidth() const { return DEFAULT_WINDOW_WIDTH; }

	[[nodiscard]] int getWindowHeight() const { return this->windowHeight; }

	[[nodiscard]] const QString& getModTemplateURL() const { return this->modTemplateURL; }

	[[nodiscard]] const QList<Section>& getSections() const { return this->sections; }

	void setVariable(const QString& variable, const QString& replacement);

private:
	QString gameDefault;
	QString gameIcon;
	bool usesLegacyBinDir = false;
	int windowHeight = DEFAULT_WINDOW_HEIGHT;
	QString modTemplateURL;
	QList<Section> sections;

	GameConfig() = default;
};
