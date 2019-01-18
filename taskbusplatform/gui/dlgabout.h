#ifndef DLGABOUT_H
#define DLGABOUT_H

#include <QDialog>

namespace Ui {
	class DlgAbout;
}

class DlgAbout : public QDialog
{
	Q_OBJECT

public:
	explicit DlgAbout(QWidget *parent = nullptr);
	~DlgAbout();

private slots:
	void on_pushButton_mainPage_clicked();
	void on_pushButton_documents_clicked();
	void on_pushButton_Next_clicked();
	void on_pushButton_Close_clicked();

private:
	Ui::DlgAbout *ui;
};

#endif // DLGABOUT_H
