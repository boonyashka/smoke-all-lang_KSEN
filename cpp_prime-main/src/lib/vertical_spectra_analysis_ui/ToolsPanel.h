/* 
 * File:   ToolsPanel.h
 * Author: ranisimov
 *
 * Created on August 8, 2023, 7:21 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_TOOLS_PANEL_H
#define VERTICAL_SPECTRA_ANALYSIS_TOOLS_PANEL_H


#include "toolkit/Appl.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

/**
 * @brief Class of tools panel.
 */
class ToolsPanel
{

    enum UIDescriptor
    {
        TOOLS_ITEM = 0,
        
        // TOOLS_ITEM
        TOOLS_ZOOM_BTN = 0,
        TOOLS_ZOOM_UNDO_BTN,
        TOOLS_ZOOM_REDO_BTN,
        TOOLS_ZOOM_VIEWALL_BTN,
        TOOLS_OBJECTS_LIST_BTN
    } ;

    /**
     * @brief Constructs object.
     * @param[in] __app Parent application.
     * @param[in] __dialog Dialog of panel location.
     * @param[in] __box Box of panel location.
     * @param[in] __items_beg Index of first item.
     */
public:
    ToolsPanel(Application * __app, int __dialog, int __box, int __items_beg ) ;

public:
    ~ToolsPanel() ;

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
    static BUTTON_BOX* make() ;

    /**
     * @brief Creates panel packing.
     * @return Pointer to UI packing.
     */
public:
    //static DIALOG::BOX_PACKING* make_packing() ;

    /**
     * @name Widgets handlers.
     * @{
     */
private:
    static void buttons_proc(int __dialog, int __box, int __item, int __button, int __subbutton, bool __state) ;
    //!@}

private:
    Application * m_app ; //!< Parent application.
    int m_dialog ; //!< Dialog Id to which navigation panel is belonged.
    int m_box ;    //!< Box Id to which navigation panel is belonged.
    int m_items_beg ; //!< First item id to which navigation panel is belonged.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_TOOLS_PANEL_H */

