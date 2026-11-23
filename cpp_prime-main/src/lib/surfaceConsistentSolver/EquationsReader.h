/* 
 * File:   BasePicksStorage.h
 * Author: rganisimov
 *
 * Created on February 25, 2017, 10:14 PM
 */

#ifndef PRIME_LIB_SURFACECONSISTENT_BASEPICKSSTORAGE_H
#define PRIME_LIB_SURFACECONSISTENT_BASEPICKSSTORAGE_H

#include <string>
#include <vector>
#include <omp.h>

#include "CommonTypes/DataVersions.h"
#include "DbTools/ProfInfo.h"
#include "dmolib/dmogates.h"
#include "Horizons/FRHorizonsTypes.h"
#include "Horizons/attribute.h"

#include "SurfacePoint.h"
#include "CoordinateCoder.h"
#include "EquationsSet.h"

namespace Prime {
namespace lib {
namespace SurfaceConsistent {

using Prime::DB::ProfInfo ;

/**
 * @brief Base class of equations (attribute picks) reader.
 * @detailed This class is a virtualization of
 * input data for surface consistent solver.
 */
class EquationsReader
{
    /**
     * Prototype of function for progress changing.
     */
    typedef void (*ProgressHandler)(int id, int value) ;

public:

    EquationsReader(EquationsSet * __equations_set,
                    const ProfInfo * __prof,
                    int __input_decimation = 1,
                    int __threads_num = 1 ) :
    equations_set(__equations_set),
    prof(__prof),
    input_decimation(__input_decimation),
    threads_num(__threads_num),
    progress_object_id(-1),
    progress_handler(NULL)
    {
        init() ;
    }

    /**
     * Method for getting type of library.
     */
protected:
    virtual Prime::DataCategory getLibType() const = 0 ;

    /**
     * Method for setting progress setter.
     * @param _progress_object_id Id of object which want to receive progress signal.
     * @param _progress_handler Function for setting progress state.
     */
public:

    void setProgressHandler(int _progress_object_id, ProgressHandler _progress_handler )
    {
        progress_object_id = _progress_object_id ;
        progress_handler = _progress_handler ;
    }

    /**
     * @brief Returns access to coordinates coder.
     */
protected:

    inline const CoordinateCoder* getCoder() const
    {
        return equations_set->getCoder() ;
    }

    /**
     * Method for class initialization.
     */
private:
    void init() ;

protected:
    EquationsSet * equations_set ;        //!< Pointer to output equations set.
    const ProfInfo * prof ;               //!< Link to profile info. Assumed that this class will be used in modules.
    int input_decimation ;                //!< Input data decimation factor.
    int threads_num ;                     //!< Number of available computational threads.
    double offset_step_x ;                //!< Step of offsets along x.
    double offset_step_y ;                //!< Step of offsets along y.
    int progress_object_id ;              //!< Id of object to which we will send progress signal.
    ProgressHandler progress_handler ;    //!< Function of progress changing.
    Prime::DataType data_type ;           //!< Type of processing data (2D or 3D).
    std::string object_name ;             //!< Name of processing object (line or grid name).
} ;

/**
 * @brief Base class of equations reader from picking library (*.gates files).
 */
class GatesFilesEquationsReader : public EquationsReader
{
    /**
     * Picks library container type.
     */
    typedef std::multimap<trInd, trField, runTimeSeismCmp> Container ;

    /**
     * Internal constants
     */
    static const float DEF_CRITICAL_WEIGHT ; //!< Minimum allowed weight. If weight is less then element will be dropped from system.

public:

    GatesFilesEquationsReader(EquationsSet * __equations_set,
                              const ProfInfo * _prof,
                              int _input_decimation = 1,
                              int _threads_num = 1) :
    EquationsReader( __equations_set, _prof, _input_decimation, _threads_num ) { }

    /**
     * Method for getting name of special
     * horizon at which picks is storing.
     */
protected:
    virtual const std::string& getPicksHorizonName() const = 0 ;

    /**
     * Method for getting name of attribute
     * which stores value of pick.
     */
protected:
    virtual const std::string& getPicksAttribName() const = 0 ;

    /**
     * Method for getting name of attribute
     * which stores weight of pick value.
     * If weighting is doesn't support just
     * return empty string.
     */
protected:
    virtual const std::string& getWeightAttribName() const = 0 ;

