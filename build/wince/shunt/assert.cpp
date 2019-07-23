







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif

MOZCE_SHUNT_API void mozce_assert(int inExpression)
{
#ifdef API_LOGGING
    mozce_printf("mozce_assert called\n");
#endif

    if(0 == inExpression)
    {
        DebugBreak();
    }
}

#if 0
{
#endif
} 

