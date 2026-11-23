
#include <memory>

#include "primeApp/PrimeAppNoX.h"
#include "utils/methodname.h"
#include "data3d/model.h"
#include "Horizons/FRHorizonsTypes.h"
#include "data3d/surface_math.h"
#include "mathlib/algorithms/sorting.h"

#include "PickingImportTask.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

PickingImportFromModel3dTask::PickingImportFromModel3dTask(PickingStorage& __picking_storage,
                                                           const std::string& __grid_name,
                                                           const BasicModel3d * __input_model,
                                                           const std::string& __input_name,
                                                           const std::string& __input_attribute_name) :
m_picking_storage(__picking_storage),
m_grid_name(__grid_name),
m_input_model(__input_model),
m_input_name(__input_name),
m_input_attribute_name(__input_attribute_name) { }

PickingImportFromModel3dTask::~PickingImportFromModel3dTask() { }

const std::string& PickingImportFromModel3dTask::name() const
{
    static const std::string name = "Importing from model" ;
    return name ;
}

bool PickingImportFromModel3dTask::run()
{
    using namespace Prime::lib::primeApp ;
    using namespace Prime::lib::mathlib ;

    // Getting domain name of output/frame model
    using namespace Prime::lib::Nmo ;
    auto hodograph_type = m_picking_storage.getHodographType() ;
    bool is_time_domain = (HodographType::get_domain_type(hodograph_type) == FR_IO_TIME) ;
    std::string domain_name = HodographType::get_domain_name( hodograph_type ) ;
    std::string subdomain_name = HodographType::get_domain_attribute_name( hodograph_type ) ;
    const double domain_scale = (is_time_domain ? 0.001 : 1.0) ;

    // Opening input model grid
    std::unique_ptr<Grid3d> grid( new Grid3d(getProjDB(), m_grid_name) ) ;

    if ( !grid )
    {
        setErrorDescription( "Can't open grid of input model." ) ;
        return false ;
    }

    // Preparation input model
    const BasicModel3d * p_input_model = m_input_model ;
    std::unique_ptr<BasicModel3d> input_model ;
    
    if ( !p_input_model )
    {
        input_model.reset( new BasicModel3d(getProjDB(), 0, grid.get(), domain_name, m_input_name) ) ;
        p_input_model = input_model.get() ;
        
        if ( !p_input_model )
        {
            setErrorDescription( "Can't open input model." ) ;
            return false ;
        }
    }

    // Preparation links to input surfaces
    std::vector<const SurfaceUChar *> input_attr_list ;
    std::vector<const SurfaceFloat *> input_arg_surf_list ; // Time or depth surfaces
    std::vector<const SurfaceFloat *> input_val_surf_list ; // Input parameter surfaces

    const auto input_horizons_names = p_input_model->listHorizonNames() ;

    for ( const auto& horizon_name : input_horizons_names )
    {
        const Horizon3d * horizon = p_input_model->getHorizon(horizon_name) ;

        if ( !horizon )
            continue ;

        input_attr_list.push_back( &horizon->getAttributes() ) ;
        input_arg_surf_list.push_back( horizon->getSurface(subdomain_name) ) ;
        input_val_surf_list.push_back( horizon->getSurface(m_input_attribute_name) ) ;

        if (!input_attr_list.back() ||
            !input_arg_surf_list.back() ||
            !input_val_surf_list.back())
            continue ;
    }

    // Checking input model horizons
    if ( input_attr_list.empty() )
    {
        setErrorDescription( "Input model is empty." ) ;
        return false ;
    }

    // Processing of all picking points
    size_t picking_points_total_num = 0 ;
    size_t picking_points_processed_num = 0 ;

    for ( const auto& object : m_picking_storage.storage() )
    {
        picking_points_total_num += (object.second ? object.second->size() : 0) ;
    }

    for ( auto& object : m_picking_storage.storage() )
    {
        if ( !object.second )
            continue ;

        for ( auto& point : *object.second )
        {
            picking_points_processed_num++ ;

            point.curve().clear() ;

            //Getting point location
            double Px, Py ;
            grid->glob2loc( point.x(), point.y(), &Px, &Py  ) ;

            // Getting values
            for ( int i = 0; i < input_attr_list.size(); ++i )
            {
                const SurfaceUChar * attr = input_attr_list[i] ;
                const SurfaceFloat * t_surf = input_arg_surf_list[i] ;
                const SurfaceFloat * v_surf = input_val_surf_list[i] ;

                if ( !attr || !t_surf || !v_surf )
                    continue ;

                float t_value, v_value ;

                if (!surf_get_interpolated_value(*t_surf, *attr, Px, Py, &t_value) ||
                    !surf_get_interpolated_value(*v_surf, *attr, Px, Py, &v_value))
                    continue ;

                point.curve().push_back( Prime::Geom::coord_2_dbl(v_value, domain_scale * t_value) ) ;
            }

            // Sorting points
            std::sort(point.curve().begin(), point.curve().end(),
                      [](const Prime::Geom::coord_2_dbl & __p0,
                         const Prime::Geom::coord_2_dbl & __p1) {
                          return __p0.y < __p1.y ;
                      }) ;

            const double progress = 100.0 * picking_points_processed_num / picking_points_total_num ;
            setProgress( progress ) ;
        }
    }
    
    m_picking_storage.informAboutChanges() ;

    return true ;
}

}
}
}
