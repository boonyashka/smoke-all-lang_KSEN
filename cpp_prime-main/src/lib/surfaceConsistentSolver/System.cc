
#include <omp.h>
#include <mkl.h>
#include <string>
#include <float.h>
#include <assert.h>
#include <algorithm>
#include <stdexcept>

#include "mathlib/algebra.h"
#include "mathlib/geometry.h"
#include "mathlib/algorithms.h"
#include "mathlib/algebra/matrix_symsparse.h"

#include "System.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

const char System::MKL_UPLO = 'U' ;
const int System::DEFAULT_RESIDUALS_REFRESH_ITERS = 10 ;
const int System::DEFAULT_ROBUST_SMOOTH_ITER_NUM = 3 ;
const int System::DEFAULT_LSM_EQUATIONS_MIN_NUM = 10 ;
const double System::DEFAULT_LSM_REGULARIZATION = 0.0001 ;
const double System::DEFAULT_SOLUTION_TOLERANCE = 1.0e-6 ;
const double System::DEFAULT_NARROW_AZIMUTH_RATIO = 2.5 ;

System::System( int _threads_num ) :
threads_num(_threads_num),
unknowns_num(0),
current_rhs(0),
equations_set( NULL ),
progress_object_id(-1),
progress_handler(NULL)
{
    for ( int i = 0; i < Component::NumberOfComponentTypes; ++i )
    {
        components[i] = Component( Component::Type(i) ) ;
    }
}

bool System::init(EquationsSet& _equations_set, const IterationsSet& _iterations)
{
    static const std::string method_prefix = "Solver::init(const EquationsReader&, const IterationsSet&): " ;

    // Checking
    if ( _equations_set.empty() )
    {
        std::cerr << method_prefix << "Error. Can't initialize solver because equations list is empty." << std::endl ;
        return false ;
    }

    if ( _iterations.get().empty() )
    {
        std::cerr << method_prefix << "Error. Can't initialize solver because iterations descriptor is empty." << std::endl ;
        return false ;
    }

    unknowns_num = 0 ;
    equations_set = &_equations_set ;
    iterations = _iterations ;

    // Getting list of component which we have to update
    IterationInfo::ComponetsList _components_list ;

    for ( int i = 0; i < iterations.get().size(); ++i )
    {
        const IterationInfo& iter = iterations.get()[i] ;
        _components_list.insert(_components_list.end(),
                                iter.updating_components.begin(),
                                iter.updating_components.end() ) ;
    }

    // Switching on components which should be updated
    for ( int i = 0; i < Component::NumberOfComponentTypes; ++i )
    {
        typedef IterationInfo::ComponetsList::const_iterator ConstIterator ;
        ConstIterator beg = _components_list.begin() ;
        ConstIterator end = _components_list.end() ;
        components[i].is_active = ( std::find( beg, end, components[i].type ) != end ) ;
    }

    // Axis mappers initialization
    for ( int i = 0; i < SurfacePoint::NumberOfAxes; ++i )
    {
        mappers[i].clear() ;
    }

    for ( int i = 0; i < Component::NumberOfComponentTypes; ++i )
    {
        if ( !components[i].is_active )
            continue ;

        Axis axis = components[i].axis ;

        if ( mappers[axis].empty() )
            mappers[axis].initAndRelink( axis, *equations_set ) ;

        components[i].unknowns_beg = unknowns_num ;
        components[i].unknowns_num = mappers[axis].size() ;
        unknowns_num += mappers[axis].size() ;
    }

    return true ;
}

