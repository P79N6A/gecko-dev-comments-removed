





#ifndef js_SliceBudget_h
#define js_SliceBudget_h

#include <stdint.h>

namespace js {

struct JS_PUBLIC_API(TimeBudget)
{
    int64_t budget;

    explicit TimeBudget(int64_t milliseconds) { budget = milliseconds; }
};

struct JS_PUBLIC_API(WorkBudget)
{
    int64_t budget;

    explicit WorkBudget(int64_t work) { budget = work; }
};







struct JS_PUBLIC_API(SliceBudget)
{
    int64_t deadline; 
    intptr_t counter;

    static const intptr_t CounterReset = 1000;

    static const int64_t Unlimited = -1;

    
    SliceBudget();

    
    explicit SliceBudget(TimeBudget time);

    
    explicit SliceBudget(WorkBudget work);

    void makeUnlimited() {
        deadline = unlimitedDeadline;
        counter = unlimitedStartCounter;
    }

    void step(intptr_t amt = 1) {
        counter -= amt;
    }

    bool isOverBudget() {
        if (counter > 0)
            return false;
        return checkOverBudget();
    }

    bool isUnlimited() {
        return deadline == unlimitedDeadline;
    }

  private:
    bool checkOverBudget();

    static const int64_t unlimitedDeadline = INT64_MAX;
    static const intptr_t unlimitedStartCounter = INTPTR_MAX;
};

} 

#endif 
