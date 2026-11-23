
#include <cstdio>
#include <algorithm>

#include "toolkit/DialogCore.h"
#include "toolkit/Resources.h"
#include "toolkit/Window.h"
#include "toolkit/DialogInterface.h"
#include "toolkit/Main.h"
#include "primeApp/PrimeAppNoX.h"
#include "files/FilesOperations.h"
#include "dm/DMlib.h"
#include "geom/scale.h"
#include "Math/fft.h"
#include "config++/libconfig.h++"
#include "mathlib/algorithms.h"
#include "filterslib/filterslib.h"
#include "utils/methodname.h"

#include "dialog.h"


using namespace Prime::lib::primeApp ;


namespace Prime {
namespace lib {
namespace filterslib {

const std::string FilterslibDialog::DEF_DIALOG_NAME = "Filters lib" ;
const std::string FilterslibDialog::DEF_FILES_FILTER_NAME = "Filters (*.flt)" ;
const std::string FilterslibDialog::DEF_FILES_FILTER_EXT = "*.flt";
const std::string FilterslibDialog::DEF_TXT_FILES_NAME = "Text (*.txt)" ;
const std::string FilterslibDialog::DEF_TXT_FILES_EXT = "*.txt";
const int FilterslibDialog::DEF_AXIS_LINE_WIDTH = 1 ;
const int FilterslibDialog::DEF_CURVE_LINE_WIDTH = 2 ;
const int FilterslibDialog::DEF_POINT_DRAW_SIZE = 5 ;

FilterslibDialog::FilterslibDialog(double __dt,
                                   DialogStyle __dialog_stype) :
callback(NULL),
callback_data(NULL),
dialog_id(-1),
time_window_id(-1),
spectrum_window_id(-1),
phase_spectrum_window_id(-1),
block_refresh( false ),
dt(__dt),
filter(0)
{
    create(__dialog_stype) ;
}

FilterslibDialog::FilterslibDialog(const std::string &__filter_name,
                                   double __dt,
                                   DialogStyle __dialog_stype) :
callback(NULL),
callback_data(NULL),
dialog_id(-1),
time_window_id(-1),
spectrum_window_id(-1),
phase_spectrum_window_id(-1),
block_refresh( false ),
dt(__dt),
filter(0)
{
    create(__dialog_stype) ;
    loadFilterFromDB(__filter_name) ;
}

FilterslibDialog::~FilterslibDialog()
{
    close() ;

    for (int i = 0; i < views.size(); i++)
    {
        if (views[i])
            delete views[i];
    }

    if ( filter )
        delete filter ;
    
    DIALOG * dialog = getDialogById( dialog_id ) ;
    
    if ( dialog )
    {
        DeleteDialogs( &dialog, 1 ) ;
        ReleaseDialog( dialog ) ;
    }
}

int FilterslibDialog::getID() const
{
    return dialog_id ;
}

void FilterslibDialog::open()
{
    OpenDialog( getID() ) ;
}

bool FilterslibDialog::openWithImmediateResponse()
{
    const int button = OpenImmediateResponseDialog( getID(), immediate_response_dialog_buttons_proc ) ;
    return ( button == BUTTON_OK || button == BUTTON_APPLY ) ;
}

void FilterslibDialog::close()
{
    CloseDialog( getID() ) ;
}

bool FilterslibDialog::isOpen() const
{
    if ( getID() < 0 )
        return false ;

    return IsDialogOpened( getID() ) ;
}

void FilterslibDialog::refresh()
{
    if ( isRefreshBlocked() )
        return ;
    
    RedrawWindow( time_window_id ) ;
    RedrawWindow( spectrum_window_id ) ;
    RedrawWindow( phase_spectrum_window_id ) ;
    
    updateDialog() ;
}

void FilterslibDialog::setCallback( CallbackType __callback, void * __data )
{
    callback = __callback ;
    callback_data = __data ;
}

void FilterslibDialog::setDt(double __dt)
{
    dt = __dt ;

    if (filter)
    {
        filter->setDt(__dt) ;
        refresh() ;
    }
}

double FilterslibDialog::getDt() const
{
    return dt ;
}

const std::string& FilterslibDialog::getFilterName() const
{
    return filter_name ;
}

std::string FilterslibDialog::getFilter() const
{
    if ( filter )
    {
        libconfig::Config config ;
        filter->toConfig( config ) ;
        return config.writeString() ;
    }

    return std::string() ;
}

bool FilterslibDialog::loadFilter( const std::string& __filter )
{
    clear() ;
    
    BaseFilter * new_filter = Filterslib::load( __filter, getDt() ) ;

    if ( !new_filter )
        return false ;
    
    filter = new_filter ;

    makeBackup() ;
    updateViews() ;
    refresh() ;
    updateDialogTitle() ;
    
    return true ;
}

bool FilterslibDialog::loadFilterFromDB( const std::string& __filtername )
{
    clear() ;
    
    BaseFilter * new_filter = Filterslib::open( __filtername, getDt() ) ;

    if ( !new_filter )
        return false ;
    
    filter = new_filter ;
    filter_name = __filtername ;

    makeBackup() ;
    updateViews() ;
    refresh() ;
    updateDialogTitle() ;
    
    return true ;
}

bool FilterslibDialog::loadFilterFromFile( const std::string& __filename )
{
    BaseFilter * new_filter = Filterslib::open_from_file( __filename, getDt() ) ;

    if ( !new_filter )
        return false ;

    clear() ;
    filter = new_filter ;

    makeBackup() ;
    updateViews() ;
    refresh() ;
    updateDialogTitle() ;

    return true ;
}

void FilterslibDialog::create( DialogStyle __dialog_style )
{
    views = BaseView::create_views();

    // Getting titles and packings of views
    std::vector<std::string> filters_titles;
    std::vector<DIALOG::BOX_PACKING> filters_packing;

    for (int i = 0; i < views.size(); i++)
    {
        filters_titles.push_back(views[i]->getTitle());
        filters_packing.push_back(views[i]->getBoxPacking());
    }

    // Dialog menu creating
    MENU::ITEM root_menu[] = {
        { "Filter",  'F', 0, 0, MENU_FILTER,  ORDINARY_M },
        { "Windows", 'W', 0, 0, MENU_WINDOWS, ORDINARY_M },
        { 0}
    };

    MENU::ITEM filter_menu[] = {
        { "New",                       'N', 0, 0, MENU_FILTER_NEW, ORDINARY_M                                  },
        { "Open...",                   'O', 0, 0, NOTHING,         ORDINARY_M, NULL, NULL, menu_open_filter    },
        { "Save",                      'S', 0, 0, NOTHING,         ORDINARY_M, NULL, NULL, menu_save_filter    },
        { "Save As...",                'S', 0, 0, NOTHING,         ORDINARY_M, NULL, NULL, menu_save_as_filter },
        { "Import...",                 'I', 0, 0, NOTHING,         ORDINARY_M, NULL, NULL, menu_import_filter  },
        { "Export...",                 'E', 0, 0, NOTHING,         ORDINARY_M, NULL, NULL, menu_export_filter  },
        { "Export filter operator...", 'E', 0, 0, NOTHING,         ORDINARY_M, NULL, NULL, menu_export_filter_operator  },
        { " ",                        '\0', 0, 0, NOTHING,         SEPARATOR_M                                 },
        { "Close",                     'C', 0, 0, NOTHING,         ORDINARY_M, NULL, NULL, menu_dialog_close   },
        { 0}
    };

    MENU::ITEM new_filter_menu[BaseFilter::NumberOfFilters + 1];
    memset( new_filter_menu, 0, (BaseFilter::NumberOfFilters + 1) * sizeof (MENU::ITEM) ) ;

    for ( int i = 0; i < BaseFilter::NumberOfFilters; i++ )
    {
        new_filter_menu[i].string = BaseFilter::NamesOfFilters[i] ;
        new_filter_menu[i].mnemonic = 'P' ;
        new_filter_menu[i].child = NOTHING ;
        new_filter_menu[i].type = ORDINARY_M ;
        new_filter_menu[i].activateExt = menu_new_filter ;
    }

    MENU::ITEM windows_menu[] = {
        { "Time signature",     'T', 0, 0, NOTHING, TOGGLE_M, 0, 0, show_hide_time_window,  show_hide_time_window_map },
        { "Amplitude spectrum", 'A', 0, 0, NOTHING, TOGGLE_M, 0, 0, show_hide_freq_window,  show_hide_freq_window_map },
        { "Phase spectrum",     'P', 0, 0, NOTHING, TOGGLE_M, 0, 0, show_hide_phase_window, show_hide_phase_window_map},
        { 0 }
    };

    MENU dialog_menu[] = {
        { root_menu        },
        { filter_menu      },
        { new_filter_menu  },
        { windows_menu     },
        { 0 }
    };

    // Windows creating
    WINDOW_BOX time_window_box = {
        "Time",
        -1,
        WINDOW::STATIC_WINDOW,
        0,
        window_expose,
        NULL,
        0,
        0,
        0,
        nullptr,
        150,
        150,
        true,
        true,
        true,
        true,
        0,
        0,
        0,
        0,
        WINDOW::GRAPHICS_2D
    };

    WINDOW_BOX freq_window_box = {
        "Amplitude spectrum",
        -1,
        WINDOW::STATIC_WINDOW,
        0,
        window_expose,
        NULL,
        0,
        0,
        0,
        nullptr,
        150,
        150,
        true,
        true,
        true,
        true,
        0,
        0,
        0,
        0,
        WINDOW::GRAPHICS_2D
    };

    WINDOW_BOX phase_window_box = {
        "Phase spectrum",
        -1,
        WINDOW::STATIC_WINDOW,
        0,
        window_expose,
        NULL,
        0,
        0,
        0,
        nullptr,
        150,
        150,
        true,
        true,
        true,
        true,
        0,
        0,
        0,
        0,
        WINDOW::GRAPHICS_2D
    };

    // Windows packing
    const int windows_num = 3 ;

    MULTI_BOX time_window_mbox[] = {
        { &time_window_box,  WINDOW_BOX_D },
        { 0 }
    };

    MULTI_BOX freq_window_mbox[] = {
        { &freq_window_box,  WINDOW_BOX_D },
        { 0 }
    };

    MULTI_BOX phase_window_mbox[] = {
        { &phase_window_box, WINDOW_BOX_D },
        { 0 }
    };

    DIALOG::BOX_PACKING::LINE windows_pack_lines[] = {
        { 1, RESIZABLE_VERTICAL_BOX },
        { 1, RESIZABLE_VERTICAL_BOX },
        { 1, RESIZABLE_VERTICAL_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING windows_packing = {
        HORIZONTAL_PACKING, windows_pack_lines, true
    };
    
    // Sampling step
    TEXT_BOX sampling_step_box[] = {
        { "Sampling step, sec", "", true, 16, 6, nullptr, on_sampling_step_changed, ufloatTextVerificator, on_sampling_step_changed },
        { 0 }
    };

    // Compiling all together
    const int boxes_num = views.size() + windows_num + 1 ;
    const int views_total_beg = windows_num + 1 ;
    const int views_column_beg = 1 ;

    USER_DEFINED_DIALOG dialog_descriptor[boxes_num + 1] ;
    memset( dialog_descriptor, 0, (boxes_num + 1) * sizeof (USER_DEFINED_DIALOG) ) ;

    // Time window box
    dialog_descriptor[0].user_dialog_box = time_window_mbox ;
    dialog_descriptor[0].type = MULTI_BOX_D ;
    dialog_descriptor[0].remove_frame = true ;
    
    // Amplitude spectrum window box
    dialog_descriptor[1].user_dialog_box = freq_window_mbox ;
    dialog_descriptor[1].type = MULTI_BOX_D ;
    dialog_descriptor[1].remove_frame = true ;
    
    // Phase spectrum window box
    dialog_descriptor[2].user_dialog_box = phase_window_mbox ;
    dialog_descriptor[2].type = MULTI_BOX_D ;
    dialog_descriptor[2].remove_frame = true ;
    
    // Sampling step box
    dialog_descriptor[3].user_dialog_box = sampling_step_box ;
    dialog_descriptor[3].type = TEXT_BOX_D ;
    dialog_descriptor[3].remove_frame = true ;

    for ( size_t i = 0; i < views.size(); ++i )
    {
        dialog_descriptor[views_total_beg + i].user_dialog_box = views[i]->getParametersBox() ;
        dialog_descriptor[views_total_beg + i].label = (char *) filters_titles[i].c_str() ;
        dialog_descriptor[views_total_beg + i].type = MULTI_BOX_D ;
        dialog_descriptor[views_total_beg + i].remove_frame = false ;
        dialog_descriptor[views_total_beg + i].packing = &filters_packing[i] ;
    }

    // Dialog boxes packing
    PackingStyle windows_packing_style[windows_num] ;
    std::fill( windows_packing_style, windows_packing_style + windows_num, PROPORTIONAL_BOX ) ;

    PackingStyle views_packing_style[views_column_beg + views.size()] ;
    std::fill( views_packing_style, views_packing_style + views_column_beg + views.size(), FIXED_SIZE_BOX ) ;
    
    for ( size_t i = 0; i < views.size(); ++i )
        views_packing_style[views_column_beg + i] = views[i]->getPackingStyle() ;

    DIALOG::PACKING::LINE dialog_pack_lines[3] ;
    memset( dialog_pack_lines, 0, 3 * sizeof (DIALOG::PACKING::LINE) ) ;

    // Windows boxes
    dialog_pack_lines[0].nboxes = windows_num ;
    dialog_pack_lines[0].style = PROPORTIONAL_BOX ;
    dialog_pack_lines[0].elements = windows_packing_style ;
    
    // Parameters column boxes
    dialog_pack_lines[1].nboxes = views_column_beg + views.size() ;
    dialog_pack_lines[1].style = FIXED_SIZE_BOX ;
    dialog_pack_lines[1].elements = views_packing_style ;

    DIALOG::PACKING dialog_packing = {
        VERTICAL_PACKING, dialog_pack_lines
    };

    // Dialog buttons
    DIALOG::DIALOG_BUTTON dialog_btns[] = {
        { "Ok",     true,  false},
        { "Apply",  false, false},
        { "Cancel", false, false},
        { 0 }
    };

    // Dialog compiling
    DIALOG dialog_prototype = {
        dialog_descriptor, (char *) DEF_DIALOG_NAME.c_str(), -1, USER_DEFINED_D, __dialog_style,
        dialog_btns, &dialog_packing, dialog_buttons_proc, dialog_menu, dialog_init, DIALOG::DIALOG_WINDOW, 0, 0
    };

    DIALOG * dialog = CopyDialog( &dialog_prototype ) ;

    AddDialogs( &dialog, 1 ) ;
    dialog_id = dialog->id ;
    SetDialogData(dialog_id, "", this) ;

    // Getting time/frequency/phase windows id
    USER_DEFINED_DIALOG * _filter_usd_dialog = (USER_DEFINED_DIALOG *) (dialog->dialog_box);

    MULTI_BOX * _time_window_mbox = (MULTI_BOX *) (_filter_usd_dialog[0].user_dialog_box);
    WINDOW_BOX * _time_window_box = (WINDOW_BOX *) (_time_window_mbox->item);
    time_window_id = _time_window_box->window_struct.id;
    SetWindowData(time_window_id, "", this);

    MULTI_BOX * _freq_window_mbox = (MULTI_BOX *) (_filter_usd_dialog[1].user_dialog_box);
    WINDOW_BOX * _freq_window_box = (WINDOW_BOX *) (_freq_window_mbox->item);
    spectrum_window_id = _freq_window_box->window_struct.id;
    SetWindowData(spectrum_window_id, "", this);

    MULTI_BOX * _phase_window_mbox = (MULTI_BOX *) (_filter_usd_dialog[2].user_dialog_box);
    WINDOW_BOX * _phase_window_box = (WINDOW_BOX *) (_phase_window_mbox->item);
    phase_spectrum_window_id = _phase_window_box->window_struct.id;
    SetWindowData(phase_spectrum_window_id, "", this);

    // Hide phase spectrum window by default
    DialogSetShowBox(getID(), PHASE_WINDOW_BOX, false);

    // Initialization of views
    for (int i = 0; i < views.size(); ++i)
    {
        views[i]->init( this, views_total_beg + i );
        views[i]->hideView();
    }
}

void FilterslibDialog::clear()
{
    if ( filter )
        delete filter ;
    filter = 0 ;

    filter_name.clear() ;

    updateViews() ;
    refresh() ;
}

void FilterslibDialog::callCallback()
{
    if (callback)
        callback( this, callback_data ) ;
}

void FilterslibDialog::blockRefresh()
{
    block_refresh = true ;
}

void FilterslibDialog::unblockRefresh()
{
    block_refresh = false ;
}

bool FilterslibDialog::isRefreshBlocked() const
{
    return block_refresh ;
}

void FilterslibDialog::makeBackup()
{
    filter_backup.readString( std::string() ) ;

    if (filter)
        filter->toConfig( filter_backup ) ;
}

void FilterslibDialog::restoreFromBackup()
{
    if ( !filter )
        return ;

    BaseFilter::Type filter_type = Filterslib::parse_filter_type(filter_backup) ;

    if ( filter->getType() == filter_type )
    {
        filter->fromConfig( filter_backup ) ;
    }
    else
    {
        BaseFilter * new_filter = Filterslib::create( filter_type, getDt() ) ;

        if ( new_filter )
        {
            delete filter ;
            filter = new_filter ;
            filter->fromConfig( filter_backup ) ;
        }
    }

    updateViews() ;
    refresh() ;
    updateDialogTitle() ;
}

bool FilterslibDialog::newFilter( BaseFilter::Type type )
{
    BaseFilter* new_filter = Filterslib::create( type, getDt() ) ;

    if ( !new_filter )
        return false ;

    clear() ;
    filter = new_filter ;

    makeBackup() ;
    updateViews() ;
    refresh() ;
    updateDialogTitle() ;

    return true ;
}

bool FilterslibDialog::loadFilter()
{
    define_methodname() ;

    try
    {
        std::string filter_name ;
        
        if ( DisplaySelectionDialog("Open filter", false, true, std::string(), std::string(),
                                    Filterslib::list(), &filter_name) == 0 )
        {
            return loadFilterFromDB( filter_name ) ;
        }
    }
    catch ( const std::exception& err )
    {
        std::cerr << __methodname__ << ": Error. " << err.what() << std::endl ;
        return false ;
    }

    return true ;
}

bool FilterslibDialog::loadFilterFromFile()
{
    define_methodname() ;
    
    try
    {
        std::string filter_file ;
        std::string path = ( getProjCD() ? getProjCD()->getCommondir() : "~" ) ;

        if (OpenFileSelectionDialog(FILE_SELECTION_CUSTOM, FILE_SELECTION_OPEN, false,
                                    "Import filter", filter_file, path.c_str(),
                                    DEF_FILES_FILTER_NAME.c_str(),
                                    DEF_FILES_FILTER_EXT.c_str()) == 0)
        {
            return loadFilterFromFile( filter_file ) ;
        }
    }
    catch ( const std::exception& err )
    {
        std::cerr << __methodname__ << ": Error. " << err.what() << std::endl ;
        return false ;
    }

    return true ;
}

bool FilterslibDialog::saveFilterToDB( const std::string& __filter_name )
{
    if ( !filter || __filter_name.empty() )
        return false ;

    if ( !Filterslib::save( *filter, __filter_name ) )
        return false ;

    filter_name = __filter_name ;
    
    makeBackup() ;
    updateDialogTitle() ;

    return true ;
}

bool FilterslibDialog::saveFilterToFile( const std::string& __file_name )
{
    if ( !filter || __file_name.empty() )
        return false ;

    if ( !Filterslib::save_to_file( *filter, __file_name ) )
        return false ;

    return true ;
}

bool FilterslibDialog::saveFilterToDB( bool __allow_save_as )
{
    if ( !filter_name.empty() )
        return saveFilterToDB( filter_name ) ;

    if ( !__allow_save_as )
        return true ;

    return saveAsFilterToDB() ;
}

bool FilterslibDialog::saveAsFilterToDB()
{
    define_methodname() ;
    
    if ( !filter )
        return false ;

    try
    {
        std::string filter_name ;
        
        if ( DisplaySelectionDialog("Open filter", true, true, std::string(), "Filter name",
                                    Filterslib::list(), &filter_name) == 0 )
        {
            return saveFilterToDB( filter_name ) ;
        }
    }
    catch ( const std::exception& err )
    {
        std::cerr << __methodname__ << ": Error. " << err.what() << std::endl ;
        return false ;
    }

    return true ;
}

bool FilterslibDialog::saveAsFilterToFile()
{
    std::string filename ;
    std::string path = ( getProjCD() ? getProjCD()->getCommondir() : "~" ) ;

    if (OpenFileSelectionDialog(FILE_SELECTION_CUSTOM, FILE_SELECTION_SAVE, true, "Export filter as...", filename, path.c_str(),
                                DEF_FILES_FILTER_NAME.c_str(), DEF_FILES_FILTER_EXT.c_str()) == 0)
    {
        return saveFilterToFile( filename ) ;
    }

    return true ;
}

bool FilterslibDialog::saveAsFilterOperatorToFile()
{
    if ( !filter )
        return false ;

    std::string name ;
    std::string path = ( getProjCD() ? getProjCD()->getCommondir() : std::string() ) ;

    if (OpenFileSelectionDialog(FILE_SELECTION_CUSTOM, FILE_SELECTION_SAVE, true, "Export filter operator as...", name, path.c_str(),
                                DEF_TXT_FILES_NAME.c_str(), DEF_TXT_FILES_EXT.c_str()) == 0)
    {
        std::ofstream file( name.c_str() ) ;
        std::vector<float> samples = filter->getTimeSamples() ;

        for ( int i = 0; i < samples.size(); ++i )
        {
            file << samples[i] << std::endl ;
        }
    }

    return true ;
}

void FilterslibDialog::updateDialog()
{
    DialogSetTextValueAsDouble( getID(), SAMPLING_STEP_BOX, SAMPLING_STEP_ITEM, getDt(), 3 ) ;
}

void FilterslibDialog::updateDialogTitle()
{
    std::string title ;

    if ( filter && !filter_name.empty() )
    {
        title = ": " + filter_name ;
    }

    DialogSetTitle( getID(), (DEF_DIALOG_NAME + title).c_str() ) ;
}

void FilterslibDialog::updateViews()
{
    blockRefresh() ;

    for ( size_t i = 0; i < views.size(); ++i )
    {
        if ( !views[i] )
            continue ;

        views[i]->hideView() ;

        if ( views[i]->connect(filter) )
        {
            views[i]->update() ;
            views[i]->showView() ;
        }
    }

    unblockRefresh() ;
}

int FilterslibDialog::dialog_init( int __dialog )
{
    FilterslibDialog * _this = (FilterslibDialog *) GetDialogData(__dialog, "") ;

    if ( !_this )
        return 0 ;
    
    _this->updateViews() ;
    _this->refresh() ;
    return 1 ;
}

void FilterslibDialog::dialog_buttons_proc(int __dialog, int __button)
{
    FilterslibDialog * _this = (FilterslibDialog *) GetDialogData(__dialog, "") ;

    if ( !_this || !_this->filter )
    {
        CloseDialog( __dialog ) ;
        return ;
    }

    if ( __button == BUTTON_OK || __button == BUTTON_APPLY )
    {
        _this->saveFilterToDB( false ) ;
        _this->callCallback() ;
    }
    else
    {
        _this->restoreFromBackup() ;
    }

    if ( __button != BUTTON_APPLY )
        _this->close() ;
}

bool FilterslibDialog::immediate_response_dialog_buttons_proc(int __dialog, int __button)
{
    FilterslibDialog * _this = (FilterslibDialog *) GetDialogData(__dialog, "") ;

    if ( !_this || !_this->filter )
    {
        return true ;
    }

    if ( __button == BUTTON_OK || __button == BUTTON_APPLY )
    {
        _this->saveFilterToDB( true ) ;
    }

    return ( __button != BUTTON_APPLY ) ;
}

void FilterslibDialog::window_expose(int __window, int __width, int __height)
{
    FilterslibDialog * _this = (FilterslibDialog *) GetWindowData(__window, "") ;

    if ( !_this || !_this->filter )
    {
        Plotter::clear_graphic_window(__window, 0, 0, __width, __height) ;
        return ;
    }

    GraphBuilder graph ;

    if ( __window == _this->time_window_id )
    {
        graph = GraphBuilder( GraphBuilder::Time, _this->filter ) ;
    }
    else if ( __window == _this->spectrum_window_id )
    {
        graph = GraphBuilder( GraphBuilder::AmplitudeSpectrum, _this->filter ) ;
    }
    else if ( __window == _this->phase_spectrum_window_id )
    {
        graph = GraphBuilder( GraphBuilder::PhaseSpectrum, _this->filter ) ;
    }

    GraphBuilder::Limits curve_limits = graph.getLimits(__width) ;

    Curve curve ;
    curve.color = ToolkitColor( 0.0, 0.0, 0.8 ) ;
    curve.width = 2 ;
    curve.xmin = curve_limits.x_min ;
    curve.xmax = curve_limits.x_max ;
    curve.ymin = curve_limits.y_min ;
    curve.ymax = curve_limits.y_max ;
    curve.xstep = (curve.xmax - curve.xmin) / (__width - 1) ;
    curve.data = graph.getCurve(__width) ;

    std::vector<Curve> curves( 1, curve ) ;

    Plotter::GraphVisualParam visual_parameters ;
    visual_parameters.draw_axis_x = true ;
    visual_parameters.draw_axis_y = true ;
    visual_parameters.fill_area = true ;
    visual_parameters.x_axis_label = graph.getHLabel() ;

    Plotter::GraphLimits limits ;
    Plotter::init_graphic_context(__window, 0, 0, __width, __height, visual_parameters, curves, limits) ;
    Plotter::plot_decoration(__window, visual_parameters, limits) ;
    Plotter::plot(__window, visual_parameters, curves, limits) ;
}

void FilterslibDialog::on_sampling_step_changed( int __dialog, int __box, int __item )
{
    FilterslibDialog * _this = (FilterslibDialog *) GetDialogData(__dialog, "") ;

    if ( _this )
        _this->setDt( DialogGetTextValueAsDouble(__dialog, __box, __item) ) ;
}

void FilterslibDialog::menu_new_filter(const MenuItemDetails & details)
{
    FilterslibDialog * _this = (FilterslibDialog*) GetDialogData(details.postedFrom.id, "");

    if ( _this )
        _this->newFilter( BaseFilter::Type(details.item) ) ;
}

void FilterslibDialog::menu_open_filter(const MenuItemDetails & details)
{
    FilterslibDialog * _this = (FilterslibDialog*) GetDialogData(details.postedFrom.id, "");

    if ( _this && !_this->loadFilter() )
        DisplayErrorDialog( "Error", "Filter opening has failed" ) ;
}

void FilterslibDialog::menu_save_filter(const MenuItemDetails & details)
{
    FilterslibDialog * _this = (FilterslibDialog*) GetDialogData(details.postedFrom.id, "");

    if (_this && _this->saveFilterToDB(true))
        _this->callCallback() ;
    else
        DisplayErrorDialog( "Error", "Filter saving has failed" ) ;
}

void FilterslibDialog::menu_save_as_filter(const MenuItemDetails & details)
{
    FilterslibDialog * _this = (FilterslibDialog*) GetDialogData(details.postedFrom.id, "");

    if (_this && _this->saveAsFilterToDB())
        _this->callCallback() ;
    else
        DisplayErrorDialog( "Error", "Filter saving has failed" ) ;
}

void FilterslibDialog::menu_import_filter( const MenuItemDetails & details )
{
    FilterslibDialog * _this = (FilterslibDialog*) GetDialogData(details.postedFrom.id, "");

    if ( _this && !_this->loadFilterFromFile() )
        DisplayErrorDialog( "Error", "Filter import has failed" ) ;
}

void FilterslibDialog::menu_export_filter( const MenuItemDetails & details )
{
    FilterslibDialog * _this = (FilterslibDialog*) GetDialogData(details.postedFrom.id, "");

    if ( _this && !_this->saveAsFilterToFile() )
        DisplayErrorDialog( "Error", "Filter export has failed" ) ;
}

void FilterslibDialog::menu_export_filter_operator( const MenuItemDetails & details )
{
    FilterslibDialog * _this = (FilterslibDialog*) GetDialogData(details.postedFrom.id, "");

    if ( _this && !_this->saveAsFilterOperatorToFile() )
        DisplayErrorDialog( "Error", "Filter operator export has failed" ) ;
}

void FilterslibDialog::menu_dialog_close(const MenuItemDetails & details)
{
    FilterslibDialog * _this = (FilterslibDialog *) GetDialogData(details.postedFrom.id, "") ;

    if ( _this )
        _this->close() ;
}

void FilterslibDialog::show_hide_window(const MenuItemDetails & details, int box )
{
    FilterslibDialog * _this = (FilterslibDialog *) GetDialogData(details.postedFrom.id, "") ;

    if ( _this )
        DialogSetShowBox( _this->getID(), box, GetMenuToggleButtonState( details ) ) ;
}

void FilterslibDialog::show_hide_window_map(const MenuItemDetails & details, int box )
{
    FilterslibDialog * _this = (FilterslibDialog *) GetDialogData(details.postedFrom.id, "") ;

    if ( _this )
        SetMenuToggleButtonState( details, DialogGetShowBox(_this->getID(), box) ) ;
}

void FilterslibDialog::show_hide_time_window(const MenuItemDetails & details)
{
    show_hide_window( details, TIME_WINDOW_BOX ) ;
}

void FilterslibDialog::show_hide_time_window_map(const MenuItemDetails & details)
{
    show_hide_window_map( details, TIME_WINDOW_BOX ) ;
}

void FilterslibDialog::show_hide_freq_window(const MenuItemDetails & details)
{
    show_hide_window( details, FREQ_WINDOW_BOX ) ;
}

void FilterslibDialog::show_hide_freq_window_map(const MenuItemDetails & details)
{
    show_hide_window_map( details, FREQ_WINDOW_BOX ) ;
}

void FilterslibDialog::show_hide_phase_window(const MenuItemDetails & details)
{
    show_hide_window( details, PHASE_WINDOW_BOX ) ;
}

void FilterslibDialog::show_hide_phase_window_map(const MenuItemDetails & details)
{
    show_hide_window_map( details, PHASE_WINDOW_BOX ) ;
}




//
//
// Graphics interpolator
//
//

const double FilterslibDialog::GraphBuilder::DEF_GRAPH_MARGIN_EXP = 0.05 ;
const double FilterslibDialog::GraphBuilder::DEF_GRAPH_HORIZONTAL = 0.001 ;

std::string FilterslibDialog::GraphBuilder::getHLabel() const
{
    return (type == Time) ? "msec" : "Hz" ;
}

FilterslibDialog::GraphBuilder::Limits
FilterslibDialog::GraphBuilder::getLimits( int length ) const
{
    return (type == Time) ? getTimeLimits(length) : getFreqLimits(length) ;
}

std::vector<float> FilterslibDialog::GraphBuilder::getCurve( int length ) const
{
    std::vector<float> curve ;

    switch (type)
    {
        case Time:
            getTimeCurve(length, curve) ;
            break ;

        case AmplitudeSpectrum:
            getFreqCurve(length, curve) ;
            break ;

        case PhaseSpectrum:
            getPhaseCurve(length, curve) ;
            break ;
    }

    return curve ;
}

Prime::Geom::broken_line_2_dbl FilterslibDialog::GraphBuilder::getPoints() const
{
    Prime::Geom::broken_line_2_dbl points ;

    if ( type == AmplitudeSpectrum && filter && filter->getType() == BaseFilter::FrequencyDomainFilterType )
    {
        const std::vector<float>& x = ((FrequencyDomainFilter*) filter)->getFrequencies() ;
        const std::vector<float>& y = ((FrequencyDomainFilter*) filter)->getAmplitudes() ;

        if ( x.size() != y.size() )
            return points ;

        for ( size_t i = 0; i < x.size(); ++i )
            points.push_back( Prime::Geom::coord_2_dbl(x[i], y[i]) ) ;
    }

    return points ;
}

FilterslibDialog::GraphBuilder::Limits
FilterslibDialog::GraphBuilder::getTimeLimits( int length ) const
{
    Limits limits = {0, 0, 0, 0} ;

    if ( filter )
    {
        const int end = filter->getTimeSamples().size() - 1 ;
        const int beg = std::min( std::max(filter->getOriginIndex(), 0), end ) ;
        limits.x_min = -1000 * filter->getDt() * beg ;
        limits.x_max = +1000 * filter->getDt() * (end - beg) ;

        std::vector<float> curve = getCurve( length ) ;
        calcVerticalLimits( curve, limits.y_min, limits.y_max ) ;
    }

    return limits ;
}

FilterslibDialog::GraphBuilder::Limits
FilterslibDialog::GraphBuilder::getFreqLimits( int length ) const
{
    Limits limits = {0, 0, 0, 0} ;

    if ( filter && filter->getDt() > 0 )
    {
        limits.x_max = 0.5 / filter->getDt() ;

        std::vector<float> curve = getCurve( length ) ;
        calcVerticalLimits( curve, limits.y_min, limits.y_max ) ;
    }

    return limits ;
}

void FilterslibDialog::GraphBuilder::getTimeCurve( int length, std::vector<float>& curve ) const
{
    if ( !filter )
        return ;

    if ( length != cache.size() )
        getInterpolatedCurve( filter->getTimeSamples(), cache, length ) ;

    curve = cache ;
}

void FilterslibDialog::GraphBuilder::getFreqCurve( int length, std::vector<float>& curve ) const
{
    if ( !filter )
        return ;

    if ( length != cache.size() )
    {
        if ( filter->getType() == BaseFilter::FrequencyDomainFilterType )
        {
            cache = ((const FrequencyDomainFilter *) filter)->getFreqSamples( length ) ;
        }
        else if ( filter->getType() == BaseFilter::AbsorptionOperatorType )
        {
            cache = ((const AbsorptionOperator *) filter)->getFreqSamples( length ) ;
        }
        else
        {
            getInterpolatedCurve( filter->getFreqSamples(), cache, length ) ;
        }
    }

    curve = cache ;
}

void FilterslibDialog::GraphBuilder::getPhaseCurve( int length, std::vector<float>& curve ) const
{
    if ( !filter )
        return ;

    if ( length != cache.size() )
        getInterpolatedCurve( filter->getPhaseSamples(), cache, length ) ;

    curve = cache ;
}

void FilterslibDialog::GraphBuilder::calcVerticalLimits(const std::vector<float>& curve, float& ymin, float& ymax) const
{
    if ( curve.empty() )
    {
        ymin = ymax = 0 ;
        return ;
    }

    ymin = (type == AmplitudeSpectrum || type == PhaseSpectrum) ? 0 : +FLT_MAX ;
    ymax = -FLT_MAX ;

    for ( size_t i = 0; i < curve.size(); ++i )
    {
        ymin = std::min( ymin, curve[i] ) ;
        ymax = std::max( ymax, curve[i] ) ;
    }
}

void FilterslibDialog::GraphBuilder::getInterpolatedCurve(const std::vector<float>& samples,
                                                          std::vector<float>& curve,
                                                          int length ) const
{
    define_methodname(...) ;

    curve.clear() ;

    if ( samples.size() < 2 || length < 2 )
        return ;

    std::vector<float> x( samples.size() ) ;

    for ( size_t i = 0, n = x.size(); i < n; ++i )
    {
        if ( isinf(samples[i]) || isnan(samples[i]) )
        {
            std::cerr << __methodname__ << ": Error. Incorrect value (Inf or Nan) at sample #" << i << " has been detected." << std::endl ;
            return ;
        }

        x[i] = i ;
    }

    Prime::lib::mathlib::linear_reinterpolation<float>(x.data(), samples.data(), samples.size(),
                                                       0, x.size() - 1, length, curve) ;
}

}
}
}
