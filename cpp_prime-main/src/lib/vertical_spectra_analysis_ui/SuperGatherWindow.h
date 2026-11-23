/* 
 * File:   SuperGatherWindow.h
 * Author: ranisimov
 *
 * Created on July 20, 2023, 2:46 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_SUPER_GATHER_WINDOW_H
#define VERTICAL_SPECTRA_ANALYSIS_SUPER_GATHER_WINDOW_H

#if HAVE_CONFIG_H
#include <config.h>
#endif


#include "toolkit/Appl.h"
#include "preprocessing_scripts/procedure_NMO.h"

#include "AnalysisWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of super-gather window.
 */
class SuperGatherWindow : public AnalysisWindow
{
    static const int DEF_LINE_WIDTH = 2 ; //!< Width of hodograph line.
    static const ToolkitColor DEF_MUTE_COLOR ; //!< Mute color, i.e. color of overlay.
    
    /**
     * @brief Constructs window object.
     * @param[in] __window Window identifier which will be used for visualization.
     */
public:
    SuperGatherWindow( int __window ) :
    AnalysisWindow(__window, SpectraDataType::SuperGather) { }

public:
    virtual ~SuperGatherWindow() {}
    
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
     * @brief Returns flag of muting curves drawing.
     */
public:
    bool getMutingDrawing() const ;
    
    /**
     * @brief Setups muting curves drawing.
     */
public:
    void setMutingDrawing( bool __draw ) ;
    
    /**
     * @brief Draws back plane content.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     */
protected:
    virtual void drawBackPlane( int __x0, int __y0, int __x1, int __y1 ) const ;
    
    /**
     * @brief Draws front plane content.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     */
protected:
    virtual void drawFrontPlane( int __x0, int __y0, int __x1, int __y1 ) const ;
    
    /**
     * @brief Draws muting data.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     */
protected:
    void drawMuting( int __x0, int __y0, int __x1, int __y1 ) const ;
    
    /**
     * @brief Draws hodographs.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     */
protected:
    void drawHodographs( int __x0, int __y0, int __x1, int __y1 ) const ;

    /**
     * @brief Process expose event of scales.
     * @param[in] __dir Direction of scale.
     * @param[in] __value Adjustment scale value.
     * @param[in] __width Width of scale.
     * @param[in] __height Height of scale.
     * @param[in] __begin Begin value of scale.
     * @param[in] __end End value of scale.
     */
private:
    virtual void do_expose_scale( WINDOW::ScaleOrientation __dir, double __value, int __width, int __height, int __begin, int __end ) const ;

    /**
     * @brief Extracts horizontal scale values.
     */
private:
    bool getHorizontalScaleValues(int __width, int __height,
                                  std::vector<Prime::Geom::TraceKeyLabel>& __scale_labels,
                                  int * __x0, int * __x1) const ;
    
    /**
     * @brief Creates window.
     */
public:
    static WINDOW_BOX * make() ;
    
    /**
     * @name Callback handlers.
     * @{ 
     */
protected:
    static void apply_nmo_menu(const MenuItemDetails& __details) ;
    static void apply_nmo_menu_map(const MenuItemDetails& __details) ;
    static void show_muting_menu(const MenuItemDetails& __details) ;
    static void show_muting_menu_map(const MenuItemDetails& __details) ;
    //!@}
    
private:
    bool m_apply_nmo = false ; //!< If true then NMO with current law will be applied.
    bool m_draw_muting = true ; //!< If true then muting will be drawn.
    Prime::lib::preprocessing_script::NmoProcedure m_nmo_procedure ; //!< Procedure of NMO.
    mutable std::vector<DMPoint> m_line_buffer ; //!< Buffer for line.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_SUPER_GATHER_WINDOW_H */

