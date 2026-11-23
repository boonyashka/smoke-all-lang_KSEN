
lib_LTLIBRARIES = libfilterslib_ui.la

libfilterslib_ui_la_SOURCES = dialog.cc \
                              dialog.h \
                              filtersviews.cc \
                              filtersviews.h                        
                                              
libfilterslib_ui_la_CPPFLAGS = -I$(top_srcdir)/src/lib\
                                 $(GUI_CPPFLAGS) \
                                 $(POSTGRES_CPPFLAGS) \
                                 $(IMKL_CPPFLAGS) 

libfilterslib_ui_la_CXXFLAGS = 

libfilterslib_ui_la_LDFLAGS = $(GUI_CPPFLAGS) $(POSTGRES_LDFLAGS) $(IMKL_LDFLAGS)

libfilterslib_ui_la_LIBADD = $(top_builddir)/src/lib/toolkit/libtoolkit.la \
                             $(top_builddir)/src/lib/config++/libconfig++.la \
                             $(top_builddir)/src/lib/files/libPrimeFiles.la \
                             $(top_builddir)/src/lib/mathlib/libmathlib.la \
                             $(top_builddir)/src/lib/geom/libPrimeGeom.la \
                             $(top_builddir)/src/lib/charts/libcharts.la \
                             $(top_builddir)/src/lib/filterslib/libfilterslib.la \
                             $(GUI_LIBS) \
                             $(POSTGRES_LIBS) \
                             $(IMKL_LIBS)
                      
                             




