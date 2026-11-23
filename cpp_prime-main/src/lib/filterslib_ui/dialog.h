/* 
 * File:   FilterslibBaseDialog.h
 * Author: rodin
 *
 * Created on February 8, 2017, 1:34 PM
 */

#ifndef PRIME_LIB_FILTERSLIB_FILTERSLIBDIALOG_H
#define PRIME_LIB_FILTERSLIB_FILTERSLIBDIALOG_H

#include <vector>
#include <string>

#include "toolkit/Menu.h"
#include "progpar/progpar.h"
#include "files/Commondir.h"
#include "geom/broken_line.h"
#include "charts/curve.h"
#include "charts/plotter.h"

#include "filtersviews.h"


namespace Prime {
namespace lib {
namespace filterslib {

/**
 * @brief Class of filters construction dialog.
 */
class FilterslibDialog
{
    /**
     * @name Internal constants
     * @{
     */
    static const std::string DEF_DIALOG_NAME ;         //!< Name of dialog
    static const std::string DEF_FILES_FILTER_NAME ;   //!< File filter name (for usage in open/save dialogs)
    static const std::string DEF_FILES_FILTER_EXT ;    //!< File filter extension (for usage in open/save dialogs)
    static const std::string DEF_TXT_FILES_NAME ;      //!< Text files filter name (for usage in export dialog)
    static const std::string DEF_TXT_FILES_EXT ;       //!< Text files filter extension (for usage in export dialog)
    static const int         DEF_AXIS_LINE_WIDTH ;     //!< Axis lines width
    static const int         DEF_CURVE_LINE_WIDTH ;    //!< Curve widht of all drawing curves
    static const int         DEF_POINT_DRAW_SIZE ;     //!< Drawing size of points
    //!}

    /**
     * @name Internal types
     * @{
     */
    typedef void (*CallbackType)( const FilterslibDialog* dialog, void* data ) ; //!< Type of user callcback
    typedef Prime::lib::charts::Curve Curve ; //!< Type of curve for charts plotting
    typedef Prime::lib::charts::Plotter Plotter ; //!< Type of charts plotter

    /**
     * @brief Dialog menus
     */
    enum DialogMenuDescriptor
    {
        MENU_ROOT = 0,
        MENU_FILTER,
        MENU_FILTER_NEW,
        MENU_WINDOWS
    } ;

    /**
     * @brief Dialog packing descriptor
     */
    enum DialogDescriptor
    {
        TIME_WINDOW_BOX,
        FREQ_WINDOW_BOX,
        PHASE_WINDOW_BOX,
        SAMPLING_STEP_BOX,
        
        // SAMPLING_STEP_BOX
        SAMPLING_STEP_ITEM = 0
    } ;

    /**
     * @brief Dialog buttons
     */
    enum DialogButtons
    {
        BUTTON_OK,
        BUTTON_APPLY,
        BUTTON_CANCEL
    } ;

    /**
     * @brief Class of curves builder for graphics
     */
    class GraphBuilder ;
    //! @}

    /**
     * @brief Constructs object of filters lib dialog.
     * @param[in] __dt Discretization step (in sec).
     * @param[in] __dialog_stype Type of dialog (modal or not).
     */
public:
    FilterslibDialog(double __dt,
                     DialogStyle __dialog_stype = MODELESS);

    /**
     * @brief Constructs object of filters lib dialog and loads required filters.
     * @param[in] __filter_name Name of required existing filter.
     * @param[in] __dt Discretization step (in sec).
     * @param[in] __dialog_stype Type of dialog (modal or not).
     */
public:
    FilterslibDialog(const std::string &__filter_name,
                     double __dt,
                     DialogStyle __dialog_stype = MODELESS);

public:
    ~FilterslibDialog();

    int getID() const ;
    void open() ;
    bool openWithImmediateResponse() ;
    void close() ;
    bool isOpen() const ;
    void refresh();
    void setCallback( CallbackType __callback, void * __data );

    void setDt(double __dt) ;
    double getDt() const ;
    
    const std::string& getFilterName() const ;
    std::string getFilter() const ;
    
    bool loadFilter( const std::string& __filter ) ;
    bool loadFilterFromDB( const std::string& __filtername ) ;
    bool loadFilterFromFile( const std::string& __filename ) ;

private:
    void create( DialogStyle __dialog_style ) ;
    void clear() ;
    void callCallback() ;

