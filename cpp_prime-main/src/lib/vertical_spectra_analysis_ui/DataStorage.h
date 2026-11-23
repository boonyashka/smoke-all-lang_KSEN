/* 
 * File:   DataStorage.h
 * Author: ranisimov
 *
 * Created on July 22, 2023, 12:37 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_DATA_STORAGE_H
#define VERTICAL_SPECTRA_ANALYSIS_DATA_STORAGE_H

#include <vector>
#include <memory>
#include <array>

#include "opsyserv/StrError.h"
#include "metacube/metacube.h"
#include "Nmo/Hodographs.h"
#include "vertical_spectra_analysis/Types.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

class DataReader ;

/**
 * @brief Class of input data storage.
 */
class DataStorage
{
public:
    static const Prime::Metacube::MetacubeSorting DEF_REQUIRED_INDEX ; //!< Required Metacube index.
    static const Prime::Sort DEF_DATA_SORT ; //!< Data sorting.
    static const int DEF_OBJECT_ID_IND = 0 ; //!< Position of object ID in index.
    static const int DEF_TYPE_IND = 1 ; //!< Position of type in index.
    static const int DEF_X_IND = 2 ; //!< Position of X in index.
    static const int DEF_Y_IND = 3 ; //!< Position of Y in index.
    static const int DEF_NCDP_IND = 4 ; //!< Position of NCDP in index.
    static const int DEF_PORTIONS_KEY_LEVEL = 5 ; //!< Key level of data portioning.

    /**
     * @brief Structure for storing data portions.
     */
public:

    struct DataPortion
    {

        /**
         * @brief Returns object id to which portion belongs.
         */
        ObjectID objectID() const
        {
            return key.size() > DEF_OBJECT_ID_IND ? int(key.vValue[DEF_OBJECT_ID_IND]) : -1 ;
        }

        /**
         * @brief Returns type of data portion.
         */
        SpectraDataType type() const
        {
            return SpectraDataType( key.size() > DEF_TYPE_IND ? int(key.vValue[DEF_TYPE_IND]) : 0 ) ;
        }
        
        /**
         * @brief Returns X-coordinate of data portion.
         */
        float x() const
        {
            return key.size() > DEF_X_IND ? int(key.vValue[DEF_X_IND]) : 0 ;
        }
        
        /**
         * @brief Returns Y-coordinate of data portion.
         */
        float y() const
        {
            return key.size() > DEF_Y_IND ? int(key.vValue[DEF_Y_IND]) : 0 ;
        }

        /**
         * @brief Returns NCDP of data portion.
         */
        int ncdp() const
        {
            return key.size() > DEF_NCDP_IND ? int(key.vValue[DEF_NCDP_IND]) : -1 ;
        }

        Prime::PortionKey key ; //!< Key of portion.
        int begin ; //!< First trace index of data portion.
        int end ; //!< Trace index after last trace of data portion.
    } ;

    /**
     * @brief Structure for storing information about data point.
     */
public:

    struct DataPoint
    {
        DataPoint() :
        ncdp(-1),
        x(0),
        y(0) {}
        
        /**
         * @brief Checks completeness of point.
         */
        bool empty() const
        {
            for ( const auto& p : portions )
            {
                if ( p.key.size() != DEF_PORTIONS_KEY_LEVEL )
                    return true ;
            }
            
            return false ;
        }
        
        int ncdp ; //!< Location of point at analysis profile, i.e. its NCDP index.
        float x ; //!< X-coordinate of point location.
        float y ; //!< Y-coordinate of point location.
        std::array<DataPortion, size_t(SpectraDataType::NumberOfTypes) > portions ; //!< Portions of data.
    } ;

    // Types definition
    typedef std::vector<DataPoint> DataPointsList ;
    typedef std::shared_ptr<DataPointsList> DataPointsListPtr ;
    typedef Prime::lib::Nmo::HodographType::Type HodographType ;

