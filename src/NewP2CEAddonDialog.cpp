#include "NewP2CEAddonDialog.h"

#include <filesystem>
#include <utility>

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
#include <QStandardPaths>
#include <QPlainTextEdit>

NewP2CEAddonDialog::NewP2CEAddonDialog(QString gameRoot_, QWidget* parent)
		: QDialog(parent)
		, gameRoot(std::move(gameRoot_)) {
	// Window setup
	this->setModal(true);
	this->setWindowTitle(tr("New Addon"));
	this->setMinimumWidth(350);

	// Create UI elements
	auto* layout = new QFormLayout{this};

	this->addonID = new QLineEdit{this};
	this->addonID->setPlaceholderText(tr("For example: p2ce, revolution, portal2"));
	layout->addRow(tr("Addon ID"), this->addonID);

	this->addonName = new QLineEdit{this};
	this->addonName->setPlaceholderText(tr("My Cool Addon"));
	layout->addRow(tr("Addon Name"), this->addonName);

	this->addonDesc = new QPlainTextEdit{this};
	this->addonDesc->setPlaceholderText(tr("A description of the addon."));
	layout->addRow(tr("Addon Description"), this->addonDesc);

	this->addonType = new QComboBox{this};
	this->addonType->addItem("Campaign");
	this->addonType->addItem("Map");
	this->addonType->addItem("Asset Pack");
	this->addonType->addItem("Other");
	layout->addRow(tr("Addon Type"), this->addonType);

	this->addShortcutOnDesktop = new QCheckBox{this};
	this->addShortcutOnDesktop->setCheckState(Qt::Unchecked);
	layout->addRow(tr("Create Desktop Shortcut"), this->addShortcutOnDesktop);

	auto* buttonBox = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this};
	layout->addWidget(buttonBox);

	// Connect ok/cancel buttons to download stuff
	QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
		// Validate mod ID
		if (this->addonID->text().trimmed().isEmpty()) {
			QMessageBox::critical(this, tr("Incorrect Input"), tr("Addon ID must not be empty."));
			return;
		}

		const auto addonInstallDir = this->getAddonInstallDir();

		// Validate install location
		if (std::filesystem::exists(addonInstallDir.toLocal8Bit().constData())) {
			QMessageBox::critical(this, tr("Incorrect Input"), tr("A folder with the name of the addon ID already exists at this install location."));
			return;
		}

		// Create directory structure
		QDir dir{this->getAddonInstallDir()};
		dir.mkpath("maps");
		dir.mkpath("materials");
		dir.mkpath("models");
		dir.mkpath("particles");
		dir.mkpath("resource");
		dir.mkpath("scenes");
		dir.mkpath("scripts");
		dir.mkpath("sound");

		// Create addon.kv3
		QFile addonKV3{this->getAddonInstallDir() + QDir::separator() + "addon.kv3"};
		addonKV3.open(QIODevice::WriteOnly | QIODevice::Text);

		static constexpr auto p2ceAddonKV3ContentsBase = R"({
	mod = "%1"
	description = """
%2
"""
	type = "%3"
	id = 0
	thumbnail = ""
	authors = [  ]
	dependencies = [  ]
	tags = [  ]
	ignore = [  ]
	metadata =
	{
	}
	assets =
	{
		logo = ""
		cover = ""
	}
})";
		{
			QTextStream out{&addonKV3};
			out << QString{p2ceAddonKV3ContentsBase}.arg(this->addonName->text(), this->addonDesc->toPlainText(), this->addonType->currentText()).toLocal8Bit();
		}
		addonKV3.close();

		// Create desktop shortcut
		if (this->addShortcutOnDesktop->isChecked()) {
			const auto shortcutPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + QDir::separator() + this->addonID->text().trimmed();
#ifdef _WIN32
			QFile::link(addonInstallDir, shortcutPath + ".lnk");
#else
			QFile::link(addonInstallDir, shortcutPath);
#endif
		}
		QDesktopServices::openUrl({QString("file:///") + addonInstallDir});

		this->accept();
	});
	QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &NewP2CEAddonDialog::reject);
}

QString NewP2CEAddonDialog::getAddonInstallDir() const {
	return this->gameRoot + QDir::separator() + "addons" + QDir::separator() + this->addonID->text().trimmed();
}

void NewP2CEAddonDialog::open(QString gameRoot, QWidget* parent) {
	auto* dialog = new NewP2CEAddonDialog{std::move(gameRoot), parent};
	dialog->exec();
	dialog->deleteLater();
}
