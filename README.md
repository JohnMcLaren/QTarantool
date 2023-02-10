## QTarantool
`Qt/C++ connector` for [Tarantool][tarantool-src] database.
Implemented the basic methods of managing the base and getting statistical information from the Tarantool server.
The connector methods and a description of how they work can be found in the [`API`](#qtarantool-api) section of this document.
In the [Demo][demo-url] directory of examples of how to use the connector, you will find - [TNT-Viewer][demo-viewer-url], which can be used to view and make some changes to the **existing** `Tarantool` database.

## Tarantool
##### ![tg-icon] [EN][tg-url] [RU][tg-ru-url] --- [# Sources][tarantool-src] --- [# Documentation][tarantool-doc] --- [# Releases][tarantool-release]

`Tarantool` is a `Lua` application server that includes a resident `NoSQL` database,
which has high speed compared to traditional RDBMS with the same properties:
persistence, ACID transactivity, replications - master-slave, master-master.
[more info..][tarantool-doc-ov-en]

## QTarantool API
> [Server](#server) [User](#user) [Space](#space) [Index](#index) [Service](#service) [+++](#add)

All connector methods are `blocking`. 
Control will be returned if a server reply is received (with `any status`) or by `timeout`.

---
#### Server

*   **connectToServer**(const QString &uri)

|| type | brief |
|-|-|-|
return | bool | Current server connection status |
uri | QString | The `URI` server string in the format: `protocol://location:port`
Connects to the `Tarantool` server.  It also generate `signalConnected( bool )` signal. If there was an established server connection at the time the method was called, it connection will be disconnected.

```c++
#include "../../src/qtarantool.h"
using namespace QTNT;

QTarantool tnt;

qDebug() << tnt.connectToServer("http://localhost:3301");
```

*   **disconnectServer**()

Disconnects the previously established connection to the `Tarantool` server. 
Also generates `signalConnected( bool )` signal.

```c++
qDebug() << tnt.disconnectServer();
```

*   **isConnected**()

|| type | brief |
|-|-|-|
return | bool | Current server connection status |

```c++
if(tnt.isConnected())
{
	// TODO
}
```

*   **ping**()

|| type | brief |
|-|-|-|
return | qint64 | Measured server reply time in `nanoseconds' |

```c++
qDebug("server ping: %lld nS", tnt.ping());
```

*   **cfg**()

|| type | brief |
|-|-|-|
return | QVariantMap | All **non-zero** server `Tarantool` configuration parameters that [`box.cfg`][box-cfg-url] returns |

```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.cfg())
							.toJson(QJsonDocument::Indented)
							.data());
```

*   **slab**(const SLAB type)

|| type | brief |
|-|-|-|
return | QVariantMap | Information on memory usage by the server. [Read more...][slab-url] |
`type` | `SLAB::INFO` | Show aggregated memory usage report |
|| `SLAB::DETAIL` | Show detailed memory usage report |
|| `SLAB::RUNTIME` | Show memory usage report for the Lua runtime environment |

```c++
qDebug("%s", QJsonDocument::fromVariant(QVariantMap {
						{"slab::info", tnt.slab(SLAB::INFO)},
						{"slab::detail", tnt.slab(SLAB::DETAIL)},
						{"slab::runtime", tnt.slab(SLAB::RUNTIME)}})
							.toJson(QJsonDocument::Indented)
							.data());
```

*   **info**()

|| type | brief |
|-|-|-|
|return | QVariantMap | All server status information returned by the [`box.info'][box-info-url] module |

```c++
	qDebug("%s", QJsonDocument::fromVariant(tnt.info())
			.toJson(QJsonDocument::Indented)
			.data());
```

*   **stat**(const STAT type)

|| type | brief |
|-|-|-|
return | QVariantMap | Statistics on server network usage and requests. [Read more...][stat-url]|
`type` | `STAT::REQUESTS` | Shows the total number of requests since start-up and the average number of requests per second |
|| `STAT::NETWORK` | Shows statistics for network requests |
|| `STAT::VINYL` | Shows the activity of the `Vinyl`-engine | 

```c++
qDebug("%s", QJsonDocument::fromVariant(QVariantMap {
					{"stat::requests", tnt.stat(STAT::REQUESTS)},
					{"stat::network", tnt.stat(STAT::NETWORK)},
					{"stat::vinyl", tnt.stat(STAT::VINYL)}})
				.toJson(QJsonDocument::Indented)
				.data());
```

*   **getServerDirectory**()

