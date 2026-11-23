
#include <omp.h>
#include <cmath>
#include <string>
#include <float.h>
#include <stdexcept>

#include "firstbreak/fbstorage.h"
#include "utils/methodname.h"

#include "EquationsReader.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

void EquationsReader::init()
{
    define_methodname() ;

    if ( !equations_set )
    {
        throw std::invalid_argument(__methodname__ + "Error. Pointer to output equations set is invalid.") ;
    }

    if ( !equations_set->getCoder() )
    {
        throw std::invalid_argument(__methodname__ + "Error. Link to coordinates coder is invalid.") ;
    }

    if ( !prof )
    {
        throw std::invalid_argument(__methodname__ + "Error. Link to profile info is invalid.") ;
    }

    // 2D case
    if ( !prof->if3d() && prof->getType() == ProfInfo::LINE )
    {
        // Getting offsets information
        if ( prof->GetLine2d() )
        {
            offset_step_x = std::max( prof->GetLine2d()->value().dl, 0.1 ) ;
            offset_step_y = 0.0 ;
            data_type = Prime::DATA2D ;
            object_name = prof->GetLine2d()->key().name ;
        }
        else
        {
            throw std::runtime_error(__methodname__ + "Error. Invalid link to profile.") ;
        }
    }
        // 3D case
    else if ( prof->if3d() && prof->getType() == ProfInfo::GRID )
    {
        // Getting offsets information
        if ( prof->GetGrid() )
        {
            offset_step_x = std::max( 2 * prof->GetGrid()->value().s_dx, 0.1 ) ;
            offset_step_y = std::max( 2 * prof->GetGrid()->value().s_dy, 0.1 ) ;
            data_type = Prime::DATA3D ;
            object_name = prof->GetGrid()->key().name ;
        }
        else
        {
            throw std::runtime_error(__methodname__ + "Error. Invalid link to grid.") ;
        }
    }
    else
    {
        std::runtime_error(__methodname__ + "Error. Incorrect processing object.") ;
    }
}



const float GatesFilesEquationsReader::DEF_CRITICAL_WEIGHT = 0.001 ;

bool GatesFilesEquationsReader::read(const std::string& linename, const std::string& libname)
{
    define_methodname() ;

    equations_set->clear() ;

    std::string fatal_error ;

    omp_lock_t db_lock ;
    omp_init_lock( &db_lock ) ;

    try
    {
        std::string error ;
        std::vector<SurfacePoint> equations_points ;
        std::vector<EquationsSet::ValueVector> equations_values ;

        if ( read( linename, libname, equations_points, equations_values, db_lock, error ) )
        {
            equations_set->addEquations( equations_points, equations_values ) ;
        }
        else
        {
            fatal_error = linename + ": " + error ;
        }
    }
    catch ( const std::exception& __err )
    {
        fatal_error = __err.what() ;
    }

    omp_destroy_lock( &db_lock ) ;

    if ( !fatal_error.empty() )
    {
        std::ostringstream message ;
        message << __methodname__ << ": Error. Fatal error has "
                << "been caught: " << fatal_error << std::endl ;
        std::cerr << message.str() ;
        return false ;
    }

    if ( equations_set->empty() )
    {
        std::ostringstream message ;
        message << __methodname__ << "Error. No equations have been read. " ;
        message << "Please check your input data." << std::endl ;
        std::cerr << message.str() ;
        return false ;
    }

    if ( progress_object_id >= 0 && progress_handler )
    {
        progress_handler( progress_object_id, 100 ) ;
    }

    return true ;
}

