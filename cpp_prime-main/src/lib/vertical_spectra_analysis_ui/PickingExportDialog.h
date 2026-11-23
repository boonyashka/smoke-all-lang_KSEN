/* 
 * File:   PickingExportDialog.h
 * Author: ranisimov
 *
 * Created on July 31, 2023, 3:01 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_PICKING_EXPORT_DIALOG_H
#define VERTICAL_SPECTRA_ANALYSIS_PICKING_EXPORT_DIALOG_H

#include <vector>
#include <string>

#include "toolkit/DialogCore.h"

#include "PickingExportTask.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class Application ;

class PickingExportDialog
{
    /**
     * @brief Dialog elements descriptor.
     */
protected:

    enum DialogDescriptor
    {
        EXPORT_OUTPUT_BOX = 0,
        EXPORT_PARAMETERS_BOX,
        OUTPUT_PARAMETERS_BOX,
        COMPUTATION_PARAMETERS_BOX,

        // EXPORT_OUTPUT_BOX
        EXPORT_OUTPUT_ITEM = 0,

        // EXPORT_PARAMETERS_BOX
        MODEL_FRAME_ITEM = 0,
        HORIZONS_STEP_ITEM,
        FRAME_MODEL_NAME_ITEM,
        FRAME_MODEL_SELECT_ITEM,
        USE_MODEL_TOPO_ITEM,
        INTERPOLATION_RADIUS_ITEM,
        SMOOTHNESS_RADIUS_ITEM,

        // OUTPUT_PARAMETERS_BOX
        OUTPUT_NAME_ITEM = 0,
        OUTPUT_SELECT_ITEM,
        OUTPUT_ATTRIBUTE_NAME_ITEM,
        OUTPUT_STEP_X_ITEM,
        OUTPUT_STEP_Y_ITEM,

        // COMPUTATION_PARAMETERS_BOX
        COMPUTATION_THREADS_NUM_ITEM = 0,
        COMPUTATION_PROGRESS_ITEM,

        // Dialog buttons
        DIALOG_BUTTON_CLOSE_X = -1,
        DIALOG_BUTTON_EXPORT,
        DIALOG_BUTTON_CANCEL
    } ;

    /**
     * @brief Types of export output.
     */
protected:

    enum OutputType
    {
        BuilderModel3d = 0,
        Model3d,
        NumberOfOutputTypes
    } ;

    static const char * NamesOfOutputTypes[] ;
    
    typedef PickingExportToModel3dTask::ModelFrameType ModelFrameType ;
    
    /**
     * @brief Structure for export parameters description.
     */
private:
    struct Parameters
    {
        OutputType output_type = NumberOfOutputTypes ; //!< Type of output.
        ModelFrameType model_frame_type = ModelFrameType::NumberOfModelFrameTypes ; //!< Type of model frame.
        double horizons_step = 250 ; //!< Step of horizons (msec | m).
        std::string frame_model_name ; //!< Name of frame model.
        bool use_topo_horizon = false ; //!< True if first horizon has to be used as topography.
        double interpolation_radius = 2000 ; //!< Radius of interpolation/extrapolation operator.
        double smoothness_radius = 200 ; //!< Radius of operator influence, i.e. smoothness.
        std::string output_name ; //!< Name of output (model/cube)
        std::string output_attribute_name ; //!< Name of output attribute.
        double output_step_x = 100 ; //!< Step of output along X.
        double output_step_y = 100 ; //!< Step of output along Y.
    };

public:
    PickingExportDialog( Application * __app, int __parent_dialog_id = -1 ) ;
    ~PickingExportDialog() ;

    /**
     * @brief Opens dialog and performs all required initializations.
     */
public:
    bool open() ;

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
     * @brief Returns export parameters.
     */
private:
    bool getParameters( Parameters& __params, bool __check = false ) ;
    
    /**
     * @brief Returns number of processing threads.
     */
private:
    int getThreadsNum() const ;
    
    /**
     * @brief Setups name of frame model.
     */
private:
    void setFrameModelName( const std::string& __name ) ;
    
    /**
     * @brief Setups name of output model/cube.
     */
private:
    void setOutputName( const std::string& __name ) ;

    /**
     * @brief Updates dialog.
     */
private:
    void update() ;

    /**
     * @brief Exports picking.
     */
private:
    bool exportPicking() ;

    /**
     * @brief Exports picking to model.
     */
private:
    bool exportPickingToModel( const Parameters& __params, int __threads_num ) ;

    /**
     * @brief Exports picking to cube.
     */
private:
    bool exportPickingToCube( const Parameters& __params, int __threads_num ) ;

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
    static void on_export_target_changed( int __dialog, int __box, int __item, int __button ) ;
    static void on_model_frame_changed( int __dialog, int __box, int __item, int __button ) ;
    static void on_horizons_step_changed( int __dialog, int __box, int __item ) ;
    static void on_frame_model_select( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state ) ;
    static void on_output_name_select( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state ) ;
    //!@}

private:
    DIALOG * m_dialog = 0 ; //!< Pointer to dialog.
    Application * m_app ;   //!< Parent application.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_PICKING_EXPORT_DIALOG_H */

