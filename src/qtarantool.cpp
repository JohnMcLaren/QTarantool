/******************************************************************
 * QTarantool Qt/C++ wrapper class for Tarantool database
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 22.07.2022 🄯 JML
******************************************************************/
#include "qtarantool.h"

using namespace QTNT;
//----------------------------------------------------------------------------------------
QTarantool::QTarantool(QObject *parent) : QThread(parent)
{
	socket = new QTcpSocket();

	connect(socket, &QTcpSocket::connected, this, &QTarantool::onSocketConnected); // [QT-NOTE] Qt::QueuedConnection required qRegisterMetaType()
	connect(socket, &QTcpSocket::disconnected, this, &QTarantool::onSocketDisconnected);
	connect(socket, &QTcpSocket::readyRead, this, &QTarantool::readyRead);

#if QT_VERSION > QT_VERSION_CHECK(5, 15, 0)
	connect(socket, &QTcpSocket::errorOccurred, this, &QTarantool::onSocketError);
#else
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
#endif
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::connectToServer(const QString &uri)
{
QUrl url(uri);

	if(!url.isValid() || url.host().isEmpty() || url.port() == (-1))
		return(false);

	disconnectServer();
	socket->connectToHost(url.host(), url.port());

	/* wait for connect & init */
	waitFor(this, SIGNAL(signalConnected(bool)), TIMEOUT);

return(isConnected() & bInit);
}
/****************************************************************************************
 *
****************************************************************************************/
void
QTarantool::disconnectServer()
{
	if(isConnected())
		socket->disconnectFromHost();

	UserName ="";
}
/****************************************************************************************
 *
****************************************************************************************/
const QString
QTarantool::getServerDirectory()
{
	exec("return package.searchroot()");

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toString());

return("");
}
/****************************************************************************************
 * Send ping to server
 * Return: Measured ping time in nanoseconds.
 *		   0 (zero) if timeout.
****************************************************************************************/
qint64
QTarantool::ping()
{
QUIntMap hdr;

	if(!isConnected())
		return(0);

	hdr[IPROTO_REQUEST_TYPE] =IPROTO_PING;
	hdr[IPROTO_SYNC] =(++syncId);

QByteArray request(sizeof(HDR_DATA_SIZE), Qt::Uninitialized); // reserve HDR_DATA_SIZE

	request +=MsgPack::pack(hdr); // append tarantool header
	((HDR_DATA_SIZE *)request.data())->mp_hdr =0xCE; // CONST MP_UINT
	((HDR_DATA_SIZE *)request.data())->data_size =(request.size() - sizeof(HDR_DATA_SIZE));

QElapsedTimer tmr;

	tmr.start();

	if(send(request))
		waitFor(socket, SIGNAL(readyRead()), TIMEOUT);

qint64 nsec =tmr.nsecsElapsed();

	if(tmr.hasExpired(TIMEOUT)) // TIMEOUT in mSec (miliseconds)
		return(0);

	socket->readAll(); // clear socket buffer

return(nsec); // result in nSec (nanoseconds)
}
/****************************************************************************************
 * Return: server config
****************************************************************************************/
const QVariantMap &
QTarantool::cfg()
{
	exec("return box.cfg");

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariantMap &>(DataList[0]));

return(MAPNULL);
}
/****************************************************************************************
 * Return: server memory info
****************************************************************************************/
const QVariantMap
QTarantool::slab(const SLAB type)
{
	switch(type)
	{
	case INFO:
		exec("return box.slab.info()");
		break;

	case DETAIL:
		exec("return box.slab.stats()");

			if(Reply.IsValid)
				return(QVariantMap {{"slabs", Reply.Data[IPROTO_DATA].toList()[0]}});

		return(QVariantMap {});

	case RUNTIME:
		exec("return box.runtime.info()");
		break;
	}

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toMap());

