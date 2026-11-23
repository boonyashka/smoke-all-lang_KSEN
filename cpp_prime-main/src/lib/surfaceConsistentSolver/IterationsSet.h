/* 
 * File:   IterationsSet.h
 * Author: rganisimov
 *
 * Created on February 25, 2017, 4:55 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_ITERATIONSSET_H
#define	PRIME_LIB_SURFACECONSISTENT_ITERATIONSSET_H

#include <vector>
#include <float.h>
#include "SurfaceComponent.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

class System ; // Solver predefinition

/**
 * @brief Single iteration descriptor.
 * @detailed Class for describing one iteration of the solver,
 * i.e. is possible to describe which components should be
 * updated and which tools we want to use for do that.
 */
class IterationInfo
{
    friend class System ;
    typedef std::vector<Component::Type> ComponetsList ;

    /**
     * Types of iterations.
     */
public:

    enum Type
    {
        GaussSeidelSolver,
        ConjugateGradientsSolver,
        StructAndRnmoSolver
    } ;

public:

    IterationInfo() :
    type(GaussSeidelSolver),
    iterations_max_num( 1 ),
    apply_smoothing( false ),
    smooth_window_x( 1 ),
    smooth_window_y( 1 ),
    robustness_threshold( 1 ) { }

    Type getType() const
    {
        return type ;
    }

    /**
     * @name Functions for creating predefined iterations.
     * @{ 
     */
public:
    static IterationInfo makeGaussSeidel(const ComponetsList& _components,
                                         bool apply_smoothing,
                                         float smooth_window_x,
                                         float smooth_window_y,
                                         float robustness_threshold,
                                         int iterations_max_num = 1000) ;

    static IterationInfo makeConjugateGradients(const ComponetsList& _components,
                                                bool apply_smoothing,
                                                float smooth_window_x,
                                                float smooth_window_y,
                                                float robustness_threshold,
                                                int iterations_max_num = 1000) ;

    static IterationInfo makeComputeStruct(bool apply_smoothing,
                                           float smooth_window_x,
                                           float smooth_window_y,
                                           float robustness_threshold,
                                           int iterations_max_num = 10) ;

    static IterationInfo makeComputeStructAndRnmo2d(bool apply_smoothing,
                                                    float smooth_window_x,
                                                    float smooth_window_y,
                                                    float robustness_threshold) ;

    static IterationInfo makeComputeStructAndRnmo3d(bool apply_smoothing,
                                                    float smooth_window_x,
                                                    float smooth_window_y,
                                                    float robustness_threshold) ;
    //! @}

protected:
    Type type ;                         //!< Type of iteration
    ComponetsList updating_components ; //!< Components which have to be updated.

    // Common parameters for all types
    // (not all usable for all types).
    int iterations_max_num ;            //!< Maximum number of Gauss-Seidel iterations.
    bool apply_smoothing ;              //!< True if smooting is required.
    float smooth_window_x ;             //!< Component smoothing window along x (inlines).
    float smooth_window_y ;             //!< Component smoothing window along y (xlines).
    float robustness_threshold ;        //!< Threshold for making robust estimations (robust smoothing).
} ;

class IterationsSet
{
public:

    void clear()
    {
        iterations.clear() ;
    }

    void add( const IterationInfo& _iteration, int repeats_max_num = 1 )
    {
        for ( int i = 0; i < repeats_max_num; ++i )
        {
            iterations.push_back( _iteration ) ;
        }
    }

    void add( const std::vector<IterationInfo>& _iterations, int repeats_max_num = 1 )
    {
        for ( int i = 0; i < repeats_max_num; ++i )
        {
            iterations.insert( iterations.end(), _iterations.begin(), _iterations.end() ) ;
        }
    }

    const std::vector<IterationInfo>& get() const
    {
        return iterations ;
    }

protected:
    std::vector<IterationInfo> iterations ; //!< List of iterations which should be done.
} ;

