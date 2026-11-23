/* 
 * File:   Application.h
 * Author: ranisimov
 *
 * Created on July 13, 2023, 6:19 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_APPLICATION_H
#define VERTICAL_SPECTRA_ANALYSIS_APPLICATION_H


#include <memory>
#include <ctime>

#include "toolkit/Appl.h"
#include "toolkit/ProgressDialog.h"
#include "seismicReader/RawReaderMetacube.h"
#include "windowDialogs/SeismicLevelsDialog.h"
#include "windowDialogs/PreferencesDialog.h"
#include "sliceWindow/SliceWindow.h"
#include "vertical_spectra_analysis/PickingStorage.h"

#include "TasksExecutor.h"
#include "DataOpenDialog.h"
#include "DataStorage.h"
#include "TopMenuPanel.h"
#include "WindowsManager.h"
#include "NavigationPanel.h"
#include "ToolsPanel.h"
#include "ObjectsListPanel.h"
#include "MapWindow.h"
#include "PickingPointsAddingDialog.h"
#include "PickingImportDialog.h"
#include "PickingExportDialog.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of vertical spectra analysis application.
 */
class Application
{    
    static const time_t DEF_PICKING_AUTOSAVE_INTERVAL ; //!< Default time interval of picking autosaving (in sec)
    
private:
    typedef Prime::Vis::SeismicLevelsDialog LevelsDialog ;
    typedef Prime::Vis::PreferencesDialog PreferencesDialog ;
    
public:
    typedef BasicModel3d * (*FunctionGetOpenedModel3d)() ;
    typedef void (*FunctionUpdateOpenedModel3d)( BasicModel3d * ) ;

public:
    Application() ;
    ~Application() ;

    /**
     * @brief Runs application.
     */
public:
    bool run(const std::string& __grid_name) ;
    
    /**
     * @brief Stops application.
     */
public:
    void stop() ;
    
    /**
     * @brief Updates application components.
     */
public:
    void update() ;
    
    /**
     * @brief Updates all components depended from picking process.
     */
public:
    void updatePickingDependedStuff() ;
    
    /**
     * @brief Binds to parent dialog.
     */
public:
    void bindDialog( int __dialog ) ;
    
    /**
     * @brief Binds top menu panel.
     */
public:
    void bindTopMenuPanel( int __dialog ) ;
    
    /**
     * @brief Binds navigation panel.
     */
public:
    void bindNavigationPanel( int __dialog, int __box ) ;
    
    /**
     * @brief Binds tools panel.
     */
public:
    void bindToolsPanel( int __dialog, int __box, int __items_beg ) ;

    /**
     * @brief Binds objects list panel.
     */
public:
    void bindObjectsListPanel( int __dialog, int __box, int __items_beg ) ;

    /**
     * @brief Binds super-gather window.
     */
public:
    void bindGatherWindow( int __window_id ) ;

    /**
     * @brief Binds spectrum window.
     */
public:
    void bindSpectrumWindow( int __window_id ) ;
    
    /**
     * @brief Binds stack-gather window.
     */
public:
    void bindStackGatherWindow( int __window_id ) ;
    
    /**
     * @brief Binds external map window.
     */
public:
    void bindMapWindow( Prime::Vis::SliceWindow * __map_window ) ;
    
    /**
     * @brief Setups function for getting opened external time model3d. 
     */
public:
    void setFunctionGetOpenedTimeModel3d( FunctionGetOpenedModel3d __function )
    {
        m_get_opened_time_model_function = __function ;
    }
    
    /**
     * @brief Setups function for getting opened external depth model3d. 
     */
public:
    void setFunctionGetOpenedDepthModel3d( FunctionGetOpenedModel3d __function )
    {
        m_get_opened_depth_model_function = __function ;
    }
    
    /**
     * @brief Returns function for getting opened external time/depth model3d. 
     * @attention Time or depth model will be returned depending on picking hodograph type.
     */
public:
    FunctionGetOpenedModel3d getFunctionGetOpenedModel3d() const ;
    
    /**
     * @brief Setups function for updating opened external model3d. 
     */
public:
    void setFunctionUpdateOpenedModel3d( FunctionUpdateOpenedModel3d __function )
    {
        m_update_opened_model3d_function = __function ;
    }
    
