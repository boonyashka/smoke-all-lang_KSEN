/* 
 * File:   PickingImportDialog.h
 * Author: ranisimov
 *
 * Created on August 11, 2023, 1:20 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_PICKING_IMPORT_DIALOG_H
#define VERTICAL_SPECTRA_ANALYSIS_PICKING_IMPORT_DIALOG_H

#include <vector>
#include <string>

#include "toolkit/DialogCore.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

class PickingImportDialog
{
    /**
     * @brief Dialog elements descriptor.
     */
protected:

    enum DialogDescriptor
    {
        IMPORT_INPUT_BOX = 0,
        INPUT_PARAMETERS_BOX,
        COMPUTATION_PARAMETERS_BOX,

        // IMPORT_INPUT_BOX
        IMPORT_INPUT_ITEM = 0,

        // INPUT_PARAMETERS_BOX
        INPUT_NAME_ITEM = 0,
        INPUT_SELECT_ITEM,
        INPUT_ATTRIBUTE_NAME_ITEM,

        // COMPUTATION_PARAMETERS_BOX
        COMPUTATION_THREADS_NUM_ITEM = 0,
        COMPUTATION_PROGRESS_ITEM,

        // Dialog buttons
        DIALOG_BUTTON_CLOSE_X = -1,
        DIALOG_BUTTON_IMPORT,
        DIALOG_BUTTON_CANCEL
    } ;

    /**
     * @brief Types of import input.
     */
protected:

    enum InputType
    {
        BuilderModel3d = 0,
        Model3d,
        NumberOfInputTypes
    } ;

    static const char * NamesOfInputTypes[] ;

    /**
     * @brief Structure for import parameters description.
     */
private:

    struct Parameters
    {
        InputType input_type = NumberOfInputTypes ; //!< Type of input.
        std::string input_name ; //!< Name of input (model/cube)
        std::string input_attribute_name ; //!< Name of input attribute.
    } ;

public:
    PickingImportDialog( Application * __app, int __parent_dialog_id = -1 ) ;
    ~PickingImportDialog() ;

    /**
     * @brief Opens dialog and performs all required initializations.
     */
public:
    bool open( PickingStorage& __picking_storage ) ;

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
     * @brief Returns parent application.
     */
private:

    Application * getApp()
    {
        return m_app ;
    }

    /**
     * @brief Returns import parameters.
     */
private:
    bool getParameters( Parameters& __params, bool __check = false ) ;

    /**
     * @brief Returns number of processing threads.
     */
private:
    int getThreadsNum() const ;
    
    /**
     * @brief Setups name of input model/cube.
     */
private:
    void setInputName( const std::string& __name ) ;

    /**
     * @brief Updates dialog.
     */
private:
    void update() ;

    /**
     * @brief Imports picking.
     */
private:
    bool importPicking() ;

    /**
     * @brief Imports picking to model.
     */
private:
    bool importPickingFromModel( const Parameters& __params, int __threads_num ) ;

    /**
     * @brief Imports picking from cube.
     */
private:
    bool importPickingFromCube( const Parameters& __params, int __threads_num ) ;

    /**
     * @brief Makes GUI part of dialog.
     * @return Pointer to created dialog or NULL in bad case.
     */
private:
    DIALOG * make() ;

    /**
     * @name Dialog handlers
     * @{
     */
private:
    static int on_dialog_init( int __dialog ) ;
    static bool on_dialog_activate( int __dialog, int __button ) ;
    static void on_input_type_changed( int __dialog, int __box, int __item, int __button ) ;
    static void on_input_name_select( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state ) ;
    //!@}

private:
    DIALOG * m_dialog = nullptr ; //!< Pointer to dialog.
    Application * m_app = nullptr ;   //!< Parent application.
    PickingStorage * m_picking_storage = nullptr ; //!< Pointer to output storage.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_PICKING_IMPORT_DIALOG_H */

