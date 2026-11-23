
lib_LTLIBRARIES = libsegd3.la

libsegd3_la_SOURCES = \
	segd3.h segd3.cc \
	segd3err.h segd3err.cc \
	segd3utils.h segd3utils.cc \
	timestamp.h timestamp.cc

libsegd3_la_CPPFLAGS = -I$(top_srcdir)/src/lib

libsegd3_la_LDFLAGS = 

libsegd3_la_LIBADD = \
	$(top_builddir)/src/lib/trshead/libtrshead.la \
	$(top_builddir)/src/lib/Seism/libSeism.la


noinst_PROGRAMS = segd3test

segd3test_SOURCES = test.cc

segd3test_CPPFLAGS = -I$(top_srcdir)/src/lib 

segd3test_CXXFLAGS = 

segd3test_LDFLAGS = 

segd3test_LDADD = \
	$(top_builddir)/src/lib/CommonTypes/libCommonTypes.la \
	$(top_builddir)/src/lib/trshead/libtrshead.la \
	$(top_builddir)/src/lib/Seism/libSeism.la \
	libsegd3.la
