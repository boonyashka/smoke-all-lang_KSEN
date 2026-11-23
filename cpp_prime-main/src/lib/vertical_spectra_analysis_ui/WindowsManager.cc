
#include <assert.h>
#include <memory>

#include "primeApp/PrimeAppNoX.h"
#include "configdirs/configdirs.h"
#include "geom/polygon.h"

#include "DataReader.h"
#include "WindowsManager.h"
#include "Application.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

WindowsManager::WindowsManager( Application * __app ) :
m_app(__app) { }

WindowsManager::~WindowsManager() { }

void WindowsManager::setGatherWindow( int __window_id )
{
    m_gather_window.reset( new SuperGatherWindow(__window_id) ) ;
    init( m_gather_window.get() ) ;
}

void WindowsManager::setSpectrumWindow( int __window_id )
{
    m_spectrum_window.reset( new SpectrumWindow(__window_id) ) ;
    init( m_spectrum_window.get() ) ;
}

void WindowsManager::setStackGatherWindow( int __window_id )
{
    m_stackgather_window.reset( new StackGatherWindow(__window_id) ) ;
    init( m_stackgather_window.get() ) ;
}

bool WindowsManager::getNmoApplying() const
{
    if (m_gather_window)
        return m_gather_window->getNmoApplying() ;

    return false ;
}

void WindowsManager::setNmoApplying( bool __apply )
{
    if (m_gather_window)
    {
        m_gather_window->setNmoApplying( __apply ) ;
        m_gather_window->redraw() ;
    }
}

bool WindowsManager::getMutingDrawing() const
{
    if (m_gather_window)
        return m_gather_window->getMutingDrawing() ;

    return false ;
}

void WindowsManager::setMutingDrawing( bool __draw )
{
    if (m_gather_window)
    {
        m_gather_window->setMutingDrawing( __draw ) ;
        m_gather_window->redraw() ;
    }
}

void WindowsManager::updateCursorTracking(const AnalysisWindow& __initiator,
                                          const Prime::Geom::coord_2_dbl& __position)
{
    using namespace Prime::Geom ;
    
    if ( !m_app )
        return ;
    
    if ( m_app->getCursorTrackingLinesUsage() )
    {
        if (m_gather_window)
        {
            auto position = (__initiator.getDataType() == SpectraDataType::SuperGather ? __position : coord_2_dbl(-1.0, __position.y)) ;
            m_gather_window->blockRedraw() ;
            m_gather_window->setDrawCursorTrackingCrossLines(true) ;
            m_gather_window->setCursorTrackingCrossLinesPosition(position) ;
            m_gather_window->unblockRedraw() ;
        }

        if (m_spectrum_window)
        {
            auto position = (__initiator.getDataType() != SpectraDataType::SuperGather ? __position : coord_2_dbl(-1.0, __position.y)) ;
            m_spectrum_window->blockRedraw() ;
            m_spectrum_window->setDrawCursorTrackingCrossLines(true) ;
            m_spectrum_window->setCursorTrackingCrossLinesPosition(position) ;
            m_spectrum_window->unblockRedraw() ;
        }

        if (m_stackgather_window)
        {
            auto position = (__initiator.getDataType() != SpectraDataType::SuperGather ? __position : coord_2_dbl(-1.0, __position.y)) ;
            m_stackgather_window->blockRedraw() ;
            m_stackgather_window->setDrawCursorTrackingCrossLines(true) ;
            m_stackgather_window->setCursorTrackingCrossLinesPosition(position) ;
            m_stackgather_window->unblockRedraw() ;
        }
        
        update( Prime::Vis::SeismicWindow::INTERACTIVE_PLANE ) ;
    }
    else
    {
        if (m_gather_window)
            m_gather_window->setDrawCursorTrackingCrossLines(false) ;

        if (m_spectrum_window)
            m_spectrum_window->setDrawCursorTrackingCrossLines(false) ;

        if (m_stackgather_window)
            m_stackgather_window->setDrawCursorTrackingCrossLines(false) ;
    }
}

void WindowsManager::update( Prime::Vis::SeismicWindow::DrawMask __mask )
{
    if (m_gather_window)
        m_gather_window->redraw( __mask ) ;

    if (m_spectrum_window)
        m_spectrum_window->redraw( __mask ) ;

    if (m_stackgather_window)
        m_stackgather_window->redraw( __mask ) ;
}

