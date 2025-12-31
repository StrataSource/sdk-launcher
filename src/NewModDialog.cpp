#include "NewModDialog.h"

#include <cstring>
#include <filesystem>
#include <utility>

#include <miniz.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QProgressDialog>
#include <QStandardPaths>

#include "Steam.h"

namespace {

[[nodiscard]] QString join(const QStringList& list, const QString& separator) {
	if (list.isEmpty()) {
		return "";
	}
	QString result = list.first();
	for (int i = 1; i < list.size(); i++) {
		result += separator + list[i];
	}
	return result;
}

[[nodiscard]] bool writeDataToFile(const QByteArray& data, const QString& path) {
	std::filesystem::create_directories(std::filesystem::path{path.toLocal8Bit().constData()}.parent_path());

	QFile file{path};
	if (!file.open(QIODevice::WriteOnly)) {
		return false;
	}
	file.write(data);
	file.close();
	return true;
}

[[nodiscard]] bool extractZIP(const QByteArray& zip, const QString& outputDir, QWidget* parent) {
	mz_zip_archive zipArchive{};

	if (!mz_zip_reader_init_mem(&zipArchive, zip.data(), zip.size(), 0)) {
		return false;
	}

	// Collect file data
	QMap<int, mz_zip_archive_file_stat> files;
	QStringList filePaths;
	const unsigned int fileCount = mz_zip_reader_get_num_files(&zipArchive);
	for (int i = 0; std::cmp_less(i, fileCount); i++) {
		if (mz_zip_reader_is_file_a_directory(&zipArchive, i)) {
			continue;
		}

		mz_zip_archive_file_stat fileStat;
		if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
			return false;
		}

		files[i] = fileStat;

		filePaths.push_back(fileStat.m_filename);
		filePaths.back().replace('\\', '/');
	}

	// Find root dir(s) using probably the slowest algorithm ever
	QList<QStringList> pathSplits;
	for (const auto& path : filePaths) {
		pathSplits.push_back(path.split('/'));
	}
	QStringList rootDirList;
	while (true) {
		bool allTheSame = true;
		QString first = pathSplits[0][0];
		for (const auto& path : pathSplits) {
			if (path.length() == 1) {
				allTheSame = false;
				break;
			}
			if (path[0] != first) {
				allTheSame = false;
				break;
			}
		}
		if (!allTheSame) {
			break;
		}
		rootDirList.push_back(std::move(first));
		for (auto& path : pathSplits) {
			path.pop_front();
		}
	}
	const qsizetype rootDirLen = ::join(rootDirList, "/").length();

	// Write file without root dir(s)
	QProgressDialog progressDialog{QObject::tr("Extracting zip..."), QObject::tr("Cancel"), 0, static_cast<int>(files.size()), parent};
	progressDialog.setWindowModality(Qt::WindowModal);
	for (auto file = files.cbegin(); file != files.cend(); ++file) {
		if (progressDialog.wasCanceled()) {
			return false;
		}

		QByteArray fileData;
		fileData.resize(static_cast<qsizetype>(file.value().m_uncomp_size));
		if (!mz_zip_reader_extract_to_mem(&zipArchive, file.key(), fileData.data(), fileData.size(), 0)) {
			return false;
		}

		if (!::writeDataToFile(fileData, outputDir + QDir::separator() + QString{file.value().m_filename}.sliced(rootDirLen))) {
			return false;
		}

		progressDialog.setValue(progressDialog.value() + 1);
	}
	progressDialog.setValue(progressDialog.maximum());

	mz_zip_reader_end(&zipArchive);
	return true;
}

} // namespace

