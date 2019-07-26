





#include "nsIdleServiceOS2.h"
#include <algorithm>


static int (*_System DSSaver_GetInactivityTime)(ULONG *, ULONG *);
#define SSCORE_NOERROR 0 // as in the DSSaver header files

NS_IMPL_ISUPPORTS_INHERITED0(nsIdleServiceOS2, nsIdleService)

nsIdleServiceOS2::nsIdleServiceOS2()
  : mHMod(NULLHANDLE), mInitialized(false)
{
  const char error[256] = "";
  if (DosLoadModule(error, 256, "SSCORE", &mHMod) == NO_ERROR) {
    if (DosQueryProcAddr(mHMod, 0, "SSCore_GetInactivityTime",
                         (PFN*)&DSSaver_GetInactivityTime) == NO_ERROR) {
      mInitialized = true;
    }
  }
}

nsIdleServiceOS2::~nsIdleServiceOS2()
{
  if (mHMod != NULLHANDLE) {
    DosFreeModule(mHMod);
  }
}

bool
nsIdleServiceOS2::PollIdleTime(uint32_t *aIdleTime)
{
  if (!mInitialized)
    return false;

  ULONG mouse, keyboard;
  if (DSSaver_GetInactivityTime(&mouse, &keyboard) != SSCORE_NOERROR) {
    return false;
  }

  
  
  *aIdleTime = std::min(mouse, keyboard);
  return true;
}

bool
nsIdleServiceOS2::UsePollMode()
{
  return mInitialized;
}