IterationInfo IterationInfo::makeGaussSeidel(const ComponetsList& _components,
                                             bool apply_smoothing,
                                             float smooth_window_x,
                                             float smooth_window_y,
                                             float robustness_threshold,
                                             int iterations_max_num)
{
    IterationInfo info ;
    info.type = IterationInfo::GaussSeidelSolver ;
    info.updating_components = _components ;
    info.iterations_max_num = iterations_max_num ;
    info.apply_smoothing = apply_smoothing ;
    info.smooth_window_x = std::max( smooth_window_x, 0.f ) ;
    info.smooth_window_y = std::max( smooth_window_y, 0.f ) ;
    info.robustness_threshold = std::max( robustness_threshold, 1.f ) ;
    return info ;
}

IterationInfo IterationInfo::makeConjugateGradients(const ComponetsList& _components,
                                                    bool apply_smoothing,
                                                    float smooth_window_x,
                                                    float smooth_window_y,
                                                    float robustness_threshold,
                                                    int iterations_max_num)
{
    IterationInfo info ;
    info.type = IterationInfo::ConjugateGradientsSolver ;
    info.updating_components = _components ;
    info.iterations_max_num = iterations_max_num ;
    info.apply_smoothing = apply_smoothing ;
    info.smooth_window_x = std::max( smooth_window_x, 0.f ) ;
    info.smooth_window_y = std::max( smooth_window_y, 0.f ) ;
    info.robustness_threshold = std::max( robustness_threshold, 1.f ) ;
    return info ;
}

IterationInfo IterationInfo::makeComputeStruct(bool apply_smoothing,
                                               float smooth_window_x,
                                               float smooth_window_y,
                                               float robustness_threshold,
                                               int iterations_max_num)
{
    IterationInfo info ;
    info.type = IterationInfo::StructAndRnmoSolver ;
    info.updating_components.clear() ;
    info.updating_components.push_back( Component::Struct ) ;
    info.iterations_max_num = iterations_max_num ;
    info.apply_smoothing = apply_smoothing ;
    info.smooth_window_x = std::max( smooth_window_x, 0.f ) ;
    info.smooth_window_y = std::max( smooth_window_y, 0.f ) ;
    info.robustness_threshold = std::max( robustness_threshold, 1.f ) ;
    return info ;
}

IterationInfo IterationInfo::makeComputeStructAndRnmo2d(bool apply_smoothing,
                                                        float smooth_window_x,
                                                        float smooth_window_y,
                                                        float robustness_threshold)
{
    IterationInfo info ;
    info.type = IterationInfo::StructAndRnmoSolver ;
    info.updating_components.clear() ;
    info.updating_components.push_back( Component::Struct ) ;
    info.updating_components.push_back( Component::RnmoL2 ) ;
    info.iterations_max_num = 10 ;
    info.apply_smoothing = apply_smoothing ;
    info.smooth_window_x = std::max( smooth_window_x, 0.f ) ;
    info.smooth_window_y = std::max( smooth_window_y, 0.f ) ;
    info.robustness_threshold = std::max( robustness_threshold, 1.f ) ;
    return info ;
}

IterationInfo IterationInfo::makeComputeStructAndRnmo3d(bool apply_smoothing,
                                                        float smooth_window_x,
                                                        float smooth_window_y,
                                                        float robustness_threshold)
{
    IterationInfo info ;
    info.type = IterationInfo::StructAndRnmoSolver ;
    info.updating_components.clear() ;
    info.updating_components.push_back( Component::Struct ) ;
    info.updating_components.push_back( Component::RnmoLxLx ) ;
    info.updating_components.push_back( Component::RnmoLxLy ) ;
    info.updating_components.push_back( Component::RnmoLyLy ) ;
    info.iterations_max_num = 10 ;
    info.apply_smoothing = apply_smoothing ;
    info.smooth_window_x = std::max( smooth_window_x, 0.f ) ;
    info.smooth_window_y = std::max( smooth_window_y, 0.f ) ;
    info.robustness_threshold = std::max( robustness_threshold, 1.f ) ;
    return info ;
}

}
}
}

#endif