    /**
     * @brief Returns function for updating opened external model3d. 
     */
public:
    FunctionUpdateOpenedModel3d getFunctionUpdateOpenedModel3d() const
    {
        return m_update_opened_model3d_function ;
    }
    
    /**
     * @brief Returns name of working grid.
     */
public:
    const std::string& getGridName() const ;
    
    /**
     * @brief Returns access to data storage.
     */
public:
    const DataStorage& getDataStorage() const ;
    
    /**
     * @brief Returns access to picking storage.
     */
public:
    const PickingStorage& getPickingStorage() const ;
    
    /**
     * @brief Creates/returns access to tasks executor.
     */
public:
    TasksExecutor& getTasksExecutor() ;
    
    /**
     * @brief Creates/returns access to progress dialog.
     */
public:
    ProgressDialog * getProgressDialog() ;
    
    /**
     * @brief Creates/returns access to top menu panel.
     */
public:
    const TopMenuPanel* getTopMenuPanel() const ;
    
    /**
     * @brief Returns access to objects list panel if it was binded.
     */
public:
    ObjectsListPanel * getObjectsListPanel() ;
    
    /**
     * @brief Returns access to windows manager.
     */
public:
    WindowsManager& getWindowsManager() ;
    
    /**
     * @brief Returns access to data selection dialog.
     */
public:
    DataOpenDialog& getDataOpenDialog() ;
    
    /**
     * @brief Returns access to picking import dialog.
     */
public:
    PickingImportDialog& getPickingImportDialog() ;
    
    /**
     * @brief Returns access to picking export dialog.
     */
public:
    PickingExportDialog& getPickingExportDialog() ;
    
    /**
     * @brief Returns access to picking points adding dialog.
     */
private:
    PickingPointsAddingDialog& getPickingPointsAddingDialog() ;
    
    /**
     * @brief Creates and returns access to levels dialog.
     */
public:
    LevelsDialog* getLevelsDialog( int __window_id = -1 ) ;
    
    /**
     * @brief Creates and returns access to preferences dialog.
     */
public:
    PreferencesDialog* getPreferencesDialog( int __window_id = -1 ) ;
    
    /**
     * @brief Updates title of application.
     */
private:
    void updateTitle() ;
    
    /**
     * @brief Creates new picking. 
     */
public:
    void newPicking() ;
    
    /**
     * @brief Loads required picking. 
     */
public:
    void loadPicking() ;
    
    /**
     * @brief Saves picking to file. 
     */
public:
    void savePicking() ;
    
    /**
     * @brief Saves picking to new file. 
     */
public:
    void savePickingAs() ;
    
    /**
     * @brief Imports picking. 
     */
public:
    void importPicking() ;
    
    /**
     * @brief Exports picking. 
     */
public:
    void exportPicking() ;
    
    /**
     * @brief Returns picking autosave flag.
     */
public:
    bool getPickingAutoSave() const ;
    
    /**
     * @brief Setups picking autosave flag.
     */
public:
    void setPickingAutoSave( bool __autosave ) ;
    
    /**
     * @brief Returns cursor tracking lines usage flag.
     */
public:
    bool getCursorTrackingLinesUsage() const ;
    
    /**
     * @brief Setups cursor tracking lines usage flag.
     */
public:
    void setCursorTrackingLinesUsage( bool __use ) ;
    
    /**
     * @brief Autosave picking if required.
     */
private:
    void autosavePickingIfRequired() ;

    /**
     * @brief Loads new metacube.
     */
private:
    bool loadData() ;

    /**
     * @brief Loads data for required object index.
     */
public:
    void loadAnalysisObjectByIndex( int __object_index ) ;
    
    /**
     * @brief Returns index of current analysis object.
     */
public:
    int getAnalysisObjectIndex() const ;
    
    /**
     * @brief Returns ID of current analysis object.
     */
public:
    int getAnalysisObjectID() const ;
    
    /**
     * @brief Returns current analysis point index.
     */
public:
    int getDataPointIndex() const ;
    
    /**
     * @brief Returns current analysis point.
     */
public:
    const DataStorage::DataPoint * getDataPoint() const ;
    