NewModDialog::NewModDialog(QString gameRoot_, QString downloadURL_, QWidget* parent)
		: QDialog(parent)
		, gameRoot(std::move(gameRoot_))
		, downloadURL(std::move(downloadURL_)) {
	// Check for sourcemods
	const bool knowsSourcemodsDirLocation = !::getSourceModsDir().isEmpty();

	// Window setup
	this->setModal(true);
	this->setWindowTitle(tr("New Mod"));
	this->setMinimumWidth(350);

	// Create UI elements
	auto* layout = new QFormLayout{this};

	this->parentFolder = new QComboBox{this};
	if (knowsSourcemodsDirLocation) {
		this->parentFolder->addItem(tr("Steam's SourceMods Folder"));
	}
	this->parentFolder->addItem(tr("Game Folder"));
	this->parentFolder->addItem(tr("Custom Location"));
	layout->addRow(tr("Install Location"), this->parentFolder);

	auto* parentFolderCustomLabel = new QLabel{tr("Custom Location"), this};
	this->parentFolderCustom = new QLineEdit{this};
	this->parentFolderCustom->setPlaceholderText(tr("path/to/mod/parent/folder"));
	layout->addRow(parentFolderCustomLabel, this->parentFolderCustom);

	this->modID = new QLineEdit{this};
	this->modID->setPlaceholderText(tr("For example: p2ce, revolution, portal2"));
	layout->addRow(tr("Mod ID"), this->modID);

	this->addShortcutOnDesktop = new QCheckBox{this};
	this->addShortcutOnDesktop->setCheckState(Qt::Checked);
	layout->addRow(tr("Create Desktop Shortcut"), this->addShortcutOnDesktop);

	auto* buttonBox = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this};
	layout->addWidget(buttonBox);

	this->downloadProgress = new QProgressBar{this};
	this->downloadProgress->setFormat(tr("%vkb / %mkb"));
	layout->addWidget(downloadProgress);

	// Create network
	this->network = new QNetworkAccessManager{this};

	// We want the custom input to be invisible unless the combo box is on the custom option
	parentFolderCustomLabel->hide();
	this->parentFolderCustom->hide();
	QObject::connect(this->parentFolder, &QComboBox::currentIndexChanged, this, [this, knowsSourcemodsDirLocation, parentFolderCustomLabel](int index) {
		const int customIndex = knowsSourcemodsDirLocation ? 2 : 1;
		parentFolderCustomLabel->setVisible(index == customIndex);
		this->parentFolderCustom->setVisible(index == customIndex);
	});

	// Connect ok/cancel buttons to download stuff
	buttonBox->show();
	this->downloadProgress->hide();
	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this, buttonBox] {
		// Validate mod ID
		if (this->modID->text().trimmed().isEmpty()) {
			QMessageBox::critical(this, tr("Incorrect Input"), tr("Mod ID must not be empty."));
			return;
		}

		const auto modInstallDir = this->getModInstallDir();

		// Validate install location
		if (modInstallDir.isEmpty() || !std::filesystem::exists(this->getModInstallDirParent().toLocal8Bit().constData())) {
			QMessageBox::critical(this, tr("Incorrect Input"), tr("Install location does not exist."));
			return;
		}
		if (std::filesystem::exists(modInstallDir.toLocal8Bit().constData())) {
			QMessageBox::critical(this, tr("Incorrect Input"), tr("A folder with the name of the mod ID already exists at this install location."));
			return;
		}

		// Initiate the download
		auto* reply = this->network->get(QNetworkRequest{QUrl(this->downloadURL)});

		// Connect download progress to progress bar, measured in kb
		QObject::connect(reply, &QNetworkReply::downloadProgress, this, [this, buttonBox](qint64 recv, qint64 total) {
			buttonBox->hide();
			this->downloadProgress->show();
			if (total < 0) {
				this->downloadProgress->setRange(0, 0);
				this->downloadProgress->setTextVisible(false);
			} else {
				this->downloadProgress->setRange(0, static_cast<int>(total / 1000));
				this->downloadProgress->setValue(static_cast<int>(recv / 1000));
				this->downloadProgress->setTextVisible(true);
			}
		});

		// Connect finished downloading response to the rest of the processing code
		QObject::connect(reply, &QNetworkReply::finished, this, [this, modInstallDir, reply] {
			// Check for a download error
			if (reply->error() != QNetworkReply::NoError) {
				QMessageBox::critical(this, tr("Error"), tr("An error occurred while downloading the mod template: %1").arg(reply->errorString()));
				this->accept();
				return;
			}

			// Extract zip contents in memory to destination
			if (!::extractZIP(reply->readAll(), modInstallDir, this)) {
				QDir{modInstallDir}.removeRecursively();
				QMessageBox::critical(this, tr("Error"), tr("An error occurred while extracting the mod template."));
				this->accept();
				return;
			}

			// Create desktop shortcut
			if (this->addShortcutOnDesktop->isChecked()) {
				const auto shortcutPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + QDir::separator() + this->modID->text().trimmed();
#ifdef _WIN32
				QFile::link(modInstallDir, shortcutPath + ".lnk");
#else
				QFile::link(modInstallDir, shortcutPath);
#endif
			}

			// If installing to sourcemods, tell user they will need to restart steam
			if (this->parentFolder->count() == 3 && this->parentFolder->currentIndex() == 0) {
				QMessageBox::information(this, tr("Info"), tr("Your mod has been installed to Steam's SourceMods folder, which means it will show up in your Steam library! This requires you to restart Steam once."));
				QDesktopServices::openUrl({QString("file:///") + modInstallDir});
			}

			this->accept();
		});
	});
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &NewModDialog::reject);
}

QString NewModDialog::getModInstallDirParent() const {
	auto selectedIndex = this->parentFolder->currentIndex();
	if (this->parentFolder->count() == 2) {
		selectedIndex++;
	}
	switch (selectedIndex) {
		case 0:
			return ::getSourceModsDir();
		case 1:
			return this->gameRoot;
		default:
		case 2:
			return this->parentFolderCustom->text();
	}
}

QString NewModDialog::getModInstallDir() const {
	return this->getModInstallDirParent() + QDir::separator() + this->modID->text().trimmed();
}

void NewModDialog::open(QString gameRoot, QString downloadURL, QWidget* parent) {
	auto* dialog = new NewModDialog{std::move(gameRoot), std::move(downloadURL), parent};
	dialog->exec();
	dialog->deleteLater();
}

void NewModDialog::reject() {
	if (!this->downloadProgress->isVisible()) {
		QDialog::reject();
	}
}
