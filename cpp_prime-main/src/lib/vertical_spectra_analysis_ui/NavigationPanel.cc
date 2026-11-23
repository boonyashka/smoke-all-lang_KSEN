
#include "Application.h"
#include "WindowsManager.h"
#include "NavigationPanel.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

NavigationPanel::NavigationPanel( Application * __app, int __dialog, int __box ) :
m_app(__app),
m_dialog(__dialog),
m_box(__box)
{
    init() ;
}

NavigationPanel::~NavigationPanel() { }

void NavigationPanel::update()
{
    if ( !m_app || m_box < 0 )
        return ;

    const int points_num = m_app->getDataPointsNum() ;
    const int point_index = m_app->getDataPointIndex() ;
    const auto * data_point = m_app->getDataPoint() ;
    const int point_ncdp = (data_point ? data_point->ncdp : 0) ;
    
    DialogSetShowItem(m_dialog, m_box, ANALYSIS_ICON_ITEM, m_app->getPickingPoint() != 0) ;
    
    if ( !isScaleBlocked() )
        DialogSetExtScaleValue(m_dialog, m_box, NAV_SCALE_ITEM, point_index + 1, 0, 1, points_num ) ;
    
    DialogSetTextValueAsInt(m_dialog, m_box, ANALYSIS_POINT_ITEM, point_ncdp) ;

    const bool bind = usePickingPointsBinding() ;
    DialogButtonSetPixmap(m_dialog, m_box, NAV_BUTTONS_ITEM, NAV_BIND_BTN, (bind ? "checkbox_on_hover_16px.png" : "checkbox_off_16px.png")) ;
    DialogSetShowItem(m_dialog, m_box, ANALYSIS_STEP_ITEM, !bind) ;
}

void NavigationPanel::init()
{
    if ( !m_app || m_box < 0 )
        return ;

    // Setup icons and tooltips of buttons
    DialogButtonSetPixmap(m_dialog, m_box, ANALYSIS_ICON_ITEM, 0, "chart_16px.png") ;
    DialogButtonSetPixmap(m_dialog, m_box, NAV_BUTTONS_ITEM, NAV_PREV_BTN, "arrow-left_16px.png") ;
    DialogButtonSetPixmap(m_dialog, m_box, NAV_BUTTONS_ITEM, NAV_NEXT_BTN, "arrow-right_16px.png") ;
    DialogButtonSetPixmap(m_dialog, m_box, NAV_BUTTONS_ITEM, NAV_BIND_BTN, "checkbox_off_16px.png") ;
    DialogSetItemTooltip(m_dialog, m_box, NAV_BUTTONS_ITEM, NAV_BIND_BTN, "Visit only picking points") ;

    // Setup link to myself
    SetDialogBoxData(m_dialog, m_box, "NavigationPanel", this) ;
}

void NavigationPanel::setAnalysisPointFromScaleBox()
{
    if ( !m_app || m_box < 0 )
        return ;

    int point_index = 0 ;
    DialogGetScaleValueAsInt( m_dialog, m_box, NAV_SCALE_ITEM, &point_index ) ;

    if ( usePickingPointsBinding() )
    {
        const ObjectID object_id = m_app->getAnalysisObjectID() ;
        const auto& point = m_app->getDataStorage().getDataPointByIndex(object_id, point_index - 1) ;
        const auto* picking_point = m_app->getPickingStorage().findClosestPoint(object_id, point.ncdp) ;

        if ( picking_point )
        {
            point_index = m_app->getDataStorage().findNearestDataPointByNcdp(object_id, picking_point->ncdp()) ;
            m_app->setDataPoint( object_id, point_index ) ;
        }
        else
        {
            return update() ;
        }
    }
    else
    {
        m_app->setDataPoint( m_app->getAnalysisObjectID(), point_index - 1 ) ;
    }
}

void NavigationPanel::setAnalysisPointFromTextBox()
{
    if ( !m_app || m_box < 0 )
        return ;

    const ObjectID object_id = m_app->getAnalysisObjectID() ;
    const int point_ncdp = DialogGetTextValueAsInt(m_dialog, m_box, ANALYSIS_POINT_ITEM) ;

    if ( usePickingPointsBinding() )
    {
        const auto* picking_point = m_app->getPickingStorage().findClosestPoint(object_id, point_ncdp) ;

        if ( picking_point )
        {
            const int point_index = m_app->getDataStorage().findNearestDataPointByNcdp(object_id, picking_point->ncdp()) ;
            m_app->setDataPoint( object_id, point_index ) ;
        }
        else
        {
            return update() ;
        }
    }
    else
    {
        const int point_index = m_app->getDataStorage().findNearestDataPointByNcdp(object_id, point_ncdp) ;
        m_app->setDataPoint( object_id, point_index ) ;
    }
}

