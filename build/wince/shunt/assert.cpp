







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif

MOZCE_SHUNT_API void mozce_assert(int inExpression)
{
    WINCE_LOG_API_CALL("mozce_assert called\n");

    if(0 == inExpression)
    {
        DebugBreak();
    }
}

#if 0
{
#endif
} 