bool System::solve(LibStorage& result, int right_hand_side_index )
{
    static const std::string method_prefix = "Solver::solve(LibStorage&): " ;

    if ( right_hand_side_index < 0 || right_hand_side_index >= equations_set->getRhsNum() )
    {
        std::ostringstream msg ;
        msg << method_prefix << "Error. Invalid index of right hand side has been requested. "
                << "Number of available right hand side is " << equations_set->getRhsNum()
                << " but " << right_hand_side_index << " has been requested." ;
        std::cerr << msg.str() << std::endl ;
        return false ;
    }

    if (progress_handler)
        progress_handler( progress_object_id, 0 ) ;

    current_rhs = right_hand_side_index ;

    Column R( equations_set->size() ) ;
    Column X( unknowns_num, 0 ) ;
    Column Xtmp( unknowns_num ) ;

    for ( int iter = 0; iter < iterations.get().size(); ++iter )
    {
        if ( iter % DEFAULT_RESIDUALS_REFRESH_ITERS == 0 )
            computeResiduals( X, R ) ;

        memcpy( Xtmp.data(), X.data(), unknowns_num * sizeof (Column::value_type) ) ;

        std::string error = "Undefined error." ;

        if ( !makeIteration( iterations.get()[iter], X, R, error ) )
        {
            std::ostringstream msg ;
            msg << method_prefix << "Error at iteration no. " ;
            msg << iter << ". " << error ;
            std::cerr << msg.str() << std::endl ;
            return false ;
        }

        if ( computeTolerance( Xtmp, X ) < DEFAULT_SOLUTION_TOLERANCE )
            break ;

        if ( progress_handler )
        {
            const double current_progress = 1.0 * (iter + 1) / iterations.get().size() ;
            const int total_progress =  100 * (current_rhs + current_progress) / equations_set->getRhsNum() ;
            progress_handler( progress_object_id, total_progress ) ;
        }
    }

    for ( int i = 0, index = 0; i < Component::NumberOfComponentTypes; ++i )
    {
        const Component& component = components[i] ;
        const Axis axis = component.axis ;
        const AxisMapper& mapper = mappers[axis] ;

        if ( !component.is_active )
            continue ;

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const double value = X[component.unknowns_beg + k] ;
            result.set( component.type, mapper.getRealCoord(k), value ) ;
        }
    }

    return true ;
}

bool System::makeIteration(const IterationInfo& info, Column& X, Column& R, std::string& error) const
{
    switch (info.type)
    {
        case IterationInfo::GaussSeidelSolver:
            return makeGaussSeidel( info, X, R, error ) ;

        case IterationInfo::ConjugateGradientsSolver:
            return makeConjugateGradients( info, X, R, error ) ;

        case IterationInfo::StructAndRnmoSolver:
            return makeComputeStructAndRnmo( info, X, R, error ) ;
    }

    error = "Undefined iteration type." ;
    return false ;
}

bool System::makeGaussSeidel( const IterationInfo& info, Column& X, Column& R, std::string& error ) const
{
    const int components_num = info.updating_components.size() ;

    if ( components_num <= 0 )
        return true ;

    Column Rtmp = R ;             // Save vector of input right part
    Column dX( X.size(), 0 ) ;    // Adds to current solution
    Column AkAk( X.size(), 0 ) ;  // Inverted squared length of k-th column
    double XkXk = 0 ;             // X vector norm, i.e. |Xk|

    // Calculation squared length of required columns
    for ( int icomp = 0; icomp < components_num; ++icomp )
    {
        Component::Type type = info.updating_components[icomp] ;
        const Component& component = components[type] ;

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const int index = component.unknowns_beg + k ;
            const double length = columnSqrLength( component, k ) ;
            AkAk[index] = ( length > FLT_EPSILON ) ? 1. / length : 0. ;
            XkXk += X[index] * X[index] ;
        }
    }

    // Gauss-Seidel iterations
    for ( int iter = 0; iter < info.iterations_max_num; ++iter )
    {
        double dXdX = 0 ;
        double XkdX = 0 ;

        for ( int icomp = 0; icomp < components_num; ++icomp )
        {
            Component::Type type = info.updating_components[icomp] ;
            const Component& component = components[type] ;

            for ( int k = 0; k < component.unknowns_num; ++k )
            {
                const int index = component.unknowns_beg + k ;
                const double AkR = columnDotVector( component, k, Rtmp ) ;
                const double delta = AkAk[index] * AkR ;
                vectorPlusColumn( Rtmp, -delta, component, k ) ;

                XkdX += (X[index] + dX[index]) * delta ;
                dXdX += delta * delta ;
                dX[index] += delta ;
            }
        }

        // Check convergence
        if ( XkXk > FLT_MIN && dXdX / XkXk < DEFAULT_SOLUTION_TOLERANCE )
            break ;

        XkXk = XkXk + 2 * XkdX + dXdX ;
    }

    // Update dX
    removeConstants( info.updating_components, dX ) ;

    // Smooth dX + update X and R
    for ( int icomp = 0; icomp < components_num; ++icomp )
    {
        Component::Type type = info.updating_components[icomp] ;
        const Component& component = components[type] ;

        if ( info.apply_smoothing &&
            !smoothComponent(component,
                             info.smooth_window_x,
                             info.smooth_window_y,
                             info.robustness_threshold,
                             dX ) )
        {
            error = "Error. Component smoothing has been failed." ;
            return false ;
        }

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const int index = component.unknowns_beg + k ;
            vectorPlusColumn( R, -dX[index], component, k ) ;
            X[index] += dX[index] ;
        }
    }

    return true ;
}

