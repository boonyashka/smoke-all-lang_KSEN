lib_LTLIBRARIES = libVerticalSpectraAnalysisUI.la

libVerticalSpectraAnalysisUI_la_SOURCES = \
	Application.h \
	Application.cc \
	AnalysisWindow.cc \
	AnalysisWindow.h \
	DataOpenDialog.h \
	DataOpenDialog.cc \
	DataOpenTask.h \
	DataReader.h \
	DataReader.cc \
	DataStorage.h \
	DataStorage.cc \
	MapWindow.h \
	MapWindow.cc \
	NavigationPanel.h \
	NavigationPanel.cc \
	ObjectsListPanel.h \
	ObjectsListPanel.cc \
	PickingExportDialog.h \
	PickingExportDialog.cc \
	PickingExportTask.h \
	PickingExportTask.cc \
	PickingImportDialog.h \
	PickingImportDialog.cc \
	PickingImportTask.h \
	PickingImportTask.cc \
	PickingInterpolator.h \
	PickingPointsAddingDialog.h \
	PickingPointsAddingDialog.cc \
	SpectrumWindow.h \
	SpectrumWindow.cc \
	StackGatherWindow.h \
	StackGatherWindow.cc \
	SuperGatherWindow.h \
	SuperGatherWindow.cc \
	TasksExecutor.h \
	TasksExecutor.cc \
	ToolsPanel.h \
	ToolsPanel.cc \
	TopMenuPanel.h \
	TopMenuPanel.cc \
	UIComponentsFactory.h \
	UIComponentsFactory.cc \
	WindowsManager.h \
	WindowsManager.cc
	
libVerticalSpectraAnalysisUI_la_CPPFLAGS = -I$(top_srcdir)/src/lib  $(GUI_CPPFLAGS) $(POSTGRES_CPPFLAGS) $(IMKL_CPPFLAGS) $(MPI_CPPFLAGS)

libVerticalSpectraAnalysisUI_la_CXXFLAGS = 

libVerticalSpectraAnalysisUI_la_LDFLAGS = $(GUI_LDFLAGS) $(POSTGRES_LDFLAGS) $(IMKL_LDFLAGS) $(MPI_LDFLAGS)

libVerticalSpectraAnalysisUI_la_LIBADD = \
 $(top_builddir)/src/lib/config++/libconfig++.la \
 $(top_builddir)/src/lib/toolkit/libtoolkit.la \
 $(top_builddir)/src/lib/Dialogs/libDialogs.la \
 $(top_builddir)/src/lib/DialogTool/libNoXDialogTool.la \
 $(top_builddir)/src/lib/DialogTool/libDialogTool.la \
 $(top_builddir)/src/lib/places/libplaces.la \
 $(top_builddir)/src/lib/preprocessing_scripts/libPreprocessingScripts.la \
 $(top_builddir)/src/lib/DbTools/libDbTools.la \
 $(top_builddir)/src/lib/trshead/libtrshead.la \
 $(top_builddir)/src/lib/seismicReader/libSeismicReader.la \
 $(top_builddir)/src/lib/windowDialogs/libWindowDialogs.la \
 $(top_builddir)/src/lib/primeAppGUI/libprimeAppGUI.la \
 $(top_builddir)/src/lib/data3d/libdata3d.la \
 $(top_builddir)/src/lib/mathlib/libmathlib.la \
 $(top_builddir)/src/lib/Nmo/libNmo.la \
 $(top_builddir)/src/lib/utils/libutils.la \
 $(top_builddir)/src/lib/Threads/libThreads.la \
 $(top_builddir)/src/lib/metacube/libmetacube.la \
 $(GUI_LIBS) $(POSTGRES_LIBS) $(IMKL_LIBS) $(MPI_LIBS)