|| type | brief | notes |
|-|-|-|-|
return | QString | Server working directory | The one marked `'.'` in the `box.cfg` configuration |

```c++
qDebug() << tnt.getServerDirectory();
```

*   **startLocalServer**(const QString &filename)
> not implemented

*   **stopLocalServer**()
> not implemented
---
#### User

*  **login**(const QString &userName, const QString &password)

|| type | brief |
|-|-|-|
return | bool | Authentication result |
userName | QString | User name |
password | QString | User password |

The authentication procedure is mandatory once a connection to the server has been established. Empty password is only allowed for the `guest` user.

```c++
if(tnt.login("Alice", "alice-password"))
	qDebug("Auth Success!");
else
	qDebug("Auth Failed.");
```

*  **logout**()

|| type | brief |
|-|-|-|
return | bool | Authentication result as `guest` user |
Closes the current user's session and login as `guest`.

```c++
tnt.logout();
```

*   **getUserName**()

|| type | brief | notes |
|-|-|-|-|
return | QString | Current user name | the current user must have `execute` privilege to execute this request |

The current user name will be **requested from the server**.

```c++
qDebug() << tnt.getUserName();
```

*  **createUser**(const QString &userName, const QString &userPassword)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of creating a new user | The current user must have `create` privilege to create new users. The user password is mandatory and cannot be empty. |
userName | QString | Name of new user | |
userPassword | const QString & | Password of new user | |

The method creates a new user with the specified password. If the user already existed the method will fail and return `false`.

```c++
qDebug() << tnt.createUser("Bob", "123");
```

*   **grantUser**(const QString &userName, const QString &userPrivileges, const QString &objectType, const QString &objectName)

|| type | brief | notes |
|-|-|-|-|
return | bool | The result of granting the user new privileges. If the user already had the specified privileges, the request will be ignored and the method will return `true`| The specified object **must exist**. Current user must have the privilege to modify the **specified object**. |
userName | QString | Name of the user who will be granted the new privileges | |
userPrivileges | QString | A string of comma-separated privileges to will be granted to the user | Valid values: <br> `read`, `write`, `execute`, `session`, `usage`, `create`, `drop`, `alter`, `reference`, `trigger`, `insert`, `update`, `delete` |
objectType | QString | The type of object for which the privileges are granted. | Valid values: <br> `'space'`, `'user'`, `'role'`, `'sequence'`, `'function'`, `'trigger'`, `'universe'` <br>(by default `'universe'`). <br> Values other than `null`/`nil` is necessarily enclosed in **single** quotes `'` |
objectName | QString | Object name | Arbitrary <br> (by default `null`). <br> Must be enclosed in **single** quotes `'` |

[More about user privileges ..][grants-url]
[Why the user **cannot** make requests like **EVAL**, i.e. - **all** of this connector ..][security-url]
```c++
// full access to 'Test' space for user 'Bob'
qDebug() << tnt.grantUser("Bob", "read,write,execute,create,drop", "'space'", "'Test'"); 
```

*   **grantUserByRole**(const QString &userName, const QString &userRole)

|| type | brief |
|-|-|-|-|
return | bool | Result of adding a new role to a user |
userName | QString | User name |
userRole | QString | Name of the **existing** new user role |

The method adds a new role to the user.

*   **resetGrants**(const QString &userName, const QString &newUserPrivileges, const QString &objectType, const QString &objectName)

> not implemented

* **deleteUser**(const QString &userName)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of deleting the specified user | If the user does not exist, the method will fail and return `false` |
userName | QString | User name |

Deletes the specified user.

```c++
qDebug() << tnt.deleteUser("Bob");
```


*  **isUserExist**(const QString &userName)

|| type | brief |
|-|-|-|
return | bool | `true` if the user exists, otherwise `false` |
userName | QString | User name |

Checks if a user with the specified name exists

```c++
qDebug() << tnt.isUserExist("John");
```

*   **user**()

|| type | brief | notes |
|-|-|-|-|
return | QString | Current user name | The name of the last successfully authenticated user stored by the `QTarantool` instance will be returned without requesting the `Tarantool` server |

```c++
qDebug() << tnt.user();
```

* **users**()

|| type | brief |
|-|-|-|
return | QVariantList | List of users, roles and their attributes |

Method returns list of users and roles containing lists of their attributes.
Equivalent to call `box.space._user:select{}`

```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.users())
							.toJson(QJsonDocument::Indented)
							.data());
```

*   **grants**(const QString &userName)

|| type | brief |
|-|-|-|
return | QVariantList | List of lists containing user privileges and types/names of objects to which these privileges apply |
userName | QString | User name |

