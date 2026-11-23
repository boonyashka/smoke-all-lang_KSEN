
#include "toolkit/Resources.h"
#include "utils/methodname.h"
#include "data/Polyline.h"
#include "configdirs/configdirs.h"
#include "dm/DMScale.h"
#include "geom/CoordTypes.h"

#include "Application.h"
#include "AnalysisWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

const char * AnalysisWindow::RES_PICKING_COLOR = "picking color" ;
const char * AnalysisWindow::RES_INTERP_PICKING_COLOR = "interpolated picking color" ;
const char * AnalysisWindow::RES_MUTING_COLOR = "muting color" ;

AnalysisWindow::AnalysisWindow( int __window, SpectraDataType __data_type ) :
SeismicWindow(__window, "", ""),
m_data_type( __data_type ),
m_reader( 0 ),
m_active_data_point_index( 0 ),
m_active_picking_point( 0 ),
m_resources( nullptr, std::function<void(ResTable*)>(ReleaseResTable) )
{   
    init() ;
    loadConfiguration() ;
}

AnalysisWindow::~AnalysisWindow()
{
    saveConfiguration() ;
}

SpectraDataType AnalysisWindow::getDataType() const
{
    return m_data_type ;
}

void AnalysisWindow::setPolyline(const Prime::Data::Polyline & __polyline)
{
    polyline = __polyline;
}

const Prime::Data::Polyline & AnalysisWindow::getPolyline() const
{
    return polyline ;
}

const Prime::Data::Polyline * AnalysisWindow::getLevelPolyline() const
{
    return &polyline ;
}

void AnalysisWindow::setDataReader( DataReader * __reader )
{
    define_methodname() ;

    using Prime::Data::Polyline ;

    try
    {
        m_reader = 0 ;
        setPolyline( Prime::Data::Polyline() ) ;

        if ( !__reader )
            return ;

        m_reader = __reader ;

        // Creating common virtual line for gathers
        // where each trace have private picket.
        Prime::Geom::broken_line_2_dbl line ;
        line.push_back( Prime::Geom::coord_2_dbl(__reader->getFirstTraceOffset(), 0) ) ;
        line.push_back( Prime::Geom::coord_2_dbl(__reader->getLastTraceOffset(), 0) ) ;

        // We associate each gather with separate segment
        std::vector<Polyline::Segment> segments ;
        const int gathers_num = __reader->getGathers().size() ;

        for ( int i = 0; i < gathers_num; ++i )
        {
            const auto& gather = __reader->getGatherByIndex(i) ;
            const double trace_step = __reader->getDp() ;
            const double coord_from = trace_step * __reader->logical2secondarykey(gather.firstTrace) ;
            const double coord_to = coord_from + (gather.lastTrace - gather.firstTrace) * trace_step ;
            const double trace_beg = trace_step * gather.firstTrace ;
            const double trace_end = trace_step * gather.lastTrace ;

            segments.push_back( Polyline::Segment(std::string(),
                                                  coord_from, coord_to,
                                                  trace_beg, trace_end, 0) ) ;
        }

        blockRedraw() ;
        updateLabels() ;
        setPolyline( Polyline(line, segments) ) ;
        setPositiveColorLevelPalette( __reader->getSpectraDataType() == SpectraDataType::Spectrum ) ;
        unblockRedraw() ;
    }
    catch ( const std::exception& __err )
    {
        std::cout << __methodname__ << ": Error. " << __err.what() << std::endl ;
        unblockRedraw() ;
    }
}

const DataReader * AnalysisWindow::getDataReader() const
{
    return m_reader ;
}

DataReader * AnalysisWindow::getDataReader()
{
    return m_reader ;
}

const Prime::Data::SeismicReader * AnalysisWindow::getReader( size_t seg ) const
{
    if ( seg != getActiveAnalysisPointIndex() )
        return 0 ;
    
    return m_reader ;
}

int AnalysisWindow::getActiveAnalysisPointIndex() const
{
    if ( !m_reader )
        return 0 ;

    if ( m_active_data_point_index < 0 )
        return 0 ;

    if ( m_active_data_point_index >= m_reader->getGathers().size() )
        return std::max( int(m_reader->getGathers().size()) - 1, 0 ) ;

    return m_active_data_point_index ;
}

