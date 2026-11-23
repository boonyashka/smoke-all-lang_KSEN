
#include "CommonTypes/HeadConstants.h"
#include "utils/methodname.h"

#include "DataReader.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

DataReader::DataReader(const DataStorage& __data_storage,
                       const int __object_id,
                       SpectraDataType __data_type) :
RawReaderBase(std::string(),
              __data_storage.getGridName(),
              std::string(),
              0, 0, 1, 1,
              Prime::Sort(),
              FR_IO_TIME),
m_data_storage(__data_storage),
m_object_id(__object_id),
m_vertical_spectra_data_type(__data_type)
{
    define_methodname() ;

    if ( !m_data_storage.getMetacube() )
        throw std::invalid_argument(__methodname__ + ": Error. Pointer to input Metacube is invalid.") ;

    if ( !m_data_storage.getMetacubeIndex() )
        throw std::invalid_argument(__methodname__ + ": Error. Pointer to input Metacube index is invalid.") ;

    loadData( m_object_id, m_vertical_spectra_data_type ) ;
}

DataReader::~DataReader() { }

SpectraDataType DataReader::getSpectraDataType() const
{
    return m_vertical_spectra_data_type ;
}

DataReader::HodographType DataReader::getHodographType() const
{
    return m_analysis_hodograph_type ;
}

void DataReader::clear()
{
    RawReaderBase::clear() ;
}

void DataReader::reload()
{
    clear() ;
}

const Prime::IO::TraceHead * DataReader::read_header(int __itrace) const
{
    if ( !read_traces({__itrace}, m_buffer) )
        return 0 ;

    m_header_buffer.Init( false, (const char*) m_buffer.data() ) ;
    applyHeaderFuncs( m_header_buffer ) ;

    return &m_header_buffer ;
}

const float * DataReader::read_trace(int __itrace, bool __apply_funcs) const
{
    if ( !read_traces({__itrace}, m_buffer) )
        return 0 ;

    m_header_buffer.Init( false, (const char*) m_buffer.data() ) ;
    applyHeaderFuncs( m_header_buffer ) ;

    float * pbody = m_buffer.data() + CGG_TR_HEAD / sizeof (float) ;

    if ( __apply_funcs )
    {
        reInitFuncs() ;
        applyFuncs( m_header_buffer, pbody ) ;
    }

    return pbody ;
}

