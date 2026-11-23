/* 
 * File:   EquationsSet.h
 * Author: ranisimov
 *
 * Created on December 3, 2019, 5:47 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_EQUATIONSSET_H
#define PRIME_LIB_SURFACECONSISTENT_EQUATIONSSET_H

#include <vector>
#include <deque>
#include <stdexcept>

#include "utils/methodname.h"

#include "SurfacePoint.h"
#include "CoordinateCoder.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

/**
 * @brief Class of equations set.
 */
class EquationsSet
{
    /**
     * @brief Storing type of values.
     */
public:
    typedef float value_type ;
    typedef std::vector<value_type> ValueVector ;

    /**
     * @brief Constructs equations set object.
     */
public:

    EquationsSet( const CoordinateCoder * __coder ) :
    m_coder(__coder),
    m_equations_rhs_num(0)
    {
        define_methodname() ;

        if ( !m_coder )
        {
            throw std::invalid_argument(__methodname__ + ": Error. Pointer to coordinates coder is invalid.") ;
        }
    }

    /**
     * @brief Checks presence of equations.
     */
public:

    inline bool empty() const
    {
        return m_surface_points.empty() ;
    }

    /**
     * @brief Checks presence of equations.
     */
public:

    inline int size() const
    {
        return m_surface_points.size() ;
    }

    /**
     * @brief Clears surface points.
     */
public:

    inline void clear()
    {
        m_equations_rhs_num = 0 ;
        m_surface_points.clear() ;
        m_values.clear() ;
    }

    /**
     * @brief Returns number of equations right hand sides.
     * @details For example cepstr is described by vector of
     * values. Number of cepstr basis function will be returned
     * by this method.
     */
public:

    inline int getRhsNum() const
    {
        return m_equations_rhs_num ;
    }
    
    /**
     * @brief Returns amount of used memory in Gb.
     */
public:
    double getMemoryUsageGb() const
    {
        double used_memory = 0 ;
        used_memory += sizeof(*this) ;
        used_memory += m_surface_points.size() * sizeof(SurfacePoint) ;
        used_memory += m_values.size() * sizeof(value_type) ;
        
        return 1.0 * used_memory / ( 1024 * 1024 * 1024 ) ;
    }

    /**
     * @brief Returns access to coordinates coder.
     */
public:

    inline const CoordinateCoder* getCoder() const
    {
        return m_coder ;
    }

    /**
     * @brief Returns surface point of equation.
     * @param[in] __equation_index Index of equation.
     */
public:

    inline SurfacePoint& getEquationPoint( int __equation_index )
    {
        define_methodname() ;

        if ( __equation_index < 0 || __equation_index >= m_surface_points.size() )
        {
            std::ostringstream message ;
            message << __methodname__ << ": Error. Requested index of equation "
                    << "is out of range. Requested index is " << __equation_index
                    << " but number of equations is " << size() << "." ;

            throw std::out_of_range( message.str() ) ;
        }

        return m_surface_points[__equation_index] ;
    }

    /**
     * @brief Returns right hand side value of equation.
     * @param[in] __equation_index Index of equation.
     * @param[in] __rhs_index Index of right side vector.
     */
public:

    inline value_type getEquationValue( size_t __equation_index, int __rhs_index ) const
    {
        define_methodname() ;

        const size_t value_index = getRhsNum() * __equation_index + __rhs_index ;

        if ( value_index < 0 || value_index >= m_values.size() )
        {
            std::ostringstream message ;
            message << __methodname__ << ": Error. Requested index of value is out "
                    << "of range. Value index is " << value_index << " but number "
                    << "of values is " << m_values.size() << ". Requested equation "
                    << "index is " << __equation_index << " but available only "
                    << size() << " equations. Requested right side vector index is "
                    << __rhs_index << " but available one is " << getRhsNum() << "." ;

            throw std::out_of_range( message.str() ) ;
        }

        return m_values[value_index] ;
    }

    /**
     * @brief Adds equations to equations set.
     */
public:

    void addEquations(const std::vector<SurfacePoint>& __equations_points,
                      const std::vector<ValueVector>& __equations_values)
    {
        define_methodname() ;

        // Checking input data
        if (__equations_points.empty())
            return ;

        if (__equations_points.size() != __equations_values.size())
        {
            std::ostringstream message ;
            message << __methodname__ << ": Error. Number of equations points is "
                    << __equations_points.size() << " but number of equations values "
                    << "vectors is " << __equations_values.size() << "." ;

            throw std::runtime_error( message.str() ) ;
        }

        // Adding new equations
        for ( int i = 0; i < __equations_points.size(); ++i )
        {
            if ( __equations_values[i].empty() )
                continue ;
            
            if ( empty() )
            {
                m_equations_rhs_num = __equations_values.front().size() ;
            }

            if ( __equations_values[i].size() != getRhsNum() )
            {
                std::ostringstream message ;
                message << __methodname__ << ": Error. "
                        << "Input data contains value vectors of different length. "
                        << "Probably you have computed input files by different tasks. "
                        << "Please fix that for normal computation. "
                        << "Size of input equation value vector is " << __equations_values[i].size()
                        << " but number of right hand side vectors is " << getRhsNum() << "." ;

                throw std::runtime_error( message.str() ) ;
            }

            m_surface_points.push_back(__equations_points[i]) ;
            m_values.insert(m_values.end(), __equations_values[i].begin(), __equations_values[i].end()) ;
        }
    }

private:
    const CoordinateCoder * m_coder ;            //!< Link to coordinates coder.
    int m_equations_rhs_num ;                    //!< Number of equations right hand sides (of cepstr for example). Will be filled by real size only after first read() calling.
    std::deque<SurfacePoint> m_surface_points ;  //!< Surface points list. One surface points describes one equation.
    std::deque<value_type> m_values ;            //!< Shared buffer of right hand values of equations.
} ;

}
}
}

#endif /* PRIME_LIB_SURFACECONSISTENT_EQUATIONSSET_H */

