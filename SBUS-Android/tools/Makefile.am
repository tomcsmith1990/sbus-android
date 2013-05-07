bin_PROGRAMS = democpt democlient speek spoke sbus broker \
	trafficgen multitraffic slowcar universalsink rdc rdcacl universalsource

LDADD = ../library/libsbus.a $(COMPATLDADD)


if USE_GNULIB
GNULIB_LDADD = ../gl/libgnu.a
GNULIB_CPPFLAGS = -I../gl
else
GNULIB_LDADD = 
GNULIB_CPPFLAGS =
endif

rdc_SOURCES = rdc.cpp persist.cpp
rdc_LDADD = -lpthread ../wrapper/libsbuswrap.a $(LDADD)

rdcacl_SOURCES = rdcacl.cpp persistacl.cpp
rdcacl_LDADD = -lpthread ../wrapper/libsbuswrap.a $(LDADD)

speek_SOURCES = speek.cpp
speek_LDADD = ../wrapper/libsbuswrap.a $(LDADD)

if USE_SQLITE
bin_PROGRAMS += spersist
spersist_SOURCES = spersist.cpp
spersist_LDADD = -lsqlite3 $(LDADD)
endif

broker_SOURCES = broker.cpp
sbus_SOURCES = sbus.cpp
democpt_SOURCES = democpt.cpp

universalsource_SOURCES = universalsource.cpp
universalsource_CPPFLAGS = $(GNULIB_CPPFLAGS)
universalsource_LDADD = $(GNULIB_LDADD) $(LDADD)

trafficgen_SOURCES = trafficgen.cpp
democlient_SOURCES = democlient.cpp
multitraffic_SOURCES = multitraffic.cpp
slowcar_SOURCES = slowcar.cpp
universalsink_SOURCES = universalsink.cpp

spoke_SOURCES = spoke.cpp
spoke_CPPFLAGS = $(GNULIB_CPPFLAGS)
spoke_LDADD = $(GNULIB_LDADD) $(LDADD)

if USE_EFENCE
LDFLAGS += -lefence
endif