void AnalysisWindow::setActiveDataPoint(int __index, PickingPoint * __picking_point,
                                        const PickingPoint * __interpolated_picking_point)
{
    if ( !m_reader || m_reader->getGathers().empty() )
        return ;

    __index = std::max(__index, 0) ;
    __index = std::min(__index, int(m_reader->getGathers().size()) - 1) ;

    m_active_data_point_index = __index ;
    m_active_picking_point = __picking_point ;
    m_active_interpolated_picking_point = __interpolated_picking_point ;
    
    blockRedraw() ;
    setExtends( calcExtends(0, 0) ) ;
    unblockRedraw() ;
}

bool AnalysisWindow::computeModelExtends( double * __begin, double * __end ) const
{
    if ( !__begin || !__end )
        return false ;

    if ( !getDataReader() || getDataReader()->getNumberOfTraces() == 0 )
        return false ;

    if (getActiveAnalysisPointIndex() < 0 ||
        getActiveAnalysisPointIndex() >= polyline.segments().size())
        return false ;

    *__begin = polyline.segments().at( getActiveAnalysisPointIndex() ).from ;
    *__end = polyline.segments().at( getActiveAnalysisPointIndex() ).to ;

    return true ;
}

bool AnalysisWindow::computeReaderDepthRange( double * __min, double * __max ) const
{
    if ( !__min || !__max )
        return false ;

    if ( !getDataReader() || getDataReader()->getNumberOfTraces() == 0 )
        return false ;

    double tmp ;

    trace2profile( getDataReader(), 0, getDataReader()->getBegin(), &tmp, __min ) ;
    profile2world( polyline.segments().front(), tmp, *__min, &tmp, __min ) ;

    trace2profile( getDataReader(), 0, getDataReader()->getEnd(), &tmp, __max ) ;
    profile2world( polyline.segments().front(), tmp, *__max, &tmp, __max ) ;

    return true ;
}

bool AnalysisWindow::computeModelDepthRange( double * __min, double * __max ) const
{
    return false ;
}

bool AnalysisWindow::computeReaderDepthRange(double x1, double x2, double * min, double * max) const
{
    return false ;
}

bool AnalysisWindow::computeModelDepthRange(double x1, double x2, double * min, double * max) const
{
    return false ;
}

void AnalysisWindow::updateLabels()
{
    if ( !m_reader )
        return ;

    std::string hlabel = "" ; //(m_reader->getSpectraDataType() == SpectraDataType::SuperGather ? m_reader->getSecondaryKeyName() : "") ;
    std::string vlabel = (m_reader->getDataType() == FR_IO_TIME) ? "msec" : "m" ;
    int scale = (m_reader->getDataType() == FR_IO_TIME) ? 1000 : 1 ;

    setScale(1, scale) ;
    setScaleLabel( hlabel, vlabel ) ;
    setScaleFormat( false, false, false, false ) ;
}

void AnalysisWindow::changeColors()
{
    SetWindowBackground( getWindowId(), backgroundColor ) ;
    SetWindowZoomColor( getWindowId(), zoomColor ) ;

    redraw( ALL_PLANES ) ;
}

void AnalysisWindow::init()
{
    // Set bounding box computation mode
    setBoundingBoxComputationMode( Prime::Vis::SeismicWindow::READER_BBOX ) ;

    // Set margins
    setMarginY( 0.0 ) ;
    setMarginX( 0.0 ) ;

    // Selection mode
    setExtendedTraceSelectionMode(false);

    // Labels initialization
    //setScaleLabel( "1/m, 10<sup>3</sup>", "m/sec" ) ;
    setScale( 1000, 1 ) ;

    // Initialize lists of actual colors
    std::vector<std::string> colors ;
    colors.push_back( Prime::Vis::SeismicWindow::BACKGROUND_COLOR_NAME ) ;
    colors.push_back( Prime::Vis::SeismicWindow::SCALE_COLOR_NAME ) ;
    colors.push_back( Prime::Vis::SeismicWindow::WIGGLE_COLOR_NAME ) ;
    colors.push_back( Prime::Vis::SeismicWindow::CURSOR_TRACKING_CROSS_LINES_COLOR_NAME ) ;
    colors.push_back( RES_PICKING_COLOR ) ;
    //colors.push_back( Prime::Vis::SeismicWindow::HIGHLIGHT_COLOR_NAME ) ;
    //colors.push_back( Prime::Vis::SeismicWindow::GRID_LINES_COLOR_NAME );
    colors.push_back( Prime::Vis::SeismicWindow::ZOOM_COLOR_NAME ) ;

    if ( getDataType() == SpectraDataType::SuperGather )
    {
        colors.push_back( RES_MUTING_COLOR ) ;
    }
    else
    {
        colors.push_back( RES_INTERP_PICKING_COLOR ) ;
    }

    setActualColors( colors ) ;

    // Initialize palette path
    setPalettePath( Prime::getPalettesDirectory() ) ;
}