void WindowsManager::updatePickingDependedStuff()
{
    if (m_gather_window)
    {
        m_gather_window->redraw(m_gather_window->getNmoApplying()
                                ? Prime::Vis::SeismicWindow::ALL_PLANES
                                : Prime::Vis::SeismicWindow::FRONT_PLANE) ;
    }

    if (m_spectrum_window)
        m_spectrum_window->redraw( Prime::Vis::SeismicWindow::FRONT_PLANE ) ;

    if (m_stackgather_window)
        m_stackgather_window->redraw( Prime::Vis::SeismicWindow::FRONT_PLANE ) ;
}

void WindowsManager::setDataPoint(int __ipoint,
                                  PickingPoint * __picking_point,
                                  const PickingPoint * __interpolated_picking_point)
{
    if (m_gather_window)
        m_gather_window->setActiveDataPoint(__ipoint, __picking_point,
                                            __interpolated_picking_point) ;

    if (m_spectrum_window)
        m_spectrum_window->setActiveDataPoint(__ipoint, __picking_point,
                                              __interpolated_picking_point) ;

    if (m_stackgather_window)
        m_stackgather_window->setActiveDataPoint(__ipoint, __picking_point,
                                                 __interpolated_picking_point) ;
    
    setZoom( m_zoom_history.current() ) ;
}

void WindowsManager::loadData(const DataStorage& __data_storage,
                              const int __object_id)
{
    try
    {
        if (!__data_storage.getMetacube() ||
            !__data_storage.getMetacubeIndex())
            return ;

        loadData(m_gather_window.get(), m_gather_reader, __data_storage,
                 __object_id, SpectraDataType::SuperGather) ;

        loadData(m_spectrum_window.get(), m_spectrum_reader, __data_storage,
                 __object_id, SpectraDataType::Spectrum) ;

        loadData(m_stackgather_window.get(), m_stackgather_reader, __data_storage,
                 __object_id, SpectraDataType::StackGather) ;
        
        if ( !m_zoom_history.current() )
            clearZoomHistory() ;
        
        adjustLevelForAll( false ) ;
    }
    catch ( const std::exception& __err )
    {
        std::cerr << __err.what() << std::endl ;
    }
}

void WindowsManager::loadData(BaseWindow * __window,
                              std::unique_ptr<Reader>& __reader,
                              const DataStorage& __data_storage,
                              const int __object_id,
                              SpectraDataType __data_type)
{
    if ( !__window )
        return ;

    __reader.reset( __data_storage.createDataReader(__object_id, __data_type) ) ;
    __window->setDataReader( __reader.get() ) ;
}

void WindowsManager::unloadData()
{
    unloadData( m_gather_window.get(), m_gather_reader ) ;
    unloadData( m_spectrum_window.get(), m_spectrum_reader ) ;
    unloadData( m_stackgather_window.get(), m_stackgather_reader ) ;
}

void WindowsManager::unloadData(BaseWindow * __window,
                                std::unique_ptr<Reader>& __reader)
{
    if ( !__window )
        return ;
    
    __window->setActiveDataPoint( 0, nullptr ) ;
    __window->setDataReader( nullptr ) ;
    __reader.reset( nullptr ) ;
}

void WindowsManager::init( BaseWindow * __window )
{
    if ( !__window )
        return ;

    // Set zoom and scroll handlers
    __window->set_zoom_handler( zoom_handler ) ;
}

void WindowsManager::initSynchronization()
{
    std::vector<BaseWindow*> windows = getWindowsList() ;

    for ( int i = 1; i < windows.size(); ++i )
    {
        ConnectScrollBars(windows[i - 1]->getWindowId(), WINDOW::VERTICAL,
                          windows[i]->getWindowId(),     WINDOW::VERTICAL) ;
    }
}

std::vector<WindowsManager::BaseWindow*> WindowsManager::getWindowsList() const
{
    std::vector<BaseWindow*> windows ;

    if (m_gather_window)
        windows.push_back( m_gather_window.get() ) ;

    if (m_spectrum_window)
        windows.push_back( m_spectrum_window.get() ) ;

    if (m_stackgather_window)
        windows.push_back( m_stackgather_window.get() ) ;

    return windows ;
}

WindowsManager::BaseWindow * WindowsManager::getWindowById( int __window_id )
{
    if (m_gather_window && m_gather_window->getWindowId() == __window_id)
        return m_gather_window.get() ;

    if (m_spectrum_window && m_spectrum_window->getWindowId() == __window_id)
        return m_spectrum_window.get() ;

    if (m_stackgather_window && m_stackgather_window->getWindowId() == __window_id)
        return m_stackgather_window.get() ;

    return 0 ;
}

