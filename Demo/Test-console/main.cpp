/************************************************************************************************
 * Tarantool Qt/C++ simple test console-client app
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 22.07.2022 🄯 JML
************************************************************************************************/
#include <QCoreApplication>
#include <QDebug>
#include "../../src/qtarantool.h"

using namespace QTNT;
typedef QString txt;
/**************************************************************************************************
 >>> Demo program for writing <tuples_total> tuples to Space <s_name> <<<

 This program implies that:
	 1. You have a Tarantool server running on <host>
	 2. There is a <user> with enough privileges to create a Space and an Index in it

 --- Some commands of the Tarantool server administrator console for managing user privileges ---

 Minimum user privileges to use the QTarantool connector:
	> box.schema.user.grant(<user>, 'execute', 'universe')

 Full user access to the database:
	> box.schema.user.grant(<user>, 'read,write,execute,create,drop', 'universe')

 more.. https://www.tarantool.io/en/doc/latest/book/admin/access_control/
**************************************************************************************************/
int main(int argc, char *argv[])
{
QCoreApplication a(argc, argv); // This is necessary for <QEventLoop> objects
QTarantool tnt;

bool batch_mode =true;
int tuples_total =10000;
QString host ="http://localhost:3301",
	user ="bob",
	password ="1234",
	SpaceName ="Test-Space",
	IndexName ="primary";

	try
	{
		qDebug("Connect to %ls ..", host.utf16());
	// Connect to server
		if(!tnt.connectToServer(host))
			throw txt("connect to host");

		qDebug("OK");
		qDebug("Ping server: %lld uS", tnt.ping() / 1000); // server ping in microseconds
		qDebug("Login as '%ls' ..", user.utf16());
	// Login
		if(!tnt.login(user, password))
			throw txt("login");

		qDebug("OK");
		qDebug("Check if space '%ls' exist ..", SpaceName.utf16());
	// Check space
		if(tnt.isSpaceExist(SpaceName))
			qDebug("Space '%ls' found.", SpaceName.utf16());
		else
		{
			qDebug("Create space '%ls' ..", SpaceName.utf16());
		// Create space [NOTE] * If the space has a format, it will not accept tuples that have fewer fields than the format of the space (but it will accept those that have more) *
			if(!tnt.createSpace(SpaceName, {"format ={'data-1', 'data-2', 'data-3', 'data-4', 'data-5', 'data-6', 'data-7', 'data-8', 'data-9', 'data-10'}"}))
				throw txt("create space");
		}

		qDebug("OK");
		qDebug("Check if index '%ls' exist ..", IndexName.utf16());
	// Check index
		if(tnt.isIndexExist(SpaceName, IndexName))
			qDebug("Index '%ls' found.", IndexName.utf16());
		else
		{
			qDebug("Create index '%ls' ..", IndexName.utf16());
		// Create index
			if(!tnt.createIndex(SpaceName, IndexName, {{1, "unsigned"}}))
				throw txt("create index");
		}

		qDebug("OK");
		qDebug("Space length: %lld tuples", tnt.getSpaceLength(SpaceName));
		qDebug("Space size: %lld bytes", tnt.getSpaceSize(SpaceName));
		qDebug("Preparing the data to send ..");

	QList<List> data;
	QElapsedTimer tmr;
	// Prepare data
		srand(time(nullptr));

		for(int c =0; c < tuples_total; ++c)
			data +=List {rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand(), rand()}; // each tuple has 10 fields

		qDebug("OK");
		qDebug("Sending %d tuples to space ..", tuples_total);
	// Send data
		tmr.start();

		if(batch_mode)
		{
			if(!tnt.setData(SpaceName, data))
				throw txt("set tuples");
		}
		else
			for(int c =0; c < tuples_total; ++c)
				if(!tnt.setData(SpaceName, data[c], false)) // what 'false' means - if a tuple with a tuple key already exists it will be replaced by the new one.
					throw txt("set tuple %d").arg(c);

	qint64 nsec =tmr.nsecsElapsed();
	uint key =rand(), tuples_check =10;

		qDebug("OK");
		qDebug("Check %d tuples from random key {%d} ..", tuples_check, key);
	// Check data
	List tuples =tnt.getData(SpaceName, {GE, {key}}, {}, tuples_check);

		if(tnt.getLastError().code)
			throw txt("check tuples");

		for(uint c =0; c < tuples_check; ++c)
			qDebug("[%d] %s", c, Key(tuples[c].toList()).text().toUtf8().data()); // Here, the IndexKey constructor is used just to format the tuple into text.

	// All tasks have been done.
		qDebug("OK");
		qDebug("Saving all tuples in space takes: %f seconds", nsec / 1000000000.0); // The tuple transfer time in seconds
		qDebug("Space length: %lld tuples", tnt.getSpaceLength(SpaceName));
		qDebug("Space size: %lld bytes", tnt.getSpaceSize(SpaceName));
		qDebug("EXIT");
	}
	// on Error
	catch(const QString &e)
	{
	const ERROR &error =tnt.getLastError();

		qDebug("\nSTOP on >>> %ls", e.utf16());
		qDebug("Tarantool error: [%d] %ls\n", error.code, error.text.utf16());
	}
}
