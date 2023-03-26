QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../src/lib/qmsgpack/msgpack.cpp \
	../../src/lib/qmsgpack/msgpackcommon.cpp \
	../../src/lib/qmsgpack/msgpackstream.cpp \
	../../src/lib/qmsgpack/private/pack_p.cpp \
	../../src/lib/qmsgpack/private/qt_types_p.cpp \
	../../src/lib/qmsgpack/private/unpack_p.cpp \
	../../src/lib/qmsgpack/stream/geometry.cpp \
	../../src/lib/qmsgpack/stream/location.cpp \
	../../src/lib/qmsgpack/stream/time.cpp \
    ../../src/qtarantool.cpp \
    forms.cpp \
    main.cpp \
    mainwindow.cpp \
	../lib/QVariantModelWidget/qvariantmodel.cpp \
	../lib/QVariantModelWidget/qvarianttablewidget.cpp \
	../lib/QVariantModelWidget/qvarianttreewidget.cpp

HEADERS += \
    ../../src/include/ExtMetaTypes.h \
	../../src/lib/qmsgpack/endianhelper.h \
	../../src/lib/qmsgpack/msgpack.h \
	../../src/lib/qmsgpack/msgpack_export.h \
	../../src/lib/qmsgpack/msgpackcommon.h \
	../../src/lib/qmsgpack/msgpackstream.h \
	../../src/lib/qmsgpack/private/pack_p.h \
	../../src/lib/qmsgpack/private/qt_types_p.h \
	../../src/lib/qmsgpack/private/unpack_p.h \
	../../src/lib/qmsgpack/stream/geometry.h \
	../../src/lib/qmsgpack/stream/location.h \
	../../src/lib/qmsgpack/stream/time.h \
    ../../src/qtarantool.h \
	../../src/include/iproto_constants.h \
	../../src/include/util.h \
    forms.h \
    mainwindow.h \
	../lib/QVariantModelWidget/qvariantmodel.h \
	../lib/QVariantModelWidget/qvarianttablewidget.h \
	../lib/QVariantModelWidget/qvarianttreewidget.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Set build out directory
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}
# For objects
OBJECTS_DIR = $$DESTDIR/.obj
# For MOC
MOC_DIR = $$DESTDIR/.moc/