int WindowsManager::getWindowIndexById( int __window_id ) const
{
    if ( __window_id < 0 )
        return -1 ;

    std::vector<BaseWindow*> windows = getWindowsList() ;

    for ( int i = 0; i < windows.size(); ++i )
    {
        if ( windows[i] && windows[i]->getWindowId() == __window_id )
            return i ;
    }

    return -1 ;
}

void WindowsManager::adjustLevelForAll( bool __redraw )
{
    if (m_gather_window)
        adjustLevel( m_gather_window.get(), __redraw ) ;

    if (m_spectrum_window)
        adjustLevel( m_spectrum_window.get(), __redraw ) ;

    if (m_stackgather_window)
        adjustLevel( m_stackgather_window.get(), __redraw ) ;
}

void WindowsManager::adjustLevel( int __window_id )
{
    adjustLevel( getWindowById(__window_id), true ) ;
}

void WindowsManager::adjustLevel( BaseWindow * __window, bool __redraw )
{
    if ( !__window )
        return ;

    double color_min, color_max, wiggle_level ;
    __window->computeDefaultLevels(&color_min, &color_max, &wiggle_level) ;
    
    __window->blockRedraw() ;
    __window->setColorLevel( color_min, color_max ) ;
    __window->setWiggleLevel( wiggle_level ) ;
    __window->unblockRedraw() ;
    
    if (__redraw)
        __window->redraw() ;
}

void WindowsManager::clearZoomHistory()
{
    m_zoom_history.clear() ;
    setViewAllZoom() ;
}

void WindowsManager::switchZoom( bool __on )
{
    if (m_gather_window)
        m_gather_window->switch_view_mode( __on ? WINDOW::VIEW_ZOOM : WINDOW::VIEW_NONE ) ;

    if (m_spectrum_window)
        m_spectrum_window->switch_view_mode( __on ? WINDOW::VIEW_ZOOM : WINDOW::VIEW_NONE ) ;

    if (m_stackgather_window)
        m_stackgather_window->switch_view_mode( __on ? WINDOW::VIEW_ZOOM : WINDOW::VIEW_NONE ) ;
}

bool WindowsManager::isZoomSwitchedOn() const
{
    if (m_gather_window)
        return m_gather_window->get_view_mode() == WINDOW::VIEW_ZOOM ;

    if (m_spectrum_window)
        return m_spectrum_window->get_view_mode() == WINDOW::VIEW_ZOOM ;

    if (m_stackgather_window)
        return m_stackgather_window->get_view_mode() == WINDOW::VIEW_ZOOM ;

    return false ;
}

void WindowsManager::setZoom( Prime::Vis::SeismicWindow * __window, const Prime::Geom::rect_dbl& __abs_zoom )
{
    if ( !__window )
        return ;

    ZoomInfo * zoom_info = new ZoomInfo ;

    if ( !zoom_info )
        return ;

    if ( m_gather_window )
        zoom_info->gather_window_zoom = calcRelZoom(m_gather_window.get(),
                                                    __window, __abs_zoom) ;

    if ( m_spectrum_window)
        zoom_info->spectrum_window_zoom = calcRelZoom(m_spectrum_window.get(),
                                                      __window, __abs_zoom) ;

    if ( m_stackgather_window )
        zoom_info->stackgather_window_zoom = calcRelZoom(m_stackgather_window.get(),
                                                         __window, __abs_zoom) ;

    m_zoom_history.add( zoom_info ) ;

    setZoom( zoom_info ) ;
}

void WindowsManager::setZoom( const ZoomInfo * __zoom_info )
{
    if ( !__zoom_info )
        return ;

    switchZoom( false ) ;

    if (m_gather_window)
        m_gather_window->setZoom( calcAbsZoom(m_gather_window.get(),
                                              __zoom_info->gather_window_zoom), false ) ;

    if (m_spectrum_window)
        m_spectrum_window->setZoom( calcAbsZoom(m_spectrum_window.get(),
                                                __zoom_info->spectrum_window_zoom), false ) ;

    if (m_stackgather_window)
        m_stackgather_window->setZoom( calcAbsZoom(m_stackgather_window.get(),
                                                   __zoom_info->stackgather_window_zoom), false ) ;

    if (m_app)
        m_app->update() ;
}

