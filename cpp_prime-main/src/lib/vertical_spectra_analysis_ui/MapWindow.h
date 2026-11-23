/* 
 * File:   MapWindow.h
 * Author: ranisimov
 *
 * Created on July 23, 2023, 1:26 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_MAP_WINDOW_H
#define VERTICAL_SPECTRA_ANALYSIS_MAP_WINDOW_H

#include <memory>

#include "toolkit/Appl.h"
#include "sliceWindow/SliceWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

/**
 * @brief Class of map window on which we display analysis points.
 */
class MapWindow
{
    /**
     * @brief Constructs map window.
     * @param[in] __app Parent application.
     * @param[in] __map_window Pointer to external map window.
     */
public:
    MapWindow(Application * __app, Prime::Vis::SliceWindow * __map_window ) ;
    
    /**
     * @brief Destructs window.
     */
public:
    ~MapWindow() ;
    
    /**
     * @brief Returns id of window.
     */
public:
    int getId() const ;
    
    /**
     * @brief Updates window content.
     */
public:
    void update() ;
    
    /**
     * @brief Updates drawing flag.
     */
public:
    void setDrawing( bool __draw ) ;
    
    /**
     * @brief Draws data on map.
     */
private:
    void draw() ;
    
    /**
     * @name Handlers.
     * @{
     */
private:
    static void map_window_draw_front_plane(const Prime::Vis::SliceWindow * __map_window) ;
    //!@}
    
private:
    Application * m_app ; //!< Parent application.
    Prime::Vis::SliceWindow * m_map_window ; //!< Pointer to map window.
};

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_MAP_WINDOW_H */