return(MAPNULL);
}
/****************************************************************************************
 * Return: server info
****************************************************************************************/
const QVariantMap &
QTarantool::info()
{
	exec("return box.info()");

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariantMap &>(DataList[0]));

return(MAPNULL);
}
/****************************************************************************************
 * Return: server stat
****************************************************************************************/
const QVariantMap &
QTarantool::stat(const STAT type)
{
	switch(type)
	{
	case REQUESTS:
		exec("return box.stat()");
		break;

	case NETWORK:
		exec("return box.stat.net()");
		break;

	case VINYL:
		exec("return box.stat.vinyl()");
		break;
	}

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariantMap &>(DataList[0]));

return(MAPNULL);
}
/****************************************************************************************
 * Return: true if successful login otherwise false.
****************************************************************************************/
bool
QTarantool::login(const QString &userName, const QString &password)
{
QByteArray scramble, salt_pswd;
QUIntMap hdr, body;

	if(password.size())
	{
		scramble =SHA1(password.toUtf8());                            // 160-bit
		salt_pswd =SHA1(salt.left(scramble.size()) + SHA1(scramble)); // 160-bit

		for(int c =0; c < scramble.size(); c++)
			scramble[c] =scramble[c] ^ salt_pswd[c];

		body[IPROTO_TUPLE] =QVariantList {"chap-sha1", scramble};
	}
	else
		body[IPROTO_TUPLE] =QVariantList {};

	hdr[IPROTO_REQUEST_TYPE] =IPROTO_AUTH;
	body[IPROTO_USER_NAME]   =userName;

	if(!sendRequest(hdr, body).IsValid)
		return(false);

	UserName =userName;

return(true);
}
/****************************************************************************************
 * Return: Current user name.
****************************************************************************************/
QString
QTarantool::getUserName()
{
	exec("return box.session.user()");

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toString());

return("");
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::createUser(const QString &userName, const QString &userPassword)
{
	return(exec("box.schema.user.create(...)", {userName, Map {{"password", userPassword}, {"if_not_exists", false}}}).IsValid);
}
/****************************************************************************************
 * Set User grants.
****************************************************************************************/
bool
QTarantool::grantUser(const QString &userName, const QString &userPrivileges, const QString &objectType, const QString &objectName)
{
// [FIXME] ERROR: 42 "Grant access to universe '' is denied for user ''"
	return(exec("box.schema.user.grant(...)", {userName, userPrivileges, objectType, objectName, Map {{"if_not_exists", true}}}).IsValid);
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::grantUserByRole(const QString &userName, const QString &userRole)
{
	return(exec("box.schema.user.grant(...)", {userName, userRole, VARNULL, VARNULL, Map {{"if_not_exists", true}}}).IsValid);
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::deleteUser(const QString &userName)
{
	return(exec("box.schema.user.drop(...)", {userName, Map {{"if_exists", false}}}).IsValid);
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::isUserExist(const QString &userName)
{
	exec("return box.schema.user.exists(...)", {userName});

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toBool());

return(false);
}
/****************************************************************************************
 * Return: List all users with all properties
****************************************************************************************/
const QVariantList &
QTarantool::users()
{
	exec("return box.space._user:select{}");

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariantList &>(DataList[0]));

return(LISTNULL);
}
/****************************************************************************************
 *
****************************************************************************************/
const QVariantList &
QTarantool::grants(const QString &userName)
{
	call("box.schema.user.info", {userName});

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariantList &>(DataList[0]));

return(LISTNULL);
}
/****************************************************************************************
 * Create new Space.
 * Return: Id of the new Space, or 0 if it fails.
****************************************************************************************/
uint
QTarantool::createSpace(const QString &spaceName, const QStringList &options)
{
//	[FIXME] call("box.schema.space.create", QVariantList {spacename, options})) // ERROR: 32 "unsupported Lua type 'function'"
	exec(tr("return box.schema.space.create('%1', {%2}).id").arg(spaceName).arg(options.join(',')));

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toUInt());

