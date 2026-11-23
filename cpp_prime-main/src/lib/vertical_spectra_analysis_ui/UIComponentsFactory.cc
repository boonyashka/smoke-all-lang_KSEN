
#include "rawWindow/RawWindow.h"

#include "TopMenuPanel.h"
#include "NavigationPanel.h"
#include "ToolsPanel.h"
#include "ObjectsListPanel.h"
#include "UIComponentsFactory.h"
#include "Application.h"
#include "SpectrumWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

MENU * UIComponentsFactory::get_main_menu_prototype()
{
    return TopMenuPanel::make() ;
}

MULTI_BOX * UIComponentsFactory::get_navigation_panel_prototype()
{
    return NavigationPanel::make() ;
}

DIALOG::BOX_PACKING* UIComponentsFactory::get_navigation_panel_packing()
{
    return NavigationPanel::make_packing() ;
}

BUTTON_BOX * UIComponentsFactory::get_tools_panel_prototype()
{
    return ToolsPanel::make() ;
}

CONTAINER_BOX * UIComponentsFactory::get_objects_list_prototype()
{
    return ObjectsListPanel::make() ;
}

WINDOW_BOX * UIComponentsFactory::get_gather_window_prototype()
{
    return SuperGatherWindow::make() ;
}

WINDOW_BOX * UIComponentsFactory::get_spectrum_window_prototype()
{
    return SpectrumWindow::make() ;
}

WINDOW_BOX * UIComponentsFactory::get_stackgather_window_prototype()
{
    return StackGatherWindow::make() ;
}

}
}
}