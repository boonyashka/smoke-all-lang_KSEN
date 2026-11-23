/* 
 * File:   data_selection_dialog.h
 * Author: ranisimov
 *
 * Created on July 13, 2023, 2:27 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_DATA_SELECTION_DIALOG_H
#define VERTICAL_SPECTRA_ANALYSIS_DATA_SELECTION_DIALOG_H

#include <vector>
#include <string>

#include "toolkit/DialogCore.h"
#include "files/Names.h"
#include "data3d/model.h"
#include "vertical_spectra_analysis/PickingStorage.h"

#include "DataStorage.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

class DataOpenDialog
{
    /**
     * @brief Dialog elements descriptor.
     */
protected:

    enum DialogDescriptor
    {
        PICKING_NAME_BOX = 0,
        GRID_NAME_BOX,
        VERSION_NAME_BOX,
        VERSION_INFO_BOX,

        // PICKING_NAME_BOX
        PICKING_NAME_ITEM = 0,

        // GRID_NAME_BOX
        GRID_NAME_ITEM = 0,

        // VERSION_NAME_BOX
        VERSION_NAME_ITEM = 0,
        VERSION_NAME_SELECT_BTN_ITEM,
        VERSION_NAME_SELECT_BTN = 0,

        // VERSION_INFO_BOX
        VERSION_INFO_ITEM = 0,

        // Dialog buttons
        DIALOG_BUTTON_CLOSE_X = -1,
        DIALOG_BUTTON_OK,
        DIALOG_BUTTON_CLOSE
    } ;

public:
    DataOpenDialog() ;
    ~DataOpenDialog() ;
    
    /**
     * @brief Returns parent application.
     */
private:

    Application * getApp()
    {
        return m_app ;
    }

    /**
     * @brief Opens dialog and performs all required initializations.
     * @return True if data was selected correctly.
     */
public:
    bool open(Application * __app,
              DataStorage& __data_storage,
              PickingStorage& __picking_storage,
              std::string& __grid_name,
              std::string& __picking_name) ;

    /**
     * @brief Closes dialog and performs all required de-initializations.
     */
public:
    void close() ;

    /**
     * @brief Returns dialog identifier.
     */
public:
    int getId() const ;

    /**
     * @brief Updates dialog, loads pickings.
     */
public:
    void update() ;

    /**
     * @brief Reads and checks parameters for output.
     */
private:
    bool readAndCheckParameters() ;
    
    /**
     * @brief Returns current selected grid name.
     */
private:
    std::string getGridName() const ;
    
    /**
     * @brief Returns current selected version name.
     */
private:
    std::string getVersionName() const ;
    
    /**
     * @brief Setups current selected version name.
     */
private:
    void setVersionName(const std::string& __version_name) ;

    /**
     * @brief Loads data.
     */
private:
    bool loadData() ;

    /**
     * @brief Makes GUI part of dialog.
     * @return Pointer to created dialog or NULL in bad case.
     */
private:
    DIALOG * make() ;

    /**
     * @brief Returns and caches list of grids.
     */
private:
    const std::vector<std::string>& getGridsList() ;

    /**
     * @brief Checks whether selected version contains required index.
     */
private:
    bool hasVersionRequiredIndex(const std::string& __grid_name,
                                 const std::string& __version_name,
                                 std::string& __message) const ;

    /**
     * @name Dialog handlers
     * @{
     */
private:
    static int on_dialog_init( int __dialog ) ;
    static bool on_dialog_activate( int __dialog, int __button ) ;
    static void on_combobox_changed( int __dialog, int __box, int __item, const char * __sel_str, int __sel_item ) ;
    static void on_version_name_changed( int __dialog, int __box, int __item ) ;
    static void on_version_selection_button( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state ) ;
    //!@}

private:
    DIALOG * m_dialog = nullptr ;                     //!< Pointer to dialog.
    Application * m_app = nullptr ;                   //!< Parent application.
    Prime::Files::Names m_names ;                     //!< Names service.
    std::vector<std::string> m_grids_list ;           //!< List of grids names (cache).
    DataStorage * m_data_storage = nullptr ;          //!< Pointer to data storage which has to be loaded.
    PickingStorage * m_picking_storage = nullptr ;    //!< Pointer to picking storage which has to be loaded.
    std::string * m_grid_name = nullptr ;             //!< Input/output name of grid.
    std::string * m_picking_name = nullptr ;          //!< Output buffer for picking name.
} ;

}
}
}


#endif /* VERTICAL_SPECTRA_ANALYSIS_DATA_SELECTION_DIALOG_H */

