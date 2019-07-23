





































#ifndef TRANSFRMX_TXXSLTPROCESSOR_H
#define TRANSFRMX_TXXSLTPROCESSOR_H

#include "txExecutionState.h"

class txXSLTProcessor
{
public:
    



    static MBool init();
    static void shutdown();


    static nsresult execute(txExecutionState& aEs);

    
    
};

#endif
