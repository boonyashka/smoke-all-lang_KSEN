

#include "dm/DMScale.h"
#include "Nmo/Hodographs.h"

#include "Application.h"
#include "SuperGatherWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

const ToolkitColor SuperGatherWindow::DEF_MUTE_COLOR( 1.0, 1.0, 1.0, 0.7 ) ;

bool SuperGatherWindow::getNmoApplying() const
{
    return m_apply_nmo ;
}

void SuperGatherWindow::setNmoApplying( bool __apply )
{
    using namespace Prime::lib::preprocessing_script ;

    m_apply_nmo = __apply ;

    if ( !getDataReader() )
        return ;

    if ( !__apply || !m_active_picking_point )
    {
        getDataReader()->setFuncs( std::vector<IProcedure*>() ) ;
        return ;
    }

    NmoProcedure::Parameters params ;
    params.nmo_source = NmoProcedure::Manual ;
    params.hodograph_type = getDataReader()->getHodographType() ;
    params.manual_params_t0.resize( m_active_picking_point->curve().size() ) ;
    params.manual_params_val.resize( m_active_picking_point->curve().size() ) ;

    const double scale_factor = (getDataReader()->getDataType() == FR_IO_TIME ? 1000 : 1) ;

    for ( int i = 0; i < m_active_picking_point->curve().size(); ++i )
    {
        params.manual_params_t0[i] = scale_factor * m_active_picking_point->curve()[i].y ;
        params.manual_params_val[i] = m_active_picking_point->curve()[i].x ;
    }

    m_nmo_procedure.resetApplyInfo() ;
    m_nmo_procedure.setParams( params ) ;
    getDataReader()->setFuncs( std::vector<IProcedure*>(1, &m_nmo_procedure) ) ;
}

bool SuperGatherWindow::getMutingDrawing() const
{
    return m_draw_muting ;
}

void SuperGatherWindow::setMutingDrawing( bool __draw )
{
    m_draw_muting = __draw ;
}

void SuperGatherWindow::drawBackPlane( int __x0, int __y0, int __x1, int __y1 ) const
{
    const_cast<SuperGatherWindow*> (this)->setNmoApplying( getNmoApplying() ) ;
    AnalysisWindow::drawBackPlane( __x0, __y0, __x1, __y1 ) ;
}

void SuperGatherWindow::drawFrontPlane( int __x0, int __y0, int __x1, int __y1 ) const
{
    using namespace Prime::lib::Nmo ;

    if ( getDataReader() )
    {
        drawHodographs( __x0, __y0, __x1, __y1 ) ;
        drawMuting( __x0, __y0, __x1, __y1 ) ;
    }

    AnalysisWindow::drawFrontPlane(__x0, __y0, __x1, __y1) ;
}

void SuperGatherWindow::drawMuting( int __x0, int __y0, int __x1, int __y1 ) const
{
    if ( !getDataReader() || !getMutingDrawing() )
        return ;

    Prime::Geom::coord_2_dbl point ;

    const int traces_num = getViewTracesCache().size() ;
    const auto& segment = polyline.segments().at(getActiveAnalysisPointIndex()) ;

    m_line_buffer.resize( 2 * traces_num + 4 ) ;
    DMPoint * ext_muting_curve = m_line_buffer.data() ;
    DMPoint * int_muting_curve = m_line_buffer.data() + traces_num + 2 ;

    bool draw_ext_muting = false ;
    bool draw_int_muting = false ;
    
    WINDOW::DrawingContext * context = GetWindowContext(getWindowId());

    for ( int i = 0; i < traces_num; ++i )
    {
        const int itrace = getViewTracesCache().indices()[i] ;
        const Prime::IO::TraceHead& header = getViewTracesCache().headers()[i] ;

        const double ext_muting = header.FGet( TYPEITEM(MutingExtHeader) ) ;
        const double int_muting = header.FGet( TYPEITEM(MutingIntHeader) ) ;

        double trace_x, trace_y ;
        trace2profile( getDataReader(), itrace, 0, &trace_x, &trace_y ) ;

        profile2world( segment, trace_x, ext_muting, &point.x, &point.y ) ;
        calcCoord( point.x, point.y, &point.x, &point.y ) ;
        ext_muting_curve[i] = {point.x, point.y} ;
        draw_ext_muting = (draw_ext_muting || (point.y > __y0 + 1 && point.y < __y1 - 1)) ;

        profile2world( segment, trace_x, int_muting, &point.x, &point.y ) ;
        calcCoord( point.x, point.y, &point.x, &point.y ) ;
        int_muting_curve[i] = {point.x, point.y} ;
        draw_int_muting = (draw_int_muting || (point.y > __y0 + 1 && point.y < __y1 - 1)) ;
    }

    if ( draw_ext_muting )
    {
        ext_muting_curve[traces_num].x = ext_muting_curve[traces_num - 1].x ;
        ext_muting_curve[traces_num].y = __y0 ;
        ext_muting_curve[traces_num + 1].x = ext_muting_curve[0].x ;
        ext_muting_curve[traces_num + 1].y = __y0 ;

        // Drawing muting overlay polygon
        DMSetColor( context, DEF_MUTE_COLOR ) ;
        DMFillPolygon( context, ext_muting_curve, traces_num + 2 ) ;

        // Drawing muting curves
        DMSetColor( context, res_muting_color ) ;
        DMSetLineStyle( context, DEF_LINE_WIDTH, DM_LINE_SOLID, DM_CAP_BUTT, DM_JOIN_MITER ) ;
        DMDrawLines( context, ext_muting_curve, traces_num ) ;
    }

    if ( draw_int_muting )
    {
        int_muting_curve[traces_num].x = int_muting_curve[traces_num - 1].x ;
        int_muting_curve[traces_num].y = __y1 ;
        int_muting_curve[traces_num + 1].x = int_muting_curve[0].x ;
        int_muting_curve[traces_num + 1].y = __y1 ;

        // Drawing muting overlay polygon
        DMSetColor( context, DEF_MUTE_COLOR ) ;
        DMFillPolygon( context, int_muting_curve, traces_num + 2 ) ;

        // Drawing muting curves
        DMSetColor( context, res_muting_color ) ;
        DMSetLineStyle( context, DEF_LINE_WIDTH, DM_LINE_SOLID, DM_CAP_BUTT, DM_JOIN_MITER ) ;
        DMDrawLines( context, int_muting_curve, traces_num ) ;
    }
}

