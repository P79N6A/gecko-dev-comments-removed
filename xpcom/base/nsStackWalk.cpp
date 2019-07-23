






































#include "nsStackWalk.h"

#if defined(_WIN32) && defined(_M_IX86) && !defined(WINCE) 
#include "nsStackFrameWin.cpp"



#elif (defined(linux) && defined(__GNUC__) && (defined(__i386) || defined(PPC) || defined(__x86_64__))) || (defined(__sun) && (defined(__sparc) || defined(sparc) || defined(__i386) || defined(i386)))
#include "nsStackFrameUnix.cpp"

#else 

EXPORT_XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

#endif