bool GatesFilesEquationsReader::read(const std::vector<std::string>& lineslist, const std::string& libname)
{
    define_methodname() ;

    equations_set->clear() ;

    #ifdef _OPENMP
    omp_set_num_threads( threads_num ) ;
    //kmp_set_blocktime(0) ;
    #endif

    int done_lines_num = 0 ;
    int current_progress = 0 ;

    std::string fatal_error ;

    omp_lock_t db_lock ;
    omp_lock_t picks_points_merge_lock ;

    omp_init_lock( &db_lock ) ;
    omp_init_lock( &picks_points_merge_lock ) ;

    #pragma omp parallel for schedule(dynamic)
    for ( int i = 0; i < lineslist.size(); ++i )
    {
        if ( !fatal_error.empty() )
        {
            i = lineslist.size() ;
            continue ;
        }

        std::string error ;
        std::vector<SurfacePoint> equations_points ;
        std::vector<EquationsSet::ValueVector> equations_values ;

        if ( read( lineslist[i], libname, equations_points, equations_values, db_lock, error ) )
        {
            if ( !equations_points.empty() )
            {
                omp_set_lock( &picks_points_merge_lock ) ;
                try
                {
                    equations_set->addEquations(equations_points, equations_values) ;
                }
                catch ( const std::exception& __err )
                {
                    omp_unset_lock( &picks_points_merge_lock ) ;
                    fatal_error = __err.what() ;
                    continue ;
                }
                omp_unset_lock( &picks_points_merge_lock ) ;
            }
        }
        else
        {
            std::string message = lineslist[i] + ": " + error ;
            std::cerr << message << std::endl ;
        }

        #pragma omp atomic
        done_lines_num++ ;

        if ( omp_get_thread_num() == 0 )
        {
            if ( progress_object_id >= 0 && progress_handler )
            {
                int _progress = int( 100.0 * done_lines_num / lineslist.size() ) ;

                if ( _progress != current_progress )
                {
                    current_progress = _progress ;
                    progress_handler( progress_object_id, _progress ) ;
                }

                std::ostringstream message ;
                message << "Current number of loaded equations: " << equations_set->size()
                        << ". Approximately required memory (Gb): " << equations_set->getMemoryUsageGb() << "." ;
                std::clog << message.str() << std::endl ;
            }
        }
    }

    omp_destroy_lock( &db_lock ) ;
    omp_destroy_lock( &picks_points_merge_lock ) ;

    if ( !fatal_error.empty() )
    {
        std::ostringstream message ;
        message << __methodname__ << ": Error. Fatal error has "
                << "been caught: " << fatal_error << std::endl ;
        std::cerr << message.str() ;
        return false ;
    }

    if ( equations_set->empty() )
    {
        std::ostringstream message ;
        message << __methodname__ << "Error. No equations have been read. " ;
        message << "Please check your input data." << std::endl ;
        std::cerr << message.str() ;
        return false ;
    }

    if ( progress_object_id >= 0 && progress_handler )
    {
        progress_handler( progress_object_id, 100 ) ;
    }

    return true ;
}

