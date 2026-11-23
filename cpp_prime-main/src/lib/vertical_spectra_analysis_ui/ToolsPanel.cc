
#include "ToolsPanel.h"
#include "Application.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

ToolsPanel::ToolsPanel( Application * __app, int __dialog, int __box, int __items_beg ) :
m_app(__app),
m_dialog(__dialog),
m_box(__box),
m_items_beg(__items_beg)
{
    init() ;
}

ToolsPanel::~ToolsPanel() { }

void ToolsPanel::update()
{
    if ( !getApp() || m_box < 0 )
        return ;

    DialogButtonSetStateWithoutNotify(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_BTN,
                                      getApp()->getWindowsManager().isZoomSwitchedOn()) ;

    DialogSetEnableItem(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_UNDO_BTN,
                        getApp()->getWindowsManager().canUndoZoom()) ;

    DialogSetEnableItem(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_REDO_BTN,
                        getApp()->getWindowsManager().canRedoZoom()) ;

    if ( getApp()->getObjectsListPanel() )
        DialogButtonSetStateWithoutNotify(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_OBJECTS_LIST_BTN,
                                          !getApp()->getObjectsListPanel()->isHidden()) ;
}

void ToolsPanel::init()
{
    if ( !m_app || m_box < 0 )
        return ;

    // Zoom in
    DialogButtonSetPixmap(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_BTN, "zoom.png") ;
    DialogSetItemTooltip(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_BTN, "Zoom in") ;

    // Previous zoom
    DialogButtonSetPixmap(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_UNDO_BTN, "zoom-minus.png") ;
    DialogSetItemTooltip(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_UNDO_BTN, "Previous zoom") ;

    // Next zoom
    DialogButtonSetPixmap(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_REDO_BTN, "zoom-plus.png") ;
    DialogSetItemTooltip(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_REDO_BTN, "Next zoom") ;

    // View all
    DialogButtonSetPixmap(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_VIEWALL_BTN, "viewall.png") ;
    DialogSetItemTooltip(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_VIEWALL_BTN, "View all") ;

    DialogButtonSetPixmap(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_VIEWALL_BTN, "viewall.png") ;
    DialogSetItemTooltip(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_ZOOM_VIEWALL_BTN, "View all") ;

    // Right panel
    DialogButtonSetPixmap(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_OBJECTS_LIST_BTN, "left-panel.png") ;
    DialogSetItemTooltip(m_dialog, m_box, m_items_beg + TOOLS_ITEM, TOOLS_OBJECTS_LIST_BTN, "Display/hide objects list") ;

    // Setup link to myself
    SetDialogBoxData(m_dialog, m_box, "ToolsPanel", this) ;
}

BUTTON_BOX* ToolsPanel::make()
{
    static BUTTON_BOX::Type buttons_types[] = {
        BUTTON_BOX::TOGGLE_BUTTON,
        BUTTON_BOX::PUSH_BUTTON,
        BUTTON_BOX::PUSH_BUTTON,
        BUTTON_BOX::PUSH_BUTTON,
        BUTTON_BOX::TOGGLE_BUTTON
    };

    static BUTTON_BOX buttons_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 5, NULL, buttons_types, 5, buttons_proc, false, true
    };

//    static MULTI_BOX box_prototype[] = {
//        { &buttons_box, BUTTON_BOX_D },
//        { 0 }
//    };
//
//    return box_prototype ;
    
    return &buttons_box ;
}

//DIALOG::BOX_PACKING* ToolsPanel::make_packing()
//{
//    static DIALOG::BOX_PACKING::LINE pack_line[] = {
//        { 1, FIXED_SIZE_BOX },
//        { 0 }
//    };
//
//    static DIALOG::BOX_PACKING packing = {
//        HORIZONTAL_PACKING, pack_line
//    };
//
//    return &packing ;
//}

void ToolsPanel::buttons_proc(int __dialog, int __box, int __item, int __button, int __subbutton, bool __state)
{
    ToolsPanel * _this = static_cast<ToolsPanel*> ( GetDialogBoxData(__dialog, __box, "ToolsPanel") ) ;

    if ( !_this || !_this->getApp() )
        return ;

    switch ( __button )
    {
        case TOOLS_ZOOM_BTN:
            _this->getApp()->getWindowsManager().switchZoom( __state ) ;
            _this->update() ;
            break ;

        case TOOLS_ZOOM_UNDO_BTN:
            _this->getApp()->getWindowsManager().undoZoom() ;
            _this->update() ;
            break ;

        case TOOLS_ZOOM_REDO_BTN:
            _this->getApp()->getWindowsManager().redoZoom() ;
            _this->update() ;
            break ;

        case TOOLS_ZOOM_VIEWALL_BTN:
            _this->getApp()->getWindowsManager().setViewAllZoom() ;
            _this->update() ;
            break ;

        case TOOLS_OBJECTS_LIST_BTN:
            _this->getApp()->getObjectsListPanel()->hide( !__state ) ;
            break ;
    }
}

}
}
}