/************************************************************************************************
 *
 * src: https://github.com/JohnMcLaren/QVariantModelWidget
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.08.2022 🄯 JML
************************************************************************************************/
#include "qvariantmodel.h"
// includes for getListValue/setListValue complite class only
#include <QBitArray>
#include <QDate>
#include <QUrl>
#include <QSize>
#include <QRect>
#include <QLine>
#include <QEasingCurve>
#include <QUuid>
#include <QImage>
#include <QBitmap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCborMap>
#include <QCborArray>
/*
#include <QCursor>
#include <QColorSpace>
#include <QKeySequence>
#include <QPalette>
#include <QIcon>
#include <QMatrix4x4>
#include <QVector2D>
#include <QTextFormat>
#include <QSizePolicy>
//*/
const QVariant QVariantModel::NULLVAR; // for ref from static functions
uint QVariantModel::UpdateEventID;
/******************************************************************************************
 *
******************************************************************************************/
quint64
QVariantModelIndex::indexOfKey(const QVariant &node, const QVariant &key)
{
	if(isInteger(key))
	{
		if(isListType(node) && key.toULongLong() < lengthList(node)) // List<any>
			return(key.toULongLong());
		else
		if(isUIntMap(node))
			return(_indexOfKey(reinterpret_cast<const QUIntMap &>(node), key.toULongLong())); // UIntMap
		else
		if(isUIntHash(node))
			return(_indexOfKey(reinterpret_cast<const QUIntHash &>(node), key.toULongLong())); // UIntHash
	}
	else
	if(isString(key))
	{
		if(isMap(node))
			return(_indexOfKey(reinterpret_cast<const QVariantMap &>(node), key.toString())); // VariantMap
		else
		if(isHash(node))
			return(_indexOfKey(reinterpret_cast<const QVariantHash &>(node), key.toString())); // VariantHash
	}

return(-1);
}
/******************************************************************************************
 *
******************************************************************************************/
void
QVariantModelIndex::keyOfIndex(const QVariant &node, const quint64 index, OUT QVariant &key)
{
	// List<any>
	if(isListType(node))
		if(index < lengthList(node))
			key =index;
		else
			return;
	// Map
	if(isMap(node))
		_keyOfIndex(reinterpret_cast<const QVariantMap &>(node), index, key);
	// UIntMap
	if(isUIntMap(node))
		_keyOfIndex(reinterpret_cast<const QUIntMap &>(node), index, key);
	// Hash
	if(isHash(node))
		_keyOfIndex(reinterpret_cast<const QVariantHash &>(node), index, key);
	// UIntHash
	if(isUIntHash(node))
		_keyOfIndex(reinterpret_cast<const QUIntHash &>(node), index, key);
}
/******************************************************************************************
 * Model constructors.
******************************************************************************************/
QVariantModel::QVariantModel(QObject *parent)
	: QAbstractItemModel{parent}
{
	resetNodeIndex();
}

