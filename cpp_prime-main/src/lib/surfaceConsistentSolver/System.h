/* 
 * File:   Solver.h
 * Author: rganisimov
 *
 * Created on February 27, 2017, 7:13 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_SOLVER_H
#define PRIME_LIB_SURFACECONSISTENT_SOLVER_H

#include <string>
#include <vector>
#include <float.h>

#include "SurfacePoint.h"
#include "SurfaceComponent.h"
#include "AxisMapper.h"
#include "IterationsSet.h"
#include "EquationsSet.h"
#include "LibStorage.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

/**
 * @brief Surface consistent linear equations system.
 * @detailed Class for solving surface consistent problems.
 * This solver is an iterative and uses information about iterations
 * from IterationsSet class, i.e. which types of solving algorithms
 * and in which order are required to do for solving the problem.
 */
class System
{
    typedef SurfacePoint::Axis Axis ;                    //!< Surface axis type.
    typedef float value_type ;                           //!< Value type.
    typedef std::vector<value_type> Column ;             //!< Type of vector-column.
    typedef void (*ProgressHandler)(int id, int value) ; //!< Prototype of function for progress changing.

    static const char MKL_UPLO ;                         //!< Mark of MKL upper triangle matrix.
    static const int DEFAULT_RESIDUALS_REFRESH_ITERS ;   //!< Number of iterations for refresing residual vector, i.e. right hand of equations.
    static const int DEFAULT_ROBUST_SMOOTH_ITER_NUM ;    //!< Number of iterations for robust smoothing.
    static const int DEFAULT_LSM_EQUATIONS_MIN_NUM ;     //!< Minimal equations number for LSM solving.
    static const double DEFAULT_LSM_REGULARIZATION ;     //!< Regularization factor for LSM solving.
    static const double DEFAULT_SOLUTION_TOLERANCE ;     //!< Criterion for convergence detection.
    static const double DEFAULT_NARROW_AZIMUTH_RATIO ;   //!< Lx/Ly or Ly/Lx offsets ratio which determs narrow azimuth data.

public:
    System( int _threads_num = 1 ) ;

    /**
     * Method for initializing solver.
     * @param _equations_reader Input equations reader.
     * @param _iterations Solving iterating scheme.
     */
public:
    bool init( EquationsSet& _equations_set, const IterationsSet& _iterations ) ;

    /**
     * Method for system solving.
     * @param result Library to which we will put answer.
     * @param right_hand_side_index Index of right hand side which should be used for solving.
     */
public:
    bool solve( LibStorage& result, int right_hand_side_index = 0 ) ;

    /**
     * Method for setting progress setter.
     * @param _progress_object_id Id of object which want to receive progress signal.
     * @param _progress_handler Function for setting progress state.
     */
public:

    void setProgressHandler(int _progress_object_id, ProgressHandler _progress_handler )
    {
        progress_object_id = _progress_object_id ;
        progress_handler = _progress_handler ;
    }

    /**
     * Make iteration according to its type.
     * @param info Iteration descriptor.
     * @param X Current solution which will be updated.
     * @param error Error description.
     */
private:
    bool makeIteration( const IterationInfo& info, Column& X, Column& R, std::string& error ) const ;

    /**
     * @name Solving algorithms.
     * @{
     */
private:
    bool makeGaussSeidel( const IterationInfo& info, Column& X, Column& R, std::string& error ) const ;
    bool makeConjugateGradients( const IterationInfo& info, Column& X, Column& R, std::string& error ) const ;
    bool makeComputeStructAndRnmo( const IterationInfo& info, Column& X, Column& R, std::string& error ) const ;
    //! @}

    /**
     * @name Algorithms utilities
     * @{
     */
private:

    /**
     * Getting matrix value, i.e. Aij.
     * Equation index is a i-index and j-index
     * will constructed by component and local index.
     * @param equation_index Index of equation.
     * @param component Component for which we get value.
     */
    inline double getMatrixValue( int equation_index, const Component& component ) const ;

