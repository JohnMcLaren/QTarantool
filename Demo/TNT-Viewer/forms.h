#pragma once
/******************************************************************
 * This file is part of the project TNT-Viewer
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.11.2022 🄯 JML
******************************************************************/
#include <QLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDebug>

/****************************************************************************************
 * Forms
****************************************************************************************/
class FormBase : public QFrame
{
	Q_OBJECT

public:
	FormBase(const QString &caption ="", QWidget *parent =nullptr);
	~FormBase() {
		delete layForm;
		delete lblCaption;
	}

protected:
	QLabel *lblCaption;
	QGridLayout *layForm;
};
//------------------------------------------------------------------------------
class FormGetData : public FormBase
{
	Q_OBJECT

public:
	FormGetData(const QString &caption ="", QWidget *parent =nullptr);

public slots:
	inline void
	setIndexNames(const QStringList &indexNames)
	{
		lstIndexName.clear();
		lstIndexName.addItems(indexNames);
		lstIndexName.updateGeometry();
	}
	inline void
	clickGet() { cmdGet.clicked(); };

private:
	QComboBox lstIndexName;
	QComboBox lstOperatorFrom;
	QComboBox lstOperatorTo;
	QLineEdit txtFrom;
	QLineEdit txtTo;

	QPushButton cmdGet {"Get"};

signals:
	void clickedGet(const QString &indexName, const int iOperatorFrom, const QString &keyFrom, const int iOperatorTo, const QString &keyTo);
};
//------------------------------------------------------------------------------
class FormSetData : public FormBase
{
	Q_OBJECT

public:
	FormSetData(const QString &caption ="", QWidget *parent =nullptr);

public slots:
	inline void
	setTuple(const QString &tuple) { txtTuple.setText(tuple); }

private:
	QLineEdit txtTuple;

	QPushButton cmdSet {"Set"};

signals:
	void clickedSet(const QString &tuple);
};
/****************************************************************************************
 * Dialogs
****************************************************************************************/
class FormNetLogin : public QDialog
{
	Q_OBJECT

public:
	FormNetLogin(QWidget *parent =nullptr);

	QString Host ="http://localhost:3301";
	QString User ="guest";
	QString Password;

public slots:
	int exec();
	int exec(const QString &host, const QString &user, const QString &password);
};

