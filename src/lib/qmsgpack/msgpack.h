/*********************************************************************
 *
 * origin-src: https://github.com/romixlab/qmsgpack
*********************************************************************/
#ifndef MSGPACK_H
#define MSGPACK_H

#include <QByteArray>
#include <QVariantList>
#include "msgpack_export.h"
#include "msgpackcommon.h"

namespace MsgPack
{
    MSGPACK_EXPORT QVariant unpack(const QByteArray &data);
    MSGPACK_EXPORT bool registerUnpacker(qint8 msgpackType, unpack_user_f unpacker);
    MSGPACK_EXPORT QByteArray pack(const QVariant &variant);
    MSGPACK_EXPORT bool registerPacker(int qType, qint8 msgpackType, pack_user_f packer);
    MSGPACK_EXPORT qint8 msgpackType(int qType);
    MSGPACK_EXPORT bool registerType(QMetaType::Type qType, quint8 msgpackType);
    MSGPACK_EXPORT void setCompatibilityModeEnabled(bool enabled);

	template<class T>
	MSGPACK_EXPORT QByteArray pack(const T &variant) { return(pack(QVariant::fromValue(variant))); };
} // MsgPack

#endif // MSGPACK_H
