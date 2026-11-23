
#include <stdexcept>

#include "dmolib/dmogates.h"
#include "dmolib/dmonamesrv.h"
#include "files/Commondir.h"
#include "files/FilesOperations.h"

#include "LibStorage.h"


namespace Prime {
namespace lib {
namespace SurfaceConsistent {

// ------------------------------------------
//           StaticsLibStorage
// ------------------------------------------

void StaticsLibStorage::clear()
{
    statics.reset_spStat() ;
    statics.reset_rpStat() ;
    statics.reset_structStat() ;
    statics.reset_rNmo2dStat() ;
    statics.reset_rNmo3dLxStat() ;
    statics.reset_rNmo3dLyStat() ;
    statics.reset_rNmo3dLxyStat() ;
}

void StaticsLibStorage::clear(Component::Type type)
{
    static const std::string method_prefix = "StaticsLibStorage::clear(Component::Type): " ;

    switch ( type )
    {
        case Component::Source:
            statics.reset_spStat() ;
            break;

        case Component::Receiv:
            statics.reset_rpStat() ;
            break ;

        case Component::Struct:
            statics.reset_structStat() ;
            break ;

        case Component::Kx:
            break ;

        case Component::Ky:
            break ;

        case Component::RnmoL2:
            statics.reset_rNmo2dStat() ;
            break ;

        case Component::RnmoLxLx:
            statics.reset_rNmo3dLxStat() ;
            break ;

        case Component::RnmoLxLy:
            statics.reset_rNmo3dLxyStat() ;
            break ;

        case Component::RnmoLyLy:
            statics.reset_rNmo3dLyStat() ;
            break ;

        default:
            throw std::invalid_argument( method_prefix + "Error. Undefined surface component has been received." ) ;
    }
}

float StaticsLibStorage::get(Component::Type type, int index) const
{
    static const std::string method_prefix = "StaticsLibStorage::get(Component::Type, int): " ;

    float value = 0 ;

    switch ( type )
    {
        case Component::Source:
            statics.get_spStat( index, &value ) ;
            break ;

        case Component::Receiv:
            statics.get_rpStat( index, &value ) ;
            break ;

        case Component::Struct:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.get_structStat( line, ncdp, &value ) ;
        }
            break ;

        case Component::Kx:
            break ;

        case Component::Ky:
            break ;

        case Component::RnmoL2:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.get_rNmo2dStat( line, ncdp, &value ) ;
        }
            break ;

        case Component::RnmoLxLx:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.get_rNmo3dLxStat( line, ncdp, &value ) ;
        }
            break ;

        case Component::RnmoLxLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.get_rNmo3dLxyStat( line, ncdp, &value ) ;
        }
            break ;

        case Component::RnmoLyLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.get_rNmo3dLyStat( line, ncdp, &value ) ;
        }
            break ;

        default:
            throw std::invalid_argument( method_prefix + "Error. Undefined surface component has been received." ) ;
    }

    return value / 1000 ; // Convert to sec
}

void StaticsLibStorage::set(Component::Type type, int index, float value)
{
    static const std::string method_prefix = "StaticsLibStorage::set(Component::Type, int, float): " ;

    value = 1000 * value ; // Convert to msec

    switch ( type )
    {
        case Component::Source:
            statics.set_spStat( index, value ) ;
            break ;

        case Component::Receiv:
            statics.set_rpStat( index, value ) ;
            break ;

        case Component::Struct:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.set_structStat( line, ncdp, value ) ;
        }
            break ;

        case Component::Kx:
            break ;

        case Component::Ky:
            break ;

        case Component::RnmoL2:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.set_rNmo2dStat( line, ncdp, value ) ;
        }
            break ;

        case Component::RnmoLxLx:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.set_rNmo3dLxStat( line, ncdp, value ) ;
        }
            break ;

        case Component::RnmoLxLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.set_rNmo3dLxyStat( line, ncdp, value ) ;
        }
            break ;

        case Component::RnmoLyLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            statics.set_rNmo3dLyStat( line, ncdp, value ) ;
        }
            break ;

        default:
            throw std::invalid_argument( method_prefix + "Error. Undefined surface component has been received." ) ;
    }
}