void SuperGatherWindow::drawHodographs( int __x0, int __y0, int __x1, int __y1 ) const
{
    if ( !getDataReader() || !m_active_picking_point || m_active_picking_point->curve().empty() )
        return ;

    Prime::Geom::coord_2_dbl point ;

    const int traces_num = getViewTracesCache().size() ;
    const auto& segment = polyline.segments().at(getActiveAnalysisPointIndex()) ;
    
    WINDOW::DrawingContext * context = GetWindowContext(getWindowId());

    for ( const auto&  curve_point : m_active_picking_point->curve() )
    {
        m_line_buffer.clear() ;
        m_line_buffer.reserve( getViewTracesCache().size() ) ;

        const double t0 = curve_point.y ;
        const double param = curve_point.x ;

        for ( int i = 0; i < traces_num; ++i )
        {
            const int itrace = getViewTracesCache().indices()[i] ;
            const Prime::IO::TraceHead& header = getViewTracesCache().headers()[i] ;

            const double l = header.GetL() ;
            const double value = (getNmoApplying() ? t0
                                  : get_hodograph_value(getDataReader()->getHodographType(), t0, param, l)) ;

            trace2profile( getDataReader(), itrace, 0, &point.x, &point.y ) ;
            profile2world( segment, point.x, value, &point.x, &point.y ) ;
            calcCoord( point.x, value, &point.x, &point.y ) ;

            m_line_buffer.push_back({point.x, point.y} ) ;
        }

        DMSetColor( context, res_picking_color ) ;
        DMSetLineStyle( context, DEF_LINE_WIDTH, DM_LINE_SOLID, DM_CAP_BUTT, DM_JOIN_MITER ) ;
        DMDrawLines( context, m_line_buffer.data(), m_line_buffer.size() ) ;
    }
}

void SuperGatherWindow::do_expose_scale( WINDOW::ScaleOrientation __dir, double __value, int __width, int __height, int __begin, int __end ) const
{
    if ( __dir == WINDOW::VERTICAL )
        return AnalysisWindow::do_expose_scale( __dir, __value, __width, __height, __begin, __end ) ;

    ScaleMode horizontalMode, verticalMode;
    getScaleMode(&horizontalMode, &verticalMode);

    if ( horizontalMode == SCALE_INSIDE )
        return ;

    int width, height ;
    GetWindowGeom( getWindowId(), &width, &height ) ;

    int x0, x1 ;
    std::vector<Prime::Geom::TraceKeyLabel> scale_values ;

    if ( getHorizontalScaleValues(width, height, scale_values, &x0, &x1))
    {
        std::string hlabel, vlabel ;
        getScaleLabel( &hlabel, &vlabel ) ;

        ToolkitColor background = GetWindowScaleBackground( getWindowId(), __dir ) ;
        ToolkitColor foreground = GetWindowScaleForeground( getWindowId(), __dir ) ;

        DMDrawScale1(getWindowId(), __dir, false, __width, __height, __begin, __end, x0, x1, 0, scale_values,
                     scaleFont, background, foreground, hlabel.c_str(), scaleLineWidth,
                     SCALE_RISKA, HORIZONTAL_SCALE_MARGIN, HORIZONTAL_SCALE_SPACING) ;
    }
}

