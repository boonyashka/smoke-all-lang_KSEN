/* 
 * File:   DataOpenTask.h
 * Author: ranisimov
 *
 * Created on August 7, 2023, 4:04 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_DATA_OPEN_TASK_H
#define VERTICAL_SPECTRA_ANALYSIS_DATA_OPEN_TASK_H

#include "vertical_spectra_analysis/PickingStorage.h"

#include "DataStorage.h"
#include "TasksExecutor.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Internal temporary class for data loading.
 */
class DataOpenTask : public ITask
{
public:

    DataOpenTask(DataStorage& __data_storage,
                 const std::string& __grid_name,
                 const std::string& __version_name) :
    m_data_storage(__data_storage),
    m_grid_name(__grid_name),
    m_version_name(__version_name) { }

    ~DataOpenTask() = default ;

    /**
     * @brief Returns name of task.
     */
public:

    virtual const std::string& name() const
    {
        static std::string name = "Loading data..." ;
        return name ;
    }

    /**
     * @brief Interface method for task starting.
     */
public:

    virtual bool run()
    {
        return m_data_storage.loadData(m_grid_name, m_version_name) ;
    }

private:
    DataStorage& m_data_storage ;
    const std::string m_grid_name ;
    const std::string m_version_name ;
} ;


}
}
}


#endif /* VERTICAL_SPECTRA_ANALYSIS_DATA_OPEN_TASK_H */

