













#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "itrbbi.h"
#include "rbbiapts.h"
#include "rbbitst.h"

#define TESTCLASS(n,classname)        \
    case n:                           \
        name = #classname;            \
        if (exec) {                   \
            logln(#classname "---");  \
            logln("");                \
            classname t;              \
            callTest(t, par);         \
        }                             \
        break


void IntlTestRBBI::runIndexedTest( int32_t index, UBool exec, const char* &name, char* par )
{
    if (exec) logln("TestSuite RuleBasedBreakIterator: ");
    switch (index) {
        TESTCLASS(0, RBBIAPITest);
        TESTCLASS(1, RBBITest);
        default: name=""; break;
    }
}

#endif 
