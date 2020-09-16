TEMPLATE = app
CONFIG += console c++14
CONFIG -= app_bundle
CONFIG -= qt

unix:!macx:LIBS+=-lX11
LIBS += -lGLU -lxcb -lboost_program_options -lboost_system -lboost_filesystem -lpthread
INCLUDEPATH += $$PWD


QMAKE_CXXFLAGS += -std=c++14 -Wall -ffloat-store -frtti -fexceptions -Werror=return-type
QMAKE_CXXFLAGS += -Wctor-dtor-privacy -Werror=delete-non-virtual-dtor -fstrict-aliasing
QMAKE_CXXFLAGS += -Werror=strict-aliasing -Wstrict-aliasing=2
CONFIG += c++14

CONFIG(debug, debug|release) {
     message( "Building the DEBUG Version" )
     #lets optimize for CPU on debug, for release - packager should do
     QMAKE_CXXFLAGS +=  -march=native -O0 -g
     DEFINES += _DEBUG
     unix:!maxc:QMAKE_CXXFLAGS += -fsanitize=undefined -fsanitize=vptr
     unix:!maxc:LIBS += -lubsan
}
else {
    DEFINES += NDEBUG
    message( "Building the RELEASE Version" )
    #delegated to packager - didn't work easy, let it be here
    #QMAKE_CXXFLAGS += -O3 -march=native
    QMAKE_CXXFLAGS_RELEASE = -O3 -march=native
}

include($$PWD/pugixml/pugixml.pri)

SOURCES += \
        main.cpp \
    tess_svg/Bounds.cpp \
    tess_svg/idumper.cpp \
    tess_svg/SvgPath.cpp \
    tess_svg/SvgProcessor.cpp \
    tess_svg/tagdparser.cpp \
    tess_svg/Tesselate.cpp \
    engine/Vector2.cpp

HEADERS += \
    engine/my_math.h \
    tess_svg/Bounds.h \
    tess_svg/callbacks.h \
    tess_svg/GlDefs.h \
    tess_svg/idumper.h \
    tess_svg/json.hpp \
    tess_svg/SvgPath.h \
    tess_svg/SvgProcessor.h \
    tess_svg/tagdparser.h \
    tess_svg/Tesselate.h \
    engine/sincos_cached.h \
    engine/Vector2.h \
    util_helpers.h
