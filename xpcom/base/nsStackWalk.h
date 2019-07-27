







#ifndef nsStackWalk_h_
#define nsStackWalk_h_



#include "nscore.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif












typedef void
(*NS_WalkStackCallback)(uint32_t aFrameNumber, void* aPC, void* aSP,
                        void* aClosure);








































XPCOM_API(nsresult)
NS_StackWalk(NS_WalkStackCallback aCallback, uint32_t aSkipFrames,
             uint32_t aMaxFrames, void* aClosure, uintptr_t aThread,
             void* aPlatformData);

typedef struct
{
  




  char library[256];
  ptrdiff_t loffset;
  




  char filename[256];
  unsigned long lineno;
  



  char function[256];
  ptrdiff_t foffset;
} nsCodeAddressDetails;








XPCOM_API(nsresult)
NS_DescribeCodeAddress(void* aPC, nsCodeAddressDetails* aDetails);



























XPCOM_API(void)
NS_FormatCodeAddress(char* aBuffer, uint32_t aBufferSize, uint32_t aFrameNumber,
                     const void* aPC, const char* aFunction,
                     const char* aLibrary, ptrdiff_t aLOffset,
                     const char* aFileName, uint32_t aLineNo);
















XPCOM_API(void)
NS_FormatCodeAddressDetails(char* aBuffer, uint32_t aBufferSize,
                            uint32_t aFrameNumber, void* aPC,
                            const nsCodeAddressDetails* aDetails);

#ifdef __cplusplus
}
#endif

#endif