```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.grants("Alice"))
							.toJson(QJsonDocument::Indented)
							.data());
```
---
 #### Space

*   **createSpace**(const QString &spaceName, const QStringList &options)

|| type | brief | notes |
|-|-|-|-|
return | uint | The identifier of the space created. `0` if failed. |
spaceName | QString | Name of the new space |
options | QStringList | List of **strings** with options for space creation. | Not mandatory. <br> Available options: <br> `engine`, `field_count`, `format`, `id`, `if_not_exists`, `is_local`, `is_sync`, `temporary`, `user` <br> [Подробнее..][space-create-url] |

Creates a new space with the specified name and optional parameters.

```c++
tnt.createSpace("Tester2", {"engine =memtx", 
							"format ={{'field1'}, {'field2'}, {'field3'}}"
							});
```
Possible variations of the [format option...][format-url]

*   **changeSpace**(const QString &spaceName, const QStringList &newParams)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of changing space parameters | Tarantool server version **>= 2.7** (Error: attempt to call method ''alter'' (a nil value)')
spaceName | QString | Name of Space |
newParams | QStringList | New Space Parameters. <br> Changeable parameters: <br> `field_count`, `user`, `format`, `temporary`, `is_sync`, `name` | The list of strings representing new parameter values has the same format as `options` for `createSpace`. |

Modifies some parameters of an existing space by a specified list of parameters and their values.

```c++
// create space with format
tnt.createSpace("Tester2", {"format ={{'field1'}, {'field2'}, {'field3'}, {'field4'}, {'field5'}}"});
// then, change format of space
tnt.changeSpace("Tester2", {"format ={{'F1'}, {'F2'}, {'F3'}}"});
```

* **clearSpace**(const QString &spaceName)

| | type | brief |
|-|-|-|
return | bool | Result of deleting a Space's **data** |
spaceName | QString | Space name |

Totally clears the specified space of **data**. All indexes and format of the space are **saved**.

```c++
qDebug() << tnt.clearSpace("Tester");
```

*  **deleteSpace**(const QString &spaceName)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of deleting the specified space | Method will fail if space with the specified name does not exist |
spaceName | QString | Space name |  |

Deletes the specified space.

```c++
qDebug() << tnt.deleteSpace("Tester");
```

*  **isSpaceExist**(const QString &spaceName)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of checking the existence of a space with the specified name | Does not fail with an error if the space does not exist |
spaceName | QString | Space name |

Checks if a space with the specified name exists.

```c++
qDebug() << tnt.isSpaceExist("Tester");
```

*   **spaces**()

|| type | brief | notes |
|-|-|-|-|
return | QVariantList | A list containing all attributes of all existing spaces. | The result of the call is fully equivalent to the Lua function `box.space._space:select{}` <br> Read more about the meaning of the [_space fields][_space-url]

The method returns a list of lists containing attributes of all existing database spaces, including system spaces. Returned values will contain only metadata of the spaces without data stored in them.
```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.spaces())
							.toJson(QJsonDocument::Indented)
							.data());
```

*   **getData**(const QString &spaceName, const Selector &selectorFrom, const Selector &selectorTo, const uint limit)

|| type | brief | notes |
|-|-|-|-|
return | QVariantList | List (array) of space tuples according to the specified selection criterions |
spaceName | QString | Space name |
selectorFrom | Selector | Structure determining the start of tuples selection. | Mandatory. <br> Also specifies the index in which tuples will be searched.
selectorTo | Selector | Structure determining the end of tuples selection. | Not mandatory. <br> By default, empty - the selection is not limited. |
limit | uint | Maximum quantity of tuples to return | Not mandatory. <br> By default =1000|

The method returns a list of tuples by selection conditions specified by `selectorFrom` / `selectorTo` and limited by `limit`. With this method, tuples cannot be searched by different indexes. The index specified in `selectorFrom` (by default `primary`) will be applied.

> **Keep in mind, that:**
	- The Selectors must be `counter-directional`
	- The `selectorTo` must be on the search direction `selectorFrom`. Otherwise the selection will end at the Space boundary before reaching the specified search boundary.

```c++
QVariant data = tnt.getData("sensors", Selector{"temperature", ">", 25}, Selector{"temperature", "<", 30}, 50);

// All data of Space "Tester". 
// Here the "Tester" space has a primary index named "primary":
	data =tnt.getData("Tester", {});

// Data of Space "Tester" - with 'primary' index >= 100 And <= 300:
	data =tnt.getData("Tester", {GE, {100}}, {LE, {300}});
// same data but in reverse sorting
	data =tnt.getData("Tester", {LE, {300}}, {GE, {100}});

	qDebug("%s", QJsonDocument::fromVariant(data)
								.toJson(QJsonDocument::Indented)
								.data());
```
* * **Selector** - a simple structure for specifying boundary conditions when selecting tuples from a space:
    ```c++
    struct Selector {
		OPERATOR Operator;
		IndexKey Key;
		QString  IndexName;
	};
    ```
    where, 
    > `Operator` - an enumeration of possible comparison operators:
    
    | Operator | Semantics | Symbol | Brief |
    |-|-|-|-|
    | ALL |  |  | Synonym for `GE` |
    | EQ | EQual | == | Exact match |
    | GE | Greater-or-Equal | >= | Greater than or equal to |
    | GT | Greater-Than | > | Greater |
    | LE | Less-or-Equal | <= | Less than or equal to (reverse direction) |
    | LT | Less-Than | < | Less (reverse direction) |
    | REQ | Reverse-EQual |  | Same as `EQ` but the reverse direction of search |
    
    > `Key` - a list of values of `particular types` of arbitrary length representing the key being searched for. Valid types of values in fields of key:  `<any number type>`, `boolean`, `string`.
   
    `IndexKey` as a inherit class of `QList` is initialized from a initialization list of the key being created or from a string representing it. An instance of a key object can be serialized to text using the `text()` instance method:
    ```c++
    IndexKey key {286, "test", "текст", -3.086};
	IndexKey key2("{286, \"test\", \"текст\", -3.086}");

		qDebug() << key.text();
		qDebug("%s", key.text().toUtf8().data());

		qDebug() << key2.text();
		qDebug("%s", key2.text().toUtf8().data());
		// output
		// "{286,\"test\",\"текст\",-3.086}"
		// {286,"test","текст",-3.086}
    ```
    Examples of simple keys on a single tuple index field:
	```
	{152}
	{"test value"}
	{3.14159265359}
	```
    multi-part keys for multi-part indexes:
    ```
    {333, 2.71828, "text", true}
    {29, 01, 2023}
    ```
	Multi-part keys with a complex structure -`Hierarchical`, not supported by `Tarantool`:
	```
	{500, {{{555, 666}, 666}, 777}, 888, {600, {700, {"800"}}}, 900}
	```
	> `IndexName` - the name of the index that will be searched for. Defaults is `primary`, but can be any `unique/non-unique` index. It is not necessary to enclose the index name in single quotes. 

*   **getData**(const QString &spaceName, const IndexKey &key, const uint field, const QString &indexName)

|| type | brief | notes |
|-|-|-|-|
return | `QVariant &` | Value of the tuple field | The return value is a constant reference.
spaceName | QString | Space name |
key | IndexKey | The tuple key in the `indexName` index |  |
field | uint | Field number of the tuple | Fields are numbered with `1` from the beginning of the tuple. Negative values are not allowed.
indexName | QString | Index name | Not mandatory <br> Name of index in which `key` will be searched for. Can be any **unique** index of space `spaceName`. |

Returns the value of the tuple field by the specified tuple key and field number.

```c++
qDebug() << tnt.getData("Tester", {5}, 3);
```

*   **setData**(const QString &spaceName, const QVariantList &tuple, const bool bIfExist =true)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of changing/adding a new tuple of data |
spaceName | QString | Space name |
tuple | QVariantList | New data tuple |
bIfExist | bool | `true` - change only **existing** tuple with new tuple key, otherwise error: "Key not found", <br> `false` - set in any case. <br> If an old tuple is found by new key then the new tuple will replace the old one, if the key does not exist then the specified tuple will be inserted as a new tuple. | By default `true`

Changes data of **existing** tuple, by **primary** index of new tuple, **or** inserts **new** tuple (see `bIfExist`).

```c++
// In this example it is expected that there is a 'Tester' space with a primary index.
qDebug() << tnt.setData("Tester", {1, "hello", true}, false);
```

*   **insertData**(const QString &spaceName, const QVariantList &tuple)

|| type | brief |
|-|-|-|
return | bool | Result of **adding** a new tuple of data |
spaceName | QString | Space name |
tuple | QVariantList | New tuple of data |

Adds a new tuple of data by primary tuple key. The tuple will be added if **does not exist** the old tuple with the primary index of the new one, otherwise the method will end with an error.

```c++
qDebug() << tnt.insertData("Tester", {2, "hello", true}); // OK, then
qDebug() << tnt.insertData("Tester", {2, "hello", true}); // Error
```
<a id="changeData1"></a>
*   **changeData**(const QString &spaceName, const IndexKey &key, const uint field, const QVariant &value, const QString &indexName) 

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of changing the tuple field |
spaceName | QString | Space name |
key | IndexKey | Key of the tuple that is to be changed in the `indexName` index |
field | int | Position of the field that is to be changed in the tuple | The fields are numerated with `1` from the beginning or `-1` from the end of the tuple. <br> You cannot change the value of a field included in a **unique index** of space. |
value | QVariant | New field value |
indexName | QString | Not mandatory. <br> Name of the index in which the tuple is searched for the specified key `key`. | Any unique index of space. <br> By default, the primary index is used |

Changes the value of an existing tuple field. If the `field` index is positive then it is counted from `1` from the beginning of the tuple, if negative then from `-1` from the end of the tuple to the beginning of the tuple.

```c++
// Let's say there is a 'Tester' space with 
// a primary index on the first field of type 'unsigned'
// And in this space, there is a tuple:
```
<a id="test-tuple"></a>
> {115726, "Bob", "Marley", 36, 135.72, "Miami", "info@bobmarley.com", "«Money can’t buy life»", true}
```c++
// in which you want 
// to change the value of the <5> field (field numeration in Lua is from 1) to <128.77>
// This means we need to change value to <128.77> the <5> field of tuple 
// that has primary key <115726>:
qDebug() << tnt.changeData("Tester", {115726}, 5, 128.77);
// Or the last field of the same tuple (bool):
qDebug() << tnt.changeData("Tester", {115726}, -1, false);
```
This method performs only one action on the tuple field - **assignment**. 
To perform other available actions on the tuple field(s), use the overloaded `changeData()` method below.

*   **changeData**(const QString &spaceName, const IndexKey &key, const Actions &actions, const QString &indexName)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of changing the tuple fields |
spaceName | QString | Space name |
key | IndexKey | Key of the tuple that is to be changed in the `indexName` index |
actions | Actions | List of `Action` structures describing actions on tuple fields |
indexName | QString | Not mandatory. <br> Name of the index in which the tuple is searched for the specified key `key`. | Any unique space index. <br> By default is used primary index |

This method, like the previous one, changes the values of the tuple specified by the `key` key. But unlike that method, it allows you to perform not only **assignment**, but also any **other** available action on one or **several** tuple fields in one request.

* * **Actions** - list of `Action` structures - planned actions on tuple fields.
The `Action` structure looks like this:
    ```c++
    struct Action {
		QString	 Action;
		int      Field;
		QVariant Value;
	};
    ```
    where,
    > **`Action`** - a string with a symbol of the action (operation) available for the field:
    
    | Symbol <br> operator | Semantics | Action | brief |
    |-|-|-|-|
    | `=` | `Assign` | `Field = Value` | Set field value to `Value` |
    | `+` | `Plus` | `Field += Value` | Add to field value the `Value` |
    | `-` | `Minus` | `Field -= Value` | Subtract from field value the `Value` |
    | `&` | `AND` | `Field &= Value` | `Bitwise AND` with `Value` |
    | `|` | `OR` | `Field |= Value` | `Bitwise OR` with `Value` |
    | `^` | `XOR` | `Field ^= Value` | `Bitwise XOR` with `Value` |
	The result of the operation will be placed in the appropriate field of the tuple as a new value.
	
	> **Keep in mind, that:**
		- Bitwise operations are performed **only** on `unsigned` fields. Those. do not apply to `bool` (*weirdness of `Tarantool`*). If the field type is not explicitly defined by the Space format, it can be changed by **assigning** a value of the desired type.
		- More than one action on the same field in one transaction is not allowed
		- If an error occurs while performing at least one operation from the list, none of the operations in the transaction will be applied
	
	> **`Field`** - The position of the field to change.
	The rules are the same as in the [previous](#changeData1) method for the `field` argument.
	
	> **`Value`** - The value that is used in the operation on the field.

To demonstrate how the method works, let's take a tuple from the [previous](#test-tuple) example and make changes:
```c++
// * Add to <4> field value: 1
// * Subtract from <5> field value: 0.015
// * Change <3> field from the end (before penultimate) to: "bob@mail.com"
qDebug() << tnt.changeData("Tester", {115726}, {{"+", 4, 1}, 
												{"-", 5, 0.015}, 
												{"=", -3, "bob@mail.com"}});
```

*   **getSpaceId**(const QString &spaceName)

|| type | brief | notes |
|-|-|-|-|
return | uint | Space identifier | `Return 0` and fail if the space does not exist |
spaceName | QString | Space name ||

Returns the identifier of the space by its name.

```c++
qDebug() << tnt.getSpaceId("Tester");
```

*   **getSpaceName**(const uint spaceId)

|| type | brief | notes |
|-|-|-|-|
return | QString | Space name | `Return an empty string` and fail if the space does not exist
spaceId | uint | Space identifier ||

Returns the name of the space by its identifier.

```c++
qDebug() << tnt.getSpaceName(512);
```

*   **getSpaceLength**(const QString &spaceName)

|| type | brief | notes |
|-|-|-|-|
return | qlonglong | Space size in number of stored **tuples** | `-1`: on error <br> `0`: if space is empty |
spaceName | QString | Space name |  |

Returns the number of tuples in the space.

```c++
qDebug() << tnt.getSpaceLength("Tester");
```

*   **getSpaceSize**(const QString &spaceName)

|| type | brief | notes |
|-|-|-|-|
return | qlonglong | Size of **data** space in **bytes** | `-1`: on error <br> `0`: if space is empty |
spaceName | QString | Space name |

Returns the size of the Space data in bytes. Only the size taken by **tuples** without metadata of Space.

```c++
qDebug() << tnt.getSpaceSize("Tester");
```
---
####  Index

*   **createIndex**(const QString &spaceName, const QString &indexName, const Parts &parts, const QStringList &options)

|| type | brief | notes |
|-|-|-|-|
return | bool | Result of index creation |
spaceName | QString | Space name |
indexName | QString | Name of the index to be created | Must be unique to the space |
parts | Parts | List of `Part` structures describing the fields of tuples that will be included in the index being created | Field positions are numbered from `1` from the beginning of the tuples. <br> You do not need to enclose string values in single quotes. |
options | QStringList | Optional parameters for the index being created | Not mandatory. <br> The `'parts'` option **should not** be used here as it is passed by the `parts` list above. <br> [Valid Options](#createIndex-options)

* * **parts** - the list contains `Part` structures like:
	```c++
	struct Part {
		QVariant Field;
		QString  Type;
	}
	```
	where,
	> **Field** - unsigned integer, indexed field position in tuples **or** a string containing the field name if it is specified by the space format.
	
	> **Type** - a string with the name of the Lua-type **value** of the indexed field. Field types that can be included in an index: `unsigned`, `string`, `varbinary`, `integer`, `number`, `double`, `boolean`, `decimal`, `datetime`, `uuid`, `array`, `scalar`, `nil`. 
	You do not need to enclose the type name in single quotes. Learn more about [indexed types..][createindex-field-types]
	
	examples of `Part` structures being passed:
	```c++
	{3, "unsigned"} // 
	{2, "signed"}   // 
	{"field5", "boolean"} // 
	```
	The fields of the tuples are included in the created index exactly in the order in which they are located in the `Parts` list. Thus, the very first `Part` structure in the `Parts` list will become the most significant part of the index, and the last one in the list will be the least significant part of the index.  Therefore, indexes that include the same fields on the same data can give different data selection results with the same query parameters:
```c++
QString sp ="test-space-index";
QVariant data;
uint spaceId;
bool bIndexExist;
	// create space with format
qDebug() << "create space: " 
		<< (spaceId =tnt.createSpace(sp, {"format ={{'F1'}, {'F2'}, {'F3'}}"}));

	if(!spaceId)
		return;
		
	// create indexes by field names
	qDebug() << "create indexes: " 
							// primary index { F1 } (redundant in this example)
			<< (bIndexExist =tnt.createIndex(sp, "primary", {{1, "unsigned"}}) 
							// ascending index { F1, F2 }
							&& tnt.createIndex(sp, "ascending", {{"F1", "unsigned"}, 
																{"F2", "unsigned"}})
							// descending index { F2, F1 }
							&& tnt.createIndex(sp, "descending", {{"F2", "unsigned"}, 
																{"F1", "unsigned"}}));
	if(!bIndexExist)
		return;

	// set tuples: { { F1, F2, F3 }, { F1, F2, F3 }, ... }
	for(int c =0; c < 6; ++c)
		tnt.setData(sp, {c, 5 - c, tr("data-%1").arg(c)}, false);

	// all data by 'ascending' index
	data =tnt.getData(sp, {ALL, {}, "ascending"});
	qDebug("%s", QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented).data());
	// all data by 'descending' index
	data =tnt.getData(sp, {ALL, {}, "descending"});
	qDebug("%s", QJsonDocument::fromVariant(data).toJson(QJsonDocument::Indented).data());
	
	/*** RESULTS ***
	All data by 'ascending' index
	[
		[0, 5, "data-0"], 
		[1, 4, "data-1"], 
		[2, 3, "data-2"], 
		[3, 2, "data-3"], 
		[4, 1, "data-4"], 
		[5, 0, "data-5"]
	]

	All data by 'descending' index
	[
		[5, 0, "data-5"], 
		[4, 1, "data-4"], 
		[3, 2, "data-3"], 
		[2, 3, "data-2"], 
		[1, 4, "data-1"], 
		[0, 5, "data-0"]
	]
	**************/
```

<a id ="createIndex-options"></a>
* *  **options** - a list of Lua-expression strings, like:
		```c++
		{ "<option> = <value>", "<option> = <value>", ... }
		```
		where, values of Lua-type `string` are enclosed in single quotes `'`.
Table of available options:

| Space engine | Option | Lua-Type | Brief | By default |
|-|-|-|-|-|
| `memtx` | `type` | `string` | `'HASH'` or `'TREE'` or `'BITSET'` or `'RTREE'` | `'TREE'` |
|  | `id` | `number` |  | last index’s id + 1 |
|  | `unique` | `boolean` |  | `true` |
|  | `if_not_exists` | `boolean` |  | `false` |
|  | `dimension` | `number` |  | `2` |
|  | `distance` | `string` | `'euclid'` или `'manhattan'` | `'euclid'` |
|  | `sequence` | `number|string` |  |  |
|  | `func` | `string` |  |  |
|  | `hint` | `boolean` |  | `true` |
| `vinyl` | `bloom_fpr` | `number` |  | `vinyl_bloom_fpr` |
|  | `page_size` | `number` |  | `vinyl_page_size` |
|  | `range_size` | `number` |  | `vinyl_range_size` |
|  | `run_count_per_level` | `number` |  | `vinyl_run_count_per_level` |
|  | `run_size_ratio` | `number` |  | `vinyl_run_size_ratio` |
More about the meaning of options [createIndex(..)][createindex-options-url]

*   **isIndexExist**(const QString &spaceName, const QString &indexName)

|| type | brief | notes |
|-|-|-|-|
return | bool | The fact that the index exists | Does not fail if the index does not exist |
spaceName | QString | Space name |
indexName | QString | Index name |

Checks if an index with the specified name exists in the space.

```c++
qDebug() << tnt.isIndexExist("Tester", "primary");
```

*   **deleteIndex**(const QString &spaceName, const QString &indexName)

|| type | brief |
|-|-|-|
return | bool | The result of deleting the space index |
spaceName | QString | Space name |
indexName | QString | The name of the index to be deleted |

> **Keep in mind, that:**
	- You cannot delete a **primary index** if any **other** indexes exist in the space except for it.
	- Deleting the primary index will delete **all** tuples of the space.

```c++
qDebug() << tnt.deleteIndex("Tester", "secondary");
```

*   **indexes**()

|| type | brief |
|-|-|-|
return | QUIntMap | An object of type `QMap <uint, QVariant>` containing attributes of all indexes for all spaces of database. |

The method executes a Lua-function on the server [box.space._index:select{}][_index-url] whose result will be converted into an object of type `QUIntMap` with the following structure:
```c++
*** QUIntMap ***

[spaceId]: List {
	
                [0]: Map { <attributes> }
                [1]: Map { <attributes> }
                ...
           },
[spaceId]: List {
	
                [0]: Map { <attributes> }
                ...
           },
...
where, <attributes> is a QVariantMap like:
{
	"iid": 0,   // index identifier
	"name": "", // index name
	"type": "", // index type
	"opts": [], // a list of the index's optional parameters
	"parts": [] // list of fields included in the index
}
```
i.e. this is a map whose value keys are **identifiers** of spaces.  Each map value is a **list** of type `QVariantList` whose length is **number** of indexes in the corresponding space. Each element of such a list is again a map, where the keys are **attribute names** of the corresponding index.

---
####  Service

*   **call**(const QString &function, const QVariantList &args)

|| type | brief |
|-|-|-|
return | `const REPLY &` | Result of server function call |
function | QString | Function name |
args | `QVariantList` | Not mandatory. <br> Function argument list |

The method calls the specified **Lua-function** on the `Tarantool` server.
The result of the function call will be placed in the server response in the format of the [REPLY](#reply_struct) structure.

```c++
qDebug() << tnt.call("os.date", {"%A %B %d"}).Data[QTNT::IPROTO_DATA].toList();
```

*   **exec**(const QString &script, const QVariantList &args)

|| type | brief | 
|-|-|-|
return | `const REPLY &` | The result of the script execution |
script | QString | Script to execute |
args | `QVariantList` | List of script arguments |

The method executes an arbitrary **Lua-script** on the `Tarantool` server.
The script arguments are passed to the location marked (`...`) in the body of the script.
The result of the script execution will be placed in the server response in the format of the [REPLY](#reply_struct) structure.
```c++
qDebug() << tnt.exec("return box.info.version").Data[QTNT::IPROTO_DATA].toList();
qDebug() << tnt.exec("return box.info[...]", {"version"}).Data[QTNT::IPROTO_DATA].toList();
```

*   **execSQL**(const QString &query, const QVariantList &args, const QVariantList &options)
> Implementation not completed.

*   **getLastError**()

|| type | brief | notes |
|-|-|-|-|
return | `ERROR` | Information about the last error while executing the request | - the `code` field is reset to `0` before each request to the server <br> - in the same format, the slot-receiving the signal `error( ERROR )` will receive information about the error |

Returns information about the last error reported by the server in structure format:
```c++
struct ERROR {
	int     code; // Error code reported by the server 
	QString text; // Text message about the error reported by the server
}
```

> #### PRIVATE 
*   **sendRequest**(QUIntMap &header, const QUIntMap &body)

|| type | brief |
|-|-|-|
return | `REPLY` | Server reply |
header | QUIntMap |  Request header in protocol format [`IPROTO`][iproto-url] |
body | QUIntMap | Request data in protocol format [`IPROTO`][iproto-url] |

This is the only class method that sends requests directly to the server. All of the methods listed above, except for `ping()`, work through it.
The server's response after unpacking the `MessagePack` is placed in the `Reply` structure of the `QTarantool` instance. When unpacking, the execution status of the request is checked, if the status does not equal to `IPROTO_OK` then the structure flag `IsValid` will be reset to `false` and an `error( ERROR )` signal will be generated.
<a id='reply_struct'></a>
`REPLY` structure:
```c++
struct REPLY {
	uint     Size;
	QUIntMap Header;
	QUIntMap Data;
	bool     IsValid;
}
```

<a id="add"></a>
#### Add QTarantool to your project (QtCreator):
```
1. Add sources Directory "../QTarantool/src"
   > Right-click on your project name in the projects window, then:
     "Add Existing Directory..." > 
       "Browse..." > 
         "Start Parsing" > 
           "Select files matching: *.cpp; *.h;" > 
             "Apply Filters" >
               "OK"

2. Add 'network' Qt-module to your project *.pro file:
	QT += network

3. Add QTarantool header to your source file(s):
	"..<relative-path-to>/qtarantool.h"
```
You can find test Qt-projects for `QTarantool` in the [Demo][demo-url] directory of this repository.

[box-cfg-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_cfg/
[slab-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_slab/slab_info/
[box-info-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_info/
[stat-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_stat/
[grants-url]: https://www.tarantool.io/en/doc/latest/book/admin/access_control/
[space-create-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_schema/space_create/
[format-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_space/format/#box-space-format
[space-alter-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_space/alter/
[_space-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_space/_space/
[createindex-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_space/create_index/
[createindex-field-types]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_space/create_index/#box-space-index-field-types
[createindex-options-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_space/create_index/#box-space-create-index-options
[_index-url]: https://www.tarantool.io/en/doc/latest/reference/reference_lua/box_space/_index/
[iproto-url]: https://www.tarantool.io/en/doc/latest/dev_guide/internals/box_protocol/
[security-url]: https://www.tarantool.io/en/doc/latest/book/admin/security/
[tarantool-src]: https://github.com/tarantool/tarantool
[tarantool-doc]: https://www.tarantool.io/en/doc/latest/
[tarantool-doc-ov-en]: https://www.tarantool.io/en/doc/latest/overview/
[tarantool-doc-ov-ru]: https://www.bigdataschool.ru/wiki/tarantool
[tarantool-release]: https://www.tarantool.io/en/doc/latest/release/
[tg-icon]: https://web.telegram.org/z/favicon.svg
[tg-url]: https://t.me/tarantool
[tg-ru-url]: https://t.me/tarantoolru
[demo-url]: https://github.com/JohnMcLaren/QTarantool/Demo
[demo-viewer-url]: https://github.com/JohnMcLaren/QTarantool/Demo/TNT-Viewer
