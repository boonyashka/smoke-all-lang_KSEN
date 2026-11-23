
#include <sstream>
#include <stdexcept>

#include "toolkit/Appl.h"
#include "primeApp/PrimeAppNoX.h"
#include "primeAppGUI/appUtilities/AppWaitingStateGuard.h"
#include "db/Grid.h"
#include "metacube/types.h"
#include "utils/methodname.h"
#include "configdirs/configdirs.h"
#include "files/Names.h"
#include "modelsel/versionsel2d.h"

#include "DataReader.h"
#include "DataOpenDialog.h"
#include "DataOpenTask.h"
#include "TasksExecutor.h"
#include "Application.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

DataOpenDialog::DataOpenDialog() :
m_dialog( 0 ),
m_app( 0 ),
m_names( *Prime::lib::primeApp::getProjCD() ),
m_data_storage( 0 ),
m_picking_storage( 0 ),
m_grid_name(0),
m_picking_name( 0 )
{
    define_methodname() ;

    m_dialog = make() ;

    if ( !m_dialog )
    {
        throw std::runtime_error( __methodname__ + ": Error. Failed to create or register dialog in toolkit." ) ;
    }

    SetDialogData( m_dialog->id, "this", this ) ;
    DialogButtonSetPixmap( m_dialog->id, VERSION_NAME_BOX,
                          VERSION_NAME_SELECT_BTN_ITEM,
                          VERSION_NAME_SELECT_BTN, "open_16px.png");
}

DataOpenDialog::~DataOpenDialog()
{
    if ( m_dialog )
    {
        DeleteDialogs( &m_dialog, 1 ) ;
        ReleaseDialog( m_dialog ) ;
    }
}

bool DataOpenDialog::open(Application * __app,
                          DataStorage& __data_storage,
                          PickingStorage& __picking_storage,
                          std::string& __grid_name,
                          std::string& __picking_name)
{
    define_methodname() ;

    if  ( getId() < 0 )
        return true ;

    m_app = __app ;
    m_data_storage = &__data_storage ;
    m_picking_storage = &__picking_storage ;
    m_grid_name = &__grid_name ;
    m_picking_name = &__picking_name ;
    m_picking_name->clear() ;

    return (OpenImmediateResponseDialog(getId(), on_dialog_activate) == DIALOG_BUTTON_OK) ;
}

void DataOpenDialog::close()
{
    if  ( getId() < 0 )
        return ;

    m_app = 0 ;
    m_data_storage = 0 ;
    m_picking_storage = 0 ;
    m_grid_name = 0 ;
    m_picking_name = 0 ;

    CloseDialog( getId() ) ;
}

int DataOpenDialog::getId() const
{
    if  ( !m_dialog )
        return -1 ;

    return m_dialog->id ;
}

void DataOpenDialog::update()
{
    if ( getId() < 0 )
        return ;

    std::string grid_name = *m_grid_name ;
    DialogSetShowItem( getId(), GRID_NAME_BOX, GRID_NAME_ITEM, grid_name.empty() ) ;

    if ( grid_name.empty() )
    {
        DialogSetComboList( getId(), GRID_NAME_BOX, GRID_NAME_ITEM, getGridsList() ) ;
        DialogSetComboActiveItem( getId(), GRID_NAME_BOX, GRID_NAME_ITEM, 0 ) ;
        grid_name = DialogGetComboActiveItemString( getId(), GRID_NAME_BOX, GRID_NAME_ITEM ) ;
    }

    DialogSetComboList( getId(), PICKING_NAME_BOX, PICKING_NAME_ITEM, m_names.listVerticalSpectraPicking3dVersions(grid_name) ) ;

    std::string message ;
    hasVersionRequiredIndex(grid_name, getVersionName(), message) ;
    DialogSetLabel( getId(), VERSION_INFO_BOX, VERSION_INFO_ITEM, message.c_str() ) ;
}

