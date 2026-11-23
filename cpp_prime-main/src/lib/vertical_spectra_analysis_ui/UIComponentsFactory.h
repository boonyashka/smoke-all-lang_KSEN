/* 
 * File:   ui_factory.h
 * Author: ranisimov
 *
 * Created on July 13, 2023, 5:12 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_UI_COMPONENTS_FACTORY_H
#define VERTICAL_SPECTRA_ANALYSIS_UI_COMPONENTS_FACTORY_H

#include "toolkit/Appl.h"
#include "toolkit/DialogCore.h"
#include "seismicWindow/SeismicWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of Vertical Spectra Analysis Tool UI elements.
 */
class UIComponentsFactory
{
    /**
     * @name Components prototypes.
     * @{ 
     */
public:
    static MENU * get_main_menu_prototype() ;
    static MULTI_BOX * get_navigation_panel_prototype() ;
    static DIALOG::BOX_PACKING* get_navigation_panel_packing() ;
    static BUTTON_BOX * get_tools_panel_prototype() ;
    static CONTAINER_BOX * get_objects_list_prototype() ;
    static WINDOW_BOX * get_gather_window_prototype() ;
    static WINDOW_BOX * get_spectrum_window_prototype() ;
    static WINDOW_BOX * get_stackgather_window_prototype() ;
    //!@}
};

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_UI_COMPONENTS_FACTORY_H */

