#pragma once
/************************************************************************************************
 * Tarantool Qt/C++ simple test gui-client app
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 22.07.2022 🄯 JML
************************************************************************************************/
#include <QApplication>
#include <QLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QFrame>
#include <QRect>
#include <QScreen>
#include "../../src/qtarantool.h"

void on_ConnectClicked();
void on_SendClicked();
void on_Connected(const bool connected);
void on_Received(const QByteArray &data);

typedef QString txt;

class Window : public QFrame
{
	Q_OBJECT

public:
	Window(const int width, const int height);
	bool bClose =false;

protected:
	void closeEvent(QCloseEvent *event) { bClose =true; };
};
