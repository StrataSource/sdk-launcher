#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QNetworkAccessManager;
class QProgressBar;

class NewModDialog : public QDialog {
	Q_OBJECT;

public:
	NewModDialog(QString gameRoot_, QString downloadURL_, QWidget* parent = nullptr);

	[[nodiscard]] QString getModInstallDirParent() const;

	[[nodiscard]] QString getModInstallDir() const;

	static void open(QString gameRoot, QString downloadURL, QWidget* parent = nullptr);

public Q_SLOTS:
	void reject() override;

private:
	QString gameRoot;
	QString downloadURL;

	QComboBox* parentFolder;
	QLineEdit* parentFolderCustom;
	QLineEdit* modID;
	QCheckBox* addShortcutOnDesktop;
	QProgressBar* downloadProgress;

	QNetworkAccessManager* network = nullptr;
};
