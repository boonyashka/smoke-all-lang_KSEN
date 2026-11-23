
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
#include "mathlib/algorithms/interpolation.h"

#include "Application.h"
#include "TasksExecutor.h"
#include "PickingExportDialog.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

const char * PickingExportDialog::NamesOfOutputTypes[PickingExportDialog::NumberOfOutputTypes + 1] = {
    "Opened model 3D",
    "Model 3D",
    0
};

PickingExportDialog::PickingExportDialog( Application * __app, int __parent_dialog_id ) :
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

PickingExportDialog::~PickingExportDialog()
{
    if ( m_dialog )
    {
        DeleteDialogs( &m_dialog, 1 ) ;
        ReleaseDialog( m_dialog ) ;
    }
}

bool PickingExportDialog::open()
{
    define_methodname() ;

    if  ( getId() < 0 || !getApp() )
        return true ;

    if ( DialogGetTextValue(getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_ATTRIBUTE_NAME_ITEM).empty() )
    {
        auto hodograph_type = getApp()->getDataStorage().getHodographType() ;
        const char * attribute_name = Prime::lib::Nmo::HodographType::get_attribute_name( hodograph_type ) ;
        DialogSetTextValue(getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_ATTRIBUTE_NAME_ITEM, attribute_name) ;
    }

    update() ;

    return (OpenImmediateResponseDialog(getId(), on_dialog_activate) == DIALOG_BUTTON_EXPORT) ;
}

void PickingExportDialog::close()
{
    if  ( getId() < 0 )
        return ;

    CloseDialog( getId() ) ;
}

int PickingExportDialog::getId() const
{
    if  ( !m_dialog )
        return -1 ;

    return m_dialog->id ;
}

bool PickingExportDialog::getParameters( Parameters& __params, bool __check )
{
    if ( getId() < 0 )
        return false ;

    __params.output_type = OutputType(DialogGetOptionValue(getId(), EXPORT_OUTPUT_BOX, EXPORT_OUTPUT_ITEM)) ;
    __params.model_frame_type = ModelFrameType(DialogGetOptionValue(getId(), EXPORT_PARAMETERS_BOX, MODEL_FRAME_ITEM)) ;
    __params.horizons_step = DialogGetTextValueAsDouble( getId(), EXPORT_PARAMETERS_BOX, HORIZONS_STEP_ITEM ) ;
    __params.frame_model_name = DialogGetTextValue( getId(), EXPORT_PARAMETERS_BOX, FRAME_MODEL_NAME_ITEM ) ;
    __params.use_topo_horizon = DialogGetToggleValue(getId(), EXPORT_PARAMETERS_BOX, USE_MODEL_TOPO_ITEM) ;
    __params.interpolation_radius = DialogGetTextValueAsDouble( getId(), EXPORT_PARAMETERS_BOX, INTERPOLATION_RADIUS_ITEM ) ;
    __params.smoothness_radius = DialogGetTextValueAsDouble( getId(), EXPORT_PARAMETERS_BOX, SMOOTHNESS_RADIUS_ITEM ) ;
    __params.output_name = DialogGetTextValue( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_NAME_ITEM ) ;
    __params.output_attribute_name = DialogGetTextValue( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_ATTRIBUTE_NAME_ITEM ) ;
    __params.output_step_x = DialogGetTextValueAsDouble( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_STEP_X_ITEM ) ;
    __params.output_step_y = DialogGetTextValueAsDouble( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_STEP_Y_ITEM ) ;

    if ( !__check )
        return true ;

    if (__params.output_type == BuilderModel3d ||
        __params.output_type == Model3d)
    {
        if (__params.model_frame_type == ModelFrameType::ConstantStepHorizons &&
            __params.horizons_step <= 0 )
        {
            DisplayErrorDialog( "Error", "Horizons step is invalid.", getId() ) ;
            return false ;
        }

        if (__params.model_frame_type == ModelFrameType::ModelHorizons &&
            __params.frame_model_name.empty() )
        {
            DisplayErrorDialog( "Error", "Name of frame model is undefined.", getId() ) ;
            return false ;
        }

        if ( __params.output_attribute_name.empty() )
        {
            DisplayErrorDialog( "Error", "Output surface name is undefined", getId() ) ;
            return false ;
        }
    }

    if ( __params.interpolation_radius <= 0 )
    {
        DisplayErrorDialog( "Error", "Interpolation radius is invalid.", getId() ) ;
        return false ;
    }

    if ( __params.smoothness_radius <= 0 )
    {
        DisplayErrorDialog( "Error", "Smoothness radius is invalid.", getId() ) ;
        return false ;
    }

    if (__params.output_type == Model3d)
    {
        if ( __params.output_name.empty() )
        {
            DisplayErrorDialog( "Error", "Output name is undefined", getId() ) ;
            return false ;
        }
    }

    return true ;
}

