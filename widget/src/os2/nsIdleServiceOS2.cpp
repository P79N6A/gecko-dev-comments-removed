





































#include "nsIdleServiceOS2.h"


static int (*_System DSSaver_GetInactivityTime)(ULONG *, ULONG *);
#define SSCORE_NOERROR 0 // as in the DSSaver header files

NS_IMPL_ISUPPORTS2(nsIdleServiceOS2, nsIIdleService, nsIdleService)

nsIdleServiceOS2::nsIdleServiceOS2()
  : mHMod(NULLHANDLE), mInitialized(PR_FALSE)
{
  const char error[256] = "";
  if (DosLoadModule(error, 256, "SSCORE", &mHMod) == NO_ERROR) {
    if (DosQueryProcAddr(mHMod, 0, "SSCore_GetInactivityTime",
                         (PFN*)&DSSaver_GetInactivityTime) == NO_ERROR) {
      mInitialized = PR_TRUE;
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
nsIdleServiceOS2::PollIdleTime(PRUint32 *aIdleTime)
{
  if (!mInitialized)
    return false;

  ULONG mouse, keyboard;
  if (DSSaver_GetInactivityTime(&mouse, &keyboard) != SSCORE_NOERROR) {
    return false;
  }

  
  
  *aIdleTime = NS_MIN(mouse, keyboard);
  return true;
}

bool
nsIdleServiceOS2::UsePollMode()
{
  return mInitialized;
}