bool StaticsLibStorage::write(const std::string& filename) const
{
    static const std::string method_prefix = "StaticsLibStorage::write(const std::string&): " ;

    try
    {
        const std::string path = Prime::Files::getFileDir( filename ) ;

        if ( !Prime::Files::Commondir::directoryExists(path) )
            Prime::Files::Commondir::makeDirectory( path, false ) ;

        FILE *output_file = fopen( filename.c_str(), "wt" ) ;

        if ( !output_file )
        {
            throw std::runtime_error( "Error. Can't open statics output file '" + filename + "'." ) ;
        }

        statics.write_base( output_file ) ;
        fclose( output_file ) ;
    }
    catch ( const std::exception& err )
    {
        std::cerr << method_prefix << err.what() << std::endl ;
        return false ;
    }

    return true ;
}

bool StaticsLibStorage::writeToLine(const Commondir* cd, const std::string& linename, const std::string& libname) const
{
    if ( !cd )
    {
        std::cerr << "StaticsLibStorage::writeToLine(..): Error. Invalid link to commondir structure." << std::endl ;
        return false ;
    }

    Prime::Files::Names names(*cd);
    std::string filename = names.library2d(SURFACECONSISTENT_STATICS, SURVEY2D, linename, libname, false) ;

    return write( filename ) ;
}

bool StaticsLibStorage::writeToGrid(const Commondir* cd, const std::string& gridname, const std::string& libname) const
{
    if ( !cd )
    {
        std::cerr << "StaticsLibStorage::writeToGrid(..): Error. Invalid link to commondir structure." << std::endl ;
        return false ;
    }

    Prime::Files::Names names(*cd);
    std::string filename = names.library3d(SURFACECONSISTENT_STATICS, gridname, libname, false) ;

    return write( filename ) ;
}

// ------------------------------------------
//           ScacLibStorage
// ------------------------------------------

void ScacLibStorage::clear()
{
    scac.clearNsp() ;
    scac.clearNrp() ;
    scac.clearStruct() ;
    scac.clearRnmo2d() ;
    scac.clearRnmo3dLx() ;
    scac.clearRnmo3dLy() ;
    scac.clearRnmo3dLxy() ;
}

void ScacLibStorage::clear(Component::Type type)
{
    static const std::string method_prefix = "ScacLibStorage::clear(Component::Type): " ;

    switch ( type )
    {
        case Component::Source:
            scac.clearNsp() ;
            break;

        case Component::Receiv:
            scac.clearNrp() ;
            break ;

        case Component::Struct:
            scac.clearStruct() ;
            break ;

        case Component::Kx:
            break ;

        case Component::Ky:
            break ;

        case Component::RnmoL2:
            scac.clearRnmo2d() ;
            break ;

        case Component::RnmoLxLx:
            scac.clearRnmo3dLx() ;
            break ;

        case Component::RnmoLxLy:
            scac.clearRnmo3dLxy() ;
            break ;

        case Component::RnmoLyLy:
            scac.clearRnmo3dLy() ;
            break ;

        default:
            throw std::invalid_argument( method_prefix + "Error. Undefined surface component has been received." ) ;
    }
}

float ScacLibStorage::get(Component::Type type, int index) const
{
    static const std::string method_prefix = "ScacLibStorage::get(Component::Type, int): " ;

    float value = 0 ;

    switch ( type )
    {
        case Component::Source:
            value = scac.getNspValue( index, basis_index ) ;
            break ;

        case Component::Receiv:
            value = scac.getNrpValue( index, basis_index ) ;
            break ;

        case Component::Struct:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            value = scac.getStructValue( line, ncdp, basis_index ) ;
        }
            break ;

        case Component::Kx:
            break ;

        case Component::Ky:
            break ;

        case Component::RnmoL2:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            value = scac.getRnmo2dValue( line, ncdp, basis_index ) ;
        }
            break ;

        case Component::RnmoLxLx:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            value = scac.getRnmo3dLxValue( line, ncdp, basis_index ) ;
        }
            break ;

        case Component::RnmoLxLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            value = scac.getRnmo3dLxyValue( line, ncdp, basis_index ) ;
        }
            break ;

        case Component::RnmoLyLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            value = scac.getRnmo3dLyValue( line, ncdp, basis_index ) ;
        }
            break ;

        default:
            throw std::invalid_argument( method_prefix + "Error. Undefined surface component has been received." ) ;
    }

    return value ;
}