ResTable * AnalysisWindow::getResources()
{
    static std::string DEF_FONT_NAME = GetDefaultWindowFont().getName() ;
    static std::string DEF_SCALE_FONT_NAME = GetDefaultWindowScaleFont().getName() ;

    if ( !m_resources )
    {
        std::string def_palette_name = "default_91" ;

        if ( getDataType() == SpectraDataType::Spectrum )
        {
            def_palette_name = "Rainbow" ;
        }

        ResTable resources[] = {

            // SeismicWindow: fonts
            { &font,            ResTable::FONT_RES, WINDOW_FONT_NAME, DEF_FONT_NAME.c_str()       },
            { &scaleFont,       ResTable::FONT_RES, SCALE_FONT_NAME,  DEF_SCALE_FONT_NAME.c_str() },

            // SeismicWindow: resources
            { &res_palette_name,               ResTable::STD_STRING_RES, "seismic palette",                 def_palette_name.c_str() },
            //{ &drawCursorTrackingCrossLines, ResTable::BOOL_RES,       "draw cursor tracking cross lines", "true"       },

            // SeismicWindow: colors
            { &backgroundColor,                ResTable::RGB_RES, BACKGROUND_COLOR_NAME,                   "White" },
            { &scaleColor,                     ResTable::RGB_RES, SCALE_COLOR_NAME,                        "Black" },
            { &wiggleColor,                    ResTable::RGB_RES, WIGGLE_COLOR_NAME,                       "Black" },
            //{ &highlightColor,                ResTable::RGB_RES, HIGHLIGHT_COLOR_NAME,                    "Red"   },
            { &zoomColor,                      ResTable::RGB_RES, ZOOM_COLOR_NAME,                         "Blue"  },
            { &cursorTrackingCrossLinesColor,  ResTable::RGB_RES, CURSOR_TRACKING_CROSS_LINES_COLOR_NAME,  "Black" },

            // AnalysisWindow: colors
            { &res_picking_color,              ResTable::RGB_RES, RES_PICKING_COLOR,                       "Yellow"  },
            { &res_interpolated_picking_color, ResTable::RGB_RES, RES_INTERP_PICKING_COLOR,                "Black" },//#FF9900" },
            { &res_muting_color,               ResTable::RGB_RES, RES_MUTING_COLOR,                        "Red"     },

            { 0 }
        };

        m_resources.reset( CopyResTable(resources) ) ;
    }

    return m_resources.get() ;
}

void AnalysisWindow::loadConfiguration()
{
    define_methodname() ;

    // Reading configurations
    ApplGetResources( MakeResourceName(GetProgramName(), getName()), getResources() ) ;

    // Setting configurations
    SetWindowBackground( getWindowId(), backgroundColor ) ;
    SetWindowZoomColor( getWindowId(), zoomColor ) ;

    drawSeismicLegend = false ;
    seismicPalette = ColorPalette( getPalettePath(), res_palette_name ) ;
}

void AnalysisWindow::saveConfiguration()
{
    // SeismicWindow: fonts
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), WINDOW_FONT_NAME), font      ) ;
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), SCALE_FONT_NAME),  scaleFont ) ;

    // SeismicWindow: resources
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), "seismic palette"),                  getSeismicPalette().getName().c_str() ) ;
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), "draw cursor tracking cross lines"), getDrawCursorTrackingCrossLines()     ) ;

    // SeismicWindow: colors
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), BACKGROUND_COLOR_NAME),                  backgroundColor               ) ;
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), HIGHLIGHT_COLOR_NAME),                   highlightColor                ) ;
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), ZOOM_COLOR_NAME),                        zoomColor                     ) ;
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), CURSOR_TRACKING_CROSS_LINES_COLOR_NAME), cursorTrackingCrossLinesColor ) ;

    // AnalysisWindow: colors
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), RES_PICKING_COLOR),        res_picking_color ) ;
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), RES_INTERP_PICKING_COLOR), res_interpolated_picking_color ) ;
    ApplPutResource( MakeResourceName(GetProgramName(), getName(), RES_MUTING_COLOR),         res_muting_color ) ;
}

