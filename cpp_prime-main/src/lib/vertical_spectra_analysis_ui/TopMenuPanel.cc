
#include "Application.h"
#include "TopMenuPanel.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

TopMenuPanel::TopMenuPanel(Application * __app, int __dialog) :
m_app(__app)
{
    SetDialogData( __dialog, "TopMenuPanel", this ) ;
}

TopMenuPanel::~TopMenuPanel() {}

MENU* TopMenuPanel::make()
{
    static MENU::ITEM menu_items[] = {
        { "Picking", 'P', 0, 0, 1, ORDINARY_M },
        { "View",    'V', 0, 0, 2, ORDINARY_M },
        { 0 }
    };

    static MENU::ITEM menu_picking_items[] = {
        { "New",                'N',  0, 0, NOTHING, ORDINARY_M, 0, 0, picking_new_menu     },
        { "Load...",            'L',  0, 0, NOTHING, ORDINARY_M, 0, 0, picking_load_menu    },
        { "Save",               'S',  0, 0, NOTHING, ORDINARY_M, 0, 0, picking_save_menu, picking_save_menu_map },
        { "Save as...",         'S',  0, 0, NOTHING, ORDINARY_M, 0, 0, picking_save_as_menu },
        { "Import...",          'I',  0, 0, NOTHING, ORDINARY_M, 0, 0, picking_import_menu, picking_import_menu_map },
        { "Export...",          'E',  0, 0, NOTHING, ORDINARY_M, 0, 0, picking_export_menu, picking_export_menu_map },
        { "Autosave",           'A',  0, 0, NOTHING, TOGGLE_M,   0, 0, picking_autosave_menu, picking_autosave_menu_map },
        //{ "Delete",             'D',  0, 0, NOTHING, ORDINARY_M,  0, 0}, //spectrum_dialog_menu_picking_delete, spectrum_dialog_menu_picking_delete_map },
        
        { "",                           '\0', 0, 0, NOTHING, SEPARATOR_M },
        { "Add picking point",          'A',  0, 0, NOTHING, ORDINARY_M,  0, 0, picking_add_picking_menu, picking_add_picking_menu_map },
        { "Add picking points...",      'A',  0, 0, NOTHING, ORDINARY_M,  0, 0, picking_add_picking_points_menu },
        { "Delete picking point",       'D',  0, 0, NOTHING, ORDINARY_M,  0, 0, picking_del_picking_menu, picking_del_picking_menu_map },
        
        //{ "",                   '\0', 0, 0, NOTHING, SEPARATOR_M                                            },
        //{ "Close",              'C',  0, 0, NOTHING, ORDINARY_M,  0, 0, close_menu  },
        { 0 }
    };

    static MENU::ITEM menu_view_items[] = {
        { "Levels..",              'L',  0, 0, NOTHING, ORDINARY_M,  0, 0, view_levels_menu      },
        { "Preferences...",        'P',  0, 0, NOTHING, ORDINARY_M,  0, 0, view_preferences_menu },
        { "",                     '\0',  0, 0, NOTHING, SEPARATOR_M                              },
        { "Cursor tracking lines", 'C',  0, 0, NOTHING, TOGGLE_M,   0, 0, cursor_tracking_menu, cursor_tracking_menu_map },
        { 0 }
    };

    static MENU menu_prototype[] = {
        { menu_items         },
        { menu_picking_items },
        { menu_view_items    },
        { 0 }
    };
    
    return menu_prototype ;
}

void TopMenuPanel::picking_new_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->newPicking() ;
}

void TopMenuPanel::picking_load_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->loadPicking() ;
}

void TopMenuPanel::picking_save_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->savePicking() ;
}

void TopMenuPanel::picking_save_menu_map(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    SetMenuItemEnabling( __details, _this->getApp()->getPickingStorage().needsSaving() ) ;
}

void TopMenuPanel::picking_save_as_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->savePickingAs() ;
}

void TopMenuPanel::picking_import_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->importPicking() ;
}

void TopMenuPanel::picking_import_menu_map(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    SetMenuItemEnabling( __details, !_this->getApp()->getPickingStorage().empty() ) ;
}

void TopMenuPanel::picking_export_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->exportPicking() ;
}

void TopMenuPanel::picking_export_menu_map(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    SetMenuItemEnabling( __details, !_this->getApp()->getPickingStorage().empty() ) ;
}

void TopMenuPanel::picking_autosave_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    bool autosave = GetMenuToggleButtonState( __details ) ;
    _this->getApp()->setPickingAutoSave( autosave ) ;
}

void TopMenuPanel::picking_autosave_menu_map(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    SetMenuToggleButtonState( __details, _this->getApp()->getPickingAutoSave() ) ;
}

void TopMenuPanel::picking_add_picking_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->addCurrentPointToPicking() ;
}

void TopMenuPanel::picking_add_picking_menu_map(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    SetMenuItemEnabling( __details, _this->getApp()->getPickingPoint() == nullptr ) ;
}

void TopMenuPanel::picking_add_picking_points_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->addPickingPoints() ;
}

void TopMenuPanel::picking_del_picking_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    _this->getApp()->deleteCurrentPickingPoint() ;
}

void TopMenuPanel::picking_del_picking_menu_map(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    SetMenuItemEnabling( __details, _this->getApp()->getPickingPoint() != nullptr ) ;
}

void TopMenuPanel::view_levels_menu(const MenuItemDetails& __details)
{    
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    if (_this->getApp()->getLevelsDialog())
        _this->getApp()->getLevelsDialog()->show() ;
}

void TopMenuPanel::view_preferences_menu(const MenuItemDetails& __details)
{    
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    if (_this->getApp()->getPreferencesDialog())
        _this->getApp()->getPreferencesDialog()->show() ;
}

void TopMenuPanel::cursor_tracking_menu(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    bool use = GetMenuToggleButtonState( __details ) ;
    _this->getApp()->setCursorTrackingLinesUsage( use ) ;
}

void TopMenuPanel::cursor_tracking_menu_map(const MenuItemDetails& __details)
{
    TopMenuPanel * _this = static_cast<TopMenuPanel*>(GetDialogData(__details.postedFrom.id, "TopMenuPanel")) ;
    
    if ( !_this || !_this->getApp() )
        return ;
    
    SetMenuToggleButtonState( __details, _this->getApp()->getCursorTrackingLinesUsage() ) ;
}

}
}
}