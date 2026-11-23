

#include <memory>
#include <sstream>
#include <stdexcept>

#include "toolkit/Appl.h"
#include "data/List.h"
#include "primeApp/PrimeAppNoX.h"
#include "utils/methodname.h"
#include "data3d/model.h"
#include "Horizons/FRHorizonsTypes.h"
#include "modelsel/modelsel3d.h"
#include "data3d/surface_math.h"
#include "mathlib/algorithms/sorting.h"

#include "Application.h"
#include "TasksExecutor.h"
#include "PickingImportTask.h"
#include "PickingImportDialog.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

const char * PickingImportDialog::NamesOfInputTypes[PickingImportDialog::NumberOfInputTypes + 1] = {
    "Opened model 3D",
    "Model 3D",
    0
};

PickingImportDialog::PickingImportDialog( Application * __app, int __parent_dialog_id ) :
m_dialog( 0 ),
m_app( __app )
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

PickingImportDialog::~PickingImportDialog()
{
    if ( m_dialog )
    {
        DeleteDialogs( &m_dialog, 1 ) ;
        ReleaseDialog( m_dialog ) ;
    }
}

bool PickingImportDialog::open( PickingStorage& __picking_storage )
{
    define_methodname() ;

    if  ( getId() < 0 || !getApp() )
        return true ;

    m_picking_storage = &__picking_storage ;

    if ( DialogGetTextValue(getId(), INPUT_PARAMETERS_BOX, INPUT_ATTRIBUTE_NAME_ITEM).empty() )
    {
        auto hodograph_type = getApp()->getDataStorage().getHodographType() ;
        const char * attribute_name = Prime::lib::Nmo::HodographType::get_attribute_name( hodograph_type ) ;
        DialogSetTextValue(getId(), INPUT_PARAMETERS_BOX, INPUT_ATTRIBUTE_NAME_ITEM, attribute_name) ;
    }

    update() ;

    return (OpenImmediateResponseDialog(getId(), on_dialog_activate) == DIALOG_BUTTON_IMPORT) ;
}

void PickingImportDialog::close()
{
    if  ( getId() < 0 )
        return ;

    m_picking_storage = nullptr ;

    CloseDialog( getId() ) ;
}

int PickingImportDialog::getId() const
{
    if  ( !m_dialog )
        return -1 ;

    return m_dialog->id ;
}

bool PickingImportDialog::getParameters( Parameters& __params, bool __check )
{
    if ( getId() < 0 )
        return false ;

    __params.input_type = InputType(DialogGetOptionValue(getId(), IMPORT_INPUT_BOX, IMPORT_INPUT_ITEM)) ;
    __params.input_name = DialogGetTextValue( getId(), INPUT_PARAMETERS_BOX, INPUT_NAME_ITEM ) ;
    __params.input_attribute_name = DialogGetTextValue( getId(), INPUT_PARAMETERS_BOX, INPUT_ATTRIBUTE_NAME_ITEM ) ;

    if ( !__check )
        return true ;

    if (__params.input_type == BuilderModel3d ||
        __params.input_type == Model3d)
    {
        if ( __params.input_attribute_name.empty() )
        {
            DisplayErrorDialog( "Error", "Input surface name is undefined", getId() ) ;
            return false ;
        }
    }

    if (__params.input_type == Model3d)
    {
        if ( __params.input_name.empty() )
        {
            DisplayErrorDialog( "Error", "Input name is undefined", getId() ) ;
            return false ;
        }
    }

    return true ;
}

int PickingImportDialog::getThreadsNum() const
{
    if ( getId() < 0 )
        return 1 ;

    int threads_num = 1 ;
    DialogGetStepperValueAsInt( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_THREADS_NUM_ITEM, &threads_num ) ;

    return std::max( threads_num, 1 ) ;
}

void PickingImportDialog::setInputName( const std::string& __name )
{
    if ( getId() < 0 )
        return ;

    DialogSetTextValue( getId(), INPUT_PARAMETERS_BOX, INPUT_NAME_ITEM, __name.c_str() ) ;
}

