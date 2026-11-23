/* 
 * File:   StackGatherWindow.h
 * Author: ranisimov
 *
 * Created on July 24, 2023, 2:38 AM
 */

#ifndef VERTICAL_SPECTRA_ANALYSIS_STACK_GATHER_WINDOW_H
#define VERTICAL_SPECTRA_ANALYSIS_STACK_GATHER_WINDOW_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "toolkit/Appl.h"

#include "SpectrumWindow.h"


namespace Prime {
namespace lib {
namespace VerticalSpectraAnalysis {

/**
 * @brief Class of spectrum window.
 */
class StackGatherWindow : public SpectrumWindow
{
    /**
     * @brief Constructs window object.
     * @param[in] __window Window identifier which will be used for visualization.
     */
public:
    StackGatherWindow( int __window ) :
    SpectrumWindow( __window, SpectraDataType::StackGather ) {}
    
public:
    virtual ~StackGatherWindow() {}
    
    /**
     * @brief Creates window.
     */
public:
    static WINDOW_BOX * make() ;
};

}
}
}

#endif /* VERTICAL_SPECTRA_ANALYSIS_STACK_GATHER_WINDOW_H */

