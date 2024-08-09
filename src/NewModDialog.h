#pragma once

#include <QDialog>
#include <QNetworkReply>

class QLineEdit;
class QNetworkAccessManager;

class NewModDialog : public QDialog {
	Q_OBJECT;

public:
	explicit NewModDialog(QNetworkAccessManager* network, QWidget* parent = nullptr);

	static void open(QNetworkAccessManager* network, QWidget* parent = nullptr);

private:
	QLineEdit* parentFolder;
	QLineEdit* modID;
};
