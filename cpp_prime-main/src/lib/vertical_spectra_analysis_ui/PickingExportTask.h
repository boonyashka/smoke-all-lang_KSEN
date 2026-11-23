/* 
 * File:   PickingExportTask.h
 * Author: ranisimov
 *
 * Created on August 3, 2023, 11:56 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_PICKING_EXPORT_TASK_H
#define VERTICAL_SPECTRA_ANALYSIS_PICKING_EXPORT_TASK_H

#include "vertical_spectra_analysis/PickingStorage.h"

#include "DataStorage.h"
#include "TasksExecutor.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of picking export task.
 */
class PickingExportToModel3dTask : public ITask
{
    /**
     * @brief Types of export model frame.
     */
public:

    enum ModelFrameType
    {
        ConstantStepHorizons = 0,
        ModelHorizons,
        NumberOfModelFrameTypes
    } ;

    static const char * NamesOfModelFrameTypes[] ;

    /**
     * @brief Constructs task.
     * @param[out] __output_model Pointer to opened output model. If 0 then model with name @a __output_name will be opened.
     */
public:
    PickingExportToModel3dTask(const PickingStorage& __picking_storage,
                               const DataStorage& __data_storage,
                               const std::string& __grid_name,
                               ModelFrameType __model_frame_type,
                               double __horizons_step,
                               const std::string& __frame_model_name,
                               bool __use_topo_horizon,
                               double __interpolation_radius,
                               double __smoothness_radius,
                               BasicModel3d * __output_model,
                               const std::string& __output_name,
                               const std::string& __output_attribute_name,
                               double __output_step_x,
                               double __output_step_y) ;

    /**
     * @brief Destructs task.
     */
public:
    ~PickingExportToModel3dTask() ;

    /**
     * @brief Returns name of task.
     */
public:
    virtual const std::string& name() const ;

    /**
     * @brief Interface method for task starting.
     */
public:
    virtual bool run() ;

private:
    const PickingStorage& m_picking_storage ;
    const DataStorage& m_data_storage ;
    std::string m_grid_name ;
    ModelFrameType m_model_frame_type ;
    double m_horizons_step ;
    const std::string m_frame_model_name ;
    bool m_use_topo_horizon ;
    double m_interpolation_radius ;
    double m_smoothness_radius ;
    BasicModel3d * m_output_model ;
    const std::string m_output_name ;
    const std::string m_output_attribute_name ;
    double m_output_step_x ;
    double m_output_step_y ;
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_PICKING_EXPORT_TASK_H */