    /**
     * @name Reading interface.
     * @{
     */
public:
    bool read(const std::string& linename,
              const std::string& libname) ;

public:
    bool read(const std::vector<std::string>& lineslist,
              const std::string& libname) ;

protected:
    bool read(const std::string& linename,
              const std::string& libname,
              std::vector<SurfacePoint>& equations_points,
              std::vector<EquationsSet::ValueVector>& equations_values,
              omp_lock_t& db_lock,
              std::string& error) const ;

protected:
    bool readValues(const Container::const_iterator& it,
                    std::vector<float>& values,
                    std::vector<float>& weights) const ;
    //! @}
} ;

/**
 * @brief Time shifts equations reader.
 * @detailed Container of measured time shifts.
 */
class StaticsEquationsReader : public GatesFilesEquationsReader
{
public:

    StaticsEquationsReader(EquationsSet * __equations_set,
                           const ProfInfo * _prof,
                           int _input_decimation = 1,
                           int _threads_num = 1) :
    GatesFilesEquationsReader( __equations_set, _prof, _input_decimation, _threads_num ) { }

protected:

    Prime::DataCategory getLibType() const
    {
        return Prime::PICKING_TRACKER ;
    }

protected:

    const std::string& getPicksHorizonName() const
    {
        static const std::string name = PICKING_RUNTIME_SHIFTS ;
        return name ;
    }

protected:

    const std::string& getPicksAttribName() const
    {
        static const std::string name = GATHER_SHIFT ;
        return name ;
    }

protected:

    const std::string& getWeightAttribName() const
    {
        static const std::string name = GATHER_WEIGHT ;
        return name ;
    }
} ;

/**
 * @brief Amplitudes equations reader.
 * @detailed Container of measured amplitudes.
 */
class ScacEquationsReader : public GatesFilesEquationsReader
{
public:

    ScacEquationsReader(EquationsSet * __equations_set,
                        const ProfInfo * _prof,
                        int _input_decimation = 1,
                        int _threads_num = 1) :
    GatesFilesEquationsReader( __equations_set, _prof, _input_decimation, _threads_num ) { }

protected:

    Prime::DataCategory getLibType() const
    {
        return Prime::PICKING_WAVESTAT ;
    }

protected:

    const std::string& getPicksHorizonName() const
    {
        static const std::string name = SURF_CON_PARAMETERIZATION ;
        return name ;
    }

protected:

    const std::string& getPicksAttribName() const
    {
        static const std::string name = GATHER_AMPL ;
        return name ;
    }

protected:

    const std::string& getWeightAttribName() const
    {
        static const std::string name = "" ;
        return name ;
    }
} ;

/**
 * @brief Cepstr reader.
 * @detailed Container of measured cepstr.
 */
class DeconEquationsReader : public GatesFilesEquationsReader
{
public:

    DeconEquationsReader(EquationsSet * __equations_set,
                         const ProfInfo * _prof,
                         int _input_decimation = 1,
                         int _threads_num = 1) :
    GatesFilesEquationsReader( __equations_set, _prof, _input_decimation, _threads_num ) { }

protected:

    Prime::DataCategory getLibType() const
    {
        return Prime::PICKING_WAVESTAT ;
    }

protected:

    const std::string& getPicksHorizonName() const
    {
        static const std::string name = SURF_CON_PARAMETERIZATION ;
        return name ;
    }

protected:

    const std::string& getPicksAttribName() const
    {
        static const std::string name = GATHER_CEPSTR ;
        return name ;
    }

protected:

    const std::string& getWeightAttribName() const
    {
        static const std::string name = "" ;
        return name ;
    }
} ;

/**
 * @brief Time shifts equations reader.
 * @detailed Container of measured time shifts.
 */
class FirstBreaksEquationsReader : public EquationsReader
{
public:

    FirstBreaksEquationsReader(EquationsSet * __equations_set,
                               const ProfInfo * _prof,
                               int _input_decimation = 1,
                               int _threads_num = 1) :
    EquationsReader( __equations_set, _prof, _input_decimation, _threads_num ) { }

protected:

    Prime::DataCategory getLibType() const
    {
        return Prime::PICKING_FIRSTBREAKS ;
    }
    
    /**
     * @brief Reads picks from requested first breaks picking library.
     * @param[in] fbpicking_name Name of first breaks picking library.
     * @return True in successful case.
     */
public:
    bool read( const std::string& fbpicking_name ) ;
} ;

}
}
}

#endif

