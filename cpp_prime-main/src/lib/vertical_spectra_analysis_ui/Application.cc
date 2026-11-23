
#include <assert.h>
#include <memory>
#include <string>

#include "primeApp/PrimeAppNoX.h"
#include "primeAppGUI/appUtilities/AppWaitingStateGuard.h"
#include "configdirs/configdirs.h"
#include "seismicReader/RawReaderFactory.h"
#include "files/Names.h"
#include "files/FilesOperations.h"

#include "Application.h"
#include "DataReader.h"
#include "PickingImportDialog.h"
#include "PickingExportDialog.h"
#include "PickingInterpolator.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

const time_t Application::DEF_PICKING_AUTOSAVE_INTERVAL = 300 ; // 5 minutes

Application::Application() :
m_dialog_id(-1),
m_current_analysis_object_id(-1),
m_current_analysis_object_index(-1),
m_current_data_point_index(-1) { }

Application::~Application() { }

bool Application::run(const std::string& __grid_name)
{
    m_grid_name = __grid_name ;
    getWindowsManager().initSynchronization() ;

    if ( !loadData() )
    {
        m_picking_storage.clear() ;
        m_data_storage.unloadData() ;
        return false ;
    }

    update() ;

    return true ;
}

void Application::stop()
{
    if (m_map_window)
    {
        m_map_window->setDrawing(false) ;
        m_map_window->update() ;
    }

    if ( m_picking_storage.needsSaving() )
    {
        std::ostringstream message ;
        message << "Current picking '" << m_picking_name << "' is not saved." << std::endl
                << "Do you want to save it before exit?" ;

        if ( DisplayYesNoQuestionDialog("Picking saving is required", message.str(), m_dialog_id) == 0 )
            savePicking() ;
    }

    if (m_windows_manager)
        m_windows_manager->unloadData() ;
    
    m_current_analysis_object_id = -1 ;
    m_current_analysis_object_index = -1 ;
    m_current_data_point_index = -1 ;
    m_current_data_point = nullptr ;
    m_current_picking_point = nullptr ;
    m_picking_storage.clear() ;
    m_data_storage.unloadData() ;
}

void Application::update()
{
    getWindowsManager().update() ;

    if (m_nav_panel)
        m_nav_panel->update() ;

    if (m_tools_panel)
        m_tools_panel->update() ;

    if (m_objects_list_panel)
        m_objects_list_panel->update() ;

    if (m_map_window)
    {
        m_map_window->setDrawing(true) ;
        m_map_window->update() ;
    }

    updateTitle() ;
}

void Application::updatePickingDependedStuff()
{
    //getWindowsManager().update( Prime::Vis::SeismicWindow::FRONT_PLANE ) ;
    getWindowsManager().updatePickingDependedStuff() ;
}

void Application::bindDialog( int __dialog )
{
    m_dialog_id = __dialog ;
}

void Application::bindTopMenuPanel( int __dialog )
{
    m_top_menu_panel.reset( new TopMenuPanel(this, __dialog) ) ;
}

void Application::bindNavigationPanel( int __dialog, int __box )
{
    m_nav_panel.reset( new NavigationPanel(this, __dialog, __box) ) ;
}

void Application::bindToolsPanel( int __dialog, int __box, int __items_beg )
{
    m_tools_panel.reset( new ToolsPanel(this, __dialog, __box, __items_beg) ) ;
}

void Application::bindObjectsListPanel( int __dialog, int __box, int __items_beg )
{
    m_objects_list_panel.reset( new ObjectsListPanel(this, __dialog, __box, __items_beg) ) ;
}

void Application::bindGatherWindow( int __window_id )
{    
    getWindowsManager().setGatherWindow( __window_id ) ;
    SetWindowData( __window_id, "Application", this ) ;
}

void Application::bindSpectrumWindow( int __window_id )
{
    getWindowsManager().setSpectrumWindow( __window_id ) ;
    SetWindowData( __window_id, "Application", this ) ;
}

void Application::bindStackGatherWindow( int __window_id )
{
    getWindowsManager().setStackGatherWindow( __window_id ) ;
    SetWindowData( __window_id, "Application", this ) ;
}

void Application::bindMapWindow( Prime::Vis::SliceWindow * __map_window )
{
    m_map_window.reset( new MapWindow(this, __map_window) ) ;
}

