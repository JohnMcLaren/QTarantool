#pragma once
/******************************************************************
 * QTarantool Qt/C++ wrapper class for Tarantool database
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 22.07.2022 🄯 JML
******************************************************************/
#include <QObject>
#include <QtEndian>
#include <QByteArray>
#include <QTimer>
#include <QElapsedTimer> // for Windows
#include <QEventLoop>
#include <QTcpSocket>
#include <QThread>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDebug>
//#include "msgpuck/msgpuck.h"
#include "qmsgpack/msgpack.h"

namespace QTNT
{
//-------------- defines QTarantool -------------------
#define TIMEOUT 3000
#define SHA1(data) QCryptographicHash::hash((data), QCryptographicHash::Sha1)
// short aliases
typedef QVariantMap Map;
typedef QVariantList List;

//-------------- defines Tarantool --------------------
#define HAVE_MEMRCHR
#define HAVE_CLOCK_GETTIME_DECL

#if !defined __GNUC_MINOR__ || defined __INTEL_COMPILER || \
	defined __SUNPRO_C || defined __SUNPRO_CC
#define MP_GCC_VERSION(major, minor) 0
#else
#define MP_GCC_VERSION(major, minor) (__GNUC__ > (major) || \
	(__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#endif

#if MP_GCC_VERSION(4, 3) || __has_builtin(__builtin_bswap32)
#define mp_bswap_u32(x) __builtin_bswap32(x)
#else /* !MP_GCC_VERSION(4, 3) */
#define mp_bswap_u32(x) ( \
	(((x) << 24) & UINT32_C(0xff000000)) | \
	(((x) <<  8) & UINT32_C(0x00ff0000)) | \
	(((x) >>  8) & UINT32_C(0x0000ff00)) | \
	(((x) >> 24) & UINT32_C(0x000000ff)) )
#endif

#include "include/iproto_constants.h"
//--------------------------------------------------------
// raw reply from server
struct REPLY
{
	REPLY() { reset(); }
	REPLY(const uint size, const QUIntMap &header, const QUIntMap &data, const bool bIsValid) :
		/* init */ Size(size), Header(header), Data(data), IsValid(bIsValid) { /* constructor */ }
	const int REPLY_VALID_LEN =3; // <size> + <header> + <body> : always 3

	uint Size;
	QUIntMap Header;
	QUIntMap Data;
	bool IsValid;

	inline void reset() {

		Size =0;
		Header.clear();
		Data.clear();
		IsValid =false;
	}

	inline REPLY& operator= (const QVariant &reply) {

		if(reply.type() == QVariant::List && reinterpret_cast<const QVariantList &>(reply).size() == REPLY_VALID_LEN)
		{
			Size =reinterpret_cast<const QVariantList &>(reply)[0].toUInt();
			Header =reinterpret_cast<const QUIntMap &>(reinterpret_cast<const QVariantList &>(reply)[1]);
			Data =reinterpret_cast<const QUIntMap &>(reinterpret_cast<const QVariantList &>(reply)[2]);
			IsValid =true;
		}
		else
			reset();

	return(*this);
	}
};

struct ERROR
{
	int code;
	QString text;
};

enum SLAB { // [INFO] https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_slab/slab_info/

	INFO,		// Show an aggregated memory usage report for slab allocator
	DETAIL,		// Show a detailed memory usage report for slab allocator
	RUNTIME		// Show a memory usage report for Lua runtime
};

enum STAT {	// [INFO] https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_stat/

	REQUESTS,	// Shows the total number of requests since startup and the average number of requests per second
	NETWORK,	// Shows network activity
	VINYL		// Shows vinyl-storage-engine activity
};

enum OPERATOR {

