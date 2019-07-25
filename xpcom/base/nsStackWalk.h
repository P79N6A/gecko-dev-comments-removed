






#ifndef nsStackWalk_h_
#define nsStackWalk_h_



#include "nscore.h"
#include <mozilla/StandardInteger.h>
#include "prtypes.h"

PR_BEGIN_EXTERN_C




typedef void
(* NS_WalkStackCallback)(void *aPC, void *aSP, void *aClosure);
























XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
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
                            char *aBuffer, uint32_t aBufferSize);

PR_END_EXTERN_C

#endif 
