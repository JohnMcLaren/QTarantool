#ifndef COMMON_H
#define COMMON_H

#include <QVariant>
#include <QtGlobal>

#ifdef Q_OS_WINRT
#include <stdint.h>
#endif

#include "msgpack_export.h"
#include "../include/ExtMetaTypes.h"

#define MSGPACK_MAJOR 0
#define MSGPACK_MINOR 1
#define MSGPACK_VERSION 0

namespace MsgPack {
/**
 * @brief pack user type to byte array data
 * @arg variant user type, must be registered first
 * @return array with user data only, all other fields will be added automatically
 */
typedef QByteArray (*pack_user_f)(const QVariant &variant);
/**
 * @brief unpack user type to QVariant
 * @arg data only user data, without size and messagepack type
 */
typedef QVariant (*unpack_user_f)(const QByteArray &data);
/**
 * @brief version
 * @return current version
 */
MSGPACK_EXPORT QString version();
/**
 * @brief The FirstByte enum
 * From Message Pack spec
 */
namespace FirstByte
{
const quint8 POSITIVE_FIXINT = 0x7f;
const quint8 FIXMAP     = 0x80;
const quint8 FIXARRAY   = 0x90;
const quint8 FIXSTR     = 0xa0;
const quint8 NIL        = 0xc0;
const quint8 NEVER_USED = 0xc1;
const quint8 MFALSE      = 0xc2;
const quint8 MTRUE       = 0xc3;
const quint8 BIN8       = 0xc4;
const quint8 BIN16      = 0xc5;
const quint8 BIN32      = 0xc6;
const quint8 EXT8       = 0xc7;
const quint8 EXT16      = 0xc8;
const quint8 EXT32      = 0xc9;
const quint8 FLOAT32	= 0xca;
const quint8 FLOAT64	= 0xcb;
const quint8 UINT8      = 0xcc;
const quint8 UINT16 	= 0xcd;
const quint8 UINT32     = 0xce;
const quint8 UINT64 	= 0xcf;
const quint8 INT8       = 0xd0;
const quint8 INT16      = 0xd1;
const quint8 INT32      = 0xd2;
const quint8 INT64      = 0xd3;
const quint8 FIXEXT1	= 0xd4;
const quint8 FIXEXT2    = 0xd5;
const quint8 FIXEXT4    = 0xd6;
const quint8 FIXEXT8    = 0xd7;
const quint8 FIXEX16    = 0xd8;
const quint8 STR8       = 0xd9;
const quint8 STR16      = 0xda;
const quint8 STR32      = 0xdb;
const quint8 ARRAY16    = 0xdc;
const quint8 ARRAY32    = 0xdd;
const quint8 MAP16      = 0xde;
const quint8 MAP32      = 0xdf;
const quint8 NEGATIVE_FIXINT = 0xe0;
}
}
#endif // COMMON_H
