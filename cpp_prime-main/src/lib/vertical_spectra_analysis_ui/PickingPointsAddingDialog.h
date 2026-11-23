/* 
 * File:   PickingPointsAddingDialog.h
 * Author: ranisimov
 *
 * Created on July 30, 2023, 2:34 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_PICKING_POINTS_ADDING_DIALOG_H
#define VERTICAL_SPECTRA_ANALYSIS_PICKING_POINTS_ADDING_DIALOG_H

#include <vector>
#include <string>

#include "toolkit/DialogCore.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class PickingPointsAddingDialog
{
    /**
     * @brief Dialog elements descriptor.
     */
protected:

    enum DialogDescriptor
    {
        POINTS_STEP_BOX = 0,
        FORALL_OBJECTS_BOX,

        // POINTS_STEP_BOX
        POINTS_STEP_ITEM = 0,
        
        // FORALL_OBJECTS_BOX
        FORALL_OBJECTS_ITEM = 0,

        // Dialog buttons
        DIALOG_BUTTON_CLOSE_X = -1,
        DIALOG_BUTTON_ADD,
        DIALOG_BUTTON_CLOSE
    } ;

public:
    PickingPointsAddingDialog( int __parent_dialog_id = -1 ) ;
    ~PickingPointsAddingDialog() ;
    
    /**
     * @brief Opens dialog and performs all required initializations.
     */
public:
    bool open(int& __points_step, bool& __add_for_all_objects) ;

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
     * @brief Reads and checks parameters for output.
     */
private:
    bool readAndCheckParameters() ;
    
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
    static void on_points_step_changed( int __dialog, int __box, int __item ) ;
    //!@}

private:
    DIALOG * m_dialog = 0 ;   //!< Pointer to dialog.
    int * m_points_step = 0 ; //!< Buffer for picking points step.
    bool * m_add_for_all_objects = 0 ; //!< Buffer for flag of adding points to all objects.
};

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_PICKING_POINTS_ADDING_DIALOG_H */

