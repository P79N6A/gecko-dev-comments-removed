




#ifndef InjectCrashReporter_h
#define InjectCrashReporter_h

#include "nsThreadUtils.h"
#include <windows.h>

namespace mozilla {

class InjectCrashRunnable : public nsRunnable
{
public:
  InjectCrashRunnable(DWORD pid);

  NS_IMETHOD Run();

private:
  DWORD mPID;
  nsString mInjectorPath;
};
  
} 

#endif
