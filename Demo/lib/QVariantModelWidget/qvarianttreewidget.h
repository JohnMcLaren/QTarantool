/************************************************************************************************
 *
 * src: https://github.com/JohnMcLaren/QVariantModelWidget
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.08.2022 🄯 JML
************************************************************************************************/
#pragma once

#include <QTreeView>
#include "qvariantmodel.h"
#include <QDebug>

class QVariantTableWidget;

class QVariantTreeWidget : public QTreeView
{
	Q_OBJECT

	friend class QVariantTableWidget;

public:
	QVariantTreeWidget(QWidget *parent =nullptr);
	QVariantTreeWidget(const QStringList &headers, QWidget *parent =nullptr);
	QVariantTreeWidget(const QVariant &rootValue, const QStringList &headers ={}, QWidget *parent =nullptr);

	// Sets the reference to the real data in the associated widget model.
	inline bool
	setDataSource(QVariant &rootValue)
	{
	bool success =model.setDataSource(rootValue);

		setRootView({}); // update view

	return(success);
	}
	// Gets the value from the real data associated with the model by VariantModelIndex.
	inline const QVariant &
	data(const QVariantModelIndex &index) const { return(model.data(index)); };
	// Set a new data to node/value by VariantModelIndex.
	inline bool
	setData(const QVariantModelIndex &index, const QVariant &data)
	{
		if(!model.setData(index, data))
			return(false);

		if(!index.size())
			updateGeometries(); // [QT-BUG] force update on root index - special case

	return(true);
	}
	// Inserts a new key of node with data by VariantModelIndex.
	inline bool
	insertData(const QVariantModelIndex &index, const QVariant &data)
	{
		if(!model.insertData(index, data))
			return(false);

		if(!index.size())
			updateGeometries(); // [QT-BUG] force update on root index - special case

	return(true);
	}
	// Deletes the existing key/node along with the data by VariantModelIndex.
	inline bool
	removeData(const QVariantModelIndex &index) { return(model.removeData(index)); }
	// Sets horizontal headers of this widget
	inline void
	setHeaders(const QStringList &headers)
	{
		model.setHeaders(headers);

		if(headers.size())
			setHeaderHidden(false);
		else
			setHeaderHidden(true);
	}
	// Setting the root node, which will be the root of the visible tree.
	bool
	setRootView(const QVariantModelIndex &index);
	//
	inline void
	setReadOnly(const bool readonly) { model.ReadOnly =readonly; }
	// Cross-connection of change signals between models of widgets.
	bool
	connectToChanges(const QVariantTreeWidget *treeWidget) const;
	bool
	connectToChanges(const QVariantTableWidget *tableWidget) const;
	// disconnect [FIXME]

public slots:
	// Incoming slot
	inline void
	updateView(const QVariantModelIndex &index ={})
	{
		model.updateView(index);
	}
	// Receives a request from the model to update the number of columns
	void
	updateColumns();

private:

	QVariantModel model;
	bool bBlockUpdateIndex =false; // For the duration of manual editing, the index must be blocked from event changes.
	QModelIndex indexBlockUpdate;

	void
	init(const QStringList &headers ={});
	// Block updating the index value for manual edit.
	inline void
	blockIndexUpdate(const QModelIndex &modelIndex)
	{
		bBlockUpdateIndex =true;
		indexBlockUpdate =modelIndex;
	}
	inline void
	unblockIndexUpdate()
	{
		bBlockUpdateIndex =false;
	}
	inline bool
	isBlockUpdateIndex(const QModelIndex &modelIndex) const
	{
		if(bBlockUpdateIndex)
			if(!modelIndex.isValid() || modelIndex == indexBlockUpdate)
				return(true);

	return(false);
	}

protected:
	// Update self 'View'
	inline void
	dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override
	{
		if(isBlockUpdateIndex(topLeft))
			return;

		QTreeView::dataChanged(topLeft, bottomRight, roles);
	}
	// Methods to block/unblock external changes on manual editing of a tree/table value.
	// On enter of manual edit mode.
	inline bool
	edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) override
	{
		if(!index.isValid()) // The root index cannot be changed manually.
			return(false);

		if(trigger == QAbstractItemView::DoubleClicked)
			blockIndexUpdate(index);

	return(QAbstractItemView::edit(index, trigger, event));
	}
	// On exit manual editing mode
	inline void
	closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint) override
	{
		if(hint)
			QAbstractItemView::commitData(editor);

		QAbstractItemView::closeEditor(editor, hint);
		unblockIndexUpdate();
	}

	inline void
	commitData(QWidget *editor) override { }

signals:
	void dataWillBeChanged(const QVariantModelIndex &index, const QVariant &data, OUT bool &AllowDataChanges);
};