bool System::makeConjugateGradients( const IterationInfo& info, Column& X, Column& R, std::string& error ) const
{
    using namespace Prime::lib::mathlib ;

    const int components_num = info.updating_components.size() ;

    if ( components_num <= 0 )
        return true ;

    // Getting number of unknowns
    // and re-indexation.
    int unknowns_num = 0 ;
    std::vector<int> unknowns_beg( components_num ) ;

    for ( int i = 0; i < components_num; ++i )
    {
        const Component::Type type = info.updating_components[i] ;
        unknowns_beg[i] = unknowns_num ;
        unknowns_num += components[type].unknowns_num ;
    }

    // Omp row locks initialization
    std::vector<omp_lock_t> row_locks( unknowns_num ) ;
    for ( int i = 0; i < row_locks.size(); ++i )
        omp_init_lock( &row_locks[i] ) ;

    // LSM matrix construction
    typedef SymSparseMatrix<value_type> Matrix ;
    Matrix matrix( unknowns_num, unknowns_num ) ;
    Column Rk( unknowns_num, 0 ) ;

    #ifdef _OPENMP
    omp_set_num_threads( threads_num ) ;
    //kmp_set_blocktime(0) ;
    #endif

    #pragma omp parallel for
    for ( int row = 0; row < equations_set->size(); ++row )
    {
        const SurfacePoint& equation_point = equations_set->getEquationPoint(row) ;

        for ( int i = 0; i < components_num; ++i )
        {
            const Component& comp1 = components[info.updating_components[i]] ;
            const int index1 = unknowns_beg[i] + equation_point.coordinate(comp1.axis) ;
            const double value1 = getMatrixValue( row, comp1 ) ;

            if ( value1 == 0 )
                continue ;

            omp_set_lock( &row_locks[index1] ) ;

            for ( int j = i; j < components_num; ++j )
            {
                const Component& comp2 = components[info.updating_components[j]] ;
                const int index2 = unknowns_beg[j] + equation_point.coordinate(comp2.axis) ;
                const double value2 = getMatrixValue( row, comp2 ) ;

                if ( value2 == 0 )
                    continue ;

                matrix.set( index1, index2 ) += value1 * value2 ;
            }

            Rk[index1] += value1 * R[row] ;
            omp_unset_lock( &row_locks[index1] ) ;
        }
    }

    // Omp locks destruction
    for ( int i = 0; i < row_locks.size(); ++i )
        omp_destroy_lock( &row_locks[i] ) ;
    row_locks.clear() ;

    // Converting matrix into MKL sparse representation
    MKL_INT n ;
    Column values ;
    std::vector<MKL_INT> columns ;
    std::vector<MKL_INT> rowIndex ;
    Matrix::convert_to_mkl( matrix, &n, &n, &values, &columns, &rowIndex, false ) ;
    matrix.clear() ;

    // Solve by Conjugate Gradients
    Column dX( n, 0 ) ;
    Column Pk( n, 0 ) ;
    Column APk( n ) ;

    memcpy( Pk.data(), Rk.data(), n * sizeof (Column::value_type) ) ;
    double RkRk = blas_dot( n, Rk.data(), 1, Rk.data(), 1 ) ;
    mkl_set_num_threads ( threads_num );

    for ( int iter = 0; iter < info.iterations_max_num; ++iter )
    {
        // A * Pk multiplication
        blas_csrsymv( MKL_UPLO, n, values.data(), rowIndex.data(), columns.data(), Pk.data(), APk.data() ) ;
        const double PkAPk = blas_dot( n, Pk.data(), 1, APk.data(), 1 ) ;

        // Alpha calculation
        double alpha = 0 ;
        if ( fabs(PkAPk) > FLT_MIN )
            alpha = RkRk / PkAPk ;

        // If step is too small then stop
        if ( fabs(alpha) < FLT_MIN )
            break ;

        // Moving to the next step

        // X (k + 1)
        blas_axpy( n, alpha, Pk.data(), 1, dX.data(), 1 ) ;

        // R (k + 1)
        blas_axpy( n, -alpha, APk.data(), 1, Rk.data(), 1 ) ;

        // || R (k + 1) ||2
        const double _RkRk = RkRk ;
        RkRk = blas_dot( n, Rk.data(), 1, Rk.data(), 1 ) ;

        if ( fabs( (RkRk - _RkRk) / _RkRk ) < DEFAULT_SOLUTION_TOLERANCE )
            break ;

        // Finding next search direction
        const double betta = RkRk / _RkRk ;
        blas_ax( n, betta, Pk.data(), 1 ) ;
        blas_axpy( n, 1.0, Rk.data(), 1, Pk.data(), 1 ) ;
    }

    // Solution full vector construction
    Column dXtmp( X.size(), 0 ) ;

    for ( int i = 0; i < components_num; ++i )
    {
        const Component::Type type = info.updating_components[i] ;
        const Component& component = components[type] ;
        const int from = unknowns_beg[i] ;
        const int to = component.unknowns_beg ;

        memcpy( dXtmp.data() + to, dX.data() + from, component.unknowns_num * sizeof (Column::value_type) ) ;
    }

    // Update dX
    removeConstants( info.updating_components, dXtmp ) ;

    // Smooth dX + update X and R
    for ( int icomp = 0; icomp < components_num; ++icomp )
    {
        Component::Type type = info.updating_components[icomp] ;
        const Component& component = components[type] ;

        if ( info.apply_smoothing &&
            !smoothComponent(component,
                             info.smooth_window_x,
                             info.smooth_window_y,
                             info.robustness_threshold,
                             dXtmp ) )
        {
            error = "Error. Component smoothing has been failed." ;
            return false ;
        }

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const int index = component.unknowns_beg + k ;
            vectorPlusColumn( R, -dXtmp[index], component, k ) ;
            X[index] += dXtmp[index] ;
        }
    }

    return true ;
}