void AnalysisWindow::drawBackPlane( int __x0, int __y0, int __x1, int __y1 ) const
{
    clearWindow() ;

    switch (visualMode)
    {
        case VISUAL_MODE_COLOR:
            drawSectionColor( polyline, getActiveAnalysisPointIndex() ) ;
            break;

        case VISUAL_MODE_WIGGLE:
        case VISUAL_MODE_WIGGLE_MIN:
        case VISUAL_MODE_WIGGLE_MAX:
            drawSectionWiggle( polyline, getActiveAnalysisPointIndex() ) ;
            break;

        default:
            break;
    }
}

void AnalysisWindow::drawFrontPlane( int __x0, int __y0, int __x1, int __y1 ) const
{
    WINDOW::PrintFormat print_format ;
    bool printed = IsWindowBeingPrinted( getWindowId(), &print_format ) ;

    if ( !printed )
    {
        ScaleMode hmode, vmode ;
        getScaleMode( &hmode, &vmode ) ;

        if ( hmode == SCALE_INSIDE )
            drawScaleInside( WINDOW::HORIZONTAL, __x0, __y0, __x1, __y1 ) ;

        if ( vmode == SCALE_INSIDE )
            drawScaleInside( WINDOW::VERTICAL, __x0, __y0, __x1, __y1 ) ;
    }

    if (getDataReader() &&
        getSeismicDrawLegend() &&
        getVisualMode() == VISUAL_MODE_COLOR)
    {
        draw_legend( getSeismicPalette(), BottomRight ) ;
    }
}

void AnalysisWindow::drawInteractivePlane( int __x0, int __y0, int __x1, int __y1 ) const
{
    if ( drawCursorTrackingCrossLines )
    {
        draw_cursor_tracking_cross_lines() ;
    }
}

void AnalysisWindow::do_configure( int __width, int __height ) const
{
    drawMask = ALL_PLANES ;
}

void AnalysisWindow::do_expose( int __x0, int __y0, int __x1, int __y1 ) const
{
    if ( !getDataReader() )
        return ;
    
    prepareCoord(__x0, __y0, __x1, __y1) ;

    if ( drawMask & BACK_PLANE )
    {
        SetWindowDrawTarget( getWindowId(), WINDOW::BackPlane ) ;
        drawBackPlane(__x0, __y0, __x1, __y1) ;

        SetWindowDrawTarget( getWindowId(), WINDOW::FrontPlane ) ;
    }

    if (drawMask & FRONT_PLANE)
    {
        RestoreWindowBackPlane(window);
        drawFrontPlane(__x0, __y0, __x1, __y1);
    }

    drawMask &= ~(BACK_PLANE | FRONT_PLANE);
}

void AnalysisWindow::do_expose_interactive(int x0, int y0, int x1, int y1) const
{
    if (drawMask & INTERACTIVE_PLANE)
        drawInteractivePlane(x0, y0, x1, y1);

    drawMask &= ~INTERACTIVE_PLANE;
}

void AnalysisWindow::do_print( int __width, int __height ) const
{
    if ( !getDataReader() )
        return ;

    int x0 = 0 ;
    int y0 = 0 ;
    int x1 = __width ;
    int y1 = __height ;

    prepareCoord( x0, y0, x1, y1 ) ;
    SetWindowViewPort( getWindowId(), x0 + 2, y0 + 1, x1 - 1, y1 - 1 ) ;

    drawBackPlane( x0, y0, x1, y1) ;
    drawFrontPlane( x0, y0, x1, y1 ) ;
}

