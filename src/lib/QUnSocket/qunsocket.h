/******************************************************************
 * QUnSocket - universal Qt/C++ wrapper class for Qt sockets
 *
 * src: https://github.com/JohnMcLaren/QUnSocket
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 27.02.2023 🄯 JML
******************************************************************/
#pragma once

#include <QLocalSocket>
#include <QTcpSocket>
#include <QUdpSocket>
#ifdef QT_WEBSOCKETS_LIB // QT += websockets AND sudo apt install libqt5websockets5-dev/libqt6websockets6-dev
	#include <QWebSocket> // QWEBSOCKET_H
#endif
#include <QSslSocket>
#include <QUrl>
#include <QEventLoop>
#include <QTimer>
#include <QMetaMethod>

/****************************************************************************************
 * Thanks to whoever invented macroses
****************************************************************************************/
#ifdef QWEBSOCKET_H
	#define WEBSOCKET_CASE(action, method, args...) action(reinterpret_cast<QUnWebSocket *>(socket)->method(args));
#else
	#define WEBSOCKET_CASE(action, method, args...)
#endif

#define CALL(action, method, args...)                                         \
		switch(Proto)                                                         \
		{                                                                     \
		case UNIX:                                                            \
			action(reinterpret_cast<QUnLocalSocket *>(socket)->method(args)); \
			break;                                                            \
		case HTTP:                                                            \
			action(reinterpret_cast<QTcpSocket *>(socket)->method(args));     \
			break;                                                            \
		case HTTPS:                                                           \
			action(reinterpret_cast<QSslSocket *>(socket)->method(args));     \
			break;                                                            \
		case TCP:                                                             \
			action(reinterpret_cast<QTcpSocket *>(socket)->method(args));     \
			break;                                                            \
		case UDP:                                                             \
			action(reinterpret_cast<QUdpSocket *>(socket)->method(args));     \
			break;                                                            \
		case WS:                                                              \
		case WSS:                                                             \
			WEBSOCKET_CASE(action, method, args)                              \
			break;                                                            \
		default:                                                              \
			;                                                                 \
		}
/****************************************************************************************
 *									QUnSocket
****************************************************************************************/
#define WARN_NO_METHOD qWarning("WARNING: class '%s' has no method '%s'.", metaObject()->className(), __FUNCTION__);
#define WARN_NO_METHOD_FULL qWarning("WARNING: class '%s' has no method '%s'.", metaObject()->className(), __PRETTY_FUNCTION__);
#define NEXT_CASE [[fallthrough]];

class QUnSocket : public QAbstractSocket
{
	Q_OBJECT

public:
	QUnSocket(QObject *parent = nullptr) : QAbstractSocket(SocketType::UnknownSocketType, parent) { }
	~QUnSocket() { _close(); }

	/*** Declaring methods aliases for some sockets ***/

	class QUnLocalSocket : public QLocalSocket
	{
	public:
		explicit QUnLocalSocket(QObject *parent = nullptr) : QLocalSocket(parent) { }

		inline QHostAddress
		localAddress() const { WARN_NO_METHOD return(QHostAddress::Null); }
		inline quint16
		localPort() const { WARN_NO_METHOD return(0); }
		inline QHostAddress
		peerAddress() const { WARN_NO_METHOD return(QHostAddress::Null); }
		inline QString
		peerName() const { WARN_NO_METHOD return(0); }
		inline quint16
		peerPort() const { WARN_NO_METHOD return(0); }
		inline bool
		bind(const QHostAddress &, quint16, BindMode) { WARN_NO_METHOD return(0); }
	};

#ifdef QWEBSOCKET_H

	class QUnWebSocket : public QWebSocket
	{
		friend class QUnSocket;

