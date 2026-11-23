/* 
 * File:   DataReader.h
 * Author: ranisimov
 *
 * Created on July 14, 2023, 2:15 PM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_DATA_READER_H
#define VERTICAL_SPECTRA_ANALYSIS_DATA_READER_H

#include <vector>
#include <map>

#include "opsyserv/StrError.h"
#include "seismicReader/RawReaderBase.h"
#include "metacube/metacube.h"
#include "Nmo/Hodographs.h"
#include "vertical_spectra_analysis/Types.h"

#include "DataStorage.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class for Vertical Spectra Analysis data reading.
 */
class DataReader : public Prime::Data::RawReaderBase
{
    typedef Prime::lib::Nmo::HodographType::Type HodographType ;
    
public:
    DataReader(const DataStorage& __data_storage,
               const int __object_id,
               SpectraDataType __data_type) ;
    
    ~DataReader() ;
    
    /**
     * @brief Returns vertical spectra data type of reader.
     */
public:
    SpectraDataType getSpectraDataType() const ;
    
    /**
     * @brief Returns type of analysis hodograph used for generating this data.
     */
public:
    HodographType getHodographType() const ;
    
    /**
     * @brief Clears reader.
     */
public:
    virtual void clear() ;

    /**
     * @brief Reloads reader virtually.
     */
public:
    virtual void reload() ;

    /**
     * @brief Reads header of required trace.
     */
public:
    virtual const Prime::IO::TraceHead * read_header(int __itrace) const ;

    /**
     * @brief Reads required trace.
     */
public:
    virtual const float * read_trace(int __itrace, bool __apply_funcs = true) const ;

    /**
     * @brief Reads block of traces.
     * @attention Function allocates memory for traces block, so you need to call free(*ppTraces).
     */
public:
    virtual void read_traces(const std::vector<int>& vTrsNum,
                             std::vector<Prime::IO::TraceHead> *headers,
                             float **ppTraceBodies, bool apply_funcs) const ;
    
    /**
     * @brief Reads traces with required indices.
     */
private:
    bool read_traces(const std::vector<int>& __itraces,
                     std::vector<float>& __buffer) const ;
    
    /**
     * @brief Loads data.
     */
private:
    void loadData(const int __object_id,
                  SpectraDataType __data_type) ;

    /**
     * @name Trace index conversion methods. 
     * @{
     */
public:
    virtual int logical2trace(int n) const ;
    virtual int trace2logical(int n) const ;
    virtual int logical2key1(int n) const ;
    virtual int logical2key2(int n) const ;
    virtual Prime::VSort logical2gatherkey(int n) const ;
    virtual int logical2secondarykey(int n) const ;
    //!@}
    
private:
    const DataStorage& m_data_storage ; //!< Storage of input data.
    int m_object_id ; //!< Index of object id.
    SpectraDataType m_vertical_spectra_data_type ; //!< Type of data (i.e. selector for data)
    HodographType m_analysis_hodograph_type ; //!< Type of analysis hodograph.
    std::vector<int> igather2logicalbeg ; //!< Gather to logical begin converter.
    mutable Prime::IO::TraceHead m_header_buffer ; //!< Header for non-existing trace.
    mutable std::vector<float> m_buffer ; //!< Memory buffer.
};

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_DATA_READER_H */

