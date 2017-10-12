PROJECT =       Data Processing GUI
TEMPLATE =      app
TARGET          += 
DEPENDPATH      += .

QWT_ROOT        = /usr/local/qwt-6.1.3
include( $${QWT_ROOT}/features/qwt.prf )

QMAKE_RPATHDIR  *= $${QWT_ROOT}/lib

OBJECTS_DIR     = obj
MOC_DIR         = moc
UI_HEADERS_DIR  = ui
UI_SOURCES_DIR  = ui
LANGUAGE        = C++
CONFIG          += qt warn_on release

# Input
FORMS           += data-processing-main.ui
HEADERS         += data-processing-main.h
SOURCES         += data-processing.cpp
SOURCES         += data-processing-main.cpp

