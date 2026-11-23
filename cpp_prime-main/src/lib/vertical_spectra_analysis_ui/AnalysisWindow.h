/* 
 * File:   canvas.h
 * Author: ranisimov
 *
 * Created on July 18, 2023, 8:19 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_BASE_WINDOW_H
#define VERTICAL_SPECTRA_ANALYSIS_BASE_WINDOW_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <vector>
#include <functional>

#include "seismicWindow/SeismicWindow.h"
#include "rawWindow/RawWindow.h"
#include "vertical_spectra_analysis/PickingPoint.h"

#include "DataReader.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of base window for visualization super-gather/spectra/stack-gather.
 */
class AnalysisWindow : public Prime::Vis::SeismicWindow
{
    static const char * RES_PICKING_COLOR ; //!< Name of resource for picking color.
    static const char * RES_INTERP_PICKING_COLOR ; //!< Name of resource for picking color (interpolated curve).
    static const char * RES_MUTING_COLOR ; //!< Name of resource for muting curves color.

    /**
     * @brief Constructs window object.
     * @param[in] __window Window identifier which will be used for visualization.
     * @param[in] __data_type Type of data for which this window will be used.
     */
public:
    AnalysisWindow( int __window, SpectraDataType __data_type ) ;

public:
    virtual ~AnalysisWindow() ;
    
    /**
     * @brief Returns type of data for which visualization this window is used.
     */
public:
    SpectraDataType getDataType() const ;

    /**
     * @brief Setups polyline for visualization.
     */
public:
    void setPolyline(const Prime::Data::Polyline & __polyline) ;

    /**
     * @brief Returns access to polyline.
     */
public:
    const Prime::Data::Polyline & getPolyline() const ;

    /**
     * @brief Returns access to level calculation polyline.
     */
public:
    const Prime::Data::Polyline * getLevelPolyline() const ;

    /**
     * @brief Sets reader of data.
     * @param[in] __reader Pointer to reader.
     */
public:
    virtual void setDataReader( DataReader * __reader ) ;
    
    /**
     * @brief Returns reader.
     */
public:
    const DataReader * getDataReader() const ;
    
    /**
     * @brief Returns reader.
     */
protected:
    DataReader * getDataReader() ;

    /**
     * @brief Returns reader.
     */
public:
    virtual const Prime::Data::SeismicReader * getReader( size_t seg = 0 ) const ;

    /**
     * @brief Returns index of active analysis point.
     */
public:
    int getActiveAnalysisPointIndex() const ;

    /**
     * @brief Sets active analysis point with required index. 
     * @param[in] __index Index of data point.
     * @param[in] __picking_point Pointer to picking point at required data point.
     * @param[in] __interpolated_picking_point Pointer to interpolated picking point at current data point.
     */
public:
    virtual void setActiveDataPoint(int __index, PickingPoint * __picking_point,
                                    const PickingPoint * __interpolated_picking_point = nullptr) ;

    /**
     * @brief Computes horizontal range of visualization.
     * @param[out] __begin Output buffer for starting of horizontal range.
     * @param[out] __end Output buffer for ending of horizontal range.
     * @return True if range was calculated correctly.
     */
protected:
    virtual bool computeModelExtends( double * __begin, double * __end ) const ;

    /**
     * @brief Computes vertical range limits of reader.
     * @param[out] __min Output buffer for minimum of vertical range.
     * @param[out] __max Output buffer for maximum of vertical range.
     * @return True if range was calculated correctly.
     */
protected:
    virtual bool computeReaderDepthRange( double * __min, double * __max ) const ;

    /**
     * @brief Computes vertical range limits of model.
     * @attention This option is not required and false will be returned in any case.
     */
protected:
    virtual bool computeModelDepthRange( double * __min, double * __max ) const ;

    /**
     * @brief Dummy realization of virtual method.
     * @attention This option is not required and false will be returned in any case.
     */
protected:
    virtual bool computeReaderDepthRange(double x1, double x2, double * min, double * max) const ;

    /**
     * @brief Dummy realization of virtual method.
     * @attention This option is not required and false will be returned in any case.
     */
protected:
    virtual bool computeModelDepthRange(double x1, double x2, double * min, double * max) const ;

    /**
     * @brief Updates labels of window.
     */
protected:
    void updateLabels() ;

    /**
     * @brief Implements handler of colors changing by preferences controller.
     */
public:
    virtual void changeColors() ;
    
    /**
     * @brief Initializes window parameters.
     */
protected:
    void init() ;
    
    /**
     * @brief Returns access to resources.
     */
protected:
    ResTable * getResources() ;

    /**
     * @brief Loads window configuration.
     */
protected:
    void loadConfiguration() ;

