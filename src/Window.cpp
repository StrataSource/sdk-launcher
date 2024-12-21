#include "Window.h"

#include <chrono>
#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QScrollArea>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QVBoxLayout>

#include "Config.h"
#include "GameConfig.h"
#include "LaunchButton.h"
#include "NewModDialog.h"
#include "NewP2CEAddonDialog.h"

#ifdef _WIN32
#include <shlobj_core.h>
#endif

namespace {

// NOLINTNEXTLINE(*-no-recursion)
void clearLayout(QLayout* layout, bool deleteWidgets = true) {
	while (QLayoutItem* item = layout->takeAt(0)) {
		if (deleteWidgets) {
			if (QWidget* widget = item->widget()) {
				widget->deleteLater();
			}
		}
		if (QLayout* childLayout = item->layout()) {
			clearLayout(childLayout, deleteWidgets);
		}
		delete item;
	}
}

#ifdef _WIN32
[[nodiscard]] QIcon getExecutableIcon(const QString& path) {
	HICON hIcon;
	if (SHDefExtractIconA(path.toLocal8Bit().constData(), 0, 0, &hIcon, nullptr, 16) != S_OK) {
		return QIcon{};
	}
	QIcon out{QPixmap::fromImage(QImage::fromHICON(hIcon))};
	DestroyIcon(hIcon);
	return out;
}
#endif

[[nodiscard]] QString getRootPath(bool usesLegacyBinDir) {
	QString rootPath = QCoreApplication::applicationDirPath();
	if (usesLegacyBinDir) {
		rootPath += "/..";
	} else {
		rootPath += "/../..";
	}
	if (auto cleanPath = QDir::cleanPath(rootPath); !cleanPath.isEmpty()) {
		return cleanPath;
	}
	return rootPath;
}

constexpr std::string_view STR_RECENT_CONFIGS = "str_recent_configs";
constexpr std::string_view STR_GAME_OVERRIDE = "str_game_override";

} // namespace