	public:
		/*
		 * Since QUnWebSocket has 2 independent receive buffers (binary and text),
		 * the common method reading buffer - "read()",  is not applicable to this socket type.
		 * The reading method must explicitly point to the correct buffer.
		*/
		inline QByteArray
		read(qint64) { WARN_NO_METHOD return(0); }
		// Read binary data buffer - 1(one) binary message
		inline QByteArray
		readLine(qint64 = 0) {

			if(buffer.size())
				return(buffer.takeFirst()); // FIFO

		return(0);
		}
		// Read text data buffer - 1(one) text message
		inline QString
		readLineText(qint64 = 0) {

			if(bufferText.size())
				return(bufferText.takeFirst()); // FIFO

		return(0);
		}
		// Full read of the binary buffer.
		inline QByteArray
		readAll() { return(takeAll(buffer)); }
		// Full read of the text buffer.
		inline QString
		readAllText() { return(takeAll(bufferText)); }
		// Send a binary message.
		inline qint64
		write(const QByteArray &data) { return(sendBinaryMessage(data)); }
		// Send a text message.
		inline quint64
		write(const QString &text) { return(sendTextMessage(text)); }
		inline bool
		waitForConnected(int msecs = 30000) { return(waitFor(this, SIGNAL(connected()), msecs)); }
		inline bool
		waitForReadyRead(int) { WARN_NO_METHOD return(0); }
		inline bool
		waitForBytesWritten(int msecs = 30000) { return(waitFor(this, SIGNAL(bytesWritten(qint64)), msecs)); }
		inline bool
		waitForDisconnected(int msecs = 30000) { return(waitFor(this, SIGNAL(disconnected()), msecs)); }
		inline bool
		isSequential() const { return(true); }
		// The size of the unread data in the binary buffer at the moment.
		inline qint64
		bytesAvailable() const { return(size(buffer)); }
		// The size of the unread data in the text buffer at the moment.
		inline qint64
		bytesAvailableText() const { return(size(bufferText)); }
		// Checks if the buffer is empty
		inline bool
		canReadLine() const { return(buffer.size()); }
		// Checks if the text-buffer is empty
		inline bool
		canReadLineText() const { return(bufferText.size()); }
		// reset buffers
		inline bool
		reset() { buffer.clear(); bufferText.clear(); return(true); }
		inline bool
		open(OpenMode) { WARN_NO_METHOD_FULL return(0); }
		inline void
		open(const QUrl &url) { QWebSocket::open(url); }
		inline bool
		bind(const QHostAddress &, quint16, BindMode) { WARN_NO_METHOD return(0); }

	private:
		QList<QByteArray> buffer;  // byte-buffer
		QList<QString> bufferText; // text-buffer

		// Get all data in byte/text buffer for QByteArray or QString only
		template<typename T, typename std::enable_if<std::is_same<T, QByteArray>::value
													 || std::is_same<T, QString>::value, int>::type = 0>
		inline const T
		takeAll(QList<T> &buffer)
		{
		T t;
		auto _end =buffer.begin();

			while(_end != buffer.end())
				t += *std::move(_end++); // post-increment

			buffer.erase(buffer.begin(), _end);

		return(t);
		}
		// Get total byte/text buffer data size for QByteArray or QString only
		template<typename T, typename std::enable_if<std::is_same<T, QByteArray>::value
													 || std::is_same<T, QString>::value, int>::type = 0>
		inline uint
		size(const QList<T> &buffer) const
		{
		uint sum =0;

			for(const auto& d : buffer)
				sum +=d.size();

		return(sum);
		}
	};

#endif /* QWEBSOCKET_H */

	/*** Basic methods ***/

