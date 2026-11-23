
#include <bits/unique_ptr.h>

#include "db/Record.h"
#include "primeApp/PrimeAppNoX.h"
#include "configdirs/configdirs.h"
#include "files/Names.h"
#include "utils/methodname.h"
#include "mathlib/algorithms/sorting.h"
#include "vertical_spectra_analysis/ObjectsDB.h"

#include "DataReader.h"
#include "DataStorage.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

const Prime::Metacube::MetacubeSorting
DataStorage::DEF_REQUIRED_INDEX(std::vector<Prime::HdrField>({Prime::HdrField(TYPEITEM(ObjectIDHeader)),
                                                             Prime::HdrField(TYPEITEM(DataTypeHeader)),
                                                             Prime::HdrField(TYPEITEM(XHeader)),
                                                             Prime::HdrField(TYPEITEM(YHeader)),
                                                             Prime::HdrField(H_NCDP),
                                                             Prime::HdrField(H_L)})) ;
const Prime::Sort DataStorage::DEF_DATA_SORT(H_NCDP, H_L) ;

DataStorage::DataStorage() { }

DataStorage::~DataStorage()
{
    unloadData() ;
}

const std::string& DataStorage::getGridName() const
{
    return m_grid_name ;
}

Prime::Metacube::MetaCube * DataStorage::getMetacube() const
{
    return m_metacube.get() ;
}

const Prime::Metacube::MetaCubeIndex* DataStorage::getMetacubeIndex() const
{
    return m_metacube_index ;
}

DataStorage::HodographType DataStorage::getHodographType() const
{
    return m_analysis_hodograph_type ;
}

bool DataStorage::isTimeDomain() const
{
    return m_is_data_in_time_domain ;
}

double DataStorage::getDt() const
{
    if ( !getMetacube() )
        return 0 ;

    return (isTimeDomain() ? 1000 : 1)  * getMetacube()->GetDt() ;
}

size_t DataStorage::getSamplesNumber() const
{
    return getMetacube() ? getMetacube()->GetTraceSamples() : 0 ;
}

double DataStorage::getSampleBeg() const
{
    return m_data_sample_begin ;
}

double DataStorage::getSampleEnd() const
{
    return m_data_sample_begin + getDt() * getSamplesNumber() ;
}

bool DataStorage::loadData(const std::string& __grid_name,
                           const std::string& __version_name)
{
    m_grid_name = __grid_name ;
    m_version_name = __version_name ;

    if ( !openInputVersion(m_grid_name, m_version_name) )
        return false ;

    if ( !openVersionIndex() )
        return false ;
    
    if ( !extractObjectsList() )
        return false ;
    
    return true ;
}

void DataStorage::unloadData()
{
    m_grid_name.clear() ;
    m_version_name.clear() ;
    m_data_points.clear() ;
    m_metacube_index = 0 ;
    m_metacube.reset( 0 ) ;
    m_analysis_objects.clear() ;
    m_analysis_objects_names.clear() ;
}

int DataStorage::getObjectIndex( ObjectID __object_id ) const
{
    for  ( int i = 0; i < m_analysis_objects.size(); ++i )
    {
        if (m_analysis_objects[i] == __object_id)
            return i ;
    }
    
    return -1 ;
}

const std::vector<ObjectID>& DataStorage::getObjects() const
{
    return m_analysis_objects ;
}

const std::vector<std::string>& DataStorage::getObjectsNames() const
{
    return m_analysis_objects_names ;
}

const DataStorage::DataPointsList& DataStorage::getDataPoints(ObjectID __object_id) const
{
    auto itobj = m_data_points.find(__object_id) ;

    if ( itobj != m_data_points.end() && itobj->second )
        return *(itobj->second) ;

    static DataPointsList empty_list ;
    return empty_list ;
}

const DataStorage::DataPoint& DataStorage::getDataPointByIndex(ObjectID __object_id, int __point_index) const
{
    const auto& points = getDataPoints(__object_id) ;

    if (__point_index >= 0 &&
        __point_index < points.size())
        return points[__point_index] ;

    static DataPoint dummy ;
    return dummy ;
}

int DataStorage::findNearestDataPointByNcdp(ObjectID __object_id, int __ncdp) const
{
    const auto& points = getDataPoints(__object_id) ;

    int best_distance = INT_MAX ;
    int best_index = -1 ;

    for ( int i = 0, n = points.size(); i < n; ++i )
    {
        const int distance = std::abs(points[i].ncdp - __ncdp) ;

        if ( distance == 0 )
            return i ;

        if ( distance < best_distance )
        {
            best_distance = distance ;
            best_index = i ;
        }
    }

    return best_index ;
}

DataReader * DataStorage::createDataReader(const int __object_id,
                                           SpectraDataType __data_type) const
{
    define_methodname() ;

    try
    {
        return new DataReader(*this, __object_id, __data_type) ;
    }
    catch (const std::exception& __err)
    {
        std::cerr << __methodname__ << ": Error. " << __err.what() << std::endl ;
    }

    return 0 ;
}

