/* 
 * File:   LibStorage.h
 * Author: rganisimov
 *
 * Created on February 27, 2017, 2:45 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_LIBSTORAGE_H
#define	PRIME_LIB_SURFACECONSISTENT_LIBSTORAGE_H

#include "files/Commondir.h"
#include "Statics/shift.h"
#include "surfaceConsistant/surfaceConsistStore.h"
#include "SurfacePoint.h"
#include "SurfaceComponent.h"
#include "CoordinateCoder.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

using Prime::Files::Commondir ;
using Prime::SurfaceConsistant::SurfaceConsistantStorage ;

/**
 * @brief Base class of surface consistent libraries.
 * @detailed This class provides an interface for getting
 * access to abstract surface consistent library. It is
 * using in the surface consistent solver for dividing
 * solving algorithms from result representation.
 */
class LibStorage
{
public:
    LibStorage( const CoordinateCoder * _coder ) : 
    coder(_coder) {}
    
    /**
     * @name Clearing
     * @{
     */
public:
    virtual void clear() =  0 ;
    virtual void clear( Component::Type type ) = 0 ;
    //! @}
    
    /**
     * @name Setters/getters
     * @{
     */
public:
    virtual void set(Component::Type type, int index, float value) = 0 ;
    virtual float get(Component::Type type, int index) const = 0 ;
    //! @}
    
    /**
     * @name Writing
     * @{
     */
public:
    virtual bool write( const std::string& filename ) const = 0 ;
    virtual bool writeToLine( const Commondir * cd, const std::string& linename, const std::string& libname ) const = 0 ;
    virtual bool writeToGrid( const Commondir * cd, const std::string& gridname, const std::string& libname ) const = 0 ;
    //! @}
    
    /**
     * Returns coordinates coder which is required
     * in derived classes for encoding/decoding coordinates.
     */
protected:
    const CoordinateCoder* getCoder() const
    {
        return coder ;
    }
    
private:
    const CoordinateCoder * coder ; //!< Link to coordinates coder.
};

/**
 * @brief Base class of surface consistent statics libraries.
 * @detailed See description of base class for more details.
 */
class StaticsLibStorage : public LibStorage
{
public:

    StaticsLibStorage( const CoordinateCoder * _coder ) :
    LibStorage( _coder ) { }
    
    /**
     * @name Clearing
     * @{
     */
public:
    void clear() ;
    void clear( Component::Type type ) ;
    //! @}
    
    /**
     * @name Setters/getters. Values in seconds.
     * @{
     */
public:
    void set(Component::Type type, int index, float value) ;
    float get(Component::Type type, int index) const ;
    //! @}
    
    /**
     * @name Writing
     * @{
     */
public:
    bool write( const std::string& filename ) const ;
    bool writeToLine( const Commondir * cd, const std::string& linename, const std::string& libname ) const ;
    bool writeToGrid( const Commondir * cd, const std::string& gridname, const std::string& libname ) const ;
    //! @}
    
private:
    primeStat statics ; //!< Container of statics shifts.
};

/**
 * @brief Base class of surface consistent amplitudes libraries.
 * @detailed See description of base class for more details.
 */
class ScacLibStorage : public LibStorage
{
public:

    ScacLibStorage( const CoordinateCoder * _coder ) :
    LibStorage( _coder ),
    basis_index( 0 ) { }
    
    /**
     * @name Clearing
     * @{
     */
public:
    void clear() ;
    void clear( Component::Type type ) ;
    //! @}
    
    /**
     * @name Setters/getters. Values in seconds.
     * @{
     */
public:
    void set(Component::Type type, int index, float value) ;
    float get(Component::Type type, int index) const ;
    //! @}
    
    /**
     * @name Writing
     * @{
     */
public:
    bool write( const std::string& filename ) const ;
    bool writeToLine( const Commondir * cd, const std::string& linename, const std::string& libname ) const ;
    bool writeToGrid( const Commondir * cd, const std::string& gridname, const std::string& libname ) const ;
    //! @}
    
protected:
    SurfaceConsistantStorage scac ; //!< Container of amplitudes logarithms
    int basis_index ;               //!< Basis function index
};

/**
 * @brief Base class of surface consistent deconvolution libraries.
 * @detailed See description of base class for more details.
 */
class DeconLibStorage : public ScacLibStorage
{
public:

    DeconLibStorage( const CoordinateCoder * _coder ) :
    ScacLibStorage( _coder ) { }
    
    /**
     * Changes index of basis function.
     */
    void setBasisIndex( int _basis_index )
    {
        if ( _basis_index < 0 )
            throw std::invalid_argument( "DeconLibStorage::setBasisIndex(int): "
                    "Error. Negative index of basis function has been received." ) ;
        
        basis_index = _basis_index ;
    }
    
    /**
     * @name Writing
     * @{
     */
public:
    bool writeToLine( const Commondir * cd, const std::string& linename, const std::string& libname ) const ;
    bool writeToGrid( const Commondir * cd, const std::string& gridname, const std::string& libname ) const ;
    //! @}
};

}
}
}

#endif