Window::Window(QWidget* parent)
		: QMainWindow(parent)
		, configUsingLegacyBinDir(false) {
	this->setWindowTitle(PROJECT_NAME.data());
	this->setMinimumHeight(400);

	// Default settings (recent configs are set later on)
	QSettings settings;
	if (!settings.contains(STR_GAME_OVERRIDE)) {
		settings.setValue(STR_GAME_OVERRIDE, QString(PROJECT_DEFAULT_MOD.data()));
	}

	// Icon
	this->setWindowIcon(QIcon{getSDKLauncherIconPath()});

	// Config menu
	auto* configMenu = this->menuBar()->addMenu(tr("Config"));

	this->config_loadDefault = configMenu->addAction("Load Default", Qt::CTRL | Qt::Key_R, [this] {
		this->loadGameConfig(QString(":/config/%1.json").arg(PROJECT_DEFAULT_MOD.data()));
	});

	configMenu->addSeparator();

	configMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("Load Custom..."), Qt::CTRL | Qt::Key_O, [this] {
		auto filename = QFileDialog::getOpenFileName(this, tr("Open Config"));
		if (!filename.isEmpty()) {
			this->loadGameConfig(filename);
		}
	});

	this->recent = configMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Load Recent..."));
	// Will be regenerated naturally later on

	// Game menu
	auto* gameMenu = this->menuBar()->addMenu(tr("Game"));

	this->game_resetToDefault = gameMenu->addAction(tr("Reset to Default"), [this] {
		QSettings settings;
		settings.setValue(STR_GAME_OVERRIDE, QString(PROJECT_DEFAULT_MOD.data()));
		this->game_overrideGame->setText(tr("Override \"%1\" Folder").arg(settings.value(STR_GAME_OVERRIDE).toString()));
		this->loadGameConfig(settings.value(STR_RECENT_CONFIGS).toStringList().first());
	});

	gameMenu->addSeparator();

	this->game_overrideGame = gameMenu->addAction(tr("Override \"%1\" Folder").arg(settings.value(STR_GAME_OVERRIDE).toString()), [this] {
		const auto rootPath = ::getRootPath(this->configUsingLegacyBinDir);
		if (auto path = QFileDialog::getExistingDirectory(this, tr("Override Game Folder"), rootPath); !path.isEmpty()) {
			const QDir rootDir{rootPath};
			QSettings settings;
			settings.setValue(STR_GAME_OVERRIDE, QDir::cleanPath(rootDir.relativeFilePath(path)));
			this->game_overrideGame->setText(tr("Override \"%1\" Folder").arg(settings.value(STR_GAME_OVERRIDE).toString()));
			this->loadGameConfig(settings.value(STR_RECENT_CONFIGS).toStringList().first());
		}
	});

	// Utilities menu
	auto* utilitiesMenu = this->menuBar()->addMenu(tr("Utilities"));

	this->utilities_createNewMod = utilitiesMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Create New Mod"), [this] {
		NewModDialog::open(::getRootPath(this->configUsingLegacyBinDir), this->configModTemplateURL, this);
	});

	this->utilities_createNewAddon = utilitiesMenu->addAction(this->style()->standardIcon(QStyle::SP_FileIcon), tr("Create New Addon"), [this] {
		QSettings settings;
		NewP2CEAddonDialog::open(::getRootPath(this->configUsingLegacyBinDir) + QDir::separator() + settings.value(STR_GAME_OVERRIDE, {PROJECT_DEFAULT_MOD.data()}).toString(), this);
	});

	// Help menu
	auto* helpMenu = this->menuBar()->addMenu(tr("Help"));

	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About"), Qt::Key_F1, [this] {
		QMessageBox about(this);
		about.setWindowTitle(tr("About"));
		about.setIconPixmap(QIcon{getStrataIconPath()}.pixmap(64, 64));
		about.setTextFormat(Qt::TextFormat::MarkdownText);
		about.setText(QString("## %1\n*Created by Strata Source Contributors*\n<br/>\n").arg(PROJECT_TITLE.data()));
		about.exec();
	});

	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About Qt"), Qt::ALT | Qt::Key_F1, [this] {
		QMessageBox::aboutQt(this);
	});

	// Add content
	auto* scrollArea = new QScrollArea;
	scrollArea->setFrameStyle(0);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAsNeeded);
	scrollArea->setWidgetResizable(true);

	this->main = new QWidget;
	scrollArea->setWidget(this->main);
	this->setCentralWidget(scrollArea);

	new QVBoxLayout(this->main);

	if (!settings.contains(STR_RECENT_CONFIGS)) {
		settings.setValue(STR_RECENT_CONFIGS, QStringList{});
		if (auto defaultConfigPath = QCoreApplication::applicationDirPath() + "/SDKLauncherDefault.json"; QFile::exists(defaultConfigPath)) {
			this->loadGameConfig(defaultConfigPath);
		} else {
			this->loadGameConfig(QString(":/config/%1.json").arg(PROJECT_DEFAULT_MOD.data()));
		}
	} else {
		this->loadGameConfig(settings.value(STR_RECENT_CONFIGS).value<QStringList>().first());
	}
}

QString Window::getStrataIconPath() {
	if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
		return ":/icons/strata_dark.png";
	}
	return ":/icons/strata_light.png";
}

QString Window::getSDKLauncherIconPath() {
	if constexpr (PROJECT_DEFAULT_MOD == "p2ce") {
		return ":/icons/p2ce_sdk.png";
	} else {
		return getStrataIconPath();
	}
}

