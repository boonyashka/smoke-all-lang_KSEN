
#include "ObjectsListPanel.h"
#include "Application.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

ObjectsListPanel::ObjectsListPanel(Application * __app, int __dialog, int __box, int __items_beg) :
m_app(__app),
m_dialog(__dialog),
m_box(__box),
m_items_beg(__items_beg)
{
    hide( false ) ;

    SetDialogBoxData( m_dialog, m_box, "ObjectsListPanel", this ) ;
}

ObjectsListPanel::~ObjectsListPanel() { }

void ObjectsListPanel::update()
{
    if ( !m_app || m_box < 0 )
        return ;

    const auto& objects_names = m_app->getDataStorage().getObjectsNames() ;

    if ( isHidden() )
        return ;

    const int prev_storage_size = m_storage.size() ;
    m_storage.resize( objects_names.size() ) ;

    for ( int i = 0; i < objects_names.size(); ++i )
    {
        CONTAINER_ITEM &item = m_storage.at(i) ;
        item.parentItem = -1 ;
        item.columns.resize(TABLE_COLUMNS_NUM) ;

        for ( int icolumn = 0; icolumn < TABLE_COLUMNS_NUM; ++icolumn )
        {
            CONTAINER_ITEM_ELEMENT &element = item.columns[icolumn] ;

            switch ( icolumn )
            {
                case TABLE_NAME_COLUMN:
                    element.type = CONTAINER_ITEM_ELEMENT::LABEL_ELEMENT ;
                    element.label = objects_names[i] ;
                    break;
            }
        }
    }

    if ( m_storage.size() != prev_storage_size || prev_storage_size == 0 )
    {
        DialogSetContainerStorage( m_dialog, m_box, m_items_beg + TABLE_ITEM, m_storage ) ;
    }
    else
    {
        DialogContainerStorageUpdateAll( m_dialog, m_box, m_items_beg + TABLE_ITEM ) ;
    }

    if ( m_app->getAnalysisObjectIndex() >= 0 )
    {
        std::vector<int> selected_objects( 1, m_app->getAnalysisObjectIndex() ) ;
        DialogSetContainerActiveItems( m_dialog, m_box, m_items_beg + TABLE_ITEM, selected_objects ) ;
    }
    
    DialogAdjustContainerWidth( m_dialog, m_box, m_items_beg + TABLE_ITEM, 150 ) ;
}

void ObjectsListPanel::hide( bool __hide )
{
    DialogSetShowItem( m_dialog, m_box, m_items_beg + TABLE_ITEM, !__hide ) ;
    update() ;
}

bool ObjectsListPanel::isHidden() const
{
    return !DialogGetShowBox( m_dialog, m_box ) ;
}

void ObjectsListPanel::loadAnalysisObject( int __object_index )
{
    if ( !m_app || m_box < 0 )
        return ;

    m_app->loadAnalysisObjectByIndex( __object_index ) ;
    std::vector<int> selected_objects( 1, m_app->getAnalysisObjectIndex() ) ;
    DialogSetContainerActiveItems( m_dialog, m_box, m_items_beg + TABLE_ITEM, selected_objects ) ;
}

CONTAINER_BOX* ObjectsListPanel::make()
{
    static CONTAINER_COLUMN table_columns[] = {
        { " Analysis objects ", CONTAINER_ITEM_ELEMENT::LABEL_ELEMENT  },
        { NULL }
    };

    static CONTAINER_BOX table_prototype = {
        CONTAINER_BOX::TREE_STORE, table_columns, 15, 0, table_activate_proc, 0, 0
    };

    return &table_prototype ;
}

void ObjectsListPanel::table_activate_proc(int __dialog, int __box, int __item, const std::vector<int>& __items)
{
    ObjectsListPanel * __this = static_cast<ObjectsListPanel*> ( GetDialogBoxData(__dialog, __box, "ObjectsListPanel") ) ;

    if (__this && __items.size() == 1)
        __this->loadAnalysisObject( __items.front() ) ;
}

}
}
}