bool System::makeComputeStructAndRnmo( const IterationInfo& info, Column& X, Column& R, std::string& error ) const
{
    static const std::string method_prefix = "System::makeComputeStructAndRnmo(...): " ;

    const int components_num = info.updating_components.size() ;

    if ( components_num < 1 )
        return true ;

    // Checking components
    for ( int icomp = 0; icomp < components_num - 1; ++icomp )
    {
        Component::Type type1 = info.updating_components[icomp] ;
        Component::Type type2 = info.updating_components[icomp + 1] ;

        if ( components[type1].axis != components[type2].axis )
        {
            error = method_prefix + "Error. Components can't be binded to different axes." ;
            return false ;
        }

        if ( components[type1].unknowns_num != components[type2].unknowns_num )
        {
            error = method_prefix + "Error. Components can't have different number of unknowns." ;
            return false ;
        }
    }

    // Are Lx, Lxy and Ly components using
    // If yes then we can merge them in critical situations
    int lx_lxy_ly_counter = 0 ;

    for ( int icomp = 0; icomp < components_num; ++icomp )
    {
        Component::Type type = info.updating_components[icomp] ;

        if (type == Component::RnmoLxLx ||
            type == Component::RnmoLxLy ||
            type == Component::RnmoLyLy )
            lx_lxy_ly_counter++ ;
    }

    const bool is_lx_ly_merging_allowed = (lx_lxy_ly_counter == 3) ;

    // Solving system for each coordinate
    Column dX( X.size(), 0 ) ;
    Axis axis = components[info.updating_components[0]].axis ;
    const AxisMapper& mapper = mappers[axis] ;

    #pragma omp parallel for num_threads(threads_num)
    for ( int index = 0; index < mapper.size(); ++index )
    {
        const std::vector<int>& rows = mapper.getEquations( index ) ;
        const int nrows = rows.size() ;

        if ( nrows < DEFAULT_LSM_EQUATIONS_MIN_NUM )
            continue ;

        // Check should we merge offsets
        bool has_lx_ly_merged = false ;
        std::vector<Component::Type> _updating_components = info.updating_components ;

        if ( is_lx_ly_merging_allowed )
        {
            double sum_lx = 0 ;
            double sum_ly = 0 ;

            for ( int i = 0; i < nrows; ++i )
            {
                const int row = rows[i] ;
                const SurfacePoint& equation_point =  equations_set->getEquationPoint(row) ;
                sum_lx += fabs( equation_point.lx() ) ;
                sum_ly += fabs( equation_point.ly() ) ;
            }

            if (sum_lx < FLT_EPSILON || sum_ly / sum_lx > DEFAULT_NARROW_AZIMUTH_RATIO ||
                sum_ly < FLT_EPSILON || sum_lx / sum_ly > DEFAULT_NARROW_AZIMUTH_RATIO)
            {
                has_lx_ly_merged = true ;
                _updating_components.clear() ;

                for ( int icomp = 0; icomp < components_num; ++icomp )
                {
                    Component::Type type = info.updating_components[icomp] ;

                    if (type != Component::RnmoLxLx &&
                        type != Component::RnmoLxLy &&
                        type != Component::RnmoLyLy)
                        _updating_components.push_back( type ) ;
                }

                _updating_components.push_back( Component::RnmoL2 ) ;
            }
        }

        const int _components_num = _updating_components.size() ;

        // Matrix extraction
        Column rhand( nrows ) ;
        Column wnorm( _components_num, 0 ) ;
        std::vector<Column> matrix( _components_num, Column( nrows ) ) ;

        for ( int i = 0; i < nrows; ++i )
        {
            const int row = rows[i] ;
            for ( int j = 0; j < _components_num; ++j )
            {
                Component::Type type = _updating_components[j] ;
                const Component& component = components[type] ;
                const double value = getMatrixValue( row, component ) ;
                matrix[j][i] = value ;
                wnorm[j] += value * value ;
            }
            rhand[i] = R[row] ;
        }

        // Matrix values normalization
        for ( int j = 0; j < _components_num; ++j )
        {
            wnorm[j] = ( wnorm[j] > FLT_EPSILON ) ? 1.0 / sqrt(wnorm[j]) : 1.0 ;

            for ( int i = 0; i < nrows; ++i )
                matrix[j][i] *= wnorm[j] ;
        }

        // A'A and A'b construction, i.e. LSM matrix calculation
        Column b( _components_num, 0 ) ;
        Column A( _components_num * _components_num ) ;

        for ( int i = 0; i < _components_num; ++i )
        {
            for ( int j = i; j < _components_num; ++j )
            {
                double value = 0 ;
                for ( int k = 0; k < nrows; ++k )
                    value += matrix[i][k] * matrix[j][k] ;

                A[i * _components_num + j] = value ;
                A[j * _components_num + i] = value ;
            }

            for ( int k = 0; k < nrows; ++k )
                b[i] += matrix[i][k] * rhand[k] ;

            A[i * _components_num + i] += DEFAULT_LSM_REGULARIZATION ;
        }

        // Solving
        Column x( _components_num, 0 ) ;
        if ( !solveLinearSystem( A, b, x ) )
            continue ;

        // dX updating
        for ( int j = 0; j < _components_num; ++j )
        {
            if ( isinf(x[j]) || isnan(x[j]) )
                continue ;

            Component::Type type = _updating_components[j] ;

            if ( !has_lx_ly_merged || type != Component::RnmoL2 )
            {
                const Component& component = components[type] ;
                dX[component.unknowns_beg + index] = wnorm[j] * x[j] ;
            }
            else
            {
                const Component& lx_component = components[Component::RnmoLxLx] ;
                const Component& ly_component = components[Component::RnmoLyLy] ;
                dX[lx_component.unknowns_beg + index] = wnorm[j] * x[j] ;
                dX[ly_component.unknowns_beg + index] = wnorm[j] * x[j] ;
            }
        }
    }

    // Smoothing + updating X and R
    for ( int icomp = 0; icomp < components_num; ++icomp )
    {
        Component::Type type = info.updating_components[icomp] ;
        const Component& component = components[type] ;

        if ( info.apply_smoothing &&
            !smoothComponent(component,
                             info.smooth_window_x,
                             info.smooth_window_y,
                             info.robustness_threshold,
                             dX ) )
        {
            error = method_prefix + "Error. Component smoothing has been failed." ;
            return false ;
        }

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const int index = component.unknowns_beg + k ;
            vectorPlusColumn( R, -dX[index], component, k ) ;
            X[index] += dX[index] ;
        }
    }

    return true ;
}

