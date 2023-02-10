/************************************************************************************************
 *
 * src: https://github.com/JohnMcLaren/QVariantModelWidget
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.08.2022 🄯 JML
************************************************************************************************/
#include "qvarianttablewidget.h"
#include "qvarianttreewidget.h"
#include <QLayout>
/******************************************************************************************
 * Constructors
******************************************************************************************/
QVariantTableWidget::QVariantTableWidget(QWidget *parent) :
	QTableView(parent)
{
	init();
}

QVariantTableWidget::QVariantTableWidget(const QStringList &headers, QWidget *parent) :
	QTableView(parent)
{
	init(headers);
}

QVariantTableWidget::QVariantTableWidget(const QVariant &rootValue, const QStringList &headers, QWidget *parent) :
	QTableView(parent)
{
	init(headers);
	setData({}, rootValue);
}
/******************************************************************************************
 * Full widget initialization
******************************************************************************************/
void
QVariantTableWidget::init(const QStringList &headers)
{
	setStyleSheet("QWidget {font: 'Arial'; font-size: 10pt;}");
	//setStyleSheet("QTableView::item {padding: 15px;}");
	//setContentsMargins(0, 0, 0, 0);
	model.Mode =QVM::Table;
	connect(&model, &QVariantModel::updateColumns, this, &QVariantTableWidget::updateColumns);
	connect(&model, &QVariantModel::dataWillBeChanged, this, &QVariantTableWidget::dataWillBeChanged); // signal to signal

	setModel(&model);
	setHeaders(headers);
}
/******************************************************************************************
 * Setting the root node, which will be the root of the visible table.
******************************************************************************************/
bool
QVariantTableWidget::setRootView(const QVariantModelIndex &index)
{
QModelIndex rootView;

	if(!QVM::isValid(model.data(index, nullptr, false, &rootView)))
		return(false);

	// set columns number for view as length of longest row
	model.RootViewIndex =index; // store root view index in model
	updateColumns();

	setRootIndex(rootView);

return(true);
}
/******************************************************************************************
 * Number of columns in the table is equal to the number of columns in the longest row of the table
 * or the number of horizontal headers if they are set.
******************************************************************************************/
void
QVariantTableWidget::updateColumns(const int number)
{
	// headers.size != 0 - Tree/Table: columns =headers.size
	if(number)
		model.columns =number;
	else
		model.columns =model.headers.size(); // [FIXME] - headers / real columns

	if(!model.columns) // if headers.size =0 - Table: columns =max row length
	{ // In Table mode, the number of columns is the size(length) of the node.
	const QVariant &nodeRootView =data(model.RootViewIndex);

		for(int r =0, rows =lengthNode(nodeRootView), nodeSize; r < rows; r++) // searching for the longest node
			if((nodeSize =lengthNode(model.data(nodeRootView, r))) > model.columns)
				model.columns =nodeSize;
	}
}
/******************************************************************************************
 * Cross-connection of change signals between models of widgets.
******************************************************************************************/
bool QVariantTableWidget::connectToChanges(const QVariantTableWidget *tableWidget) const
{
	if(tableWidget != this)
		return(model.connectToChanges(&tableWidget->model));

return(false);
}

bool QVariantTableWidget::connectToChanges(const QVariantTreeWidget *treeWidget) const
{
	return(model.connectToChanges(&treeWidget->model));
}