int PickingExportDialog::getThreadsNum() const
{
    if ( getId() < 0 )
        return 1 ;

    int threads_num = 1 ;
    DialogGetStepperValueAsInt( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_THREADS_NUM_ITEM, &threads_num ) ;

    return std::max( threads_num, 1 ) ;
}

void PickingExportDialog::setFrameModelName( const std::string& __name )
{
    if ( getId() < 0 )
        return ;

    DialogSetTextValue( getId(), EXPORT_PARAMETERS_BOX, FRAME_MODEL_NAME_ITEM, __name.c_str() ) ;
}

void PickingExportDialog::setOutputName( const std::string& __name )
{
    if ( getId() < 0 )
        return ;

    DialogSetTextValue( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_NAME_ITEM, __name.c_str() ) ;
}

void PickingExportDialog::update()
{
    if ( getId() < 0 )
        return ;

    Parameters params ;

    if ( !getParameters(params) )
        return ;
    
    using namespace Prime::lib::Nmo ;
    auto hodograph_type = getApp()->getPickingStorage().getHodographType() ;
    const bool is_depth = (HodographType::get_domain_type(hodograph_type) == FR_IO_DEPTH) ;
    const bool is_model_output = (params.output_type == BuilderModel3d || params.output_type == Model3d) ;

    DialogSetShowItem( getId(), EXPORT_PARAMETERS_BOX, MODEL_FRAME_ITEM, is_model_output ) ;
    DialogSetShowItem( getId(), EXPORT_PARAMETERS_BOX, HORIZONS_STEP_ITEM, is_model_output && params.model_frame_type == ModelFrameType::ConstantStepHorizons ) ;
    DialogSetShowItem( getId(), EXPORT_PARAMETERS_BOX, FRAME_MODEL_NAME_ITEM, is_model_output && params.model_frame_type == ModelFrameType::ModelHorizons ) ;
    DialogSetShowItem( getId(), EXPORT_PARAMETERS_BOX, FRAME_MODEL_SELECT_ITEM, is_model_output && params.model_frame_type == ModelFrameType::ModelHorizons ) ;
    DialogSetShowItem( getId(), EXPORT_PARAMETERS_BOX, USE_MODEL_TOPO_ITEM, is_depth && is_model_output && params.model_frame_type == ModelFrameType::ModelHorizons ) ;

    DialogSetItemLabel( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_NAME_ITEM, is_model_output ? "Output model " : "Output cube " ) ;
    DialogSetShowItem( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_NAME_ITEM, params.output_type != BuilderModel3d ) ;
    DialogSetShowItem( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_SELECT_ITEM, params.output_type != BuilderModel3d ) ;
    DialogSetShowItem( getId(), OUTPUT_PARAMETERS_BOX, OUTPUT_ATTRIBUTE_NAME_ITEM, is_model_output ) ;

    DialogSetShowItem( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM, false ) ;
}

bool PickingExportDialog::exportPicking()
{
    if ( getId() < 0 )
        return true ;

    // Getting parameters of export
    Parameters params ;

    if ( !getParameters(params, true) )
        return false ;

    const int threads_num = getThreadsNum() ;

    // Calling to appropriate export
    switch ( params.output_type )
    {
        case BuilderModel3d:
        case Model3d:
            return exportPickingToModel( params, threads_num ) ;

//        case Cube:
//            return exportPickingToCube( params, threads_num ) ;
            
        default:
            return false ;
    }

    return true ;
}

