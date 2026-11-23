
#include <unistd.h>
#include <omp.h>

#include "toolkit/Main.h"
#include "toolkit/DialogInterface.h"
#include "utils/methodname.h"

#include "TasksExecutor.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

ITask::ITask() { }

ITask::~ITask()
{
    if ( !isRunning() )
        stop() ;
}

void ITask::setRunningFlag( bool __is_running )
{
    Prime::lib::threads::MutexAutoLocker locker( lock ) ;
    m_is_running = __is_running ;
}

bool ITask::isRunning() const
{
    Prime::lib::threads::MutexAutoLocker locker( lock ) ;
    return m_is_running ;
}

void ITask::stop()
{
    setStopFlag( true ) ;
}

void ITask::setStopFlag( bool __is_stopped )
{
    Prime::lib::threads::MutexAutoLocker locker( lock ) ;
    m_is_stopped = __is_stopped ;
}

bool ITask::isStopped() const
{
    Prime::lib::threads::MutexAutoLocker locker( lock ) ;
    return m_is_stopped ;
}

void ITask::setProgress( double __progress )
{
    Prime::lib::threads::MutexAutoLocker locker( lock ) ;
    m_progress = std::max( __progress, 0.0 ) ;
    m_progress = std::min( m_progress, 100.0 ) ;
}

double ITask::getProgress() const
{
    Prime::lib::threads::MutexAutoLocker locker( lock ) ;
    return m_progress ;
}

void ITask::setErrorDescription( const std::string& __error )
{
    m_error_description = __error ;
}

const std::string& ITask::getErrorDescription() const
{
    return m_error_description ;
}

void ITask::setThreadsNum( int __threads_num )
{
    m_threads_num = std::max<int>(__threads_num, 1) ;
}

int ITask::getThreadsNum() const
{
    return m_threads_num ;
}

TasksExecutor::TasksExecutor() { }

TasksExecutor::~TasksExecutor() { }

void TasksExecutor::bindProgressDialog( ProgressDialog * __progress_dialog )
{
    m_progress_dialog = __progress_dialog ;
}

void TasksExecutor::resetProgressDialog()
{
    m_progress_dialog = nullptr ;
}

void TasksExecutor::bindProgressBar( int __dialog, int __box, int __item )
{
    m_progress_bar.dialog = __dialog ;
    m_progress_bar.box = __box ;
    m_progress_bar.item = __item ;
}

void TasksExecutor::resetProgressBar()
{
    m_progress_bar = WidgetID() ;
}

bool TasksExecutor::runTask( ITask& __task, int __threads_num )
{
    define_methodname() ;

    omp_set_nested( true ) ;
    omp_set_max_active_levels(INT_MAX) ;
    omp_set_dynamic( false ) ;

    bool result_status = false ;

    __task.setErrorDescription( "" ) ;
    __task.setThreadsNum( __threads_num ) ;
    __task.setRunningFlag( true ) ;
    __task.setStopFlag( false ) ;
    __task.setProgress( 0 ) ;
    
    #pragma omp parallel sections num_threads( 2 )
    {
        // Interactive thread
        #pragma omp section
        {
            int current_progress = -1 ;

            while ( true )
            {
                if ( !__task.isRunning() )
                {
                    updateProgressStatus( __task, 100 ) ;
                    break ;
                }

                // Updating progress bar if required
                int progress = int( __task.getProgress() ) ;

                if ( progress == 0 || progress != current_progress )
                {
                    current_progress = progress ;
                    updateProgressStatus( __task, progress ) ;
                }

                // Updating GUI
                ProcessEventsIfAnyExist() ;

                // Sleeping for a short time
                const unsigned int microseconds = 100000 ;
                usleep( microseconds ) ;
            }
        }

        // Computational thread
        #pragma omp section
        {
            try
            {
                result_status = __task.run() ;
            }
            catch ( const std::exception& __err )
            {
                result_status = false ;

                std::cout << __methodname__ << ": Error. " << __err.what() << std::endl ;
            }
            catch (...)
            {
                result_status = false ;
            }

            __task.setRunningFlag( false ) ;
            __task.setProgress( 100 ) ;
        }
    }

    return result_status ;
}

void TasksExecutor::updateProgressStatus( const ITask& __task, int __progress )
{
    updateProgressDialog( __task, __progress ) ;
    updateProgressWidget( __task, __progress ) ;
}

void TasksExecutor::updateProgressDialog( const ITask& __task, int __progress )
{
    if ( !m_progress_dialog )
        return ;
    
    m_progress_dialog->setLabel( __task.name().c_str() ) ;
    
//    if ( __progress == 0 )
//    {
//        m_progress_dialog->pulse() ;
//    }
//    else
    {
        m_progress_dialog->setProgress( __progress ) ;
    }
}

void TasksExecutor::updateProgressWidget( const ITask& __task, int __progress )
{
    if ( !m_progress_bar.defined() )
        return ;

    DialogSetProgressLabel( m_progress_bar.dialog, m_progress_bar.box, m_progress_bar.item, __task.name().c_str() ) ;
    
    if ( __progress == 0 )
    {
        DialogProgressPulse( m_progress_bar.dialog, m_progress_bar.box, m_progress_bar.item ) ;
    }
    else
    {
        DialogSetProgressFraction( m_progress_bar.dialog, m_progress_bar.box, m_progress_bar.item, __progress / 100.0 ) ;
    }
}

}
}
}

