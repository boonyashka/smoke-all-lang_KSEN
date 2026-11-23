
#include <stdlib.h>
#include <algorithm>
#include <iostream>

#include "toolkit/DialogInterface.h"
#include "toolkit/FileSelectionDialog.h"
#include "toolkit/MessageDialog.h"
#include "toolkit/Verificator.h"
#include "progpar/progpar.h"
#include "primeApp/PrimeAppNoX.h"
#include "files/FilesOperations.h"
#include "opsyserv/Verificator.h"
#include "mathlib/algorithms.h"

#include "dialog.h"
#include "filtersviews.h"


using namespace Prime::lib::primeApp ;


namespace Prime {
namespace lib {
namespace filterslib {

std::vector<BaseView *> BaseView::create_views()
{
    BaseView * view ;
    std::vector<BaseView *> views ;

    if ( (view = new TimeDomainView) )
        views.push_back( view ) ;

    if ( (view = new FrequencyDomainView) )
        views.push_back( view ) ;

    if ( (view = new RickerWaveletView) )
        views.push_back( view ) ;

    if ( (view = new AbsorptionView) )
        views.push_back( view ) ;
    
    if ( (view = new PhaseFilterView) )
        views.push_back( view ) ;

    if ( (view = new LowFrequenciesPhaseFilterView) )
        views.push_back( view ) ;
    
    if ( (view = new SweepView) )
        views.push_back( view) ;
    
    return views;
}


//
// BaseView
//

BaseView::BaseView() :
box(-1),
parameters_box(NULL),
filter(NULL),
dialog(NULL) { }

void BaseView::init( FilterslibDialog * _dialog, int _box )
{
    if ( !_dialog )
        return ;

    dialog = _dialog;
    box = _box;

    SetDialogBoxData( _dialog->getID(), box, "", this ) ;
}

bool BaseView::connect( BaseFilter * _filter )
{
    return false ;
}

std::string BaseView::getTitle() const
{
    return std::string();
}

PackingStyle BaseView::getPackingStyle() const
{
    return FIXED_SIZE_BOX ;
}

DIALOG::BOX_PACKING BaseView::getBoxPacking() const
{
    return DIALOG::BOX_PACKING();
}

FilterslibDialog * BaseView::getDialog()
{
    return dialog ;
}

void BaseView::showView()
{
    if ( getDialog() )
        DialogShowBox( getDialog()->getID(), box ) ;
}

void BaseView::hideView()
{
    if ( getDialog() )
        DialogHideBox( getDialog()->getID(), box ) ;
}



//
// TimeDomainView
//

const std::string TimeDomainView::DEF_FILES_FILTER_NAME = "Text file (*.txt)" ;
const std::string TimeDomainView::DEF_FILES_FILTER_MASK = "*.txt" ;

TimeDomainView::TimeDomainView()
{
    static TEXT_BOX zero_index_box[] = {
        { "Origin index", "0", true, 0, 10, nullptr, zero_index_change_proc, intTextVerificator, zero_index_change_proc },
        { 0 }
    };

    static OPTIONS_BOX window_box[] = {
        { "Windows", (char**) Types::NamesOfWeightingWindowTypes, window_change_proc },
        { 0 }
    };

    static CONTAINER_COLUMN table_columns[] = {
        { " # ",         CONTAINER_ITEM_ELEMENT::LABEL_ELEMENT },
        { " Amplitude ", CONTAINER_ITEM_ELEMENT::TEXT_ELEMENT  },
        { NULL}
    };

    static CONTAINER_BOX table_box = {
        CONTAINER_BOX::TREE_STORE, table_columns, 10, table_selection_proc, NULL, NULL, table_change_proc
    };

    static BUTTON_BOX table_buttons_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 3, NULL, NULL, 1, table_buttons_box_proc, false, true
    };

    static MULTI_BOX box_prototype[] = {
        { &zero_index_box,    TEXT_BOX_D      },
        { &window_box,        OPTIONS_BOX_D   },
        { &table_box,         CONTAINER_BOX_D },
        { &table_buttons_box, BUTTON_BOX_D    },
        { 0 }
    };

    parameters_box = new MULTI_BOX[VIEW_ITEMS_NUM + 1] ;