bool PickingExportDialog::exportPickingToModel( const Parameters& __params, int __threads_num )
{
    using namespace Prime::lib::primeApp ;
    using namespace Prime::lib::mathlib ;

    if ( getId() < 0 || !getApp() )
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
    
    // Checking for output model existence
    BasicModel3d * output_model3d = 0 ;

    if ( __params.output_type == BuilderModel3d )
    {
        auto * get_function = getApp()->getFunctionGetOpenedModel3d() ;
        output_model3d = (get_function ? get_function() : 0) ;
        
        if ( !output_model3d )
        {
            std::ostringstream message ;
            message << domain_name << " model is not opened." ;
            
            DisplayErrorDialog( "Error", message.str(), getId() ) ;
            return false ;
        }
        
        if ( !output_model3d->empty() )
        {
            std::ostringstream message ;
            message << "Output model '" << output_model3d->getName() << "' is not empty." << std::endl
                    << "Do you want to overwrite it?" ;

            if ( DisplayYesNoQuestionDialog("Overwriting", message.str(), getId()) != 0 )
                return false ;
        }
    }
    else if ( __params.output_type == Model3d )
    {
        std::vector<std::string> models_list = (is_time_domain)
                ? Prime::Data::getTimeModels3d(getProjDB(), getApp()->getGridName())
                : Prime::Data::getDepthModels3d(getProjDB(), getApp()->getGridName()) ;

        if ( std::find(models_list.begin(), models_list.end(), __params.output_name) != models_list.end() )
        {
            std::ostringstream message ;
            message << "Output model '" << __params.output_name << "' exists already." << std::endl
                    << "Do you want to overwrite it?" ;

            if ( DisplayYesNoQuestionDialog("Overwriting", message.str(), getId()) != 0 )
                return false ;
        }
    }
    
    // Running export task
    PickingExportToModel3dTask task(getApp()->getPickingStorage(),
                                    getApp()->getDataStorage(),
                                    getApp()->getGridName(),
                                    __params.model_frame_type,
                                    __params.horizons_step,
                                    __params.frame_model_name,
                                    __params.use_topo_horizon,
                                    __params.interpolation_radius,
                                    __params.smoothness_radius,
                                    output_model3d,
                                    __params.output_name,
                                    __params.output_attribute_name,
                                    __params.output_step_x,
                                    __params.output_step_y) ;

    DialogSetShowItem( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM, true ) ;

    getApp()->getTasksExecutor().bindProgressBar( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM ) ;
    bool result = getApp()->getTasksExecutor().runTask( task, __threads_num ) ;
    getApp()->getTasksExecutor().resetProgressBar() ;

    DialogSetShowItem( getId(), COMPUTATION_PARAMETERS_BOX, COMPUTATION_PROGRESS_ITEM, false ) ;
    
    if (__params.output_type == BuilderModel3d &&
        getApp()->getFunctionUpdateOpenedModel3d())
    {
        getApp()->getFunctionUpdateOpenedModel3d()( output_model3d ) ;
    }

    if ( !result && !task.getErrorDescription().empty() )
    {
        DisplayErrorDialog( "Error", task.getErrorDescription(), getId() ) ;
        return false ;
    }

    return true ;
}

bool PickingExportDialog::exportPickingToCube( const Parameters& __params, int __threads_num )
{
    if ( getId() < 0 )
        return true ;

    return true ;
}