bool DataStorage::openInputVersion(const std::string& __grid_name,
                                   const std::string& __version_name)
{
    define_methodname() ;

    try
    {
        if ( __version_name.empty() )
            return false ;

        std::unique_ptr<Prime::Metacube::MetaCube> metacube_tmp ;
        metacube_tmp.swap( m_metacube ) ;

        m_metacube.reset( new Prime::Metacube::MetaCube() );

        if ( !m_metacube )
        {
            m_metacube.swap( metacube_tmp ) ;
            return false ;
        }

        std::string version_path = Prime::lib::primeApp::getProjCD()->getDirectory(/*DATA3D,*/ SEISMIC_PRESTACK, __grid_name, __version_name ) ;
        m_metacube->Open( version_path ) ;

        // Reading first trace for getting information about data
        std::unique_ptr<char> pheader( m_metacube->GetTraceHeaders({0}, 0, false) ) ;

        if ( !pheader )
            throw std::runtime_error( "Can't read header of first trace header for getting information about data." ) ;

        m_analysis_hodograph_type = HodographType( Prime::getHeaderValue<int>(pheader.get(), TYPEITEM(HodographTypeHeader)) ) ;
        m_is_data_in_time_domain = (Prime::lib::Nmo::HodographType::get_domain_type(m_analysis_hodograph_type) == FR_IO_TIME) ;
        m_data_sample_begin = (m_is_data_in_time_domain ? 1000 : 1) * Prime::getHeaderValue<float>(pheader.get(), H_TBC) ;
    }
    catch ( const std::exception& __err )
    {
        std::cerr << __methodname__ << ": Error. " << __err.what() << std::endl ;
        return false ;
    }

    return true ;
}

bool DataStorage::openVersionIndex()
{
    define_methodname() ;

    using namespace Prime::Metacube ;

    try
    {
        m_data_points.clear() ;

        if ( !m_metacube )
            throw std::invalid_argument("Pointer to input Metacube is invalid.") ;

        m_metacube_index = m_metacube->GetIndex( DEF_REQUIRED_INDEX );

        if ( !m_metacube_index )
            throw CMetaCubeException( Prime::Metacube::CMetaCubeException::MCE_INDEX_NOT_FOUND ) ;

        if ( m_metacube_index->Open(DEF_PORTIONS_KEY_LEVEL, MetaCubeIndex::WHOLE_DATA) <= 0 )
        {
            throw CMetaCubeException("Input Metacube doesn't contain any data.") ;
        }

        const size_t portions_num = m_metacube_index->getPortionsNumber() ;

        auto itobj = m_data_points.end() ;

        for ( size_t iportion = 0; iportion < portions_num; ++iportion )
        {
            // Getting next portion of data
            TTraceNum portion_beg ;
            size_t portion_size ;

            DataPortion portion ;
            portion.key = m_metacube_index->getPortionInfo(iportion, portion_beg, portion_size) ;
            portion.begin = portion_beg ;
            portion.end = portion_beg + portion_size ;

            if (portion_beg < 0 ||
                portion_beg >= m_metacube_index->getTotalItems() ||
                portion_size <= 0 ||
                portion_beg + portion_size > m_metacube_index->getTotalItems())
                continue ;

            // Getting access to object storage
            if (itobj == m_data_points.end() ||
                itobj->first != portion.objectID())
            {
                auto res = m_data_points.insert( std::make_pair(portion.objectID(), std::make_shared<DataPointsList>()) ) ;

                if ( !res.second )
                    continue ;

                itobj = res.first ;
            }

            // Adding point to data if required
            auto it = std::lower_bound(itobj->second->begin(), itobj->second->end(), portion.ncdp(),
                                       [](const DataPoint& __p, int __ncdp) -> bool {
                                           return __p.ncdp < __ncdp ;
                                       });

            if ( it == itobj->second->end() || it->ncdp != portion.ncdp() )
            {
                DataPoint data_point ;
                data_point.ncdp = portion.ncdp() ;
                data_point.x = portion.x() ;
                data_point.y = portion.y() ;

                it = itobj->second->insert( it, data_point ) ;
            }

            // Adding information about portion
            if ( it != itobj->second->end() )
                it->portions.at(size_t(portion.type())) = portion ;
        }
    }
    catch ( const std::exception& __err )
    {
        std::cerr << __methodname__ + ": Error. " << __err.what() << std::endl ;
        return false ;
    }

    return true ;
}

bool DataStorage::extractObjectsList()
{
    define_methodname() ;

    try
    {
        ObjectsDB objects_db ;
        objects_db.load() ;
        
        m_analysis_objects.clear() ;
        m_analysis_objects_names.clear() ;

        const int objects_num = m_data_points.size() ;
        m_analysis_objects.reserve( objects_num ) ;
        m_analysis_objects_names.reserve( objects_num ) ;

        for ( const auto& object : m_data_points )
        {
            ObjectID object_id = object.first ;
            const auto * object_info = objects_db.findObjectByID( object_id ) ;

            m_analysis_objects.push_back( object_id ) ;

            if ( object_info )
            {
                m_analysis_objects_names.push_back( object_info->m_name ) ;
            }
            else
            {
                std::ostringstream name ;
                name << object_id ;
                m_analysis_objects_names.push_back( name.str() ) ;
            }
        }
        
        Prime::lib::mathlib::sort(&m_analysis_objects_names,
                                  &m_analysis_objects,
                                  Prime::DB::compare23dkey) ;
    }
    catch ( const std::exception& __err )
    {
        std::cerr << __methodname__ << ": Error. " << __err.what() << std::endl ;
        m_analysis_objects.clear() ;
        m_analysis_objects_names.clear() ;
    }
    
    return !m_analysis_objects.empty() ;
}

}
}
}
