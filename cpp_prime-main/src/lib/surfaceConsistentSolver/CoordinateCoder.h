/* 
 * File:   CoordinateCoder.h
 * Author: rganisimov
 *
 * Created on March 3, 2017, 5:44 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_COORDINATECODER_H
#define	PRIME_LIB_SURFACECONSISTENT_COORDINATECODER_H

#include "DbTools/ProfInfo.h"
#include "Bining/SelectIndex.h"
#include "Bining/SpRpIndexes.h"

using Prime::DB::ProfInfo ;
using Prime::Bining::SpIndex ;
using Prime::Bining::RpIndex ;

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

/**
 * @brief Coordinates coder.
 * @detailed This class is required for encoding/decoding
 * coordinates described by multi values to/from single value.
 * For example any cmp point is described by line/ncdp values
 * but we need to represent it by single value.
 */
class CoordinateCoder
{    
public:
    CoordinateCoder( const ProfInfo * prof ) ;

    /**
     * @name Methods for encoding/decoding line/ncdp.
     * @{
     */
public:
    int encodeLineCdp( int line, int ncdp ) const ;
    void decodeLineCdp( int index, int& line, int& ncdp ) const ;
    void decodeLineCdpCoord( int index, float& x, float& y ) const ;
    void getLineCdpByCoord( float x, float y, int& line, int& ncdp ) const ;
    float getCdpStep() const ;
    float getLineStep() const ;
    //! @}
    
    /**
     * @name Methods for encoding/decoding nSp/nRp.
     * @{
     */
public:
    void decodeSourceCoord( int index, float& x, float& y ) const ;
    void decodeReceivCoord( int index, float& x, float& y ) const ;
    //! @}

private:
    int cdp_num_per_line ;     //!< Number of cdp points per 3d line for encoding nline/ncdp by unique index.
    float cdp_step ;           //!< Metric step between cdp points.
    float line_step ;          //!< Metric setp between lines.
    SpIndex spIndex ;          //!< Sources index file (usable only for running on Grids).
    RpIndex rpIndex ;          //!< Receivers index file (usable only for running on Grids).
    Tree::LineInfo line_info ; //!< Line information (for 2D only).
    Tree::GridInfo grid_info ; //!< Grid information (for 3D only).
} ;

}
}
}

#endif

