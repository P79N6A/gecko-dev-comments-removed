





































#ifndef TRANSFRMX_TXXSLTPROCESSOR_H
#define TRANSFRMX_TXXSLTPROCESSOR_H

#include "txExecutionState.h"

class txXSLTProcessor
{
public:
    



    static bool init();
    static void shutdown();


    static nsresult execute(txExecutionState& aEs);

    
    
};

#endif