void PickingImportDialog::update()
{
    if ( getId() < 0 )
        return ;

    Parameters params ;

    if ( !getParameters(params) )
        return ;

    const bool is_model_input = (params.input_type == BuilderModel3d || params.input_type == Model3d) ;
    DialogSetItemLabel( getId(), INPUT_PARAMETERS_BOX, INPUT_NAME_ITEM, is_model_input ? "Input model " : "Input cube " ) ;
    DialogSetShowItem( getId(), INPUT_PARAMETERS_BOX, INPUT_NAME_ITEM, params.input_type != BuilderModel3d ) ;
    DialogSetShowItem( getId(), INPUT_PARAMETERS_BOX, INPUT_SELECT_ITEM, params.input_type != BuilderModel3d ) ;
    DialogSetShowItem( getId(), INPUT_PARAMETERS_BOX, INPUT_ATTRIBUTE_NAME_ITEM, is_model_input ) ;
    DialogSetShowItem( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM, false ) ;
}

bool PickingImportDialog::importPicking()
{
    if ( getId() < 0 || !getApp() || !m_picking_storage )
        return true ;

    // Getting parameters of export
    Parameters params ;

    if ( !getParameters(params, true) )
        return false ;

    const int threads_num = getThreadsNum() ;

    // Calling to appropriate export
    switch ( params.input_type )
    {
        case BuilderModel3d:
        case Model3d:
            if ( !importPickingFromModel(params, threads_num) )
                return false ;

            getApp()->update() ;
            return true ;
            
        default:
            return false ;
    }

    return true ;
}

bool PickingImportDialog::importPickingFromModel( const Parameters& __params, int __threads_num )
{
    using namespace Prime::lib::primeApp ;
    using namespace Prime::lib::mathlib ;

    if ( getId() < 0 || !getApp() || !m_picking_storage )
        return true ;

    if ( getApp()->getPickingStorage().empty() )
    {
        DisplayErrorDialog( "Error", "There is no any picking points.\nPlease, firstly define picking points.", getId() ) ;
        return false ;
    }

    // Getting domain type
    using namespace Prime::lib::Nmo ;
    auto hodograph_type = getApp()->getPickingStorage().getHodographType() ;
    bool is_time_domain = (HodographType::get_domain_type(hodograph_type) == FR_IO_TIME) ;
    std::string domain_name = HodographType::get_domain_name( hodograph_type ) ;

    // Checking for input model existence
    BasicModel3d * input_model3d = 0 ;

    if ( __params.input_type == BuilderModel3d )
    {
        auto * get_function = getApp()->getFunctionGetOpenedModel3d() ;
        input_model3d = (get_function ? get_function() : 0) ;
        
        if ( !input_model3d )
        {
            std::ostringstream message ;
            message << domain_name << " model is not opened." ;
            
            DisplayErrorDialog( "Error", message.str(), getId() ) ;
            return false ;
        }
    }
    else if ( __params.input_type == Model3d )
    {
        std::vector<std::string> models_list = (is_time_domain)
                ? Prime::Data::getTimeModels3d(getProjDB(), getApp()->getGridName())
                : Prime::Data::getDepthModels3d(getProjDB(), getApp()->getGridName()) ;

        if ( std::find(models_list.begin(), models_list.end(), __params.input_name) == models_list.end() )
        {
            std::ostringstream message ;
            message << "Input model '" << __params.input_name << "' doesn't exist." ;

            DisplayErrorDialog( "Error", message.str(), getId() ) ;
            return false ;
        }
    }

    // Running import task
    PickingImportFromModel3dTask task(*m_picking_storage,
                                      getApp()->getGridName(),
                                      input_model3d,
                                      __params.input_name,
                                      __params.input_attribute_name) ;

    DialogSetShowItem( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM, true ) ;

    getApp()->getTasksExecutor().bindProgressBar( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM ) ;
    bool result = getApp()->getTasksExecutor().runTask( task, __threads_num ) ;
    getApp()->getTasksExecutor().resetProgressBar() ;

    DialogSetShowItem( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM, false ) ;

    if ( !result && !task.getErrorDescription().empty() )
    {
        DisplayErrorDialog( "Error", task.getErrorDescription(), getId() ) ;
        return false ;
    }

    return true ;
}

bool PickingImportDialog::importPickingFromCube( const Parameters& __params, int __threads_num )
{
    if ( getId() < 0 || !m_picking_storage )
        return true ;

    return true ;
}