double System::columnSqrLength(const Component& component, int index) const
{
    const AxisMapper& mapper = mappers[component.axis] ;
    const std::vector<int>& equations_list = mapper.getEquations( index ) ;

    double result = 0.0 ;

    for ( int i = 0; i < equations_list.size(); ++i )
    {
        const int row = equations_list[i] ;
        const double value = getMatrixValue( row, component ) ;
        result += value * value ;
    }

    return result ;
}

double System::columnDotVector( const Component& component, int index, const Column& vector ) const
{
    const AxisMapper& mapper = mappers[component.axis] ;
    const std::vector<int>& equations_list = mapper.getEquations( index ) ;

    double result = 0.0 ;

    for ( int i = 0; i < equations_list.size(); ++i )
    {
        const int row = equations_list[i] ;
        const double value = getMatrixValue( row, component ) ;
        result += value * vector[row] ;
    }

    return result ;
}

void System::vectorPlusColumn( Column& vector, double scalar, const Component& component, int index ) const
{
    const AxisMapper& mapper = mappers[component.axis] ;
    const std::vector<int>& equations_list = mapper.getEquations( index ) ;

    for ( int i = 0; i < equations_list.size(); ++i )
    {
        const int row = equations_list[i] ;
        const double value = getMatrixValue( row, component ) ;
        vector[row] += scalar * value ;
    }
}

