/******************************************************************
 * This file is part of the project TNT-Viewer
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.11.2022 🄯 JML
******************************************************************/
#include "forms.h"
/****************************************************************************************
 *										Forms
****************************************************************************************/
FormBase::FormBase(const QString &caption, QWidget *parent) :
	QFrame(parent)
{
	setFrameStyle(QFrame::Box);
	setStyleSheet("QFrame FormBase {border: 1px solid grey; margin-top: 5px;}");

	lblCaption = new QLabel(tr("%1").arg(caption), this);
	lblCaption->setStyleSheet("border: none; margin-top: 0px; font-size: 10pt;");
	lblCaption->setGeometry(0, -3, lblCaption->width(), lblCaption->height()/2);

	layForm = new QGridLayout;
	layForm->setContentsMargins(2,10,2,3);
	setLayout(layForm);
}
//---------------------------------------------------------------------------------------
FormGetData::FormGetData(const QString &caption, QWidget *parent) :
	FormBase(caption, parent)
{
	layForm->addWidget(new QLabel("Index name:"), 1,0,1,1, Qt::AlignRight);
	layForm->addWidget(new QLabel("From:"), 2,0,1,1, Qt::AlignRight);
	layForm->addWidget(new QLabel("To:"), 3,0,1,1, Qt::AlignRight);
	layForm->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Fixed), 4, 0, 1, 10);

	lstIndexName.setMinimumHeight(18);
	layForm->addWidget(&lstIndexName, 1, 1, 1, 1);
	lstOperatorFrom.addItems({"ALL", "==", ">=", ">", "<=", "<", "REQ"});
	lstOperatorFrom.setMinimumHeight(18);
	layForm->addWidget(&lstOperatorFrom, 2, 1, 1, 1);
	lstOperatorTo.addItems({"ALL", "==", ">=", ">", "<=", "<", "REQ"});
	lstOperatorTo.setMinimumHeight(18);
	layForm->addWidget(&lstOperatorTo, 3, 1, 1, 1);

	txtFrom.setMinimumHeight(18);
	txtFrom.setPlaceholderText("{search key from}");
	txtFrom.setStyleSheet("font-family: 'arial'; font-size: 10pt; qproperty-alignment:AlignHCenter;");
	layForm->addWidget(&txtFrom, 2, 2, 1, -1);

	txtTo.setMinimumHeight(18);
	txtTo.setPlaceholderText("{search key to}");
	txtTo.setStyleSheet(txtFrom.styleSheet());
	layForm->addWidget(&txtTo, 3, 2, 1, -1);

	cmdGet.setFixedHeight(18);
	layForm->addWidget(&cmdGet, 4, 10, 1, 1);

	connect(&cmdGet, &QPushButton::clicked, this, [&] {

		emit clickedGet(lstIndexName.currentText(), lstOperatorFrom.currentIndex(), txtFrom.text(), lstOperatorTo.currentIndex(), txtTo.text());
	});
}
//---------------------------------------------------------------------------------------
FormSetData::FormSetData(const QString &caption, QWidget *parent) :
	FormBase(caption, parent)
{
	layForm->addItem(new QSpacerItem(10, 18, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0, 1, 10);
	layForm->addItem(new QSpacerItem(10, 18, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0, 1, 10);

	layForm->addWidget(new QLabel("Tuple:"), 1,0,1,1, Qt::AlignRight);
	txtTuple.setMinimumHeight(18);
	txtTuple.setPlaceholderText("{tuple}");
	txtTuple.setStyleSheet("font-family: 'arial'; font-size: 10pt; qproperty-alignment:AlignHCenter;");
	layForm->addWidget(&txtTuple, 1, 1, 1, -1);

	cmdSet.setFixedHeight(18);
	layForm->addWidget(&cmdSet, 3, 10, 1, 1);

	connect(&cmdSet, &QPushButton::clicked, this, [&] {

		emit clickedSet(txtTuple.text());
	});
}
/****************************************************************************************
 *									Dialogs
****************************************************************************************/
FormNetLogin::FormNetLogin(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle("Login");
	setModal(true);
	resize(269, 95);
	setFixedSize(size());
	setStyleSheet("QWidget {font: 'Arial'; font-size: 10pt}"); // for all Widgets on form
}
//...................................
int
FormNetLogin::exec(const QString &host, const QString &user, const QString &password)
{
	Host =host;
	User =user;
	Password =password;

return(exec());
}
//...................................
int
FormNetLogin::exec()
{
QLabel lbl("Host:", this), lbl2("User:", this), lbl3("Password:", this);

	lbl.setGeometry(5, 10, 61, 18);
	lbl2.setGeometry(5, 30, 61, 18);
	lbl3.setGeometry(5, 50, 61, 18);
	lbl.setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
	lbl2.setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
	lbl3.setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

QLineEdit txtHost(Host, this), txtUser(User, this), txtPassword(Password, this);

	txtHost.setGeometry(65, 10, 201, 18);
	txtHost.setPlaceholderText("http://localhost:3301");
	txtUser.setGeometry(65, 30, 201, 18);
	txtUser.setPlaceholderText("guest");
	txtPassword.setGeometry(65, 50, 201, 18);
	txtPassword.setEchoMode(QLineEdit::Password);

	connect(&txtHost, &QLineEdit::textChanged, [&] (const QString &host) {
		Host =host;
	});
	connect(&txtUser, &QLineEdit::textChanged, [&] (const QString &user) {
		User =user;
	});
	connect(&txtPassword, &QLineEdit::textChanged, [&] (const QString &password) {
		Password =password;
	});

QDialogButtonBox buttonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok, Qt::Horizontal, this);

	buttonBox.setGeometry(105, 70, 161, 21);

	connect(&buttonBox, &QDialogButtonBox::accepted, this, &FormNetLogin::accept);
	connect(&buttonBox, &QDialogButtonBox::rejected, this, &FormNetLogin::reject);

return(QDialog::exec());
}
//------------------------------------------------------------------------------------