bool DataOpenDialog::readAndCheckParameters()
{
    if ( getId() < 0 || !m_picking_name )
        return false ;

    // Getting and checking input data
    std::string version_name = getVersionName() ;
    *m_grid_name = getGridName() ;
    *m_picking_name = DialogGetComboActiveItemString( getId(), PICKING_NAME_BOX, PICKING_NAME_ITEM ) ;

    bool has_error = false ;
    std::ostringstream message ;

    if ( m_picking_name->empty() )
    {
        has_error = true ;
        message << (has_error ? "\n" : "") << "Picking name is undefined." ;
    }

    if ( m_grid_name->empty() )
    {
        has_error = true ;
        message << (has_error ? "\n" : "") << "Grid name is undefined." ;
    }

    if ( version_name.empty() )
    {
        has_error = true ;
        message << (has_error ? "\n" : "") << "No valid input data was selected." ;
    }

    if ( has_error )
    {
        DisplayErrorDialog( "Error", message.str() ) ;
        return false ;
    }

    // Getting and checking required index existence
    std::string error ;
    if ( !hasVersionRequiredIndex(*m_grid_name, version_name, error) )
    {
        DisplayErrorDialog( "Error", error ) ;
        return false ;
    }

    return true ;
}

std::string DataOpenDialog::getGridName() const
{
    if ( !m_grid_name || m_grid_name->empty() )
        return DialogGetComboActiveItemString( getId(), GRID_NAME_BOX, GRID_NAME_ITEM ) ;

    return *m_grid_name ;
}

std::string DataOpenDialog::getVersionName() const
{
    return DialogGetTextValue( getId(), VERSION_NAME_BOX, VERSION_NAME_ITEM ) ;
}

void DataOpenDialog::setVersionName(const std::string& __version_name)
{
    DialogSetTextValue( getId(), VERSION_NAME_BOX, VERSION_NAME_ITEM, __version_name.c_str() ) ;
}

bool DataOpenDialog::loadData()
{
    if ( !Prime::lib::primeApp::getProjCD() )
        return false ;

    if ( !getApp() )
        return false ;

    if ( !m_data_storage || !m_picking_storage )
        return false ;

    if ( !m_grid_name || !m_picking_name )
        return false ;

    Prime::Files::Names names( *Prime::lib::primeApp::getProjCD() ) ;
    std::string picking_filename = names.verticalSpectraPicking3dName( *m_grid_name, *m_picking_name ) ;

    if (!m_picking_storage->load(picking_filename) &&
        !m_picking_storage->empty())
    {
        std::ostringstream message ;
        message << "Can't load picking file '" << picking_filename << "'. " << std::endl
                << "Do you want clear it and proceed?" ;

        if ( DisplayYesNoQuestionDialog("Picking loading error", message.str(), getId()) != 0 )
            return false ;

        m_picking_storage->clear() ;
    }

    //DialogSetShowItem( getId(), DATA_LOAD_PROGRESS_BOX, DATA_LOAD_PROGRESS_ITEM, true ) ;
    //getApp()->getTasksExecutor().bindProgressBar( getId(), DATA_LOAD_PROGRESS_BOX, DATA_LOAD_PROGRESS_ITEM ) ;

    DataOpenTask task( *m_data_storage, *m_grid_name, getVersionName() ) ;

    if ( !getApp()->getTasksExecutor().runTask(task) )
        return false ;

    //DialogSetShowItem( getId(), DATA_LOAD_PROGRESS_BOX, DATA_LOAD_PROGRESS_ITEM, false ) ;
    //getApp()->getTasksExecutor().resetProgressBar() ;

    if (!m_picking_storage->empty() &&
        m_picking_storage->getHodographType() != m_data_storage->getHodographType())
    {
        std::ostringstream message ;
        message << "Selected picking '" << *m_picking_name << "' has picks and its "
                << "type of analysis hodograph differs from hodograph type of input data." << std::endl
                << "Do you want clear it and proceed?" ;

        if ( DisplayYesNoQuestionDialog("Picking loading error", message.str(), getId()) != 0 )
            return false ;

        m_picking_storage->clear() ;
    }

    m_picking_storage->setHodographType( m_data_storage->getHodographType() ) ;

    return true ;
}

