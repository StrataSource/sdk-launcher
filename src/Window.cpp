#include "Window.h"

#include <QApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QScrollArea>
#include <QStyle>
#include <QStyleHints>
#include <QToolButton>
#include <QVBoxLayout>

#include "Config.h"
#include "GameConfig.h"

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
	if (SHDefExtractIconA(path.toLocal8Bit().constData(), 0, 0, &hIcon, nullptr, 16) != S_OK) {
		return QIcon{};
	}
	QIcon out{QPixmap::fromImage(QImage::fromHICON(hIcon))};
	DestroyIcon(hIcon);
	return out;
}
#endif

constexpr int FIXED_WINDOW_WIDTH = 256;

} // namespace

Window::Window(QWidget* parent)
		: QMainWindow(parent) {
	this->setWindowTitle(PROJECT_NAME.data());
	this->setFixedSize(FIXED_WINDOW_WIDTH, 450);

	// Icon
	if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
		this->setWindowIcon(QIcon{":/icons/strata_dark.png"});
	} else {
		this->setWindowIcon(QIcon{":/icons/strata_light.png"});
	}

	// Profile menu
	auto* configMenu = this->menuBar()->addMenu(tr("Config"));

	configMenu->addAction("Load Default", [this] {
		this->loadGameConfig(QString(":/config/%1.json").arg(PROJECT_DEFAULT_MOD.data()));
	});

	configMenu->addSeparator();

	configMenu->addAction(this->style()->standardIcon(QStyle::SP_DirIcon), tr("Load Custom"), Qt::CTRL | Qt::Key_O, [this] {
		auto filename = QFileDialog::getOpenFileName(this, tr("Open Config"));
		if (!filename.isEmpty()) {
			this->loadGameConfig(filename);
		}
	});

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
	scrollArea->setWidget(this->main);
	this->setCentralWidget(scrollArea);

	new QVBoxLayout(this->main);

	if (auto defaultConfigPath = QCoreApplication::applicationDirPath() + "/SDKLauncherDefault.json"; QFile::exists(defaultConfigPath)) {
		this->loadGameConfig(defaultConfigPath);
	} else {
		this->loadGameConfig(QString(":/config/%1.json").arg(PROJECT_DEFAULT_MOD.data()));
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

	QString rootPath = QCoreApplication::applicationDirPath();
	if (gameConfig->getUsesLegacyBinDir()) {
		rootPath += "/..";
	} else {
		rootPath += "/../..";
	}
	QIcon gameIcon{QString("%1/%2/resource/%3").arg(rootPath, gameConfig->getGame(), gameConfig->getGameIcon())};

	for (int i = 0; i < gameConfig->getSections().size(); i++) {
		auto& section = gameConfig->getSections()[i];

		auto* name = new QLabel(section.name, this->main);
		name->setStyleSheet("QLabel { font-size: 11pt; }");
		layout->addWidget(name);

		auto* line = new QFrame(this->main);
		line->setFrameShape(QFrame::HLine);
		layout->addWidget(line);

		for (const auto& entry : section.entries) {
			auto* button = new QToolButton(this->main);
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
				if (entry.iconOverride == "${GAME_ICON}") {
					if (!gameIcon.isNull() && !gameIcon.availableSizes().isEmpty()) {
						button->setIcon(gameIcon);
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
			action.replace("${ROOT}", rootPath);
#if defined(_WIN32)
			action.replace("${PLATFORM}", "win64");
#elif defined(__APPLE__)
			action.replace("${PLATFORM}", "osx64");
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
					QObject::connect(button, &QToolButton::clicked, this, [this, action, args=entry.arguments, cwd=rootPath] {
						auto* process = new QProcess;
						QObject::connect(process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError code) {
							QString error;
							switch (code) {
								using enum QProcess::ProcessError;
								case FailedToStart:
									error = tr("The process failed to start. Perhaps the executable it points to might not exist?");
									break;
								case Crashed:
									error = tr("The process crashed.");
									break;
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