    for (int i = 0; i <= VIEW_ITEMS_NUM; i++)
        parameters_box[i] = box_prototype[i] ;
}

void TimeDomainView::init( FilterslibDialog * _dialog, int _box )
{
    BaseView::init( _dialog, _box ) ;

    if ( !getDialog() )
        return ;

    DialogButtonSetPixmap(getDialog()->getID(), box, TABLE_BUTTONS_ITEM,
                                TABLE_BUTTON_ADD, "plus_24px.png");

    DialogButtonSetPixmap(getDialog()->getID(), box, TABLE_BUTTONS_ITEM,
                                TABLE_BUTTON_DELETE, "delete.png");

    DialogButtonSetPixmap(getDialog()->getID(), box, TABLE_BUTTONS_ITEM,
                                TABLE_BUTTON_OPEN, "open.png");
}

bool TimeDomainView::connect( BaseFilter * _filter )
{
    if ( !_filter || _filter->getType() != BaseFilter::TimeDomainFilterType )
    {
        filter = NULL ;
        return false ;
    }

    filter = _filter ;
    return true ;
}

void TimeDomainView::update()
{
    if ( !getDialog() || !filter )
        return ;

    TimeDomainFilter * _filter = (TimeDomainFilter *) filter ;

    DialogSetTextValueAsInt( getDialog()->getID(), box, ZERO_INDEX_ITEM, _filter->getOriginIndex() ) ;
    DialogSetOptionValue( getDialog()->getID(), box, WEIGHT_WINDOW_ITEM, _filter->getWindow() ) ;

    std::vector<float> samples = _filter->getSamples() ;
    table.resize( samples.size() ) ;

    for ( size_t irow = 0; irow < samples.size(); ++irow )
    {
        CONTAINER_ITEM &item = table.at(irow) ;

        std::ostringstream label ;
        label << irow ;

        item.parentItem = -1 ;
        //item.label = label.str() ;
        item.columns.resize(TABLE_COLUMNS_NUM) ;

        for ( int column = 0; column < TABLE_COLUMNS_NUM; ++column )
        {
            CONTAINER_ITEM_ELEMENT& element = item.columns[column] ;

            switch ( column )
            {
                case TABLE_COLUMN_POS:
                {
                    element.type = CONTAINER_ITEM_ELEMENT::LABEL_ELEMENT ;
                    
                    element.label = label.str() ;
                }
                    break;
                                  
                case TABLE_COLUMN_AMPLITUDE:
                {
                    element.type = CONTAINER_ITEM_ELEMENT::TEXT_ELEMENT ;

                    std::ostringstream str ;
                    str << samples[irow] ;
                    element.text = str.str() ;
                }
                    break;
            }
        }
    }

    DialogSetContainerStorage( getDialog()->getID(), box, TABLE_ITEM, table ) ;
    DialogContainerStorageUpdateAll( getDialog()->getID(), box, TABLE_ITEM ) ;
    DialogDisableItem( getDialog()->getID(), box, TABLE_BUTTONS_ITEM, TABLE_BUTTON_DELETE ) ;

    getDialog()->refresh() ;
}

std::string TimeDomainView::getTitle() const
{
    return " Time domain signature " ;
}

PackingStyle TimeDomainView::getPackingStyle() const
{
    return PROPORTIONAL_BOX ;
}

DIALOG::BOX_PACKING TimeDomainView::getBoxPacking() const
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX   },
        { 1, FIXED_SIZE_BOX   },
        { 1, PROPORTIONAL_BOX },
        { 1, FIXED_SIZE_BOX   },
        { 0 }
    };

    DIALOG::BOX_PACKING packing = {
        HORIZONTAL_PACKING, pack_line
    };

    return packing ;
}

void TimeDomainView::zero_index_change_proc(int dialog, int box, int item)
{
    TimeDomainView * view = (TimeDomainView*) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    TimeDomainFilter * filter = (TimeDomainFilter *) view->filter ;
    int value = DialogGetTextValueAsInt(dialog, box, item) ;
    filter->setOriginIndex( value ) ;
    view->update() ;
}

void TimeDomainView::window_change_proc(int dialog, int box, int item, int button)
{
    TimeDomainView * view = (TimeDomainView*) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    TimeDomainFilter * filter = (TimeDomainFilter *) view->filter ;
    int window = DialogGetOptionValue(dialog, box, item ) ;
    filter->setWindow( Types::WeightingWindowType(window) ) ;
    view->update() ;
}

void TimeDomainView::table_change_proc(int dialog, int box, int item, int n, int column)
{
    TimeDomainView * view = (TimeDomainView*) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    TimeDomainFilter * filter = (TimeDomainFilter *) view->filter ;

    std::vector<float> samples = filter->getSamples() ;
    if ( n < 0 || n >= samples.size() )
        return ;

    const std::string& text = view->table.at(n).columns[column].text ;
    if ( !verifyFloatString(text.c_str()) )
        return ;

    samples[n] = atof( text.c_str() ) ;
    filter->setSamples(samples) ;
    view->update() ;
}

void TimeDomainView::table_selection_proc(int dialog, int box, int item, const std::vector<int> & items)
{
    DialogSetEnableItem( dialog, box, TABLE_BUTTONS_ITEM, TABLE_BUTTON_DELETE, !items.empty() ) ;
}

void TimeDomainView::table_buttons_box_proc(int dialog, int box, int item, int button, int subbutton, bool state)
{
    TimeDomainView *view = (TimeDomainView *) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->filter || !view->getDialog() )
        return ;

    TimeDomainFilter * filter = (TimeDomainFilter *) (view->filter) ;

    std::vector<float> samples ;

    switch ( button )
    {
        case TABLE_BUTTON_ADD:
        {
            samples = filter->getSamples() ;
            samples.push_back(0.0) ;
            filter->setSamples(samples) ;
        }
            break;

        case TABLE_BUTTON_DELETE:
        {
            std::vector<int> items = DialogGetContainerActiveItems( view->getDialog()->getID(), view->box, TABLE_ITEM ) ;

            if ( !items.empty() )
            {
                samples = filter->getSamples() ;
                std::sort( items.rbegin(), items.rend() ) ;

                for ( size_t i = 0; i < items.size(); ++i )
                {
                    if ( items[i] >= 0 && items[i] < samples.size() )
                        samples.erase( samples.begin() + items[i] ) ;
                }
            }

            filter->setSamples( samples ) ;
        }
            break;

        case TABLE_BUTTON_OPEN:
        {
            std::string filename ;
            std::string path = ( getProjCD() ? getProjCD()->getCommondir() : "~" ) ;

            if ( OpenFileSelectionDialog(FILE_SELECTION_CUSTOM, FILE_SELECTION_OPEN, false,
                                         "Load filter", filename, path.c_str(),
                                         DEF_FILES_FILTER_NAME.c_str(),
                                         DEF_FILES_FILTER_MASK.c_str(),
                                         view->getDialog()->getID()) == 0 )
            {
                try
                {
                    //Opening txt file
                    std::ifstream file( filename.c_str(), std::ifstream::in ) ;
                    if ( !file.is_open() )
                        throw std::runtime_error( "Can't open the file '" + filename + "'" ) ;

                    std::string str ;
                    getline(file, str, '\0') ;

                    //Parsing text file
                    std::istringstream in(str) ;

                    while (in)
                    {
                        float value ;
                        in >> value ;

                        if ( in.eof() )
                            break ;

                        samples.push_back( value ) ;
                    }

                    //Saving changes
                    filter->setSamples( samples ) ;
                }
                catch (std::exception & e)
                {
                    std::cerr << "TimeDomainView::model_buttons_box_proc(): Error. " << e.what() << std::endl ;
                    DisplayErrorDialog( "Error", "Failed to open file" ) ;
                    return ;
                }
            }
        }
            break;
    }

    view->update();
}