	ALL =0, // Synonym for 'GE'
	EQ,		// ==
	GE,		// >=
	GT,		// >
	LE,		// <=
	LT,		// <
	REQ,	// == reverse order
};

inline static const char *
ToStr(const OPERATOR op)
{
	switch(op)
	{
	case ALL:
		return("ALL");
	case EQ:
		return("EQ");
	case GE:
		return("GE");
	case GT:
		return("GT");
	case LE:
		return("LE");
	case LT:
		return("LT");
	case REQ:
		return("REQ");
	}

return("");
}
/************************************************************************************************
 *										Key of Index
************************************************************************************************/
typedef class IndexKey : public QList<QVariant>
{
public:
	inline
	IndexKey() : QList<QVariant>() { }
	inline
	IndexKey(const QVariantList &list) : QList<QVariant>(list) { }
	inline
	IndexKey(std::initializer_list<QVariant> args) : QList<QVariant>(args) { }
	inline
	IndexKey(const QString &key)
	{
	QStringList keys ={""};
	QStringList k =key.trimmed().remove(QRegExp("^[{]|[}]$")).split('"'); // remove first '{' and last '}'

		if(!k.size() || (k.size() == 1 && k[0].isEmpty()))
			return;

		try {
			// Re-construction of the string part of the IndexKey.
			if(k.size() & 1)
				for(int c =0; c < k.size(); c++)
					if(c & 1) // odd ("string")
						keys.last() +=k[c].prepend('"').append('"'); // + string
					else // even
					{
					QStringList kk =k[c].split(',');

						keys.last() +=kk.takeFirst(); // + final part of the preceding object
						keys +=kk; // + remaining list items
					}
			else
				throw(0); // Unclosed string.

		QStringList::Iterator begin =keys.begin(), end =keys.end();

			*this =IndexKey {begin, end};
		}
		catch(int nestingLevel) {

			IsValid =false;
			qDebug("Construct error: %d", nestingLevel);
		}
	}
	 // index to string
	inline const QString
	text() const
	{
		try
		{
			return(toString(*this));
		}
		catch(...)
		{
			return("");
		}
	}

private:

	bool IsValid =true;

	IndexKey(QStringList::Iterator &beginKey, QStringList::Iterator &endKey, int nestingLevel =0, int *pEndIndexKeyCount =nullptr)
	{
	int EndIndexKeyCount =0;

		while(beginKey != endKey && !EndIndexKeyCount)
		{
		QString strKey =beginKey->trimmed();
			// Object begin/end
			if(strKey.startsWith('{')) // begin new nested IndexKey
			{
				beginKey->remove(0, 1 + beginKey->indexOf('{')); // remove '{'
				this->insert(this->size(), IndexKey(beginKey, endKey, 1, &EndIndexKeyCount));
				continue;
			}
			else
			while(strKey.endsWith('}')) // end nested IndexKey
			{
				strKey.remove(-1, 1); // remove '}'
				EndIndexKeyCount++;
			}
			// Value
			if((strKey.startsWith('"') || strKey.startsWith('\'')) && (strKey.endsWith('"') || strKey.endsWith('\''))) // string key
				this->append(strKey.remove(0, 1).remove(-1, 1));
			else
			if(strKey.toLower() == "true" || strKey.toLower() == "false") // bool key
				this->append(strKey.toLower() == "true");
			else // number key
			if(strKey.toLong() == strKey.toDouble()) // integer
				this->append(strKey.toLongLong());
			else
				this->append(strKey.toDouble()); // float

			beginKey++;
		}

		if(EndIndexKeyCount--)
		{
			nestingLevel--;

			if(pEndIndexKeyCount)
				*pEndIndexKeyCount =EndIndexKeyCount;
		}

		if(nestingLevel)
			throw(nestingLevel);
	}

	inline static QString
	toString(const IndexKey &key)
	{
	QString s;

		if(!key.IsValid)
			throw(0);

		for(const auto &k : key)
			if(k.type() && k.type() <= QVariant::Double) // number or bool (double ??)
				s +=k.toString().append(',');
			else
			if(k.type() == QVariant::String) // string
				s +=QString("\"%1\"").arg(k.toString()).append(',');
			else
			if(k.type() == QVariant::List) // Key in the Key
				s +=toString(reinterpret_cast<const IndexKey &>(k)).append(',');
			else
				throw(k.type()); // unknown key type

	return(s.left(s.size() - 1).prepend('{').append('}')); // remove last ','
	}

} Key;
/************************************************************************************************
 *									Tarantool class
************************************************************************************************/
class QTarantool : public QThread
{
	Q_OBJECT

public:
	explicit QTarantool(QObject *parent = nullptr);
	~QTarantool() { disconnectServer(); socket->deleteLater(); };

