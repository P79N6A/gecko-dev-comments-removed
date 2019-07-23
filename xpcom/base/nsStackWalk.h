






































#ifndef nsStackWalk_h_
#define nsStackWalk_h_



#include "nscore.h"

PR_BEGIN_EXTERN_C

typedef void
(* PR_CALLBACK NS_WalkStackCallback)(char *aFrame, void *aClosure);

















XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, PRUint32 aSkipFrames,
             void *aClosure);

PR_END_EXTERN_C

#endif 
