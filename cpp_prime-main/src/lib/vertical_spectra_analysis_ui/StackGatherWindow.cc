
#include "StackGatherWindow.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

WINDOW_BOX * StackGatherWindow::make()
{
    static MENU::ITEM window_menu_items[] = {
        { "Add picking",    'A', 0, 0, NOTHING, ORDINARY_M,  0, 0, popup_add_picking_menu, popup_add_picking_menu_map },
        { "Delete picking", 'D', 0, 0, NOTHING, ORDINARY_M,  0, 0, popup_del_picking_menu, popup_del_picking_menu_map },
        { "",              '\0', 0, 0, NOTHING, SEPARATOR_M                                          },
        { "Level...",      'L',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_window_level_menu },
        { "Adjust level",  'A',  0, 0, NOTHING, ORDINARY_M,  0, 0, popup_window_adjust_level_menu },
        { 0 }
    };

    static MENU window_menu[] = {
        { window_menu_items },
        { 0 }
    };

    static WINDOW_BOX window_prototype = {
        "Stack-gather",
        -1,
        WINDOW::APPL_SCROLL_WINDOW,
        configure,
        expose,
        expose_interactive,
        print,
        mouse,
        nullptr,
        NULL,
        200,
        200,
        true,
        true,
        true,
        true,
        scroll,
        expose_scale,
        window_menu,
        0, //map_delta,
        WINDOW::GRAPHICS_2D
    };

    return &window_prototype ;
}

}
}
}
