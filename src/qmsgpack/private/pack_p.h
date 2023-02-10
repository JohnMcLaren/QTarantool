#include <QHash>
#include <QMetaType>
#include <QReadWriteLock>
#include "../msgpackcommon.h"

#ifndef PACK_P_H
#define PACK_P_H

class QByteArray;
class QString;

namespace MsgPackPrivate {
/* if wr (write) == false, packer just moves pointer forward
 *
 */
typedef struct
{
    MsgPack::pack_user_f packer;
    qint8 type;

} packer_t;

bool register_packer(int q_type, qint8 msgpack_type, MsgPack::pack_user_f packer);
qint8 msgpack_type(QMetaType::Type q_type);
extern QHash<int, packer_t> user_packers;
extern QReadWriteLock packers_lock;
extern bool compatibilityMode;

quint8 * pack(const QVariant &v, quint8 *p, bool wr, QVector<QByteArray> &user_data);
//template<class T>
//quint8 * pack(const T &v, quint8 *p, bool wr, QVector<QByteArray> &user_data) { return(pack(QVariant::fromValue(v), p, wr, user_data)); };

quint8 * pack_nil(quint8 *p, bool wr);

quint8 * pack_int(qint32 i, quint8 *p, bool wr);
quint8 * pack_uint(quint32 i, quint8 *p, bool wr);
quint8 * pack_longlong(qint64 i, quint8 *p, bool wr);
quint8 * pack_ulonglong(quint64 i, quint8 *p, bool wr);

quint8 * pack_bool(const QVariant &v, quint8 *p, bool wr);

quint8 * pack_arraylen(quint32 len, quint8 *p, bool wr);
quint8 * pack_array(const QVariantList &list, quint8 *p, bool wr, QVector<QByteArray> &user_data);
quint8 * pack_stringlist(const QStringList &list, quint8 *p, bool wr);

quint8 * pack_string_raw(const char *str, quint32 len, quint8 *p, bool wr);
quint8 * pack_string(const QString &str, quint8 *p, bool wr);
quint8 * pack_float(float f, quint8 *p, bool wr);
quint8 * pack_double(double i, quint8 *p, bool wr);
quint8 * pack_bin_header(quint32 len, quint8 *p, bool wr);
quint8 * pack_bin(const QByteArray &arr, quint8 *p, bool wr);
//quint8 * pack_map(const QVariantMap &map, quint8 *p, bool wr, QVector<QByteArray> &user_data);
//quint8 * pack_map(const QUIntMap &map, quint8 *p, bool wr, QVector<QByteArray> &user_data);
quint8 * pack_user(const QVariant &v, quint8 *p, bool wr, QVector<QByteArray> &user_data);

template<class T>
quint8 * pack_map(const T &map, quint8 *p, bool wr, QVector<QByteArray> &user_data);
}

#endif // PACK_P_H
