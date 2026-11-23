
#include <memory>

#include "primeApp/PrimeAppNoX.h"
#include "utils/methodname.h"
#include "data3d/model.h"
#include "Horizons/FRHorizonsTypes.h"
#include "data3d/surface_math.h"
#include "mathlib/algorithms/sorting.h"
#include "mathlib/algorithms/interpolation.h"

#include "PickingExportTask.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

const char * PickingExportToModel3dTask::NamesOfModelFrameTypes[PickingExportToModel3dTask::NumberOfModelFrameTypes + 1] = {
    "Constant step horizons",
    "Model",
    0
};

PickingExportToModel3dTask::PickingExportToModel3dTask(const PickingStorage& __picking_storage,
                                                       const DataStorage& __data_storage,
                                                       const std::string& __grid_name,
                                                       ModelFrameType __model_frame_type,
                                                       double __horizons_step,
                                                       const std::string& __frame_model_name,
                                                       bool __use_topo_horizon,
                                                       double __interpolation_radius,
                                                       double __smoothness_radius,
                                                       BasicModel3d * __output_model,
                                                       const std::string& __output_name,
                                                       const std::string& __output_attribute_name,
                                                       double __output_step_x,
                                                       double __output_step_y) :
m_picking_storage(__picking_storage),
m_data_storage(__data_storage),
m_grid_name(__grid_name),
m_model_frame_type(__model_frame_type),
m_horizons_step(__horizons_step),
m_frame_model_name(__frame_model_name),
m_use_topo_horizon(__use_topo_horizon),
m_interpolation_radius(__interpolation_radius),
m_smoothness_radius(__smoothness_radius),
m_output_model(__output_model),
m_output_name(__output_name),
m_output_attribute_name(__output_attribute_name),
m_output_step_x(__output_step_x),
m_output_step_y(__output_step_y) { }

PickingExportToModel3dTask::~PickingExportToModel3dTask() { }

const std::string& PickingExportToModel3dTask::name() const
{
    static const std::string name = "Exporting to model" ;
    return name ;
}

