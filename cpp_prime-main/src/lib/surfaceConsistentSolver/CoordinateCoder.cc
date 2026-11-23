
#include <stdexcept>

#include "CoordinateCoder.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {


CoordinateCoder::CoordinateCoder( const ProfInfo * prof )
{
    static const std::string method_prefix = "CoordinateCoder::CoordinateCoder( ProfInfo* ): " ;

    if ( !prof )
    {
        throw std::invalid_argument( method_prefix + "Error. Link to profile info is invalid.") ;
    }

    if ( !prof->if3d() && prof->GetLine2d() )
    {
        // Reading Sp indices
        try
        {
            spIndex.Read( prof->FS.FS->getDirectory(/*Prime::DATA2D,*/ Prime::INDICES, Prime::SURVEY2D, prof->name() ).c_str() ) ;
        }
        catch ( const std::exception& err )
        {
            std::cerr << err.what() << "Sp indexes information will be skipped." << std::endl ;
        }

        // Reading Rp indices
        try
        {
            rpIndex.Read( prof->FS.FS->getDirectory(/*Prime::DATA2D,*/ Prime::INDICES, Prime::SURVEY2D, prof->name() ).c_str() ) ;
        }
        catch ( const std::exception& err )
        {
            std::cerr << err.what() << "Rp indexes information will be skipped." << std::endl ;
        }

        cdp_num_per_line = prof->GetLine2d()->value().np ;
        cdp_step = prof->GetLine2d()->value().dp ;
        line_step = prof->GetLine2d()->value().dp ;
        prof->Select( line_info ) ;
    }
    else if ( prof->if3d() && prof->GetGrid() )
    {
        // Reading Sp indices
        spIndex.Read( prof->FS.FS->getDirectory(/*Prime::DATA3D,*/ Prime::INDICES, prof->name(Prime::DB::ProfInfo::GRID), "" ).c_str() ) ;

        // Reading Rp indices
        rpIndex.Read( prof->FS.FS->getDirectory(/*Prime::DATA3D,*/ Prime::INDICES, prof->name(Prime::DB::ProfInfo::GRID), "" ).c_str() ) ;

        cdp_num_per_line = prof->GetGrid()->value().nx ;
        cdp_step = prof->GetGrid()->value().dx ;
        line_step = prof->GetGrid()->value().dy ;
        prof->Select( grid_info ) ;
    }
    else
    {
        std::runtime_error( method_prefix + "Error. Incorrect processing object." ) ;
    }
}

int CoordinateCoder::encodeLineCdp( int line, int ncdp ) const
{
    return line * cdp_num_per_line + ncdp ;
}

void CoordinateCoder::decodeLineCdp( int index, int& line, int& ncdp ) const
{
    line = index / cdp_num_per_line ;
    ncdp = index % cdp_num_per_line ;
}

void CoordinateCoder::decodeLineCdpCoord( int index, float& x, float& y ) const
{
    int line, ncdp ;
    decodeLineCdp( index, line, ncdp ) ;
    x = ncdp * cdp_step ;
    y = line * line_step ;
}

void CoordinateCoder::getLineCdpByCoord( float x, float y, int& line, int& ncdp ) const
{
    if ( !line_info.empty() )
    {
        line = 0 ;
        ncdp = int( line_info.Picket(x, y)  / cdp_step + 0.4999 ) ;
    }
    else if ( !grid_info.empty() )
    {
        grid_info.CdpLine( x, y, &ncdp, &line ) ;
    }
    else
    {
        throw std::runtime_error( "Error. No line or grid information" ) ;
    }
}

float CoordinateCoder::getCdpStep() const
{
    return cdp_step ;
}

float CoordinateCoder::getLineStep() const
{
    return line_step ;
}

void CoordinateCoder::decodeSourceCoord( int index, float& x, float& y ) const
{
    const int k = spIndex.Find( index ) ;
    if ( k < 0 )
    {
        std::ostringstream message ;
        message << "Error. Can't find source index " << index ;
        message << " in the Sp.Index file. Please fix that and retry again." ;
        throw std::runtime_error( message.str() ) ;
    }

    x = spIndex.recs[k].x ;
    y = spIndex.recs[k].y ;
}

void CoordinateCoder::decodeReceivCoord( int index, float& x, float& y ) const
{
    const int k = rpIndex.Find( index ) ;
    if ( k < 0 )
    {
        std::ostringstream message ;
        message << "Error. Can't find receiver index " << index ;
        message << " in the Rp.Index file. Please fix that and retry again." ;
        throw std::runtime_error( message.str() ) ;
    }

    x = rpIndex.recs[k].x ;
    y = rpIndex.recs[k].y ;
}

}
}
}