void ScacLibStorage::set(Component::Type type, int index, float value)
{
    static const std::string method_prefix = "ScacLibStorage::set(Component::Type, int, float): " ;

    switch ( type )
    {
        case Component::Source:
            scac.setNspValue( index, value, basis_index ) ;
            break ;

        case Component::Receiv:
            scac.setNrpValue( index, value, basis_index ) ;
            break ;

        case Component::Struct:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            scac.setStructValue( line, ncdp, value, basis_index ) ;
        }
            break ;

        case Component::Kx:
            break ;

        case Component::Ky:
            break ;

        case Component::RnmoL2:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            scac.setRnmo2dValue( line, ncdp, value, basis_index ) ;
        }
            break ;

        case Component::RnmoLxLx:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            scac.setRnmo3dLxValue( line, ncdp, value, basis_index ) ;
        }
            break ;

        case Component::RnmoLxLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            scac.setRnmo3dLxyValue( line, ncdp, value, basis_index ) ;
        }
            break ;

        case Component::RnmoLyLy:
        {
            int line, ncdp ;
            getCoder()->decodeLineCdp( index, line, ncdp ) ;
            scac.setRnmo3dLyValue( line, ncdp, value, basis_index ) ;
        }
            break ;

        default:
            throw std::invalid_argument( method_prefix + "Error. Undefined surface component has been received." ) ;
    }
}

bool ScacLibStorage::write(const std::string& filename) const
{
    static const std::string method_prefix = "ScacLibStorage::write(const std::string&): " ;

    try
    {
        const std::string path = Prime::Files::getFileDir( filename ) ;

        if ( !Prime::Files::Commondir::directoryExists(path) )
            Prime::Files::Commondir::makeDirectory( path, false ) ;
        std::string test = filename;
        FILE *output_file = fopen( filename.c_str(), "wt" ) ;

        if ( !output_file )
        {
            throw std::runtime_error( "Error. Can't open output file '" + filename + "'." ) ;
        }

        if ( !scac.Write(output_file) )
        {
            throw std::runtime_error( "Error. Writing to output file '" + filename + "' has failed." ) ;
        }

        fclose( output_file ) ;
    }
    catch ( const std::exception& err )
    {
        std::cerr << method_prefix << err.what() << std::endl ;
        return false ;
    }

    return true ;
}

bool ScacLibStorage::writeToLine(const Commondir* cd, const std::string& linename, const std::string& libname) const
{
    if ( !cd )
    {
        std::cerr << "ScacLibStorage::writeToLine(..): Error. Invalid link to commondir structure." << std::endl ;
        return false ;
    }

    Prime::Files::Names names(*cd);
    std::string filename = names.library2d(SURFACECONSISTENT_SCAC, SURVEY2D, linename, libname, false) ;

    return write( filename ) ;
}

bool ScacLibStorage::writeToGrid(const Commondir* cd, const std::string& gridname, const std::string& libname) const
{
    if ( !cd )
    {
        std::cerr << "ScacLibStorage::writeToGrid(..): Error. Invalid link to commondir structure." << std::endl ;
        return false ;
    }

    Prime::Files::Names names(*cd);
    std::string filename = names.library3d(SURFACECONSISTENT_SCAC, gridname, libname, false) ;

    return write( filename ) ;
}

// ------------------------------------------
//           DeconLibStorage
// ------------------------------------------
bool DeconLibStorage::writeToLine(const Commondir* cd, const std::string& linename, const std::string& libname) const
{
    if ( !cd )
    {
        std::cerr << "DeconLibStorage::writeToLine(..): Error. Invalid link to commondir structure." << std::endl ;
        return false ;
    }

    Prime::Files::Names names(*cd);
    std::string filename = names.library2d(SURFACECONSISTENT_SCDEC, SURVEY2D, linename, libname, false) ;

    return write( filename ) ;
}

bool DeconLibStorage::writeToGrid(const Commondir* cd, const std::string& gridname, const std::string& libname) const
{
    if ( !cd )
    {
        std::cerr << "DeconLibStorage::writeToGrid(..): Error. Invalid link to commondir structure." << std::endl ;
        return false ;
    }

    Prime::Files::Names names(*cd);
    std::string filename = names.library3d(SURFACECONSISTENT_SCDEC, gridname, libname, false) ;

    return write( filename ) ;
}

}
}
}
