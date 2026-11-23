/* 
 * File:   PickingImportTask.h
 * Author: ranisimov
 *
 * Created on August 11, 2023, 2:06 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_PICKING_IMPORT_TASK_H
#define VERTICAL_SPECTRA_ANALYSIS_PICKING_IMPORT_TASK_H

#include "vertical_spectra_analysis/PickingStorage.h"

#include "DataStorage.h"
#include "TasksExecutor.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of task for picking import from model 3d.
 */
class PickingImportFromModel3dTask : public ITask
{
    /**
     * @brief Constructs task.
     * @param[out] __picking_storage Output picking storage. Has to contain picking points to which curve from model will be exported.
     * @param[in] __grid_name Name of grid.
     * @param[in] __input_model Pointer to opened input model. If 0 then model with name @a __input_name will be opened.
     * @param[in] __input_attribute_name Attribute name from which picking curve will be extracted.
     */
public:
    PickingImportFromModel3dTask(PickingStorage& __picking_storage,
                                 const std::string& __grid_name,
                                 const BasicModel3d * __input_model,
                                 const std::string& __input_name,
                                 const std::string& __input_attribute_name) ;

    /**
     * @brief Destructs task.
     */
public:
    ~PickingImportFromModel3dTask() ;

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
    PickingStorage& m_picking_storage ;
    std::string m_grid_name ;
    const BasicModel3d * m_input_model ;
    const std::string m_input_name ;
    const std::string m_input_attribute_name ;
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_PICKING_IMPORT_TASK_H */

