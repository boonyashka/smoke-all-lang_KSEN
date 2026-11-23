/* 
 * File:   WindowsManager.h
 * Author: ranisimov
 *
 * Created on July 12, 2023, 4:55 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_COMPONENTS_MANAGER_H
#define VERTICAL_SPECTRA_ANALYSIS_COMPONENTS_MANAGER_H

#include <vector>
#include <memory>

#include "seismicWindow/SeismicWindow.h"
#include "rawWindow/RawWindow.h"
#include "geom/RectTypes.h"
#include "primeAppGUI/appUtilities/HistoryStack.h"
#include "vertical_spectra_analysis/Types.h"
#include "vertical_spectra_analysis/PickingPoint.h"

#include "AnalysisWindow.h"
#include "SpectrumWindow.h"
#include "StackGatherWindow.h"
#include "SuperGatherWindow.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

class WindowsManager
{
    friend class Application ;

    // Internal types    
public:
    typedef DataReader Reader ;
    typedef AnalysisWindow BaseWindow ;
    typedef Prime::Geom::rect_dbl RectDbl ;

    /**
     * @brief Class of zoom info.
     */
private:

    struct ZoomInfo
    {

        bool operator == ( const ZoomInfo& __item ) const
        {
            return  gather_window_zoom == __item.gather_window_zoom &&
                    spectrum_window_zoom == __item.spectrum_window_zoom &&
                    stackgather_window_zoom == __item.stackgather_window_zoom ;
        }

        RectDbl gather_window_zoom ; //!< Zoom parameters of gather window (relative values).
        RectDbl spectrum_window_zoom ; //!< Zoom parameters of spectrum window (relative values).
        RectDbl stackgather_window_zoom ; //!< Zoom parameters of stack-gather window (relative values).
    } ;

    /**
     * @brief Bidirectional history stack of zoom parameters.
     * @details Extends base class providing direct access
     * to history elements with aims for its modification.
     */
private:

    class ZoomHistory : public Prime::lib::primeApp::HistoryStack<ZoomInfo>
    {
        typedef Prime::lib::primeApp::HistoryStack<ZoomInfo> BaseClass ;
    public:

        ZoomInfo& operator [] (int k)
        {

            if ( k < 0 || k >= BaseClass::size() )
                throw std::invalid_argument( "HistoryStack::operator[](int): Error. Requested index is out of range." ) ;

            return *BaseClass::m_history_stack[k] ;
        }
    } ;

    /**
     * @brief Constructs windows manager.
     */
public:
    WindowsManager( Application * __app ) ;

public:
    ~WindowsManager() ;

    /**
     * @brief Setups super-gather window. 
     */
public:
    void setGatherWindow( int __window_id ) ;

    /**
     * @brief Setups super-gather window. 
     */
public:
    void setSpectrumWindow( int __window_id ) ;

    /**
     * @brief Setups stack-gather window. 
     */
public:
    void setStackGatherWindow( int __window_id ) ;

    /**
     * @brief Returns flag of NMO applying.
     */
public:
    bool getNmoApplying() const ;

    /**
     * @brief Setups NMO applying.
     */
public:
    void setNmoApplying( bool __apply ) ;

    /**
     * @brief Returns flag of muting drawing.
     */
public:
    bool getMutingDrawing() const ;

    /**
     * @brief Setups flag of muting drawing.
     */
public:
    void setMutingDrawing( bool __draw ) ;
    
    /**
     * @brief Setups cursor tracking lines.
     */
public:
    void updateCursorTracking(const AnalysisWindow& __initiator,
                              const Prime::Geom::coord_2_dbl& __position) ;

    /**
     * @brief Updates/redraws windows.
     * @param[in] __mask Updating mask.
     */
public:
    void update( Prime::Vis::SeismicWindow::DrawMask __mask = Prime::Vis::SeismicWindow::ALL_PLANES ) ;

    /**
     * @brief Updates all components depended from picking process.
     */
protected:
    void updatePickingDependedStuff() ;

    /**
     * @brief Setups for visualization required portion index.
     * @param[in] __index Index of data point.
     * @param[in] __picking_point Pointer to picking point at required data point.
     * @param[in] __interpolated_picking_point Pointer to interpolated picking point at current data point.
     */
private:
    void setDataPoint(int __ipoint,
                      PickingPoint * __picking_point,
                      const PickingPoint * __interpolated_picking_point = nullptr) ;

    /**
     * @brief Loads data to windows.
     */
