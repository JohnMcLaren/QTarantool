/************************************************************************************************
 *
 * src: https://github.com/JohnMcLaren/QVariantModelWidget
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.08.2022 🄯 JML
************************************************************************************************/
#include "qvarianttreewidget.h"
#include "qvarianttablewidget.h"
/******************************************************************************************
 * Constructors
******************************************************************************************/
QVariantTreeWidget::QVariantTreeWidget(QWidget *parent) :
	QTreeView(parent)
{
	init();
}

QVariantTreeWidget::QVariantTreeWidget(const QStringList &headers, QWidget *parent) :
	QTreeView(parent)
{
	init(headers);
}

QVariantTreeWidget::QVariantTreeWidget(const QVariant &rootValue, const QStringList &headers, QWidget *parent) :
	QTreeView(parent)
{
	init(headers);
	setData({}, rootValue);
}
/******************************************************************************************
 * Full widget initialization
******************************************************************************************/
void
QVariantTreeWidget::init(const QStringList &headers)
{
	setStyleSheet("QWidget {font: 'Arial'; font-size: 10pt}");
	setAnimated(false);
	setIndentation(12);
	//setHeaderHidden(true);
	connect(&model, &QVariantModel::updateColumns, this, &QVariantTreeWidget::updateColumns);
	connect(&model, &QVariantModel::dataWillBeChanged, this, &QVariantTreeWidget::dataWillBeChanged); // signal to signal

	setModel(&model);
	setHeaders(headers);

}
/******************************************************************************************
 * Setting the root node, which will be the root of the visible tree.
******************************************************************************************/
bool
QVariantTreeWidget::setRootView(const QVariantModelIndex &index)
{
QModelIndex rootView;

	if(!QVM::isValid(model.data(index, nullptr, false, &rootView)))
		return(false);

	// set columns for view
	model.RootViewIndex =index; // store root view index in model
	updateColumns();

	setRootIndex(rootView);

return(true);
}
/******************************************************************************************
 * The number of columns in the tree - constant.
******************************************************************************************/
void QVariantTreeWidget::updateColumns()
{
	// headers.size != 0 - Tree/Table: columns =headers.size
	model.columns =model.headers.size();

	if(!model.columns) // if headers.size =0 - Tree: columns =1
		model.columns =1;
}
/******************************************************************************************
 * Cross-connection of change signals between models of widgets.
******************************************************************************************/
bool QVariantTreeWidget::connectToChanges(const QVariantTreeWidget *treeWidget) const
{
	if(treeWidget != this)
		return(model.connectToChanges(&treeWidget->model));

return(false);
}

bool QVariantTreeWidget::connectToChanges(const QVariantTableWidget *tableWidget) const
{
	return(model.connectToChanges(&tableWidget->model));
}




