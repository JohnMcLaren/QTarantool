/************************************************************************************************
 *
 * src: https://github.com/JohnMcLaren/QVariantModelWidget
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.08.2022 🄯 JML
************************************************************************************************/
#pragma once

#include <QAbstractItemModel>
#include <QDebug>

#define OUT
#define IN_OUT

#ifndef QUINTMAP_QUINTHASH
#define QUINTMAP_QUINTHASH

typedef QMap<quint64, QVariant> QUIntMap;
typedef QHash<quint64, QVariant> QUIntHash;

Q_DECLARE_METATYPE(QUIntMap)
Q_DECLARE_METATYPE(QUIntHash)

#endif

/************************************************************************************************
 *									Check for QVariant node type
 ***********************************************************************************************/
	inline static bool
	isInteger(const QVariant &value) { return(value.type() <= QVariant::ULongLong && value.type() >= QVariant::Int); }
	inline static bool
	isString(const QVariant &value) { return(value.type() == QVariant::String); }
	inline static bool
	isList(const QVariant &node) { return(node.type() == QVariant::List); }
	inline static bool
	isMap(const QVariant &node) { return(node.type() == QVariant::Map); }
	inline static bool
	isUIntMap(const QVariant &node) { return(node.userType() == qMetaTypeId<QUIntMap>()); }
	inline static bool
	isHash(const QVariant &node) { return(node.type() == QVariant::Hash); }
	inline static bool
	isUIntHash(const QVariant &node) { return(node.userType() == qMetaTypeId<QUIntHash>()); }
	inline static bool
	isListType(const QVariant &node) { return(node.canConvert(QVariant::List)); }
	inline static bool
	isMapType(const QVariant &node) { return(isMap(node)|| isUIntMap(node)); }
	inline static bool
	isListOrMapType(const QVariant &node) { return(isListType(node) || isMapType(node)); }
	inline static bool
	isHashType(const QVariant &node) { return(isHash(node) || isUIntHash(node)); }
	inline static bool
	isNode(const QVariant &node)
	{
		if(isListType(node)) // QList<any>
			return(true);

		switch((int)node.type())
		{
		case QVariant::Map:
		case QVariant::Hash:

			return(true);

		case QVariant::UserType:
			if(isUIntMap(node) || isUIntHash(node))
				return(true);
		}

	return(false);
	}
	// Return length (count childs) of Node
	inline static quint64
	lengthList(const QVariant &list) { return(reinterpret_cast<const QVariantList &>(list).size()); } // int ??
	inline static quint64
	lengthNode(const QVariant &node)
	{
		if(isListType(node)) //  List<any>
			return(lengthList(node));
		else // Map / UIntMap
		if(isMapType(node))
			return(reinterpret_cast<const QVariantMap &>(node).size()); // int ?
		else // Hash / UIntHash
		if(isHashType(node))
			return(reinterpret_cast<const QVariantHash &>(node).size()); // int ?

	return(0);
	}
	inline static QVariant &
	formatKeyName(QVariant &key, bool bIsNode =false)
	{
		if(isString(key))
			key =key.toString().prepend('"').append('"');

		if(bIsNode)
			key =key.toString().prepend('[').append(']');

	return(key);
	}
