






































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

EXPORT_XPCOM_API(nsresult)
NS_DescribeCodeAddress(void *aPC, nsCodeAddressDetails *aDetails)
{
    aDetails->library[0] = '\0';
    aDetails->loffset = 0;
    aDetails->filename[0] = '\0';
    aDetails->lineno = 0;
    aDetails->function[0] = '\0';
    aDetails->foffset = 0;
    return NS_ERROR_NOT_IMPLEMENTED;
}

EXPORT_XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, PRUint32 aBufferSize)
{
    aBuffer[0] = '\0';
    return NS_ERROR_NOT_IMPLEMENTED;
}

#endif
