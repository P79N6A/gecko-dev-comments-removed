






#ifndef SkHRESULT_DEFINED
#define SkHRESULT_DEFINED

#include "SkTypes.h"

void SkTraceHR(const char* file, unsigned long line,
               HRESULT hr, const char* msg);

#ifdef SK_DEBUG
#define SK_TRACEHR(_hr, _msg) SkTraceHR(__FILE__, __LINE__, _hr, _msg)
#else
#define SK_TRACEHR(_hr, _msg) _hr
#endif

#define HR_GENERAL(_ex, _msg, _ret) {\
    HRESULT _hr = _ex;\
    if (FAILED(_hr)) {\
        SK_TRACEHR(_hr, _msg);\
        return _ret;\
    }\
}












#define HR(ex) HR_GENERAL(ex, NULL, _hr)
#define HRM(ex, msg) HR_GENERAL(ex, msg, _hr)

#define HRB(ex) HR_GENERAL(ex, NULL, false)
#define HRBM(ex, msg) HR_GENERAL(ex, msg, false)

#define HRV(ex) HR_GENERAL(ex, NULL, )
#define HRVM(ex, msg) HR_GENERAL(ex, msg, )

#endif
