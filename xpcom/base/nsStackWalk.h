






































#ifndef nsStackWalk_h_
#define nsStackWalk_h_



#include "nscore.h"
#include <mozilla/StandardInteger.h>

PR_BEGIN_EXTERN_C

typedef void
(* NS_WalkStackCallback)(void *aPC, void *aClosure);
























XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure, uintptr_t aThread);

typedef struct {
    




    char library[256];
    PRUptrdiff loffset;
    




    char filename[256];
    unsigned long lineno;
    



    char function[256];
    PRUptrdiff foffset;
} nsCodeAddressDetails;








XPCOM_API(nsresult)
NS_DescribeCodeAddress(void *aPC, nsCodeAddressDetails *aDetails);


















XPCOM_API(nsresult)
NS_FormatCodeAddressDetails(void *aPC, const nsCodeAddressDetails *aDetails,
                            char *aBuffer, PRUint32 aBufferSize);

PR_END_EXTERN_C

#endif 
