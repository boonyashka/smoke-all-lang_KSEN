/* 
 * File:   NavigationPanel.h
 * Author: ranisimov
 *
 * Created on July 16, 2023, 2:34 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_NAVIGATION_PANEL_H
#define VERTICAL_SPECTRA_ANALYSIS_NAVIGATION_PANEL_H

#include "toolkit/Appl.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

/**
 * @brief Class of navigation panel for 
 */
class NavigationPanel
{
    enum UIDescriptor
    {
        ANALYSIS_ICON_ITEM = 0,
        NAV_SCALE_ITEM,
        NAV_BUTTONS_ITEM,
        ANALYSIS_POINT_ITEM,
        ANALYSIS_STEP_ITEM,
        
        // NAV_BUTTONS_ITEM
        NAV_PREV_BTN = 0,
        NAV_NEXT_BTN,
        NAV_BIND_BTN
    } ;

    /**
     * @brief Constructs navigation panel.
     * @param[in] __app Parent application.
     * @param[in] __dialog Dialog of panel location.
     * @param[in] __box Box of panel location.
     */
public:
    NavigationPanel(Application * __app, int __dialog, int __box ) ;
    
public:
    ~NavigationPanel() ;
    
    /**
     * @brief Updates navigation panel.
     */
public:
    void update() ;
    
    /**
     * @brief Initializes panel.
     */
private:
    void init() ;
    
    /**
     * @brief Setups block of scale updating.
     */
private:
    void setScaleBlocking( bool __blocking )
    {
        m_scale_blocking = __blocking ;
    }
    
    /**
     * @brief Returns scale blocking flag.
     */
private:
    bool isScaleBlocked() const
    {
        return m_scale_blocking ;
    }
    
    /**
     * @brief Setups analysis point from scale widget.
     */
private:
    void setAnalysisPointFromScaleBox() ;
    
    /**
     * @brief Updates FFID from scaler widget.
     */
private:
    void setAnalysisPointFromTextBox() ;
    
    /**
     * @brief Shifts to next/prev analysis point.
     */
private:
    void shiftAnalysisPoint( int __direction ) ;
    
    /**
     * @brief Checks whether binding to picking points is required.
     */
private:
    bool usePickingPointsBinding() const ;
    
    /**
     * @brief Creates panel.
     * @return Pointer to UI box.
     */
public:
    static MULTI_BOX* make() ;

    /**
     * @brief Creates panel packing.
     * @return Pointer to UI packing.
     */
public:
    static DIALOG::BOX_PACKING* make_packing() ;
    
    /**
     * @name Widgets handlers.
     * @{
     */
private:
    static void on_nav_scale_changed(int __dialog, int __box, int __scale, double __value, int __decimal_point) ;
    static void on_nav_button_pressed(int __dialog, int __box, int __item, int __button, int __subbutton, bool __state) ;
    static void on_analysis_point_changed(int __dialog, int __box, int __button) ;
    static void on_analysis_step_changed(int __dialog, int __box, int __item, double __value) ;
    //!@}
    
private:
    Application * m_app ; //!< Parent application.
    int m_dialog ; //!< Dialog Id to which navigation panel is belonged.
    int m_box ;    //!< Box Id to which navigation panel is belonged.
    bool m_scale_blocking = false ; //!< Scale updating blocking.
};

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_NAVIGATION_PANEL_H */