return(0);
}
/****************************************************************************************
 * Return: Id of the Space by Name, or 0 if not exist.
****************************************************************************************/
uint
QTarantool::getSpaceId(const QString &spaceName)
{
	exec("return box.space[...].id", {spaceName});

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toUInt());

return(0);
}
/****************************************************************************************
 * Return: Name of the Space by Id, or empty string if not exist.
****************************************************************************************/
QString
QTarantool::getSpaceName(const uint spaceId)
{
	exec("return box.space[...].name", {spaceId});

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toString());

return("");
}
/****************************************************************************************
 * Return: true if successful change settings of Space<Id> otherwise false.
 * [!] Tarantool version >= 2.7
****************************************************************************************/
bool
QTarantool::changeSpace(const QString &spaceName, const QStringList &newParams)
{
	return(exec(tr("box.space['%1']:alter({%2})").arg(spaceName).arg(newParams.join(','))).IsValid);
}
/****************************************************************************************
 * Return: true if successful delete data of Space[<name>] otherwise false.
****************************************************************************************/
bool
QTarantool::clearSpace(const QString &spaceName)
{
	return(exec("box.space[...]:truncate()", {spaceName}).IsValid);
}
/****************************************************************************************
 * Return: true if successful deleted/destroyed Space[<name>] otherwise false.
****************************************************************************************/
bool
QTarantool::deleteSpace(const QString &spaceName)
{
	return(exec("box.space[...]:drop()", {spaceName}).IsValid);
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::isSpaceExist(const QString &spaceName)
{
	exec(tr("return box.space[...] ~= null"), {spaceName});

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toBool());

return(false);
}
/****************************************************************************************
 * Return: list of tuples
****************************************************************************************/
const QVariantList &
QTarantool::getData(const QString &spaceName, const Selector &selectorFrom, const Selector &selectorTo, const uint limit)
{
bool fwd =false;

	if(selectorTo.Operator) // if !ALL
		if((selectorFrom.Operator < LE && selectorTo.Operator < LE) || (selectorFrom.Operator >= LE && selectorTo.Operator >= LE))
			return(LISTNULL); // unidirectional selectors
		else
		if(selectorFrom.Operator < LE)
			fwd =true; // forward

QString lua(tr("local idx =box.space['%1'].index['%2'];").arg(spaceName).arg(selectorFrom.IndexName));

	lua +=tr("local result =idx:pairs(%1,{iterator='%2'});").arg(selectorFrom.Key.text()).arg(ToStr(selectorFrom.Operator));

	if(selectorTo.Operator) // if !ALL
	{
		lua +=tr("local k =require('key_def').new(idx.parts);"
				 "local stop =idx:select(%1, {iterator='%2'})[1];").arg(selectorTo.Key.text()).arg(ToStr(selectorTo.Operator));
		lua +=tr("result =result:take_while(function(t) return k:compare(t, stop) %1 0 end);").arg(fwd ? "<=" : ">=");
	}

	lua +=tr("return result:take(%1):totable();").arg(limit);

	exec(lua);

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariantList &>(DataList[0]));

return(LISTNULL);
}
/****************************************************************************************
 * Get field of tuple
- Fields are numbered from 1
- Accessing a field that does not exist does NOT cause an ERROR [TNT-Note]
- 'field' number are only positive
****************************************************************************************/
const QVariant &
QTarantool::getData(const QString &spaceName, const IndexKey &key, const uint field, const QString &indexName)
{
	if(indexName.isEmpty())
		exec(tr("return box.space['%1']:get(...)[%2]").arg(spaceName).arg(field), key);
	else
		exec(tr("return box.space['%1'].index['%2']:get(...)[%3]").arg(spaceName).arg(indexName).arg(field), key);

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariant &>(DataList[0]));