QVariantModel::QVariantModel(const QVariant &rootValue, QObject *parent)
	: QAbstractItemModel(parent)
{
	resetNodeIndex();
	setData(QVMI {}, rootValue);

}
/******************************************************************************************
 * The main function used by widgets to retrieve model data.
******************************************************************************************/
QVariant
QVariantModel::data(const QModelIndex &index, int role) const
{
	if(!index.internalPointer() || !columns)
		return QVariant();

//	if(role == Qt::TextAlignmentRole && Mode == Table)
//		return(AlignmentTable);

	if(role != Qt::DisplayRole && role != Qt::EditRole)
		return QVariant(); // [IMPORTANT]

QVariant key;
const QVariant &value =data(data(((NodeIndex *)index.internalPointer())->index), index.row(), &key);

	if(Mode == Tree)		/*** Tree mode ***/
	{
		// Switch the return value depending on the number of columns of the tree view.
		switch(columns)
		{
		case 1:
			return(QString("%1: %2").arg(formatKeyName(key, isNode(value)).toString()).arg(value.toString()));
		case 2:
		case 3:
			switch(index.column()) // current cell column (of tree)
			{
			case 0:
				return(formatKeyName(key, isNode(value)));
			case 2:
				return(value.typeName());
			}
		}
	}
	else					 /*** Table mode ***/
	if(Mode == Table)
	{
	const QVariant &valueCell =data(value, index.column(), &key);

		if(isNode(valueCell))
			return(formatKeyName(key, true));
		else
			return(valueCell);
	}

//return(QString("[%1,%2] 0x%3").arg(index.row()).arg(index.column()).arg((qlonglong)index.internalPointer(), 8, 16)); // DEBUG
return(value);
}
/******************************************************************************************
 *
******************************************************************************************/
const QVariant &
QVariantModel::data(const QVariant &node, const uint position, OUT QVariant * const pKey) const
{
	if(position < lengthNode(node))
		if(isListType(node))
		{
			if(pKey)
				*pKey =position;

			if(isList(node)) // List
				return(reinterpret_cast<const QVariantList &>(node)[position]);
			else			// List<any>
				TEMPVAR =getListValue(node, position); // using temp QVariant for ref to 'QVariant &' from 'any' value

		return(TEMPVAR); // This is possible because QTreeView makes a copy of the data for itself.
		}
		else
		if(isMapType(node)) // Map
		{
		const auto &it=(reinterpret_cast<const QVariantMap &>(node).begin() + position);

			if(pKey)
				if(isMap(node))
					*pKey =it.key();
				else
					*pKey =reinterpret_cast<const QUIntMap::ConstIterator &>(it).key();

		return(it.value());
		}
		else
		if(isHashType(node)) // Hash
		{
		const auto &it =(reinterpret_cast<const QVariantHash &>(node).begin() + position);

			if(pKey)
				if(isHash(node))
					*pKey =it.key();
				else
					*pKey =reinterpret_cast<const QUIntHash::ConstIterator &>(it).key();

		return(it.value());
		}

return(NULLVAR);
}
/*********************************************************************************************************
 * If the value of QList<any> is returned, the function will return a reference to the node QList<any>
 * and the 'key' of the value in this node, for subsequent processing of the result by the caller.
 * This function can calculate the ModelIndex while the VariantIndex is unwound,
 * just specify a valid receiver address 'pModelIndex'.
**********************************************************************************************************/
const QVariant &
QVariantModel::data(const QVariant &node, const QVariantModelIndex &index, OUT QVariant * const pKey, const bool bReturnParent, OUT QModelIndex * const pModelIndex) const
{
QVariant *pNode =const_cast<QVariant *>(&node);
int c =(-1), rowTree =0, rowTable =0;

	// while the index was not completely unwound.
	while(++c < (index.length() - (int)bReturnParent))
	{
	// if there is a ModellIndex calculation request
		if(pModelIndex)
			if((c + 1) == index.length()) // if 'c' is last key of index
				rowTree =QVMI::indexOfKey(*pNode, index[c]);
			else
			if((c + 2) == index.length()) // if 'c' is penultimate (parent) key of index
				rowTable =QVMI::indexOfKey(*pNode, index[c]);

	// Check 'index' key must be a integer or string
		if(isInteger(index[c]))
			// Check if the 'node' is a container of a known type.
			/******************************************************************************************
			 * [QT-NOTE] Here we cannot take constant references to nested nodes.
			 * Otherwise, due to QVariant optimization, when assigning such a reference as a new value,
			 * it will copy the reference to the object instead of copying it.
			******************************************************************************************/
			if(isList(*pNode))																					// QVariantList
				if(index[c].toULongLong() < lengthList(*pNode))
					pNode = &reinterpret_cast<QVariantList &>(*pNode)[index[c].toULongLong()];
				else
					return(NULLVAR); // index out of range list.
			else
			if(isListType(*pNode)) // QList<any> cannot contain nodes - exit in any case						// QList<any>
				if(pKey)
					break; // stop unwind index and go return ref to parent list with child key
				else
					return(NULLVAR); // Cannot return a reference to a list value because there is nowhere to save the child key name.
			else
			if(isUIntMap(*pNode))																				// QUIntMap
				pNode = &_valueOfNode(reinterpret_cast<QUIntMap &>(*pNode), index[c].toULongLong());
			else
			if(isUIntHash(*pNode))																				// QUIntHash
				pNode = &_valueOfNode(reinterpret_cast<QUIntHash &>(*pNode), index[c].toULongLong());
			else
				return(NULLVAR); // The 'node' is not a known type container for integer key
		else
		if(isString(index[c])) // if 'node' is Map or Hash
			if(isMap(*pNode))																					// QVariantMap
				pNode = &_valueOfNode(reinterpret_cast<QVariantMap &>(*pNode), reinterpret_cast<const QString &>(index[c]));
			else
			if(isHash(*pNode))																					// QVariantHash
				pNode = &_valueOfNode(reinterpret_cast<QVariantHash &>(*pNode), reinterpret_cast<const QString &>(index[c]));
			else
				return(NULLVAR);  // The 'node' is not a known type container for string key.
		else // wrong key type
			return(NULLVAR);

	// check ..
		if(!isValid(*pNode))
			return(NULLVAR); // wrong index / the key does not exist in the node
	}

	if(pKey) // if(bReturnParent)
		if((c + 1) == index.length()) // The caller is ready to accept parent node + key value, and index is unwound.
			*pKey =index[c]; // [NOTE] the list key 'index[c]' is not checked for existence
		else
			*pKey ={};

	if(pModelIndex && index.size())
		if(bReturnParent)
			*pModelIndex =createIndex(rowTable, rowTree, (void *)getParentNodeIndex(index)->parent);
		else
			*pModelIndex =createIndex(rowTree, 0, (void *)getParentNodeIndex(index));

return(*pNode); // OK. The index is unwound.
}
/******************************************************************************************
 * This overridden QAbstractItemModel method is only used by the widget.
 * It is this method that 'QAbstractItemModel' will be called when the manual editing of the value is finished.
******************************************************************************************/
bool
QVariantModel::setData(const QModelIndex &modelIndex, const QVariant &value, int role)
{
	if(!modelIndex.internalPointer())
		return(false);

	// convert ModellIndex to VariantModelIndex.
QVariantModelIndex index =((NodeIndex *)modelIndex.internalPointer())->index;
const QVariant &parentNode =data(index);

	if(!isValid(parentNode))
		return(false);

	index.appendKeyByPosition(parentNode, modelIndex.row());

	if(Mode == Table)
		index.appendKeyByPosition(data(index), modelIndex.column());

bool AllowChanges =true;

	emit dataWillBeChanged(index, value, AllowChanges);

	if(!AllowChanges)
		return(false);

return(setData(index, value));
}
/******************************************************************************************
 * This is the main method of modifying the model data.
 * The method only changes the data by the existing index (the root index '{}' always exists).
******************************************************************************************/
bool
QVariantModel::setData(const QVariantModelIndex &index, const QVariant value)
{
QVariant key, &nodeValue =const_cast<QVariant &>(data(index, &key, false)); // get reference to data value of node

	if(!isValid(nodeValue)) // 'index' not valid
		return(false);

NodeIndexAction action =NoAction;

	if(!key.isValid()) // 'nodeValue' is normal node or value (not node)
	{
		if(isNode(nodeValue) ^ isNode(value))	// 'node' ='value' OR 'value' ='node'
			if(isNode(nodeValue)) // 'node' ='value'
				if(index.size())
					action =NodeRemoved;
				else
					return(false); // A root can only be a Node but not a normal Value.
			else				  // 'value' ='node'
				action =NodeInserted;
		else
		if(isNode(nodeValue)) // 'node' ='node'
			action =(NodeIndexAction)(NodeRemoved | NodeInserted);

		nodeValue =value; // copy data by ref: 'value/node' ='value/node'
	}
	else // QList<any> special case
	if(!setListValue(nodeValue, key.toUInt(), value))
		return(false);

	updateView(index, action); // NodeChanged =0 - in all cases setData, because the number of child elements in the node does not change.

return(true);
}
/******************************************************************************************
 *
*******************************************************************************************/
bool
QVariantModel::insertData(const QVariantModelIndex &index, const QVariant &value)
{
	if(!index.size())
		return(false); // It is not possible to insert one more root node.

QVariant key, &parentNode =const_cast<QVariant &>(data(index, &key, true));

	if(!insertKey(parentNode, key, value))
		return(false);

	updateView(index, (NodeIndexAction)(NodeInserted | NodeChanged));

return(true);
}
/******************************************************************************************
 * removeData
 * Deletes data by VarIndex along with the key and fix the list of nodes.
*******************************************************************************************/
bool
QVariantModel::removeData(const QVariantModelIndex &index)
{
	if(!index.size())
		return(false); // It is not possible to remove the root node.

QVariant key, &parentNode =const_cast<QVariant &>(data(index, &key, true));

	if(!removeKey(parentNode, key))
		return(false);

	updateView(index, (NodeIndexAction)(NodeRemoved | NodeChanged));

return(true);
}
/******************************************************************************************
 * Cross-connection of change/update signals between models.
 * Models must be connected to the root of the same data. Otherwise the VariantModelIndexes in models will be different.
*******************************************************************************************/
bool
QVariantModel::connectToChanges(const QVariantModel *anotherModel) const
{
	if(anotherModel == this || !RootDataSource || RootDataSource != anotherModel->RootDataSource)
		return(false);

			// connect to signal of another model
	return(connect(anotherModel,
				   SIGNAL(dataChanged(const QVariantModelIndex &, const NodeIndexAction)),
				   SLOT(changeNodeIndex(const QVariantModelIndex &, const NodeIndexAction)),
				   Qt::UniqueConnection) &&
		   // connect another model to 'this' model signal
		   connect(this,
				   SIGNAL(dataChanged(const QVariantModelIndex &, const NodeIndexAction)),
				   anotherModel,
				   SLOT(changeNodeIndex(const QVariantModelIndex &, const NodeIndexAction)),
				   Qt::UniqueConnection));
}
/******************************************************************************************
 * Insert Key of Node
 * Inserts a new key into a node of any type with data value.
******************************************************************************************/
bool
QVariantModel::insertKey(QVariant &node, const QVariant &key, const QVariant &value)
{
	if(node.isValid())
		if(isInteger(key))
			// Check if the 'node' is a container of a known type.
			if(isList(node))																		// QVariantList
				if(key.toUInt() <= lengthList(node))
					reinterpret_cast<QVariantList &>(node).insert(key.toUInt(), value);
				else
					return(false); // index out of range list.
			else
			if(isListType(node)) //																	// QList<any>
				return(insertListValue(node, key.toUInt(), value));
			else
			if(isUIntMap(node))																		// QUIntMap
				reinterpret_cast<QUIntMap &>(node)[key.toULongLong()] =value;
			else
			if(isUIntHash(node))																	// QUIntHash
				reinterpret_cast<QUIntHash &>(node)[key.toULongLong()] =value;
			else
				return(false); // The 'node' is not a known type container for integer key
		else
		if(isString(key)) // if 'node' is Map or Hash
			if(isMap(node))																			// QVariantMap
				reinterpret_cast<QVariantMap &>(node)[key.toString()] =value;
			else
			if(isHash(node))																		// QVariantHash
				reinterpret_cast<QVariantHash &>(node)[key.toString()] =value;
			else
				return(false);  // The 'node' is not a known type container for string key.
		else // wrong index value type
			return(false);
	else // invalid variant type
		return(false);

return(true);
}
/******************************************************************************************
 * Remove Key of Node
 * Removes an existing key from a node of any type, along with the data.
******************************************************************************************/
bool
QVariantModel::removeKey(QVariant &node, const QVariant &key)
{
	if(node.isValid())
		if(isInteger(key))
			// Check if the 'node' is a container of a known type.
			if(isList(node))																		// QVariantList
				if(key.toULongLong() < lengthList(node))
					reinterpret_cast<QVariantList &>(node).removeAt(key.toUInt()); // squeeze() ??
				else
					return(false); // index out of range list.
			else
			if(isListType(node)) // QList<any> cannot contain nodes - exit in any case				// QList<any>
				return(removeListValue(node, key.toUInt()));
			else
			if(isUIntMap(node))																		// QUIntMap
				reinterpret_cast<QUIntMap &>(node).remove(key.toULongLong());
			else
			if(isUIntHash(node))																	// QUIntHash
				reinterpret_cast<QUIntHash &>(node).remove(key.toULongLong());
			else
				return(false); // The 'node' is not a known type container for integer key
		else
			if(isString(key)) // if 'node' is Map or Hash
				if(isMap(node))																		// QVariantMap
					reinterpret_cast<QVariantMap &>(node).remove(key.toString());
				else
				if(isHash(node))																	// QVariantHash
					reinterpret_cast<QVariantHash &>(node).remove(key.toString());
				else
					return(false);  // The 'node' is not a known type container for string key.
			else // wrong index type
				return(false);
	else // invalid variant type
		return(false);

return(true);
}
/******************************************************************************************
 *
******************************************************************************************/
bool
QVariantModel::setDataSource(QVariant &dataRoot) // set data root
{
	resetNodeIndex();
	_data.clear();
	RootDataSource =static_cast<QVariant *>(&dataRoot);

return(createNodeIndex({}, dataRoot));
}
/******************************************************************************************
 * Parses data and inserts node structures 'NodeIndex' into the list (Nodes).
 * Return absolut position index of list
******************************************************************************************/
int
QVariantModel::createNodeIndex(const QVariantModelIndex &index, const QVariant &data, OUT int * const pNodeIndexPosition, int position, const NodeIndex *pParentNodeIndex)
{
QVariantModelIndex _index =index;

	if(!position)
		position =getNewNodeIndexPosition(index, &pParentNodeIndex);

	if(pNodeIndexPosition)
		*pNodeIndexPosition =position;

	OldNodeTag +=2;
	NewNodeTag =(OldNodeTag + 1);

return(createNodeIndex(_index, data, position - 1, pParentNodeIndex));
}
// *** recursive ***
int
QVariantModel::createNodeIndex(QVariantModelIndex &index, const QVariant &data, int position, const NodeIndex *pParentNodeIndex)
{
	if(isNode(data)) // all allowable types of Nodes.
	{
		while(position < Nodes.size() && index > Nodes[position].index) // skip garbage nodes if exist
			position++;

		// There, position <= Nodes.size() AND index <= Nodes[position].index.
		if(position == Nodes.size() || index < Nodes[position].index) // new position OR
			Nodes.insert(position, NodeIndex {pParentNodeIndex, index, false, NewNodeTag});
		else
			Nodes[position].ExistenceTag =OldNodeTag; // Node already exists.

		pParentNodeIndex = &Nodes[position];
		position++;

		index.push_back({}); // create last key node index

		if(isList(data)) // VariantList [NOTE] QList<any> cannot contain Nodes
		{
		auto list =reinterpret_cast<const QVariantList &>(data);

			const_cast<NodeIndex *>(pParentNodeIndex)->IsList =true;

			for(auto it =list.cbegin(); it != list.cend(); it++)
			{
				index[index.size() - 1] =std::distance(list.cbegin(), it); // set last key node index
				position =createNodeIndex(index, it.i->t(), position, pParentNodeIndex);
			}
		}
		else // Map & UIntMap
		if(isMap(data))
		{
		auto map =reinterpret_cast<const QVariantMap &>(data);

			for(auto it =map.cbegin(); it != map.cend(); it++)
			{
				index[index.size() - 1] =it.key();
				position =createNodeIndex(index, it.value(), position, pParentNodeIndex);
			}
		}
		else
		if(isUIntMap(data))
		{
		auto umap =reinterpret_cast<const QUIntMap &>(data);

			for(auto it =umap.cbegin(); it != umap.cend(); it++)
			{
				index[index.size() - 1] =it.key();
				position =createNodeIndex(index, it.value(), position, pParentNodeIndex);
			}
		}
		else // Hash & UIntHash
		if(isHash(data))
		{
		auto hash =reinterpret_cast<const QVariantHash &>(data);

			for(auto it =hash.cbegin(); it != hash.cend(); it++)
			{
				index[index.size() - 1] =it.key();
				position =createNodeIndex(index, it.value(), position, pParentNodeIndex);
			}
		}
		else
		if(isUIntHash(data))
		{
		auto uhash =reinterpret_cast<const QUIntHash &>(data);

			for(auto it =uhash.cbegin(); it != uhash.cend(); it++)
			{
				index[index.size() - 1] =it.key();
				position =createNodeIndex(index, it.value(), position, pParentNodeIndex);
			}
		}

		index.pop_back(); // remove last key node index
	}

return(position);
}
/******************************************************************************************
 * Inserts node structures 'NodeIndex' to the list.
******************************************************************************************/
int
QVariantModel::insertNodeIndex(const QVariantModelIndex &index, const QVariant &data, const bool bFixChildList)
{
const NodeIndex *parent;
int position =getNewNodeIndexPosition(index, &parent);

	if(parent) // skip if null-parent
		if(parent->IsList && bFixChildList) // if parent list AND need fix indexes 'NodeIndex' after insert child node 'index'
		{
		auto it =Nodes.begin() + (position - 1);

			while(it != Nodes.end() && it->index.size() >= index.size())
			{
				it->index[index.size() - 1] =(it->index[index.size() - 1].toInt() + 1); // fix current list indexes for 1 inserted node
				it++;
			}
		}
		else
		if(Nodes[position - 1].index == index) // Node has a non enumerated key type (map/hash) OR the fix of enumerated keys is not explicitly allowed by 'bFixChildList'
			return(0); // but the key to be inserted already exists.

return(createNodeIndex(index, data, nullptr, position, parent) - position); // creates new NodeIndex structs
}
/******************************************************************************************
 * Removes node structures 'NodeIndex' from the list.
 * Returns the total number of NodIndexes removed (parent 'index' + childs).
******************************************************************************************/
int
QVariantModel::removeNodeIndex(const QVariantModelIndex &index, const bool bFixChildList)
{
	if(!index.size()) // if root index
		return(0); // It is not possible to remove the root node.

const auto &begin =std::lower_bound(Nodes.begin(),
								  Nodes.end(),
								  index,
								  [](const NodeIndex &_node, const QVariantModelIndex &_index) { return(_node.index < _index); });
	// if node found
	if(begin != Nodes.end() && begin->index.size() == index.size())
	{
	auto it =begin;
	int childs =0; // child NodeIndexes
	int LastKeyPosition =(index.size() - 1); // for fix enumerated keys
	bool bParentIslist =it->parent->IsList; // for checking the enumeration of a node's keys

		// find end of delete index of node
		while(++it != Nodes.end() && index.isParentOf(it->index))
			childs++;

		// delete all 'NodeIndex' where 'index' is parent node (include 'index')
		it =Nodes.erase(begin, it); // squeeze() ??

		if(bFixChildList && bParentIslist) // if parent list need fix indexes after delete child node 'index'
			while(it != Nodes.end() && it->index.size() > LastKeyPosition)
			{
				it->index[LastKeyPosition] =(it->index[LastKeyPosition].toInt() - 1); // fix current list indexes for 1 deleted node
				it++;
			}

	return(childs + 1);
	}

return(0);
}
/******************************************************************************************
 *
******************************************************************************************/
void
QVariantModel::changeNodeIndex(const QVariantModelIndex &index, const NodeIndexAction action)
{
	if(LastUpdateEventID == UpdateEventID)  // If event has already been processed
		return; // exit

	LastUpdateEventID =UpdateEventID;

QModelIndex topLeft; // full update view by default

	if(action) // The NodeIndex list will change only if there have been actions with nodes.
	{
	QVariant key;
	bool bIsTableCell =(Mode == Table) ? (index.size() - RootViewIndex.size() == 2 ? true : false) : false;
	bool bIsTableRow =(Mode == Table) ? (index.size() - RootViewIndex.size() == 1 ? true : false) : false;
	bool bIsTable =(Mode == Table) ? (index.size() - RootViewIndex.size() == 0 ? true : false) : false;
	bool bIsNodeToNode =((action & NodeRemoved) && (action & NodeInserted));
	bool bRepaintRows =(Mode == Tree) || bIsTableRow;
	const QVariant &nodeValue =convertVMItoMI(index, &topLeft, &key, bIsTableCell);

		// NodeIndex actions
		// Table cell & Node to Node action - special cases
		if(bIsNodeToNode)
		{
			if(index.size()) // if not root index
			{
			int position;

				createNodeIndex(index, nodeValue, &position); // from start 'position' to end of Node 'index'

				if(bRepaintRows)
				{
					beginInsertRows(topLeft.parent(), 0, -1); // Updating the view after inserting new nodes
					endInsertRows();
				}

				// Well, now let's delete the nodes that no longer exist in the new data.
				while(position < Nodes.size() && index.isParentOf(Nodes[position].index)) // walk on all childs of 'index'
					if(Nodes[position].ExistenceTag != NewNodeTag && Nodes[position].ExistenceTag != OldNodeTag) // If a node is not present in the new data.
					{
						if(bIsTable || bRepaintRows)
						{	// Notify widget about changes
							removePersistentModelIndexesFor(Nodes[position].index);

							if(bRepaintRows)
							{// [QT-NOTE] No other methods(insert*/remove*) change the appearance of the widget.
								beginRemoveRows(createIndex(0, 0, (void *)Nodes[position].parent).parent(), 0, -1);
								endRemoveRows();
							}
						}

						removeNodeIndex(Nodes[position].index, false);
					}
					else
						position++;
			}
			else // Full replacement content of the root node.
			{
				resetNodeIndex();
				createNodeIndex(index, nodeValue);
			}

			topLeft =QModelIndex(); // full update view
		}
		else
		if(action & NodeRemoved)
		{
			if(bIsTable || bRepaintRows)
			{
				removePersistentModelIndexesFor(index);

				if(bRepaintRows)
				{
					beginRemoveRows(topLeft.parent(), topLeft.row(), topLeft.row());
					endRemoveRows();
				}
			}

			removeNodeIndex(index, action & NodeChanged);
		}
		else
		if(action & NodeInserted)
		{
			insertNodeIndex(index, nodeValue, action & NodeChanged);

			if(bRepaintRows)
			{
				beginInsertRows(topLeft.parent(), topLeft.row(), topLeft.row());
				endInsertRows();
			}
		}
	}

	emit QAbstractItemModel::dataChanged(topLeft, topLeft); // signal to widget
	emit dataChanged(index, action); // signal to another model
}
/******************************************************************************************
 *						Standard QAbstractItemModel methods								  *
******************************************************************************************/
// Return child QModelIndex by [row, column] position
QModelIndex
QVariantModel::index(int row, int column, const QModelIndex &parentIndex) const
{
	if(parentIndex.isValid() && parentIndex.column() != 0) // ??
		return(QModelIndex());

	if(!parentIndex.internalPointer() && Nodes.size())
		return(createIndex(row, column, (void *)&Nodes[0]));

	if(parentIndex.internalPointer())
	{
	const NodeIndex *nodeIndex =getNodeIndex(((NodeIndex *)parentIndex.internalPointer())->index, parentIndex.row()); // child NodeIndex

		if(nodeIndex)
			return(createIndex(row, column, (void *)nodeIndex)); // create ModelIndex for child
	}

return(QModelIndex());
}
//-------------------------------------------------------------------------------------
QModelIndex
QVariantModel::parent(const QModelIndex &index) const
{
	if(index.internalPointer())
	{
	NodeIndex nodeIndex = *(NodeIndex *)index.internalPointer();

		if(nodeIndex.index.size())
		{
		QVariant key =nodeIndex.index.takeLast(); // pop last key (child) of index and make parent index

			return(createIndex(QVMI::indexOfKey(data(nodeIndex.index), key), 0, (void *)nodeIndex.parent)); // [NOTE] column always 0
		}
	}

return(QModelIndex());
}
//...............................................................................................
int
QVariantModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)

	if(!columns)
		emit const_cast<QVM *>(this)->updateColumns(); // signal to widget: columns =0