    /**
     * @brief Constructs default empty object.
     */
public:
    DataStorage() ;

public:
    ~DataStorage() ;

    /**
     * @brief Returns name of grid.
     */
public:
    const std::string& getGridName() const ;

    /**
     * @brief Returns access to metacube.
     */
public:
    Prime::Metacube::MetaCube* getMetacube() const ;

    /**
     * @brief Returns metacube index of input data.
     */
public:
    const Prime::Metacube::MetaCubeIndex* getMetacubeIndex() const ;
    
    /**
     * @brief Returns type of analysis hodograph used for generating this data.
     */
public:
    HodographType getHodographType() const ;
    
    /**
     * @brief Returns true if data in time domain and false if in depth domain.
     */
public:
    bool isTimeDomain() const ;
    
    /**
     * @brief Returns sampling step of data (in msec or meters).
     */
public:
    double getDt() const ;
    
    /**
     * @brief Returns length (number of samples) in data.
     */
public:
    size_t getSamplesNumber() const ;
    
    /**
     * @brief Returns time/depth of first sample (in msec or meters).
     */
public:
    double getSampleBeg() const ;
    
    /**
     * @brief Returns time/depth after last sample (in msec or meters).
     */
public:
    double getSampleEnd() const ;

    /**
     * @brief Loads required data.
     */
public:
    bool loadData(const std::string& __grid_name,
                  const std::string& __version_name) ;
    
    /**
     * @brief Unloads all data.
     */
public:
    void unloadData() ;
    
    /**
     * @brief Returns index of required object from it ID.
     */
public:
    int getObjectIndex( ObjectID __object_id ) const ;

    /**
     * @brief Returns access to analysis objects list.
     */
public:
    const std::vector<ObjectID>& getObjects() const ;

    /**
     * @brief Returns analysis objects list as strings.
     */
public:
    const std::vector<std::string>& getObjectsNames() const ;

    /**
     * @brief Returns access to input data points.
     */
public:
    const DataPointsList& getDataPoints(ObjectID __object_id) const ;
    
    /**
     * @brief Returns access to data point.
     */
public:
    const DataPoint& getDataPointByIndex(ObjectID __object_id, int __point_index) const ;
    
    /**
     * @brief Finds nearest data point index by required CDP number.
     */
public:
    int findNearestDataPointByNcdp(ObjectID __object_id, int __ncdp) const ;

    /**
     * @brief Creates reader for required version.
     */
public:
    DataReader * createDataReader(const int __object_id,
                                  SpectraDataType __data_type) const ;

    /**
     * @brief Opens input version.
     */
private:
    bool openInputVersion(const std::string& __grid_name,
                          const std::string& __version_name) ;
    
    /**
     * @brief Opens required version index.
     */
private:
    bool openVersionIndex() ;

    /**
     * @brief Extracts list of objects.
     */
private:
    bool extractObjectsList() ;

    

private:
    std::string m_grid_name ;                                       //!< Name of grid of input metacube.
    std::string m_version_name ;                                    //!< Name of input version.
    mutable std::unique_ptr<Prime::Metacube::MetaCube> m_metacube ; //!< Metacube with analysis data.
    Prime::Metacube::MetaCubeIndex * m_metacube_index = 0 ;         //!< Reading index of Metacube.
    HodographType m_analysis_hodograph_type ;                       //!< Type of analysis hodograph.
    bool m_is_data_in_time_domain = true ;                          //!< True if data in time domain and false if in depth domain.
    double m_data_sample_begin = 0 ;                                //!< Begin sample time/depth (in msec or meters). It is extracting from first trace (potentially buggy).
    std::vector<ObjectID> m_analysis_objects ;                      //!< List of analysis objects.
    std::vector<std::string> m_analysis_objects_names ;             //!< List of analysis objects names.
    std::map<ObjectID, DataPointsListPtr> m_data_points ;           //!< Points of data (with information about its portions)
} ;


}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_DATA_STORAGE_H */

