




#ifndef mozilla_BackgroundHangMonitor_h
#define mozilla_BackgroundHangMonitor_h

#include "mozilla/RefPtr.h"

#include <stdint.h>

namespace mozilla {

class BackgroundHangThread;


















































































class BackgroundHangMonitor
{
private:
  RefPtr<BackgroundHangThread> mThread;

public:
  



  static void Startup();

  



  static void Shutdown();

  








  BackgroundHangMonitor(const char* aName,
                        uint32_t aTimeoutMs,
                        uint32_t aMaxTimeoutMs);

  



  BackgroundHangMonitor();

  



  ~BackgroundHangMonitor();

  




  void NotifyActivity();

  




  void NotifyWait();
};

} 

#endif 
