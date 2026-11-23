/* 
 * File:   TasksExecutor.h
 * Author: ranisimov
 *
 * Created on August 3, 2023, 2:32 AM
 */

#ifndef PRIME_LIB_TASKS_EXECUTOR_H
#define PRIME_LIB_TASKS_EXECUTOR_H

#include "toolkit/ProgressDialog.h"
#include "Threads/mutex.h"
#include "Threads/mutex_locker.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class ITask ;
class TasksExecutor ;

/**
 * @brief Base class of some task.
 * @attention Task should not contain to any calls to UI components (message dialogs for example).
 */
class ITask
{
    friend class TasksExecutor ;
    
    /**
     * @brief Constructs default object.
     */
public:
    ITask() ;
    
    /**
     * @brief Destructs task object.
     */
public:
    virtual ~ITask() ;
    
    /**
     * @brief Returns name of task.
     */
public:
    virtual const std::string& name() const = 0 ;
    
    /**
     * @brief Interface method for task starting.
     */
public:
    virtual bool run() = 0 ;
    
    /**
     * @brief Setups running flag.
     */
protected:
    void setRunningFlag( bool __is_running ) ;
    
    /**
     * @brief Returns running flag.
     */
public:
    bool isRunning() const ;
    
    /**
     * @brief Method for setup stop flag.
     */
public:
    void stop() ;
    
    /**
     * @brief Setups stop flag.
     */
protected:
    void setStopFlag( bool __is_stopped ) ;
    
    /**
     * @brief Method for checking stop flag.
     */
public:
    bool isStopped() const ;
    
    /**
     * @brief Setups progress of task (in percents).
     */
public:
    void setProgress( double __progress ) ;
    
    /**
     * @brief Returns current progress of task (in percents).
     */
public:
    double getProgress() const ;
    
    /**
     * @brief Setups error description.
     */
protected:
    void setErrorDescription( const std::string& __error ) ;
    
    /**
     * @brief Returns error description.
     */
public:
    const std::string& getErrorDescription() const ;
    
    /**
     * @brief Setups computational threads number.
     */
protected:
    void setThreadsNum( int __threads_num ) ;
    
    /**
     * @brief Returns number of computational threads.
     */
protected:
    int getThreadsNum() const ;
    
private:
    bool m_is_running = false ; //!< Running flag.
    bool m_is_stopped = false ; //!< Stop flag.
    double m_progress = 0 ; //!< Current progress of task.
    std::string m_error_description ; //!< Description of error.
    int m_threads_num = 1 ; //!< Number of computational threads.
    mutable Prime::lib::threads::MutexOMP lock ; //!< Lock for changes.
};

/**
 * @brief Class of task executor.
 * @details Task executor performs tasks in separate thread for avoiding UI interface
 * freezing and progress bars handling.
 */
class TasksExecutor
{
    /**
     * @brief Structure for storing widget information.
     */
private:
    struct WidgetID
    {
        WidgetID() :
        dialog(-1),
        box(-1),
        item(-1) {}
        
        /**
         * @brief Checks whether widget is defined.
         */
        bool defined() const
        {
            return ( dialog >= 0 || box >= 0 || item >= 0 ) ;
        }
        
        int dialog ; //!< Dialog id of widget.
        int box ; //!< Box id of widget.
        int item ; //!< Item id of widget.
    };
    
    /**
     * @brief Constructs default object.
     */
public:
    TasksExecutor() ;
    
    /**
     * @brief Destructs object.
     */
public:
    virtual ~TasksExecutor() ;
    
    /**
     * @brief Binds progress dialog.
     */
public:
    void bindProgressDialog( ProgressDialog * __progress_dialog ) ;
    
    /**
     * @brief Resets progress dialog.
     */
public:
    void resetProgressDialog() ;
    
    /**
     * @brief Binds progress bar widget.
     */
public:
    void bindProgressBar( int __dialog, int __box, int __item ) ;
    
    /**
     * @brief Resets progress bar.
     */
public:
    void resetProgressBar() ;
    
    /**
     * @brief Runs required task.
     */
public:
    bool runTask( ITask& __task, int __threads_num = 1 ) ;
    
    /**
     * @brief Updates progress bar if required.
     */
private:
    void updateProgressStatus( const ITask& __task, int __progress ) ;
    
    /**
     * @brief Updates progress of binded progress dialog.
     */
private:
    void updateProgressDialog( const ITask& __task, int __progress ) ;
    
    /**
     * @brief Updates progress of binded progress widget.
     */
private:
    void updateProgressWidget( const ITask& __task, int __progress ) ;
    
private:
    ProgressDialog * m_progress_dialog = nullptr ; //!< External progress dialog.
    WidgetID m_progress_bar ; //!< External widget of progress bar.
};

}
}
}

#endif /* PRIME_LIB_TASKS_EXECUTOR_H */

