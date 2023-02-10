/**********************************************
 * Extra types declaration
**********************************************/
#include <QVariant>
#include <QDebug>

#ifndef QUINTMAP_QUINTHASH
#define QUINTMAP_QUINTHASH

typedef QMap<quint64, QVariant> QUIntMap;
typedef QHash<quint64, QVariant> QUIntHash;

Q_DECLARE_METATYPE(QUIntMap)
Q_DECLARE_METATYPE(QUIntHash)

//*
//template <class Key, class T>
inline QDebug operator<< (QDebug dbg, const QUIntMap &map)
{
QDebugStateSaver saver(dbg);

	dbg.nospace() << "QUIntMap(";

	for(typename QUIntMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
		dbg << "<[" << it.key() << "]: " << it.value() << ">";

	dbg << ')';

return(dbg);
}
//*/
#endif