Application::FunctionGetOpenedModel3d Application::getFunctionGetOpenedModel3d() const
{
    // Getting domain type
    using namespace Prime::lib::Nmo ;
    auto hodograph_type = getPickingStorage().getHodographType() ;
    bool is_time_domain = (HodographType::get_domain_type(hodograph_type) == FR_IO_TIME) ;

    return is_time_domain
            ? m_get_opened_time_model_function
            : m_get_opened_depth_model_function ;
}

const std::string& Application::getGridName() const
{
    return m_grid_name ;
}

const DataStorage& Application::getDataStorage() const
{
    return m_data_storage ;
}

const PickingStorage& Application::getPickingStorage() const
{
    return m_picking_storage ;
}

TasksExecutor& Application::getTasksExecutor()
{
    if (!m_tasks_executor)
    {
        m_tasks_executor.reset( new TasksExecutor() ) ;
        assert( m_tasks_executor.get() ) ;
    }

    return *m_tasks_executor ;
}

ProgressDialog * Application::getProgressDialog()
{
    if (!m_progress_dialog)
    {
        m_progress_dialog.reset( new ProgressDialog(PROGRESS_BOX::ACTIVITY, "Progress", getDialogById(m_dialog_id), false) ) ;

        if (m_progress_dialog)
            m_progress_dialog->close() ;
    }

    return m_progress_dialog.get() ;
}

const TopMenuPanel* Application::getTopMenuPanel() const
{
    return m_top_menu_panel.get() ;
}

ObjectsListPanel * Application::getObjectsListPanel()
{
    return m_objects_list_panel.get() ;
}

WindowsManager& Application::getWindowsManager()
{
    if (!m_windows_manager)
    {
        m_windows_manager.reset( new WindowsManager(this) ) ;
        assert( m_windows_manager.get() ) ;
    }

    return *m_windows_manager ;
}

DataOpenDialog& Application::getDataOpenDialog()
{
    if (!m_data_open_dialog)
    {
        m_data_open_dialog.reset( new DataOpenDialog() ) ;
        assert( m_data_open_dialog.get() ) ;
    }

    return *m_data_open_dialog ;
}

PickingImportDialog& Application::getPickingImportDialog()
{
    if (!m_picking_import_dialog)
    {
        m_picking_import_dialog.reset( new PickingImportDialog(this, m_dialog_id) ) ;
        assert( m_picking_import_dialog.get() ) ;
    }

    return *m_picking_import_dialog ;
}

PickingExportDialog& Application::getPickingExportDialog()
{
    if (!m_picking_export_dialog)
    {
        m_picking_export_dialog.reset( new PickingExportDialog(this, m_dialog_id) ) ;
        assert( m_picking_export_dialog.get() ) ;
    }

    return *m_picking_export_dialog ;
}

PickingPointsAddingDialog& Application::getPickingPointsAddingDialog()
{
    if (!m_picking_points_adding_dialog)
    {
        m_picking_points_adding_dialog.reset( new PickingPointsAddingDialog(m_dialog_id) ) ;
        assert( m_picking_points_adding_dialog.get() ) ;
    }

    return *m_picking_points_adding_dialog ;
}

Application::LevelsDialog* Application::getLevelsDialog( int __window_id )
{
    if ( !m_levels_dialog )
    {
        std::vector<WindowsManager::BaseWindow*> windows = getWindowsManager().getWindowsList() ;

        if ( windows.empty() )
            return 0 ;

        typedef Prime::Vis::SeismicLevelsController LevelsController ;
        std::vector<LevelsController *> controllers( windows.size() + 1, 0 ) ;
        std::vector<LevelsController::Element> elements( windows.size() ) ;

        for ( int i = 0; i < windows.size(); ++i )
        {
            controllers[i] = windows[i] ;
            elements[i] = LevelsController::Element(LevelsController::COLOR |
                                                    LevelsController::WIGGLE |
                                                    LevelsController::ADJUST) ;
        }

        m_levels_dialog.reset( new LevelsDialog(controllers.data(), elements.data(), m_dialog_id) ) ;
    }

    if ( !m_levels_dialog )
        return 0 ;

    const int index = getWindowsManager().getWindowIndexById( __window_id ) ;
    m_levels_dialog->setActiveController( index < 0 ? 0 : index ) ;
    //m_levels_dialog->setChangeValueHandler( levels_dialog_update_handler, this ) ;
    return m_levels_dialog.get() ;
}