//
// FrequencyDomainVIew
//

const std::string FrequencyDomainView::DEF_FILES_FILTER_NAME = "Text file (*.txt)" ;
const std::string FrequencyDomainView::DEF_FILES_FILTER_MASK = "*.txt" ;

FrequencyDomainView::FrequencyDomainView()
{
    static OPTIONS_BOX interpolation_box[] = {
        { "Interpolation", FrequencyDomainFilter::NamesOfInterpolations, interpolation_change_proc},
        { 0 }
    };

    static OPTIONS_BOX phase_box[] = {
        { "Phase", (char**) Types::NamesOfPhaseTypes, phase_change_proc },
        { 0 }
    };

    static TEXT_BOX noise_box[] = {
        { "White noise level", "0.01", true, 0, 10, nullptr, noise_change_proc, NULL, noise_change_proc },
        { 0 }
    };

    static STEPPER_BOX max_length_box[] = {
        { "Max length, samples", 5, 201, 1, 1101, 2, 0, max_length_change_proc },
        { 0 }
    };

    static CONTAINER_COLUMN table_columns[] = {
        { " # ", CONTAINER_ITEM_ELEMENT::LABEL_ELEMENT},
        { " Frequency ", CONTAINER_ITEM_ELEMENT::TEXT_ELEMENT},
        { " Amplitude ", CONTAINER_ITEM_ELEMENT::TEXT_ELEMENT},
        { NULL}
    };

    static CONTAINER_BOX table_box = {
        CONTAINER_BOX::TREE_STORE, table_columns, 10, table_selection_proc, NULL, NULL, table_change_proc
    };

    static BUTTON_BOX table_buttons_box = {
        BUTTON_BOX::DEFAULT_LAYOUT, 4, NULL, NULL, 1, table_buttons_proc, false, true
    };

    static MULTI_BOX box_prototype[] = {
        { interpolation_box,  OPTIONS_BOX_D   },
        { phase_box,          OPTIONS_BOX_D   },
        { noise_box,          TEXT_BOX_D      },
        { max_length_box,     STEPPER_BOX_D   },
        { &table_box,         CONTAINER_BOX_D },
        { &table_buttons_box, BUTTON_BOX_D    },
        { 0 }
    };

    parameters_box = new MULTI_BOX[VIEW_ITEMS_NUM + 1] ;

    for (int i = 0; i <= VIEW_ITEMS_NUM; i++)
        parameters_box[i] = box_prototype[i] ;
}

void FrequencyDomainView::init( FilterslibDialog * _dialog, int _box )
{
    BaseView::init( _dialog, _box ) ;

    if ( !getDialog() )
        return ;

    DialogButtonSetPixmap(getDialog()->getID(), box, TABLE_BUTTONS_ITEM,
                                TABLE_BUTTON_ADD, "plus_24px.png");

    DialogButtonSetPixmap(getDialog()->getID(), box, TABLE_BUTTONS_ITEM,
                                TABLE_BUTTON_DELETE, "delete.png");

    DialogButtonSetPixmap(getDialog()->getID(), box, TABLE_BUTTONS_ITEM,
                                TABLE_BUTTON_SORT, "SortModule.png");

    DialogButtonSetPixmap(getDialog()->getID(), box, TABLE_BUTTONS_ITEM,
                                TABLE_BUTTON_OPEN, "open.png");
}

bool FrequencyDomainView::connect(BaseFilter * _filter)
{
    if ( !_filter || _filter->getType() != BaseFilter::FrequencyDomainFilterType )
    {
        filter = NULL ;
        return false ;
    }

    filter = _filter ;
    return true ;
}

void FrequencyDomainView::update()
{
    if ( !getDialog() || !filter )
        return ;

    FrequencyDomainFilter * _filter = (FrequencyDomainFilter *) filter ;

    DialogSetOptionValue( getDialog()->getID(), box, INTERPOLATION_TYPE_ITEM, _filter->getInterpolation() ) ;
    DialogSetOptionValue( getDialog()->getID(), box, PHASE_TYPE_ITEM, _filter->getPhase() ) ;
    DialogSetTextValueAsDouble( getDialog()->getID(), box, NOISE_LEVEL_ITEM, _filter->getNoise(), 3, false ) ;
    DialogSetStepperValue( getDialog()->getID(), box, MAX_LENGTH_ITEM, _filter->getMaxLength() ) ;
    DialogSetShowItem( getDialog()->getID(), box, NOISE_LEVEL_ITEM, _filter->getPhase() == Types::MinPhase ) ;

    table.clear() ;
    const std::vector<float>& frequencies = _filter->getFrequencies() ;
    const std::vector<float>& amplitudes = _filter->getAmplitudes() ;

    if ( frequencies.size() == amplitudes.size() )
    {
        table.resize( frequencies.size() ) ;

        for ( int row = 0; row < frequencies.size(); ++row )
        {
            CONTAINER_ITEM &item = table.at(row) ;
            item.parentItem = -1 ;
            //item.label = toString("%d", row + 1) ;
            item.columns.resize(TABLE_COLUMNS_NUM) ;

            for ( int column = 0; column < TABLE_COLUMNS_NUM; ++column )
            {
                CONTAINER_ITEM_ELEMENT &element = item.columns[column];

                switch ( column )
                {
                    case TABLE_COLUMN_POS:
                        element.type = CONTAINER_ITEM_ELEMENT::LABEL_ELEMENT;
                        element.text = toString("%d", row + 1);
                        break;
                                          
                    case TABLE_COLUMN_FREQUENCY:
                        element.type = CONTAINER_ITEM_ELEMENT::TEXT_ELEMENT;
                        element.text = toString("%.1f", frequencies[row]);
                        break;

                    case TABLE_COLUMN_AMPLITUDE:
                        element.type = CONTAINER_ITEM_ELEMENT::TEXT_ELEMENT;
                        element.text = toString("%.2f", amplitudes[row]);
                        break;
                }
            }
        }
    }

    DialogSetContainerStorage( getDialog()->getID(), box, TABLE_ITEM, table ) ;
    DialogContainerStorageUpdateAll( getDialog()->getID(), box, TABLE_ITEM ) ;
    DialogDisableItem( getDialog()->getID(), box, TABLE_BUTTONS_ITEM, TABLE_BUTTON_DELETE ) ;

    getDialog()->refresh() ;
}

