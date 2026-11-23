
#include <sstream>
#include <stdexcept>

#include "toolkit/Appl.h"
#include "primeApp/PrimeAppNoX.h"
#include "utils/methodname.h"

#include "PickingPointsAddingDialog.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

PickingPointsAddingDialog::PickingPointsAddingDialog( int __parent_dialog_id ) :
m_dialog( 0 )
{
    define_methodname() ;

    m_dialog = make() ;

    if ( !m_dialog )
    {
        throw std::runtime_error( __methodname__ + ": Error. Failed to create or register dialog in toolkit." ) ;
    }

    DialogSetParent( m_dialog->id, __parent_dialog_id ) ;
    SetDialogData( m_dialog->id, "this", this ) ;
}

PickingPointsAddingDialog::~PickingPointsAddingDialog()
{
    if ( m_dialog )
    {
        DeleteDialogs( &m_dialog, 1 ) ;
        ReleaseDialog( m_dialog ) ;
    }
}

bool PickingPointsAddingDialog::open( int& __points_step, bool& __add_for_all_objects)
{
    define_methodname() ;

    if  ( getId() < 0 )
        return true ;

    m_points_step = &__points_step ;
    m_add_for_all_objects = &__add_for_all_objects ;
    
    return (OpenImmediateResponseDialog(getId(), on_dialog_activate) == DIALOG_BUTTON_ADD) ;
}

void PickingPointsAddingDialog::close()
{
    if  ( getId() < 0 )
        return ;

    m_points_step = 0 ;
    m_add_for_all_objects = 0 ;

    CloseDialog( getId() ) ;
}

int PickingPointsAddingDialog::getId() const
{
    if  ( !m_dialog )
        return -1 ;

    return m_dialog->id ;
}

bool PickingPointsAddingDialog::readAndCheckParameters()
{
    if ( getId() < 0 || !m_points_step || !m_add_for_all_objects )
        return false ;

    // Getting and checking input data
    *m_points_step = DialogGetTextValueAsInt( getId(), POINTS_STEP_BOX, POINTS_STEP_ITEM ) ;
    *m_add_for_all_objects = DialogGetToggleValue( getId(), FORALL_OBJECTS_BOX, FORALL_OBJECTS_ITEM ) ;

    return true ;
}

DIALOG * PickingPointsAddingDialog::make()
{
    define_methodname() ;

    //
    // POINTS_STEP_BOX
    //
    static TEXT_BOX points_step_box[] = {
        { "Points step (ncdp)", "40", true, 0, 32, 0, on_points_step_changed, uintTextVerificator, on_points_step_changed },
        { 0 }
    };

    //
    // FORALL_OBJECTS_BOX
    //
    static TOGGLE_BOX forall_objects_box[] = {
        { "Add picking points for all objects", false },
        { 0 }
    };

    //
    // Expander
    //
    static LABEL_BOX expander_box[] = {
        { "" },
        { 0 }
    };

    // 
    // Dialog assembling
    //
    static USER_DEFINED_DIALOG dialog_descrption[] = {
        { points_step_box,    0, TEXT_BOX_D,   true },
        { forall_objects_box, 0, TOGGLE_BOX_D, true },
        { expander_box,       0, LABEL_BOX_D,  true },
        { 0 }
    };

    static DIALOG::DIALOG_BUTTON dialog_buttons[] = {
        { "Ok",     true,  false },
        { "Cancel", false, false },
        { 0 }
    };

    static DIALOG dialog_prototype = {
        dialog_descrption, "Add picking points", -1,
        USER_DEFINED_D, MODAL, dialog_buttons, 0,
        0, 0, on_dialog_init
    };

    //
    // Clone dialog prom prototype
    //
    DIALOG * dialog = CopyDialog( &dialog_prototype ) ;

    if ( !AddDialogs(&dialog, 1) )
    {
        std::cerr << __methodname__ << ": Error. Failed to register dialog in toolkit." << std::endl ;
        ReleaseDialog( dialog ) ;
        return NULL ;
    }

    return dialog ;
}

int PickingPointsAddingDialog::on_dialog_init(int __dialog)
{
    PickingPointsAddingDialog * __this = static_cast<PickingPointsAddingDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !__this )
        return 0 ;

    return 1 ;
}

bool PickingPointsAddingDialog::on_dialog_activate(int __dialog, int __button)
{
    PickingPointsAddingDialog * __this = static_cast<PickingPointsAddingDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !__this )
        return false ;

    switch ( __button )
    {
        case DIALOG_BUTTON_ADD:
            return __this->readAndCheckParameters() ;

        case DIALOG_BUTTON_CLOSE_X:
        case DIALOG_BUTTON_CLOSE:
            return true ;
    }
    
    return false ;
}

void PickingPointsAddingDialog::on_points_step_changed( int __dialog, int __box, int __item )
{
    if (DialogGetTextValueAsInt(__dialog, __box, __item) <= 0)
        DialogSetTextValueAsInt(__dialog, __box, __item,  1) ;
}

}
}
}