/************************************************************************************************
 *											Index
************************************************************************************************/
typedef class QVariantModelIndex : public QList<QVariant>
{
public:
	inline
	QVariantModelIndex() : QList<QVariant>() { }
	inline
	QVariantModelIndex(std::initializer_list<QVariant> args) : QList<QVariant>(args) { }
	 // index to string
	inline const QString
	text() const
	{
	QString s;

		for(const auto &l : *this)
			if(l.type() == QVariant::String)
				s +=QString("\"%1\"").arg(l.toString()).append(':');
			else
				s +=l.toString().append(':');

	return(s.left(s.size() - 1)); // remove last ':'
	}
	// truncate index
	inline void
	truncate(const uint newLength)
	{
		if(newLength < size())
			erase(begin() + newLength, end());
		else
			return;
	}
	// return parent index
	inline const QVariantModelIndex
	parent() const { auto tmp(*this); tmp.removeLast(); return(tmp); }
	// append key to index
	inline const QVariantModelIndex &
	appendKeyByPosition(const QVariant &node, const int position)
	{
		this->push_back({});
		keyOfIndex(node, position, this->last());

	return(*this);
	}
	// check keys for equal
	inline bool
	beginWith(const QVariantModelIndex &index) const
	{
		if(index.size() > this->size())
			return(false);

	auto comp =index.begin();

		for(const auto &key : *this)
			if(comp == index.end())
				break; // end comparsion
			else
			if(key != comp.i->t()) // compare keys for equal
				return(false);
			else
				comp++;

	return(true); // The start of 'this' index is exactly the same as the 'index'.
	}
	//
	inline bool
	isParentOf(const QVariantModelIndex &index) const
	{
		return(index.beginWith(*this));
	}
	// find index of key by name in node
	static quint64
	indexOfKey(const QVariant &node, const QVariant &key);
	static void
	keyOfIndex(const QVariant &node, const quint64 index, OUT QVariant &key);

	// [NOTE] Overloading a comparison operator to fix a Qt bug with QVariant(QString) string comparisons
	inline bool
	operator< (const QVariantModelIndex &index) const
	{
	auto comp =index.begin();
		// iterate over the index as long as the keys are equal
		for(const auto &key : *this)
			if(comp == index.end())
				return(false); // 'this' is longer than 'index'
			else
			if(key != comp.i->t()) // compare keys if it are not equal
				if(isString(key))// [QT-BUG] compares QVariant(QString) are case INSENSITIVE ! (i.e. "A" > "_")
					if(isInteger(comp.i->t()))
						return(false); // The integer key is lower than the string key.
					else
						return(reinterpret_cast<const QString &>(key) < reinterpret_cast<const QString &>(comp.i->t()));
				else
					if(isString(comp.i->t()))
						return(true); // The string key is greater than the integer key.
					else
						return(key < comp.i->t());
			else
				comp++; // keys equal - go to next key

	return(comp == index.end() ? false : true); // 'this' equal OR is shorter(less) than 'index'
	}

	inline bool
	operator> (const QVariantModelIndex &index) const { return(index < *this); }

	inline const QVariantModelIndex
	operator+ (const QString &key) const { QVariantModelIndex tmp(*this); tmp +=key; return(tmp); }

	inline const QVariantModelIndex
	operator+ (const quint64 &key) const { QVariantModelIndex tmp(*this); tmp +=key; return(tmp); }

private:
	template<typename T_Map, typename T_Key>
	inline static quint64
	_indexOfKey(const T_Map &container, const T_Key &key)
	{
	const auto it =container.constFind(key);

		if(it != container.constEnd())
			return(std::distance(container.constBegin(), it));

	return(-1);
	}

	template<typename T_Map>
	inline static void
	_keyOfIndex(const T_Map &container, const quint64 index, OUT QVariant &key)
	{
		if(index < lengthNode(reinterpret_cast<const QVariant &>(container)))
			key =(container.constBegin() + index).key();
	}

} QVMI;

