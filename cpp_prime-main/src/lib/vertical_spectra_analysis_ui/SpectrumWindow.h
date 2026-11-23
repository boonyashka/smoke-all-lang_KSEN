/* 
 * File:   SpectrumWindow.h
 * Author: ranisimov
 *
 * Created on July 24, 2023, 2:38 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_SPECTRUM_WINDOW_H
#define VERTICAL_SPECTRA_ANALYSIS_SPECTRUM_WINDOW_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "toolkit/Appl.h"

#include "AnalysisWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of spectrum window.
 */
class SpectrumWindow : public AnalysisWindow
{
    static const int DEF_PICKING_LINE_WIDTH = 3 ; //!< Width of picking line.
    static const int DEF_PICKING_POINT_SIZE = 9 ; //!< Diameter of picking point.
    static const int DEF_INTERPOLATED_PICKING_LINE_WIDTH = 2 ; //!< Width of picking line.
    static const int DEF_INTERPOLATED_PICKING_POINT_SIZE = 6 ; //!< Diameter of picking point.
    static const int DEF_PICKING_POINT_TOLLERANCE = 10 ; //!< Tollerance radius of picking point.

    /**
     * @brief Constructs window object.
     * @param[in] __window Window identifier which will be used for visualization.
     * @param[in] __data_type Type of data for which this window will be used.
     */
public:

    SpectrumWindow( int __window, SpectraDataType __data_type = SpectraDataType::Spectrum ) :
    AnalysisWindow( __window, __data_type ) { }

public:

    virtual ~SpectrumWindow() { }

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
     * @brief Draws picking point curve at window.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     * @param[in] __picking_point Picking point to be drawn.
     * @param[in] __picking_color Color for picking drawing.
     * @param[in] __line_width Width of line for picking drawing.
     * @param[in] __point_size Point size of picking points drawing.
     */
protected:
    void drawPickingCurve(int __x0, int __y0, int __x1, int __y1,
                          const PickingPoint * __picking_point,
                          const ToolkitColor& __picking_color,
                          int __line_width, int __point_size) const ;

    /**
     * @brief Process mouse event of window.
     * @param[in] __event Pointer to mouse event.
     */
protected:
    virtual void do_mouse( const Event * __event ) const ;

    /**
     * @brief Converts screen coordinate to data real coordinates.
     * @param[in] __screen_x Screen coordinate along x.
     * @param[in] __screen_y Screen coordinate along y.
     * @param[out] __x Output buffer for data real coordinate along x.
     * @param[out] __y Output buffer for data real coordinate along y.
     */
protected:
    void screen2data( double __screen_x, double __screen_y, double * __x, double * __y ) const ;

    /**
     * @brief Converts data real coordinates to screen coordinates.
     * @param[in] __x Data real coordinate along x.
     * @param[in] __y Data real coordinate along y.
     * @param[out] __screen_x Output buffer for screen coordinate along x.
     * @param[out] __screen_y Output buffer for screen coordinate along y.
     */
protected:
    void data2screen( double __x, double __y, double * __screen_x, double * __screen_y ) const ;

    /**
     * @brief Checks whether picked point is very close to node point.
     */
protected:
    bool isClickVeryCloseToPoint(const Event * __event, const Prime::Geom::coord_2_dbl& __point) const ;

    /**
     * @brief Creates window.
     */
public:
    static WINDOW_BOX * make() ;

private:
    mutable std::vector<DMPoint> m_line_buffer ; //!< Buffer for line.
    mutable std::vector<DMCircle> m_points_buffer ; //!< Buffer for points.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_SPECTRUM_WINDOW_H */