Prime::Geom::rect_dbl WindowsManager::calcRelZoom(const Prime::Vis::SeismicWindow * __window,
                                                  const Prime::Vis::SeismicWindow * __source_window,
                                                  const Prime::Geom::rect_dbl& __abs_zoom) const
{
    Prime::Geom::rect_dbl zoom ;
    zoom.p1.x = zoom.p1.y = 0.0 ;
    zoom.p2.x = zoom.p2.y = 1.0 ;

    if ( !__source_window )
        return zoom ;

    Prime::Geom::rect_dbl ext = __source_window->calcExtends(0, 0) ;

    if ( __source_window == __window )
    {
        const double ext_size_x = (ext.p2.x - ext.p1.x) ;

        zoom.p1.x = (ext_size_x > 0 ? (__abs_zoom.p1.x - ext.p1.x) / ext_size_x : 0.0) ;
        zoom.p1.x = std::max<double>( zoom.p1.x, 0.0 ) ;
        zoom.p1.x = std::min<double>( zoom.p1.x, 1.0 ) ;

        zoom.p2.x = (ext_size_x > 0 ? (__abs_zoom.p2.x - ext.p1.x) / ext_size_x : 1.0) ;
        zoom.p2.x = std::max<double>( zoom.p2.x, 0.0 ) ;
        zoom.p2.x = std::min<double>( zoom.p2.x, 1.0 ) ;
    }

    const double ext_size_y = (ext.p2.y - ext.p1.y) ;

    zoom.p1.y = (ext_size_y > 0 ? (__abs_zoom.p1.y - ext.p1.y) / ext_size_y : 0.0) ;
    zoom.p1.y = std::max<double>( zoom.p1.y, 0.0 ) ;
    zoom.p1.y = std::min<double>( zoom.p1.y, 1.0 ) ;

    zoom.p2.y = (ext_size_y > 0 ? (__abs_zoom.p2.y - ext.p1.y) / ext_size_y : 1.0) ;
    zoom.p2.y = std::max<double>( zoom.p2.y, 0.0 ) ;
    zoom.p2.y = std::min<double>( zoom.p2.y, 1.0 ) ;

    return zoom ;
}

Prime::Geom::rect_dbl WindowsManager::calcAbsZoom(const Prime::Vis::SeismicWindow * __window,
                                                  const Prime::Geom::rect_dbl& __rel_zoom) const
{
    Prime::Geom::rect_dbl zoom(Prime::Geom::coord_2_dbl(0.0, 0.0),
                               Prime::Geom::coord_2_dbl(1.0, 1.0)) ;
    
    if ( !__window )
        return zoom ;

    Prime::Geom::rect_dbl ext = __window->calcExtends(0, 0) ;
    zoom.p1.x = (1.0 - __rel_zoom.p1.x) * ext.p1.x + __rel_zoom.p1.x * ext.p2.x ;
    zoom.p2.x = (1.0 - __rel_zoom.p2.x) * ext.p1.x + __rel_zoom.p2.x * ext.p2.x ;
    zoom.p1.y = (1.0 - __rel_zoom.p1.y) * ext.p1.y + __rel_zoom.p1.y * ext.p2.y ;
    zoom.p2.y = (1.0 - __rel_zoom.p2.y) * ext.p1.y + __rel_zoom.p2.y * ext.p2.y ;

    return zoom ;
}

bool WindowsManager::canUndoZoom() const
{
    return m_zoom_history.canUndo() ;
}

bool WindowsManager::canRedoZoom() const
{
    return m_zoom_history.canRedo() ;
}

void WindowsManager::undoZoom()
{
    if ( m_zoom_history.canUndo() )
        setZoom( m_zoom_history.undo() ) ;
}

void WindowsManager::redoZoom()
{
    if ( m_zoom_history.canRedo() )
        setZoom( m_zoom_history.redo() ) ;
}

void WindowsManager::setViewAllZoom()
{
    ZoomInfo * zoom_info = new ZoomInfo ;

    if ( !zoom_info )
        return ;

    Prime::Geom::rect_dbl zoom(Prime::Geom::coord_2_dbl(0.0, 0.0),
                               Prime::Geom::coord_2_dbl(1.0, 1.0)) ;

    if ( m_gather_window )
        zoom_info->gather_window_zoom = zoom ;

    if ( m_spectrum_window)
        zoom_info->spectrum_window_zoom = zoom ;

    if ( m_stackgather_window )
        zoom_info->stackgather_window_zoom = zoom ;

    m_zoom_history.add( zoom_info ) ;

    setZoom( zoom_info ) ;
}

void WindowsManager::zoom_handler( Prime::Vis::SeismicWindow * __window, const Prime::Geom::rect_dbl& __zoom )
{
    if ( !__window )
        return ;

    Application * app = static_cast<Application*> (GetWindowData(__window->getWindowId(), "Application")) ;

    if ( !app )
        return ;

    app->getWindowsManager().setZoom( __window, __zoom ) ;
}

}
}
}