/* 
 * File:   FiltersViews.h
 * Author: rodin
 *
 * Created on February 18, 2017, 10:44 PM
 */


#ifndef PRIME_LIB_FILTERSLIB_FILTERSVIEWS_H
#define PRIME_LIB_FILTERSLIB_FILTERSVIEWS_H

#include <vector>

#include "toolkit/DialogCore.h"
#include "toolkit/Resources.h"
#include "toolkit/Window.h"
#include "toolkit/Main.h"
#include "filterslib/filters.h"


namespace Prime {
namespace lib {
namespace filterslib {

class FilterslibDialog ;

class BaseView
{
public:
    BaseView() ;
    virtual ~BaseView( ) {}

    virtual void init( FilterslibDialog * _dialog, int _box ) ;
    virtual bool connect( BaseFilter * _filter ) ;
    virtual void update() {}
    virtual std::string getTitle() const ;
    virtual PackingStyle getPackingStyle() const ;
    virtual DIALOG::BOX_PACKING getBoxPacking() const ;
    virtual MULTI_BOX * getParametersBox()
    {
        return parameters_box;
    }
    
    void showView() ;
    void hideView() ;
    
public:
    static std::vector<BaseView*> create_views() ;

protected:
    FilterslibDialog * getDialog() ;

protected:
    int box;
    MULTI_BOX * parameters_box ;
    BaseFilter * filter ;
    FilterslibDialog * dialog ;
} ;



class TimeDomainView : public BaseView
{
    static const std::string DEF_FILES_FILTER_NAME ; //!< Default name of files filter
    static const std::string DEF_FILES_FILTER_MASK ; //!< Default mask of files filter
    
    typedef Prime::lib::mathlib::SignalTypes Types ;
    
private:
    enum
    {
        ZERO_INDEX_ITEM = 0,
        WEIGHT_WINDOW_ITEM,
        TABLE_ITEM,
        TABLE_BUTTONS_ITEM,
        VIEW_ITEMS_NUM,
        
        TABLE_COLUMN_POS = 0,
        TABLE_COLUMN_AMPLITUDE,
        TABLE_COLUMNS_NUM,
        
        TABLE_BUTTON_ADD = 0,
        TABLE_BUTTON_DELETE,
        TABLE_BUTTON_OPEN
    } ;

public:
    TimeDomainView() ;
    
    void init( FilterslibDialog * _dialog, int _box ) ;
    bool connect( BaseFilter * _filter ) ;
    void update() ;
    std::string getTitle() const ;
    PackingStyle getPackingStyle() const ;
    DIALOG::BOX_PACKING getBoxPacking() const ;
    
private:
    static void zero_index_change_proc( int, int, int );
    static void window_change_proc( int, int, int, int );
    static void table_change_proc( int, int, int, int, int );
    static void table_selection_proc( int, int, int, const std::vector<int> & items ) ;
    static void table_buttons_box_proc( int dialog, int box, int item, int button, int subbutton, bool state );

private:
    std::vector<CONTAINER_ITEM> table ;
} ;



class FrequencyDomainView : public BaseView
{
    static const std::string DEF_FILES_FILTER_NAME ; //!< Default name of files filter
    static const std::string DEF_FILES_FILTER_MASK ; //!< Default mask of files filter
    
    typedef Prime::lib::mathlib::SignalTypes Types ;
    
private:
    enum
    {
        INTERPOLATION_TYPE_ITEM = 0,
        PHASE_TYPE_ITEM,
        NOISE_LEVEL_ITEM,
        MAX_LENGTH_ITEM,
        TABLE_ITEM,
        TABLE_BUTTONS_ITEM,
        VIEW_ITEMS_NUM,
        
        TABLE_COLUMN_POS = 0,
        TABLE_COLUMN_FREQUENCY,
        TABLE_COLUMN_AMPLITUDE,
        TABLE_COLUMNS_NUM,
        
        TABLE_BUTTON_ADD = 0,
        TABLE_BUTTON_DELETE,
        TABLE_BUTTON_SORT,
        TABLE_BUTTON_OPEN
    } ;
    
public:
    FrequencyDomainView() ;
    