bool PickingExportToModel3dTask::run()
{
    using namespace Prime::lib::primeApp ;
    using namespace Prime::lib::mathlib ;

    // Getting domain name of output/frame model
    using namespace Prime::lib::Nmo ;
    auto hodograph_type = m_picking_storage.getHodographType() ;
    bool is_time_domain = (HodographType::get_domain_type(hodograph_type) == FR_IO_TIME) ;
    std::string domain_name = HodographType::get_domain_name( hodograph_type ) ;
    std::string subdomain_name = HodographType::get_domain_attribute_name( hodograph_type ) ;
    const double domain_scale = (is_time_domain ? 1000.0 : 1.0) ;

    // Opening output/frame model grid
    std::unique_ptr<Grid3d> grid( new Grid3d(getProjDB(), m_grid_name) ) ;

    if ( !grid )
    {
        setErrorDescription( "Can't open grid of output model." ) ;
        return false ;
    }

    // Getting output mesh parameters
    double output_dx = m_output_step_x ;
    double output_dy = m_output_step_y ;

    int output_step_XL = std::max<int>( std::round(output_dx / grid->dx), 1 ) ;
    int output_step_IL = std::max<int>( std::round(output_dy / grid->dy), 1 ) ;

    output_dx = output_step_XL * grid->dx ;
    output_dy = output_step_IL * grid->dy ;

    int output_nx = std::max( grid->nx / output_step_XL, 1 ) ;
    int output_ny = std::max( grid->ny / output_step_IL, 1 ) ;

    Geom3d output_geom(grid->x0, grid->y0,
                       output_dx, output_dy,
                       output_nx, output_ny) ;
    
    // Preparation output model
    BasicModel3d * p_output_model = m_output_model ;
    std::unique_ptr<BasicModel3d> output_model ;
    
    if ( !p_output_model )
    {
        output_model.reset( new BasicModel3d( grid.get(), domain_name, m_output_name ) ) ;
        p_output_model = output_model.get() ;
        
        if ( !p_output_model )
        {
            setErrorDescription( "Can't open output model." ) ;
            return false ;
        }
    }
    
    // undo group
    if (p_output_model->getUndoManager())
      p_output_model->getUndoManager()->beginUndoGroup();
    
    p_output_model->clear() ;

    // Getting parameters of output frame
    std::string weights_surface_name = "weights_sum" ;

    if ( m_model_frame_type == ConstantStepHorizons )
    {
        if ( m_horizons_step <= 0 )
        {
            setErrorDescription( "Horizons step is incorrect." ) ;
            return false ;
        }

        const double data_length =
                m_data_storage.getSampleEnd() -
                m_data_storage.getSampleBeg() ;

        if ( data_length <= 0 )
        {
            setErrorDescription( "Input data length is incorrect." ) ;
            return false ;
        }

        int horizons_num = int( data_length / m_horizons_step ) ;
        horizons_num = std::max( horizons_num, 1 ) ;

        SurfaceUChar attribute( output_geom ) ;
        SurfaceFloat surface( output_geom ) ;

        attribute = HOR_ATTR_NORMAL ;

        for ( int i = 0; i < horizons_num; ++i )
        {
            const float frame_value = m_data_storage.getSampleBeg() + (i + 1) * m_horizons_step ;

            std::ostringstream name ;
            name << (i + 1) ;

            if ( !p_output_model->addHorizon(Horizon3d(name.str(), i), false, false) )
                continue ;

            Horizon3d * horizon = p_output_model->getHorizon( name.str() ) ;

            if ( !horizon )
                continue ;

            surface = frame_value ;
            horizon->setAttributes( attribute ) ;
            horizon->addSurface( subdomain_name, surface ) ;
            horizon->addSurface( m_output_attribute_name, surface ) ;
            horizon->addSurface( weights_surface_name, surface ) ;
        }
    }
    else if ( m_model_frame_type == ModelFrameType::ModelHorizons )
    {
        std::unique_ptr<BasicModel3d> frame_model( new BasicModel3d(getProjDB(), 0, grid.get(), domain_name, m_frame_model_name) ) ;

        if ( !frame_model )
        {
            setErrorDescription( "Can't load frame model '" + m_frame_model_name + "'." ) ;
            return false ;
        }

        SurfaceUChar attribute ;
        SurfaceFloat surface ;

        for ( const auto& it : frame_model->getHorizons() )
        {
            const Horizon3d& frame_horizon = it.second ;

            if (frame_model->getDomain() == Domain::Depth &&
                m_use_topo_horizon &&
                frame_horizon.getNumber() == 0 )
                continue ;

            const SurfaceFloat * frame_surface = frame_horizon.getSurface( subdomain_name ) ;

            if ( !frame_surface )
                continue ;

            if ( !p_output_model->addHorizon(Horizon3d(frame_horizon.getName(),
                                                    frame_horizon.getNumber()), false, false) )
                continue ;

            Horizon3d * horizon = p_output_model->getHorizon( frame_horizon.getName() ) ;

            if ( !horizon )
                continue ;

            // Changing geometry of input frame
            attribute = frame_horizon.getAttributes() ;
            surface = *frame_surface ;
            surface_geometry_change( surface, attribute, output_geom ) ;

            // Adding surfaces to output model
            horizon->setAttributes( attribute ) ;
            horizon->addSurface( subdomain_name, surface ) ;
            horizon->addSurface( m_output_attribute_name, surface ) ;
            horizon->addSurface( weights_surface_name, surface ) ;
        }
    }

    // Preparation links to output surfaces
    std::vector<SurfaceUChar *> output_attr_list ;
    std::vector<SurfaceFloat *> output_arg_surf_list ; // Time or depth surfaces
    std::vector<SurfaceFloat *> output_val_surf_list ; // Output parameter surfaces
    std::vector<SurfaceFloat *> weight_sum_surf_list ; // Temporary surfaces of weights stack

    const auto output_horizons_names = p_output_model->listHorizonNames() ;

    for ( const auto& horizon_name : output_horizons_names )
    {
        Horizon3d * horizon = p_output_model->getHorizon(horizon_name) ;

        if ( !horizon )
            continue ;

        output_attr_list.push_back( const_cast<SurfaceUChar*> (&horizon->getAttributes()) ) ;
        output_arg_surf_list.push_back( horizon->getSurface(subdomain_name) ) ;
        output_val_surf_list.push_back( horizon->getSurface(m_output_attribute_name) ) ;
        weight_sum_surf_list.push_back( horizon->getSurface(weights_surface_name) ) ;

        if (!output_attr_list.back() ||
            !output_arg_surf_list.back() ||
            !output_val_surf_list.back() ||
            !weight_sum_surf_list.back())
            continue ;

        *output_val_surf_list.back() = 0 ;
        *weight_sum_surf_list.back() = 0 ;
    }

    // Checking frame of output model
    if ( p_output_model->empty() )
    {
        setErrorDescription( "Output model is empty." ) ;
        return false ;
    }

    // Getting interpolation radius parameters
    // For interpolation we use self-made modification of IDW method (inverse distance weighting).
    // We use the next expression of weights:
    //
    // w(x) = (1/R^2) * (R^2 - x^2) / (Ksm * x^2 + 1), where
    //
    // R - maximal interpolation/extrapolation radius, i.e. radius where some input point has non-zero weight
    // x - distance from point, has to be <= R
    // Ksm - smoothing factor (see below).
    //
    // Ksm = ( (1 - hamma) * R^2 - Rsm^2 ) / (hamma * R^2 * Rsm^2), where
    //
    // Rsm - distance from input point where its weight equal to hamma.
    // By default hamma = 0.5, i.e. Rsm - distance where influence weight of point is 0.5.
    // Rsm <= R * sqrt( 1 - hamma).
    //
    double interp_radius = m_interpolation_radius ;
    interp_radius = std::max( interp_radius, 1.0 ) ;
    
    const double interp_radius_2 = interp_radius * interp_radius ;
    const double inv_interp_radius_2 = 1.0 / interp_radius_2 ;
    const int interp_radius_nx = int( interp_radius / output_geom.dx + 1.0 ) ;
    const int interp_radius_ny = int( interp_radius / output_geom.dy + 1.0 ) ;

    // Calculating smoothness factor
    double influence_level = 0.5 ;
    influence_level = std::max( influence_level, 0.1 ) ;
    influence_level = std::min( influence_level, 0.9 ) ;

    double smoothness_radius = m_smoothness_radius ;
    smoothness_radius = std::max( smoothness_radius, 1.0 ) ;
    smoothness_radius = std::min( smoothness_radius, interp_radius * std::sqrt(1 - influence_level) ) ;

    const double smoothness_radius_2 = smoothness_radius * smoothness_radius ;
    const double smoothness_factor =
            ((1 - influence_level) * interp_radius_2 - smoothness_radius_2)
            / (influence_level * interp_radius_2 * smoothness_radius_2) ;

    // Processing of all picking points
    std::vector<int> frame_indices_buffer ;
    std::vector<float> frame_values_buffer ;
    std::vector<float> buffer ;

    size_t picking_points_total_num = 0 ;
    size_t picking_points_processed_num = 0 ;

    for ( const auto& object : m_picking_storage.storage() )
    {
        picking_points_total_num += (object.second ? object.second->size() : 0) ;
    }

    for ( const auto& object : m_picking_storage.storage() )
    {
        if ( !object.second )
            continue ;

        for ( const auto& point : *object.second )
        {
            picking_points_processed_num++ ;
            
            if ( point.curve().empty() )
                continue ;

            //Getting point location
            double Px, Py ;
            grid->glob2loc( point.x(), point.y(), &Px, &Py  ) ;

            // Getting frame values
            std::vector<int> * frame_indices = &frame_indices_buffer ;
            frame_indices->clear() ;

            std::vector<float> * frame_values = &frame_values_buffer ;
            frame_values->clear() ;

            bool need_sorting = false ;

            for ( int i = 0; i < output_attr_list.size(); ++i )
            {
                const SurfaceUChar * attr = output_attr_list[i] ;
                const SurfaceFloat * surf = output_arg_surf_list[i] ;

                if ( !attr || !surf )
                    continue ;

                float value ;

                if ( !surf_get_interpolated_value(*surf, *attr, Px, Py, &value) )
                    continue ;

                if ( !frame_values->empty() && frame_values->back() > value )
                    need_sorting = true ;

                frame_indices->push_back( i ) ;
                frame_values->push_back( value ) ;
            }

            if ( need_sorting )
                sort( frame_values, frame_indices ) ;

            if (!frame_indices || !frame_values ||
                frame_indices->size() != frame_values->size())
                continue ;

            // Re-interpolation curve to required frame
            const int curve_size = point.curve().size() ;
            const int frame_size = frame_indices->size() ;

            buffer.resize( 2 * curve_size + frame_size ) ;
            float * curve_x = buffer.data() ;
            float * curve_y = buffer.data() + curve_size ;
            float * attribute_values = buffer.data() + 2 * curve_size ;

            for ( int i = 0; i < curve_size; ++i )
            {
                curve_x[i] = point.curve()[i].y * domain_scale ;
                curve_y[i] = point.curve()[i].x ;
            }

            linear_reinterpolation(curve_size, curve_x, curve_y,
                                   frame_size, frame_values->data(), attribute_values) ;

            // Expansion value around current point
            #pragma omp parallel for num_threads( getThreadsNum() )
            for ( int iframe = 0; iframe < frame_size; ++iframe )
            {
                const int index = frame_indices->at( iframe ) ;
                const float attribute_value = attribute_values[iframe] ;

                if ( index < 0 || index >= output_attr_list.size() )
                    continue ;

                SurfaceUChar * attr = output_attr_list[index] ;
                SurfaceFloat * surf = output_val_surf_list[index] ;
                SurfaceFloat * weights = weight_sum_surf_list[index] ;

                if ( !attr || !surf || !weights )
                    continue ;

                // Getting point location and area
                // which hits the interpolation radius
                int i0, j0 ;
                attr->getNearestIJ( Px, Py, &i0, &j0 ) ;

                int ibeg = i0 - interp_radius_nx ;
                ibeg = std::max( ibeg, 0 ) ;
                ibeg = std::min( ibeg, attr->nx - 1 ) ;

                int iend = i0 + interp_radius_nx ;
                iend = std::max( iend, 0 ) ;
                iend = std::min( iend, attr->nx - 1 ) ;

                int jbeg = j0 - interp_radius_ny ;
                jbeg = std::max( jbeg, 0 ) ;
                jbeg = std::min( jbeg, attr->ny - 1 ) ;

                int jend = j0 + interp_radius_ny ;
                jend = std::max( jend, 0 ) ;
                jend = std::min( jend, attr->ny - 1 ) ;

                // Visiting all points in interpolation radius
                for ( int j = jbeg; j <= jend; ++j )
                {
                    const double y = attr->y0 + j * attr->dy ;
                    const double dy = y - Py ;
                    const double dy2 = dy * dy ;

                    if ( dy2 > interp_radius_2 )
                        continue ;

                    for ( int i = ibeg; i <= iend; ++i )
                    {
                        if ( attr->getDataPtr()[j * attr->nx + i] == HOR_ATTR_GAP )
                            continue ;

                        const double x = attr->x0 + i * attr->dx ;
                        const double dx = x - Px ;
                        const double dx2 = dx * dx ;

                        if ( dx2 > interp_radius_2 )
                            continue ;

                        const double D2 = dx2 + dy2 ;

                        if ( D2 > interp_radius_2 )
                            continue ;
                        
                        const double weight = inv_interp_radius_2 * (interp_radius_2 - D2) / (smoothness_factor * D2 + 1) ;
                        surf->getDataPtrWrt()[j * surf->nx + i] += weight * attribute_value ;
                        weights->getDataPtrWrt()[j * weights->nx + i] += weight ;
                    }
                }
            }

            const double progress = 100.0 * picking_points_processed_num / picking_points_total_num ;
            setProgress( progress ) ;
        }
    }

    // Output surfaces normalization
    for ( const auto& horizon_name : output_horizons_names )
    {
        Horizon3d * horizon = p_output_model->getHorizon(horizon_name) ;

        if ( !horizon )
            continue ;

        SurfaceUChar * attr = const_cast<SurfaceUChar*> (&horizon->getAttributes()) ;
        SurfaceFloat * surf = horizon->getSurface(m_output_attribute_name) ;
        SurfaceFloat * weights = horizon->getSurface(weights_surface_name) ;

        if (!attr || !surf || !weights)
            continue ;

        for ( int k = 0; k < attr->nx * attr->ny; ++k )
        {
            if ( attr->getDataPtr()[k] == HOR_ATTR_GAP )
                continue ;

            const float weights_sum = weights->getDataPtr()[k] ;

            if ( weights_sum <= 0 )
            {
                attr->getDataPtrWrt()[k] = HOR_ATTR_GAP ;
                surf->getDataPtrWrt()[k] = 0 ;
                continue ;
            }

            surf->getDataPtrWrt()[k] /= weights_sum ;
        }

        horizon->del( weights_surface_name ) ;
    }

    // undo group
    if (p_output_model->getUndoManager())
      p_output_model->getUndoManager()->endUndoGroup();

    // Saving to external model
    if ( !m_output_model )
    {
        setProgress( 99 ) ; // For showing that task is not done yet
        p_output_model->connect( getProjDB() ) ;
        p_output_model->save( ) ;
    }

    return true ;
}

}
}
}

