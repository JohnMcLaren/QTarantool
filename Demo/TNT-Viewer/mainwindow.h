#pragma once
/******************************************************************
 * TNT-Viewer Qt-client for Tarantool database.
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.11.2022 🄯 JML
******************************************************************/
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QStyleFactory>
#include <QLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QPlainTextEdit>
#include <QDebug>
#include "forms.h"
#include "../lib/QVariantModelWidget/qvarianttreewidget.h"
#include "../lib/QVariantModelWidget/qvarianttablewidget.h"
#include "../../src/qtarantool.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	//~MainWindow();

private:
	QTNT::QTarantool tnt;
	// Window widgets
	QWidget *centralWidget;
	QStatusBar *statusBar;
	QGridLayout *layMain;
	QSplitter *splitLeft, *splitRight, *splitCenter;
	QGroupBox boxSummary {"Summary"}, boxServerInfo {"Server Info"}, boxEdit {"Editor"}, boxControl {"Control"}, boxData {"Data"};
	// Tree / Table widgets
	QVariantTreeWidget *treeSummary;
	QVariantTreeWidget *treeServerInfo;
	QVariantTableWidget *tableDetail;
	QPlainTextEdit *txtEditor;
	// Forms
	FormGetData *frmGetSpaceData;
	FormSetData *frmSetSpaceData;
	// Buttons
	QPushButton cmdConnect {"Connect"};
	QPushButton cmdPing {"Ping"};
	QPushButton cmdTest {"test"};
	// Data objects
	QVariant mapSummary;
	// ...
	QString CurrentSpaceName;
	bool IsExitState =false;

	void makeMainWindow();
	void setTheme();
	void updateSummary();
	void updateServerInfo();

private slots:
	void connectToServer(const QString &host ="", const QString &user ="", const QString &password ="");
	void on_Connected(const bool bConnected);
	void reset() {
		CurrentSpaceName ="";
	}

	void on_TreeSummaryClicked(const QModelIndex &index);
	void on_TableDataWillBeChanged(const QVariantModelIndex &index, const QVariant &data, OUT bool &AllowDataChanges);
	void on_GetClicked(const QString &indexName, const int iOperatorFrom, const QString &keyFrom, const int iOperatorTo, const QString &keyTo);
	void on_SetClicked(const QString &tuple);
	void on_PingClicked();
	void on_TestClicked();
	void on_TarantoolError(const QTNT::ERROR &error) {

		statusBar->showMessage(QString("Tarantool error: [%1] %2").arg(error.code).arg(error.text), 20000);
		statusBar->setStyleSheet("color: red");
	}

protected:
	void closeEvent(QCloseEvent *) { IsExitState =true; }
};

