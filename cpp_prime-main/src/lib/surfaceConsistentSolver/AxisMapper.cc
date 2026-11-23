
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include "EquationsSet.h"
#include "AxisMapper.h"


namespace Prime {
namespace lib {
namespace SurfaceConsistent {

void AxisMapper::initAndRelink(SurfacePoint::Axis _type, EquationsSet& equations_set)
{
    typedef std::map<int,int>::iterator Iterator ;
    
    clear() ;
    type = _type ;
    
    // Filling map of real coordinates by dummy indices
    for ( int i = 0; i < equations_set.size(); ++i )
    {
        const int coord = equations_set.getEquationPoint(i).coordinate(type) ;
        Iterator it = real_to_index.find( coord ) ;

        if ( it == real_to_index.end() )
            real_to_index[coord] = -1 ;
    }
    
    // Sorting indices by real coordinate
    Iterator it = real_to_index.begin() ;
    for ( int index = 0; it != real_to_index.end(); ++it, ++index )
    {
        it->second = index ;
        index_to_real.push_back( it->first ) ;
    }
    
    // Relinking coordinates by indices
    // and filling links to equations.
    index_to_equations.resize( index_to_real.size() ) ;
    
    for ( int i = 0; i < equations_set.size(); ++i )
    {
        SurfacePoint& point = equations_set.getEquationPoint(i) ;
        int index = real_to_index[point.coordinate(type)] ;
        
        point.setCoordinate(type, index) ;
        index_to_equations[index].push_back( i ) ;
    }
}

const std::vector<int>& AxisMapper::getEquations(int index) const
{
    static const std::string method_prefix = "AxisMapper::getEquations(int): " ;
    
    if ( index < 0 || index >= size() )
    {
        std::ostringstream error ;
        error << method_prefix << "Error. Invalid index has been received." ;
        error << " Number of unique coordinates is " << size() << " but required" ;
        error << " index is " << index << ". This is an internal bug." ;
        throw std::runtime_error( error.str() ) ;
    }
    
    return index_to_equations[index] ;
}

int AxisMapper::getRealCoord(int index) const
{
    static const std::string method_prefix = "AxisMapper::getRealCoord(int): " ;
    
    if ( index < 0 || index >= size() )
    {
        std::ostringstream error ;
        error << method_prefix << "Error. Invalid index has been received." ;
        error << " Number of unique coordinates is " << size() << " but required" ;
        error << " index is " << index << ". This is an internal bug." ;
        throw std::runtime_error( error.str() ) ;
    }
    
    return index_to_real[index] ;
}

int AxisMapper::getIndex( int coordinate ) const
{
    static const std::string method_prefix = "AxisMapper::getIndex(int): " ;
    
    std::map<int,int>::const_iterator it = real_to_index.find( coordinate ) ;
    if ( it == real_to_index.end() )
    {
        std::ostringstream error ;
        error << method_prefix << "Error. Undefined coordinate value has been received: " ;
        error << coordinate << " value has been requested. Type of axis is " << type << "." ;
        throw std::runtime_error( error.str() ) ;
    }
    
    return it->second ;
}

}
}
}