bool GatesFilesEquationsReader::read(const std::string& linename,
                                     const std::string& libname,
                                     std::vector<SurfacePoint>& equations_points,
                                     std::vector<EquationsSet::ValueVector>& equations_values,
                                     omp_lock_t& db_lock,
                                     std::string& error ) const
{
    try
    {
        lineGatherAttr picking( prof->FS.FS, linename ) ;
        picking.read_base_lineGatherAttr( libname, runTimeSeismCmp::nSp_nRp, getLibType() ) ;

        if ( !picking.isBaseReadigOk() )
        {
            std::string message ;
            message += "Warning. Can't load '" + libname + "' on the " + linename + " profile. " ;
            message += "You can fix it and try again or just ignore this message." ;
            throw std::runtime_error( message ) ;
        }

        if ( !picking.exist_horGatherAttr( getPicksHorizonName() ) )
        {
            std::string message ;
            message += "Warning. Library '" + libname + "' doesn't contain " ;
            message += getPicksHorizonName() + " horizon. " ;
            message += "You can create it and try again or just ignore this message." ;
            throw std::runtime_error( message ) ;
        }

        // Getting profile parameters
        bool is_inline ;
        int line ;
        int cdp_beg, cdp_step ;
        double xdp_beg, xdp_step ;

        omp_set_lock( &db_lock ) ;
        {
            try
            {
                if ( !prof->if3d() )
                {
                    Prime::DB::Line2d profile( prof->GetArea(), Prime::DB::Line2d::Key( linename ) ) ;
                    is_inline = true ;
                    line = 0 ;
                    xdp_beg = profile.value().pini ;
                    cdp_beg = profile.value().cdpini ;
                    xdp_step = profile.value().dp ;
                    cdp_step = profile.value().dcdp ;
                }
                else
                {
                    Prime::DB::Grid grid( prof->GetArea(), Prime::DB::Grid::Key(Prime::DB::Line3d::Key::name2key( linename ).grid));
                    Prime::DB::Line3d profile( prof->GetArea(), &grid, Prime::DB::Line3d::Key::name2key( linename ) ) ;
                    is_inline = profile.key().orientation ;
                    line = profile.key().n ;
                    xdp_beg = profile.value().pini ;
                    cdp_beg = profile.value().cdpini ;
                    xdp_step = profile.value().dp ;
                    cdp_step = profile.value().dcdp ;
                }
            }
            catch (...)
            {
                omp_unset_lock( &db_lock ) ;
                throw ;
            }
        }

        omp_unset_lock( &db_lock ) ;

        horGatherAttr& attr = picking.get_horGatherAttr( getPicksHorizonName() ) ;
        const Container& storage = attr.get_horGatherAttr() ;

        int equations_rhs_num = 0 ;
        int loaded_points_num = 0 ;

        for ( Container::const_iterator it = storage.begin(); it != storage.end(); )
        {
            const int nSp = it->first.nSp ;
            const int nRp = it->first.nRp ;

            // Cmp coordinate (line, ncdp) encoding.
            const int ncdp = cdp_beg + O( ( it->first.xdp - xdp_beg ) / xdp_step ) * cdp_step ;
            const int cmp_index = ( is_inline ) ? getCoder()->encodeLineCdp( line, ncdp ) : getCoder()->encodeLineCdp( ncdp, line ) ;

            // Getting offsets
            int Kx, Ky ;
            double Lx, Ly ;

            if ( !prof->if3d() )
            {
                Kx = O( fabsf(it->first.offset2d) / offset_step_x ) ;
                Ky = 0 ;
                Lx = it->first.offset2d ;
                Ly = 0.0 ;
            }
            else
            {
                float Sx, Sy, Rx, Ry ;
                getCoder()->decodeSourceCoord( nSp, Sx, Sy ) ;
                getCoder()->decodeReceivCoord( nRp, Rx, Ry ) ;

                Kx = O( (Rx - Sx) / offset_step_x ) ;
                Ky = O( (Ry - Sy) / offset_step_y ) ;
                Ky = ( Kx >= 0 ) ? Ky : -Ky ;
                Kx = ( Kx >= 0 ) ? Kx : -Kx ;
                Lx = Rx - Sx ;
                Ly = Ry - Sy ;
            }

            // All shifts are stored ordered and we want combine
            // all traces with equal keys to the single value
            Container::const_iterator end = storage.upper_bound( it->first ) ;

            std::vector<float> values_sum ;
            std::vector<float> weights_sum ;

            for ( ; it != end; ++it )
            {
                std::vector<float> values ;
                std::vector<float> weights ;

                if ( !readValues( it, values, weights ) )
                    continue ;

                if ( equations_rhs_num && values.size() != equations_rhs_num )
                {
                    std::ostringstream message ;
                    message << "Error. Library '" << libname << "' contains "
                            << "inconsistent length of '" << getPicksAttribName() << "' "
                            << "attribute vectors at point nSp(" << nSp << "), "
                            << "nRp(" << nRp << "). Check your data." ;
                    throw std::length_error( message.str() ) ;
                }

                equations_rhs_num = values.size() ;

                if ( values_sum.empty() )
                {
                    values_sum.swap( values ) ;
                    weights_sum.swap( weights ) ;
                }
                else
                {
                    for ( int i = 0; i < equations_rhs_num; ++i )
                    {
                        values_sum[i] += weights[i] * values[i] ;
                        weights_sum[i] += weights[i] ;
                    }
                }
            }

            if ( equations_rhs_num <= 0 )
                continue ;

            if ( values_sum.empty() || values_sum.size() != weights_sum.size() )
                continue ;

            // Normalization + skipping bad points
            EquationsSet::ValueVector value( equations_rhs_num ) ;

            bool skip_bad_point = false ;

            for ( int i = 0; i < equations_rhs_num; ++i )
            {
                if ( weights_sum[i] < DEF_CRITICAL_WEIGHT )
                {
                    skip_bad_point = true ;
                    continue ;
                }

                value[i] = 1.0 * values_sum[i] / weights_sum[i] ;
            }

            if ( skip_bad_point )
                continue ;

            // Input data decimation
            loaded_points_num++ ;

            if ( loaded_points_num % input_decimation != 0 )
                continue ;

            // Adding new point
            SurfacePoint point ;
            point.setCoordinate(SurfacePoint::nSp, nSp) ;
            point.setCoordinate(SurfacePoint::nRp, nRp) ;
            point.setCoordinate(SurfacePoint::Cmp, cmp_index) ;
            point.setCoordinate(SurfacePoint::Kx, Kx) ;
            point.setCoordinate(SurfacePoint::Ky, Ky) ;
            point.setLx(Lx) ;
            point.setLy(Ly) ;

            equations_points.push_back( point ) ;
            equations_values.push_back( value ) ;
        }
    }
    catch ( const std::exception& err )
    {
        error = err.what() ;
        return false ;
    }
    catch (...)
    {
        return false ;
    }

    if ( equations_points.empty() )
    {
        error = "Error. Input doesn't contain any data, or attribute vectors have a zero length." ;
        return false ;
    }

    return true ;
}

