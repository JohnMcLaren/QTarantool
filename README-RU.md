## QTarantool
`Qt/C++ коннектор` для базы данных [Tarantool][tarantool-src].
Реализованы основные методы управления базой и получения статистической информации Tarantool-сервера.
Методы коннектора и описание их работы находятся в разделе [`API`](#qtarantool-api) этого документа.
В каталоге [Demo][demo-url], примеров использования коннектора, вы найдете [TNT-Viewer][demo-viewer-url], который 
можно использовать для просмотра и некоторых изменений данных **существующей** базы `Tarantool`.

## Tarantool
| ![tg-icon] [EN][tg-url] [RU][tg-ru-url] | [# Sources][tarantool-src] | [# Documentation][tarantool-doc] | [# Releases][tarantool-release] |
|-|-|-|-|

`Tarantool` - сервер приложений `Lua`, включающий в себя резидентную `NoSQL` базу данных,
которая отличается высокой скоростью работы по сравнению с традиционными СУБД, обладая теми же свойствами: 
персистентности, транзакционности ACID, репликации master-slave, master-master.
[подробнее..][tarantool-doc-ov-ru]

## QTarantool API
| [Server](#server) | [User](#user) | [Space](#space) | [Index](#index) | [Service](#service) | [+++](#add) |
|-|-|-|-|-|-|

Все методы коннектора блокирующие. 
Управление будет возвращено в случае получения ответа сервера (с любым статусом) либо по таймауту.

---
#### Server

*   **connectToServer**(const QString &uri)

|| тип | значение |
|-|-|-|
возвращает | bool | Текущее состояние соединения с сервером |
uri | QString | Строка `URI` сервера в формате: `protocol://location:port` |

Устанавливает соединение с `Tarantool` сервером. Также генерируется сигнал `signalConnected( bool )`. Если на момент вызова метода имелось установленное соединение сервером, то оно будет разорвано.

```c++
#include "../../src/qtarantool.h"
using namespace QTNT;

QTarantool tnt;

qDebug() << tnt.connectToServer("http://localhost:3301");
```

*   **disconnectServer**()

Разрывает установленное ранее соединение с сервером. 
Также генерируется сигнал `signalConnected( bool )`.

```c++
qDebug() << tnt.disconnectServer();
```

*   **isConnected**()

|| тип | значение |
|-|-|-|
возвращает | bool | Текущее состояние соединения с сервером |

```c++
if(tnt.isConnected())
{
    // TODO
}
```

*   **ping**()

|| тип | значение |
|-|-|-|
возвращает | qint64 | Измеренное время ответа сервера в `наносекундах`|

```c++
qDebug("server ping: %lld nS", tnt.ping());
```

*   **cfg**()

|| тип | значение |
|-|-|-|
возвращает | QVariantMap | Все **не нулевые** параметры конфигурации `Tarantool` сервера которые возвращает [`box.cfg`][box-cfg-url] |

```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.cfg())
                            .toJson(QJsonDocument::Indented)
                            .data());
```

*   **slab**(const SLAB type)

|| тип | значение |
|-|-|-|
возвращает | QVariantMap | Информация по использованию памяти сервером. [Подробнее..][slab-url] |
type | SLAB::INFO | Показать агрегированный отчет об использовании памяти |
|| SLAB::DETAIL | Показать подробный отчет об использовании памяти |
|| SLAB::RUNTIME | Показать отчет об использовании памяти для среды выполнения Lua |

```c++
qDebug("%s", QJsonDocument::fromVariant(QVariantMap {
                                        {"slab::info", tnt.slab(SLAB::INFO)},
                                        {"slab::detail", tnt.slab(SLAB::DETAIL)},
                                        {"slab::runtime", tnt.slab(SLAB::RUNTIME)}})
                            .toJson(QJsonDocument::Indented)
                            .data());
```

*   **info**()

||тип|значение|
|-|-|-|
|возвращает | QVariantMap | Вся информация о состоянии сервера возвращаемая модулем [`box.info`][box-info-url] |

```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.info())
                            .toJson(QJsonDocument::Indented)
                            .data());
```

*   **stat**(const STAT type)

||тип|значение|
|-|-|-|
возвращает | QVariantMap | Различная статистическая информация по использованию сервером сети и запросам. [Подробнее..][stat-url]|
type | STAT::REQUESTS | Показывает общее количество запросов с момента запуска и среднее количество запросов в секунду |
|| STAT::NETWORK | Показывает статистику по сетевым запросам |
|| STAT::VINYL | Показывает активность Vinyl-движка | 

```c++
qDebug("%s", QJsonDocument::fromVariant(QVariantMap {
                                        {"stat::requests", tnt.stat(STAT::REQUESTS)},
                                        {"stat::network", tnt.stat(STAT::NETWORK)},
                                        {"stat::vinyl", tnt.stat(STAT::VINYL)}})
                            .toJson(QJsonDocument::Indented)
                            .data());
```

*   **getServerDirectory**()

||тип|значение| примечание |
|-|-|-|-|
возвращает | QString | Рабочая директория сервера | Та которая в конфигурации `box.cfg` отмечена `'.'`|

```c++
qDebug() << tnt.getServerDirectory();
```

*   **startLocalServer**(const QString &filename)
> не реализовано

*   **stopLocalServer**()
> не реализовано
---
#### User

*  **login**(const QString &userName, const QString &password)

||тип| значение |
|-|-|-|
возвращает | bool | Результат аутентификации |
userName | QString | Имя пользователя |
password | QString | Пароль пользователя |

Процедура аутентификации является обязательной после установки соединения с сервером. Пустой пароль допускается только для пользователя `guest`.

```c++
if(tnt.login("Alice", "alice-password"))
    qDebug("Auth Success!");
else
    qDebug("Auth Failed.");
```

*  **logout**()

|| тип | значение |
|-|-|-|
возвращает | bool | Результат аутентификации как пользователя `guest`|

Завершение сессии текущего пользователя и вход как `guest`.

```c++
tnt.logout();
```

*   **getUserName**()

|| тип | значение | примечание |
|-|-|-|-|
возвращает | QString | Имя текущего пользователя | текущий пользователь должен обладать правом `execute` на выполнение этого запроса |

Имя текущего пользователя будет **запрошено у сервера**.

```c++
qDebug() << tnt.getUserName();
```

*  **createUser**(const QString &userName, const QString &userPassword)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат создания нового пользователя |Теущий пользователь должен иметь право `create` на создание новых пользователей. Пароль пользователя обязателен и не может быть пустым. |
userName | QString | Имя нового пользователя | |
userPassword | QString | Пароль нового пользователя | |

Метод создает нового пользователя с заданным паролем. Если пользователь уже существовал то метод завершится ошибкой и вернет `false`.

```c++
qDebug() << tnt.createUser("Bob", "123");
```

*   **grantUser**(const QString &userName, const QString &userPrivileges, const QString &objectType, const QString &objectName)

||тип|значение|примечание|
|-|-|-|-|
возвращает|bool|Результат наделения пользователя новыми правами. Если пользователь уже обладал указанными правами, запрос будет проигнорирован и метод вернет `true`| Указанный объект **должен существовать**. Теущий пользователь должен обладать правом изменения **указанного объекта** |
userName | QString | Имя пользователя который будет наделен новыми правами | |
userPrivileges | QString | Строка перечисленных через запятую прав предоставляемых пользователю | Допустимые значения: <br> `read`, `write`, `execute`, `session`, `usage`, `create`, `drop`, `alter`, `reference`, `trigger`, `insert`, `update`, `delete` |
objectType | QString | Тип объекта на который распространяются предоставляемые права | Допустимые значения: <br> `'space'`, `'user'`, `'role'`, `'sequence'`, `'function'`, `'trigger'`, `'universe'` <br>(по умолчанию `'universe'`). <br>Значение кроме `null`/`nil` обязательно заключается в **одинарные** кавычки `'` |
objectName | QString | Имя объекта | Произвольное <br>(по умолчанию `null`). <br>Обязательно заключается в **одинарные** кавычки `'` |

[О правах пользователей подробней ..][grants-url] <br> [Почему пользователь **не может** выполнять запросы типа **EVAL**, то есть - **все** этого коннектора ..][security-url]

```c++
// full access to 'Test' space for user 'Bob'
qDebug() << tnt.grantUser("Bob", "read,write,execute,create,drop", "'space'", "'Test'"); 
```

*   **grantUserByRole**(const QString &userName, const QString &userRole)

|| тип | значение |
|-|-|-|
возвращает | bool | `true` если пользователь получил новую роль |
userName | QString | имя пользователя |
userRole | QString | название **существующей** новой роли пользователя |

Метод добавляет новую роль пользователю.

*   **resetGrants**(const QString &userName, const QString &newUserPrivileges, const QString &objectType, const QString &objectName)

> не реализовано

* **deleteUser**(const QString &userName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | `true` - если пользователь успешно удалён, `false` - в ином случае | Если пользователь не существует, метод завершится с ошибкой и вернет `false`
userName | QString | Имя пользователя |

Удаляет указанного пользователя.

```c++
qDebug() << tnt.deleteUser("Bob");
```


*  **isUserExist**(const QString &userName)

|| тип | значение |
|-|-|-|
возвращает | bool | `true` если пользователь существует, иначе `false` |
userName | QString | Имя пользователя |

Проверяет существует ли пользователь с указанным именем

```c++
qDebug() << tnt.isUserExist("John");
```

*   **user**()

|| тип | значение | примечание |
|-|-|-|-|
возвращает | QString | Имя текущего пользователя | Будет возвращено сохраненное экземпляром `QTarantool` имя последнего успешно авторизованного пользователя без обращения к серверу `Tarantool` |

```c++
qDebug() << tnt.user();
```

* **users**()

|| тип | значение |
|-|-|-|
возвращает | QVariantList | список пользователей, ролей и их атрибутов |

Метод возвращает список пользователей и ролей, содержащий списки их атрибутов.
Эквивалент вызова `box.space._user:select{}`

```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.users())
                            .toJson(QJsonDocument::Indented)
                            .data());
```

*   **grants**(const QString &userName)

|| тип | значение |
|-|-|-|
возвращает | QVariantList | Список списков содержащих привелегии пользователя и типы/имена объектов на которые эти привелегии распространяются |
userName | QString | Имя пользователя |

```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.grants("Alice"))
                            .toJson(QJsonDocument::Indented)
                            .data());
```
---
 #### Space

*   **createSpace**(const QString &spaceName, const QStringList &options)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | uint | Идентификатор созданного спейса. 0 в случае неудачи. |
spaceName | QString | Имя нового спейса |
options | QStringList | Список **строк** с опциями создания спейса. | Не обязательный <br> Доступные опции: <br> `engine`, `field_count`, `format`, `id`, `if_not_exists`, `is_local`, `is_sync`, `temporary`, `user` <br> [Подробнее..][space-create-url] |

Создает новый спейс с указанным именем и опциональными параметрами.

```c++
tnt.createSpace("Tester2", {"engine =memtx", 
                            "format ={{'field1'}, {'field2'}, {'field3'}}"
                            });
```
Возможные варианты опции [format..][format-url]

*   **changeSpace**(const QString &spaceName, const QStringList &newParams)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат изменения параметров спейса | версия Tarantool сервера **>= 2.7** (Error: attempt to call method ''alter'' (a nil value)')
spaceName | QString | Имя спейса |
newParams | QStringList | Новые параметры спейса. <br> Изменяемые параметры: <br> `field_count`, `user`, `format`, `temporary`, `is_sync`, `name` | Список строк представляющих новые значения параметров имеет такой же формат как `options` для `createSpace`. |

Изменяет некоторые параметры существующего спейса по указанному списку параметров и их значений.

```c++
// create space with format
tnt.createSpace("Tester2", {"format ={{'field1'}, {'field2'}, {'field3'}, {'field4'}, {'field5'}}"});
// then, change format of space
tnt.changeSpace("Tester2", {"format ={{'F1'}, {'F2'}, {'F3'}}"});
```

* **clearSpace**(const QString &spaceName)

| | тип | значение |
|-|-|-|
возвращает | bool | Результат очистки данных спейса |
spaceName | QString | Имя спейса |

Полностью очищает указанный спейс от **данных**. Все индексы и формат спейса **сохраняются**.

```c++
qDebug() << tnt.clearSpace("Tester");
```

*  **deleteSpace**(const QString &spaceName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат удаления спейса | Метод завершится с ошибкой если спейс с указанным именем не существует |
spaceName | QString | Имя спейса |  |

Удаляет указанный спейс.

```c++
qDebug() << tnt.deleteSpace("Tester");
```

*  **isSpaceExist**(const QString &spaceName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат проверки существования спейса с указанным именем | Не завершается с ошибкой если спейс не существует |
spaceName | QString | Имя спейса |

Проверяет существование спейса с указанным именем.

```c++
qDebug() << tnt.isSpaceExist("Tester");
```

*   **spaces**()

|| тип | значение | примечание |
|-|-|-|-|
возвращает | QVariantList | Список содержащий все атрибуты всех существующих спейсов. | Результат вызова полностью аналогичен Lua-функции `box.space._space:select{}` <br> Подробнее о значении полей [_space][_space-url]

Метод возвращает список списков содержащих атрибуты всех существующих спейсов базы включая системные. Возвращаемые значения будут содержать только метаданные спейсов без хранящихся в них данных
```c++
qDebug("%s", QJsonDocument::fromVariant(tnt.spaces())
                            .toJson(QJsonDocument::Indented)
                            .data());
```

*   **getData**(const QString &spaceName, const Selector &selectorFrom, const Selector &selectorTo, const uint limit)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | QVariantList | Список (массив) кортежей спейса по указанным критериям выборки |
spaceName | QString | Имя спейса |
selectorFrom | Selector | Структура определяющая начало выборки кортежей. | Обязательный. <br> Также задает индекс в котором будет выполнен поиск кортежей.
selectorTo | Selector | Структура определяющая конец выборки кортежей. | Не обязательный. <br> По умолчанию пуст - выборка не ограничена. |
limit | uint | Максимальное количество кортежей для возврата | Не обязательный. <br> По умолчанию =1000|

Метод возвращает список кортежей по условиям выборки заданных селекторами `selectorFrom` `selectorTo`  и ограниченный количеством `limit`. Данным методом, не может быть выполнен поиск кортежей  по разным индексам. Будет применен индекс указанный в `selectorFrom` (по умолчанию =`primary`).

> **Следует иметь в виду, что:** <br>
    - Селектора должны быть `разнонаправлены` <br>
    - `selectorTo` должен находиться на направлении поиска `selectorFrom`. Иначе выборка завершится на границе спейса не достигнув указанной границы поиска.

```c++
QVariant data;

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
* * **Selector** - простая структура для указания граничных условий при выборке кортежей из спейса:
    ```c++
    struct Selector {
        OPERATOR Operator;
        IndexKey Key;
        QString  IndexName;
    };
    ```
    где, 
    > `Operator` - перечисление возможных операторов сравнения:
    
    | Оператор | Семантика | Символ | Значение |
    |-|-|-|-|
    | ALL |  |  | Синоним `GE` |
    | EQ | EQual | == | Точное совпадение |
    | GE | Greater-or-Equal | >= | Больше или равно |
    | GT | Greater-Than | > | Больше |
    | LE | Less-or-Equal | <= | Меньше или равно (обратное направление) |
    | LT | Less-Than | < | Меньше (обратное направление) |
    | REQ | Reverse-EQual |  | То же что `EQ` но обратное направление поиска |
    
    > `Key` - список значений `определенных типов` произвольной длины представляющих искомый ключ. <br>
    Допустимые типы значений в полях ключа:  `<любой числовой тип>`, `логический`, `строка`.
   
    `IndexKey` как наследник `QList` инициализируется списком инициализации значений полей создаваемого ключа или из строки представляющей его. Экземпляр объекта ключа может быть сериализован в текст методом экземпляра `text()`:
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
    Примеры простых ключей по одному индексному полю кортежей:
    ```
    {152}
    {"test value"}
    {3.14159265359}
    ```
    составные ключи для составных индексов:
    ```
    {333, 2.71828, "text", true}
    {29, 01, 2023}
    ```
    составные ключи со сложной структурой -`'Иерархические'`, не поддерживаются `Tarantool`:
    ```
    {500, {{{555, 666}, 666}, 777}, 888, {600, {700, {"800"}}}, 900}
    ```
    > `IndexName` - имя индекса по которому будет вестись поиск. По умолчанию `primary`, но может быть любым `уникальным / не уникальным` индексом. Заключать имя индекса в одинарные кавычки не нужно. 

*   **getData**(const QString &spaceName, const IndexKey &key, const uint field, const QString &indexName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | `QVariant &` | Значение поля кортежа | Возвращаемое значение является константной ссылкой.
spaceName | QString | Имя спейса |
key | IndexKey | Ключ кортежа в индексе `indexName` |  |
field | uint | Номер поля кортежа | Поля нумеруются с `1` от начала кортежа. Отрицательные значения не допускаются.
indexName | QString | Имя индекса | Не обязательный <br> Имя индекса в котором будет поиск ключа `key`. Может быть любым **уникальным** индексом спейса `spaceName`. |

Возвращает значение поля кортежа по указанному ключу кортежа и номеру поля.

```c++
qDebug() << tnt.getData("Tester", {5}, 3);
```

*   **setData**(const QString &spaceName, const QVariantList &tuple, const bool bIfExist =true)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат изменения/добавления нового кортежа данных |
spaceName | QString | Имя спейса |
tuple | QVariantList | Новый кортеж данных |
bIfExist | bool | `true` - изменить только **существующий** кортеж с ключом нового кортежа, иначе ошибка: "Key not found" <br> `false` - установить в любом случае. <br> Если по ключу нового найден старый кортеж то новый кортеж заменит старый, если ключ не существует то указанный кортеж будет вставлен как новый. | по умолчанию `true`

Изменяет данные **существующего** кортежа, по **первичному** индексу нового кортежа, **или** вставляет **новый** кортеж (см. `bIfExist`).

```c++
// In this example it is expected that there is a 'Tester' space with a primary index.
qDebug() << tnt.setData("Tester", {1, "hello", true}, false);
```

*   **insertData**(const QString &spaceName, const QVariantList &tuple)

|| тип | значение |
|-|-|-|
возвращает | bool | Результат **добавления** нового кортежа данных |
spaceName | QString | Имя спейса |
tuple | QVariantList | Новый кортеж данных |

Добавляет новый кортеж данных по первичному ключу кортежа. Кортеж будет добавлен если **не существует** старый кортеж с первичным индексом нового иначе метод завершится ошибкой.

```c++
qDebug() << tnt.insertData("Tester", {2, "hello", true}); // OK, then
qDebug() << tnt.insertData("Tester", {2, "hello", true}); // Error
```
<a id="changeData1"></a>
*   **changeData**(const QString &spaceName, const IndexKey &key, const uint field, const QVariant &value, const QString &indexName) 

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат изменения поля кортежа |
spaceName | QString | Имя спейса |
key | IndexKey | Ключ изменяемого кортежа в индексе `indexName` |
field | int | Позиция изменяемого поля в кортеже | Поля нумеруются с `1` от начала или с `-1` от конца кортежа. <br> Нельзя изменять значение поля входящего в **уникальный индекс** спейса. |
value | QVariant | Новое значение поля |
indexName | QString | Не обязательный. <br> Имя индекса, в котором производится поиск кортежа по указанному ключу `key`. | Любой уникальный индекс спейса. <br> По умолчанию используется первичный индекс |

Изменяет значение поля существующего кортежа. Если индекс поля `field` положительный то отсчет ведется с `1` от начала кортежа, если отрицательный то с `-1` от конца кортежа к его началу.

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
Этот метод выполняет над полем кортежа только одно действие - **присваивание**. 
Чтобы выполнить другие доступные действия над полем/полями кортежа, воспользуйтесь перегруженным методом `changeData()` ниже.

*   **changeData**(const QString &spaceName, const IndexKey &key, const Actions &actions, const QString &indexName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат изменения полей кортежа |
spaceName | QString | Имя спейса |
key | IndexKey | Ключ изменяемого кортежа в индексе `indexName` |
actions | Actions | Список структур `Action` описывающих действия над полями кортежа |
indexName | QString | Не обязательный. <br> Имя индекса, в котором производится поиск кортежа по указанному ключу `key`. | Любой уникальный индекс спейса. <br> По умолчанию используется первичный индекс |

Этот метод как и предыдущий изменяет значения кортежа укзанного ключом `key`. Но в отличии от того метода позволяет выполнить не только **присваивание** но и любое **другое** доступное действие на одном или **нескольких** полях кортежа за один запрос.
* * **Actions** - список структур `Action` - планируемых действий над полями кортежа.
        Структура `Action` имеет вид:
    ```c++
    struct Action {
        QString  Action;
        int      Field;
        QVariant Value;
    };
    ```
    где,
    > **`Action`** - строка с символом доступного для поля действия (операции):
    
    | Символ <br> оператора | Семантика | Действие | Значение |
    |-|-|-|-|
    | `=` | `Assign` | `Field = Value` | Установить значение поля равным `Value` |
    | `+` | `Plus` | `Field += Value` | Прибавить к значению поля `Value` |
    | `-` | `Minus` | `Field -= Value` | Вычесть из значения поля `Value` |
    | `&` | `AND` | `Field &= Value` | `Побитовое И` с `Value` |
    | `\|` | `OR` | `Field \|= Value` | `Побитовое ИЛИ` с `Value` |
    | `^` | `XOR` | `Field ^= Value` | `Побитовое исключающее ИЛИ` с `Value` |
    
    Результат действия будет помещен в соответствующее поле кортежа как новое значение.
    
    > **Следует иметь в виду, что:** <br>
        1. Побитовые операции выполняются **только** над полями типа `unsigned`. Т.е. не применимы к `bool` (*странность `Tarantool`*). Если тип поля не определен явно форматом спейса, его можно изменить **присваиванием** значения нужного типа. <br>
        2. Не допускается более одного действия над одним и тем же полем в одной транзакции <br>
        3. Если при выполнении хотя бы одной операции из списка произойдет ошибка - ни одна операция в транзакции не будет применена <br>
    
    > **`Field`** - Позиция изменяемого поля. 
    Правила те же что и в [предыдущем](#changeData1) методе для аргумента `field`.
    
    > **`Value`** - Значение применяемое в операции с полем.

Для демонстрации работы метода возьмем кортеж из [предыдущего](#test-tuple) примера и внесем изменения:
```c++
// * Add to <4> field value: 1
// * Subtract from <5> field value: 0.015
// * Change <3> field from the end (before penultimate) to: "bob@mail.com"
qDebug() << tnt.changeData("Tester", {115726}, {{"+", 4, 1}, 
                                                {"-", 5, 0.015}, 
                                                {"=", -3, "bob@mail.com"}});
```

*   **getSpaceId**(const QString &spaceName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | uint | Идентификатор спейса | `Вернет 0` и завершится с ошибкой если спейс не существует |
spaceName | QString | Имя спейса ||

Возвращает идентификатор спейса по его имени. 

```c++
qDebug() << tnt.getSpaceId("Tester");
```

*   **getSpaceName**(const uint spaceId)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | QString | Имя спейса | `Вернет пустую строку` и завершится с ошибкой если спейс не существует
spaceId | uint | Идентификатор спейса ||

Возвращает имя спейса по его идентификатору.

```c++
qDebug() << tnt.getSpaceName(512);
```

*   **getSpaceLength**(const QString &spaceName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | qlonglong | Размер спейса в кол-ве хранящихся **кортежей** | `-1`: при ошибке <br> `0`: если спейс пуст |
spaceName | QString | Имя спейса |  |

Возвращает количество кортежей в спейсе.

```c++
qDebug() << tnt.getSpaceLength("Tester");
```

*   **getSpaceSize**(const QString &spaceName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | qlonglong | Размер **данных** спейса в **байтах** | `-1`: при ошибке <br> `0`: если спейс пуст |
spaceName | QString | Имя спейса |

Возвращает размер данных спейса в байтах. Учитывается только размер занимаемый **кортежами** без метаданных спейса.

```c++
qDebug() << tnt.getSpaceSize("Tester");
```
---
####  Index

*   **createIndex**(const QString &spaceName, const QString &indexName, const Parts &parts, const QStringList &options)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Результат создания индекса |
spaceName | QString | Имя спейса |
indexName | QString | Имя создаваемого индекса | Должно быть уникальным для спейса |
parts | Parts | Список структур `Part` описывающих поля кортежей которые войдут в создаваемый индекс | Позиции полей нумеруются с `1` от начала кортежей. <br> Заключать значения строк в одинарные кавычки не нужно. |
options | QStringList | Опциональные параметры создаваемого индекса | Не обязательный. <br> Опция `'parts'` **не должна** здесь использоваться, поскольку передается списком `parts` выше. <br> [Допустимые опции](#createIndex-options)

* * **parts** - список содержит структуры `Part` вида:
    ```c++
    struct Part {
        QVariant Field;
        QString  Type;
    }
    ```
    где, 
    > **Field** - беззнаковое целое число, позиция индексируемого поля в кортежах **или** строка содержащая имя поля если оно задано форматом спейса.
    
    > **Type** - строка с именем Lua-типа **значения** индексируемого поля. Типы полей которые могут быть включены в индекс: `unsigned`, `string`, `varbinary`, `integer`, `number`, `double`, `boolean`, `decimal`, `datetime`, `uuid`, `array`, `scalar`, `nil`. 
    Заключать имя типа в одинарные кавычки не нужно. Подробнее об [индексируемых типах..][createindex-field-types]
    
    примеры передаваемых структур `Part`:
    ```c++
    {3, "unsigned"} // 
    {2, "signed"}   // 
    {"field5", "boolean"} // 
    ```
    Поля кортежей включаются в создаваемый индекс именно в том порядке в котором они расположены в списке `Parts`. Так, самая первая структура `Part` в списке `Parts` станет самой старшей значимой частью индекса а последняя в списке самой младшей.  Поэтому, индексы составленные по одним и тем же полям одних и тех же данных могут давать разные результаты выборки данных при одних и тех же параметрах запроса:
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
* *  **options** - список строк Lua-выражений, вида:
        ```c++
        { "<option> = <value>", "<option> = <value>", ... }
        ```
        где, значения Lua-типа `string` заключаются в одинарные кавычки `'`. <br>
        Таблица доступных опций:

| Движок спейса | Опция | Lua-Тип | Значение | по умолчанию |
|-|-|-|-|-|
| `memtx` | `type` | `string` | `'HASH'` или <br>`'TREE'` или <br>`'BITSET'` или <br>`'RTREE'` | `'TREE'` |
|  | `id` | `number` |  | last index’s id + 1 |
|  | `unique` | `boolean` |  | `true` |
|  | `if_not_exists` | `boolean` |  | `false` |
|  | `dimension` | `number` |  | `2` |
|  | `distance` | `string` | `'euclid'` или <br>`'manhattan'` | `'euclid'` |
|  | `sequence` | `number\|string` |  |  |
|  | `func` | `string` |  |  |
|  | `hint` | `boolean` |  | `true` |
| `vinyl` | `bloom_fpr` | `number` |  | `vinyl_bloom_fpr` |
|  | `page_size` | `number` |  | `vinyl_page_size` |
|  | `range_size` | `number` |  | `vinyl_range_size` |
|  | `run_count_per_level` | `number` |  | `vinyl_run_count_per_level` |
|  | `run_size_ratio` | `number` |  | `vinyl_run_size_ratio` |

Подробнее о значении опций [createIndex(..)][createindex-options-url]

*   **isIndexExist**(const QString &spaceName, const QString &indexName)

|| тип | значение | примечание |
|-|-|-|-|
возвращает | bool | Факт существования индекса | Не завершается с ошибкой если индекс не существует |
spaceName | QString | Имя спейса |
indexName | QString | Имя индекса |

Проверяет существует ли в спейсе индекс с указанным именем.

```c++
qDebug() << tnt.isIndexExist("Tester", "primary");
```

*   **deleteIndex**(const QString &spaceName, const QString &indexName)

|| тип | значение |
|-|-|-|
возвращает | bool | Результат удаления индекса спейса |
spaceName | QString | Имя спейса |
indexName | QString | Имя удаляемого индекса |

> **Следует иметь в виду, что:** <br>
    - Нельзя удалить **первичный индекс** если в спейсе существуют любые **другие** индексы кроме него. <br>
    - При удалении первичного индекса будут удалены **все** кортежи спейса.

```c++
qDebug() << tnt.deleteIndex("Tester", "secondary");
```

*   **indexes**()

|| тип | значение |
|-|-|-|
возвращает | QUIntMap | Объект типа `QMap <uint, QVariant>` содержащий атрибуты всех индексов во всех спейсах базы. |

Метод выполняет `Lua`-функцию на сервере [box.space._index:select{}][_index-url] результат которой будет преобразован в объект типа `QUIntMap` следующей структуры:
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
т.е. это карта, ключами значений которой являются **идентификаторы** спейсов.  Каждое значение карты является **списком** типа `QVariantList`, размер которого равен **количеству** индексов в соответствующем спейсе. Каждый элемент такого списка является опять же картой, где ключами будут **имена атрибутов** соответствующего индекса.

---
####  Service

*   **call**(const QString &function, const QVariantList &args)

|| тип | значение |
|-|-|-|
возвращает | `const REPLY &` | Результат вызова функции сервера |
function | QString | Имя функции |
args | `QVariantList` | Не обязательный. <br> Список аргументов функции |

Метод вызывает указанную `Lua`-функцию на сервере `Tarantool`. <br>
Результат вызова функции будет помещен в ответ сервера в формате структуры [REPLY](#reply_struct).

```c++
qDebug() << tnt.call("os.date", {"%A %B %d"}).Data[QTNT::IPROTO_DATA].toList();
```

*   **exec**(const QString &script, const QVariantList &args)

|| тип | значение | 
|-|-|-|
возвращает | `const REPLY &` | Результат выполнения скрипта |
script | QString | Скрипт для выполнения |
args | `QVariantList` | Список аргументов скрипта |

Метод выполняет произвольный `Lua`-скрипт на сервере `Tarantool`. <br>
Аргументы скрипта передаются в место отмеченное (`...`) в теле скрипта. <br>
Результат выполнения скрипта будет помещен в ответ сервера в формате структуры [REPLY](#reply_struct).
```c++
qDebug() << tnt.exec("return box.info.version").Data[QTNT::IPROTO_DATA].toList();
qDebug() << tnt.exec("return box.info[...]", {"version"}).Data[QTNT::IPROTO_DATA].toList();
```

*   **execSQL**(const QString &query, const QVariantList &args, const QVariantList &options)
> Реализация не завершена.

*   **getLastError**()

|| тип | значение | примечание |
|-|-|-|-|
возвращает | `ERROR` | Информацию о последней ошибке при выполнении запроса | - поле `code` сбрасывается в `0` перед каждым запросом к серверу <br> - в таком же формате информацию об ошибке получит слот-получатель сигнала `error( ERROR )` |

Возвращает информацию о последней ошибке сообщенной сервером в формате структуры:
```c++
struct ERROR {
    int     code; // Error code reported by the server 
    QString text; // Text message about the error reported by the server
}
```

> #### PRIVATE 
*   **sendRequest**(QUIntMap &header, const QUIntMap &body)

||  тип | значение |
|-|-|-|
возвращает | `const REPLY &` | Ответ сервера |
header | QUIntMap |  Заголовок запроса в формате протокола [`IPROTO`][iproto-url] |
body | QUIntMap | Данные запроса в формате протокола [`IPROTO`][iproto-url] |

Единственный метод передачи запросов непосредственно на сервер. Все перечисленные выше методы, кроме `ping()`, работают через него. <br>
Ответ сервера после распаковки `MessagePack` помещается в структуру `Reply` экземпляра `QTarantool`. При распаковке проверяется статус выполнения запроса, если статус не соответствует  `IPROTO_OK` то флаг структуры `IsValid` будет сброшен в `false` и сгенерирован сигнал `error( ERROR )`.

<a id='reply_struct'></a>
Структура `REPLY`:
```c++
struct REPLY {
    uint     Size;
    QUIntMap Header;
    QUIntMap Data;
    bool     IsValid;
}
```

<a id="add"></a>
#### Добавление QTarantool в ваш проект (QtCreator):
```
1. Добавить файлы директории "../QTarantool/src" в ваш проект:
   > Правый клик мыши на имени вашего проекта в окне проектов, затем:
     "Add Existing Directory..." > 
       "Browse..." > 
         "Start Parsing" > 
           "Select files matching: *.cpp; *.h;" > 
             "Apply Filters" >
               "OK"

2. Добавить Qt-модуль 'network' в *.pro файл вашего проекта:
    QT += network

3. Включить заголовочный файл QTarantool в исходники вашего проекта:
    #include "..<relative-path-to>/qtarantool.h"
```
Тестовые примеры Qt-проектов `QTarantool` вы можете найти в каталоге [Demo][demo-url] этого репозитория.

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
[demo-url]: https://github.com/JohnMcLaren/QTarantool/tree/main/Demo
[demo-viewer-url]: https://github.com/JohnMcLaren/QTarantool/tree/main/Demo/TNT-Viewer