	inline bool
	connectToServer(const QUrl &url, const int timeout =0)
	{
		try
		{
			close();
			Proto =getProto(url);

			switch(Proto)
			{
			default:	// check port - TCP, UDP
				if(url.port() == (-1))
					throw(0);
				NEXT_CASE
			case HTTP:	// check host (default ports: 80/443)
			case HTTPS:
			case WS:
			case WSS:
				if(url.host().isEmpty())
					throw(0);
				NEXT_CASE
			case UNIX:	// check valid path
				if(!url.isValid())
					throw(0);
			}

			switch(Proto)
			{
			case UNIX:
				socket = new QUnLocalSocket(this);
				proxySignals<QUnLocalSocket>();
				reinterpret_cast<QUnLocalSocket *>(socket)->connectToServer(url.path());
				break;
			case HTTP:
				socket = new QTcpSocket(this);
				proxySignals<QTcpSocket>();
				reinterpret_cast<QTcpSocket *>(socket)->connectToHost(url.host(), url.port() == (-1) ? 80 : url.port());
				break;
			case HTTPS:
				socket = new QSslSocket(this);
				proxySignals<QSslSocket>();
				reinterpret_cast<QSslSocket *>(socket)->connectToHostEncrypted(url.host(), url.port() == (-1) ? 443 : url.port());
				break;
			case TCP:
				socket = new QTcpSocket(this);
				proxySignals<QTcpSocket>();
				reinterpret_cast<QTcpSocket *>(socket)->connectToHost(url.host(), url.port());
				break;
			case UDP:
				socket = new QUdpSocket(this);
				proxySignals<QUdpSocket>();
				reinterpret_cast<QUdpSocket *>(socket)->connectToHost(url.host(), url.port());
				break;
#ifdef QWEBSOCKET_H
			case WS:
			case WSS:
				socket = new QUnWebSocket;
				proxySignals<QUnWebSocket>();
				reinterpret_cast<QUnWebSocket *>(socket)->open(url);
				break;
#endif
			default:
				throw(0);
			}

			if(timeout)
				return(waitForConnected(timeout));

		return(true);
		}
		catch(int)
		{
			close();
		}

	return(false);
	}
	inline void
	disconnectFromServer() { close(); }
	inline void
	disconnectFromHost() override { disconnectFromServer(); }

	inline QByteArray
	read(qint64 maxlen) { CALL(return, read, maxlen); return(0); }
	inline QByteArray
	readLine(qint64 maxlen = 0) { CALL(return, readLine, maxlen); return(0); }
	inline QByteArray
	readAll() { CALL(return, readAll); return(0); }
	inline QString
	readAllText() {

#ifdef QWEBSOCKET_H
		if(Proto >= PROTO::WS) // QUnWebSocket only
			return(reinterpret_cast<QUnWebSocket *>(socket)->readAllText());
#endif
		WARN_NO_METHOD

	return(0);
	}
	inline qint64
	write(const QByteArray &data) { CALL(return, write, data); return(0); }
	inline quint64
	write(const QString &text) {

		if(Proto < PROTO::WS) // All exclude QUnWebSocket
			CALL(return, write, text.toUtf8())
#ifdef QWEBSOCKET_H
		else // QUnWebSocket only
			return(reinterpret_cast<QUnWebSocket *>(socket)->write(text));
#endif
	return(0);
	}

	inline bool
	waitForConnected(int msecs = 30000) override { CALL(return, waitForConnected, msecs); return(0); }
	/***************************************************************
	* [QT-NOTE]
	* waitForReadyRead(...) - This function may fail randomly on Windows.
	* Consider using the event loop and the readyRead() signal
	* if your software will run on Windows.
	* [Win-QT-BUG] https://bugreports.qt.io/browse/QTBUG-24451
	***************************************************************/
	inline bool
	waitForReadyRead(int msecs = 30000) override {
#if !defined(Q_OS_WIN) /* Linux & Co */
		if(Proto < PROTO::WS) // All exclude QUnWebSocket
		{
		bool res =false;

			++ReadyReadReceivers;
			CALL(res =, waitForReadyRead, msecs)
			--ReadyReadReceivers;

		return(res);
		}
#endif
	// WebSockets only OR Windows
	return(waitFor(this, SIGNAL(readyRead()), msecs));
	}
	inline bool
	waitForBytesWritten(int msecs = 30000) override { CALL(return, waitForBytesWritten, msecs); return(0); }
	inline bool
	waitForDisconnected(int msecs = 30000) override { CALL(return, waitForDisconnected, msecs); return(0); }