void NavigationPanel::shiftAnalysisPoint( int __direction )
{
    if ( !m_app || m_box < 0 )
        return ;

    if ( usePickingPointsBinding() )
    {
        if ( m_app->getDataPoint() )
        {
            const ObjectID object_id = m_app->getAnalysisObjectID() ;
            const int point_ncdp = m_app->getDataPoint()->ncdp ;
            const auto* picking_point = (__direction < 0
                                         ? m_app->getPickingStorage().findLeftClosestPoint(object_id, point_ncdp)
                                         : m_app->getPickingStorage().findRightClosestPoint(object_id, point_ncdp)) ;

            if ( picking_point )
            {
                const int point_index = m_app->getDataStorage().findNearestDataPointByNcdp(object_id, picking_point->ncdp()) ;
                m_app->setDataPoint( object_id, point_index ) ;
            }
            else
            {
                return update() ;
            }
        }
    }
    else
    {
        int shift ;
        DialogGetStepperValueAsInt( m_dialog, m_box, ANALYSIS_STEP_ITEM, &shift ) ;

        const int point_index = m_app->getDataPointIndex() ;
        m_app->setDataPoint( m_app->getAnalysisObjectID(), point_index + __direction * shift ) ;
    }
}

bool NavigationPanel::usePickingPointsBinding() const
{
    if ( m_box < 0 )
        return false ;

    return DialogButtonGetState(m_dialog, m_box, NAV_BUTTONS_ITEM, NAV_BIND_BTN) ;
}

MULTI_BOX * NavigationPanel::make()
{
    static BUTTON_BOX analysis_icon_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 1, NULL, 0, 1, 0, true, true
    };
    
    static SCALE_BOX scale_box[] = {
        { "", true, 1, 1, 100, 0, on_nav_scale_changed },
        { 0 }
    };

    static BUTTON_BOX::Type buttons_types[] = {
        BUTTON_BOX::PUSH_BUTTON, BUTTON_BOX::PUSH_BUTTON, BUTTON_BOX::TOGGLE_BUTTON
    };

    static BUTTON_BOX next_prev_bind_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 3, NULL, buttons_types, 1, on_nav_button_pressed, true, true
    };

    static TEXT_BOX analysis_point_box[] = {
        { "", "1", true, 8, 8, nullptr, on_analysis_point_changed },
        { 0 }
    };

    static STEPPER_BOX analysis_step_box[] = {
        { "Step", 3, 40, 1, 10000, 1, 0, on_analysis_step_changed },
        { 0 }
    };

    static MULTI_BOX box_prototype[] = {
        { &analysis_icon_box,  BUTTON_BOX_D  },
        { scale_box,           SCALE_BOX_D   },
        { &next_prev_bind_box, BUTTON_BOX_D  },
        { analysis_point_box,  TEXT_BOX_D    },
        { analysis_step_box,   STEPPER_BOX_D },
        { 0 }
    };

    return box_prototype ;
}

DIALOG::BOX_PACKING* NavigationPanel::make_packing()
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX   },
        { 1, PROPORTIONAL_BOX },
        { 1, FIXED_SIZE_BOX   },
        { 1, FIXED_SIZE_BOX   },
        { 1, FIXED_SIZE_BOX   },
        { 0 }
    };

    static DIALOG::BOX_PACKING packing = {
        VERTICAL_PACKING, pack_line
    };

    return &packing ;
}

void NavigationPanel::on_nav_scale_changed(int __dialog, int __box, int __scale, double __value, int __decimal_point)
{
    NavigationPanel * __this = static_cast<NavigationPanel*> ( GetDialogBoxData(__dialog, __box, "NavigationPanel") ) ;

    if ( !__this )
        return ;

    __this->setScaleBlocking( true ) ;
    __this->setAnalysisPointFromScaleBox() ;
    __this->setScaleBlocking( false ) ;
}

void NavigationPanel::on_nav_button_pressed(int __dialog, int __box, int __item, int __button, int __subbutton, bool __state)
{
    NavigationPanel * __this = static_cast<NavigationPanel*> ( GetDialogBoxData(__dialog, __box, "NavigationPanel") ) ;

    if ( !__this )
        return ;

    switch ( __button )
    {
        case NAV_PREV_BTN:
            __this->shiftAnalysisPoint( -1 ) ;
            break ;

        case NAV_NEXT_BTN:
            __this->shiftAnalysisPoint( +1 ) ;
            break ;
            
        case NAV_BIND_BTN:
            __this->update() ;
            break ;
    }
}

void NavigationPanel::on_analysis_point_changed(int __dialog, int __box, int __button)
{
    NavigationPanel * __this = static_cast<NavigationPanel*> ( GetDialogBoxData(__dialog, __box, "NavigationPanel") ) ;

    if ( !__this )
        return ;

    __this->setAnalysisPointFromTextBox() ;
}

void NavigationPanel::on_analysis_step_changed(int __dialog, int __box, int __item, double __value)
{
    NavigationPanel * __this = static_cast<NavigationPanel*> ( GetDialogBoxData(__dialog, __box, "NavigationPanel") ) ;

    if ( !__this )
        return ;
}

}
}
}