Application::PreferencesDialog* Application::getPreferencesDialog( int __window_id )
{
    if ( !m_preferences_dialog )
    {
        std::vector<WindowsManager::BaseWindow*> windows = getWindowsManager().getWindowsList() ;

        if ( windows.empty() )
            return 0 ;

        typedef Prime::Vis::PreferencesController PController ;

        std::vector<PController *> controllers( windows.size() + 1, 0 ) ;
        std::vector<PController::Element> elements( windows.size() ) ;
        std::vector<PController::VisualMode> visual_modes( windows.size() ) ;
        PController::Element additional_elements = PController::Element(PController::FONTS) ;

        for ( int i = 0; i < windows.size(); ++i )
        {
            controllers[i] = windows[i] ;
            elements[i] = PController::Element(PController::VISUAL_MODE |
                                               PController::SEISMIC |
                                               PController::COLORS) ;

            visual_modes[i] = PController::VisualMode(PController::VISUAL_MODE_NONE |
                                                      PController::VISUAL_MODE_COLOR |
                                                      PController::VISUAL_MODE_WIGGLE |
                                                      PController::VISUAL_MODE_WIGGLE_MIN |
                                                      PController::VISUAL_MODE_WIGGLE_MAX) ;
        }

        m_preferences_dialog.reset( new PreferencesDialog(controllers.data(), elements.data(),
                                                          additional_elements, visual_modes.data(),
                                                          PController::VISUAL_MODE_NONE, m_dialog_id) ) ;
    }

    if ( !m_preferences_dialog )
        return 0 ;

    const int index = getWindowsManager().getWindowIndexById( __window_id ) ;
    m_preferences_dialog->setActiveController( index < 0 ? 0 : index ) ;
    //m_preferences_dialog->setChangeValueHandler( preferences_dialog_update_handler, this ) ;
    return m_preferences_dialog.get() ;
}

void Application::updateTitle()
{
    std::string title = "Vertical spectra analysis" ;

    if ( !m_picking_name.empty() )
    {
        title += ". Picking: " + m_picking_name ;

        if ( m_picking_storage.needsSaving() )
            title += "*" ;
    }

    DialogSetTitle( m_dialog_id, title.c_str() ) ;
}

void Application::newPicking()
{
    m_picking_name.clear() ;
    m_current_picking_point = 0 ;
    m_interpolated_picking_point.curve().clear() ;
    m_picking_storage.clear() ;
    setDataPoint( getAnalysisObjectID(), getDataPointIndex() ) ;
}

void Application::loadPicking()
{
    if ( !Prime::lib::primeApp::getProjCD() )
        return ;

    Prime::lib::primeApp::AppWaitingStateGuard waiting_guard( true ) ;
    Prime::Files::Names names( *Prime::lib::primeApp::getProjCD() ) ;

    std::string picking_name ;

    if ( DisplaySelectionDialog("Load vertical spectra picking",  false, true,
                                "Pickings", "Picking name",
                                names.listVerticalSpectraPicking3dVersions(m_grid_name),
                                &picking_name, m_dialog_id) != 0 )
    {
        return ;
    }

    std::string picking_filename = names.verticalSpectraPicking3dName( m_grid_name, picking_name ) ;

    if ( m_picking_storage.load(picking_filename) )
        m_picking_name = picking_name ;

    setDataPoint( getAnalysisObjectID(), getDataPointIndex() ) ;
}

void Application::savePicking()
{
    if ( !Prime::lib::primeApp::getProjCD() )
        return ;

    if ( !m_picking_name.empty() )
    {
        Prime::lib::primeApp::AppWaitingStateGuard waiting_guard( true ) ;
        Prime::Files::Names names( *Prime::lib::primeApp::getProjCD() ) ;
        std::string picking_filename = names.verticalSpectraPicking3dName( m_grid_name, m_picking_name ) ;
        m_picking_storage.save( picking_filename ) ;
        update() ;
    }
    else
    {
        savePickingAs() ;
    }
}

