lib_LIBRARIES = libsbus.a
libsbus_a_SOURCES = component.cpp net.cpp error.cpp datatype.cpp hash.cpp builder.cpp \
		multiplex.cpp dimension.cpp xml.cpp lowlevel.cpp
pkginclude_HEADERS = builder.h component.h datatype.h dimension.h error.h hash.h multiplex.h net.h sbus.h lowlevel.h

if USE_COMPAT
CPPFLAGS += -I../compat
endif