    /**
     * @brief Returns current picking point.
     */
public:
    const PickingPoint * getPickingPoint() const ;
    
    /**
     * @brief Returns number of analysis points for current object.
     */
public:
    int getDataPointsNum() ;
    
    /**
     * @brief Setups current analysis point.
     */
public:
    void setDataPoint( ObjectID __object_id, int __point_index ) ;
    
    /**
     * @brief Adds points to picking.
     */
public:
    void addPickingPoints() ;
    
    /**
     * @brief Adds current point to picking.
     */
public:
    void addCurrentPointToPicking() ;
    
    /**
     * @brief Adds current point to picking and fill it by interpolated curve.
     */
public:
    void addCurrentPointToPickingWithInterpolation() ;
    
    /**
     * @brief Setups interpolated picking curve to current picking point.
     */
public:
    void setInterpolatedPickingPoint() ;
    
    /**
     * @brief Adds picking points for required object.
     */
private:
    void addPickingPoints( ObjectID __object_id, int __ncdp_step ) ;
    
    /**
     * @brief Deletes current picking point.
     */
public:
    void deleteCurrentPickingPoint() ;

private:
    int m_dialog_id ; //!< Dialog to which this application is binded.
    std::string m_grid_name ; //!< Name of grid at which this application is running (can be empty).
    std::string m_picking_name ; //!< Name of picking session name.
    
    FunctionGetOpenedModel3d m_get_opened_time_model_function = nullptr ; //!< Function for getting opened external time model 3d.
    FunctionGetOpenedModel3d m_get_opened_depth_model_function = nullptr ; //!< Function for getting opened external depth model 3d.
    FunctionUpdateOpenedModel3d m_update_opened_model3d_function = nullptr ; //!< Function for updating opened external model 3d.
    
    bool m_picking_autosave = false ; //!< Picking autosave flag.
    time_t m_picking_autosave_last_time ; //!< Time of last autosave printing (in seconds).
    bool m_use_cursor_tracking_lines = false ; //!< Cursor tracking lines flag.
    
    int m_current_analysis_object_id = -1 ; //!< ID of current analysis object loaded for analysis.
    int m_current_analysis_object_index = -1 ; //!< Index of current analysis object.
    int m_current_data_point_index = -1 ; //!< Index of current analysis point of current analysis object.
    const DataStorage::DataPoint * m_current_data_point = nullptr ; //!< Pointer to current analysis data point.
    PickingPoint * m_current_picking_point = nullptr ; //!< Pointer to analysis point.
    PickingPoint m_interpolated_picking_point ; //!< Interpolated picking curve at the current data point.
    
    DataStorage m_data_storage ; //!< Input data storagee.
    PickingStorage m_picking_storage ; //!< Analysis points picking storage.
    
    std::unique_ptr<TasksExecutor> m_tasks_executor ; //!< Object for interactive tasks running.
    std::unique_ptr<ProgressDialog> m_progress_dialog ; //!< Dialog for progress showing.
    
    std::unique_ptr<TopMenuPanel> m_top_menu_panel ; //!< Top menu panel.
    std::unique_ptr<NavigationPanel> m_nav_panel ; //!< Navigation panel.
    std::unique_ptr<ToolsPanel> m_tools_panel ; //!< Tools panel.
    std::unique_ptr<ObjectsListPanel> m_objects_list_panel ; //!< Panel of analysis objects list.
    std::unique_ptr<WindowsManager> m_windows_manager ; //!< Windows manager component.
    std::unique_ptr<MapWindow> m_map_window ; //!< Map window component.
    
    std::unique_ptr<DataOpenDialog> m_data_open_dialog ; //!< Input data selection dialog.
    std::unique_ptr<PickingImportDialog> m_picking_import_dialog ; //!< Picking import dialog.
    std::unique_ptr<PickingExportDialog> m_picking_export_dialog ; //!< Picking export dialog.
    std::unique_ptr<PickingPointsAddingDialog> m_picking_points_adding_dialog ; //!< Dialog for adding picking points.
    std::unique_ptr<LevelsDialog> m_levels_dialog ; //!< Levels dialog.
    std::unique_ptr<PreferencesDialog> m_preferences_dialog ; //!< Preferences dialog.
} ;

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_APPLICATION_H */

