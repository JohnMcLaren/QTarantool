/************************************************************************************************
 * Tarantool Qt/C++ simple test gui-client app
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 22.07.2022 🄯 JML
************************************************************************************************/
#include "main.h"

using namespace QTNT;

Window *win;
QGridLayout *lay;
QPushButton *cmdConnect, *cmdSend;
QLineEdit *txtServer;
QPlainTextEdit *txtTerminal;
bool bAuthenticated =false;
QTarantool tnt;
//--------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
QApplication app(argc, argv);

	win = new Window(600, 400);
	lay = new QGridLayout(win);

	cmdConnect = new QPushButton("connect", win);
	cmdSend = new QPushButton("send", win);
	txtServer = new QLineEdit("http://localhost:3301", win);
	txtTerminal = new QPlainTextEdit(win);

	lay->addWidget(txtServer, 0, 0, 1, 6);
	lay->addWidget(cmdConnect, 0, 6, 1, 1);
	lay->addWidget(txtTerminal, 1, 0, 10, 7);
	lay->addWidget(cmdSend, 11, 6, 1, 1);
	app.connect(cmdConnect, &QPushButton::clicked, on_ConnectClicked);
	app.connect(cmdSend, &QPushButton::clicked, on_SendClicked);
	//app.connect(&tnt, &QTarantool::signalReceived, on_Received);
	app.connect(&tnt, &QTarantool::signalConnected, on_Connected);
	app.connect(&tnt, &QTarantool::error, [&] (const ERROR &error) {

		qDebug() << QString("Tarantool error: [%1] %2").arg(error.code).arg(error.text);
	});

	win->show();

return app.exec();
}
//--------------------------------------------------------------------------------------------------
void
on_SendClicked()
{
	if(!tnt.isConnected())
		return;

	if(!bAuthenticated && !(bAuthenticated =tnt.login("guest", "")))
	{
		tnt.disconnectServer();

	return;
	}
//................................................. TODO
QStringList out;
QVariantList Spaces =tnt.spaces();

	out << "Spaces total:" << txt::number(Spaces.length());
	out << "\n\nFirst Space meta-information:\n" << QJsonDocument::fromVariant(Spaces[0]).toJson(QJsonDocument::Indented).data();
	out << "\nServer Config:\n";
	out << QJsonDocument::fromVariant(tnt.cfg()).toJson(QJsonDocument::Indented).data();

	txtTerminal->setPlainText(out.join(""));
}
//------------------------------------------------------- signals from Tarantool instance
void
on_ConnectClicked()
{
	if(txtServer->isEnabled())	// connect
		tnt.connectToServer(txtServer->text());
	else						// disconnect
	{
		tnt.disconnectServer();
		bAuthenticated =false;
	}
}
//------------------------------------------------------
// * There should NOT be requests to the server here. *
void
on_Connected(const bool connected)
{
	if(connected)
	{
		txtServer->setEnabled(false);
		cmdConnect->setText("disconnect");
	}
	else
	if(!win->bClose)
	{
		cmdConnect->setText("connect");
		txtServer->setEnabled(true);
	}
}
//--------------------------------------------------------------------------------------------------
void
on_Received(const QByteArray &data)
{
	txtTerminal->setPlainText(data);
}
//--------------------------------------------------------------------------------------------------
Window::Window(const int width, const int height)
{
	setGeometry(QRect(screen()->size().width() / 3, screen()->size().height() / 3, width, height));
}