	// for 'Get' tuples methods
	struct Selector
	{
		Selector(const OPERATOR Operator =ALL, const IndexKey &Key ={}, const QString &IndexName ="primary") :
			/* init */ Operator(Operator), Key(Key), IndexName(IndexName) { /* constructor */ }

		OPERATOR Operator;
		IndexKey Key;
		QString  IndexName;
	};
	// for 'Set' tuple methods
	typedef struct FieldAction
	{
		FieldAction(const QString &action, const int field, const QVariant &value) :
			Action(action), Field(field), Value(value) { }

		QString	 Action;
		int      Field;
		QVariant Value;
		// cast struct to QVariant
		inline operator QVariant() const { return(QList<QVariant> {Action, Field, Value}); }

	} Action;

	typedef class FieldActionsList : public QList<FieldAction>
	{
	public:
		inline
		FieldActionsList(std::initializer_list<FieldAction> args) : QList<FieldAction>(args) { }
		// cast QList<FieldAction> to QList<QVariant>
		inline operator QVariantList() const
		{
		QVariantList actions;

			for(const auto &action : *this)
				actions +=action;

		return(actions);
		}

	} Actions;

	// Parts of Index
	typedef struct IndexPart
	{
		IndexPart(const uint field, const QString &type) : Field(field), Type(type) { }
		IndexPart(const QString &name, const QString &type) : Field(name), Type(type) { }

		QVariant Field;
		QString  Type;

		inline QString
		text() const
		{
			if((Field.type() != QVariant::UInt && Field.type() != QVariant::String) || Type.isEmpty())
				return("{}");
			else
				if(Field.type() == QVariant::UInt)
					return(tr("{%1, '%2'}").arg(Field.toUInt()).arg(Type));
				else
					return(tr("{'%1', '%2'}").arg(Field.toString()).arg(Type));
		}
		// cast struct to QVariant
		inline operator QVariant() const { return(QList<QVariant> {Field, Type}); }

	} Part;

	typedef class IndexPartsList : public QList<IndexPart>
	{
	public:
		inline
		IndexPartsList(std::initializer_list<IndexPart> args) : QList<IndexPart>(args) { }

		inline const QString
		text() const
		{
		QString s;

			for(const auto &part : *this)
				s +=part.text().append(',');

		return(s.left(s.size() - 1).prepend('{').append('}'));
		}

		// cast QList<IndexPart> to QList<QVariant>
		inline operator QVariantList() const
		{
		QVariantList parts;

			for(const auto &part : *this)
				parts +=part;

		return(parts);
		}

	} Parts;

// *** Server ***
	const QString
	getServerDirectory();
	bool
	connectToServer(const QString &uri);
	void
	disconnectServer();
	inline bool
	isConnected() { return(socket->state() == QAbstractSocket::ConnectedState); }
	qint64
	ping();
	const QVariantMap &
	cfg();
	const QVariantMap
	slab(const SLAB type);
	const QVariantMap &
	info();
	const QVariantMap &
	stat(const STAT type);
	static bool
	startLocalServer(const QString &filename);
	static bool
	stopLocalServer();

// *** User ***
	bool
	login(const QString &userName, const QString &password);
	inline bool
	logout() { return(login("guest", "")); }
	QString
	getUserName(); // return current user name from server
	bool
	createUser(const QString &userName, const QString &userPassword);
	bool
	grantUser(const QString &userName, const QString &userPrivileges, const QString &objectType ="'universe'", const QString &objectName ="null");
	bool
	grantUserByRole(const QString &userName, const QString &userRole);
	bool
	resetGrants(const QString &userName, const QString &newUserPrivileges, const QString &objectType ="'universe'", const QString &objectName ="null");
	bool
	deleteUser(const QString &userName);
	bool
	isUserExist(const QString &userName);
	inline const QString &
	user() { return(UserName); }; // return last successful login user name
	const QVariantList &
	users(); // all users and their attributes
	const QVariantList &
	grants(const QString &userName); // return grants for User

// *** Space ***
	uint
	createSpace(const QString &spaceName, const QStringList &options ={});
	bool
	changeSpace(const QString &spaceName, const QStringList &newParams);
	bool
	clearSpace(const QString &spaceName);
	bool
	deleteSpace(const QString &spaceName);
	bool
	isSpaceExist(const QString &spaceName);
	const QVariantList &
	getData(const QString &spaceName, const Selector &selectorFrom, const Selector &selectorTo ={}, const uint limit =1000);
	const QVariant &
	getData(const QString &spaceName, const IndexKey &key, const uint field, const QString &indexName ="");
	bool
	setData(const QString &spaceName, const QVariantList &tuple, const bool bIfExist =true); // Set existing tuple (by primary index of tuple) Or Insert as new if not exist.
	bool
	setData(const QString &spaceName, const QList<QVariantList> &tuples, const int batchSize =1000);
	bool
	insertData(const QString &spaceName, const QVariantList &tuple); // Insert only as new tuple by primary index, error - if exist.
	bool
	changeData(const QString &spaceName, const IndexKey &key, const int field, const QVariant &value, const QString &indexName =""); // change field value of tuple
	bool
	changeData(const QString &spaceName, const IndexKey &key, const Actions &actions, const QString &indexName ="");
	bool
	deleteData(const QString &spaceName, const IndexKey &key, const QString &indexName =""); // Delete existing tuple (by any unique index). Return 'false' on error.
	uint
	getSpaceId(const QString &spaceName);
	QString
	getSpaceName(const uint spaceId);
	qlonglong
	getSpaceLength(const QString &spaceName);
	qlonglong
	getSpaceSize(const QString &spaceName);
	const QVariantList &
	spaces(); // all spaces and their attributes

// *** Index ***
	bool
	createIndex(const QString &spaceName, const QString &indexName, const Parts &parts, const QStringList &options ={});
	bool
	isIndexExist(const QString &spaceName, const QString &indexName);
	bool
	deleteIndex(const QString &spaceName, const QString &indexName);
	const QUIntMap
	indexes(); // map of all indexes for all spaces and their attributes

// *** Service ***
	const REPLY &
	call(const QString &function, const QVariantList &args ={});
	const REPLY &
	exec(const QString &script, const QVariantList &args ={});
	const REPLY &
	execSQL(const QString &query, const QVariantList &args ={}, const QVariantList &options ={});

// ...
	const ERROR &
	getLastError() { return(lasterror); }

