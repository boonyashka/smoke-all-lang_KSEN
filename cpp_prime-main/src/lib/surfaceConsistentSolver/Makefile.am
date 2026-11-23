lib_LTLIBRARIES = libsurfaceConsistentSolver.la

libsurfaceConsistentSolver_la_SOURCES = \
                        AxisMapper.h \
	                AxisMapper.cc \
	                CoordinateCoder.h \
			CoordinateCoder.cc \
	                EquationsReader.h \
			EquationsReader.cc \
                        EquationsSet.h \
			IterationsSet.h \
	                LibStorage.h \
	                LibStorage.cc \
			SurfaceComponent.h \
                        SurfacePoint.h \
	                System.h \
	                System.cc
			

libsurfaceConsistentSolver_la_CPPFLAGS = \
			$(LTDLINCL) \
			-I$(top_srcdir)/src/lib \
			$(POSTGRES_CPPFLAGS) \
			$(IMKL_CPPFLAGS)

libsurfaceConsistentSolver_la_LDFLAGS = \
			$(POSTGRES_LDFLAGS) \
			$(IMKL_LDFLAGS)

libsurfaceConsistentSolver_la_LIBADD = \
                        $(top_builddir)/src/lib/DbTools/libDbTools.la \
			$(top_builddir)/src/lib/Statics/libStatics.la \
			$(top_builddir)/src/lib/surfaceConsistant/libsurfaceConsistant.la \
	                $(top_builddir)/src/lib/Bining/libBining.la \
	                $(top_builddir)/src/lib/dmolib/libdmolib.la \
	                $(top_builddir)/src/lib/mathlib/libmathlib.la \
			$(top_builddir)/src/lib/firstbreak/libfirstbreak.la \
			$(LIBLTDL) \
			$(POSTGRES_LIBS) \
			$(IMKL_LIBS)