void System::computeResiduals( const Column& X, Column& R ) const
{
    static const std::string method_prefix = "System::computeResiduals( const Column&, Column& ): " ;

    if ( X.size() != unknowns_num )
    {
        throw std::runtime_error( method_prefix + "Error. Size of X vector doesn't equal to unknowns number." ) ;
    }

    R.resize( equations_set->size() ) ;

    for ( size_t i = 0, n = equations_set->size(); i < n; ++i )
    {
        R[i] = equations_set->getEquationValue(i, current_rhs) ;
    }

    for ( int icomp = 0; icomp < Component::NumberOfComponentTypes; ++icomp )
    {
        const Component& component = components[icomp] ;

        if ( !component.is_active )
            continue ;

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const int index = component.unknowns_beg + k ;
            vectorPlusColumn( R, -X[index], component, k ) ;
        }
    }
}

double System::computeTolerance( const Column& X0, const Column& X1 ) const
{
    static const std::string method_prefix = "System::computeTolerance( const Column&, const Column& ): " ;

    if ( X0.size() != X1.size() )
    {
        throw std::invalid_argument( method_prefix + "Error. Sizes of X0 and X1 aren't equal." ) ;
    }

    double dXdX = 0 ;
    double X0X0 = 0 ;

    for ( int i = 0, size = X0.size(); i < size; ++i )
    {
        dXdX += (X1[i] - X0[i])*(X1[i] - X0[i]) ;
        X0X0 += X0[i] * X0[i] ;
    }

    return ( X0X0 > FLT_MIN ) ? dXdX / X0X0 : FLT_MAX ;
}

