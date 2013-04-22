bin_PROGRAMS = checkschema checkimport checkmetadata checkvalidate \
	litmuscode analysecpt archive extract

check_PROGRAMS = misctest nettest

LDADD = ../wrapper/libsbuswrap.a ../library/libsbus.a  $(COMPATLDADD)

checkschema_SOURCES = checkschema.cpp
checkimport_SOURCES = checkimport.cpp
checkmetadata_SOURCES = checkmetadata.cpp
checkvalidate_SOURCES = checkvalidate.cpp
litmuscode_SOURCES = litmuscode.cpp
analysecpt_SOURCES = analysecpt.cpp
archive_SOURCES = archive.cpp
extract_SOURCES = extract.cpp
misctest_SOURCES = misctest.cpp
nettest_SOURCES = nettest.cpp

if USE_EFENCE
LDFLAGS += -lefence
endif