	QHostAddress
	localAddress() const { CALL(return, localAddress); return(QHostAddress::Null); }
	quint16
	localPort() const { CALL(return, localPort); return(0); }
	QHostAddress
	peerAddress() const { CALL(return, peerAddress); return(QHostAddress::Null); }
	QString
	peerName() const { CALL(return, peerName); return(0); }
	quint16
	peerPort() const { CALL(return, peerPort); return(0); }
	inline bool
	isSequential() const override { CALL(return, isSequential); return(0); }
	inline qint64
	bytesAvailable() const override { CALL(return, bytesAvailable); return(0); }
	inline qint64
	bytesAvailableText() const {

#ifdef QWEBSOCKET_H
		if(Proto >= PROTO::WS) // QUnWebSocket only
			return(reinterpret_cast<QUnWebSocket *>(socket)->bytesAvailableText());
#endif
		WARN_NO_METHOD

	return(0);
	}
	inline qint64
	bytesToWrite() const override { CALL(return, bytesToWrite); return(0); }
	inline bool
	canReadLine() const override { CALL(return, canReadLine); return(0); }
	inline bool
	canReadLineText() const {

#ifdef QWEBSOCKET_H
		if(Proto >= PROTO::WS) // QUnWebSocket only
			return(reinterpret_cast<QUnWebSocket *>(socket)->canReadLineText());
#endif
		WARN_NO_METHOD

	return(0);
	}
	inline int
	state() const { CALL(return, state); return(-1); }
	inline void
	abort() { CALL(, abort); }
	inline bool
	reset() override { CALL(, reset); return(0); }
	inline bool
	open(OpenMode openMode = ReadWrite) override { CALL(return, open, openMode); return(0); }
	inline bool
	bind(const QHostAddress &address, quint16 port = 0, BindMode mode = DefaultForPlatform) { CALL(return, bind, address, port, mode); return(0); }
	inline void
	close() override { _close(); }

	enum PROTO {

		none,
		UNIX,
		HTTP,
		HTTPS,	/* secured */
		TCP,
		UDP,
		WS,
		WSS		/* secured */
	};
	Q_ENUM(PROTO) // debug pretty print

	inline PROTO
	protocol() const { return(Proto); }
	inline bool
	isBuffered() const { return(ReadyReadReceivers); }
	inline bool
	isConnected() const { return(state() == ConnectedState); }

private:
	PROTO Proto =none;
	void *socket =nullptr;
	uint ReadyReadReceivers =0;

	template<typename T>
	void proxySignals()
	{	/*** Device ***/
#ifdef QWEBSOCKET_H
		// QUnWebSocket only
		if constexpr (std::is_same<T, QUnWebSocket>::value)
		{
			connect(reinterpret_cast<T *>(socket), &T::binaryMessageReceived, this, [&] (const QByteArray &data) {

				if(ReadyReadReceivers)
				{
					reinterpret_cast<QUnWebSocket *>(socket)->buffer +=data; // FIFO
					emit readyRead();
				}
				else
					emit received(data);
			});
			connect(reinterpret_cast<T *>(socket), &T::textMessageReceived, this, [&] (const QString &text) {

				if(ReadyReadReceivers)
				{
					reinterpret_cast<QUnWebSocket *>(socket)->bufferText +=text; // FIFO
					emit readyRead();
				}
				else
					emit receivedText(text);
			});
		}
		else
#endif
		// All exclude QUnWebSocket
		{
			connect(reinterpret_cast<T *>(socket), &T::readyRead, this, [&] () {

				if(ReadyReadReceivers)
					emit readyRead(); // sync/async buffered
				else
					emit received(readAll()); // async non-buffered
			});
			connect(reinterpret_cast<T *>(socket), &T::channelReadyRead, this, &QUnSocket::channelReadyRead);
			connect(reinterpret_cast<T *>(socket), &T::channelBytesWritten, this, &QUnSocket::channelBytesWritten);
		}

		connect(reinterpret_cast<T *>(socket), &T::bytesWritten, this, &QUnSocket::bytesWritten);
		connect(reinterpret_cast<T *>(socket), &T::aboutToClose, this, &QUnSocket::aboutToClose);
		connect(reinterpret_cast<T *>(socket), &T::readChannelFinished, this, &QUnSocket::readChannelFinished);

		/*** Socket ***/
		// All exclude QUnLocalSocket / QUnWebSocket
		if constexpr(!(std::is_same<T, QUnLocalSocket>::value
#ifdef QWEBSOCKET_H
						|| std::is_same<T, QUnWebSocket>::value
#endif
				))
			connect(reinterpret_cast<T *>(socket), &T::hostFound, this, &QUnSocket::hostFound);
		// All
		connect(reinterpret_cast<T *>(socket), &T::connected, this, &QUnSocket::connected);
		connect(reinterpret_cast<T *>(socket), &T::disconnected, this, [&] () {
			_reset();
			emit disconnected();
		});
		connect(reinterpret_cast<T *>(socket), &T::stateChanged, this, [&] (const int state) {
			emit stateChanged((SocketState)state);
		});

#ifdef QWEBSOCKET_H
		// QUnWebSocket only
		if constexpr(std::is_same<T, QUnWebSocket>::value)
			connect(reinterpret_cast<T *>(socket), static_cast<void(QWebSocket::*)(QAbstractSocket::SocketError)>(&T::error), this, [&] (const int error) {
				emit errorOccurred((SocketError)error);
			});
		else
#endif
		// All exclude QUnWebSocket
			connect(reinterpret_cast<T *>(socket), &T::errorOccurred, this, [&] (const int error) {
				emit errorOccurred((SocketError)error);
			});


		/*** SSL ***/
		// QSslSocket only
		if constexpr(std::is_same<T, QSslSocket>::value)
		{
			connect(reinterpret_cast<T *>(socket), &T::modeChanged, this, &QUnSocket::modeChanged);
			connect(reinterpret_cast<T *>(socket), &T::encrypted, this, &QUnSocket::encrypted);
			connect(reinterpret_cast<T *>(socket), &T::peerVerifyError, this, &QUnSocket::peerVerifyError);
			// Explicitly specify the signal type, because QSslSocket has a signal 'sslErrors' and method 'sslErrors()'
			connect(reinterpret_cast<T *>(socket), static_cast<void(T::*)(const QList<QSslError> &)>(&T::sslErrors), this, &QUnSocket::sslErrors);
		}
#ifdef QWEBSOCKET_H
		else
		// QUnWebSocket only
		if constexpr(std::is_same<T, QUnWebSocket>::value)
			connect(reinterpret_cast<T *>(socket), &T::sslErrors, this, &QUnSocket::sslErrors);
#endif
	}

