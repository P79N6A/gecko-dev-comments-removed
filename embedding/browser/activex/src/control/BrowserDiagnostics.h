




































#ifndef BROWSER_DIAGNOSTICS_H
#define BROWSER_DIAGNOSTICS_H

#ifdef _DEBUG
#  include <assert.h>
#  define NG_TRACE NgTrace
#  define NG_TRACE_METHOD(fn) \
    { \
        NG_TRACE(_T("0x%04x %s()\n"), (int) GetCurrentThreadId(), _T(#fn)); \
    }
#  define NG_TRACE_METHOD_ARGS(fn, pattern, args) \
    { \
        NG_TRACE(_T("0x%04x %s(") _T(pattern) _T(")\n"), (int) GetCurrentThreadId(), _T(#fn), args); \
    }
#  define NG_ASSERT(expr) assert(expr)
#  define NG_ASSERT_POINTER(p, type) \
    NG_ASSERT(((p) != NULL) && NgIsValidAddress((p), sizeof(type), FALSE))
#  define NG_ASSERT_NULL_OR_POINTER(p, type) \
    NG_ASSERT(((p) == NULL) || NgIsValidAddress((p), sizeof(type), FALSE))
#else
#  define NG_TRACE 1 ? (void)0 : NgTrace
#  define NG_TRACE_METHOD(fn)
#  define NG_TRACE_METHOD_ARGS(fn, pattern, args)
#  define NG_ASSERT(X)
#  define NG_ASSERT_POINTER(p, type)
#  define NG_ASSERT_NULL_OR_POINTER(p, type)
#endif

#define NG_TRACE_ALWAYS AtlTrace

inline void _cdecl NgTrace(LPCSTR lpszFormat, ...)
{
    va_list args;
    va_start(args, lpszFormat);

    int nBuf;
    char szBuffer[512];

    nBuf = _vsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
    NG_ASSERT(nBuf < sizeof(szBuffer)); 

    OutputDebugStringA(szBuffer);
    va_end(args);
}

inline BOOL NgIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE)
{
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

#endif