    /**
     * Calculates scalar product of k-th column, i.e. Aj*Aj.
     * @param component Component for which we calculate product.
     * @param index Local index of component.
     */
    inline double columnSqrLength( const Component& component, int index ) const ;

    /**
     * Calculates scalar product of k-th column and vector, i.e. Aj*R.
     * @param component Component for which we calculate product.
     * @param index Local index of component.
     * @param vector Vector which should be multiplied by column.
     */
    inline double columnDotVector( const Component& component, int index, const Column& vector ) const ;

    /**
     * Adds to vector column multiplied by constant, i.e. R += k*Aj.
     * @param vector Vector which should be changed.
     * @param scalar Scalar by which column will be multiplied.
     * @param component Component by which we change vector.
     * @param index Local index of component.
     */
    inline void vectorPlusColumn( Column& vector, double scalar, const Component& component, int index ) const ;

    /**
     * Calculates residuals vector, i.e. right
     * hand of equations.
     * @param X Link to current solution.
     * @param R Link to result vector.
     */
    void computeResiduals( const Column& X, Column& R ) const ;

    /**
     * Calculates relative tolerance.
     * @param X0 Link to previous vector.
     * @param X1 Link to current vector.
     */
    double computeTolerance( const Column& X0, const Column& X1 ) const ;

    /**
     * Removes constant shifts if possible.
     * @param components_types List of components for which constant removal will be produced.
     * @param vector Values vector from which we want remove constant.
     */
    void removeConstants( const std::vector<Component::Type>& components_types, Column& vector ) const ;

    /**
     * Makes robust smoothing of component values
     * using spatial filtering by linear plane operator.
     * @param component Component which has to be smoothed. 
     * @param smooth_window_x Smoothing operator size along x.
     * @param smooth_window_y Smoothing operator size along y.
     * @param threshold Emissions correction threshold for robust smoothing.
     * @param vector Full vector of unknowns. Values range described by component will be smoothed.
     */
    bool smoothComponent( const Component& component, float smooth_window_x, float smooth_window_y, float threshold, Column& vector ) const ;

    /**
     * Solves linear system Ax = b by LU factorization.
     * @attention Size of system will be received from size of b vector.
     * @param A Rectangular matrix of coefficients linearized along rows.
     * @param b Right hand values. Size of b determines system size.
     * @param x Link to result vector.
     */
    static bool solveLinearSystem( const Column& A, const Column& b, Column& x ) ;
    //! @}

private:
    int threads_num ;                                         //!< Number of threads available for computations.
    int unknowns_num ;                                        //!< Number of unknowns which we want to determ.
    int current_rhs ;                                         //!< Index of current right hand side.
    EquationsSet * equations_set ;                            //!< Link to equations set.
    IterationsSet iterations ;                                //!< Iterations order descriptor.
    Component components[Component::NumberOfComponentTypes] ; //!< Components descriptor.
    AxisMapper mappers[SurfacePoint::NumberOfAxes] ;          //!< Axis to equations mappers.
    int progress_object_id ;                                  //!< Id of object to which we will send progress signal.
    ProgressHandler progress_handler ;                        //!< Function of progress changing.
} ;

double System::getMatrixValue(int equation_index, const Component& component) const
{
    static const std::string method_prefix = "Solver::getMatrixValue(int, const Component&): " ;

    // For Source, Receiver, Struct, Kx, Ky components
    if ( component.has_unit_weight )
        return 1.0 ;

    switch (component.type)
    {
        case Component::RnmoL2:
            return equations_set->getEquationPoint(equation_index).l2() ;

        case Component::RnmoLxLx:
            return equations_set->getEquationPoint(equation_index).lxlx() ;

        case Component::RnmoLxLy:
            return equations_set->getEquationPoint(equation_index).lxly() ;

        case Component::RnmoLyLy:
            return equations_set->getEquationPoint(equation_index).lyly() ;
    }

    throw std::runtime_error( method_prefix + "Error. Undefined component." ) ;
    return 1.0 ;
}

}
}
}

#endif