DIALOG * PickingExportDialog::make()
{
    define_methodname() ;

    //
    // EXPORT_TARGET_BOX
    //
    static OPTIONS_BOX export_target_box[] = {
        { "Export to", NamesOfOutputTypes, on_export_target_changed },
        { 0 }
    };

    //
    // EXPORT_PARAMETERS_BOX
    //
    static OPTIONS_BOX model_frame_box[] = {
        { "Model frame", PickingExportToModel3dTask::NamesOfModelFrameTypes, on_model_frame_changed },
        { 0 }
    };

    static TEXT_BOX horizons_step_box[] = {
        { "Horizons step (msec | m) ", "150", true, 0, 32, 0, on_horizons_step_changed, uintTextVerificator, on_horizons_step_changed },
        { 0 }
    };

    static TEXT_BOX frame_model_name_box[] = {
        { "Frame model ", "", true, 0, 32 },
        { 0 }
    };

    static const char * frame_model_select_label[] = {
        "open_16px.png",
        0
    };

    static BUTTON_BOX frame_model_select_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 1, frame_model_select_label, 0, 1, on_frame_model_select,
    };

    static TOGGLE_BOX use_topography_datum_horizon_box[] = {
        { "Use first horizon as topography datum", 0, 0 },
        { 0 }
    };

    static TEXT_BOX interpolation_radius_box[] = {
        { "Interpolation radius (m)", "2000", true, 0, 32, 0, 0, ufloatTextVerificator, 0 },
        { 0 }
    };

    static TEXT_BOX smoothness_radius_box[] = {
        { "Smoothness radius (m)", "500", true, 0, 32, 0, 0, ufloatTextVerificator, 0 },
        { 0 }
    };

    static MULTI_BOX export_parameters_mbox[] = {
        { model_frame_box,                  OPTIONS_BOX_D },
        { horizons_step_box,                TEXT_BOX_D    },
        { frame_model_name_box,             TEXT_BOX_D    },
        { &frame_model_select_box,          BUTTON_BOX_D  },
        { use_topography_datum_horizon_box, TOGGLE_BOX_D  },
        { interpolation_radius_box,         TEXT_BOX_D    },
        { smoothness_radius_box,            TEXT_BOX_D    },
        { 0 }
    } ;

    static PackingStyle name_and_select_packing_style[] = {
        PROPORTIONAL_BOX, FIXED_SIZE_BOX
    };

    DIALOG::PACKING::LINE export_parameters_mbox_packlines[] = {
        { 1, PROPORTIONAL_BOX },
        { 1, PROPORTIONAL_BOX },
        { 2, PROPORTIONAL_BOX, name_and_select_packing_style },
        { 1, PROPORTIONAL_BOX },
        { 1, PROPORTIONAL_BOX },
        { 1, PROPORTIONAL_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING export_parameters_mbox_packing = {
        HORIZONTAL_PACKING, export_parameters_mbox_packlines
    };

    //
    // OUTPUT_PARAMETERS_BOX
    //
    static TEXT_BOX output_name_box[] = {
        { "Output model ", "", true, 0, 32 },
        { 0 }
    };

    static const char * output_select_label[] = {
        "open_16px.png",
        0
    };

    static BUTTON_BOX output_name_select_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 1, output_select_label, 0, 1, on_output_name_select,
    };

    static TEXT_BOX output_attribute_name_box[] = {
        { "Output attribute name ", "", true, 0, 32, 0, 0, 0, 0 },
        { 0 }
    };

    static TEXT_BOX output_stepX_box[] = {
        { "Output step X (m)", "100", true, 0, 32, 0, 0, ufloatTextVerificator, 0 },
        { 0 }
    };

    static TEXT_BOX output_stepY_box[] = {
        { "Output step Y (m)", "100", true, 0, 32, 0, 0, ufloatTextVerificator, 0 },
        { 0 }
    };

    static MULTI_BOX output_parameters_mbox[] = {
        { output_name_box,           TEXT_BOX_D   },
        { &output_name_select_box,   BUTTON_BOX_D },
        { output_attribute_name_box, TEXT_BOX_D   },
        { output_stepX_box,          TEXT_BOX_D   },
        { output_stepY_box,          TEXT_BOX_D   },
        { 0 }
    } ;

    DIALOG::PACKING::LINE output_parameters_mbox_packlines[] = {
        { 2, PROPORTIONAL_BOX, name_and_select_packing_style },
        { 1, PROPORTIONAL_BOX },
        { 1, PROPORTIONAL_BOX },
        { 1, PROPORTIONAL_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING output_parameters_mbox_packing = {
        HORIZONTAL_PACKING, output_parameters_mbox_packlines
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
        { export_target_box,           0,                   OPTIONS_BOX_D, true },
        { export_parameters_mbox,      "Export parameters", MULTI_BOX_D, false, &export_parameters_mbox_packing },
        { output_parameters_mbox,      "Output parameters", MULTI_BOX_D, false, &output_parameters_mbox_packing },
        { computation_parameters_mbox, "Computation",       MULTI_BOX_D, false },
        { expander_box,                0,                   LABEL_BOX_D, true },
        { 0 }
    };

    static DIALOG::DIALOG_BUTTON dialog_buttons[] = {
        { "Export", true,  false },
        { "Cancel", false, false },
        { 0 }
    };

    static DIALOG dialog_prototype = {
        dialog_descrption, "Export", -1,
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

int PickingExportDialog::on_dialog_init(int __dialog)
{
    PickingExportDialog * _this = static_cast<PickingExportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return 0 ;

    return 1 ;
}

bool PickingExportDialog::on_dialog_activate(int __dialog, int __button)
{
    PickingExportDialog * _this = static_cast<PickingExportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return false ;

    switch ( __button )
    {

        case DIALOG_BUTTON_EXPORT:
            return _this->exportPicking() ;

        case DIALOG_BUTTON_CLOSE_X:
        case DIALOG_BUTTON_CANCEL:
            return true ;
    }
    
    return false ;
}

void PickingExportDialog::on_export_target_changed( int __dialog, int __box, int __item, int __button )
{
    PickingExportDialog * _this = static_cast<PickingExportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return ;

    _this->update() ;
}

void PickingExportDialog::on_model_frame_changed( int __dialog, int __box, int __item, int __button )
{
    PickingExportDialog * _this = static_cast<PickingExportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return ;

    _this->update() ;
}

void PickingExportDialog::on_horizons_step_changed( int __dialog, int __box, int __item )
{
    if (DialogGetTextValueAsInt(__dialog, __box, __item) <= 25)
        DialogSetTextValueAsInt(__dialog, __box, __item, 25) ;
}

void PickingExportDialog::on_frame_model_select( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state )
{
    using namespace Prime::lib::primeApp ;
    using namespace Prime::lib::Nmo ;

    PickingExportDialog * _this = static_cast<PickingExportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this || !_this->getApp() )
        return ;

    Parameters params ;
    if ( !_this->getParameters(params) )
        return ;

    auto hodograph_type = _this->getApp()->getPickingStorage().getHodographType() ;
    auto hodograph_domain_type = HodographType::get_domain_type(hodograph_type) ;

    ModelSel3d model_selector( getProjDB() ) ;
    ModelSel3d::ModelType domain = (hodograph_domain_type == FR_IO_TIME ? ModelSel3d::TIME : ModelSel3d::DEPTH ) ;

    bool is_new = false ;
    std::string model_name = params.frame_model_name ;
    const std::string title = "Select frame model" ;
    model_selector.getModelName( domain, _this->getApp()->getGridName(), title, model_name, is_new, __dialog ) ;
    _this->setFrameModelName( model_name ) ;
}

void PickingExportDialog::on_output_name_select( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state )
{
    using namespace Prime::lib::primeApp ;
    using namespace Prime::lib::Nmo ;

    PickingExportDialog * _this = static_cast<PickingExportDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this || !_this->getApp() )
        return ;

    Parameters params ;
    if ( !_this->getParameters(params) )
        return ;

    if ( params.output_type == Model3d )
    {
        auto hodograph_type = _this->getApp()->getPickingStorage().getHodographType() ;
        auto hodograph_domain_type = HodographType::get_domain_type(hodograph_type) ;

        ModelSel3d model_selector( getProjDB() ) ;
        ModelSel3d::ModelType domain = (hodograph_domain_type == FR_IO_TIME ? ModelSel3d::TIME : ModelSel3d::DEPTH ) ;

        bool is_new = false ;
        std::string model_name = params.output_name ;
        const std::string title = "Select output model" ;
        model_selector.getModelName( domain, _this->getApp()->getGridName(), title, model_name, is_new, __dialog ) ;
        _this->setOutputName( model_name ) ;
    }
//    else if ( params.output_type == Cube )
//    {
//        ///
//    }
}

}
}
}