	static PROTO
	getProto(const QUrl &url)
	{
		if(url.scheme() == "unix") // by url.path
			return(UNIX);
		else
		if(url.scheme() == "http") // port: 80
			return(HTTP);
		else
		if(url.scheme() == "https") // port: 443
			return(HTTPS);
		else
		if(url.scheme() == "tcp")
			return(TCP);
		else
		if(url.scheme() == "udp")
			return(UDP);
		else
		if(url.scheme() == "ws")
			return(WS);
		else
		if(url.scheme() == "wss")
			return(WSS);
		else
			throw(0);
	}
	inline void
	_close() { CALL(, close); }
	inline void
	_reset() {

		if(socket)
		{
			abort();
			CALL(, deleteLater);
			socket =nullptr;
		}

		Proto =none;
	}
	/****************************************************************************
	* [QT-NOTE]: Requires a <QApplication> instance
	****************************************************************************/
	static bool
	waitFor(const QObject *object, const char *signal, int timeout)
	{
	QEventLoop loop;
	bool bOK =true;

	   if(timeout)
		   QTimer::singleShot(timeout, &loop, [&] () {
			   bOK =false;
			   loop.quit();
		   });

	   loop.connect(object, signal, &loop, SLOT(quit())); // Qt::QueuedConnection
	   loop.exec();

	return(bOK);
	}

protected:
	void
	connectNotify(const QMetaMethod &signal) override {

		if(signal == QMetaMethod::fromSignal(&QUnSocket::readyRead))
			++ReadyReadReceivers;
	}
	void
	disconnectNotify(const QMetaMethod &signal) override {

		if(signal == QMetaMethod::fromSignal(&QUnSocket::readyRead) && ReadyReadReceivers)
			--ReadyReadReceivers;
	}

signals:
	void encrypted();
	void peerVerifyError(const QSslError &error);
	void sslErrors(const QList<QSslError> &errors);
	void modeChanged(QSslSocket::SslMode newMode);
	void received(const QByteArray &data);
	void receivedText(const QString &text);
};

