#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;

class NewP2CEAddonDialog : public QDialog {
	Q_OBJECT;

public:
	explicit NewP2CEAddonDialog(QString gameRoot_, QWidget* parent = nullptr);

	[[nodiscard]] QString getAddonInstallDir() const;

	static void open(QString gameRoot, QWidget* parent = nullptr);

private:
	QString gameRoot;

	QLineEdit* addonID;
	QLineEdit* addonName;
	QPlainTextEdit* addonDesc;
	QComboBox* addonType;
	QCheckBox* addShortcutOnDesktop;
};