return(VARNULL);
}
/****************************************************************************************
 * Set new values for 1(one) tuple by primary index.
 * bIfExist =true : will set only the existing tuple otherwise it will return an error.
 * bIfExist =false : if the tuple did not exist it will be inserted as a new tuple.
****************************************************************************************/
bool
QTarantool::setData(const QString &spaceName, const QVariantList &tuple, const bool bIfExist) // [FIXME] double values
{
	if(tuple.size())
		if(!bIfExist)
			return(exec(tr("box.space['%1']:replace{...}").arg(spaceName), tuple).IsValid);
		else
			return(exec(tr("local s =box.space['%1'];").arg(spaceName) +
							"local t ={...};"
							"if s:get(require('key_def').new(s.index[0].parts):extract_key(t)) then"
							"	s:replace(t);"
							"else error('Key not found.') end", tuple).IsValid);
return(false);
}
/****************************************************************************************
 * Set new values for tuples by primary index.
 * If the tuple from list 'tuples' did not exist it will be inserted as a new tuple.
****************************************************************************************/
bool
QTarantool::setData(const QString &spaceName, const QList<QVariantList> &tuples, const int batchSize)
{
bool result =true;
QString lua =tr("local tt ={...};"
				"for i = 1, #tt do"
				"	box.space['%1']:replace(tt[i]) end;").arg(spaceName);

	for(auto it =tuples.begin(), it_end =it; it != tuples.end() && result; it =it_end)
	{
		it_end =(it + batchSize <= tuples.end() ? it + batchSize : tuples.end());
		result =exec(lua, {it, it_end}).IsValid;
	}

return(result);
}
/****************************************************************************************
 * Insert new 1(one) tuple by primary index
 * If the tuple exists, method will return an error.
****************************************************************************************/
bool
QTarantool::insertData(const QString &spaceName, const QVariantList &tuple) // [FIXME] double values
{
	if(tuple.size())
		return(exec(tr("box.space['%1']:insert{...}").arg(spaceName), tuple).IsValid);

return(false);
}
/****************************************************************************************
 * Changes the value of the specified field for tuple that matched the specified 'key'.
 * The tuple will be searched in the specified index 'indexName' (any unique index).
****************************************************************************************/
bool
QTarantool::changeData(const QString &spaceName, const IndexKey &key, const int field, const QVariant &value, const QString &indexName)
{
	if(!key.size())
		return(false);

	if(indexName.isEmpty())
		return(exec(tr("box.space['%1']:update(...)").arg(spaceName), {key, List { List {"=", field, value}}}).IsValid);
	else
		return(exec(tr("box.space['%1'].index['%2']:update(...)").arg(spaceName).arg(indexName), {key, List { List {"=", field, value}}}).IsValid);
}
/****************************************************************************************
 * Applies the chosen 'actions' to the fields of the selected tuple by 'key' in any unique index 'indexName'.
****************************************************************************************/
bool
QTarantool::changeData(const QString &spaceName, const IndexKey &key, const Actions &actions, const QString &indexName)
{
	if(!key.size())
		return(false);

	if(indexName.isEmpty())
		return(exec(tr("box.space['%1']:update(...)").arg(spaceName), {key, {actions}}).IsValid);
	else
		return(exec(tr("box.space['%1'].index['%2']:update(...)").arg(spaceName).arg(indexName), {key, {actions}}).IsValid);
}
/****************************************************************************************
 * Delete tuple by 'key'
****************************************************************************************/
bool
QTarantool::deleteData(const QString &spaceName, const IndexKey &key, const QString &indexName)
{
	if(!key.size())
		return(false);

	if(indexName.isEmpty())
		return(exec(tr("box.space['%1']:delete{...}").arg(spaceName), key).IsValid);
	else
		return(exec(tr("box.space['%1'].index['%2']:delete{...}").arg(spaceName).arg(indexName), key).IsValid);
}
/****************************************************************************************
 * Returns the quantity of tuples in the Space.
 *    0: space is empty
 * (-1): on error
****************************************************************************************/
qlonglong
QTarantool::getSpaceLength(const QString &spaceName)
{
	exec("return box.space[...]:len()", {spaceName});

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].value<qlonglong>());

