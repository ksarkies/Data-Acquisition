PROJECT =       Data Acquisition GUI
TEMPLATE =      app
TARGET          += 
DEPENDPATH      += .
QT              += widgets
QT              += serialport

OBJECTS_DIR     = obj
MOC_DIR         = moc
UI_DIR          = ui
LANGUAGE        = C++
CONFIG          += qt warn_on release

# Input
FORMS           += data-acquisition-main.ui
FORMS           += data-acquisition-record.ui
FORMS           += data-acquisition-test.ui
FORMS           += data-acquisition-configure.ui
HEADERS         += data-acquisition.h
HEADERS         += data-acquisition-main.h
HEADERS         += data-acquisition-record.h
HEADERS         += data-acquisition-test.h
HEADERS         += data-acquisition-configure.h
SOURCES         += data-acquisition.cpp
SOURCES         += data-acquisition-main.cpp
SOURCES         += data-acquisition-record.cpp
SOURCES         += data-acquisition-test.cpp
SOURCES         += data-acquisition-configure.cpp

