/* 
 * File:   ObjectsListPanel.h
 * Author: ranisimov
 *
 * Created on July 17, 2023, 3:28 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_OBJECTS_LIST_PANEL_H
#define VERTICAL_SPECTRA_ANALYSIS_OBJECTS_LIST_PANEL_H

#include "toolkit/Appl.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

/**
 * @brief Class of objects list panel.
 */
class ObjectsListPanel
{
    enum UIDescriptor
    {
        TABLE_ITEM = 0,
        
        // TABLE_ITEM
        TABLE_NAME_COLUMN = 0,
        TABLE_COLUMNS_NUM
    } ;
    
    /**
     * @brief Constructs navigation panel.
     * @param[in] __app Parent application.
     * @param[in] __dialog Dialog of panel location.
     * @param[in] __box Box of panel location.
     */
public:
    ObjectsListPanel(Application * __app, int __dialog, int __box, int __items_beg) ;

public:
    ~ObjectsListPanel() ;
    
    /**
     * @brief Updates navigation panel.
     */
public:
    void update() ;
    
    /**
     * @brief Hides/shows panel.
     */
public:
    void hide( bool __hide ) ;
    
    /**
     * @brief Checks whether panel is hidden.
     */
public:
    bool isHidden() const ;
    
    /**
     * @brief Loads required object for analysis.
     */
private:
    void loadAnalysisObject( int __object_index ) ;
    
    /**
     * @brief Creates panel.
     * @return Pointer to UI box.
     */
public:
    static CONTAINER_BOX* make() ;
    
    /**
     * @name Widgets handlers.
     * @{
     */
private:
    static void table_activate_proc(int __dialog, int __box, int __item, const std::vector<int>& __items) ;
    //!@}

private:
    Application * m_app ; //!< Parent application.
    int m_dialog ; //!< Dialog Id to which panel is belonged.
    int m_box ;    //!< Box Id to which panel is belonged.
    int m_items_beg ; //!< First item id to which panel is belonged.
    std::vector<CONTAINER_ITEM> m_storage; //!< Table storage.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_OBJECTS_LIST_PANEL_H */