    void init( FilterslibDialog * _dialog, int _box ) ;
    bool connect( BaseFilter * _filter ) ;
    void update() ;
    std::string getTitle() const ;
    PackingStyle getPackingStyle() const ;
    DIALOG::BOX_PACKING getBoxPacking() const ;
    
private:
    static void phase_change_proc( int dialog, int box, int item, int button ) ;
    static void interpolation_change_proc( int dialog, int box, int item, int button ) ;
    static void noise_change_proc( int dialog, int box, int item ) ;
    static void max_length_change_proc( int dialog, int box, int item, double value ) ;
    static void table_change_proc( int dialog, int box, int item, int n, int element ) ;
    static void table_selection_proc( int dialog, int box, int item, const std::vector<int> & items ) ;
    static void table_buttons_proc( int dialog, int box, int item, int button, int subbutton, bool state ) ;

private:
    std::vector<CONTAINER_ITEM> table ;
} ;



class RickerWaveletView : public BaseView
{
    enum
    {
        F0_ITEM = 0,
        LENGTH_ITEM,
        VIEW_ITEMS_NUM
    } ;

public:
    RickerWaveletView() ;
    
    bool connect( BaseFilter * _filter ) ;
    void update() ;
    std::string getTitle() const ;
    DIALOG::BOX_PACKING getBoxPacking( ) const ;
    
private:
    static void f0_change_proc( int, int, int );
    static void length_change_proc( int, int, int, double );
} ;



class AbsorptionView : public BaseView
{
    typedef Prime::lib::mathlib::SignalTypes Types ;
    
private:
    enum
    {
        FILTER_DIRECTION_ITEM,
        FILTER_TYPE_ITEM,
        NORMALIZATION_ITEM,
        FREQ_LOW_ITEM,
        FREQ_HIGH_ITEM,
        BETTA_ITEM,
        LENGTH_ITEM,
        VIEW_ITEMS_NUM
    } ;

public:
    AbsorptionView() ;
    
    bool connect( BaseFilter * _filter ) ;
    void update() ;
    std::string getTitle() const ;
    DIALOG::BOX_PACKING getBoxPacking() const ;
    
private:
    static void operator_direction_change_proc( int, int, int, int );
    static void operator_type_change_proc( int, int, int, int );
    static void normalization_change_proc( int, int, int, int ) ;
    
    static void flow_change_proc( int, int, int );
    static void fhigh_change_proc( int, int, int );
    static void betta_change_proc( int, int, int, double );
    static void length_change_proc( int, int, int, double );
};

class PhaseFilterView : public BaseView
{
    enum
    {
        PHASE_ITEM,
        LENGTH_ITEM,
        VIEW_ITEMS_NUM
    };

public:
    PhaseFilterView( );
    
    bool connect( BaseFilter * _filter ) ;
    void update() ;
    std::string getTitle() const ;
    DIALOG::BOX_PACKING getBoxPacking( ) ;
    
private:
    static void phase_change_proc( int, int, int );
    static void length_change_proc( int, int, int, double );
};

class LowFrequenciesPhaseFilterView : public BaseView
{
    enum
    {
        ALPHA_ITEM,
        BETTA_ITEM,
        LENGTH_ITEM,
        VIEW_ITEMS_NUM
    };

public:
    LowFrequenciesPhaseFilterView( );
    
    bool connect( BaseFilter * _filter ) ;
    void update() ;
    std::string getTitle() const ;
    DIALOG::BOX_PACKING getBoxPacking( ) ;
    
private:
    static void alpha_change_proc( int, int, int );
    static void betta_change_proc( int, int, int );
    static void length_change_proc( int, int, int, double);
};

class SweepView : public BaseView
{
    enum
    {
        FIRST_FREQ_ITEM,
        LAST_FREQ_ITEM,
        FIRST_TAPER_ITEM,
        LAST_TAPER_ITEM,
        LENGTH_ITEM,
        MODE_ITEM,
        REG_ITEM,
        WINMODE_ITEM,
        WINLEN_ITEM,
        VIEW_ITEMS_NUM,
    };

public:
    SweepView( );
    
    bool connect( BaseFilter * _filter ) ;
    void update() ;
    std::string getTitle() const ;
    DIALOG::BOX_PACKING getBoxPacking( ) ;
    
private:
    static void first_freq_change_proc( int, int, int );
    static void last_freq_change_proc( int, int, int );
    static void first_taper_change_proc( int, int, int );
    static void last_taper_change_proc( int, int, int );
    static void length_change_proc( int, int, int);
    static void waveletmode_select_proc(int, int, int, int);
    static void reg_change_proc( int, int, int);
    static void winlen_change_proc( int, int, int);
    static void winmode_select_proc (int, int, int, int);
    
};



}
}
}
#endif /* PRIME_LIB_FILTERSLIB_FILTERSVIEWS_H */