private:
    void loadData(const DataStorage& __data_storage,
                  const int __object_id) ;

    /**
     * @brief Loads data to window.
     */
private:
    void loadData(BaseWindow * __window,
                  std::unique_ptr<Reader>& __reader,
                  const DataStorage& __data_storage,
                  const int __object_id,
                  SpectraDataType __data_type) ;
    
    /**
     * @brief Unloads all data from windows.
     */
private:
    void unloadData() ;
    
    /**
     * @brief Unloads data of required window.
     */
private:
    void unloadData(BaseWindow * __window,
                    std::unique_ptr<Reader>& __reader) ;

    /**
     * @brief Initializes window.
     */
private:
    void init( BaseWindow * __window ) ;

    /**
     * @brief Makes synchronization of all connected windows.
     */
private:
    void initSynchronization() ;

    /**
     * @brief Returns list of windows.
     */
private:
    std::vector<BaseWindow*> getWindowsList() const ;

    /**
     * @brief Returns window by its id.
     */
private:
    BaseWindow * getWindowById( int __window_id ) ;

    /**
     * @brief Returns window index by its id.
     */
private:
    int getWindowIndexById( int __window_id ) const ;

    /**
     * @brief Adjusts level of all windows.
     */
public:
    void adjustLevelForAll( bool __redraw = true ) ;

    /**
     * @brief Makes adjusting of level.
     */
public:
    void adjustLevel( int __window_id ) ;

    /**
     * @brief Makes adjusting of level.
     */
private:
    void adjustLevel( BaseWindow * __window, bool __redraw = true ) ;

    /**
     * @brief Clears zoom history.
     */
private:
    void clearZoomHistory() ;

    /**
     * @brief Switches zoom mode.
     */
public:
    void switchZoom( bool __on ) ;

    /**
     * @brief Checks whether zoom mode is switched on.
     */
public:
    bool isZoomSwitchedOn() const ;

    /**
     * @brief Synchronizes and setups windows zoom.
     */
private:
    void setZoom( Prime::Vis::SeismicWindow * __window, const Prime::Geom::rect_dbl& __abs_zoom ) ;

    /**
     * @brief Setups zoom to windows.
     */
private:
    void setZoom( const ZoomInfo * __zoom_info ) ;

    /**
     * @brief Calculates relative zoom from absolute one.
     */
private:
    Prime::Geom::rect_dbl calcRelZoom(const Prime::Vis::SeismicWindow * __window,
                                      const Prime::Vis::SeismicWindow * __source_window,
                                      const Prime::Geom::rect_dbl& __abs_zoom) const ;

    /**
     * @brief Calculates absolute zoom from relative one.
     */
private:
    Prime::Geom::rect_dbl calcAbsZoom(const Prime::Vis::SeismicWindow * __window,
                                      const Prime::Geom::rect_dbl& __rel_zoom) const ;

    /**
     * @brief Checks possibility of zoom undo.
     */
public:
    bool canUndoZoom() const ;

    /**
     * @brief Checks possibility of zoom redo.
     */
public:
    bool canRedoZoom() const ;

    /**
     * @brief Undo zoom.
     */
public:
    void undoZoom() ;

    /**
     * @brief Redo zoom.
     */
public:
    void redoZoom() ;

    /**
     * @brief Setups view all zoom.
     */
public:
    void setViewAllZoom() ;

    /**
     * @name Window handlers.
     * @{
     */
private:
    static void zoom_handler( Prime::Vis::SeismicWindow * __window, const Prime::Geom::rect_dbl& __zoom ) ;
    //!@}

private:
    Application * m_app = nullptr ; //!< Pointer to parent application.
    ZoomHistory m_zoom_history ; //!< Zoom history stack.
    std::unique_ptr<SuperGatherWindow> m_gather_window ; //!< Super-gather window.
    std::unique_ptr<SpectrumWindow> m_spectrum_window ; //!< Super-gather window.
    std::unique_ptr<StackGatherWindow> m_stackgather_window ; //!< Stack-gather window.
    std::unique_ptr<Reader> m_gather_reader ; //!< Data reader of super-gather window.
    std::unique_ptr<Reader> m_spectrum_reader ; //!< Data reader of spectrum window.
    std::unique_ptr<Reader> m_stackgather_reader ; //!< Data reader of stack-gather window.
} ;

}
}
}


#endif /* VERTICAL_SPECTRA_ANALYSIS_COMPONENTS_MANAGER_H */

