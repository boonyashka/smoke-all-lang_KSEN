
#include <vector>
#include <algorithm>

#include "Application.h"
#include "SpectrumWindow.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

void SpectrumWindow::drawFrontPlane( int __x0, int __y0, int __x1, int __y1 ) const
{
    drawPickingCurve(__x0, __y0, __x1, __y1, m_active_interpolated_picking_point, res_interpolated_picking_color,
                     DEF_INTERPOLATED_PICKING_LINE_WIDTH, DEF_INTERPOLATED_PICKING_POINT_SIZE) ;

    drawPickingCurve(__x0, __y0, __x1, __y1, m_active_picking_point, res_picking_color,
                     DEF_PICKING_LINE_WIDTH, DEF_PICKING_POINT_SIZE) ;

    AnalysisWindow::drawFrontPlane(__x0, __y0, __x1, __y1) ;
}

void SpectrumWindow::drawPickingCurve(int __x0, int __y0, int __x1, int __y1,
                                      const PickingPoint * __picking_point,
                                      const ToolkitColor& __picking_color,
                                      int __line_width, int __point_size) const
{
    if ( !__picking_point || __picking_point->empty() )
        return ;

    m_line_buffer.clear() ;
    m_line_buffer.reserve( __picking_point->curve().size() ) ;

    m_points_buffer.clear() ;
    m_points_buffer.reserve( __picking_point->curve().size() ) ;

    DMPoint point ;
    DMCircle circle ;
    circle.diam = __point_size ;
    
    WINDOW::DrawingContext * context = GetWindowContext(getWindowId());

    for ( const auto& p : __picking_point->curve() )
    {
        data2screen( p.x, p.y, &point.x, &point.y ) ;

        circle.x = point.x ;
        circle.y = point.y ;

        m_line_buffer.push_back( point ) ;
        m_points_buffer.push_back( circle ) ;
    }

    DMSetColor( context, __picking_color ) ;
    DMSetLineStyle( context, __line_width, DM_LINE_SOLID, DM_CAP_BUTT, DM_JOIN_ROUND ) ;
    DMDrawLines( context, m_line_buffer.data(), m_line_buffer.size() ) ;
    DMFillCircles( context, m_points_buffer.data(), m_points_buffer.size() ) ;

    if ( !m_line_buffer.empty() )
    {
        DMSetLineStyle( context, __line_width, DM_LINE_DASHED, DM_CAP_BUTT, DM_JOIN_ROUND ) ;
        DMDrawLine( context, m_line_buffer.front().x, m_line_buffer.front().y, m_line_buffer.front().x, __y0 ) ;
        DMDrawLine( context, m_line_buffer.back().x, m_line_buffer.back().y, m_line_buffer.back().x, __y1 ) ;
    }
}

