/* 
 * File:   AxisMapper.h
 * Author: rganisimov
 *
 * Created on February 28, 2017, 3:30 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_AXISMAPPER_H
#define	PRIME_LIB_SURFACECONSISTENT_AXISMAPPER_H

#include <map>
#include <vector>

#include "SurfacePoint.h"
#include "EquationsSet.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

/**
 * @brief Axis values mapper.
 * @detaield Special class for re-indexation
 * real coordinates by sequential indexes for internal
 * usage and linking coordinates to equations.
 */
class AxisMapper
{
    typedef std::vector<int> EquationsList ;
    
public:
    AxisMapper() {}
    
    /**
     * Mapper clearing.
     */
public:
    void clear()
    {
        real_to_index.clear() ;
        index_to_real.clear() ;
        index_to_equations.clear() ;
    }
    
    /**
     * Checking for an empty state.
     */
public:
    bool empty() const
    {
        return real_to_index.empty() ;
    }
    
    /**
     * Getting of coordinates unique values.
     */
public:
    int size() const
    {
        return index_to_real.size() ;
    }
    
    /**
     * Method for mapper initialization
     * and relinking real coordinates.
     * @param type Type of axis which has to be mapped.
     * @param equations List of input picks, i.e. equations.
     */
public:
    void initAndRelink( SurfacePoint::Axis type, EquationsSet& equations_set ) ;
    
    /**
     * Getting equations associated with required coordinate.
     * @param index Sequential index by which coordinate has been encoded.
     */
public:
    const std::vector<int>& getEquations( int index ) const ;
    
    /**
     * Getting real coordinate by sequential index.
     * @param index Sequential index by which coordinate has been encoded.
     */
public:
    int getRealCoord( int index ) const ;
    
    /**
     * Getting sequential index by real coordinate.
     * @param coordinate Real coordinate.
     */
public:
    int getIndex( int coordinate ) const ;
    
private:
    SurfacePoint::Axis type ;                       //!< Type of axis which is mapped by this mapper.
    std::map<int,int> real_to_index ;               //!< Map which stores mapping of real coordinates to indexes.
    std::vector<int> index_to_real ;                //!< Index to real mapping.
    std::vector<EquationsList> index_to_equations ; //!< Index to equations mapping.
};

}
}
}

#endif