bool SuperGatherWindow::getHorizontalScaleValues(int __width, int __height,
                                                 std::vector<Prime::Geom::TraceKeyLabel>& __scale_labels,
                                                 int * __x0, int * __x1) const
{
    if ( __width <= 0 || __height <= 0 )
        return false ;

    if ( !getDataReader() )
        return false ;

    if (getActiveAnalysisPointIndex() < 0 ||
        getActiveAnalysisPointIndex() >= getPolyline().segments().size())
        return false ;

    prepareCoord( 0, 0, __width, __height ) ;

    const auto& view_cache = getViewTracesCache() ;
    __scale_labels.resize( view_cache.size() ) ;

    if ( view_cache.size() <= 0 )
        return false ;

    *__x0 = __width ;
    *__x1 = 0 ;

    const Prime::Data::Polyline::Segment& segment = getPolyline().segments().at(getActiveAnalysisPointIndex()) ;

    for ( int i = 0; i < view_cache.size(); ++i )
    {
        const int itrace = view_cache.indices()[i] ;

        Prime::Geom::coord_2_dbl profile, world, screen ;
        trace2profile( getDataReader(), itrace, 0, &profile.x, &profile.y ) ;
        profile2world( segment, profile.x, profile.y, &world.x, &world.y ) ;
        calcCoord( world.x, world.y, &screen.x, &screen.y ) ;

        const int screen_x = int( screen.x + 0.4999 ) ;
        *__x0 = std::min( *__x0, screen_x ) ;
        *__x1 = std::max( *__x1, screen_x ) ;

        std::ostringstream skey ;
        skey << getDataReader()->logical2secondarykey(itrace) ;

        __scale_labels[i].x = screen_x ;
        __scale_labels[i].label = skey.str() ;
    }

    return true ;
}

WINDOW_BOX * SuperGatherWindow::make()
{
    static MENU::ITEM window_menu_items[] = {
        { "Apply NMO",     'A',  0, 0, NOTHING, TOGGLE_M,    0, 0, apply_nmo_menu, apply_nmo_menu_map },
        { "Show muting",   'D',  0, 0, NOTHING, TOGGLE_M,    0, 0, show_muting_menu, show_muting_menu_map },
        { "",              '\0', 0, 0, NOTHING, SEPARATOR_M },
        { "Level...",      'L',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_window_level_menu },
        { "Adjust level",  'A',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_window_adjust_level_menu },
        { 0 }
    };

    static MENU window_menu[] = {
        { window_menu_items },
        { 0 }
    };

    static WINDOW_BOX window_prototype = {
        "Gather",
        -1,
        WINDOW::APPL_SCROLL_WINDOW,
        configure,
        expose,
        expose_interactive,
        print,
        mouse,
        nullptr,
        NULL,
        200,
        200,
        true,
        true,
        true,
        true,
        scroll,
        expose_scale,
        window_menu,
        0, //map_delta,
        WINDOW::GRAPHICS_2D
    };

    return &window_prototype ;
}

void SuperGatherWindow::apply_nmo_menu(const MenuItemDetails& __details)
{
    if (__details.postedFrom.area != WINDOW_AREA)
        return ;

    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    bool apply = GetMenuToggleButtonState( __details ) ;
    app->getWindowsManager().setNmoApplying( apply ) ;
}

void SuperGatherWindow::apply_nmo_menu_map(const MenuItemDetails& __details)
{
    if (__details.postedFrom.area != WINDOW_AREA)
        return ;

    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    bool apply = app->getWindowsManager().getNmoApplying() ;
    SetMenuToggleButtonState( __details, apply ) ;
}

void SuperGatherWindow::show_muting_menu(const MenuItemDetails& __details)
{
    if (__details.postedFrom.area != WINDOW_AREA)
        return ;

    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    bool draw = GetMenuToggleButtonState( __details ) ;
    app->getWindowsManager().setMutingDrawing( draw ) ;
}

void SuperGatherWindow::show_muting_menu_map(const MenuItemDetails& __details)
{
    if (__details.postedFrom.area != WINDOW_AREA)
        return ;

    Application * app = static_cast<Application*> (GetWindowData(__details.postedFrom.id, "Application")) ;

    if ( !app )
        return ;

    bool draw = app->getWindowsManager().getMutingDrawing() ;
    SetMenuToggleButtonState( __details, draw ) ;
}

}
}
}
