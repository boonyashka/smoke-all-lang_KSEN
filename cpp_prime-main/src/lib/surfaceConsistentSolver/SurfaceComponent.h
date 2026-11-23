/* 
 * File:   SurfaceComponent.h
 * Author: rganisimov
 *
 * Created on February 24, 2017, 5:46 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_SURFACECOMPONENT_H
#define	PRIME_LIB_SURFACECONSISTENT_SURFACECOMPONENT_H

#include "SurfacePoint.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

class System ; // Solver predefinition

/**
 * @brief Surface component (term) descriptor.
 * @detailed Class for surface component
 * description. It is possible to bind
 * component to required coordinate axis (source
 * component can be binded to source point number
 * for example).
 */
class Component
{
    friend class System ;

    /**
     * List of available types of surface component,
     * i.e. surface-consistent term. If you want to
     * add new term you have to start from adding it
     * to this enumeration.
     */
public:

    enum Type
    {
        Source,   //!< Source point term (source statics for example).
        Receiv,   //!< Receiver point term (receiver statics for example).
        Struct,   //!< Structural term (structural statics for example).
        Kx,       //!< X-offset term.
        Ky,       //!< Y-offset term.
        RnmoL2,   //!< L*L offset 2d term.
        RnmoLxLx, //!< Lx*Lx offset term.
        RnmoLxLy, //!< Lx*Ly offset term.
        RnmoLyLy, //!< Ly*Ly offset term.
        NumberOfComponentTypes
    } ;

public:

    Component( Type _type = Source ) :
    type(_type),
    is_active(false),
    axis(SurfacePoint::nSp),
    unknowns_beg(0),
    unknowns_num(0)
    {
        switch (type)
        {
            case Source:
                axis = SurfacePoint::nSp ;
                has_unit_weight = true ;
                break ;

            case Receiv:
                axis = SurfacePoint::nRp ;
                has_unit_weight = true ;
                break ;

            case Struct:
                axis = SurfacePoint::Cmp ;
                has_unit_weight = true ;
                break ;

            case Kx:
                axis = SurfacePoint::Kx ;
                has_unit_weight = true ;
                break ;

            case Ky:
                axis = SurfacePoint::Ky ;
                has_unit_weight = true ;
                break ;

            case RnmoL2:
            case RnmoLxLx:
            case RnmoLxLy:
            case RnmoLyLy:
                axis = SurfacePoint::Cmp ;
                has_unit_weight = false ;
                break ;
        }
    }

private:
    Type type ;               //!< Type of current component.
    bool is_active ;          //!< Is current component active, i.e. have to be updated?
    SurfacePoint::Axis axis ; //!< To which coordinate axis binded this component.
    int unknowns_beg ;        //!< Begin index of current component in an unknowns buffer.
    int unknowns_num ;        //!< Number of unknowns (i.e. number of elements with unique axis value).
    bool has_unit_weight ;    //!< True if weight of component in equation is 1.
} ;

}
}
}

#endif