std::string FrequencyDomainView::getTitle() const
{
    return " Frequency domain filter " ;
}

PackingStyle FrequencyDomainView::getPackingStyle() const
{
    return PROPORTIONAL_BOX ;
}

DIALOG::BOX_PACKING FrequencyDomainView::getBoxPacking() const
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX   },
        { 1, FIXED_SIZE_BOX   },
        { 1, FIXED_SIZE_BOX   },
        { 1, FIXED_SIZE_BOX   },
        { 1, PROPORTIONAL_BOX },
        { 1, FIXED_SIZE_BOX   },
        { 0 }
    };

    DIALOG::BOX_PACKING packing = {
        HORIZONTAL_PACKING, pack_line
    };

    return packing ;
}

void FrequencyDomainView::phase_change_proc(int dialog, int box, int item, int button)
{
    FrequencyDomainView * view = (FrequencyDomainView *) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    FrequencyDomainFilter * filter = (FrequencyDomainFilter *) view->filter ;
    filter->setPhase( Types::PhaseType(button) ) ;
    view->update() ;
}

void FrequencyDomainView::interpolation_change_proc(int dialog, int box, int item, int button)
{
    FrequencyDomainView * view = (FrequencyDomainView *) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    FrequencyDomainFilter * filter = (FrequencyDomainFilter *) view->filter ;
    filter->setInterpolation( FrequencyDomainFilter::InterpolationType(button) ) ;
    view->update() ;
}

void FrequencyDomainView::noise_change_proc(int dialog, int box, int item)
{
    FrequencyDomainView * view = (FrequencyDomainView *) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    FrequencyDomainFilter * filter = (FrequencyDomainFilter *) view->filter ;
    filter->setNoise( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void FrequencyDomainView::max_length_change_proc( int dialog, int box, int item, double value )
{
    FrequencyDomainView * view = (FrequencyDomainView *) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    FrequencyDomainFilter * filter = (FrequencyDomainFilter *) view->filter ;
    filter->setMaxLength( value ) ;
    view->update() ;
}

void FrequencyDomainView::table_change_proc(int dialog, int box, int item, int n, int column)
{
    FrequencyDomainView * view = (FrequencyDomainView *) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    FrequencyDomainFilter * filter = (FrequencyDomainFilter *) view->filter ;
    std::vector<float> frequencies = filter->getFrequencies() ;
    std::vector<float> amplitudes = filter->getAmplitudes() ;

    if (frequencies.size() != amplitudes.size() ||
        n < 0 || n >= frequencies.size() )
        return ;

    const std::string& text = view->table.at(n).columns[column].text ;

    switch ( column )
    {
        case TABLE_COLUMN_FREQUENCY:
            frequencies[n] = atof( text.c_str() ) ;
            break;

        case TABLE_COLUMN_AMPLITUDE:
            amplitudes[n] = atof( text.c_str() ) ;
            break;
    }

    filter->setSpectrumPoints( frequencies, amplitudes ) ;
    view->update() ;
}

void FrequencyDomainView::table_selection_proc(int dialog, int box, int item, const std::vector<int> & items)
{
    DialogSetEnableItem( dialog, box, TABLE_BUTTONS_ITEM, TABLE_BUTTON_DELETE, !items.empty() ) ;
}

void FrequencyDomainView::table_buttons_proc(int dialog, int box, int item, int button, int subbutton, bool state)
{
    FrequencyDomainView * view = (FrequencyDomainView*) GetDialogBoxData(dialog, box, "");

    if ( !view || !view->filter || !view->getDialog() )
        return ;

    FrequencyDomainFilter * filter = (FrequencyDomainFilter *) view->filter ;

    std::vector<float> frequencies, amplitudes ;

    switch ( button )
    {
        case TABLE_BUTTON_ADD:
        {
            frequencies = filter->getFrequencies() ;
            amplitudes = filter->getAmplitudes() ;
            frequencies.push_back( 0 ) ;
            amplitudes.push_back( 0 ) ;

            filter->setSpectrumPoints( frequencies, amplitudes ) ;
        }
            break;

        case TABLE_BUTTON_DELETE:
        {
            std::vector<int> items = DialogGetContainerActiveItems( view->getDialog()->getID(), view->box, TABLE_ITEM ) ;

            if ( !items.empty() )
            {
                std::sort( items.rbegin(), items.rend() ) ;

                frequencies = filter->getFrequencies() ;
                amplitudes = filter->getAmplitudes() ;

                for ( size_t i = 0; i < items.size(); ++i )
                {
                    if ( items[i] >= 0 && items[i] < frequencies.size() )
                        frequencies.erase( frequencies.begin() + items[i] ) ;

                    if ( items[i] >= 0 && items[i] < amplitudes.size() )
                        amplitudes.erase( amplitudes.begin() + items[i] ) ;
                }
            }

            filter->setSpectrumPoints( frequencies, amplitudes ) ;
        }
            break;

        case TABLE_BUTTON_SORT:
        {
            frequencies = filter->getFrequencies() ;
            amplitudes = filter->getAmplitudes() ;
            Prime::lib::mathlib::sort( &frequencies, &amplitudes ) ;

            filter->setSpectrumPoints( frequencies, amplitudes ) ;
        }
            break ;

        case TABLE_BUTTON_OPEN:
        {
            std::string filename ;
            std::string path = ( getProjCD() ? getProjCD()->getCommondir() : "~" ) ;

            if ( OpenFileSelectionDialog(FILE_SELECTION_CUSTOM, FILE_SELECTION_OPEN, false,
                                         "Load filter", filename, path.c_str(),
                                         DEF_FILES_FILTER_NAME.c_str(),
                                         DEF_FILES_FILTER_MASK.c_str(),
                                         view->getDialog()->getID()) == 0 )
            {
                try
                {
                    std::ifstream file(filename.c_str(), std::ifstream::in);
                    if ( !file.is_open() )
                        throw std::runtime_error( "Can't open the file '" + filename + "'" ) ;

                    std::string str;
                    getline(file, str, '\0');

                    //Parsing text file
                    std::istringstream in(str) ;

                    for ( int index = 0; in; ++index )
                    {
                        float value ;
                        in >> value ;

                        if ( in.eof() )
                            break ;

                        if ( index % 2 == 0)
                            frequencies.push_back( value ) ;
                        else
                            amplitudes.push_back( value ) ;
                    }

                    if ( frequencies.size() != amplitudes.size() )
                        throw std::runtime_error( "Can't parse the file '" + filename + "'" ) ;

                    //Saving changes
                    filter->setSpectrumPoints( frequencies, amplitudes ) ;
                }
                catch (std::exception & e)
                {
                    std::cerr << "FrequencyDomainView::table_buttons_proc(): Error. " << e.what() << std::endl ;
                    DisplayErrorDialog( "Error", "Failed to open file" ) ;
                }
            }
        }
            break;
    }

    view->update() ;
}



//
// RickerWaveletView
//

RickerWaveletView::RickerWaveletView()
{
    static TEXT_BOX f0_index_box[] = {
        { "f0, Hz", "0", true, 0, 10, nullptr, f0_change_proc, ufloatTextVerificator, f0_change_proc },
        { 0 }
    };

    static STEPPER_BOX length_box[] = {
        { "Length, samples", 5, 101, 11, 1101, 2, 0, length_change_proc },
        { 0 }
    };

    static MULTI_BOX box_prototype[] = {
        { f0_index_box, TEXT_BOX_D    },
        { length_box,   STEPPER_BOX_D },
        { 0 }
    };

    parameters_box = new MULTI_BOX[VIEW_ITEMS_NUM + 1] ;

    for (int i = 0; i <= VIEW_ITEMS_NUM; i++)
        parameters_box[i] = box_prototype[i];
}

bool RickerWaveletView::connect(BaseFilter * _filter)
{
    if ( !_filter || _filter->getType() != BaseFilter::RickerWaveletType )
    {
        filter = NULL ;
        return false ;
    }

    filter = _filter ;
    return true ;
}

void RickerWaveletView::update()
{
    if ( !getDialog() || !filter )
        return ;

    RickerWavelet* _filter = (RickerWavelet *) filter ;
    DialogSetTextValueAsDouble( getDialog()->getID(), box, F0_ITEM, _filter->getF0(), 0 ) ;
    DialogSetStepperValue( getDialog()->getID(), box, LENGTH_ITEM, _filter->getLength() ) ;

    getDialog()->refresh() ;
}

std::string RickerWaveletView::getTitle() const
{
    return " Ricker wavelet " ;
}

DIALOG::BOX_PACKING RickerWaveletView::getBoxPacking() const
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 0}
    };

    DIALOG::BOX_PACKING packing = {
        HORIZONTAL_PACKING, pack_line
    };

    return packing ;
}

