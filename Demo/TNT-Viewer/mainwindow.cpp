/******************************************************************
 * TNT-Viewer Qt-client for Tarantool database.
 *
 * src: https://github.com/JohnMcLaren/QTarantool
 * license: GPLv3 2007 - https://www.gnu.org/licenses/gpl-3.0.html
 * 15.11.2022 🄯 JML
******************************************************************/
#include <QApplication>
#include "mainwindow.h"

using namespace QTNT;
/******************************************************************************************
 * MainWindow
******************************************************************************************/
MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setTheme();
	makeMainWindow();

	connect(&tnt, &QTarantool::signalConnected, this, &MainWindow::on_Connected);
	connect(&tnt, &QTarantool::error, this, &MainWindow::on_TarantoolError);
	connect(treeSummary, &QVariantTreeWidget::clicked, this, &MainWindow::on_TreeSummaryClicked);
	connect(tableDetail, &QVariantTableWidget::dataWillBeChanged, this, &MainWindow::on_TableDataWillBeChanged);
	connect(frmGetSpaceData, &FormGetData::clickedGet, this, &MainWindow::on_GetClicked);
	connect(frmSetSpaceData, &FormSetData::clickedSet, this, &MainWindow::on_SetClicked);
	connect(&cmdConnect, &QPushButton::clicked, this, [&] () {

		if(tnt.isConnected())
			tnt.disconnectServer();
		else
			connectToServer();
	});
	connect(&cmdPing, &QPushButton::clicked, this, &MainWindow::on_PingClicked);
	connect(&cmdTest, &QPushButton::clicked, this, &MainWindow::on_TestClicked);

	treeSummary->setHeaders({"name", "value"});
	treeSummary->setDataSource(mapSummary);

	treeServerInfo->setHeaders({"param", "value"});

	tableDetail->setDataSource(mapSummary);
	tableDetail->connectToChanges(treeSummary);
	tableDetail->setAlternatingRowColors(true);

	tableDetail->setReadOnly(false);
	treeSummary->setReadOnly(false);

	statusBar->showMessage(tr("* status *"));
}
/******************************************************************************************
 * Connect to Tarantool server
******************************************************************************************/
void
MainWindow::connectToServer(const QString &host, const QString &user, const QString &password)
{
// [FIXME] qt.qpa.xcb: QXcbConnection: XCB error: 3 (BadWindow), sequence: 2651, resource id: 9615554, major code: 40 (TranslateCoords), minor code: 0
FormNetLogin loginForm(this);

	if(loginForm.exec())
		if(tnt.connectToServer(loginForm.Host))
			if(tnt.login(loginForm.User, loginForm.Password))
			{
				updateSummary();
				updateServerInfo();
			}
			else
				tnt.disconnectServer();
}
/******************************************************************************************
 * on Tarantool connected event
******************************************************************************************/
void
MainWindow::on_Connected(const bool bConnected)
{
	if(bConnected)
	{
		reset();
		cmdConnect.setText("Disconnect");
	}
	else
	if(!IsExitState)
	{
		cmdConnect.setText("Connect");
	}
}
/******************************************************************************************
 *
******************************************************************************************/
void
MainWindow::on_TreeSummaryClicked(const QModelIndex &index)
{
	if(!index.internalPointer())
		return;

QVM::NodeIndex nodeIndex = *(QVM::NodeIndex *)index.internalPointer();

	if(!nodeIndex.index.size() || nodeIndex.index[0] != "Spaces")
		return;

	if(nodeIndex.index.size() < 2) // [FIXME] Space index
		return;
	else
		nodeIndex.index.truncate(2); // Trunc to the name of the Space.

	if(nodeIndex.index.last().toString() == CurrentSpaceName)
		return; // Space is the same

	CurrentSpaceName =nodeIndex.index.last().toString();

	tableDetail->setHeaders(treeSummary->data(nodeIndex.index + "fields").toStringList());
	tableDetail->setRootView(nodeIndex.index + "DATA");
	tableDetail->resizeColumnsToContents();

QStringList indexNames;

	for(const auto &i : treeSummary->data((nodeIndex.index + "index")).toList())
		indexNames +=i.toMap()["name"].toString();

	frmGetSpaceData->setIndexNames(indexNames);
	boxControl.setTitle("Control - \"" + CurrentSpaceName + "\"");
	boxData.setTitle("Data - \"" + CurrentSpaceName + "\"");
}
/******************************************************************************************
 *
******************************************************************************************/
void
MainWindow::on_TableDataWillBeChanged(const QVariantModelIndex &index, const QVariant &data, bool &AllowDataChanges)
{
	try
	{
	const QVariantList &tuple =reinterpret_cast<const QVariantList &>(tableDetail->data(index.parent())); // get full row of table as tuple
	Key key;
		// Construct key of index for tuple
		if(tuple.size())
			for(const auto &part : tableDetail->data({"Spaces", CurrentSpaceName, "index", 0, "parts"}).toList()) // get list parts of index
			{
			uint i =reinterpret_cast<const QVariantList &>(part)[0].toUInt(); // get index of part in tuple

				if(i < (uint)tuple.size())
					key +=tuple[i]; // append part value of index from tuple to key
				else
					throw(0);
			}
		else
			throw(0);

		if(!tnt.changeData(CurrentSpaceName, key, index.last().toUInt() + 1, data)) // '+1' : In Lua, field indexes begin from 1 instead of 0.
			throw(0);

		AllowDataChanges =true;
	}
	catch(int)
	{
		AllowDataChanges =false;
	}
}
/******************************************************************************************
 *
******************************************************************************************/
void
MainWindow::on_GetClicked(const QString &indexName, const int iOperatorFrom, const QString &keyFrom, const int iOperatorTo, const QString &keyTo)
{
	if(indexName.isEmpty())
		return;

	statusBar->showMessage("");

const QVariant &data =tnt.getData(CurrentSpaceName, {(OPERATOR)iOperatorFrom, Key(keyFrom), indexName}, {(OPERATOR)iOperatorTo, Key(keyTo)});

	tableDetail->setData({"Spaces", CurrentSpaceName, "DATA"}, data);
	tableDetail->resizeColumnsToContents();
}
/******************************************************************************************
 *
******************************************************************************************/
void
MainWindow::on_SetClicked(const QString &tuple)
{
const QVariantList &data =Key(tuple); // string to tuple

	if(tnt.setData(CurrentSpaceName, data, false)) // 'false' - set data anyway
		frmGetSpaceData->clickGet();
}
/******************************************************************************************
 *
******************************************************************************************/
void MainWindow::on_PingClicked()
{
	statusBar->showMessage(tr("Ping: %1 uS").arg(tnt.ping() / 1000), 30000); // Server ping in microseconds
}
/******************************************************************************************
 * Update Tarantool info
******************************************************************************************/
void
MainWindow::updateSummary()
{
const QVariantList listSpaces =tnt.spaces();
const QUIntMap mapIndexes =tnt.indexes();
QVariantMap mapSpaces;
bool bSkipSystemSpaces =false;

	if(!listSpaces.size())
		return;

	for(const auto& s : listSpaces)
	{
	const QVariantList &attr =s.toList();
	const uint spaceId =attr[0].toUInt();
	const QString spaceName =attr[2].toString();
	const bool IsIndexExist =mapIndexes.value(spaceId).isValid();
	QStringList fields;

		if(attr.size() != 7) // unexpected format
			continue;

		if(bSkipSystemSpaces && spaceName[0] == '_') // system space names start with '_'
			continue;

		for(const auto &f : attr[6].toList()) // tuples fields names
			fields.append(f.toMap().value("name").toString());

		mapSpaces[spaceName] =QVariantMap { // spaces[name] =attributes

			{"id", spaceId},
			{"owner", attr[1]},
			{"engine", attr[3]},
			{"fields", fields},
			{"length", tnt.getSpaceLength(spaceName)},
			{"size", tnt.getSpaceSize(spaceName)},
			{"index", mapIndexes.value(spaceId)},
			{"DATA", IsIndexExist ? tnt.getData(spaceName, {}) : QVariant {}}
		};
	}

const QVariantList listUsers =tnt.users();
QVariantMap mapUsers;

	for(const auto& u : listUsers)
	{
	const QVariantList &attr =u.toList();

		mapUsers[attr[2].toString()] =QVariantMap { // users[name] =attributes

			{"id", attr[0]},
			{"type", attr[3]},
			{"attributes", attr[4]},
			{"grants", (attr[3].toString() == "user") ? tnt.grants(attr[2].toString()) : QVariantList {}} // if type == 'user' get grants
		};
	}

	treeSummary->setData({}, QVariantMap {{"Spaces", mapSpaces}, {"Users", mapUsers}});
	//treeSummary->setData({}, QVariantMap {{"Spaces", listSpaces}, {"Users", listUsers}}); // raw server reply
}
/******************************************************************************************
 *
******************************************************************************************/
void
MainWindow::updateServerInfo()
{
	treeServerInfo->setData({}, Map {

		{"Config", tnt.cfg()},
		{"Slab", Map {{"Aggregated", tnt.slab(SLAB::INFO)},
					{"Detail", tnt.slab(SLAB::DETAIL)},
					{"Runtime", tnt.slab(SLAB::RUNTIME)}}},
		{"Info", tnt.info()},
		{"Stat", Map {{"Requests", tnt.stat(STAT::REQUESTS)},
					{"Network", tnt.stat(STAT::NETWORK)},
					{"Vinyl", tnt.stat(STAT::VINYL)}}}
	});
}
/******************************************************************************************
 *
******************************************************************************************/
void
MainWindow::makeMainWindow()
{
	// create window widgets
	frmGetSpaceData = new FormGetData("Get space data", this);
	frmSetSpaceData = new FormSetData("Set space data", this);
	treeSummary = new QVariantTreeWidget(this);
	treeServerInfo = new QVariantTreeWidget(this);
	tableDetail = new QVariantTableWidget(this);

	// set window widgets
	setObjectName(QString::fromUtf8("MainWindow"));
	centralWidget = new QWidget(this);
	centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
	setCentralWidget(centralWidget);
	statusBar = new QStatusBar(this);
	statusBar->setObjectName(QString::fromUtf8("statusBar"));
	setStatusBar(statusBar);

	resize(600, 400);

	layMain = new QGridLayout;
	splitLeft = new QSplitter(Qt::Vertical);
	splitRight = new QSplitter(Qt::Vertical);
	splitCenter = new QSplitter(Qt::Horizontal);
	// Summary box
	boxSummary.setLayout(new QVBoxLayout);
	((QVBoxLayout *)boxSummary.layout())->addWidget(treeSummary);
	// Server Info box
	boxServerInfo.setLayout(new QVBoxLayout);
	((QVBoxLayout *)boxServerInfo.layout())->addWidget(treeServerInfo);
	// Edit box
//	boxEdit.setLayout(new QVBoxLayout);
//	((QVBoxLayout *)boxEdit.layout())->addWidget(&txtEditor);
	// Control box
	boxControl.setLayout(new QGridLayout);
	boxControl.layout()->setContentsMargins(2, 0, 2, 0);
	((QGridLayout *)boxControl.layout())->addWidget(frmGetSpaceData, 0, 0, 3, 4);
	((QGridLayout *)boxControl.layout())->addWidget(frmSetSpaceData, 0, 5, 3, 4);
	((QGridLayout *)boxControl.layout())->addItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 9, 3, 4);
	((QGridLayout *)boxControl.layout())->addWidget(&cmdConnect, 0, 15, 1, 1);
	((QGridLayout *)boxControl.layout())->addWidget(&cmdPing, 1, 15, 1, 1);
	((QGridLayout *)boxControl.layout())->addWidget(&cmdTest, 2, 15, 1, 1);
	boxControl.setFixedHeight(120);
	// Details box
	boxData.setLayout(new QVBoxLayout);
	((QVBoxLayout *)boxData.layout())->addWidget(tableDetail);
	// set splitters
	splitLeft->addWidget(&boxSummary);
	splitLeft->addWidget(&boxServerInfo);
	//splitRight->addWidget(&boxEdit);
	splitRight->addWidget(&boxControl);
	splitRight->addWidget(&boxData);
	splitCenter->addWidget(splitLeft);
	splitCenter->addWidget(splitRight);
	splitCenter->setStretchFactor(1, 100); // set the width of the left part - "lazy".
	// set layers
	layMain->addWidget(splitCenter);
	centralWidget->setLayout(layMain);
	// mainwindow menu
	menuBar()->addMenu(tr("?"))->addAction("about Qt", [&] () { QApplication::aboutQt(); });
}
/******************************************************************************************
 *
******************************************************************************************/
void
MainWindow::setTheme()
{
	setStyle(QStyleFactory::create("Fusion"));

// https://colorswall.com/palettes
QColor color                       ="#d6d6d6",
		background_color           ="#212022",
		selection_color            ="#E0E1E3",
		selection_background_color ="#3d397d";

	setStyleSheet(
		"QWidget {"
		+ tr("color: %1;").arg(color.name())
		+ tr("background-color: %1;").arg(background_color.name())
		+ tr("alternate-background-color: %1;").arg(background_color.lighter(115).name()) // +15% lighter
		+ tr("selection-color: %1;").arg(selection_color.name())
		+ tr("selection-background-color: %1;").arg(selection_background_color.name())

		+ "font-family: 'Arial'; font-size: 10pt"
		"}"
		"QLineEdit {"
		+ tr("background-color: %1;").arg(background_color.darker(120).name())

		+ "border: none;"
		"}");
}
/******************************************************************************************
 * DEBUG
 * A place to test your ideas.
******************************************************************************************/
void
MainWindow::on_TestClicked()
{
	qDebug() << "TODO";
}
//----------------------------------------

