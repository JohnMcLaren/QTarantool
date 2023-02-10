## QVariantModelWidget
This project contains classes for manipulating and displaying 
complex arbitrary `QVariant` data objects in the view of a tree or table.
The classes inherited from `QAbstractItemModel`, `QTreeView`, `QTableView`.

### Key Features:
- All data (if possible) is returned by the `'const QVariant &'` reference. 
	If the data source is **external**, the model stores information about the data structure but <u>not a copy</u> of it.
	An exception is a container (node) of type `QList<any type>`. In this case, the value of the node is copied to a temporary `QVariant` to return a reference to it.
- The data is also changed by reference. If the model data is changed, the data of the **original object** will be changed.
- Model handling is completely changed to the new `QVariantModelIndex` model data addressing type.
	`QModelIndex` support is kept only for compatibility with item widgets (`QTreeView/QTableView`).
	All data in a model view is a Tree (of the `[key]=value` type) and is addressed by a multi-key `QVariantModelIndex` of the form
	`[key : key : key]` - where `key` can be a string or an integer which is the key of the corresponding `node`.
	Since the node data is not stored in the model, the index is "unwound" for access whenever it is accessed.
- The model supports 6 types of containers (nodes). Nodes of type `QUIntMap` and `QUIntHash` were added to support `MsgPack` integer keys.
```
	enumerated node key types(simple arrays):
		* QList<any>   : `key` is integer, `data` is <any> type
		* QVariantList : `key` is an integer, `data` is QVariant
	non enumerated node key types:
		* QVariantMap  : `key` is string, `data` is QVariant
		* QUIntMap     : `key` is an integer, `data` is QVariant
		* QVariantHash : `key` is string, `data` is QVariant
		* QUIntHash    : `key` is an integer, `data` is QVariant
```
- The model has a built-in data change signaling/handling system. 
	If different models are connected to the same source data object 
	then when data changes, the model can notify the linked models about it.
	Thus all linked widgets show the current state of the same data object in the same way.
	Models can be connected not only by a "star" scheme, but also by a "chain".
- At this point, model-based widgets of the Tree/Table type are implemented.
- Native cross-platform - Linux / Windows.

### Limitations:
0. If an **external data source** is used, it **should exist** as long as the model/widget is in use.
1. The model allows the values of the data to change externally. **But not their structure!**  
	Number of nodes, names and number of keys of nodes **should not** change externally, otherwise model's behavior is undefined.
	For correct change of model data use methods of the model/widget - `setData() / insertData() / removeData()`.
2. The model is **not thread-safe**. The model has no thread blocks, do not try to read/write data from different threads at the same time.
3. There are no data `View` decorations in the widgets. 
	Sort order, background/text colors, key icons - all these attributes of data view 
	<u>require additional structure</u> for their storage which isn`t realized in the widgets.
    
### Minimal usage examples:
1. Using the Tree and Table widgets on the same data. Widgets are not linked by data update signals. The data is copied to the internal data source of the model during initialisation. That is, the original `varTable` object will not be changed if the model data changes:
```c++
#include "../qvarianttablewidget.h"
#include "../qvarianttreewidget.h"

int main(int argc, char *argv[])
{
QApplication app(argc, argv);
QVariant varTable {QVariantList { // rows
		QVariantList {"(top-left)", "(0,1)", "(0,2)", "(0,3)", "(top-right)"},		// columns of row 0
		QVariantList {"(1,0)", "(0,1)", "(1,2)", "(1,3)", "(1,4)"},					// columns of row 1
		QVariantList {"(bottom-left)", "(2,1)", "(2,2)", "(2,3)", "(bottom-right)"}  // columns of row 2
	}};
QVariantTableWidget table(varTable); // init by varTable object
QVariantTreeWidget tree(varTable); // init by varTable object

	table.show();
	tree.show();

	table.setData({1, 2}, "center of Table"); // set new data to [row =1, column =2]
	tree.setData({1, 2}, "center of Tree"); // set new data to [row =1, column =2]

return(app.exec());
}
```
2. An example where the Tree and Table widgets refer to the same external data source:
```c++
	table.show();
	tree.show();

	table.setDataSource(varTable); // set up a reference to an external data source
	table.setReadOnly(false); // allow manual editing of values.
	tree.setDataSource(varTable);
	tree.setReadOnly(false);
	tree.connectToChanges(&table); // As both widgets use the same external data source, they can be interconnected by data change signals.

	table.setData({1, 2}, "center of Table");
	qDebug() << varTable;
	tree.setData({1, 2}, "center of Tree");
	qDebug() << varTable;

```
3. If you don`t need to display data, you can only use the model for manipulation with the data of the object:
```c++
QJsonDocument jdoc =QJsonDocument::fromJson("{\"isbn\": \"\",  \"editor\": {\"lastname\": \"Smith\", \"firstname\": \"Jane\"}, \"title\": \"The Ultimate Database Study Guide\", \"category\": [\"Non-Fiction\", \"Technology\"]}");
QVariantModel model(jdoc.toVariant());

	qDebug() << jdoc.toJson(QJsonDocument::Indented).data();

	model.setData({"isbn"}, "123-456-222");
	model.insertData({"author"}, QVariantMap {{"lastname", "Doe"}, {"firstname", "Jane"}});
	model.removeData({"editor"});

	qDebug() << QJsonDocument(model.data({}).toJsonObject()).toJson(QJsonDocument::Indented).data();
```
### Add QVariantModelWidget to your project (QtCreator):
```
1. Add Directory "../QVariantModelWidget"
    "Add Existing Directory..." > 
      "Browse..." > 
        "Start Parsing" > 
          "Select files matching: *.cpp; *.h;" > 
            "Apply Filters" >
              "OK"

2. Add qvarianttablewidget.h/qvarianttreewidget.h headers to your source file
```