DIALOG * PickingImportDialog::make()
{
    define_methodname() ;

    //
    // IMPORT_INPUT_BOX
    //
    static OPTIONS_BOX import_input_box[] = {
        { "Import from", NamesOfInputTypes, on_input_type_changed },
        { 0 }
    };

    //
    // INPUT_PARAMETERS_BOX
    //
    static TEXT_BOX input_name_box[] = {
        { "Input model ", "", true, 0, 32 },
        { 0 }
    };

    static const char * input_select_label[] = {
        "open_16px.png",
        0
    };

    static BUTTON_BOX input_name_select_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 1, input_select_label, 0, 1, on_input_name_select,
    };

    static TEXT_BOX input_attribute_name_box[] = {
        { "Input attribute name ", "", true, 0, 32, 0, 0, 0, 0 },
        { 0 }
    };

    static MULTI_BOX input_parameters_mbox[] = {
        { input_name_box,           TEXT_BOX_D   },
        { &input_name_select_box,   BUTTON_BOX_D },
        { input_attribute_name_box, TEXT_BOX_D   },
        { 0 }
    } ;

    static PackingStyle name_and_select_packing_style[] = {
        PROPORTIONAL_BOX, FIXED_SIZE_BOX
    };

    DIALOG::PACKING::LINE input_parameters_mbox_packlines[] = {
        { 2, PROPORTIONAL_BOX, name_and_select_packing_style },
        { 1, PROPORTIONAL_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING input_parameters_mbox_packing = {
        HORIZONTAL_PACKING, input_parameters_mbox_packlines
    };

    //
    // COMPUTATION_PARAMETERS_BOX
    //
    static STEPPER_BOX computation_threads_num_box[] =  {
        { "Number of threads ", 2, 4, 1, 96, 1, 0 },
        { 0 }
    };

    static PROGRESS_BOX computation_progress_box = {
        PROGRESS_BOX::PERCENTAGE, ""
    };

    static MULTI_BOX computation_parameters_mbox[] = {
        { computation_threads_num_box, STEPPER_BOX_D  },
        { &computation_progress_box,   PROGRESS_BOX_D },
        { 0 }
    } ;

    //
    // Expander
    //
    static LABEL_BOX expander_box[] = {
        { "" },
        { 0 }
    };

    // 

    // 
    // Dialog assembling
    //
    static USER_DEFINED_DIALOG dialog_descrption[] = {
        { import_input_box,           0,                  OPTIONS_BOX_D, true },
        { input_parameters_mbox,      "Input parameters", MULTI_BOX_D, false, &input_parameters_mbox_packing },
        { computation_parameters_mbox, "Computation",     MULTI_BOX_D, false },
        { expander_box,                0,                 LABEL_BOX_D, true },
        { 0 }
    };

    static DIALOG::DIALOG_BUTTON dialog_buttons[] = {
        { "Import", true,  false },
        { "Cancel", false, false },
        { 0 }
    };

    static DIALOG dialog_prototype = {
        dialog_descrption, "Import", -1,
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

int PickingImportDialog::on_dialog_init(int __dialog)
{
    PickingImportDialog * _this = static_cast<PickingImportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return 0 ;

    return 1 ;
}

bool PickingImportDialog::on_dialog_activate(int __dialog, int __button)
{
    PickingImportDialog * _this = static_cast<PickingImportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return false ;

    switch ( __button )
    {

        case DIALOG_BUTTON_IMPORT:
            return _this->importPicking() ;

        case DIALOG_BUTTON_CLOSE_X:
        case DIALOG_BUTTON_CANCEL:
            return true ;
    }
    
    return false ;
}

void PickingImportDialog::on_input_type_changed( int __dialog, int __box, int __item, int __button )
{
    PickingImportDialog * _this = static_cast<PickingImportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return ;

    _this->update() ;
}

void PickingImportDialog::on_input_name_select( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state )
{
    using namespace Prime::lib::primeApp ;
    using namespace Prime::lib::Nmo ;

    PickingImportDialog * _this = static_cast<PickingImportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this || !_this->getApp() )
        return ;

    Parameters params ;
    if ( !_this->getParameters(params) )
        return ;

    if ( params.input_type == Model3d )
    {
        auto hodograph_type = _this->getApp()->getPickingStorage().getHodographType() ;
        auto hodograph_domain_type = HodographType::get_domain_type(hodograph_type) ;

        ModelSel3d model_selector( getProjDB() ) ;
        ModelSel3d::ModelType domain = (hodograph_domain_type == FR_IO_TIME ? ModelSel3d::TIME : ModelSel3d::DEPTH ) ;

        std::string model_name = params.input_name ;
        const std::string title = "Select input model" ;
        model_selector.getExistingModelName( domain, _this->getApp()->getGridName(), title, model_name, __dialog ) ;
        _this->setInputName( model_name ) ;
    }
    //    else if ( params.input_type == Cube )
    //    {
    //        ///
    //    }
}

}
}
}