void AnalysisWindow::do_expose_scale( WINDOW::ScaleOrientation __dir, double __value, int __width, int __height, int __begin, int __end ) const
{
    if ( __begin >= __end )
        return ;

    ScaleMode horizontal_mode, vertical_mode ;
    getScaleMode( &horizontal_mode, &vertical_mode ) ;

    ToolkitColor background = GetWindowScaleBackground(getWindowId(), __dir);
    ToolkitColor foreground = GetWindowScaleForeground(getWindowId(), __dir);

    //double x0, y0, x1, y1 ;
    //profile2data( zoom.p1.x, zoom.p1.y, &x0, &y0 ) ;
    //profile2data( zoom.p2.x, zoom.p2.y, &x1, &y1 ) ;
    double x0 = zoom.p1.x ;
    double y0 = zoom.p1.y ;
    double x1 = zoom.p2.x ;
    double y1 = zoom.p2.y ;

    double x_scale, y_scale ;
    getScale( &x_scale, &y_scale ) ;

    std::string hlabel, vlabel ;
    getScaleLabel( &hlabel, &vlabel ) ;

    if ( __dir == WINDOW::HORIZONTAL )
    {
        if ( horizontal_mode != SCALE_OUTSIDE )
            return ;

        const bool reverse = ( getCoordSystem() == RIGHT_TO_LEFT ) ;
        DMDrawScale(getWindowId(), __dir, false, __width, __height, __begin, __end, 0, x0 * x_scale, x1 * x_scale,
                    reverse, true, 0, scaleFont, background, foreground, hlabel.c_str(), scaleLineWidth,
                    SCALE_RISKA, HORIZONTAL_SCALE_MARGIN, HORIZONTAL_SCALE_SPACING) ;
    }
    else if ( __dir == WINDOW::VERTICAL )
    {
        if ( vertical_mode != SCALE_OUTSIDE )
            return ;

        const bool reverse = ( y0 > y1 ) ;
        const int max_chars = computeVerticalScaleMaxChars() ;
        DMDrawScale(getWindowId(), __dir, false, __width, __height, __begin, __end, 0, y0 * y_scale, y1 * y_scale,
                    reverse, true, max_chars, scaleFont, background, foreground, vlabel.c_str(), scaleLineWidth,
                    SCALE_RISKA, VERTICAL_SCALE_MARGIN, VERTICAL_SCALE_SPACING) ;
    }
}

void AnalysisWindow::do_mouse( const Event * __event ) const
{
    if ( get_view_mode() != WINDOW::VIEW_NONE )
        return ;
    
    Application * app = static_cast<Application*> (GetWindowData(getWindowId(), "Application")) ;

    if ( !app )
        return ;
    
    if (app->getCursorTrackingLinesUsage() ||
        getDrawCursorTrackingCrossLines())
    {
        switch (__event->type)
        {
            case ENTER_NOTIFY:
            case MOTION_NOTIFY:
                app->getWindowsManager().updateCursorTracking( *this, {__event->motion.x, __event->motion.y} ) ;
                break;
            
            case LEAVE_NOTIFY:
                app->getWindowsManager().updateCursorTracking( *this, {-1.0, -1.0} ) ;
                break;
                
            default:
                break;
        }
    }
}

void AnalysisWindow::do_select_grasp_proc( const ButtonEvent * __button_event )
{
    return ;
}

void AnalysisWindow::do_select_exit_proc()
{
    return ;
}

int AnalysisWindow::do_edit_grasp_proc( const ButtonEvent * __button_event )
{
    return 0;
}

void AnalysisWindow::do_edit_move_proc( const MotionEvent * __motion_event )
{
    return ;
}

void AnalysisWindow::do_edit_free_proc( const ButtonEvent * __button_event )
{
    return ;
}

void AnalysisWindow::do_edit_exit_proc()
{
    return ;
}

void AnalysisWindow::popup_add_picking_menu(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    app->addCurrentPointToPicking() ;
}

void AnalysisWindow::popup_add_picking_menu_map(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    SetMenuItemEnabling( __details, app->getPickingPoint() == nullptr ) ;
}

void AnalysisWindow::popup_add_interp_picking_menu(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    app->addCurrentPointToPickingWithInterpolation() ;
}

void AnalysisWindow::popup_add_interp_picking_menu_map(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    SetMenuItemEnabling( __details, app->getPickingPoint() == nullptr ) ;
}

void AnalysisWindow::popup_del_picking_menu(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    app->deleteCurrentPickingPoint() ;
}

void AnalysisWindow::popup_del_picking_menu_map(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    SetMenuItemEnabling( __details, app->getPickingPoint() != nullptr ) ;
}

void AnalysisWindow::popup_set_interp_curve_menu(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    app->setInterpolatedPickingPoint() ;
}

void AnalysisWindow::popup_set_interp_curve_menu_map(const MenuItemDetails& __details)
{
    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    SetMenuItemEnabling( __details, app->getPickingPoint() != nullptr ) ;
}

void AnalysisWindow::popup_window_level_menu(const MenuItemDetails& __details)
{
    if (__details.postedFrom.area != WINDOW_AREA)
        return ;

    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    app->getLevelsDialog(__details.postedFrom.id)->show() ;
}

void AnalysisWindow::popup_window_adjust_level_menu(const MenuItemDetails& __details)
{
    if (__details.postedFrom.area != WINDOW_AREA)
        return ;

    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    app->getWindowsManager().adjustLevel( __details.postedFrom.id ) ;
    app->getLevelsDialog(__details.postedFrom.id)->update() ;
}

}
}
}
