lib_LIBRARIES = libsbuswrap.a
libsbuswrap_a_SOURCES = express.cpp litmus.cpp marshall.cpp unmarshall.cpp validate.cpp peernet.cpp permission.cpp
pkginclude_HEADERS = express.h litmus.h marshall.h unmarshall.h validate.h wrap.h wrapper.h peernet.h permission.h
bin_PROGRAMS = sbuswrapper
sbuswrapper_SOURCES = wrapper.cpp libsbuswrap.a ../library/libsbus.a
sbuswrapper_LDADD = ../library/libsbus.a ./libsbuswrap.a $(COMPATLDADD)

if USE_EFENCE
LDFLAGS += -lefence
endif