void RickerWaveletView::f0_change_proc(int dialog, int box, int item)
{
    RickerWaveletView * view = (RickerWaveletView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;


    RickerWavelet * filter = (RickerWavelet *) view->filter ;
    filter->setF0( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void RickerWaveletView::length_change_proc(int dialog, int box, int item, double value)
{
    RickerWaveletView * view = (RickerWaveletView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;


    RickerWavelet * filter = (RickerWavelet *) view->filter ;
    filter->setLength( value ) ;
    view->update() ;
}



//
// AbsorptionView
//

AbsorptionView::AbsorptionView()
{
    static OPTIONS_BOX filter_direction_box[] = {
        { "Direction", (char**) Types::NamesOfFilterDirectionTypes, operator_direction_change_proc},
        { 0 }
    };

    static OPTIONS_BOX filter_type_box[] = {
        { "Type", (char**) Types::NamesOfAbsorptionFilterTypes, operator_type_change_proc},
        { 0 }
    };

    static OPTIONS_BOX normalization_box[] = {
        { "Normalization", (char**) AbsorptionOperator::NamesOfNormalizationTypes, normalization_change_proc },
        { 0 }
    };

    static TEXT_BOX f_low_box[] = {
        { "Low frequency, Hz", "0", true, 0, 10, nullptr, flow_change_proc, ufloatTextVerificator, flow_change_proc },
        { 0 }
    };

    static TEXT_BOX f_high_box[] = {
        { "High frequency, Hz ", "0", true, 0, 10, nullptr, fhigh_change_proc, ufloatTextVerificator, fhigh_change_proc },
        { 0 }
    };

    static STEPPER_BOX betta_box[] = {
        { "Betta", 5, 0, 0, 10, 0.001, 3, betta_change_proc },
        { 0 }
    };

    static STEPPER_BOX length_box[] = {
        { "Length, samples", 5, 101, 11, 1101, 2, 0, length_change_proc },
        { 0 }
    };

    static MULTI_BOX box_prototype[] = {
        { filter_direction_box, OPTIONS_BOX_D },
        { filter_type_box,      OPTIONS_BOX_D },
        { normalization_box,    OPTIONS_BOX_D },
        { f_low_box,            TEXT_BOX_D    },
        { f_high_box,           TEXT_BOX_D    },
        { betta_box,            STEPPER_BOX_D },
        { length_box,           STEPPER_BOX_D },
        { 0 }
    };

    parameters_box = new MULTI_BOX[VIEW_ITEMS_NUM + 1];

    for (int i = 0; i <= VIEW_ITEMS_NUM; i++)
        parameters_box[i] = box_prototype[i] ;
}

bool AbsorptionView::connect(BaseFilter * _filter)
{
    if ( !_filter || _filter->getType() != BaseFilter::AbsorptionOperatorType )
    {
        filter = NULL ;
        return false ;
    }

    filter = _filter ;
    return true ;
}

void AbsorptionView::update()
{
    if ( !getDialog() || !filter )
        return ;

    const int id = getDialog()->getID() ;
    AbsorptionOperator* _filter = (AbsorptionOperator *) filter ;
    DialogSetOptionValue( id, box, FILTER_DIRECTION_ITEM, int(_filter->getFilterDirection()) ) ;
    DialogSetOptionValue( id, box, FILTER_TYPE_ITEM, int(_filter->getFilterType()) ) ;
    DialogSetOptionValue( id, box, NORMALIZATION_ITEM, int(_filter->getNormalization()) ) ;
    DialogSetTextValueAsDouble( id, box, FREQ_LOW_ITEM, _filter->getFLow(), 0 ) ;
    DialogSetTextValueAsDouble( id, box, FREQ_HIGH_ITEM, _filter->getFHigh(), 0 ) ;
    DialogSetStepperValue( id, box, BETTA_ITEM, _filter->getBetta() ) ;
    DialogSetStepperValue( id, box, LENGTH_ITEM, _filter->getLength() ) ;

    const bool is_inverse = (_filter->getFilterDirection() == Types::InverseFilter) ;
    const bool is_phase = (_filter->getFilterType() == Types::AbsorptionPhaseFilter) ;
    DialogSetShowItem( id, box, NORMALIZATION_ITEM, is_inverse && !is_phase ) ;
    DialogSetShowItem( id, box, FREQ_LOW_ITEM, is_inverse && !is_phase );
    DialogSetShowItem( id, box, FREQ_HIGH_ITEM, is_inverse && !is_phase ) ;

    getDialog()->refresh() ;
}

std::string AbsorptionView::getTitle() const
{
    return " Absorption operator " ;
}

DIALOG::BOX_PACKING AbsorptionView::getBoxPacking() const
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING packing = {
        HORIZONTAL_PACKING, pack_line
    };

    return packing ;
}

void AbsorptionView::normalization_change_proc(int dialog, int box, int item, int button)
{
    AbsorptionView * view = (AbsorptionView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    AbsorptionOperator * filter = (AbsorptionOperator *) view->filter ;
    filter->setNormalization( AbsorptionOperator::NormalizationType(button) ) ;
    view->update() ;
}

void AbsorptionView::operator_direction_change_proc(int dialog, int box, int item, int button)
{
    AbsorptionView * view = (AbsorptionView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    AbsorptionOperator * filter = (AbsorptionOperator *) view->filter ;
    filter->setFilterDirection( Types::FilterDirectionType(button) ) ;
    view->update() ;
}

void AbsorptionView::operator_type_change_proc(int dialog, int box, int item, int button)
{
    AbsorptionView * view = (AbsorptionView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    AbsorptionOperator * filter = (AbsorptionOperator *) view->filter ;
    filter->setFilterType( Types::AbsorptionFilterType(button) ) ;
    view->update() ;
}

void AbsorptionView::flow_change_proc(int dialog, int box, int item)
{
    AbsorptionView * view = (AbsorptionView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    AbsorptionOperator * filter = (AbsorptionOperator *) view->filter ;
    filter->setFLow( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void AbsorptionView::fhigh_change_proc(int dialog, int box, int item)
{
    AbsorptionView * view = (AbsorptionView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    AbsorptionOperator * filter = (AbsorptionOperator *) view->filter ;
    filter->setFHigh( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void AbsorptionView::betta_change_proc(int dialog, int box, int item, double value)
{
    AbsorptionView * view = (AbsorptionView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    AbsorptionOperator * filter = (AbsorptionOperator *) view->filter ;
    filter->setBetta( value ) ;
    view->update() ;
}

void AbsorptionView::length_change_proc(int dialog, int box, int item, double value)
{
    AbsorptionView * view = (AbsorptionView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    AbsorptionOperator * filter = (AbsorptionOperator *) view->filter ;
    filter->setLength( value ) ;
    view->update() ;
}


//
//
// PhaseFilterView
//
//

PhaseFilterView::PhaseFilterView()
{
    static TEXT_BOX phase_box[] = {
        { "Phase, degrees", "0", true, 0, 10, nullptr, phase_change_proc, NULL, phase_change_proc },
        { 0 }
    };

    static STEPPER_BOX length_box[] = {
        { "Length, samples", 5, 101, 11, 1101, 2, 0, length_change_proc },
        { 0 }
    };

    static MULTI_BOX box_prototype[] = {
        { phase_box,  TEXT_BOX_D    },
        { length_box, STEPPER_BOX_D },
        { 0 }
    };

    parameters_box = new MULTI_BOX[VIEW_ITEMS_NUM + 1];

    for (int i = 0; i <= VIEW_ITEMS_NUM; i++)
        parameters_box[i] = box_prototype[i] ;
}

bool PhaseFilterView::connect(BaseFilter * _filter)
{
    if ( !_filter || _filter->getType() != BaseFilter::PhaseFilterType )
    {
        filter = NULL ;
        return false ;
    }

    filter = _filter ;
    return true ;
}

void PhaseFilterView::update()
{
    if ( !getDialog() || !filter )
        return ;

    const int id = getDialog()->getID() ;
    PhaseFilter * _filter = (PhaseFilter *) filter ;
    DialogSetTextValueAsDouble( id, box, PHASE_ITEM, _filter->getPhase(), 2 ) ;
    DialogSetStepperValue( id, box, LENGTH_ITEM, _filter->getLength() ) ;

    getDialog()->refresh() ;
}

std::string PhaseFilterView::getTitle() const
{
    return " Phase rotation filter " ;
}

DIALOG::BOX_PACKING PhaseFilterView::getBoxPacking()
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING packing = {
        HORIZONTAL_PACKING, pack_line
    };

    return packing ;
}

void PhaseFilterView::phase_change_proc(int dialog, int box, int item)
{
    PhaseFilterView * view = (PhaseFilterView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    PhaseFilter * filter = (PhaseFilter *) view->filter ;
    filter->setPhase( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void PhaseFilterView::length_change_proc(int dialog, int box, int item, double value)
{
    PhaseFilterView * view = (PhaseFilterView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    PhaseFilter * filter = (PhaseFilter *) view->filter ;
    filter->setLength( value ) ;
    view->update() ;
}


//
//
// LowFrequenciesPhaseFilterView
//
//

LowFrequenciesPhaseFilterView::LowFrequenciesPhaseFilterView()
{
    static TEXT_BOX alpha_box[] = {
        { "Alpha", "1", true, 0, 10, nullptr, alpha_change_proc, NULL, alpha_change_proc },
        { 0 }
    };

    static TEXT_BOX betta_box[] = {
        { "Betta, Hz", "10", true, 0, 10, nullptr, betta_change_proc, NULL, betta_change_proc },
        { 0 }
    };

    static STEPPER_BOX length_box[] = {
        { "Length, samples", 5, 101, 11, 1101, 2, 0, length_change_proc },
        { 0 }
    };

    static MULTI_BOX box_prototype[] = {
        { alpha_box,  TEXT_BOX_D    },
        { betta_box,  TEXT_BOX_D    },
        { length_box, STEPPER_BOX_D },
        { 0 }
    };

    parameters_box = new MULTI_BOX[VIEW_ITEMS_NUM + 1];

    for (int i = 0; i <= VIEW_ITEMS_NUM; i++)
        parameters_box[i] = box_prototype[i] ;
}

bool LowFrequenciesPhaseFilterView::connect(BaseFilter * _filter)
{
    if ( !_filter || _filter->getType() != BaseFilter::LowFrequenciesPhaseFilterType )
    {
        filter = NULL ;
        return false ;
    }

    filter = _filter ;
    return true ;
}

void LowFrequenciesPhaseFilterView::update()
{
    if ( !getDialog() || !filter )
        return ;

    const int id = getDialog()->getID() ;
    LowFrequenciesPhaseFilter * _filter = (LowFrequenciesPhaseFilter *) filter ;
    DialogSetTextValueAsDouble( id, box, ALPHA_ITEM, _filter->getAlpha(), 2 ) ;
    DialogSetTextValueAsDouble( id, box, BETTA_ITEM, _filter->getBetta(), 0 ) ;
    DialogSetStepperValue( id, box, LENGTH_ITEM, _filter->getLength() ) ;

    getDialog()->refresh() ;
}

std::string LowFrequenciesPhaseFilterView::getTitle() const
{
    return " Low frequencies phase filter " ;
}

DIALOG::BOX_PACKING LowFrequenciesPhaseFilterView::getBoxPacking()
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING packing = {
        HORIZONTAL_PACKING, pack_line
    };

    return packing ;
}

void LowFrequenciesPhaseFilterView::alpha_change_proc(int dialog, int box, int item)
{
    LowFrequenciesPhaseFilterView * view = (LowFrequenciesPhaseFilterView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    LowFrequenciesPhaseFilter * filter = (LowFrequenciesPhaseFilter *) view->filter ;
    filter->setAlpha( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void LowFrequenciesPhaseFilterView::betta_change_proc(int dialog, int box, int item)
{
    LowFrequenciesPhaseFilterView * view = (LowFrequenciesPhaseFilterView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    LowFrequenciesPhaseFilter * filter = (LowFrequenciesPhaseFilter *) view->filter ;
    filter->setBetta( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void LowFrequenciesPhaseFilterView::length_change_proc(int dialog, int box, int item, double value)
{
    LowFrequenciesPhaseFilterView * view = (LowFrequenciesPhaseFilterView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    LowFrequenciesPhaseFilter * filter = (LowFrequenciesPhaseFilter *) view->filter ;
    filter->setLength( value ) ;
    view->update() ;
}

//
//
// SweepView
//
//

SweepView::SweepView()
{
    static TEXT_BOX first_freq[] = {
        { "Start frequency, Hz","1", true, 0, 10, nullptr, first_freq_change_proc, ufloatTextVerificator, first_freq_change_proc },
        { 0 }
    };

    static TEXT_BOX last_freq[] = {
        { "Final frequency, Hz", "1", true, 0, 10, nullptr, last_freq_change_proc, ufloatTextVerificator, last_freq_change_proc },
        { 0 }
    };
    
    static TEXT_BOX first_taper[] = {
        { "Start taper, msec", "1", true, 0, 10, nullptr, first_taper_change_proc, ufloatTextVerificator, first_taper_change_proc },
        { 0 }
    };

    static TEXT_BOX last_taper[] = {
        { "Final taper, msec", "1", true, 0, 10, nullptr, last_taper_change_proc, ufloatTextVerificator, last_taper_change_proc },
        { 0 }
    };

    static TEXT_BOX length_box[] = {
        { "Sweep length, msec", "1", true, 0, 10, nullptr, length_change_proc, ufloatTextVerificator, length_change_proc },
        { 0 }
    };
    static TEXT_BOX regularization_box[] = {
        { "Regularization, %", "1", true, 0, 10, nullptr, reg_change_proc, ufloatTextVerificator, reg_change_proc },
        { 0 }
    };
    static TEXT_BOX window_len_box[] = {
        { "Wavelet length, ms", "1", true, 0, 10, nullptr, winlen_change_proc, ufloatTextVerificator, winlen_change_proc },
        { 0 }
    };
    
    static const char * graphModes[] = {"Sweep signal", "Klauder wavelet", "Inverse Klauder wavelet", "Minphase Klauder wavelet ", "Inverse minphase Klauder wavelet ", "Klauder to Minphase Klauder", "Minphase Klauder to Klauder", 0};
    
    static OPTIONS_BOX mode_box[] = { 
        {"Mode", graphModes, waveletmode_select_proc},
        { 0 }
    };
    static const char * windowModes[] = {"Rectangle", "Trapezoidal", "Hemming", 0};
    
    static OPTIONS_BOX winmode_box[] = {
        {"Window function", windowModes, winmode_select_proc},
        { 0 }
    };
    

    static MULTI_BOX box_prototype[] = {
        { first_freq,  TEXT_BOX_D  },
        { last_freq,   TEXT_BOX_D  },
        { first_taper, TEXT_BOX_D  },
        { last_taper,  TEXT_BOX_D  },
        { length_box,  TEXT_BOX_D  },
        { mode_box,    OPTIONS_BOX_D},
        { regularization_box,  TEXT_BOX_D  },
        { winmode_box, OPTIONS_BOX_D},
        { window_len_box,  TEXT_BOX_D  },
        { 0 }
    };

    parameters_box = new MULTI_BOX[VIEW_ITEMS_NUM + 1];

    for (int i = 0; i <= VIEW_ITEMS_NUM; i++){
        parameters_box[i] = box_prototype[i] ;
        }
}       

bool SweepView::connect(BaseFilter * _filter)
{
    if ( !_filter || _filter->getType() != BaseFilter::SweepFilterType )
    {
        filter = NULL ;
        return false ;
    }

    filter = _filter ;
    return true ;
}

void SweepView::update()
{
    
    if ( !getDialog() || !filter )
        return ;

    const int id = getDialog()->getID() ;
    SweepFilter * _filter = (SweepFilter *) filter ;
    DialogSetTextValueAsDouble( id, box, FIRST_FREQ_ITEM, _filter->getFreqBeg(), 0 ) ;
    DialogSetTextValueAsDouble( id, box, LAST_FREQ_ITEM, _filter->getFreqEnd(), 0 ) ;
    DialogSetTextValueAsDouble( id, box, FIRST_TAPER_ITEM, _filter->getTaperBeg(), 0 ) ;
    DialogSetTextValueAsDouble( id, box, LAST_TAPER_ITEM, _filter->getTaperEnd(), 0 ) ;
    DialogSetTextValueAsDouble( id, box, REG_ITEM, (_filter->getReg())*100, 2) ;
    DialogSetTextValueAsDouble( id, box, WINLEN_ITEM, (_filter->getWaveletLength()), 0) ; 
    DialogSetTextValueAsDouble( id, box, LENGTH_ITEM, _filter->getSweepLength(), 0 ) ;
    DialogSetOptionValue( id, box, WINMODE_ITEM, int(_filter->getWindowType()) ) ;
    DialogSetOptionValue( id, box, MODE_ITEM, int(_filter->getWaveletType()) ) ;
    const bool is_reg = (_filter->getWaveletType() > 1 ) ;
    const bool is_win = (_filter->getWaveletType() > 0);
    DialogSetShowItem(id, box, REG_ITEM, is_reg);
    DialogSetShowItem(id, box, WINLEN_ITEM, is_win);
    DialogSetShowItem(id, box, WINMODE_ITEM, is_win);
    getDialog()->refresh() ;
}

std::string SweepView::getTitle() const
{
    return " Sweep filter " ;
}

DIALOG::BOX_PACKING SweepView::getBoxPacking()
{
    static DIALOG::BOX_PACKING::LINE pack_line[] = {
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 1, FIXED_SIZE_BOX },
        { 0 }
    };

    DIALOG::BOX_PACKING packing = {
        HORIZONTAL_PACKING, pack_line
    };

    return packing ;
}

void SweepView::length_change_proc(int dialog, int box, int item)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;
    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setSweepLength(DialogGetTextValueAsDouble(dialog, box, item)) ;
    view->update() ;
}

void SweepView::first_freq_change_proc(int dialog, int box, int item)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;
    
    
    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setFreqBeg( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void SweepView::last_freq_change_proc(int dialog, int box, int item)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setFreqEnd( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void SweepView::first_taper_change_proc(int dialog, int box, int item)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setTaperBeg( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}

void SweepView::last_taper_change_proc(int dialog, int box, int item)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setTaperEnd( DialogGetTextValueAsDouble(dialog, box, item) ) ;
    view->update() ;
}
void SweepView::waveletmode_select_proc(int dialog, int box, int menu, int button)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setWaveletType(SweepFilter::WaveletType(button));
        
    view->update() ;
}

void SweepView::reg_change_proc(int dialog, int box, int item)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setReg(DialogGetTextValueAsDouble(dialog, box, item)/100);
    
    view->update() ;
}
void SweepView::winlen_change_proc(int dialog, int box, int item)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "");
    
    if (!view || !view->getDialog() || !view->filter)
        return;
    SweepFilter * filter = (SweepFilter *) view->filter;
    filter->setWaveletLength(DialogGetTextValueAsDouble(dialog, box, item));

    view->update();
    
}
void SweepView::winmode_select_proc(int dialog, int box, int menu, int button)
{
    SweepView * view = (SweepView *) GetDialogBoxData(dialog, box, "") ;

    if ( !view || !view->getDialog() || !view->filter )
        return ;

    SweepFilter * filter = (SweepFilter *) view->filter ;
    filter->setWindowType(SweepFilter::WindowType(button));
        
    view->update() ;
}

}
}
}
