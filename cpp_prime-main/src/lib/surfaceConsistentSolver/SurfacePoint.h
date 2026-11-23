/* 
 * File:   SurfacePoint.h
 * Author: rganisimov
 *
 * Created on February 24, 2017, 4:45 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_SURFACEPOINT_H
#define	PRIME_LIB_SURFACECONSISTENT_SURFACEPOINT_H

#include <vector>

#include "Statics/shift.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

/**
 * @brief Descriptor of surface point
 * @detailed Class for trace location description
 * and value (time shift, amplitude etc.) assigning.
 */
class SurfacePoint
{    
    /**
     * @brief List of available coordinate components.
     * @details List of available coordinate components,
     * i.e. it is possible to positioning trace
     * by source number, receiver number, cmp position,
     * source-receiver offsets along x and y etc.
     * If you need to add new coordinate component
     * just modify this enumerator.
     */
public:

    enum Axis
    {
        nSp,         //!< Source point number
        nRp,         //!< Receiver point number
        Cmp,         //!< Cmp point number
        Kx,          //!< Source-receiver offset index along x
        Ky,          //!< Source-receiver offset index along y
        NumberOfAxes
    } ;
    
    /**
     * @name Setters interface.
     * @{
     *************************/
    inline void setCoordinate( int __type, int __value )
    {
        m_coordinate[__type] = __value ;
    }
    
    inline void setLx(float __value)
    {
        m_lx = __value / statCont::RNMO_OFFSET_NORMALIZER ;
    }
    
    inline void setLy(float __value)
    {
        m_ly = __value / statCont::RNMO_OFFSET_NORMALIZER ;
    }
    //! @}

    /**
     * @name Getters interface.
     * @{
     *************************/
public:
    
    inline int coordinate( int __type ) const
    {
        return m_coordinate[__type] ;
    }
    
    inline float lx() const
    {
        return m_lx ;
    }
    
    inline float ly() const
    {
        return m_ly ;
    }

    inline double l2() const
    {
        return 1.0 * m_lx * m_lx + m_ly * m_ly ;
    }

    inline double lxlx() const
    {
        return 1.0 * m_lx * m_lx ;
    }
    
    inline double lxly() const
    {
        return 1.0 * m_lx * m_ly ;
    }

    inline double lyly() const
    {
        return 1.0 * m_ly * m_ly ;
    }
    //! @}
    
private:
    int m_coordinate[NumberOfAxes] ; //!< Point coordinate vector.
    float m_lx ;                     //!< Source-receiver offset along x.
    float m_ly ;                     //!< Source-receiver offset along y.
} ;

}
}
}

#endif

