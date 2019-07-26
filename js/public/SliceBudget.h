





#ifndef js_SliceBudget_h
#define js_SliceBudget_h

#include <stdint.h>

namespace js {







struct JS_PUBLIC_API(SliceBudget)
{
    int64_t deadline; 
    intptr_t counter;

    static const intptr_t CounterReset = 1000;

    static const int64_t Unlimited = 0;
    static int64_t TimeBudget(int64_t millis);
    static int64_t WorkBudget(int64_t work);

    
    SliceBudget();

    
    SliceBudget(int64_t budget);

    void reset() {
        deadline = INT64_MAX;
        counter = INTPTR_MAX;
    }

    void step(intptr_t amt = 1) {
        counter -= amt;
    }

    bool checkOverBudget();

    bool isOverBudget() {
        if (counter >= 0)
            return false;
        return checkOverBudget();
    }
};

} 

#endif 