void Window::loadGameConfig(const QString& path) {
	auto* layout = dynamic_cast<QVBoxLayout*>(this->main->layout());
	::clearLayout(layout);

	auto gameConfig = GameConfig::parse(path);
	if (!gameConfig) {
		auto* test = new QLabel(tr("Invalid game configuration."), this->main);
		layout->addWidget(test);
		layout->addStretch();
		return;
	}

	this->configUsingLegacyBinDir = gameConfig->getUsesLegacyBinDir();
	this->configModTemplateURL = gameConfig->getModTemplateURL();
	this->utilities_createNewMod->setDisabled(this->configModTemplateURL.isEmpty());
	this->utilities_createNewAddon->setDisabled(!gameConfig->supportsP2CEAddons());

	QSettings settings;
	auto recentConfigs = settings.value(STR_RECENT_CONFIGS).value<QStringList>();
	if (recentConfigs.contains(path)) {
		recentConfigs.removeAt(recentConfigs.indexOf(path));
	}
	recentConfigs.push_front(path);
	if (recentConfigs.size() > 10) {
		recentConfigs.pop_back();
	}
	settings.setValue(STR_RECENT_CONFIGS, recentConfigs);
	this->regenerateRecentConfigs();

	// Set ${ROOT}
	const auto rootPath = ::getRootPath(this->configUsingLegacyBinDir);
	gameConfig->setVariable("ROOT", rootPath);

	// Set ${PLATFORM}
#if defined(_WIN32)
	gameConfig->setVariable("PLATFORM", "win64");
#elif defined(__APPLE__)
	gameConfig->setVariable("PLATFORM", "osx64");
#elif defined(__linux__)
	gameConfig->setVariable("PLATFORM", "linux64");
#else
	#warning "Unknown platform! ${PLATFORM} will not be substituted!"
#endif

	// tiny hack: get default game icon before ${GAME} substitution
	QString defaultGameIconPath = gameConfig->getGameIcon();
	defaultGameIconPath.replace("${GAME}", PROJECT_DEFAULT_MOD.data());
	if (QIcon defaultGameIcon{defaultGameIconPath}; !defaultGameIcon.isNull() && !defaultGameIcon.availableSizes().isEmpty()) {
		this->config_loadDefault->setIcon(defaultGameIcon);
		this->game_resetToDefault->setIcon(defaultGameIcon);
	} else {
		this->config_loadDefault->setIcon(this->style()->standardIcon(QStyle::SP_FileLinkIcon));
		this->game_resetToDefault->setIcon(this->style()->standardIcon(QStyle::SP_FileLinkIcon));
	}

	// Set ${GAME}
	QString gameDir = settings.contains(STR_GAME_OVERRIDE) ? settings.value(STR_GAME_OVERRIDE).toString() : gameConfig->getGameDefault();
	gameConfig->setVariable("GAME", gameDir);

	// Set ${GAME_ICON}
	if (QIcon gameIcon{gameConfig->getGameIcon()}; !gameIcon.isNull() && !gameIcon.availableSizes().isEmpty()) {
		this->game_overrideGame->setIcon(gameIcon);
		gameConfig->setVariable("GAME_ICON", gameConfig->getGameIcon());
	} else {
		this->game_overrideGame->setIcon(this->style()->standardIcon(QStyle::SP_FileLinkIcon));
		gameConfig->setVariable("GAME_ICON", "");
	}

	// Set ${STRATA_ICON}
	gameConfig->setVariable("STRATA_ICON", getStrataIconPath());

	// Set ${SDKLAUNCHER_ICON}
	gameConfig->setVariable("SDKLAUNCHER_ICON", getSDKLauncherIconPath());

	for (int i = 0; i < gameConfig->getSections().size(); i++) {
		auto& section = gameConfig->getSections()[i];

		auto* name = new QLabel(section.name, this->main);
		name->setStyleSheet("QLabel { font-size: 11pt; }");
		layout->addWidget(name);

		auto* line = new QFrame(this->main);
		line->setFrameShape(QFrame::HLine);
		layout->addWidget(line);

		for (auto& entry : section.entries) {
			auto* button = new LaunchButton(this->main);
			button->setStyleSheet(
					"LaunchButton          { background-color: rgba(  0,   0, 0,  0); border: none; }\n"
					"LaunchButton::pressed { background-color: rgba(220, 220, 0, 32); border: none; }\n"
					"LaunchButton::hover   { background-color: rgba(220, 220, 0, 32); border: none; }");
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			button->setText(entry.name);
			button->setIconSize({16, 16});
			button->setFixedWidth(gameConfig->getWindowWidth() - 18);
			layout->addWidget(button);

			bool iconSet = false;
			if (!entry.iconOverride.isEmpty()) {
				button->setIcon(QIcon{entry.iconOverride});
				iconSet = true;
			}

			QString action = entry.action;
			if (action.endsWith('/') || action.endsWith('\\')) {
				action = action.sliced(0, action.size() - 1);
			}
			switch (entry.type) {
				case GameConfig::ActionType::INVALID:
					button->setIcon(this->style()->standardIcon(QStyle::SP_MessageBoxCritical));
					button->setToolTip(tr("This button has an invalid type. Check the config for any spelling errors."));
					break;
				case GameConfig::ActionType::COMMAND:
					if (!iconSet) {
#ifdef _WIN32
						if (auto icon = ::getExecutableIcon(action + ".exe"); !icon.isNull()) {
							button->setIcon(icon);
						} else {
							button->setIcon(this->style()->standardIcon(QStyle::SP_FileLinkIcon));
						}
#else
						button->setIcon(this->style()->standardIcon(QStyle::SP_FileLinkIcon));
#endif
					}
					button->setToolTip(action + " " + entry.arguments.join(" "));
					QObject::connect(button, &LaunchButton::doubleClicked, this, [this, action, args=entry.arguments, cwd=rootPath] {
						auto* process = new QProcess;
						QObject::connect(process, &QProcess::errorOccurred, this, [this, timeStart = std::chrono::steady_clock::now()](QProcess::ProcessError code) {
							QString error;
							switch (code) {
								using enum QProcess::ProcessError;
								case FailedToStart:
									error = tr("The process failed to start. Perhaps the executable it points to might not exist?");
									break;
								case Crashed: {
									auto timeEnd = std::chrono::steady_clock::now();
									if (std::chrono::duration<float, std::milli>(timeEnd - timeStart).count() > 30'000) {
										return;
									}
									error = tr("The process crashed.");
									break;
								}
								case Timedout:
									error = tr("The process timed out.");
									break;
								case ReadError:
								case WriteError:
									error = tr("The process hit an I/O error.");
									break;
								case UnknownError:
									error = tr("The process hit an unknown error.");
									break;
							}
							QMessageBox::critical(this, tr("Error"), tr("An error occurred executing this command: %1").arg(error));
						});
						process->setWorkingDirectory(cwd);
						process->start(action, args);
					});
					break;
				case GameConfig::ActionType::LINK:
					if (!iconSet) {
						button->setIcon(this->style()->standardIcon(QStyle::SP_MessageBoxInformation));
					}
					button->setToolTip(action);
					QObject::connect(button, &LaunchButton::doubleClicked, this, [action] {
						QDesktopServices::openUrl({action});
					});
					break;
				case GameConfig::ActionType::DIRECTORY:
					if (!iconSet) {
						button->setIcon(this->style()->standardIcon(QStyle::SP_DirLinkIcon));
					}
					button->setToolTip(action);
					QObject::connect(button, &LaunchButton::doubleClicked, this, [action] {
						QDesktopServices::openUrl({QString("file:///") + action});
					});
					break;
			}
		}

		if (i + 1 != gameConfig->getSections().size()) {
			layout->addSpacing(16);
		}
	}

	layout->addStretch();

	// Set window sizing
	this->resize(gameConfig->getWindowWidth(), gameConfig->getWindowHeight());
	this->setFixedWidth(gameConfig->getWindowWidth());
}

void Window::regenerateRecentConfigs() {
	this->recent->clear();

	auto paths = QSettings().value(STR_RECENT_CONFIGS).value<QStringList>();
	if (paths.empty()) {
		auto* noRecentFilesAction = this->recent->addAction(tr("No recent files."));
		noRecentFilesAction->setDisabled(true);
		return;
	}
	for (int i = 0; i < paths.size(); i++) {
		this->recent->addAction(("&%1: \"" + paths[i] + "\"").arg((i + 1) % 10), [this, path=paths[i]] {
			this->loadGameConfig(path);
		});
	}
	this->recent->addSeparator();
	this->recent->addAction(tr("Clear"), [this] {
		QSettings().setValue(STR_RECENT_CONFIGS, QStringList{});
		this->regenerateRecentConfigs();
	});
}