void DataReader::read_traces(const std::vector<int>& vTrsNum,
                             std::vector<Prime::IO::TraceHead> *headers,
                             float **ppTraceBodies, bool apply_funcs) const
{
    define_methodname() ;

    try
    {
        if ( !ppTraceBodies || !headers )
        {
            throw Memory::Exception( "%s: Error. Output buffers are invalid.", __methodname__.c_str() ) ;
        }

        // Initialization output memory
        *ppTraceBodies = (float*) calloc( vTrsNum.size() * nSamples, sizeof (float) ) ;

        if ( !*ppTraceBodies )
        {
            int err = errno;
            throw Memory::Exception( "%s: Error. Allocation memory for trace bodies: %s", __methodname__.c_str(), Prime::SysService::StrError(err) ) ;
        }

        headers->assign( vTrsNum.size(), m_header_buffer ) ;

        reInitFuncs() ;

        if ( read_traces(vTrsNum, m_buffer) )
        {
            const int trace_length_f = CGG_TR_HEAD / sizeof (float) + getNumberOfSamples() ;

            for ( int i = 0; i < vTrsNum.size(); ++i )
            {
                float * ptrace = m_buffer.data() + i * trace_length_f ;
                headers->at(i).Init( false, (const char *) ptrace ) ;

                float * pbody = ptrace + CGG_TR_HEAD / sizeof (float) ;
                float * obody = *ppTraceBodies + i * nSamples ;
                std::copy(pbody, pbody + nSamples, obody) ;

                if ( apply_funcs )
                    applyFuncs( headers->at(i), obody ) ;
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << __methodname__ << ": Error. " << e.what() << std::endl;

        if ( *ppTraceBodies )
        {
            free(*ppTraceBodies) ;
            *ppTraceBodies = 0 ;
        }
    }
}

bool DataReader::read_traces(const std::vector<int>& __itraces,
                             std::vector<float>& __buffer) const
{
    define_methodname() ;

    using namespace Prime::Metacube ;

    try
    {
        if ( !m_data_storage.getMetacube() || !m_data_storage.getMetacubeIndex() || __itraces.empty() )
            return false ;

        const int trace_length_f = CGG_TR_HEAD / sizeof (float) + getNumberOfSamples() ;
        __buffer.resize( __itraces.size() * trace_length_f ) ;

        std::vector<TTraceNum> traces_indices ;

        for ( int itrace : __itraces )
        {
            size_t trace_index = itrace / getDcdp() ;

            if ( trace_index >= m_data_storage.getMetacubeIndex()->size() )
            {
                throw CMetaCubeException( "Trace number %d points to metacube index item %lu, \
that is out of range [0, %lu)", itrace, trace_index, m_data_storage.getMetacubeIndex()->size());
            }

            traces_indices.push_back( m_data_storage.getMetacubeIndex()->getItem(trace_index).Trace() ) ;
        }

        m_data_storage.getMetacube()->GetTraces( traces_indices, __buffer.data() ) ;
    }
    catch ( const std::exception& __err )
    {
        std::cerr << __methodname__ << ": Error. " << __err.what() << std::endl ;
        return false ;
    }

    return true ;
}

void DataReader::loadData(const int __object_id,
                          SpectraDataType __data_type)
{
    define_methodname() ;

    using namespace Prime::Metacube ;

    try
    {
        if ( !m_data_storage.getMetacube() )
            throw std::invalid_argument("Pointer to input Metacube is invalid.") ;

        // Preparation sorting
        m_sort = DataStorage::DEF_DATA_SORT ;
        const int sort_size = m_sort.size() ;

        // Input data analysis
        nTraces = m_data_storage.getMetacubeIndex()->getTotalItems() ;
        
        if ( nTraces == 0 )
            throw Prime::Metacube::CMetaCubeException( "No traces in input Metacube." ) ;
        
        firstTrace = 0 ;
        lastTrace = firstTrace + (nTraces - 1) * getDcdp() ;
        dt = m_data_storage.getMetacube()->GetDt() ;
        nSamples = m_data_storage.getMetacube()->GetTraceSamples();

        v_gathers.clear() ;

        size_t gather_index = 0 ;
        int first_trace_index = INT_MAX ;
        
        const auto& data_points = m_data_storage.getDataPoints(__object_id) ;

        for ( const auto& point : data_points )
        {
            if ( point.empty() )
                continue ;
            
            const auto& portion = point.portions.at(size_t(__data_type)) ;
            
            if ( portion.type() != __data_type )
                continue ;

            Prime::VSort vkey ;
            for (int i = 0; i < sort_size - 1; ++i)
                vkey.K[i] = portion.key[DataStorage::DEF_NCDP_IND + i] ;

            Gather gather ;
            gather.num = gather_index++ ;
            gather.firstTrace = portion.begin ;
            gather.lastTrace = portion.end - 1 ;

            v_gathers[vkey] = gather ;
            first_trace_index = std::min( first_trace_index, portion.begin ) ;
        }

        if ( v_gathers.empty() )
            throw Prime::Metacube::CMetaCubeException( "No traces in input Metacube." ) ;

        // Gather to logical begin map preparation
        igather2logicalbeg.assign( v_gathers.size(), -1 ) ;

        for ( const auto& it : v_gathers )
        {
            if (it.second.num < 0 ||
                it.second.num >= igather2logicalbeg.size())
                continue ;

            igather2logicalbeg[it.second.num] = it.second.firstTrace ;
        }

        // Getting parameters of data
        if ( !read_traces({first_trace_index}, m_buffer) )
            throw Prime::Metacube::CMetaCubeException("Can't analyze Metacube.") ;

        m_header_buffer.Init( false, (const char*) m_buffer.data() ) ;
        beg = m_header_buffer.beg() ;
        end = m_header_buffer.end() ;
        
        m_analysis_hodograph_type = HodographType( m_header_buffer.Get(TYPEITEM(HodographTypeHeader)) ) ;
        m_data_type = Prime::lib::Nmo::HodographType::get_domain_type(m_analysis_hodograph_type) ;
        dt = (m_data_type == FR_IO_DEPTH ? 1000 * dt : dt) ;

        profileDp = (__data_type == SpectraDataType::SuperGather) ? 1.0 :
                m_header_buffer.FGet( TYPEITEM(ParameterStepHeader) ) ;

        if (nSamples != (end - beg) + 1)
            throw CMetaCubeException("Trace samples in metacube (%lu) differs \
from trace samples in trace header (%d)", nSamples, (end - beg) + 1) ;
        
    }
    catch ( const std::exception& __err )
    {
        std::cerr << __methodname__ + ": Error. " << __err.what() << std::endl ;
        clear() ;
        throw ;
    }
}

int DataReader::logical2trace(int n) const
{
    if ( !m_data_storage.getMetacube() || !m_data_storage.getMetacubeIndex() )
        return -1 ;

    size_t index = n / getDcdp() ;

    if ( index < m_data_storage.getMetacubeIndex()->size() )
        return int( m_data_storage.getMetacube()->GetTraceNumber(m_data_storage.getMetacubeIndex()->getItem(index).Trace()) ) ;

    return -1 ;
}

int DataReader::trace2logical(int n) const
{
    return -1 ;
    //TODO: по номеру трассы взять из заголовков метакуба значения полей и найти их в индексе
}

int DataReader::logical2key1(int n) const
{
    if ( !m_data_storage.getMetacube() || !m_data_storage.getMetacubeIndex() )
        return -1 ;

    size_t index = n / getDcdp() ;

    if ( index < m_data_storage.getMetacubeIndex()->size() )
        return int(m_data_storage.getMetacubeIndex()->getItem(index)[0]) ;

    return -1 ;
}

int DataReader::logical2key2(int n) const
{
    if ( !m_data_storage.getMetacube() || !m_data_storage.getMetacubeIndex() )
        return -1 ;

    size_t index = n / getDcdp() ;

    if ( index < m_data_storage.getMetacubeIndex()->size() )
        return int(m_data_storage.getMetacubeIndex()->getItem(index)[1]) ;

    return -1 ;
}

Prime::VSort DataReader::logical2gatherkey(int n) const
{
    // Fast search
    auto it = std::upper_bound(igather2logicalbeg.begin(), igather2logicalbeg.end(), n) ;

    if ( it != igather2logicalbeg.end() )
    {
        if ( it != igather2logicalbeg.begin() )
            --it ;

        if ( *it >= 0 )
        {
            auto itgather = v_gathers.begin() ;
            std::advance(itgather, std::distance(igather2logicalbeg.begin(), it)) ;

            if (itgather->second.firstTrace <= n &&
                itgather->second.lastTrace  >= n)
                return itgather->first ;
        }
    }

    // Brute-force search
    for ( auto itgather : v_gathers )
    {
        if (itgather.second.firstTrace <= n &&
            itgather.second.lastTrace  >= n)
            return itgather.first ;
    }

    return Prime::VSort(0) ;
}

int DataReader::logical2secondarykey(int n) const
{
    if ( !m_data_storage.getMetacube() || !m_data_storage.getMetacubeIndex() )
        return -1 ;

    if ( m_data_storage.getMetacubeIndex()->GetKeyLevel() < 0 )
        return -1 ;

    size_t index = n / getDcdp() ;

    if ( index < m_data_storage.getMetacubeIndex()->size() )
    {
        const int ikey = m_data_storage.getMetacubeIndex()->GetKeyLevel() ;
        return int(m_data_storage.getMetacubeIndex()->getItem(index)[ikey]) ;
    }

    return -1 ;
}

}
}
}