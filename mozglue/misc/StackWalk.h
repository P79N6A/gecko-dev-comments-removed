







#ifndef mozilla_StackWalk_h
#define mozilla_StackWalk_h



#include "mozilla/Types.h"
#include <stdint.h>












typedef void
(*MozWalkStackCallback)(uint32_t aFrameNumber, void* aPC, void* aSP,
                        void* aClosure);



























MFBT_API bool
MozStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
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
} MozCodeAddressDetails;








MFBT_API bool
MozDescribeCodeAddress(void* aPC, MozCodeAddressDetails* aDetails);



























MFBT_API void
MozFormatCodeAddress(char* aBuffer, uint32_t aBufferSize, uint32_t aFrameNumber,
                     const void* aPC, const char* aFunction,
                     const char* aLibrary, ptrdiff_t aLOffset,
                     const char* aFileName, uint32_t aLineNo);
















MFBT_API void
MozFormatCodeAddressDetails(char* aBuffer, uint32_t aBufferSize,
                            uint32_t aFrameNumber, void* aPC,
                            const MozCodeAddressDetails* aDetails);

namespace mozilla {

MFBT_API bool
FramePointerStackWalk(MozWalkStackCallback aCallback, uint32_t aSkipFrames,
                      uint32_t aMaxFrames, void* aClosure, void** aBp,
                      void* aStackEnd);

} 





MFBT_API void
StackWalkInitCriticalAddress(void);

#endif