void System::removeConstants( const std::vector<Component::Type>& components_types, Column& vector ) const
{
    if ( components_types.size() <= 1 )
        return ;

    // Check can we do removal or not
    for ( int icomp = 0; icomp < components_types.size(); ++icomp )
    {
        Component::Type type = components_types[icomp] ;
        const Component& component = components[type] ;

        if ( !component.has_unit_weight )
            return ;
    }

    // Average levels calculation
    double common_level = 0 ;
    int valid_components_num = 0 ;
    Column components_level( components_types.size(), 0 ) ;

    for ( int icomp = 0; icomp < components_types.size(); ++icomp )
    {
        Component::Type type = components_types[icomp] ;
        const Component& component = components[type] ;

        if ( component.unknowns_num <= 0 )
            continue ;

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const int index = component.unknowns_beg + k ;
            components_level[icomp] += vector[index] ;
        }

        components_level[icomp] /= component.unknowns_num ;
        common_level += components_level[icomp] ;
        valid_components_num++ ;
    }

    if ( valid_components_num == 0 )
        return ;

    common_level /= valid_components_num ;

    // Removal of constant shifts
    for ( int icomp = 0; icomp < components_types.size(); ++icomp )
    {
        Component::Type type = components_types[icomp] ;
        const Component& component = components[type] ;
        const double shift = common_level - components_level[icomp] ;

        for ( int k = 0; k < component.unknowns_num; ++k )
        {
            const int index = component.unknowns_beg + k ;
            vector[index] += shift ;
        }
    }
}