return(columns);
}
//...............................................................................................
int
QVariantModel::rowCount(const QModelIndex &parent) const
{
	if(parent.isValid() && parent.column() > 0) // Headers ?
		return(0);

	if(!parent.isValid()) // root - dataSource
		return(RootDataSource ? lengthNode(*RootDataSource) : 0);

	if(parent.internalPointer())
		return(lengthNode(data(data(((NodeIndex *)parent.internalPointer())->index), parent.row()))); // data (node, row)

return(0);
}
//...............................................................................................
Qt::ItemFlags
QVariantModel::flags(const QModelIndex &index) const
{

	if(!index.isValid())
		return(Qt::NoItemFlags);

	if(Mode == Tree)
		return(QAbstractItemModel::flags(index) | (ReadOnly ? Qt::NoItemFlags : (index.column() == (bool)(columns - 1) ? Qt::ItemIsEditable : Qt::NoItemFlags)));
	/* Table */
return(QAbstractItemModel::flags(index) | (ReadOnly ? Qt::NoItemFlags : Qt::ItemIsEditable));
}
/*******************************************************************************************************
 *											Hardcore
 * The fastest 'switch' possible.
 * The '&&' operator is only available in the GCC compiler.
 *
*******************************************************************************************************/
uint QVariantModel::MinUserTypeId =0;
void *QVariantModel::GetList$JumpTable[QMetaType::HighestInternalId + 1] ={},
*QVariantModel::SetList$JumpTable[QMetaType::HighestInternalId + 1] ={},
*QVariantModel::RemoveList$JumpTable[QMetaType::HighestInternalId + 1] ={},
*QVariantModel::InsertList$JumpTable[QMetaType::HighestInternalId + 1] ={};
//................................................................
const QVariant
QVariantModel::getListValue(const QVariant &list, const uint index)
{
	if(!GetList$JumpTable[0])
		goto init;

begin:	// User types
	if(isListType(list) && index < lengthList(list))
		if(list.type() == QVariant::UserType && list.type() <= (MinUserTypeId + QMetaType::HighestInternalId))
			goto *(GetList$JumpTable[list.userType() - MinUserTypeId]);
		else // standard types
		if(list.type() == QVariant::StringList)
			return(reinterpret_cast<const QStringList &>(list)[index]);
Unknown:
	return(QVariant());
QListBool:
	return(reinterpret_cast<const QList<bool> &>(list)[index]);
QListInt:
	return(reinterpret_cast<const QList<int> &>(list)[index]);
QListUInt:
	return(reinterpret_cast<const QList<uint> &>(list)[index]);
QListLongLong:
	return(reinterpret_cast<const QList<qlonglong> &>(list)[index]);
QListULongLong:
	return(reinterpret_cast<const QList<qulonglong> &>(list)[index]);
QListDouble:
	return(reinterpret_cast<const QList<double> &>(list)[index]);
QListQChar:
	return(reinterpret_cast<const QList<QChar> &>(list)[index]);
QListVariantMap:
	return(reinterpret_cast<const QList<QVariantMap> &>(list)[index]);
	//QListVariantList: not UserType
	//	return(reinterpret_cast<const QList<QVariantList> &>(list)[index]);
QListString:
	return(reinterpret_cast<const QList<QString> &>(list)[index]);
QListStringList:
	return(reinterpret_cast<const QList<QStringList> &>(list)[index]);
	//QListByteArray: not UserType
	//	return(reinterpret_cast<const QList<QByteArray> &>(list)[index]);
QListBitArray:
	return(reinterpret_cast<const QList<QBitArray> &>(list)[index]);
QListDate:
	return(reinterpret_cast<const QList<QDate> &>(list)[index]);
QListTime:
	return(reinterpret_cast<const QList<QTime> &>(list)[index]);
QListDateTime:
	return(reinterpret_cast<const QList<QDateTime> &>(list)[index]);
QListUrl:
	return(reinterpret_cast<const QList<QUrl> &>(list)[index]);
QListLocale:
	return(reinterpret_cast<const QList<QLocale> &>(list)[index]);
QListRect:
	return(reinterpret_cast<const QList<QRect> &>(list)[index]);
QListRectF:
	return(reinterpret_cast<const QList<QRectF> &>(list)[index]);
QListSize:
	return(reinterpret_cast<const QList<QSize> &>(list)[index]);
QListSizeF:
	return(reinterpret_cast<const QList<QSizeF> &>(list)[index]);
QListLine:
	return(reinterpret_cast<const QList<QLine> &>(list)[index]);
QListLineF:
	return(reinterpret_cast<const QList<QLineF> &>(list)[index]);
QListPoint:
	return(reinterpret_cast<const QList<QPoint> &>(list)[index]);
QListPointF:
	return(reinterpret_cast<const QList<QPointF> &>(list)[index]);
QListRegExp:
	return(reinterpret_cast<const QList<QRegExp> &>(list)[index]);
QListVariantHash:
	return(reinterpret_cast<const QList<QVariantHash> &>(list)[index]);
QListEasingCurve:
	return(reinterpret_cast<const QList<QEasingCurve> &>(list)[index]);
QListUuid:
	return(reinterpret_cast<const QList<QUuid> &>(list)[index]);
QListVoidStar:
	return(reinterpret_cast<const QList<const void *> &>(list)[index]);
QListLong:
	return(QVariant::fromValue(reinterpret_cast<const QList<long> &>(list)[index]));
QListShort:
	return(reinterpret_cast<const QList<short> &>(list)[index]);
QListChar:
	return(reinterpret_cast<const QList<char> &>(list)[index]);
QListULong:
	return(QVariant::fromValue(reinterpret_cast<const QList<ulong> &>(list)[index]));
QListUShort:
	return(reinterpret_cast<const QList<ushort> &>(list)[index]);
QListUChar:
	return(reinterpret_cast<const QList<uchar> &>(list)[index]);
QListFloat:
	return(reinterpret_cast<const QList<float> &>(list)[index]);
QListObjectStar:
	return(reinterpret_cast<const QList<const QObject *> &>(list)[index]);
QListSChar: // what is ??
	return(reinterpret_cast<const QList<char> &>(list)[index]);
//QListVariant: not UserType
//	return(reinterpret_cast<const QList<QVariant> &>(list)[index]);
QListModelIndex:
	return(reinterpret_cast<const QList<QModelIndex> &>(list)[index]);
//QListVoid:  empty result
//	return(QVariant());
QListRegularExpression:
	return(reinterpret_cast<const QList<QRegularExpression> &>(list)[index]);
QListJsonValue:
	return(reinterpret_cast<const QList<QJsonValue> &>(list)[index]);
QListJsonObject:
	return(reinterpret_cast<const QList<QJsonObject> &>(list)[index]);
QListJsonArray:
	return(reinterpret_cast<const QList<QJsonArray> &>(list)[index]);
QListJsonDocument:
	return(reinterpret_cast<const QList<QJsonDocument> &>(list)[index]);
QListByteArrayList:
	return(QVariant::fromValue(reinterpret_cast<const QList<QByteArrayList> &>(list)[index]));
QListPersistentModelIndex:
	return(reinterpret_cast<const QList<QPersistentModelIndex> &>(list)[index]);
//QListNullptr:  empty result
//	return(QVariant());
QListCborSimpleType:
	return(reinterpret_cast<const QList<quint8> &>(list)[index]);
QListCborValue:
	return(QVariant::fromValue(reinterpret_cast<const QList<QCborValue> &>(list)[index]));
QListCborArray:
	return(QVariant::fromValue(reinterpret_cast<const QList<QCborArray> &>(list)[index]));
QListCborMap:
	return(QVariant::fromValue(reinterpret_cast<const QList<QCborMap> &>(list)[index]));
// don't think it will ever come in handy.
/*
QListFont:
	return(QVariant::fromValue(reinterpret_cast<const QList<QFont> &>(list)[index]));
QListPixmap:
	return(QVariant::fromValue(reinterpret_cast<const QList<QPixmap> &>(list)[index]));
QListBrush:
	return(QVariant::fromValue(reinterpret_cast<const QList<QBrush> &>(list)[index]));
QListColor:
	return(QVariant::fromValue(reinterpret_cast<const QList<QColor> &>(list)[index]));
QListPalette:
	return(QVariant::fromValue(reinterpret_cast<const QList<QPalette> &>(list)[index]));
QListIcon:
	return(QVariant::fromValue(reinterpret_cast<const QList<QIcon> &>(list)[index]));
QListImage:
	return(QVariant::fromValue(reinterpret_cast<const QList<QImage> &>(list)[index]));
QListPolygon:
	return(QVariant::fromValue(reinterpret_cast<const QList<QPolygon> &>(list)[index]));
QListRegion:
	return(QVariant::fromValue(reinterpret_cast<const QList<QRegion> &>(list)[index]));
QListBitmap:
	return(QVariant::fromValue(reinterpret_cast<const QList<QBitmap> &>(list)[index]));
QListCursor:
	return(QVariant::fromValue(reinterpret_cast<const QList<QCursor> &>(list)[index]));
QListKeySequence:
	return(QVariant::fromValue(reinterpret_cast<const QList<QKeySequence> &>(list)[index]));
QListPen:
	return(QVariant::fromValue(reinterpret_cast<const QList<QPen> &>(list)[index]));
QListTextLength:
	return(QVariant::fromValue(reinterpret_cast<const QList<QTextLength> &>(list)[index]));
QListTextFormat:
	return(QVariant::fromValue(reinterpret_cast<const QList<QTextFormat> &>(list)[index]));
QListMatrix:
	return(QVariant::fromValue(reinterpret_cast<const QList<QMatrix> &>(list)[index]));
QListTransform:
	return(QVariant::fromValue(reinterpret_cast<const QList<QTransform> &>(list)[index]));
QListMatrix4x4:
	return(QVariant::fromValue(reinterpret_cast<const QList<QMatrix4x4> &>(list)[index]));
QListVector2D:
	return(QVariant::fromValue(reinterpret_cast<const QList<QVector2D> &>(list)[index]));
QListVector3D:
	return(QVariant::fromValue(reinterpret_cast<const QList<QVector3D> &>(list)[index]));
QListVector4D:
	return(QVariant::fromValue(reinterpret_cast<const QList<QVector4D> &>(list)[index]));
QListQuaternion:
	return(QVariant::fromValue(reinterpret_cast<const QList<QQuaternion> &>(list)[index]));
QListPolygonF:
	return(QVariant::fromValue(reinterpret_cast<const QList<QPolygonF> &>(list)[index]));
QListColorSpace:
	return(QVariant::fromValue(reinterpret_cast<const QList<QColorSpace> &>(list)[index]));
QListSizePolicy:
	return(QVariant::fromValue(reinterpret_cast<const QList<QSizePolicy> &>(list)[index]));
*/
//................................................................
init:
uint c =1;

	if(MinUserTypeId)
		c =0;

	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<bool>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<bool>>() : &&QListBool;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<int>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<int>>() : &&QListInt;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<uint>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<uint>>() : &&QListUInt;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<qlonglong>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<qlonglong>>() : &&QListLongLong;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<qulonglong>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<qulonglong>>() : &&QListULongLong;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<double>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<double>>() : &&QListDouble;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QChar>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QChar>>() : &&QListQChar;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QVariantMap>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QVariantMap>>() : &&QListVariantMap;
//	userTypeIndexes[c ? c++ : qMetaTypeId<QList<QVariantList>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QVariantList>>() : &&QListVariantList; // not UserType
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QString>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QString>>() : &&QListString;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QStringList>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QStringList>>() : &&QListStringList;
//	userTypeIndexes[c ? c++ : qMetaTypeId<QList<QByteArray>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QByteArray>>() : &&QListByteArray;  // not UserType
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QBitArray>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QBitArray>>() : &&QListBitArray;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QDate>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QDate>>() : &&QListDate;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QTime>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QTime>>() : &&QListTime;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QDateTime>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QDateTime>>() : &&QListDateTime;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QUrl>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QUrl>>() : &&QListUrl;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QLocale>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QLocale>>() : &&QListLocale;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QRect>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QRect>>() : &&QListRect;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QRectF>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QRectF>>() : &&QListRectF;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QSize>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QSize>>() : &&QListSize;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QSizeF>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QSizeF>>() : &&QListSizeF;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QLine>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QLine>>() : &&QListLine;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QLineF>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QLineF>>() : &&QListLineF;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPoint>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPoint>>() : &&QListPoint;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPointF>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPointF>>() : &&QListPointF;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QRegExp>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QRegExp>>() : &&QListRegExp;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QVariantHash>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QVariantHash>>() : &&QListVariantHash;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QEasingCurve>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QEasingCurve>>() : &&QListEasingCurve;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QUuid>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QUuid>>() : &&QListUuid;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<void *>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<void *>>() : &&QListVoidStar;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<long>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<long>>() : &&QListLong;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<short>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<short>>() : &&QListShort;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<char>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<char>>() : &&QListChar;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<ulong>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<ulong>>() : &&QListULong;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<ushort>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<ushort>>() : &&QListUShort;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<uchar>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<uchar>>() : &&QListUChar;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<float>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<float>>() : &&QListFloat;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QObject *>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QObject *>>() : &&QListObjectStar;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<char>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<char>>() : &&QListSChar;
//	userTypeIndexes[c ? c++ : qMetaTypeId<QList<QVariant>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QVariant>>() : &&QListVariant; // not UserType
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QModelIndex>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QModelIndex>>() : &&QListModelIndex;
	// &&QListVoid;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QRegularExpression>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QRegularExpression>>() : &&QListRegularExpression;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QJsonValue>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QJsonValue>>() : &&QListJsonValue;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QJsonObject>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QJsonObject>>() : &&QListJsonObject;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QJsonArray>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QJsonArray>>() : &&QListJsonArray;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QJsonDocument>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QJsonDocument>>() : &&QListJsonDocument;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QByteArrayList>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QByteArrayList>>() : &&QListByteArrayList;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPersistentModelIndex>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPersistentModelIndex>>() : &&QListPersistentModelIndex;
	// &&QListNullptr:
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<quint8>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<quint8>>() : &&QListCborSimpleType;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QCborValue>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QCborValue>>() : &&QListCborValue;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QCborArray>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QCborArray>>() : &&QListCborArray;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QCborMap>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QCborMap>>() : &&QListCborMap;
/*
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QFont>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QFont>>() : &&QListFont;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPixmap>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPixmap>>() : &&QListPixmap;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QBrush>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QBrush>>() : &&QListBrush;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QColor>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QColor>>() : &&QListColor;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPalette>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPalette>>() : &&QListPalette;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QIcon>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QIcon>>() : &&QListIcon;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QImage>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QImage>>() : &&QListImage;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPolygon>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPolygon>>() : &&QListPolygon;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QRegion>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QRegion>>() : &&QListRegion;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QBitmap>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QBitmap>>() : &&QListBitmap;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QCursor>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QCursor>>() : &&QListCursor;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QKeySequence>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QKeySequence>>() : &&QListKeySequence;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPen>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPen>>() : &&QListPen;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QTextLength>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QTextLength>>() : &&QListTextLength;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QTextFormat>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QTextFormat>>() : &&QListTextFormat;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QMatrix>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QMatrix>>() : &&QListMatrix;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QTransform>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QTransform>>() : &&QListTransform;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QMatrix4x4>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QMatrix4x4>>() : &&QListMatrix4x4;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QVector2D>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QVector2D>>() : &&QListVector2D;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QVector3D>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QVector3D>>() : &&QListVector3D;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QVector4D>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QVector4D>>() : &&QListVector4D;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QQuaternion>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QQuaternion>>() : &&QListQuaternion;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QPolygonF>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QPolygonF>>() : &&QListPolygonF;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QColorSpace>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QColorSpace>>() : &&QListColorSpace;
	GetList$JumpTable[c ? c++ : qMetaTypeId<QList<QSizePolicy>>() - MinUserTypeId] = c ? (void *)qMetaTypeId<QList<QSizePolicy>>() : &&QListSizePolicy;
//*/
	if(c) // 1 pass
	{
		// search min type Id
		for(c =1, MinUserTypeId =(-1); c < (QMetaType::HighestInternalId + 1); c++)
			if(*(uint *)&GetList$JumpTable[c] && *(uint *)&GetList$JumpTable[c] < MinUserTypeId)
				MinUserTypeId = *(uint *)&GetList$JumpTable[c];

		if(MinUserTypeId < QVariant::UserType)
		{
			MinUserTypeId =0;
			goto Unknown; // ERROR: A type incompatible with the jump table was detected.
		}

		// init array default address
		for(c =0; c < (QMetaType::HighestInternalId + 1); c++)
			GetList$JumpTable[c] = &&Unknown;

		goto init; // OK. pass 2
	}

	goto begin; // ready
}
//......................................................................................
bool
QVariantModel::setListValue(QVariant &list, const uint index, const QVariant &value)
{
	if(!SetList$JumpTable[0])
		goto init;

begin:		// User types
	if(isListType(list) && index < lengthList(list))
		if(list.type() == QVariant::UserType && list.type() <= (MinUserTypeId + QMetaType::HighestInternalId))
			goto *(SetList$JumpTable[list.userType() - MinUserTypeId]);
		else // standard types
		if(list.type() == QVariant::StringList)
		{
			reinterpret_cast<QStringList &>(list)[index] =value.toString();

		return(true);
		}

Unknown:
	return(false);

QListBool:
	reinterpret_cast<QList<bool> &>(list)[index] =value.toBool();
	return(true);
QListInt:
	reinterpret_cast<QList<int> &>(list)[index] =value.toInt();
	return(true);
QListUInt:
	reinterpret_cast<QList<uint> &>(list)[index] =value.toUInt();
	return(true);
QListLongLong:
	reinterpret_cast<QList<qlonglong> &>(list)[index] =value.toLongLong();
	return(true);
QListULongLong:
	reinterpret_cast<QList<qulonglong> &>(list)[index] =value.toULongLong();
	return(true);
QListDouble:
	reinterpret_cast<QList<double> &>(list)[index] =value.toDouble();
	return(true);
QListQChar:
	reinterpret_cast<QList<QChar> &>(list)[index] =value.toChar();
	return(true);
QListVariantMap:
	reinterpret_cast<QList<QVariantMap> &>(list)[index] =value.toMap();
	return(true);
//QListVariantList: not UserType

QListString:
	reinterpret_cast<QList<QString> &>(list)[index] =value.toString();
	return(true);
QListStringList:
	reinterpret_cast<QList<QStringList> &>(list)[index] =value.toStringList();
	return(true);
//QListByteArray: not UserType

QListBitArray:
	reinterpret_cast<QList<QBitArray> &>(list)[index] =value.toBitArray();
	return(true);
QListDate:
	reinterpret_cast<QList<QDate> &>(list)[index] =value.toDate();
	return(true);
QListTime:
	reinterpret_cast<QList<QTime> &>(list)[index] =value.toTime();
	return(true);
QListDateTime:
	reinterpret_cast<QList<QDateTime> &>(list)[index] =value.toDateTime();
	return(true);
QListUrl:
	reinterpret_cast<QList<QUrl> &>(list)[index] =value.toUrl();
	return(true);
QListLocale:
	reinterpret_cast<QList<QLocale> &>(list)[index] =value.toLocale();
	return(true);
QListRect:
	reinterpret_cast<QList<QRect> &>(list)[index] =value.toRect();
	return(true);
QListRectF:
	reinterpret_cast<QList<QRectF> &>(list)[index] =value.toRectF();
	return(true);
QListSize:
	reinterpret_cast<QList<QSize> &>(list)[index] =value.toSize();
	return(true);
QListSizeF:
	reinterpret_cast<QList<QSizeF> &>(list)[index] =value.toSizeF();
	return(true);
QListLine:
	reinterpret_cast<QList<QLine> &>(list)[index] =value.toLine();
	return(true);
QListLineF:
	reinterpret_cast<QList<QLineF> &>(list)[index] =value.toLineF();
	return(true);
QListPoint:
	reinterpret_cast<QList<QPoint> &>(list)[index] =value.toPoint();
	return(true);
QListPointF:
	reinterpret_cast<QList<QPointF> &>(list)[index] =value.toPointF();
	return(true);
QListRegExp:
	reinterpret_cast<QList<QRegExp> &>(list)[index] =value.toRegExp();
	return(true);
QListVariantHash:
	reinterpret_cast<QList<QVariantHash> &>(list)[index] =value.toHash();
	return(true);
QListEasingCurve:
	reinterpret_cast<QList<QEasingCurve> &>(list)[index] =value.toEasingCurve();
	return(true);
QListUuid:
	reinterpret_cast<QList<QUuid> &>(list)[index] =value.toUuid();
	return(true);
QListVoidStar:
	reinterpret_cast<QList<void *> &>(list)[index] =value.value<void *>();
	return(true);
QListLong:
	reinterpret_cast<QList<long> &>(list)[index] =value.toLongLong();
	return(true);
QListShort:
	reinterpret_cast<QList<short> &>(list)[index] =value.toInt();
	return(true);
QListChar:
	reinterpret_cast<QList<char> &>(list)[index] =value.toInt();
	return(true);
QListULong:
	reinterpret_cast<QList<ulong> &>(list)[index] =value.toULongLong();
	return(true);
QListUShort:
	reinterpret_cast<QList<ushort> &>(list)[index] =value.toUInt();
	return(true);
QListUChar:
	reinterpret_cast<QList<uchar> &>(list)[index] =value.toUInt();
	return(true);
QListFloat:
	reinterpret_cast<QList<float> &>(list)[index] =value.toFloat();
	return(true);
QListObjectStar:
	reinterpret_cast<QList<QObject *> &>(list)[index] =value.value<QObject *>();
	return(true);
QListSChar: // what is ??
	reinterpret_cast<QList<signed char> &>(list)[index] =value.toInt();
	return(true);
//QListVariant: not UserType
QListModelIndex:
	reinterpret_cast<QList<QModelIndex> &>(list)[index] =value.toModelIndex();
	return(true);
//QListVoid:  empty result
QListRegularExpression:
	reinterpret_cast<QList<QRegularExpression> &>(list)[index] =value.toRegularExpression();
	return(true);
QListJsonValue:
	reinterpret_cast<QList<QJsonValue> &>(list)[index] =value.toJsonValue();
	return(true);
QListJsonObject:
	reinterpret_cast<QList<QJsonObject> &>(list)[index] =value.toJsonObject();
	return(true);
QListJsonArray:
	reinterpret_cast<QList<QJsonArray> &>(list)[index] =value.toJsonArray();
	return(true);
QListJsonDocument:
	reinterpret_cast<QList<QJsonDocument> &>(list)[index] =value.toJsonDocument();
	return(true);
QListByteArrayList:
	reinterpret_cast<QList<QByteArrayList> &>(list)[index] =value.value<QByteArrayList>();
	return(true);
QListPersistentModelIndex:
	reinterpret_cast<QList<QPersistentModelIndex> &>(list)[index] =value.toPersistentModelIndex();
	return(true);
//QListNullptr:  empty result
QListCborSimpleType:
	reinterpret_cast<QList<quint8> &>(list)[index] =value.toUInt();
	return(true);
QListCborValue:
	reinterpret_cast<QList<QCborValue> &>(list)[index] =value.value<QCborValue>();
	return(true);
QListCborArray:
	reinterpret_cast<QList<QCborArray> &>(list)[index] =value.value<QCborArray>();
	return(true);
QListCborMap:
	reinterpret_cast<QList<QCborMap> &>(list)[index] =value.value<QCborMap>();
	return(true);
/*
QListFont:
	reinterpret_cast<QList<QFont> &>(list)[index] =value.value<QFont>();
	return(true);
QListPixmap:
	reinterpret_cast<QList<QPixmap> &>(list)[index] =value.value<QPixmap>();
	return(true);
QListBrush:
	reinterpret_cast<QList<QBrush> &>(list)[index] =value.value<QBrush>();
	return(true);
QListColor:
	reinterpret_cast<QList<QColor> &>(list)[index] =value.value<QColor>();
	return(true);
QListPalette:
	reinterpret_cast<QList<QPalette> &>(list)[index] =value.value<QPalette>();
	return(true);
QListIcon:
	reinterpret_cast<QList<QIcon> &>(list)[index] =value.value<QIcon>();
	return(true);
QListImage:
	reinterpret_cast<QList<QImage> &>(list)[index] =value.value<QImage>();
	return(true);
QListPolygon:
	reinterpret_cast<QList<QPolygon> &>(list)[index] =value.value<QPolygon>();
	return(true);
QListRegion:
	reinterpret_cast<QList<QRegion> &>(list)[index] =value.value<QRegion>();
	return(true);
QListBitmap:
	reinterpret_cast<QList<QBitmap> &>(list)[index] =value.value<QBitmap>();
	return(true);
QListCursor:
	reinterpret_cast<QList<QCursor> &>(list)[index] =value.value<QCursor>();
	return(true);
QListKeySequence:
	reinterpret_cast<QList<QKeySequence> &>(list)[index] =value.value<QKeySequence>();
	return(true);
QListPen:
	reinterpret_cast<QList<QPen> &>(list)[index] =value.value<QPen>();
	return(true);
QListTextLength:
	reinterpret_cast<QList<QTextLength> &>(list)[index] =value.value<QTextLength>();
	return(true);
QListTextFormat:
	reinterpret_cast<QList<QTextFormat> &>(list)[index] =value.value<QTextFormat>();
	return(true);
QListMatrix:
	reinterpret_cast<QList<QMatrix> &>(list)[index] =value.value<QMatrix>();
	return(true);
QListTransform:
	reinterpret_cast<QList<QTransform> &>(list)[index] =value.value<QTransform>();
	return(true);
QListMatrix4x4:
	reinterpret_cast<QList<QMatrix4x4> &>(list)[index] =value.value<QMatrix4x4>();
	return(true);
QListVector2D:
	reinterpret_cast<QList<QVector2D> &>(list)[index] =value.value<QVector2D>();
	return(true);
QListVector3D:
	reinterpret_cast<QList<QVector3D> &>(list)[index] =value.value<QVector3D>();
	return(true);
QListVector4D:
	reinterpret_cast<QList<QVector4D> &>(list)[index] =value.value<QVector4D>();
	return(true);
QListQuaternion:
	reinterpret_cast<QList<QQuaternion> &>(list)[index] =value.value<QQuaternion>();
	return(true);
QListPolygonF:
	reinterpret_cast<QList<QPolygonF> &>(list)[index] =value.value<QPolygonF>();
	return(true);
QListColorSpace:
	reinterpret_cast<QList<QColorSpace> &>(list)[index] =value.value<QColorSpace>();
	return(true);
QListSizePolicy:
	reinterpret_cast<QList<QSizePolicy> &>(list)[index] =value.value<QSizePolicy>();
	return(true);
//*/
//................................................................
init:
	if(!MinUserTypeId)
	{
		getListValue({}, 0); // call for calc MinUserTypeId

		if(!MinUserTypeId)
			return(false);
	}

	SetList$JumpTable[qMetaTypeId<QList<bool>>() - MinUserTypeId] = &&QListBool;
	SetList$JumpTable[qMetaTypeId<QList<int>>() - MinUserTypeId] = &&QListInt;
	SetList$JumpTable[qMetaTypeId<QList<uint>>() - MinUserTypeId] = &&QListUInt;
	SetList$JumpTable[qMetaTypeId<QList<qlonglong>>() - MinUserTypeId] = &&QListLongLong;
	SetList$JumpTable[qMetaTypeId<QList<qulonglong>>() - MinUserTypeId] = &&QListULongLong;
	SetList$JumpTable[qMetaTypeId<QList<double>>() - MinUserTypeId] = &&QListDouble;
	SetList$JumpTable[qMetaTypeId<QList<QChar>>() - MinUserTypeId] = &&QListQChar;
	SetList$JumpTable[qMetaTypeId<QList<QVariantMap>>() - MinUserTypeId] = &&QListVariantMap;
//	SetList$JumpTable[qMetaTypeId<QList<QVariantList>>() - MinUserTypeId] = &&QListVariantList; // not UserType
	SetList$JumpTable[qMetaTypeId<QList<QString>>() - MinUserTypeId] = &&QListString;
	SetList$JumpTable[qMetaTypeId<QList<QStringList>>() - MinUserTypeId] = &&QListStringList;
//	SetList$JumpTable[qMetaTypeId<QList<QByteArray>>() - MinUserTypeId] = &&QListByteArray;  // not UserType
	SetList$JumpTable[qMetaTypeId<QList<QBitArray>>() - MinUserTypeId] = &&QListBitArray;
	SetList$JumpTable[qMetaTypeId<QList<QDate>>() - MinUserTypeId] = &&QListDate;
	SetList$JumpTable[qMetaTypeId<QList<QTime>>() - MinUserTypeId] = &&QListTime;
	SetList$JumpTable[qMetaTypeId<QList<QDateTime>>() - MinUserTypeId] = &&QListDateTime;
	SetList$JumpTable[qMetaTypeId<QList<QUrl>>() - MinUserTypeId] = &&QListUrl;
	SetList$JumpTable[qMetaTypeId<QList<QLocale>>() - MinUserTypeId] = &&QListLocale;
	SetList$JumpTable[qMetaTypeId<QList<QRect>>() - MinUserTypeId] = &&QListRect;
	SetList$JumpTable[qMetaTypeId<QList<QRectF>>() - MinUserTypeId] = &&QListRectF;
	SetList$JumpTable[qMetaTypeId<QList<QSize>>() - MinUserTypeId] = &&QListSize;
	SetList$JumpTable[qMetaTypeId<QList<QSizeF>>() - MinUserTypeId] = &&QListSizeF;
	SetList$JumpTable[qMetaTypeId<QList<QLine>>() - MinUserTypeId] = &&QListLine;
	SetList$JumpTable[qMetaTypeId<QList<QLineF>>() - MinUserTypeId] = &&QListLineF;
	SetList$JumpTable[qMetaTypeId<QList<QPoint>>() - MinUserTypeId] = &&QListPoint;
	SetList$JumpTable[qMetaTypeId<QList<QPointF>>() - MinUserTypeId] = &&QListPointF;
	SetList$JumpTable[qMetaTypeId<QList<QRegExp>>() - MinUserTypeId] = &&QListRegExp;
	SetList$JumpTable[qMetaTypeId<QList<QVariantHash>>() - MinUserTypeId] = &&QListVariantHash;
	SetList$JumpTable[qMetaTypeId<QList<QEasingCurve>>() - MinUserTypeId] = &&QListEasingCurve;
	SetList$JumpTable[qMetaTypeId<QList<QUuid>>() - MinUserTypeId] = &&QListUuid;
	SetList$JumpTable[qMetaTypeId<QList<void *>>() - MinUserTypeId] = &&QListVoidStar;
	SetList$JumpTable[qMetaTypeId<QList<long>>() - MinUserTypeId] = &&QListLong;
	SetList$JumpTable[qMetaTypeId<QList<short>>() - MinUserTypeId] = &&QListShort;
	SetList$JumpTable[qMetaTypeId<QList<char>>() - MinUserTypeId] = &&QListChar;
	SetList$JumpTable[qMetaTypeId<QList<ulong>>() - MinUserTypeId] = &&QListULong;
	SetList$JumpTable[qMetaTypeId<QList<ushort>>() - MinUserTypeId] = &&QListUShort;
	SetList$JumpTable[qMetaTypeId<QList<uchar>>() - MinUserTypeId] = &&QListUChar;
	SetList$JumpTable[qMetaTypeId<QList<float>>() - MinUserTypeId] = &&QListFloat;
	SetList$JumpTable[qMetaTypeId<QList<QObject *>>() - MinUserTypeId] = &&QListObjectStar;
	SetList$JumpTable[qMetaTypeId<QList<char>>() - MinUserTypeId] = &&QListSChar;
//	SetList$JumpTable[qMetaTypeId<QList<QVariant>>() - MinUserTypeId] = &&QListVariant; // not UserType
	SetList$JumpTable[qMetaTypeId<QList<QModelIndex>>() - MinUserTypeId] = &&QListModelIndex;
	// &&QListVoid;
	SetList$JumpTable[qMetaTypeId<QList<QRegularExpression>>() - MinUserTypeId] = &&QListRegularExpression;
	SetList$JumpTable[qMetaTypeId<QList<QJsonValue>>() - MinUserTypeId] = &&QListJsonValue;
	SetList$JumpTable[qMetaTypeId<QList<QJsonObject>>() - MinUserTypeId] = &&QListJsonObject;
	SetList$JumpTable[qMetaTypeId<QList<QJsonArray>>() - MinUserTypeId] = &&QListJsonArray;
	SetList$JumpTable[qMetaTypeId<QList<QJsonDocument>>() - MinUserTypeId] = &&QListJsonDocument;
	SetList$JumpTable[qMetaTypeId<QList<QByteArrayList>>() - MinUserTypeId] = &&QListByteArrayList;
	SetList$JumpTable[qMetaTypeId<QList<QPersistentModelIndex>>() - MinUserTypeId] = &&QListPersistentModelIndex;
	// &&QListNullptr:
	SetList$JumpTable[qMetaTypeId<QList<quint8>>() - MinUserTypeId] = &&QListCborSimpleType;
	SetList$JumpTable[qMetaTypeId<QList<QCborValue>>() - MinUserTypeId] = &&QListCborValue;
	SetList$JumpTable[qMetaTypeId<QList<QCborArray>>() - MinUserTypeId] = &&QListCborArray;
	SetList$JumpTable[qMetaTypeId<QList<QCborMap>>() - MinUserTypeId] = &&QListCborMap;
/*
	SetList$JumpTable[qMetaTypeId<QList<QFont>>() - MinUserTypeId] = &&QListFont;
	SetList$JumpTable[qMetaTypeId<QList<QPixmap>>() - MinUserTypeId] = &&QListPixmap;
	SetList$JumpTable[qMetaTypeId<QList<QBrush>>() - MinUserTypeId] = &&QListBrush;
	SetList$JumpTable[qMetaTypeId<QList<QColor>>() - MinUserTypeId] = &&QListColor;
	SetList$JumpTable[qMetaTypeId<QList<QPalette>>() - MinUserTypeId] = &&QListPalette;
	SetList$JumpTable[qMetaTypeId<QList<QIcon>>() - MinUserTypeId] = &&QListIcon;
	SetList$JumpTable[qMetaTypeId<QList<QImage>>() - MinUserTypeId] = &&QListImage;
	SetList$JumpTable[qMetaTypeId<QList<QPolygon>>() - MinUserTypeId] = &&QListPolygon;
	SetList$JumpTable[qMetaTypeId<QList<QRegion>>() - MinUserTypeId] = &&QListRegion;
	SetList$JumpTable[qMetaTypeId<QList<QBitmap>>() - MinUserTypeId] = &&QListBitmap;
	SetList$JumpTable[qMetaTypeId<QList<QCursor>>() - MinUserTypeId] = &&QListCursor;
	SetList$JumpTable[qMetaTypeId<QList<QKeySequence>>() - MinUserTypeId] = &&QListKeySequence;
	SetList$JumpTable[qMetaTypeId<QList<QPen>>() - MinUserTypeId] = &&QListPen;
	SetList$JumpTable[qMetaTypeId<QList<QTextLength>>() - MinUserTypeId] = &&QListTextLength;
	SetList$JumpTable[qMetaTypeId<QList<QTextFormat>>() - MinUserTypeId] = &&QListTextFormat;
	SetList$JumpTable[qMetaTypeId<QList<QMatrix>>() - MinUserTypeId] = &&QListMatrix;
	SetList$JumpTable[qMetaTypeId<QList<QTransform>>() - MinUserTypeId] = &&QListTransform;
	SetList$JumpTable[qMetaTypeId<QList<QMatrix4x4>>() - MinUserTypeId] = &&QListMatrix4x4;
	SetList$JumpTable[qMetaTypeId<QList<QVector2D>>() - MinUserTypeId] = &&QListVector2D;
	SetList$JumpTable[qMetaTypeId<QList<QVector3D>>() - MinUserTypeId] = &&QListVector3D;
	SetList$JumpTable[qMetaTypeId<QList<QVector4D>>() - MinUserTypeId] = &&QListVector4D;
	SetList$JumpTable[qMetaTypeId<QList<QQuaternion>>() - MinUserTypeId] = &&QListQuaternion;
	SetList$JumpTable[qMetaTypeId<QList<QPolygonF>>() - MinUserTypeId] = &&QListPolygonF;
	SetList$JumpTable[qMetaTypeId<QList<QColorSpace>>() - MinUserTypeId] = &&QListColorSpace;
	SetList$JumpTable[qMetaTypeId<QList<QSizePolicy>>() - MinUserTypeId] = &&QListSizePolicy;
//*/
	goto begin; // ready
}
//......................................................................................
bool
QVariantModel::removeListValue(QVariant &list, const uint index)
{
	if(!RemoveList$JumpTable[0])
		goto init;

begin:		// User types
	if(isListType(list) && index < lengthList(list))
		if(list.type() == QVariant::UserType && list.type() <= (MinUserTypeId + QMetaType::HighestInternalId))
			goto *(RemoveList$JumpTable[list.userType() - MinUserTypeId]);
		else // standard types
		if(list.type() == QVariant::StringList)
		{
			reinterpret_cast<QStringList &>(list).removeAt(index);

		return(true);
		}

Unknown:
	return(false);

QListBool:
	reinterpret_cast<QList<bool> &>(list).removeAt(index);
	return(true);
QListInt:
	reinterpret_cast<QList<int> &>(list).removeAt(index);
	return(true);
QListUInt:
	reinterpret_cast<QList<uint> &>(list).removeAt(index);
	return(true);
QListLongLong:
	reinterpret_cast<QList<qlonglong> &>(list).removeAt(index);
	return(true);
QListULongLong:
	reinterpret_cast<QList<qulonglong> &>(list).removeAt(index);
	return(true);
QListDouble:
	reinterpret_cast<QList<double> &>(list).removeAt(index);
	return(true);
QListQChar:
	reinterpret_cast<QList<QChar> &>(list).removeAt(index);
	return(true);
QListVariantMap:
	reinterpret_cast<QList<QVariantMap> &>(list).removeAt(index);
	return(true);
//QListVariantList: not UserType

QListString:
	reinterpret_cast<QList<QString> &>(list).removeAt(index);
	return(true);
QListStringList:
	reinterpret_cast<QList<QStringList> &>(list).removeAt(index);
	return(true);
//QListByteArray: not UserType

QListBitArray:
	reinterpret_cast<QList<QBitArray> &>(list).removeAt(index);
	return(true);
QListDate:
	reinterpret_cast<QList<QDate> &>(list).removeAt(index);
	return(true);
QListTime:
	reinterpret_cast<QList<QTime> &>(list).removeAt(index);
	return(true);
QListDateTime:
	reinterpret_cast<QList<QDateTime> &>(list).removeAt(index);
	return(true);
QListUrl:
	reinterpret_cast<QList<QUrl> &>(list).removeAt(index);
	return(true);
QListLocale:
	reinterpret_cast<QList<QLocale> &>(list).removeAt(index);
	return(true);
QListRect:
	reinterpret_cast<QList<QRect> &>(list).removeAt(index);
	return(true);
QListRectF:
	reinterpret_cast<QList<QRectF> &>(list).removeAt(index);
	return(true);
QListSize:
	reinterpret_cast<QList<QSize> &>(list).removeAt(index);
	return(true);
QListSizeF:
	reinterpret_cast<QList<QSizeF> &>(list).removeAt(index);
	return(true);
QListLine:
	reinterpret_cast<QList<QLine> &>(list).removeAt(index);
	return(true);
QListLineF:
	reinterpret_cast<QList<QLineF> &>(list).removeAt(index);
	return(true);
QListPoint:
	reinterpret_cast<QList<QPoint> &>(list).removeAt(index);
	return(true);
QListPointF:
	reinterpret_cast<QList<QPointF> &>(list).removeAt(index);
	return(true);
QListRegExp:
	reinterpret_cast<QList<QRegExp> &>(list).removeAt(index);
	return(true);
QListVariantHash:
	reinterpret_cast<QList<QVariantHash> &>(list).removeAt(index);
	return(true);
QListEasingCurve:
	reinterpret_cast<QList<QEasingCurve> &>(list).removeAt(index);
	return(true);
QListUuid:
	reinterpret_cast<QList<QUuid> &>(list).removeAt(index);
	return(true);
QListVoidStar:
	reinterpret_cast<QList<void *> &>(list).removeAt(index);
	return(true);
QListLong:
	reinterpret_cast<QList<long> &>(list).removeAt(index);
	return(true);
QListShort:
	reinterpret_cast<QList<short> &>(list).removeAt(index);
	return(true);
QListChar:
	reinterpret_cast<QList<char> &>(list).removeAt(index);
	return(true);
QListULong:
	reinterpret_cast<QList<ulong> &>(list).removeAt(index);
	return(true);
QListUShort:
	reinterpret_cast<QList<ushort> &>(list).removeAt(index);
	return(true);
QListUChar:
	reinterpret_cast<QList<uchar> &>(list).removeAt(index);
	return(true);
QListFloat:
	reinterpret_cast<QList<float> &>(list).removeAt(index);
	return(true);
QListObjectStar:
	reinterpret_cast<QList<QObject *> &>(list).removeAt(index);
	return(true);
QListSChar: // what is ??
	reinterpret_cast<QList<signed char> &>(list).removeAt(index);
	return(true);
//QListVariant: not UserType
QListModelIndex:
	reinterpret_cast<QList<QModelIndex> &>(list).removeAt(index);
	return(true);
//QListVoid:  empty result
QListRegularExpression:
	reinterpret_cast<QList<QRegularExpression> &>(list).removeAt(index);
	return(true);
QListJsonValue:
	reinterpret_cast<QList<QJsonValue> &>(list).removeAt(index);
	return(true);
QListJsonObject:
	reinterpret_cast<QList<QJsonObject> &>(list).removeAt(index);
	return(true);
QListJsonArray:
	reinterpret_cast<QList<QJsonArray> &>(list).removeAt(index);
	return(true);
QListJsonDocument:
	reinterpret_cast<QList<QJsonDocument> &>(list).removeAt(index);
	return(true);
QListByteArrayList:
	reinterpret_cast<QList<QByteArrayList> &>(list).removeAt(index);
	return(true);
QListPersistentModelIndex:
	reinterpret_cast<QList<QPersistentModelIndex> &>(list).removeAt(index);
	return(true);
//QListNullptr:  empty result
QListCborSimpleType:
	reinterpret_cast<QList<quint8> &>(list).removeAt(index);
	return(true);
QListCborValue:
	reinterpret_cast<QList<QCborValue> &>(list).removeAt(index);
	return(true);
QListCborArray:
	reinterpret_cast<QList<QCborArray> &>(list).removeAt(index);
	return(true);
QListCborMap:
	reinterpret_cast<QList<QCborMap> &>(list).removeAt(index);
	return(true);
/*
QListFont:
	reinterpret_cast<QList<QFont> &>(list).removeAt(index);
	return(true);
QListPixmap:
	reinterpret_cast<QList<QPixmap> &>(list).removeAt(index);
	return(true);
QListBrush:
	reinterpret_cast<QList<QBrush> &>(list).removeAt(index);
	return(true);
QListColor:
	reinterpret_cast<QList<QColor> &>(list).removeAt(index);
	return(true);
QListPalette:
	reinterpret_cast<QList<QPalette> &>(list).removeAt(index);
	return(true);
QListIcon:
	reinterpret_cast<QList<QIcon> &>(list).removeAt(index);
	return(true);
QListImage:
	reinterpret_cast<QList<QImage> &>(list).removeAt(index);
	return(true);
QListPolygon:
	reinterpret_cast<QList<QPolygon> &>(list).removeAt(index);
	return(true);
QListRegion:
	reinterpret_cast<QList<QRegion> &>(list).removeAt(index);
	return(true);
QListBitmap:
	reinterpret_cast<QList<QBitmap> &>(list).removeAt(index);
	return(true);
QListCursor:
	reinterpret_cast<QList<QCursor> &>(list).removeAt(index);
	return(true);
QListKeySequence:
	reinterpret_cast<QList<QKeySequence> &>(list).removeAt(index);
	return(true);
QListPen:
	reinterpret_cast<QList<QPen> &>(list).removeAt(index);
	return(true);
QListTextLength:
	reinterpret_cast<QList<QTextLength> &>(list).removeAt(index);
	return(true);
QListTextFormat:
	reinterpret_cast<QList<QTextFormat> &>(list).removeAt(index);
	return(true);
QListMatrix:
	reinterpret_cast<QList<QMatrix> &>(list).removeAt(index);
	return(true);
QListTransform:
	reinterpret_cast<QList<QTransform> &>(list).removeAt(index);
	return(true);
QListMatrix4x4:
	reinterpret_cast<QList<QMatrix4x4> &>(list).removeAt(index);
	return(true);
QListVector2D:
	reinterpret_cast<QList<QVector2D> &>(list).removeAt(index);
	return(true);
QListVector3D:
	reinterpret_cast<QList<QVector3D> &>(list).removeAt(index);
	return(true);
QListVector4D:
	reinterpret_cast<QList<QVector4D> &>(list).removeAt(index);
	return(true);
QListQuaternion:
	reinterpret_cast<QList<QQuaternion> &>(list).removeAt(index);
	return(true);
QListPolygonF:
	reinterpret_cast<QList<QPolygonF> &>(list).removeAt(index);
	return(true);
QListColorSpace:
	reinterpret_cast<QList<QColorSpace> &>(list).removeAt(index);
	return(true);
QListSizePolicy:
	reinterpret_cast<QList<QSizePolicy> &>(list).removeAt(index);
	return(true);
//*/
//................................................................
init:
	if(!MinUserTypeId)
	{
		getListValue({}, 0); // call for calc MinUserTypeId

		if(!MinUserTypeId)
			return(false);
	}

	RemoveList$JumpTable[qMetaTypeId<QList<bool>>() - MinUserTypeId] = &&QListBool;
	RemoveList$JumpTable[qMetaTypeId<QList<int>>() - MinUserTypeId] = &&QListInt;
	RemoveList$JumpTable[qMetaTypeId<QList<uint>>() - MinUserTypeId] = &&QListUInt;
	RemoveList$JumpTable[qMetaTypeId<QList<qlonglong>>() - MinUserTypeId] = &&QListLongLong;
	RemoveList$JumpTable[qMetaTypeId<QList<qulonglong>>() - MinUserTypeId] = &&QListULongLong;
	RemoveList$JumpTable[qMetaTypeId<QList<double>>() - MinUserTypeId] = &&QListDouble;
	RemoveList$JumpTable[qMetaTypeId<QList<QChar>>() - MinUserTypeId] = &&QListQChar;
	RemoveList$JumpTable[qMetaTypeId<QList<QVariantMap>>() - MinUserTypeId] = &&QListVariantMap;
//	RemoveList$JumpTable[qMetaTypeId<QList<QVariantList>>() - MinUserTypeId] = &&QListVariantList; // not UserType
	RemoveList$JumpTable[qMetaTypeId<QList<QString>>() - MinUserTypeId] = &&QListString;
	RemoveList$JumpTable[qMetaTypeId<QList<QStringList>>() - MinUserTypeId] = &&QListStringList;
//	RemoveList$JumpTable[qMetaTypeId<QList<QByteArray>>() - MinUserTypeId] = &&QListByteArray;  // not UserType
	RemoveList$JumpTable[qMetaTypeId<QList<QBitArray>>() - MinUserTypeId] = &&QListBitArray;
	RemoveList$JumpTable[qMetaTypeId<QList<QDate>>() - MinUserTypeId] = &&QListDate;
	RemoveList$JumpTable[qMetaTypeId<QList<QTime>>() - MinUserTypeId] = &&QListTime;
	RemoveList$JumpTable[qMetaTypeId<QList<QDateTime>>() - MinUserTypeId] = &&QListDateTime;
	RemoveList$JumpTable[qMetaTypeId<QList<QUrl>>() - MinUserTypeId] = &&QListUrl;
	RemoveList$JumpTable[qMetaTypeId<QList<QLocale>>() - MinUserTypeId] = &&QListLocale;
	RemoveList$JumpTable[qMetaTypeId<QList<QRect>>() - MinUserTypeId] = &&QListRect;
	RemoveList$JumpTable[qMetaTypeId<QList<QRectF>>() - MinUserTypeId] = &&QListRectF;
	RemoveList$JumpTable[qMetaTypeId<QList<QSize>>() - MinUserTypeId] = &&QListSize;
	RemoveList$JumpTable[qMetaTypeId<QList<QSizeF>>() - MinUserTypeId] = &&QListSizeF;
	RemoveList$JumpTable[qMetaTypeId<QList<QLine>>() - MinUserTypeId] = &&QListLine;
	RemoveList$JumpTable[qMetaTypeId<QList<QLineF>>() - MinUserTypeId] = &&QListLineF;
	RemoveList$JumpTable[qMetaTypeId<QList<QPoint>>() - MinUserTypeId] = &&QListPoint;
	RemoveList$JumpTable[qMetaTypeId<QList<QPointF>>() - MinUserTypeId] = &&QListPointF;
	RemoveList$JumpTable[qMetaTypeId<QList<QRegExp>>() - MinUserTypeId] = &&QListRegExp;
	RemoveList$JumpTable[qMetaTypeId<QList<QVariantHash>>() - MinUserTypeId] = &&QListVariantHash;
	RemoveList$JumpTable[qMetaTypeId<QList<QEasingCurve>>() - MinUserTypeId] = &&QListEasingCurve;
	RemoveList$JumpTable[qMetaTypeId<QList<QUuid>>() - MinUserTypeId] = &&QListUuid;
	RemoveList$JumpTable[qMetaTypeId<QList<void *>>() - MinUserTypeId] = &&QListVoidStar;
	RemoveList$JumpTable[qMetaTypeId<QList<long>>() - MinUserTypeId] = &&QListLong;
	RemoveList$JumpTable[qMetaTypeId<QList<short>>() - MinUserTypeId] = &&QListShort;
	RemoveList$JumpTable[qMetaTypeId<QList<char>>() - MinUserTypeId] = &&QListChar;
	RemoveList$JumpTable[qMetaTypeId<QList<ulong>>() - MinUserTypeId] = &&QListULong;
	RemoveList$JumpTable[qMetaTypeId<QList<ushort>>() - MinUserTypeId] = &&QListUShort;
	RemoveList$JumpTable[qMetaTypeId<QList<uchar>>() - MinUserTypeId] = &&QListUChar;
	RemoveList$JumpTable[qMetaTypeId<QList<float>>() - MinUserTypeId] = &&QListFloat;
	RemoveList$JumpTable[qMetaTypeId<QList<QObject *>>() - MinUserTypeId] = &&QListObjectStar;
	RemoveList$JumpTable[qMetaTypeId<QList<char>>() - MinUserTypeId] = &&QListSChar;
//	RemoveList$JumpTable[qMetaTypeId<QList<QVariant>>() - MinUserTypeId] = &&QListVariant; // not UserType
	RemoveList$JumpTable[qMetaTypeId<QList<QModelIndex>>() - MinUserTypeId] = &&QListModelIndex;
	// &&QListVoid;
	RemoveList$JumpTable[qMetaTypeId<QList<QRegularExpression>>() - MinUserTypeId] = &&QListRegularExpression;
	RemoveList$JumpTable[qMetaTypeId<QList<QJsonValue>>() - MinUserTypeId] = &&QListJsonValue;
	RemoveList$JumpTable[qMetaTypeId<QList<QJsonObject>>() - MinUserTypeId] = &&QListJsonObject;
	RemoveList$JumpTable[qMetaTypeId<QList<QJsonArray>>() - MinUserTypeId] = &&QListJsonArray;
	RemoveList$JumpTable[qMetaTypeId<QList<QJsonDocument>>() - MinUserTypeId] = &&QListJsonDocument;
	RemoveList$JumpTable[qMetaTypeId<QList<QByteArrayList>>() - MinUserTypeId] = &&QListByteArrayList;
	RemoveList$JumpTable[qMetaTypeId<QList<QPersistentModelIndex>>() - MinUserTypeId] = &&QListPersistentModelIndex;
	// &&QListNullptr:
	RemoveList$JumpTable[qMetaTypeId<QList<quint8>>() - MinUserTypeId] = &&QListCborSimpleType;
	RemoveList$JumpTable[qMetaTypeId<QList<QCborValue>>() - MinUserTypeId] = &&QListCborValue;
	RemoveList$JumpTable[qMetaTypeId<QList<QCborArray>>() - MinUserTypeId] = &&QListCborArray;
	RemoveList$JumpTable[qMetaTypeId<QList<QCborMap>>() - MinUserTypeId] = &&QListCborMap;
/*
	RemoveList$JumpTable[qMetaTypeId<QList<QFont>>() - MinUserTypeId] = &&QListFont;
	RemoveList$JumpTable[qMetaTypeId<QList<QPixmap>>() - MinUserTypeId] = &&QListPixmap;
	RemoveList$JumpTable[qMetaTypeId<QList<QBrush>>() - MinUserTypeId] = &&QListBrush;
	RemoveList$JumpTable[qMetaTypeId<QList<QColor>>() - MinUserTypeId] = &&QListColor;
	RemoveList$JumpTable[qMetaTypeId<QList<QPalette>>() - MinUserTypeId] = &&QListPalette;
	RemoveList$JumpTable[qMetaTypeId<QList<QIcon>>() - MinUserTypeId] = &&QListIcon;
	RemoveList$JumpTable[qMetaTypeId<QList<QImage>>() - MinUserTypeId] = &&QListImage;
	RemoveList$JumpTable[qMetaTypeId<QList<QPolygon>>() - MinUserTypeId] = &&QListPolygon;
	RemoveList$JumpTable[qMetaTypeId<QList<QRegion>>() - MinUserTypeId] = &&QListRegion;
	RemoveList$JumpTable[qMetaTypeId<QList<QBitmap>>() - MinUserTypeId] = &&QListBitmap;
	RemoveList$JumpTable[qMetaTypeId<QList<QCursor>>() - MinUserTypeId] = &&QListCursor;
	RemoveList$JumpTable[qMetaTypeId<QList<QKeySequence>>() - MinUserTypeId] = &&QListKeySequence;
	RemoveList$JumpTable[qMetaTypeId<QList<QPen>>() - MinUserTypeId] = &&QListPen;
	RemoveList$JumpTable[qMetaTypeId<QList<QTextLength>>() - MinUserTypeId] = &&QListTextLength;
	RemoveList$JumpTable[qMetaTypeId<QList<QTextFormat>>() - MinUserTypeId] = &&QListTextFormat;
	RemoveList$JumpTable[qMetaTypeId<QList<QMatrix>>() - MinUserTypeId] = &&QListMatrix;
	RemoveList$JumpTable[qMetaTypeId<QList<QTransform>>() - MinUserTypeId] = &&QListTransform;
	RemoveList$JumpTable[qMetaTypeId<QList<QMatrix4x4>>() - MinUserTypeId] = &&QListMatrix4x4;
	RemoveList$JumpTable[qMetaTypeId<QList<QVector2D>>() - MinUserTypeId] = &&QListVector2D;
	RemoveList$JumpTable[qMetaTypeId<QList<QVector3D>>() - MinUserTypeId] = &&QListVector3D;
	RemoveList$JumpTable[qMetaTypeId<QList<QVector4D>>() - MinUserTypeId] = &&QListVector4D;
	RemoveList$JumpTable[qMetaTypeId<QList<QQuaternion>>() - MinUserTypeId] = &&QListQuaternion;
	RemoveList$JumpTable[qMetaTypeId<QList<QPolygonF>>() - MinUserTypeId] = &&QListPolygonF;
	RemoveList$JumpTable[qMetaTypeId<QList<QColorSpace>>() - MinUserTypeId] = &&QListColorSpace;
	RemoveList$JumpTable[qMetaTypeId<QList<QSizePolicy>>() - MinUserTypeId] = &&QListSizePolicy;
//*/
	goto begin; // ready
}
//..........................................................................................
bool
QVariantModel::insertListValue(QVariant &list, const uint index, const QVariant &value)
{
	if(!InsertList$JumpTable[0])
		goto init;

begin:		// User types
	if(isListType(list) && index <= lengthList(list))
		if(list.type() == QVariant::UserType && list.type() <= (MinUserTypeId + QMetaType::HighestInternalId))
			goto *(InsertList$JumpTable[list.userType() - MinUserTypeId]);
		else // standard types
		if(list.type() == QVariant::StringList)
		{
			reinterpret_cast<QStringList &>(list).insert(index, value.toString());

		return(true);
		}

Unknown:
	return(false);

QListBool:
	reinterpret_cast<QList<bool> &>(list).insert(index, value.toBool());
	return(true);
QListInt:
	reinterpret_cast<QList<int> &>(list).insert(index, value.toInt());
	return(true);
QListUInt:
	reinterpret_cast<QList<uint> &>(list).insert(index, value.toUInt());
	return(true);
QListLongLong:
	reinterpret_cast<QList<qlonglong> &>(list).insert(index, value.toLongLong());
	return(true);
QListULongLong:
	reinterpret_cast<QList<qulonglong> &>(list).insert(index, value.toULongLong());
	return(true);
QListDouble:
	reinterpret_cast<QList<double> &>(list).insert(index, value.toDouble());
	return(true);
QListQChar:
	reinterpret_cast<QList<QChar> &>(list).insert(index, value.toChar());
	return(true);
QListVariantMap:
	reinterpret_cast<QList<QVariantMap> &>(list).insert(index, value.toMap());
	return(true);
//QListVariantList: not UserType

QListString:
	reinterpret_cast<QList<QString> &>(list).insert(index, value.toString());
	return(true);
QListStringList:
	reinterpret_cast<QList<QStringList> &>(list).insert(index, value.toStringList());
	return(true);
//QListByteArray: not UserType

QListBitArray:
	reinterpret_cast<QList<QBitArray> &>(list).insert(index, value.toBitArray());
	return(true);
QListDate:
	reinterpret_cast<QList<QDate> &>(list).insert(index, value.toDate());
	return(true);
QListTime:
	reinterpret_cast<QList<QTime> &>(list).insert(index, value.toTime());
	return(true);
QListDateTime:
	reinterpret_cast<QList<QDateTime> &>(list).insert(index, value.toDateTime());
	return(true);
QListUrl:
	reinterpret_cast<QList<QUrl> &>(list).insert(index, value.toUrl());
	return(true);
QListLocale:
	reinterpret_cast<QList<QLocale> &>(list).insert(index, value.toLocale());
	return(true);
QListRect:
	reinterpret_cast<QList<QRect> &>(list).insert(index, value.toRect());
	return(true);
QListRectF:
	reinterpret_cast<QList<QRectF> &>(list).insert(index, value.toRectF());
	return(true);
QListSize:
	reinterpret_cast<QList<QSize> &>(list).insert(index, value.toSize());
	return(true);
QListSizeF:
	reinterpret_cast<QList<QSizeF> &>(list).insert(index, value.toSizeF());
	return(true);
QListLine:
	reinterpret_cast<QList<QLine> &>(list).insert(index, value.toLine());
	return(true);
QListLineF:
	reinterpret_cast<QList<QLineF> &>(list).insert(index, value.toLineF());
	return(true);
QListPoint:
	reinterpret_cast<QList<QPoint> &>(list).insert(index, value.toPoint());
	return(true);
QListPointF:
	reinterpret_cast<QList<QPointF> &>(list).insert(index, value.toPointF());
	return(true);
QListRegExp:
	reinterpret_cast<QList<QRegExp> &>(list).insert(index, value.toRegExp());
	return(true);
QListVariantHash:
	reinterpret_cast<QList<QVariantHash> &>(list).insert(index, value.toHash());
	return(true);
QListEasingCurve:
	reinterpret_cast<QList<QEasingCurve> &>(list).insert(index, value.toEasingCurve());
	return(true);
QListUuid:
	reinterpret_cast<QList<QUuid> &>(list).insert(index, value.toUuid());
	return(true);
QListVoidStar:
	reinterpret_cast<QList<void *> &>(list).insert(index, value.value<void *>());
	return(true);
QListLong:
	reinterpret_cast<QList<long> &>(list).insert(index, value.toLongLong());
	return(true);
QListShort:
	reinterpret_cast<QList<short> &>(list).insert(index, value.toInt());
	return(true);
QListChar:
	reinterpret_cast<QList<char> &>(list).insert(index, value.toInt());
	return(true);
QListULong:
	reinterpret_cast<QList<ulong> &>(list).insert(index, value.toULongLong());
	return(true);
QListUShort:
	reinterpret_cast<QList<ushort> &>(list).insert(index, value.toUInt());
	return(true);
QListUChar:
	reinterpret_cast<QList<uchar> &>(list).insert(index, value.toUInt());
	return(true);
QListFloat:
	reinterpret_cast<QList<float> &>(list).insert(index, value.toFloat());
	return(true);
QListObjectStar:
	reinterpret_cast<QList<QObject *> &>(list).insert(index, value.value<QObject *>());
	return(true);
QListSChar: // what is ??
	reinterpret_cast<QList<signed char> &>(list).insert(index, value.toInt());
	return(true);
//QListVariant: not UserType
QListModelIndex:
	reinterpret_cast<QList<QModelIndex> &>(list).insert(index, value.toModelIndex());
	return(true);
//QListVoid:  empty result
QListRegularExpression:
	reinterpret_cast<QList<QRegularExpression> &>(list).insert(index, value.toRegularExpression());
	return(true);
QListJsonValue:
	reinterpret_cast<QList<QJsonValue> &>(list).insert(index, value.toJsonValue());
	return(true);
QListJsonObject:
	reinterpret_cast<QList<QJsonObject> &>(list).insert(index, value.toJsonObject());
	return(true);
QListJsonArray:
	reinterpret_cast<QList<QJsonArray> &>(list).insert(index, value.toJsonArray());
	return(true);
QListJsonDocument:
	reinterpret_cast<QList<QJsonDocument> &>(list).insert(index, value.toJsonDocument());
	return(true);
QListByteArrayList:
	reinterpret_cast<QList<QByteArrayList> &>(list).insert(index, value.value<QByteArrayList>());
	return(true);
QListPersistentModelIndex:
	reinterpret_cast<QList<QPersistentModelIndex> &>(list).insert(index, value.toPersistentModelIndex());
	return(true);
//QListNullptr:  empty result
QListCborSimpleType:
	reinterpret_cast<QList<quint8> &>(list).insert(index, value.toUInt());
	return(true);
QListCborValue:
	reinterpret_cast<QList<QCborValue> &>(list).insert(index, value.value<QCborValue>());
	return(true);
QListCborArray:
	reinterpret_cast<QList<QCborArray> &>(list).insert(index, value.value<QCborArray>());
	return(true);
QListCborMap:
	reinterpret_cast<QList<QCborMap> &>(list).insert(index, value.value<QCborMap>());
	return(true);
/*
QListFont:
	reinterpret_cast<QList<QFont> &>(list).insert(index, {});
	return(true);
QListPixmap:
	reinterpret_cast<QList<QPixmap> &>(list).insert(index, {});
	return(true);
QListBrush:
	reinterpret_cast<QList<QBrush> &>(list).insert(index, {});
	return(true);
QListColor:
	reinterpret_cast<QList<QColor> &>(list).insert(index, {});
	return(true);
QListPalette:
	reinterpret_cast<QList<QPalette> &>(list).insert(index, {});
	return(true);
QListIcon:
	reinterpret_cast<QList<QIcon> &>(list).insert(index, {});
	return(true);
QListImage:
	reinterpret_cast<QList<QImage> &>(list).insert(index, {});
	return(true);
QListPolygon:
	reinterpret_cast<QList<QPolygon> &>(list).insert(index, {});
	return(true);
QListRegion:
	reinterpret_cast<QList<QRegion> &>(list).insert(index, {});
	return(true);
QListBitmap:
	reinterpret_cast<QList<QBitmap> &>(list).insert(index, {});
	return(true);
QListCursor:
	reinterpret_cast<QList<QCursor> &>(list).insert(index, {});
	return(true);
QListKeySequence:
	reinterpret_cast<QList<QKeySequence> &>(list).insert(index, {});
	return(true);
QListPen:
	reinterpret_cast<QList<QPen> &>(list).insert(index, {});
	return(true);
QListTextLength:
	reinterpret_cast<QList<QTextLength> &>(list).insert(index, {});
	return(true);
QListTextFormat:
	reinterpret_cast<QList<QTextFormat> &>(list).insert(index, {});
	return(true);
QListMatrix:
	reinterpret_cast<QList<QMatrix> &>(list).insert(index, {});
	return(true);
QListTransform:
	reinterpret_cast<QList<QTransform> &>(list).insert(index, {});
	return(true);
QListMatrix4x4:
	reinterpret_cast<QList<QMatrix4x4> &>(list).insert(index, {});
	return(true);
QListVector2D:
	reinterpret_cast<QList<QVector2D> &>(list).insert(index, {});
	return(true);
QListVector3D:
	reinterpret_cast<QList<QVector3D> &>(list).insert(index, {});
	return(true);
QListVector4D:
	reinterpret_cast<QList<QVector4D> &>(list).insert(index, {});
	return(true);
QListQuaternion:
	reinterpret_cast<QList<QQuaternion> &>(list).insert(index, {});
	return(true);
QListPolygonF:
	reinterpret_cast<QList<QPolygonF> &>(list).insert(index, {});
	return(true);
QListColorSpace:
	reinterpret_cast<QList<QColorSpace> &>(list).insert(index, {});
	return(true);
QListSizePolicy:
	reinterpret_cast<QList<QSizePolicy> &>(list).insert(index, {});
	return(true);
//*/
//................................................................
init:
	if(!MinUserTypeId)
	{
		getListValue({}, 0); // call for calc MinUserTypeId

		if(!MinUserTypeId)
			return(false);
	}

	InsertList$JumpTable[qMetaTypeId<QList<bool>>() - MinUserTypeId] = &&QListBool;
	InsertList$JumpTable[qMetaTypeId<QList<int>>() - MinUserTypeId] = &&QListInt;
	InsertList$JumpTable[qMetaTypeId<QList<uint>>() - MinUserTypeId] = &&QListUInt;
	InsertList$JumpTable[qMetaTypeId<QList<qlonglong>>() - MinUserTypeId] = &&QListLongLong;
	InsertList$JumpTable[qMetaTypeId<QList<qulonglong>>() - MinUserTypeId] = &&QListULongLong;
	InsertList$JumpTable[qMetaTypeId<QList<double>>() - MinUserTypeId] = &&QListDouble;
	InsertList$JumpTable[qMetaTypeId<QList<QChar>>() - MinUserTypeId] = &&QListQChar;
	InsertList$JumpTable[qMetaTypeId<QList<QVariantMap>>() - MinUserTypeId] = &&QListVariantMap;
//	InsertList$JumpTable[qMetaTypeId<QList<QVariantList>>() - MinUserTypeId] = &&QListVariantList; // not UserType
	InsertList$JumpTable[qMetaTypeId<QList<QString>>() - MinUserTypeId] = &&QListString;
	InsertList$JumpTable[qMetaTypeId<QList<QStringList>>() - MinUserTypeId] = &&QListStringList;
//	InsertList$JumpTable[qMetaTypeId<QList<QByteArray>>() - MinUserTypeId] = &&QListByteArray;  // not UserType
	InsertList$JumpTable[qMetaTypeId<QList<QBitArray>>() - MinUserTypeId] = &&QListBitArray;
	InsertList$JumpTable[qMetaTypeId<QList<QDate>>() - MinUserTypeId] = &&QListDate;
	InsertList$JumpTable[qMetaTypeId<QList<QTime>>() - MinUserTypeId] = &&QListTime;
	InsertList$JumpTable[qMetaTypeId<QList<QDateTime>>() - MinUserTypeId] = &&QListDateTime;
	InsertList$JumpTable[qMetaTypeId<QList<QUrl>>() - MinUserTypeId] = &&QListUrl;
	InsertList$JumpTable[qMetaTypeId<QList<QLocale>>() - MinUserTypeId] = &&QListLocale;
	InsertList$JumpTable[qMetaTypeId<QList<QRect>>() - MinUserTypeId] = &&QListRect;
	InsertList$JumpTable[qMetaTypeId<QList<QRectF>>() - MinUserTypeId] = &&QListRectF;
	InsertList$JumpTable[qMetaTypeId<QList<QSize>>() - MinUserTypeId] = &&QListSize;
	InsertList$JumpTable[qMetaTypeId<QList<QSizeF>>() - MinUserTypeId] = &&QListSizeF;
	InsertList$JumpTable[qMetaTypeId<QList<QLine>>() - MinUserTypeId] = &&QListLine;
	InsertList$JumpTable[qMetaTypeId<QList<QLineF>>() - MinUserTypeId] = &&QListLineF;
	InsertList$JumpTable[qMetaTypeId<QList<QPoint>>() - MinUserTypeId] = &&QListPoint;
	InsertList$JumpTable[qMetaTypeId<QList<QPointF>>() - MinUserTypeId] = &&QListPointF;
	InsertList$JumpTable[qMetaTypeId<QList<QRegExp>>() - MinUserTypeId] = &&QListRegExp;
	InsertList$JumpTable[qMetaTypeId<QList<QVariantHash>>() - MinUserTypeId] = &&QListVariantHash;
	InsertList$JumpTable[qMetaTypeId<QList<QEasingCurve>>() - MinUserTypeId] = &&QListEasingCurve;
	InsertList$JumpTable[qMetaTypeId<QList<QUuid>>() - MinUserTypeId] = &&QListUuid;
	InsertList$JumpTable[qMetaTypeId<QList<void *>>() - MinUserTypeId] = &&QListVoidStar;
	InsertList$JumpTable[qMetaTypeId<QList<long>>() - MinUserTypeId] = &&QListLong;
	InsertList$JumpTable[qMetaTypeId<QList<short>>() - MinUserTypeId] = &&QListShort;
	InsertList$JumpTable[qMetaTypeId<QList<char>>() - MinUserTypeId] = &&QListChar;
	InsertList$JumpTable[qMetaTypeId<QList<ulong>>() - MinUserTypeId] = &&QListULong;
	InsertList$JumpTable[qMetaTypeId<QList<ushort>>() - MinUserTypeId] = &&QListUShort;
	InsertList$JumpTable[qMetaTypeId<QList<uchar>>() - MinUserTypeId] = &&QListUChar;
	InsertList$JumpTable[qMetaTypeId<QList<float>>() - MinUserTypeId] = &&QListFloat;
	InsertList$JumpTable[qMetaTypeId<QList<QObject *>>() - MinUserTypeId] = &&QListObjectStar;
	InsertList$JumpTable[qMetaTypeId<QList<char>>() - MinUserTypeId] = &&QListSChar;
//	InsertList$JumpTable[qMetaTypeId<QList<QVariant>>() - MinUserTypeId] = &&QListVariant; // not UserType
	InsertList$JumpTable[qMetaTypeId<QList<QModelIndex>>() - MinUserTypeId] = &&QListModelIndex;
	// &&QListVoid;
	InsertList$JumpTable[qMetaTypeId<QList<QRegularExpression>>() - MinUserTypeId] = &&QListRegularExpression;
	InsertList$JumpTable[qMetaTypeId<QList<QJsonValue>>() - MinUserTypeId] = &&QListJsonValue;
	InsertList$JumpTable[qMetaTypeId<QList<QJsonObject>>() - MinUserTypeId] = &&QListJsonObject;
	InsertList$JumpTable[qMetaTypeId<QList<QJsonArray>>() - MinUserTypeId] = &&QListJsonArray;
	InsertList$JumpTable[qMetaTypeId<QList<QJsonDocument>>() - MinUserTypeId] = &&QListJsonDocument;
	InsertList$JumpTable[qMetaTypeId<QList<QByteArrayList>>() - MinUserTypeId] = &&QListByteArrayList;
	InsertList$JumpTable[qMetaTypeId<QList<QPersistentModelIndex>>() - MinUserTypeId] = &&QListPersistentModelIndex;
	// &&QListNullptr:
	InsertList$JumpTable[qMetaTypeId<QList<quint8>>() - MinUserTypeId] = &&QListCborSimpleType;
	InsertList$JumpTable[qMetaTypeId<QList<QCborValue>>() - MinUserTypeId] = &&QListCborValue;
	InsertList$JumpTable[qMetaTypeId<QList<QCborArray>>() - MinUserTypeId] = &&QListCborArray;
	InsertList$JumpTable[qMetaTypeId<QList<QCborMap>>() - MinUserTypeId] = &&QListCborMap;
/*
	InsertList$JumpTable[qMetaTypeId<QList<QFont>>() - MinUserTypeId] = &&QListFont;
	InsertList$JumpTable[qMetaTypeId<QList<QPixmap>>() - MinUserTypeId] = &&QListPixmap;
	InsertList$JumpTable[qMetaTypeId<QList<QBrush>>() - MinUserTypeId] = &&QListBrush;
	InsertList$JumpTable[qMetaTypeId<QList<QColor>>() - MinUserTypeId] = &&QListColor;
	InsertList$JumpTable[qMetaTypeId<QList<QPalette>>() - MinUserTypeId] = &&QListPalette;
	InsertList$JumpTable[qMetaTypeId<QList<QIcon>>() - MinUserTypeId] = &&QListIcon;
	InsertList$JumpTable[qMetaTypeId<QList<QImage>>() - MinUserTypeId] = &&QListImage;
	InsertList$JumpTable[qMetaTypeId<QList<QPolygon>>() - MinUserTypeId] = &&QListPolygon;
	InsertList$JumpTable[qMetaTypeId<QList<QRegion>>() - MinUserTypeId] = &&QListRegion;
	InsertList$JumpTable[qMetaTypeId<QList<QBitmap>>() - MinUserTypeId] = &&QListBitmap;
	InsertList$JumpTable[qMetaTypeId<QList<QCursor>>() - MinUserTypeId] = &&QListCursor;
	InsertList$JumpTable[qMetaTypeId<QList<QKeySequence>>() - MinUserTypeId] = &&QListKeySequence;
	InsertList$JumpTable[qMetaTypeId<QList<QPen>>() - MinUserTypeId] = &&QListPen;
	InsertList$JumpTable[qMetaTypeId<QList<QTextLength>>() - MinUserTypeId] = &&QListTextLength;
	InsertList$JumpTable[qMetaTypeId<QList<QTextFormat>>() - MinUserTypeId] = &&QListTextFormat;
	InsertList$JumpTable[qMetaTypeId<QList<QMatrix>>() - MinUserTypeId] = &&QListMatrix;
	InsertList$JumpTable[qMetaTypeId<QList<QTransform>>() - MinUserTypeId] = &&QListTransform;
	InsertList$JumpTable[qMetaTypeId<QList<QMatrix4x4>>() - MinUserTypeId] = &&QListMatrix4x4;
	InsertList$JumpTable[qMetaTypeId<QList<QVector2D>>() - MinUserTypeId] = &&QListVector2D;
	InsertList$JumpTable[qMetaTypeId<QList<QVector3D>>() - MinUserTypeId] = &&QListVector3D;
	InsertList$JumpTable[qMetaTypeId<QList<QVector4D>>() - MinUserTypeId] = &&QListVector4D;
	InsertList$JumpTable[qMetaTypeId<QList<QQuaternion>>() - MinUserTypeId] = &&QListQuaternion;
	InsertList$JumpTable[qMetaTypeId<QList<QPolygonF>>() - MinUserTypeId] = &&QListPolygonF;
	InsertList$JumpTable[qMetaTypeId<QList<QColorSpace>>() - MinUserTypeId] = &&QListColorSpace;
	InsertList$JumpTable[qMetaTypeId<QList<QSizePolicy>>() - MinUserTypeId] = &&QListSizePolicy;
//*/
	goto begin; // ready
}
