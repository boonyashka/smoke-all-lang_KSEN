
#include "Application.h"
#include "MapWindow.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

MapWindow::MapWindow(Application * __app, Prime::Vis::SliceWindow * __map_window ) :
m_app(__app),
m_map_window(__map_window)
{
    if ( m_map_window )
        SetWindowData( m_map_window->getWindowId(), "VerticalSpectraAnalysis::MapWindow", this ) ;
}

MapWindow::~MapWindow() { }

int MapWindow::getId() const
{
    if ( !m_map_window )
        return -1 ;

    return m_map_window->getWindowId() ;
}

void MapWindow::update()
{
    if ( !m_map_window )
        return ;

    m_map_window->redraw() ;
}

void MapWindow::setDrawing( bool __draw )
{
    if ( !m_map_window )
        return ;

    if (__draw)
    {
        m_map_window->switch_highlight_mode( Prime::Vis::SliceWindow::HIGHLIGHT_USER_DEFINED ) ;
        m_map_window->set_after_draw_front_plane_handler( map_window_draw_front_plane ) ;
    }
    else
    {
        m_map_window->switch_highlight_mode( Prime::Vis::SliceWindow::HIGHLIGHT_NONE ) ;
        m_map_window->set_after_draw_front_plane_handler( 0 ) ;
    }
}

void MapWindow::draw()
{
    const int DEF_POINT_SIZE = 3 ;
    const int DEF_ACTIVE_POINT_SIZE = 5 ;
    const int DEF_ANALYSIS_POINT_SIZE = 10 ;
    const int DEF_CURRENT_POINT_SIZE = 15 ;

    const ToolkitColor DEF_COLOR( 0.35, 0.35, 0.35 ) ;
    const ToolkitColor DEF_ACTIVE_COLOR( 0, 0, 0.9 ) ;
    const ToolkitColor DEF_CURRENT_COLOR( 0, 0, 0 ) ;
    const ToolkitColor DEF_ANALYSIS_DONE_COLOR( 0, 0.9, 0 ) ;
    const ToolkitColor DEF_ANALYSIS_EMPTY_COLOR( 0.9, 0, 0 ) ;

    if ( !m_app || !m_map_window )
        return ;

    const int window_id = getId() ;
    const ObjectID active_object_id = m_app->getAnalysisObjectID() ;
    const int active_point_index = m_app->getDataPointIndex() ;

    Prime::Geom::coord_2_dbl active_point_screen ;
    int active_point_ncdp = INT_MIN ;
    ToolkitColor active_point_color ;
    int active_point_size = 0 ;


    WINDOW::DrawingContext * context = GetWindowContext(window_id);

    // Drawing all data points
    const auto& objects = m_app->getDataStorage().getObjects() ;

    for ( const auto& object_id : objects )
    {
        // Drawing data points
        {
            const auto& points = m_app->getDataStorage().getDataPoints(object_id) ;
            const ToolkitColor& color = (object_id == active_object_id ? DEF_ACTIVE_COLOR : DEF_COLOR) ;
            int point_size = (object_id == active_object_id ? DEF_ACTIVE_POINT_SIZE : DEF_POINT_SIZE) ;

            DMSetColor( context, color ) ;

            for ( size_t ipoint = 0; ipoint < points.size(); ++ipoint )
            {
                const auto& point = points[ipoint] ;

                Prime::Geom::coord_2_dbl screen ;
                m_map_window->calcCoord( point.x, point.y, &screen.x, &screen.y ) ;
                DMFillCircle( context, screen.x, screen.y, point_size ) ;

                if (ipoint == active_point_index && object_id == active_object_id )
                {
                    active_point_screen = screen ;
                    active_point_ncdp = point.ncdp ;
                    active_point_color = color ;
                    active_point_size = point_size ;
                }
            }
        }

        // Drawing analysis points
        for ( const auto& point : m_app->getPickingStorage().getPickingPoints(object_id) )
        {
            const ToolkitColor& color = (point.empty() ? DEF_ANALYSIS_EMPTY_COLOR : DEF_ANALYSIS_DONE_COLOR) ;

            Prime::Geom::coord_2_dbl screen ;
            m_map_window->calcCoord( point.x(), point.y(), &screen.x, &screen.y ) ;
            DMSetColor( context, color ) ;
            DMFillCircle( context, screen.x, screen.y, DEF_ANALYSIS_POINT_SIZE ) ;

            if ( point.ncdp() == active_point_ncdp && object_id == active_object_id )
            {
                active_point_color = color ;
                active_point_size = DEF_ANALYSIS_POINT_SIZE ;
            }
        }
    }

    // Drawing active point
    if ( active_point_ncdp != INT_MIN )
    {
        const Prime::Geom::coord_2_dbl& screen = active_point_screen ;

        DMSetColor( context, DEF_CURRENT_COLOR ) ;
        DMFillCircle( context, screen.x, screen.y, DEF_CURRENT_POINT_SIZE ) ;

        DMSetColor( context, active_point_color ) ;
        DMFillCircle( context, screen.x, screen.y, active_point_size ) ;
    }
}

void MapWindow::map_window_draw_front_plane(const Prime::Vis::SliceWindow * __map_window)
{
    if ( !__map_window )
        return ;

    MapWindow * _this = static_cast<MapWindow*> ( GetWindowData(__map_window->getWindowId(), "VerticalSpectraAnalysis::MapWindow") ) ;

    if ( !_this )
        return ;

    _this->draw() ;
}

}
}
}