    /**
     * @brief Saves window configuration.
     */
protected:
    void saveConfiguration() ;
    
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
     * @brief Draws interactive plane content.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     */
protected:
    void drawInteractivePlane( int __x0, int __y0, int __x1, int __y1 ) const ;

    /**
     * @brief Process configure signal of window.
     * @param[in] __width Width of window.
     * @param[in] __height Height of window.
     */
protected:
    virtual void do_configure( int __width, int __height ) const ;

    /**
     * @brief Process expose signal of window.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     */
protected:
    virtual void do_expose( int __x0, int __y0, int __x1, int __y1 ) const ;

    /**
     * @brief Process expose_interactive signal of window.
     * @param[in] __x0 X-coordinate of top-left corner.
     * @param[in] __y0 Y-coordinate of top-left corner.
     * @param[in] __x1 X-coordinate of bottom-right corner.
     * @param[in] __y1 Y-coordinate of bottom-right corner.
     */
protected:
    virtual void do_expose_interactive( int __x0, int __y0, int __x1, int __y1 ) const ;

    /**
     * @brief Process print signal of window.
     * @param[in] __width Width of window.
     * @param[in] __height Height of window.
     */
protected:
    virtual void do_print( int __width, int __height ) const ;
    
    /**
     * @brief Process expose event of scales.
     * @param[in] __dir Direction of scale.
     * @param[in] __value Adjustment scale value.
     * @param[in] __width Width of scale.
     * @param[in] __height Height of scale.
     * @param[in] __begin Begin value of scale.
     * @param[in] __end End value of scale.
     */
protected:
    virtual void do_expose_scale( WINDOW::ScaleOrientation __dir, double __value, int __width, int __height, int __begin, int __end ) const ;

    /**
     * @brief Process mouse event of window.
     * @param[in] __event Pointer to mouse event.
     */
protected:
    virtual void do_mouse( const Event * __event ) const ;

    /**
     * @brief Process selection event of window.
     * @param[in] __button_event Pointer to button event.
     */
protected:
    virtual void do_select_grasp_proc( const ButtonEvent * __button_event ) ;

    /**
     * @brief Process selection exit event of window.
     */
protected:
    virtual void do_select_exit_proc() ;

    /**
     * @brief Process starting of editing event.
     * @param[in] __button_event Pointer to button event.
     * @return True if editing can be processed.
     */
protected:
    virtual int do_edit_grasp_proc( const ButtonEvent * __button_event ) ;

    /**
     * @brief Process mouse move event on editing process.
     * @param[in] __motion_event Pointer to mouse motion event.
     */
protected:
    virtual void do_edit_move_proc( const MotionEvent * __motion_event ) ;

    /**
     * @brief Process event of editing ending.
     * @param[in] __button_event Pointer to button event.
     */
protected:
    virtual void do_edit_free_proc( const ButtonEvent * __button_event ) ;

    /**
     * @brief Process event of exiting from editing.
     */
protected:
    virtual void do_edit_exit_proc() ;
    
    /**
     * @name Callback handlers.
     * @{ 
     */
protected:
    static void popup_add_picking_menu(const MenuItemDetails& __details) ;
    static void popup_add_picking_menu_map(const MenuItemDetails& __details) ;
    static void popup_add_interp_picking_menu(const MenuItemDetails& __details) ;
    static void popup_add_interp_picking_menu_map(const MenuItemDetails& __details) ;
    static void popup_del_picking_menu(const MenuItemDetails& __details) ;
    static void popup_del_picking_menu_map(const MenuItemDetails& __details) ;
    static void popup_set_interp_curve_menu(const MenuItemDetails& __details) ;
    static void popup_set_interp_curve_menu_map(const MenuItemDetails& __details) ;
    static void popup_window_level_menu(const MenuItemDetails& __details) ;
    static void popup_window_adjust_level_menu(const MenuItemDetails& __details) ;
    //!@}

protected:
    SpectraDataType m_data_type ; //!< Data type of window.
    Prime::Data::Polyline polyline ;
    DataReader * m_reader = nullptr ; //!< Pointer to data reader.
    int m_active_data_point_index = -1 ; //!< Index of active analysis point.
    PickingPoint * m_active_picking_point = nullptr ; //!< Pointer to active analysis point.
    const PickingPoint * m_active_interpolated_picking_point = nullptr ; //!< Pointer to interpolated picking point at current data point.
    
    // Preparation resources table
    std::string res_palette_name ;   //!< Name of color palette.
    ToolkitColor res_picking_color ; //!< Color of picking curve.
    ToolkitColor res_interpolated_picking_color ; //!< Color of interpolated picking curve.
    ToolkitColor res_muting_color ; //!< Color of muting curve.
    std::unique_ptr< ResTable, std::function<void(ResTable*)> > m_resources ; //!< Resources of window.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_BASE_WINDOW_H */