DIALOG * DataOpenDialog::make()
{
    define_methodname() ;

    //
    // PICKING_NAME_BOX
    //
    static COMBO_BOX picking_box = {
        "Picking", true, true, 32
    };

    //
    // Name of grid
    //
    static COMBO_BOX grid_name_box = {
        "Grid", false, true, 32, 0, 0, on_combobox_changed
    };

    //
    // VERSION_NAME_BOX
    //    
    static TEXT_AND_BUTTON_BOX version_name_box = {
        { "Version ", "", true, 0, 32, on_version_name_changed, on_version_name_changed, nullptr, on_version_name_changed },
        { BUTTON_BOX::END_LAYOUT, 1, nullptr, nullptr, 1, on_version_selection_button, false, false }
    };

    //
    // VERSION_INDEX_INFO_BOX
    //
    static LABEL_BOX version_info_box[] = {
        { "" },
        { 0 }
    };

    // 
    // Dialog assembling
    //
    static USER_DEFINED_DIALOG dialog_descrption[] = {
        { &picking_box,             0, COMBO_BOX_D,           true },
        { &grid_name_box,           0, COMBO_BOX_D,           true },
        { &version_name_box,        0, TEXT_AND_BUTTON_BOX_D, true },
        { version_info_box,         0, LABEL_BOX_D,           true },
        { 0 }
    };

    static DIALOG::DIALOG_BUTTON dialog_buttons[] = {
        { "Ok",     true,  false },
        { "Cancel", false, false },
        { 0 }
    };

    static DIALOG dialog_prototype = {
        dialog_descrption, "Open vertical spectra analysis", -1,
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

const std::vector<std::string>& DataOpenDialog::getGridsList()
{
    using namespace Prime::lib::primeApp;

    if ( !m_grids_list.empty() )
        return m_grids_list ;

    // Loading information from database
    std::vector<Prime::DB::Grid> dbGrids;

    try
    {
        Prime::DB::Grid::select(getProjDB(), Prime::DB::Grid::Key(), dbGrids);
    }
    catch (const Prime::DB::Exception &err)
    {
        dbGrids.clear();
        std::cerr << "Error loading Grids list from project db: " << err.what() << std::endl;
    }

    for (auto const &it : dbGrids)
    {
        m_grids_list.push_back(it.key().name) ;
    }

    return m_grids_list ;
}

bool DataOpenDialog::hasVersionRequiredIndex(const std::string& __grid_name,
                                             const std::string& __version_name,
                                             std::string& __message) const

{
    __message.clear() ;

    Prime::Metacube::MetacubeDescription info(Prime::SEISMIC_PRESTACK, __grid_name, __version_name,
                                              Prime::lib::primeApp::getProjCD());

    for ( auto const &index_name : info.GetIndexesNames() )
    {
        Prime::Metacube::MetacubeSorting sorting(index_name) ;

        if ( sorting.GetIndexName() == DataStorage::DEF_REQUIRED_INDEX.GetIndexName() )
            return true ;
    }

    std::ostringstream message ;
    message << "<i>Version has to contain index '" << DataStorage::DEF_REQUIRED_INDEX.GetIndexName() << "'.</i>" ;
    __message = message.str() ;

    return false ;
}

int DataOpenDialog::on_dialog_init(int __dialog)
{
    DataOpenDialog * __this = static_cast<DataOpenDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !__this )
        return 0 ;

    __this->update() ;

    return 1 ;
}

bool DataOpenDialog::on_dialog_activate(int __dialog, int __button)
{
    DataOpenDialog * _this = static_cast<DataOpenDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !_this )
        return false ;

    switch ( __button )
    {
        case DIALOG_BUTTON_OK:
            if (_this->readAndCheckParameters())
                return _this->loadData() ;
            return false ;

        case DIALOG_BUTTON_CLOSE_X:
        case DIALOG_BUTTON_CLOSE:
            _this->close() ;
            return true ;
    }

    return true ;
}

void DataOpenDialog::on_combobox_changed( int __dialog, int __box, int __item, const char * __sel_str, int __sel_item )
{
    DataOpenDialog * __this = static_cast<DataOpenDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !__this )
        return ;

    __this->update() ;
}

void DataOpenDialog::on_version_name_changed( int __dialog, int __box, int __item )
{
    DataOpenDialog * __this = static_cast<DataOpenDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !__this )
        return ;

    __this->update() ;
}

void DataOpenDialog::on_version_selection_button( int __dialog, int __box, int __item, int __button, int __subbutton, bool __state )
{
    DataOpenDialog * __this = static_cast<DataOpenDialog*> ( GetDialogData(__dialog, "this") ) ;

    if ( !__this )
        return ;

    using namespace Prime::lib::primeApp ;
    VersionSel version_selector(getProjDB(), getProjCD()) ;
    Prime::DataCategory category = Prime::SEISMIC_PRESTACK ;
    std::string survey_name = __this->getGridName() ;
    std::string version_name = __this->getVersionName() ;
    bool isNew = false ;

    if (version_selector.getVersion(category, survey_name, VersionSel::ANY_SEGMENTS,
                                    "Select version name", version_name, isNew,
									VersionSel::BLOCK_CATEGORY | VersionSel::BLOCK_SURVEY,
                                    __dialog))
    {
        __this->setVersionName( version_name ) ;
        __this->update() ;
    }
}

}
}
}