bool System::smoothComponent(const Component& component,
                             float smooth_window_x,
                             float smooth_window_y,
                             float threshold,
                             Column& vector) const
{
    static const std::string method_prefix = "System::smoothComponent(...): " ;

    using namespace Prime::lib::mathlib ;

    if ( smooth_window_x <= 0 && smooth_window_y <= 0 )
        return true ;

    const AxisMapper& mapper = mappers[component.axis] ;
    std::vector<Point> input( component.unknowns_num ) ;
    std::vector<Point> output( component.unknowns_num ) ;

    float xmin = +FLT_MAX ;
    float xmax = -FLT_MAX ;
    float ymin = +FLT_MAX ;
    float ymax = -FLT_MAX ;

    for ( int k = 0; k < component.unknowns_num; ++k )
    {
        float x, y ;
        const int index = component.unknowns_beg + k ;
        const int realcoord = mapper.getRealCoord( k ) ;

        switch (component.axis)
        {
            case SurfacePoint::nSp:
                equations_set->getCoder()->decodeSourceCoord( realcoord, x, y ) ;
                break ;

            case SurfacePoint::nRp:
                equations_set->getCoder()->decodeReceivCoord( realcoord, x, y ) ;
                break ;

            case SurfacePoint::Cmp:
                equations_set->getCoder()->decodeLineCdpCoord( realcoord, x, y ) ;
                break ;

            default:
                throw std::runtime_error( "Undefined component for smoothing." ) ;
        }

        xmin = std::min( xmin, x ) ;
        xmax = std::max( xmax, x ) ;
        ymin = std::min( ymin, y ) ;
        ymax = std::max( ymax, y ) ;

        input[k].x = output[k].x = x ;
        input[k].y = output[k].y = y ;
        input[k].z = output[k].z = vector[index] ;
    }

    try
    {
        const float xstep = equations_set->getCoder()->getCdpStep() ;
        const float ystep = equations_set->getCoder()->getLineStep() ;

        // Median filtering (with larger step for faster computations)
        PointsMedianFilter<Point> filter( threshold ) ;
        filter.set_num_threads(threads_num) ;
        filter.init( input, output, 3 * xstep, 3 * ystep ) ;
        filter.run( smooth_window_x, smooth_window_y ) ;
        input.swap( output ) ;

        // Median filter initialization for deltas filtering (robust smoothing)
        std::vector<Point> input_diff = input ;
        std::vector<Point> output_diff = input ;

        filter.setThreshold( 1.0 ) ;
        filter.init( input_diff, output_diff, 3 * xstep, 3 * ystep ) ;

        // Adding 4 edge point for avoiding edge effects
        output.push_back( Point( xmin - 0.5 * smooth_window_x, ymin - 0.5 * smooth_window_y, FLT_MAX ) ) ;
        output.push_back( Point( xmin - 0.5 * smooth_window_x, ymin + 0.5 * smooth_window_y, FLT_MAX ) ) ;
        output.push_back( Point( xmin + 0.5 * smooth_window_x, ymin - 0.5 * smooth_window_y, FLT_MAX ) ) ;
        output.push_back( Point( xmin + 0.5 * smooth_window_x, ymin + 0.5 * smooth_window_y, FLT_MAX ) ) ;

        PointsPlaneSmoothAlgorithm<Point> smoother( FLT_MAX ) ;
        smoother.set_num_threads(threads_num) ;
        smoother.init( input, output, xstep, ystep ) ;

        for ( int iter = 0; iter < DEFAULT_ROBUST_SMOOTH_ITER_NUM; ++iter )
        {
            smoother.run( smooth_window_x, smooth_window_y ) ;

            if ( iter == DEFAULT_ROBUST_SMOOTH_ITER_NUM - 1 )
                continue ;

            for ( int k = 0; k < component.unknowns_num; ++k )
                input_diff[k].z = fabsf( input[k].z - output[k].z ) ;

            filter.run( smooth_window_x, smooth_window_y ) ;

            bool was_edited = false ;
            for ( int k = 0; k < component.unknowns_num; ++k )
            {
                if ( input_diff[k].z > threshold * output_diff[k].z )
                {
                    was_edited = true ;
                    input[k].z = output[k].z ;
                }
            }

            if ( !was_edited )
                break ;
        }
    }
    catch ( const std::exception& err )
    {
        std::cerr << method_prefix << "Error. " << err.what() << std::endl ;
        return false ;
    }

    for ( int k = 0; k < component.unknowns_num; ++k )
    {
        const int index = component.unknowns_beg + k ;
        vector[index] = output[k].z ;
    }

    return true ;
}

bool System::solveLinearSystem( const Column& A, const Column& b, Column& x )
{
    using namespace Prime::lib::mathlib ;

    const int n = b.size() ;
    assert( n > 0 ) ;
    assert( A.size() == n * n ) ;

    std::vector<MKL_INT> ipiv( n ) ;

    Column LU = A ;
    if ( !decompose_LU(n, LU.data(), ipiv.data()) )
        return false ;

    x = b ;
    if ( !solve_LU('T', n, LU.data(), ipiv.data(), x.data()) )
        return false ;

    return true ;
}

}
}
}