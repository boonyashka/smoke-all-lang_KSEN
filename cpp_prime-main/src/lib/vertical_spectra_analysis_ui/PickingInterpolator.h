/* 
 * File:   PickingInterpolator.h
 * Author: ranisimov
 *
 * Created on August 4, 2023, 10:20 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_PICKING_INTERPOLATOR_H
#define VERTICAL_SPECTRA_ANALYSIS_PICKING_INTERPOLATOR_H

#include "mathlib/algorithms/interpolation.h"

#include "vertical_spectra_analysis/PickingPoint.h"

#include "DataStorage.h"

namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

PickingPoint interpolate_picking_point(const DataStorage::DataPoint& __point,
                                       const PickingPoint * __p0,
                                       const PickingPoint * __p1)
{
    using namespace Prime::lib::mathlib ;

    if ( !__p0 && !__p1 )
    {
        return PickingPoint(__point.ncdp, __point.x, __point.y) ;
    }

    if ( __p0 && __p1 && __p0->ncdp() > __p1->ncdp() )
    {
        std::swap( __p0, __p1 ) ;
    }

    if ( __p0 && (!__p1 || __p1->empty() || __point.ncdp <= __p0->ncdp()) )
    {
        PickingPoint p(__point.ncdp, __point.x, __point.y) ;
        p.curve() = __p0->curve() ;

        return p ;
    }

    if ( __p1 && (!__p0 || __p0->empty() || __point.ncdp >= __p1->ncdp()) )
    {
        PickingPoint p(__point.ncdp, __point.x, __point.y) ;
        p.curve() = __p1->curve() ;

        return p ;
    }

    PickingPoint p(__point.ncdp, __point.x, __point.y) ;

    if ( __p0->empty() && __p1->empty() )
        return p ;

    const int ncdp_range = __p1->ncdp() - __p0->ncdp() ;
    const double w = (ncdp_range > 0) ? 1.0 * (p.ncdp() - __p0->ncdp()) / ncdp_range : 0 ;

    // Gathering all points in sorted order
    std::vector<double> t ;

    for ( int i0 = 0, i1 = 0; ; )
    {
        double y = 0 ;

        if (i0 < __p0->curve().size() &&
            i1 < __p1->curve().size())
        {
            y = (__p0->curve()[i0].y <= __p1->curve()[i1].y)
                    ? __p0->curve()[i0++].y
                    : __p1->curve()[i1++].y ;
        }
        else if (i0 < __p0->curve().size())
        {
            y = __p0->curve()[i0++].y ;
        }
        else if (i1 < __p1->curve().size())
        {
            y = __p1->curve()[i1++].y ;
        }
        else break ;

        if ( t.empty() || y > t.back() + FLT_EPSILON )
        {
            t.push_back( y ) ;
        }
    }
    
    // Resizing output curve and buffer prepare
    std::vector<double> v( t.size() ) ;
    p.curve().resize( t.size() ) ;

    // Left side values getting at output
    // points and adding to result
    std::vector<double> t0( __p0->curve().size() ) ;
    std::vector<double> v0( __p0->curve().size() ) ;
    
    for ( int i = 0; i < __p0->curve().size(); ++i )
    {
        t0[i] = __p0->curve()[i].y ;
        v0[i] = __p0->curve()[i].x ;
    }
    
    linear_reinterpolation(t0.size(), t0.data(), v0.data(),
                           t.size(),  t.data(),  v.data()) ;
    
    for ( int i = 0; i < t.size(); ++i )
    {
        p.curve()[i].y = t[i] ;
        p.curve()[i].x = (1 - w) * v[i] ;
    }

    // Right side values getting at output
    // points and adding to result
    std::vector<double> t1( __p1->curve().size() ) ;
    std::vector<double> v1( __p1->curve().size() ) ;
    
    for ( int i = 0; i < __p1->curve().size(); ++i )
    {
        t1[i] = __p1->curve()[i].y ;
        v1[i] = __p1->curve()[i].x ;
    }
    
    linear_reinterpolation(t1.size(), t1.data(), v1.data(),
                           t.size(),  t.data(),  v.data()) ;
    
    for ( int i = 0; i < t.size(); ++i )
    {
        p.curve()[i].x += w * v[i] ;
    }

    return p ;
}

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_PICKING_INTERPOLATOR_H */