    void blockRefresh() ;
    void unblockRefresh() ;
    bool isRefreshBlocked() const ;

    void makeBackup() ;
    void restoreFromBackup() ;

    bool newFilter( BaseFilter::Type __type ) ;
    
    bool loadFilter() ;
    bool loadFilterFromFile() ;
    
    bool saveFilterToDB( const std::string& __filter_name ) ;
    bool saveFilterToFile( const std::string& __file_name ) ;

    bool saveFilterToDB( bool __allow_save_as ) ;
    bool saveAsFilterToDB() ;
    bool saveAsFilterToFile() ;
    bool saveAsFilterOperatorToFile() ;

    void updateDialog() ;
    void updateDialogTitle() ;
    void updateViews() ;

private:
    static int dialog_init( int __dialog );
    static void dialog_buttons_proc( int __dialog, int __button );
    static bool immediate_response_dialog_buttons_proc(int __dialog, int __button);
    static void window_expose(int __window, int __width, int __height) ;
    static void on_sampling_step_changed( int __dialog, int __box, int __item ) ;
    static void menu_new_filter( const MenuItemDetails & details );
    static void menu_open_filter( const MenuItemDetails & details );
    static void menu_save_filter( const MenuItemDetails & details );
    static void menu_save_as_filter( const MenuItemDetails & details );
    static void menu_import_filter( const MenuItemDetails & details );
    static void menu_export_filter( const MenuItemDetails & details );
    static void menu_export_filter_operator( const MenuItemDetails & details );
    static void menu_dialog_close( const MenuItemDetails & details );
    static void show_hide_window(const MenuItemDetails & details, int box );
    static void show_hide_window_map(const MenuItemDetails & details, int box );
    static void show_hide_time_window(const MenuItemDetails & details);
    static void show_hide_time_window_map(const MenuItemDetails & details);
    static void show_hide_freq_window(const MenuItemDetails & details);
    static void show_hide_freq_window_map(const MenuItemDetails & details);
    static void show_hide_phase_window(const MenuItemDetails & details);
    static void show_hide_phase_window_map(const MenuItemDetails & details);

private:
    double dt ;
    
    BaseFilter * filter ;
    std::string filter_name ;
    libconfig::Config filter_backup ;
    
    bool block_refresh ;
    CallbackType callback ;
    void * callback_data ;
    
    int dialog_id ;
    int time_window_id ;
    int spectrum_window_id ;
    int phase_spectrum_window_id ;
    
    std::vector<BaseView *> views ;
} ;

class FilterslibDialog::GraphBuilder
{
    static const double DEF_GRAPH_MARGIN_EXP ; //!< Factor for enlarging graphic limits (for margins)
    static const double DEF_GRAPH_HORIZONTAL ; //!< Criterion for supposing graph horizontal

public:

    enum WindowType
    {
        Time,
        AmplitudeSpectrum,
        PhaseSpectrum
    } ;

public:

    struct Limits
    {
        float x_min;
        float x_max;
        float y_min;
        float y_max;
    } ;

public:

    GraphBuilder() : type(Time), filter(NULL) { }

    GraphBuilder( WindowType _type, const BaseFilter* _filter ) : type(_type), filter(_filter) { }

    std::string getHLabel() const ;
    Limits getLimits( int length ) const ;
    std::vector<float> getCurve( int length ) const ;
    Prime::Geom::broken_line_2_dbl getPoints() const ;

private:
    Limits getTimeLimits( int length ) const ;
    Limits getFreqLimits( int length ) const ;
    void getTimeCurve( int length, std::vector<float>& curve ) const ;
    void getFreqCurve( int length, std::vector<float>& curve ) const ;
    void getPhaseCurve( int length, std::vector<float>& curve ) const ;

    void calcVerticalLimits(const std::vector<float>& curve, float& ymin, float& ymax) const ;
    void getInterpolatedCurve(const std::vector<float>& samples,
                              std::vector<float>& curve,
                              int length ) const ;

private:
    WindowType type ;
    const BaseFilter* filter ;
    mutable std::vector<float> cache ;
} ;

}
}
}

#endif /* PRIME_LIB_FILTERSLIB_FILTERSLIBDIALOG_H */
