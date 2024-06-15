#include "Window.h"

#include <filesystem>

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QScrollArea>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QToolButton>
#include <QVBoxLayout>

#include "Config.h"
#include "GameConfig.h"
#include "GameFinder.h"

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
QIcon getExecutableIcon(const QString& path) {
	HICON hIcon;
	if (SHDefExtractIconA((LPCSTR) path.toLocal8Bit().constData(), 0, 0, &hIcon, nullptr, 16) != S_OK) {
		return QIcon{};
	}
	QIcon out{QPixmap::fromImage(QImage::fromHICON(hIcon))};
	DestroyIcon(hIcon);
	return out;
}
#endif

constexpr int FIXED_WINDOW_WIDTH = 256;

constexpr std::string_view STR_RECENT_CONFIGS = "str_recent_configs";

} // namespace

Window::Window(QWidget* parent)
		: QMainWindow(parent) {
	this->setWindowTitle(PROJECT_NAME.data());
	this->setFixedSize(FIXED_WINDOW_WIDTH, 450);

	// Profile menu
	auto* configMenu = this->menuBar()->addMenu(tr("Config"));

	auto* gameConfigMenu = configMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogListView), tr("Open Game Config"));
	if (!GameFinder::getGameInstallPath(440000).isEmpty()) {
		gameConfigMenu->addAction(GameFinder::getGameIcon(440000), "Portal 2: Community Edition", [this] {
			this->loadGameConfig(":/config/440000.json");
		});
	}
	if (!GameFinder::getGameInstallPath(601360).isEmpty()) {
		gameConfigMenu->addAction(GameFinder::getGameIcon(601360), "Portal: Revolution", [this] {
			this->loadGameConfig(":/config/601360.json");
		});
	}
	if (!GameFinder::getGameInstallPath(1802710).isEmpty()) {
		gameConfigMenu->addAction(GameFinder::getGameIcon(1802710), "Momentum Mod", [this] {
			this->loadGameConfig(":/config/1802710.json");
		});
	}

	configMenu->addSeparator();

	configMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("Open Custom Config"), Qt::CTRL | Qt::Key_O, [this] {
		auto filename = QFileDialog::getOpenFileName(this, tr("Open Config"));
		if (!filename.isEmpty()) {
			this->loadGameConfig(filename);
		}
	});

	this->recent = configMenu->addMenu(this->style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Open Recent Config"));
	// Will be regenerated naturally later on

	// Help menu
	auto* helpMenu = this->menuBar()->addMenu(tr("Help"));

	helpMenu->addAction(this->style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About"), Qt::Key_F1, [this] {
		QMessageBox about(this);
		about.setWindowTitle(tr("About"));
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
	scrollArea->setWidget(main);
	this->setCentralWidget(scrollArea);

	new QVBoxLayout(main);

	if (QSettings settings; !settings.contains(STR_RECENT_CONFIGS)) {
		settings.setValue(STR_RECENT_CONFIGS, QStringList{});
		auto appId = GameFinder::getDefaultGameAppId();
		this->loadGameConfig(QString(":/config/%1.json").arg(appId));
	} else {
		this->loadGameConfig(settings.value(STR_RECENT_CONFIGS).value<QStringList>().first());
	}
}

void Window::loadGameConfig(const QString& path) {
	auto* layout = dynamic_cast<QVBoxLayout*>(this->main->layout());
	::clearLayout(layout);

	auto gameConfig = GameConfig::parse(path);
	if (!gameConfig) {
		auto* test = new QLabel(tr("Invalid game configuration."), main);
		layout->addWidget(test);
		layout->addStretch();
		return;
	}

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

	if (auto appId = gameConfig->getAppId()) {
		this->setWindowIcon(GameFinder::getGameIcon(appId));
	}

	for (int i = 0; i < gameConfig->getSections().size(); i++) {
		auto& section = gameConfig->getSections()[i];

		auto* name = new QLabel(section.name, main);
		name->setStyleSheet("QLabel { font-size: 11pt; }");
		layout->addWidget(name);

		auto* line = new QFrame(main);
		line->setFrameShape(QFrame::HLine);
		layout->addWidget(line);

		for (const auto& entry : section.entries) {
			auto* button = new QToolButton(main);
			button->setStyleSheet(
					"QToolButton          { background-color: rgba(  0,   0, 0,  0); border: none; }\n"
					"QToolButton::pressed { background-color: rgba(220, 220, 0, 32); border: none; }\n"
					"QToolButton::hover   { background-color: rgba(220, 220, 0, 32); border: none; }");
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			button->setText(entry.name);
			button->setIconSize({16, 16});
			button->setFixedWidth(FIXED_WINDOW_WIDTH - 18);
			layout->addWidget(button);

			bool iconSet = false;
			if (!entry.iconOverride.isEmpty()) {
				if (entry.iconOverride == "${GAME_ICON}" && gameConfig->getAppId()) {
					if (auto icon = GameFinder::getGameIcon(gameConfig->getAppId()); !icon.isNull()) {
						button->setIcon(icon);
					} else {
						button->setIcon(this->style()->standardIcon(QStyle::SP_FileLinkIcon));
					}
				} else if (entry.iconOverride == "${STRATA_ICON}") {
					if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
						button->setIcon(QIcon{":/icons/strata_dark.png"});
					} else {
						button->setIcon(QIcon{":/icons/strata_light.png"});
					}
				} else {
					button->setIcon(QIcon{entry.iconOverride});
				}
				iconSet = true;
			}

			QString action = entry.action;
			action.replace("${ROOT}", gameConfig->getRoot());
#if defined(_WIN32)
			action.replace("${PLATFORM}", "win64");
#elif defined(__linux__)
			action.replace("${PLATFORM}", "linux64");
#else
			#warning "Unknown platform! ${PLATFORM} will not be substituted!"
#endif
			if (action.endsWith('/') || action.endsWith('\\')) {
				action = action.sliced(0, action.size() - 1);
			}
			switch (entry.type) {
				case GameConfig::ActionType::INVALID:
					button->setIcon(this->style()->standardIcon(QStyle::SP_MessageBoxCritical));
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
					QObject::connect(button, &QToolButton::clicked, this, [action, args=entry.arguments, cwd=gameConfig->getRoot()] {
						QProcess::startDetached(action, args, cwd);
					});
					break;
				case GameConfig::ActionType::LINK:
					if (!iconSet) {
						button->setIcon(this->style()->standardIcon(QStyle::SP_MessageBoxInformation));
					}
					QObject::connect(button, &QToolButton::clicked, this, [action] {
						QDesktopServices::openUrl({action});
					});
					break;
				case GameConfig::ActionType::DIRECTORY:
					if (!iconSet) {
						button->setIcon(this->style()->standardIcon(QStyle::SP_DirLinkIcon));
					}
					QObject::connect(button, &QToolButton::clicked, this, [action] {
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