	int exec() =delete; // hide parent <exec> method

private:
	QAbstractSocket *socket;
	QString version; // server version
	QByteArray salt; // session salt
	bool bInit =false;
	quint64 syncId; // IPROTO syncId
	ERROR lasterror;
	QString UserName ="";
	REPLY Reply;
	const QVariantList LISTNULL ={};
	const QVariantMap MAPNULL ={};
	const QVariant VARNULL ={};

	const REPLY &
	sendRequest(QUIntMap &header, const QUIntMap &body);
	qint64
	send(const QByteArray &data);
	inline void
	setLastError(const ERROR &msg) {
		lasterror =msg;
		emit error(msg);
	}
	/***************************************************************
	* [QT-NOTE]
	* waitForReadyRead(...) - This function may fail randomly on Windows.
	* Consider using the event loop and the readyRead() signal
	* if your software will run on Windows.
	* [Win-QT-BUG] https://bugreports.qt.io/browse/QTBUG-24451
	***************************************************************/
	inline bool
	waitForSocketRead(int timeout) {
	#if !defined(Q_OS_WIN) /* Linux & Co */
		return(socket->waitForReadyRead(timeout));
	#else				/* Windows */
		return(waitFor(socket, SIGNAL(readyRead()), timeout));
	#endif
	}
	static bool
	waitFor(const QObject *object, const char *signal, int timeout);

#pragma pack(1)

	struct HDR_DATA_SIZE
	{
		HDR_DATA_SIZE() :
			/* init */ mp_hdr(0xCE), data_size(0) { /* constructor */ }

		quint8 mp_hdr;
		quint32_be data_size;
	};

#pragma pack()

	enum {

		IPROTO_STATUS =0,
		ERROR_STRING  =IPROTO_ERROR_24
	};

private slots:
	void on_SocketConnected();
	void on_SocketDisconnected();
	void on_SocketError(QAbstractSocket::SocketError error);

signals:
	void signalConnected(const bool bConnected);
	void error(const ERROR &msg);
};

}