void SpectrumWindow::do_mouse( const Event * __event ) const
{
    if ( get_view_mode() != WINDOW::VIEW_NONE )
        return ;
    
    AnalysisWindow::do_mouse( __event ) ;

    Application * app = static_cast<Application*> (GetWindowData(getWindowId(), "Application")) ;

    if ( !app )
        return ;

    if ( !m_active_picking_point )
        return ;

    static Prime::Geom::coord_2_dbl * captured_point = 0 ;
    static int captured_point_index = 0 ;

    if ( __event->type == BUTTON_PRESS && __event->button.button == BUTTON1 )
    {
        captured_point = 0 ;

        Prime::Geom::coord_2_dbl pick ;
        screen2data( __event->button.x, __event->button.y, &pick.x, &pick.y ) ;
        
        const bool is_ctrl_pressed = (__event->button.state & CONTROL_MASK) ;
        
        // Finding clicked point
        for ( auto it = m_active_picking_point->curve().begin();
                it != m_active_picking_point->curve().end(); ++it )
        {
            if ( isClickVeryCloseToPoint(__event, *it) )
            {
                captured_point = &(*it) ;
                captured_point_index = std::distance(m_active_picking_point->curve().begin(), it) ;
                break ;
            }
        }

        // Adding new point
        if ( !captured_point && !is_ctrl_pressed )
        {
            auto it = std::lower_bound(m_active_picking_point->curve().begin(),
                                       m_active_picking_point->curve().end(), pick.y,
                                       [](const Prime::Geom::coord_2_dbl& __p, double __y) {
                                           return __p.y < __y ;
                                       }) ;
            
            it = m_active_picking_point->curve().insert( it, pick ) ;
            captured_point = &(*it) ;
            captured_point_index = std::distance(m_active_picking_point->curve().begin(), it) ;
        }
            // Deleting picked point
        else if ( captured_point && is_ctrl_pressed )
        {
            auto& curve = m_active_picking_point->curve() ;
            curve.erase( curve.begin() + captured_point_index ) ;
            captured_point = 0 ;
        }

        app->updatePickingDependedStuff() ;
    }
    else if ( __event->type == BUTTON_RELEASE )
    {
        if ( captured_point )
        {
            // Don't allow to move point above or below top/down points
            const auto& curve = m_active_picking_point->curve() ;

            if (captured_point_index >= 0 &&
                captured_point_index < curve.size())
            {
                if ( captured_point_index > 0 )
                    captured_point->y = std::max(captured_point->y, curve[captured_point_index - 1].y + getDataReader()->getDt()) ;

                if ( captured_point_index + 1 < curve.size() )
                    captured_point->y = std::min(captured_point->y, curve[captured_point_index + 1].y - getDataReader()->getDt()) ;
            }

            // Reset captured point and update
            captured_point = 0 ;
            app->getPickingStorage().informAboutChanges() ;
            app->updatePickingDependedStuff() ;
        }
    }
    else if ( __event->type == MOTION_NOTIFY )
    {
        if ( captured_point )
        {
            int width, height ;
            GetWindowGeom( getWindowId(), &width, &height ) ;

            double screen_x = std::max<double>( __event->button.x, 0 ) ;
            screen_x = std::min<double>( screen_x, width ) ;

            double screen_y = std::max<double>( __event->button.y, 0 ) ;
            screen_y = std::min<double>( screen_y, height ) ;

            screen2data( screen_x, screen_y, &captured_point->x, &captured_point->y ) ;
            app->updatePickingDependedStuff() ;
        }
    }
}

void SpectrumWindow::screen2data( double __screen_x, double __screen_y, double * __x, double * __y ) const
{
    if (getActiveAnalysisPointIndex() < 0 ||
        getActiveAnalysisPointIndex() >= polyline.segments().size())
        return ;

    calcWorld( __screen_x, __screen_y, __x, __y ) ;
}

void SpectrumWindow::data2screen( double __x, double __y, double * __screen_x, double * __screen_y ) const
{
    if (getActiveAnalysisPointIndex() < 0 ||
        getActiveAnalysisPointIndex() >= polyline.segments().size())
        return ;

    calcCoord( __x, __y, __screen_x, __screen_y ) ;
}

bool SpectrumWindow::isClickVeryCloseToPoint(const Event * __event, const Prime::Geom::coord_2_dbl& __point) const
{
    if ( !__event )
        return false ;

    Prime::Geom::coord_2_dbl screen ;
    data2screen( __point.x, __point.y, &screen.x, &screen.y ) ;

    return (std::abs(screen.x - __event->button.x) < DEF_PICKING_POINT_TOLLERANCE &&
            std::abs(screen.y - __event->button.y) < DEF_PICKING_POINT_TOLLERANCE) ;
}

WINDOW_BOX * SpectrumWindow::make()
{
    static MENU::ITEM window_menu_items[] = {
        { "Add picking",              'A',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_add_picking_menu, popup_add_picking_menu_map },
        { "Add interpolated picking", 'A',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_add_interp_picking_menu, popup_add_interp_picking_menu_map },
        { "Delete picking",           'D',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_del_picking_menu, popup_del_picking_menu_map },
        { "Set interpolated curve",   'S',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_set_interp_curve_menu, popup_set_interp_curve_menu_map },
        { "",                         '\0', 0, 0, NOTHING, SEPARATOR_M                                          },
        { "Level...",                 'L',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_window_level_menu },
        { "Adjust level",             'A',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_window_adjust_level_menu },
        { 0 }
    };

    static MENU window_menu[] = {
        { window_menu_items },
        { 0 }
    };

    static WINDOW_BOX window_prototype = {
        "Spectrum",
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

}
}
}
