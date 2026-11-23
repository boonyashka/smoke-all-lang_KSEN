/* 
 * File:   TopMenuPanel.h
 * Author: ranisimov
 *
 * Created on July 17, 2023, 2:14 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_TOP_MENU_PANEL_H
#define VERTICAL_SPECTRA_ANALYSIS_TOP_MENU_PANEL_H

#include "toolkit/Appl.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

/**
 * @brief Class of top menu.
 */
class TopMenuPanel
{
    /**
     * @brief Constructs object.
     * @param[in] __app Parent application.
     * @param[in] __dialog Dialog of panel location.
     */
public:
    TopMenuPanel(Application * __app, int __dialog) ;

public:
    ~TopMenuPanel() ;

    /**
     * @brief Returns parent application.
     */
private:

    Application * getApp()
    {
        return m_app ;
    }

    /**
     * @brief Creates panel.
     * @return Pointer to UI box.
     */
public:
    static MENU* make() ;

    /**
     * @name Widgets handlers.
     * @{
     */
private:
    static void picking_new_menu(const MenuItemDetails& __details) ;
    static void picking_load_menu(const MenuItemDetails& __details) ;
    static void picking_save_menu(const MenuItemDetails& __details) ;
    static void picking_save_menu_map(const MenuItemDetails& __details) ;
    static void picking_save_as_menu(const MenuItemDetails& __details) ;
    static void picking_import_menu(const MenuItemDetails& __details) ;
    static void picking_import_menu_map(const MenuItemDetails& __details) ;
    static void picking_export_menu(const MenuItemDetails& __details) ;
    static void picking_export_menu_map(const MenuItemDetails& __details) ;
    static void picking_autosave_menu(const MenuItemDetails& __details) ;
    static void picking_autosave_menu_map(const MenuItemDetails& __details) ;
    static void picking_add_picking_menu(const MenuItemDetails& __details) ;
    static void picking_add_picking_menu_map(const MenuItemDetails& __details) ;
    static void picking_add_picking_points_menu(const MenuItemDetails& __details) ;
    static void picking_del_picking_menu(const MenuItemDetails& __details) ;
    static void picking_del_picking_menu_map(const MenuItemDetails& __details) ;
    
    static void view_levels_menu(const MenuItemDetails& __details) ;
    static void view_preferences_menu(const MenuItemDetails& __details) ;
    static void cursor_tracking_menu(const MenuItemDetails& __details) ;
    static void cursor_tracking_menu_map(const MenuItemDetails& __details) ;
    //!@}

private:
    Application * m_app ; //!< Parent application.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_TOP_MENU_PANEL_H */

