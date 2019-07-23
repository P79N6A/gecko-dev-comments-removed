





































#ifndef txPatternOptimizer_h__
#define txPatternOptimizer_h__

#include "txXPathOptimizer.h"

class txPattern;

class txPatternOptimizer
{
public:
    





    nsresult optimize(txPattern* aInPattern, txPattern** aOutPattern);

private:

    
    nsresult optimizeStep(txPattern* aInPattern, txPattern** aOutPattern);

    txXPathOptimizer mXPathOptimizer;
};

#endif 