return(-1);
}
/****************************************************************************************
 * Returns the size taken by data (tuples) of Space in bytes.
****************************************************************************************/
qlonglong
QTarantool::getSpaceSize(const QString &spaceName)
{
	exec("return box.space[...]:bsize()", {spaceName});

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].value<qlonglong>());

return(-1);
}
/****************************************************************************************
 * Return: List all spaces with attributes
****************************************************************************************/
const QVariantList &
QTarantool::spaces()
{
	exec("return box.space._space:select{}");

const auto &DataList =reinterpret_cast<const QVariantList &>(Reply.Data[IPROTO_DATA]);

	if(Reply.IsValid)
		return(reinterpret_cast<const QVariantList &>(DataList[0]));

return(LISTNULL);
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::createIndex(const QString &spaceName, const QString &indexName, const Parts &parts, const QStringList &options)
{
QStringList tmp(options);
// [FIXME] format string is temp solution
	tmp +=("parts =" + parts.text());

return(exec(tr("box.space['%1']:create_index('%2', {%3})").arg(spaceName).arg(indexName).arg(tmp.join(','))).IsValid);
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::isIndexExist(const QString &spaceName, const QString &indexName)
{
	exec(tr("return box.space['%1'].index['%2'] ~= null").arg(spaceName).arg(indexName));

	if(Reply.IsValid)
		return(Reply.Data[IPROTO_DATA].toList()[0].toBool());

return(false);
}
/****************************************************************************************
 *
****************************************************************************************/
bool
QTarantool::deleteIndex(const QString &spaceName, const QString &indexName)
{
	return(exec(tr("box.space['%1'].index['%2']:drop()").arg(spaceName).arg(indexName)).IsValid);
}
/****************************************************************************************
 * Returns all indexes of all Spaces as a map (UIntMap),
 * where the map key is the Id of the associated Space.
****************************************************************************************/
const QUIntMap
QTarantool::indexes()
{
	exec("return box.space._index:select{}");

	if(Reply.IsValid)
	{
	QUIntMap indexes;

		for(const auto& varAttr : Reply.Data[IPROTO_DATA].toList()[0].toList())
		{
		const QVariantList &attr =varAttr.toList();
		uint spaceId =attr[0].toUInt();
		QVariantList &indexes_ref =reinterpret_cast<QVariantList &>(indexes[spaceId].isValid() ? indexes[spaceId] : (indexes[spaceId] =QVariantList {}));

			indexes_ref.append(QVariantMap { // indexes[spaceId] = <index attributes>

				{"iid", attr[1]},
				{"name", attr[2]},
				{"type", attr[3]},
				{"opts", attr[4]},
				{"parts", attr[5]}
			});
		}

	return(indexes);
	}

return(QUIntMap {});
}
/****************************************************************************************
 * Calls an arbitrary Lua-function on the server.
****************************************************************************************/
const REPLY &
QTarantool::call(const QString &function, const QVariantList &args)
{
QUIntMap hdr, body;

	hdr[IPROTO_REQUEST_TYPE] =IPROTO_CALL;
	body[IPROTO_FUNCTION_NAME] =function;
	body[IPROTO_TUPLE] =args;

return(sendRequest(hdr, body));
}
/****************************************************************************************
 * Executes an arbitrary Lua-expression on the server.
****************************************************************************************/
const REPLY &
QTarantool::exec(const QString &script, const QVariantList &args)
{
QUIntMap hdr, body;

	hdr[IPROTO_REQUEST_TYPE] =IPROTO_EVAL;
	body[IPROTO_EXPR] =script;
	body[IPROTO_TUPLE] =args;

return(sendRequest(hdr, body));
}
/****************************************************************************************
 * Executes an arbitrary SQL-request to the server.
 * Returns:
****************************************************************************************/
const REPLY &
QTarantool::execSQL(const QString &query, const QVariantList &args, const QVariantList &options)
{
QUIntMap hdr, body;

	hdr[IPROTO_REQUEST_TYPE] =IPROTO_EXECUTE;
	body[IPROTO_SQL_TEXT] =query;
	body[IPROTO_SQL_BIND] =args;
	body[IPROTO_OPTIONS] =options;

return(sendRequest(hdr, body));
}
/****************************************************************************************
 * Prepares and sends a request msgpack-packet to the server.
 * If it receives an error status from the server it will return an empty REPLY structure,
 * - see REPLY structure assignment operator overloading.
****************************************************************************************/
const REPLY &
QTarantool::sendRequest(QUIntMap &header, const QUIntMap &body)
{
	lasterror ={0, ""};
	header[IPROTO_SYNC] =(++syncId);

QByteArray request(sizeof(HDR_DATA_SIZE), Qt::Uninitialized);

	request +=MsgPack::pack(header);
	request +=MsgPack::pack(body);

	((HDR_DATA_SIZE *)request.data())->mp_hdr =0xCE; // CONST MP_UINT
	((HDR_DATA_SIZE *)request.data())->data_size =(request.size() - sizeof(HDR_DATA_SIZE));

	if(send(request))
		waitFor(socket, SIGNAL(readyRead()), TIMEOUT);

	Reply =MsgPack::unpack(socket->readAll());

	if(!Reply.IsValid)
		setLastError({0, "Malformed server response."});
	else
	if(Reply.Header[IPROTO_STATUS] != IPROTO_OK) // if ERROR return
	{
		Reply.IsValid =false;
		setLastError({Reply.Header[IPROTO_STATUS].toInt() & 0x7FFF, Reply.Data[ERROR_STRING].toString()});
	}

return(Reply);
}
/****************************************************************************************
 * Send raw data to server
 * Returns: the number of bytes sent.
****************************************************************************************/
qint64
QTarantool::send(const QByteArray &data)
{
	if(isConnected())
		return(socket->write(data));

return(0);
}
/****************************************************************************************
 * Socket events handlers
****************************************************************************************/
void
QTarantool::onSocketConnected() // ignore
{
	qDebug("Connected to server. [%d]", isConnected());
	syncId =0;
}
//----------------------------------------------------------------------------------------
void
QTarantool::onSocketDisconnected()
{
	qDebug("Disconnected server.");
	bInit =false;
	emit signalConnected(false);
}
//----------------------------------------------------------------------------------------
void
QTarantool::onSocketError(QAbstractSocket::SocketError error)
{
	setLastError({0, QString("Socket error [%1].").arg(error)});
	emit signalConnected(isConnected() & bInit);
}
//----------------------------------------------------------------------------------------
void
QTarantool::readyRead()
{
	//qDebug("Received[%lld]", socket->bytesAvailable());

	if(!bInit) // need init
	{
	QByteArray baReply(socket->readAll());
	QList slReply =baReply.split('\n');

		if(baReply.size() == 128 && slReply.length() == 3) // Server 'greetings' exactly 128 bytes & must be 3 lines
		{
			version =slReply.at(0);
			salt =QByteArray::fromBase64(slReply.at(1));
			bInit =true;
		}
		else
			disconnectServer();

		emit signalConnected(isConnected() & bInit);

	return;
	}

	//emit signalReceived(socket->readAll()); // ??
}
/****************************************************************************************
 * Wait signal for object in a single threaded application
 * [QT-NOTE]: Requires a <QApplication> instance
****************************************************************************************/
void
QTarantool::waitFor(const QObject *object, const char *signal, int timeout)
{
QEventLoop loop;

	if(timeout)
		QTimer::singleShot(timeout, &loop, &QEventLoop::quit);

	loop.connect(object, signal, &loop, SLOT(quit()), Qt::QueuedConnection);
	loop.exec();
}