bool GatesFilesEquationsReader::readValues( const Container::const_iterator& it, std::vector<float>& values, std::vector<float>& weights ) const
{
    if ( !it->second.get_value( getPicksAttribName(), &values ) )
        return false ;

    for ( int i = 0, n = values.size(); i < n; ++i )
    {
        if ( isnan(values[i]) || isinf(values[i]) )
            return false ;
    }

    if ( !getWeightAttribName().empty() )
    {
        if ( !it->second.get_value( getWeightAttribName(), &weights ) )
            weights.assign( values.size(), 1.f ) ;

        for ( int i = 0, n = weights.size(); i < n; ++i )
        {
            if ( weights[i] < DEF_CRITICAL_WEIGHT )
                return false ;
        }

        if ( weights.size() == 1 )
            weights.assign( values.size(), weights.front() ) ;
    }
    else
    {
        weights.assign( values.size(), 1.f ) ;
    }

    if ( weights.size() != values.size() )
        return false ;

    for ( int i = 0, n = weights.size(); i < n; ++i )
    {
        if ( isnan(weights[i]) || isinf(weights[i]) )
            return false ;
    }

    //! @todo Weighting has been removed temporary
    weights.assign( weights.size(), 1.f ) ;

    return !values.empty() ;
}

bool FirstBreaksEquationsReader::read( const std::string& fbpicking_name )
{
    define_methodname() ;

    using namespace Prime::lib::firstbreak ;
    const int DEF_READ_BLOCK_SIZE = 1000 ;

    equations_set->clear() ;

    // Open first breaks storage
    FbPickingStorage fbstorage ;
    std::string fatal_error ;

    if ( !fbstorage.open(data_type, object_name, fbpicking_name, &fatal_error) )
    {
        std::ostringstream message ;
        message << __methodname__ << ": Error. Can't open first breaks storage. Reason: " << fatal_error ;
        std::clog << message.str() << std::endl ;
        return false ;
    }

    // Reading all first break picks from storage
    int current_progress = 0 ;    
    int processed_fbpicks_num = 0 ;
    int not_read_fbpicks_num = 0 ;
    int loaded_points_num = 0 ;

    #ifdef _OPENMP
    omp_set_num_threads( threads_num ) ;
    //kmp_set_blocktime(0) ;
    #endif

    omp_lock_t equations_set_access_lock ;
    omp_init_lock( &equations_set_access_lock ) ;

    #pragma omp parallel
    {
        std::vector<SurfacePoint> equations_points ;
        std::vector<EquationsSet::ValueVector> equations_values ;
        std::vector<FbPick> fbpicks_buffer ;
        EquationsSet::ValueVector value( 1 ) ;

        #pragma omp for schedule(dynamic)
        for ( FbIndexType ifbpick = 0; ifbpick < fbstorage.size(); ifbpick += DEF_READ_BLOCK_SIZE )
        {
            if ( !fatal_error.empty() )
            {
                ifbpick = fbstorage.size() ;
                continue ;
            }

            FbIndexType fbpick_beg = ifbpick ;
            FbIndexType fbpick_end = std::min<FbIndexType>( ifbpick + DEF_READ_BLOCK_SIZE, fbstorage.size() ) ;

            if ( !fbstorage.readPicks(fbpick_beg, fbpick_end, &fbpicks_buffer) )
            {
                #pragma omp atomic
                processed_fbpicks_num += (fbpick_end - fbpick_beg) ;

                #pragma omp atomic
                not_read_fbpicks_num += (fbpick_end - fbpick_beg) ;

                continue ;
            }

            equations_points.clear() ;
            equations_values.clear() ;

            for ( const FbPick& fbpick : fbpicks_buffer )
            {
                if (!FbPick::is_valid(fbpick.fbtime) ||
                    !FbPick::is_valid(fbpick.fbtime_model))
                    continue ;

                // Input data decimation
                #pragma omp atomic
                loaded_points_num++ ;

                if ( loaded_points_num % input_decimation != 0 )
                    continue ;

                // Getting CMP/CDP position of point
                const double xcdp = 0.5 * (fbpick.xsp + fbpick.xrp) ;
                const double ycdp = 0.5 * (fbpick.ysp + fbpick.yrp) ;

                int line = 0, ncdp = 0 ;
                getCoder()->getLineCdpByCoord( xcdp, ycdp, line, ncdp ) ;

                // Getting offsets
                int Kx, Ky ;
                double Lx, Ly ;

                if ( !prof->if3d() )
                {
                    Lx = fbpick.offset() ;
                    Ly = 0.0 ;
                    Kx = O( Lx / offset_step_x ) ;
                    Ky = 0 ;
                }
                else
                {
                    Lx = fbpick.xrp - fbpick.xsp ;
                    Ly = fbpick.yrp - fbpick.ysp ;
                    Kx = O( Lx / offset_step_x ) ;
                    Ky = O( Ly / offset_step_y ) ;
                    Ky = ( Kx >= 0 ) ? Ky : -Ky ;
                    Kx = ( Kx >= 0 ) ? Kx : -Kx ;
                }

                // Adding new point
                SurfacePoint point ;
                point.setCoordinate(SurfacePoint::nSp, fbpick.nsp) ;
                point.setCoordinate(SurfacePoint::nRp, fbpick.nrp) ;
                point.setCoordinate(SurfacePoint::Cmp, getCoder()->encodeLineCdp(line, ncdp) ) ;
                point.setCoordinate(SurfacePoint::Kx, Kx) ;
                point.setCoordinate(SurfacePoint::Ky, Ky) ;
                point.setLx(Lx) ;
                point.setLy(Ly) ;

                value[0] = fbpick.fbtime - fbpick.fbtime_model ;

                equations_points.push_back( point ) ;
                equations_values.push_back( value ) ;
            }

            // Adding read point to equations set
            if ( !equations_points.empty() )
            {
                omp_set_lock( &equations_set_access_lock ) ;
                try
                {
                    equations_set->addEquations(equations_points, equations_values) ;
                }
                catch ( const std::exception& __err )
                {
                    omp_unset_lock( &equations_set_access_lock ) ;
                    fatal_error = __err.what() ;
                    continue ;
                }
                omp_unset_lock( &equations_set_access_lock ) ;
            }

            #pragma omp atomic
            processed_fbpicks_num += (fbpick_end - fbpick_beg) ;

            if ( omp_get_thread_num() == 0 )
            {
                if ( progress_object_id >= 0 && progress_handler )
                {
                    int _progress = int( 100.0 * processed_fbpicks_num / fbstorage.size() ) ;

                    if ( _progress != current_progress )
                    {
                        current_progress = _progress ;
                        progress_handler( progress_object_id, _progress ) ;
                    }

                    omp_set_lock( &equations_set_access_lock ) ;
                    std::ostringstream message ;
                    message << "Current number of loaded equations: " << equations_set->size() << "/" << fbstorage.size()
                            << ". Approximately required memory (Gb): " << equations_set->getMemoryUsageGb() << "." ;
                    std::clog << message.str() << std::endl ;
                    omp_unset_lock( &equations_set_access_lock ) ;
                }
            }
        }
    }

    omp_destroy_lock( &equations_set_access_lock ) ;

    if ( !fatal_error.empty() )
    {
        std::ostringstream message ;
        message << __methodname__ << ": Error. Fatal error has "
                << "been caught: " << fatal_error << std::endl ;
        std::cerr << message.str() ;
        return false ;
    }

    if ( equations_set->empty() )
    {
        std::ostringstream message ;
        message << __methodname__ << "Error. No equations have been read. " ;
        message << "Please check your input data." << std::endl ;
        std::cerr << message.str() ;
        return false ;
    }

    if ( progress_object_id >= 0 && progress_handler )
    {
        progress_handler( progress_object_id, 100 ) ;
    }

    return true ;
}


}
}
}