void Application::savePickingAs()
{
    if ( !Prime::lib::primeApp::getProjCD() )
        return ;

    Prime::Files::Names names( *Prime::lib::primeApp::getProjCD() ) ;

    std::string picking_name ;

    if ( DisplaySelectionDialog("Save vertical spectra picking",  true, false,
                                "Pickings", "Picking name",
                                names.listVerticalSpectraPicking3dVersions(m_grid_name),
                                &picking_name, m_dialog_id) != 0 )
    {
        return ;
    }

    if ( picking_name.empty() )
    {
        DisplayErrorDialog( "Error", "Name of output picking was not specified." ) ;
        return ;
    }

    std::string picking_filename = names.verticalSpectraPicking3dName( m_grid_name, picking_name ) ;

    if ( Prime::Files::isFileExist(picking_filename) )
    {
        std::ostringstream message ;
        message << "Picking '" << picking_name << "' is already exists." << std::endl
                << "Do you want to overwrite it?" ;

        if ( DisplayYesNoQuestionDialog("Picking overwriting", message.str(), m_dialog_id) != 0 )
            return ;
    }

    Prime::lib::primeApp::AppWaitingStateGuard waiting_guard( true ) ;

    if ( m_picking_storage.save(picking_filename) )
        m_picking_name = picking_name ;

    update() ;
}

void Application::importPicking()
{
    getPickingImportDialog().open( m_picking_storage ) ;
}

void Application::exportPicking()
{
    getPickingExportDialog().open() ;
}

bool Application::getPickingAutoSave() const
{
    return m_picking_autosave ;
}

void Application::setPickingAutoSave( bool __autosave )
{
    m_picking_autosave = __autosave ;
    m_picking_autosave_last_time = time( 0 ) ;
}

bool Application::getCursorTrackingLinesUsage() const
{
    return m_use_cursor_tracking_lines ;
}

void Application::setCursorTrackingLinesUsage( bool __use )
{
    m_use_cursor_tracking_lines = __use ;
}

void Application::autosavePickingIfRequired()
{
    if ( !getPickingAutoSave() )
        return ;

    if ( time(0) - m_picking_autosave_last_time < DEF_PICKING_AUTOSAVE_INTERVAL )
        return ;

    if ( !getPickingStorage().needsSaving() )
        return ;

    savePicking() ;
    m_picking_autosave_last_time = time( 0 ) ;
}

bool Application::loadData()
{
    if ( !getDataOpenDialog().open(this,
                                   m_data_storage,
                                   m_picking_storage,
                                   m_grid_name,
                                   m_picking_name) )
        return false ;

    setPickingAutoSave( getPickingAutoSave() ) ;
    loadAnalysisObjectByIndex( 0 ) ;

    return true ;
}

void Application::loadAnalysisObjectByIndex( int __object_index )
{
    if ( m_data_storage.getObjects().empty() )
        return ;

    __object_index = std::max<int>(__object_index, 0) ;
    __object_index = std::min<int>(__object_index, m_data_storage.getObjects().size() - 1) ;

    const int object_id = m_data_storage.getObjects().at(__object_index) ;
    setDataPoint( object_id, 0 ) ;
}

int Application::getAnalysisObjectIndex() const
{
    if (m_current_analysis_object_index < 0 ||
        m_current_analysis_object_index >= m_data_storage.getObjects().size())
        return -1 ;

    return m_current_analysis_object_index ;
}

int Application::getAnalysisObjectID() const
{
    return m_current_analysis_object_id ;
}

int Application::getDataPointIndex() const
{
    return m_current_data_point_index ;
}

const DataStorage::DataPoint * Application::getDataPoint() const
{
    return m_current_data_point ;
}

const PickingPoint * Application::getPickingPoint() const
{
    return m_current_picking_point ;
}

int Application::getDataPointsNum()
{
    return getDataStorage().getDataPoints(getAnalysisObjectID()).size() ;
}