Q_DECLARE_METATYPE(QVariantModelIndex)
/************************************************************************************************
 *											Model
************************************************************************************************/
typedef class QVariantModel : public QAbstractItemModel
{
	Q_OBJECT

	friend class QVariantTreeWidget;
	friend class QVariantTableWidget;
	/*-------------------------------------------------------
	 * Acceptable data Node types
	 *
	 * QList<any>	: 'key' is integer, 'data' is <any> type
	 * QVariantList	: 'key' is integer, 'data' is QVariant
	 * QVariantMap	: 'key' is string, 'data' is QVariant
	 * QUIntMap		: 'key' is integer, 'data' is QVariant
	 * QVariantHash	: 'key' is string, 'data' is QVariant
	 * QUIntHash	: 'key' is integer, 'data' is QVariant
	//-----------------------------------------------------*/
public:
	explicit QVariantModel(QObject *parent =nullptr);
	QVariantModel(const QVariant &rootValue, QObject *parent =nullptr);

	// Set reference to data root object -> data source cannot be temporary.
	bool
	setDataSource(QVariant &dataRoot);
	// Return ref (exlude QList<any> case) to Node/Node value in data source by VariantModelIndex
	inline const QVariant &
	data(const QVariantModelIndex &index, OUT QVariant * const pKey =nullptr, const bool bReturnParent =false, QModelIndex * const pModelIndex =nullptr) const
	{
		return(RootDataSource ? data(*RootDataSource, index, pKey, bReturnParent, pModelIndex) : NULLVAR); // *RootDataSource
	}
	// Set new data value by VarModelIndex
	bool
	setData(const QVariantModelIndex &index, const QVariant value);
	// Inserts a new key/node with data.
	bool
	insertData(const QVariantModelIndex &index, const QVariant &value);
	// Removes the existing key/node along with the data.
	bool
	removeData(const QVariantModelIndex &index);
	inline void
	setHeaders(const QStringList &headers)
	{
		this->headers =headers;
		emit updateColumns();
	}
	// Check for a valid VariantModelIndex
	inline bool
	isExist(const QVariantModelIndex &index) const // Searches by node indexes or node values.
	{
		return(data(index) != NULLVAR);
	}
	inline bool
	isExistNode(const QVariantModelIndex &index) const // Searches only for NodeIndexes.
	{
		return((bool)getNodeIndex(index));
	}
	//
	inline void
	setReadOnly(const bool readonly) { ReadOnly =readonly; }
	// Cross-connection of change signals between models.
	bool
	connectToChanges(const QVariantModel *anotherModel) const;

	enum NodeIndexAction {

		NoAction     =0,
		NodeInserted =1,
		NodeRemoved  =2,
		NodeChanged  =4
	};
	/*-----------------------------------------------------------
		*** Kept for compatibility with item based widgets ***
	-----------------------------------------------------------*/
	QVariant data(const QModelIndex &index, int role) const override; // return data by ModelIndex (used only by 'item' widgets)
	bool setData(const QModelIndex &modelIndex, const QVariant &value, int role = Qt::EditRole) override; // set data by ModelIndex
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	// [NOTE] The 'header' is an element of decoration of the model's appearance ('View' of model),
	//        why it is defined in the model itself is unclear.
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
	{
		if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
			if(section < headers.size())
				return(headers[section]);

	return(QVariant());
	}
//..................................................... Debug
	void
	debugPrintNodeIndexes() const
	{
		qDebug() << "TOTAL NODEINDEXES:" << Nodes.size();

		for(int c =0; c < Nodes.size(); c++)
			qDebug("%d(0x%lx) [%ls] >>> (parent: 0x%lx) ExistenceTag: %u IsList: %d", c, &Nodes[c],
																						Nodes[c].index.text().utf16(),
																						Nodes[c].parent,
																						Nodes[c].ExistenceTag,
																						Nodes[c].IsList);
	}
	QString
	debugPrintModelIndex(const QModelIndex &index) const
	{
		return(QString("0x%1:[%2] - [%3, %4]").arg((uintptr_t)index.internalPointer(), 2, 16)
											.arg(index.internalPointer() ? ((NodeIndex *)index.internalPointer())->index.text() : "NULL")
											.arg(index.row())
											.arg(index.column()));
	}
	void
	debugPrintModelIndexList(const QModelIndexList &list)
	{
		for(int c =0; c < list.size(); c++)
			qDebug("%d - %ls", c, debugPrintModelIndex(list[c]).utf16());
	}
	void
	debugPrintPersistentModelIndexes()
	{
	const auto list =persistentIndexList();

		qDebug() << "TOTAL PERSISTENT INDEXES:" << list.size();
		debugPrintModelIndexList(list);
	}

public slots:

	inline void
	updateView(const QVariantModelIndex &index)
	{
		updateView(index, NoAction);
	}

private slots:

	void
	changeNodeIndex(const QVariantModelIndex &index, const NodeIndexAction action);

public:
	// This structure is only needed for model compatibility with the 'QModelIndex' view widgets. [QModelIndex legacy]
	struct NodeIndex
	{
		const NodeIndex *parent;
		QVariantModelIndex index;
		bool IsList;
		uint ExistenceTag; // When attempting to create existing nodes, they are marked with this marker 'ExistenceTag'.
							// All new nodes created are marked with 'NewNodeTag'.
	};
	// Checks the value for validity as the data may contain a normal 'QVariant' value in the 'Invalid' state.
	inline static bool
	isValid(const QVariant &value)
	{
		if(&value == &NULLVAR)
			return(false);

	return(true);
	}

private:
	// Return data by index (consistent position in node)
	const QVariant &
	data(const QVariant &node,
		 const uint position,
		 OUT QVariant * const pKey =nullptr) const;
	// Return data by QVariantModelIndex
	const QVariant &
	data(const QVariant &node,
		 const QVariantModelIndex &index,
		 OUT QVariant * const pKey =nullptr,
		 const bool bReturnParent =false,
		 OUT QModelIndex * const pModelIndex =nullptr) const;
	// Removes the existing key of any node along with the data.
	static bool
	removeKey(QVariant &node, const QVariant &key);
	// Inserts new key with value to any node.
	static bool
	insertKey(QVariant &node, const QVariant &key, const QVariant &value);

	// [QModelIndex legacy]
	enum ViewMode {
		Tree =0,
		Table
	};
	// New ID update view event
	inline void
	updateView(const QVariantModelIndex &index, const NodeIndexAction action)
	{
		UpdateEventID++;
		changeNodeIndex(index, action);
	}
	// Converts VariantModelIndex to ModelIndex(-es)
	const QVariant &
	convertVMItoMI(const QVariantModelIndex &index, OUT QModelIndex * const pTopLeft, OUT QVariant * const pKey, const bool bIsTableCell =false)
	{
	const QVariant &nodeValue = bIsTableCell ? data(index, pKey, true, pTopLeft) :
												data(index, pKey, false, pTopLeft);

		if(pTopLeft && pKey)
			if(bIsTableCell)
				*pTopLeft =pTopLeft->siblingAtColumn(QVMI::indexOfKey(nodeValue, *pKey));
			else
			if(Mode == Tree)
				*pTopLeft =pTopLeft->siblingAtColumn((bool)(columns - 1));

	return(nodeValue);
	}
	//
	inline void
	removePersistentModelIndexesFor(const QVariantModelIndex &index)
	{
		for(const auto &persistentIndex : persistentIndexList())
			if(persistentIndex.internalPointer() && ((NodeIndex *)persistentIndex.internalPointer())->index.beginWith(index)) // skip 'index' AND childs indexes
				changePersistentIndex(persistentIndex, QModelIndex());
	}
	// Create/Modify NodeIndex list (Nodes)
	int
	createNodeIndex(const QVariantModelIndex &index, const QVariant &data, OUT int * const pNodeIndexPosition =nullptr, int position =0, const NodeIndex *pParentNodeIndex =nullptr);
	int
	createNodeIndex(QVariantModelIndex &index, const QVariant &data, int position, const NodeIndex *pParentNodeIndex); // recursive
	int
	insertNodeIndex(const QVariantModelIndex &index, const QVariant &data, const bool bFixChildList);
	int
	removeNodeIndex(const QVariantModelIndex &index, const bool bFixChildList);
	inline void
	resetNodeIndex()
	{
		removePersistentModelIndexesFor({});
		beginRemoveRows(QModelIndex(), 0, -1);
		endRemoveRows();
		Nodes ={NodeIndex {}};
	}
	// Return index position for new insert in NodeIndex list.
	inline int
	getNewNodeIndexPosition(const QVariantModelIndex &index, OUT const NodeIndex **parentNodeIndex =nullptr) const
	{
	int i =std::distance(Nodes.begin(), getNewNodeIndexIterator(index));

		if(parentNodeIndex)
		{
			*parentNodeIndex = &Nodes[i - 1]; // Taking the address of a brother node or parent (if we are lucky)

			while(*parentNodeIndex && (*parentNodeIndex)->index.size() >= index.size()) // Unwound upwards, towards the common parent.
				*parentNodeIndex =(*parentNodeIndex)->parent;
		}

	return(i);
	}

	inline QList<NodeIndex>::const_iterator
	getNewNodeIndexIterator(const QVariantModelIndex &index) const
	{
		return(std::upper_bound(Nodes.begin(),
								Nodes.end(),
								index,
								[](const QVariantModelIndex &_index, const NodeIndex &_node) { return(_node.index > _index); }));
	}
	// Returns NodeIndex address by VarModelIndex
	inline const NodeIndex *
	getNodeIndex(const QVariantModelIndex &index) const
	{
	const auto &it =std::lower_bound(Nodes.begin(),
									  Nodes.end(),
									  index,
									  [](const NodeIndex &_node, const QVariantModelIndex &_index) { return(_node.index < _index); });

	return((it != Nodes.end() && it->index == index) ? &it.i->t() : nullptr);
	}
	// Find address NodeIndex in Nodes by position in parent node (i.e. returns a child NodeIndex)
	inline const NodeIndex *
	getNodeIndex(const QVariantModelIndex &parentIndex, const int i) const
	{
	QVariantModelIndex index =parentIndex;

		return(getNodeIndex(index.appendKeyByPosition(data(index), i)));
	}
	// returns parent NodeIndex
	inline const NodeIndex *
	getParentNodeIndex(const QVariantModelIndex &index) const
	{
	QVariantModelIndex parentIndex =index;

		if(index.size())
			parentIndex.removeLast();
		else
			return(nullptr); // {} is root

	return(getNodeIndex(parentIndex));
	}
//************************************************************************ Variables
	QVariant *RootDataSource = &_data;		// Data Root pointer
	QList<NodeIndex> Nodes;	// List of NodeIndex - contains only nodes. [QModelIndex legacy]

	// [FIXME] Headers / Columns / ViewMode (Tree/Table) are 'View' attributes, not models, and should be in the widget.
	QStringList headers;	// Headers are stored in the model because they are requested from the model.
	int columns =0;			// [QModelIndex legacy] - The number of columns only makes sense in the context of the 'View' and is set by the widget.
	ViewMode Mode =Tree;	// View mode of the widget associated with the model.
	QVariantModelIndex RootViewIndex;
	bool ReadOnly =true;
	Qt::AlignmentFlag AlignmentTable =Qt::AlignVCenter; // Qt::AlignCenter

	static uint UpdateEventID;		// Global identifier for model data update event (all models of all ViewModes)
	uint LastUpdateEventID;
	uint OldNodeTag;				// Stores the current value of the existence tag.
	uint NewNodeTag;				// Stores the current value of the new node tag.
	static const QVariant NULLVAR;	// for 'invalid' reference in static functions
	mutable QVariant TEMPVAR;		// for return reference List<any> Node case
	// The data can be stored in the model itself to '_data'. The data source can be changed by 'setDataSource(...)'.
	// If an external data source is set, the data in '_data' will be erased.
	QVariant _data;
//************************************************************************ Helper functions
	// Return reference to node value OR to NULLVAR if key is not exist.
	template<typename T_Node, typename T_Key>
	inline static QVariant &
	_valueOfNode(T_Node &node, const T_Key &key)
	{
	const auto it =node.constFind(key);

	return(it != node.constEnd() ? const_cast<QVariant &>(it.value()) : const_cast<QVariant &>(NULLVAR) ={});
	}
//************************************************************************ Hardcore stuff
	// Get data by index from list of <any> list type
	static const QVariant // (data copy)
	getListValue(const QVariant &list, const uint index);
	// Set data by index to list of <any> list type
	static bool
	setListValue(QVariant &list, const uint index, const QVariant &value);
	// Remove exist index of list of <any> list type
	static bool
	removeListValue(QVariant &list, const uint index);
	// Insert new index in list of <any> list type
	static bool
	insertListValue(QVariant &list, const uint index, const QVariant &value);

	static void *GetList$JumpTable[], *SetList$JumpTable[], *RemoveList$JumpTable[], *InsertList$JumpTable[];
	static uint MinUserTypeId;

signals:
	void updateColumns(const int number =0);
	void dataChanged(const QVariantModelIndex &index, const NodeIndexAction action);
	void dataWillBeChanged(const QVariantModelIndex &index, const QVariant &data, OUT bool &AllowDataChanges);

} QVM;

	inline static bool
	isValid(const QVariant &value) { return(QVM::isValid(value)); }
