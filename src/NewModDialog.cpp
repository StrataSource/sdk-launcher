#include "NewModDialog.h"

#include <filesystem>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>

#ifdef _WIN32
	#include <memory>
	#include <Windows.h>
#else
	#include <cstdlib>
#endif

namespace {

/// Copied from sourcepp
QString getDefaultModParentFolder() {
	std::filesystem::path steamLocation;
	std::error_code ec;

#ifdef _WIN32
	{
		// 16383 being the maximum length of a path
		static constexpr DWORD STEAM_LOCATION_MAX_SIZE = 16383;
		std::unique_ptr<char[]> steamLocationData{new char[STEAM_LOCATION_MAX_SIZE]};

		HKEY steam;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Valve\Steam)", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &steam) != ERROR_SUCCESS) {
			return;
		}

		DWORD steamLocationSize = STEAM_LOCATION_MAX_SIZE;
		if (RegQueryValueExA(steam, "InstallPath", nullptr, nullptr, reinterpret_cast<LPBYTE>(steamLocationData.get()), &steamLocationSize) != ERROR_SUCCESS) {
			return;
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

// https://github.com/StrataSource/p2ce-mod-template/archive/refs/heads/main.zip

NewModDialog::NewModDialog(QNetworkAccessManager* network, QWidget* parent)
		: QDialog(parent) {
	this->setModal(true);
	this->setWindowTitle(tr("New Mod"));
	this->setMinimumWidth(400);

	auto* layout = new QFormLayout{this};

	auto* parentFolderLabel = new QLabel{tr("Parent Folder"), this};
	this->parentFolder = new QLineEdit{this};
	this->parentFolder->setText(::getDefaultModParentFolder());
	this->parentFolder->setPlaceholderText(tr("path/to/mod/parent/folder/"));
	layout->addRow(parentFolderLabel, this->parentFolder);

	auto* modIDLabel = new QLabel{tr("Mod ID"), this};
	this->modID = new QLineEdit{this};
	this->modID->setPlaceholderText("p2ce");
	layout->addRow(modIDLabel, this->modID);

	auto* buttonBox = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this};
	layout->addWidget(buttonBox);

	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
		this->accept();
	});
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &NewModDialog::reject);
}

void NewModDialog::open(QNetworkAccessManager* network, QWidget* parent) {
	auto* dialog = new NewModDialog{network, parent};
	dialog->exec();
	dialog->deleteLater();
}