void Application::setDataPoint( ObjectID __object_id, int __point_index )
{
    const auto& data_points = getDataStorage().getDataPoints(__object_id) ;

    if ( data_points.empty() )
        return ;

    __point_index = std::max<int>(__point_index, 0) ;
    __point_index = std::min<int>(__point_index, data_points.size() - 1) ;

    // Trying to find required point in picking storage
    const int ncdp = data_points[__point_index].ncdp ;
    m_current_data_point = &data_points[__point_index] ;
    m_current_picking_point = m_picking_storage.getPickingPoint(__object_id, ncdp) ;

    // Calculating interpolated curve
    const PickingPoint * left_picking_point = m_picking_storage.findLeftClosestPoint(__object_id, ncdp, true) ;
    const PickingPoint * right_picking_point = m_picking_storage.findRightClosestPoint(__object_id, ncdp, true) ;

    m_interpolated_picking_point = interpolate_picking_point(*m_current_data_point,
                                                             left_picking_point,
                                                             right_picking_point) ;

    // Loading required data to windows
    // If new object was selected then data reader reloading is required
    if ( m_current_analysis_object_id != __object_id )
    {
        getWindowsManager().loadData( getDataStorage(), __object_id ) ;
        getWindowsManager().setDataPoint(__point_index, m_current_picking_point, &m_interpolated_picking_point) ;
    }
    else
    {
        getWindowsManager().setDataPoint(__point_index, m_current_picking_point, &m_interpolated_picking_point) ;
    }

    // Save current point
    m_current_analysis_object_id = __object_id ;
    m_current_analysis_object_index = getDataStorage().getObjectIndex(__object_id) ;
    m_current_data_point_index = __point_index ;

    autosavePickingIfRequired() ;
    update() ;
}

void Application::addPickingPoints()
{
    int points_ncdp_step = 1 ;
    bool add_for_all_objects = false ;

    if ( !getPickingPointsAddingDialog().open(points_ncdp_step,
                                              add_for_all_objects) )
        return ;

    if ( add_for_all_objects )
    {
        for ( ObjectID object_id : getDataStorage().getObjects() )
            addPickingPoints( object_id, points_ncdp_step ) ;
    }
    else
    {
        addPickingPoints( getAnalysisObjectID(), points_ncdp_step ) ;
    }
}

void Application::addCurrentPointToPicking()
{
    if ( m_current_picking_point )
        return ;

    const auto& data_points = getDataStorage().getDataPoints(getAnalysisObjectID()) ;

    if (data_points.empty() ||
        m_current_data_point_index < 0 ||
        m_current_data_point_index >= data_points.size())
        return ;

    const auto& point = data_points.at( m_current_data_point_index ) ;
    m_picking_storage.addPickingPoint(getAnalysisObjectID(),
                                      PickingPoint(point.ncdp, point.x, point.y)) ;

    setDataPoint( getAnalysisObjectID(), getDataPointIndex() ) ;
}

void Application::addCurrentPointToPickingWithInterpolation()
{
    if ( m_current_picking_point )
        return ;

    m_picking_storage.addPickingPoint(getAnalysisObjectID(), m_interpolated_picking_point) ;
    setDataPoint( getAnalysisObjectID(), getDataPointIndex() ) ;
}

void Application::setInterpolatedPickingPoint()
{
    if ( !m_current_picking_point )
        return ;

    *m_current_picking_point = m_interpolated_picking_point ;
    setDataPoint( getAnalysisObjectID(), getDataPointIndex() ) ;
}

void Application::addPickingPoints( ObjectID __object_id, int __ncdp_step )
{
    const auto& data_points = getDataStorage().getDataPoints(__object_id) ;

    if ( data_points.empty() )
        return ;

    std::vector<PickingPoint> analysis_points ;
    analysis_points.reserve( data_points.size() ) ;

    int prev_ncdp = INT_MAX ;

    for ( const auto& point : data_points )
    {
        if (prev_ncdp != INT_MAX &&
            prev_ncdp + __ncdp_step > point.ncdp)
            continue ;

        analysis_points.push_back( PickingPoint(point.ncdp, point.x, point.y) ) ;
        prev_ncdp = point.ncdp ;
    }

    m_picking_storage.addPickingPoints(__object_id, analysis_points) ;

    update() ;
}

void Application::deleteCurrentPickingPoint()
{
    if ( !getPickingPoint() )
        return ;

    m_picking_storage.deletePickingPoint( getAnalysisObjectID(), getPickingPoint()->ncdp() ) ;
    setDataPoint( getAnalysisObjectID(), getDataPointIndex() ) ;
}

}
}
}