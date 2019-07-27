





#ifndef mozilla_BackgroundHangMonitor_h
#define mozilla_BackgroundHangMonitor_h

#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"

#include <stdint.h>

namespace mozilla {

namespace Telemetry {
class ThreadHangStats;
};



#if !defined(RELEASE_BUILD) && !defined(DEBUG)

#define MOZ_ENABLE_BACKGROUND_HANG_MONITOR
#endif

class BackgroundHangThread;




















































































class BackgroundHangMonitor
{
private:
  RefPtr<BackgroundHangThread> mThread;

public:
  static const uint32_t kNoTimeout = 0;

  















  class ThreadHangStatsIterator : public MonitorAutoLock
  {
  private:
    BackgroundHangThread* mThread;

    ThreadHangStatsIterator(const ThreadHangStatsIterator&);
    ThreadHangStatsIterator& operator=(const ThreadHangStatsIterator&);

  public:
    



    ThreadHangStatsIterator();

    



    Telemetry::ThreadHangStats* GetNext();
  };

  



  static void Startup();

  



  static void Shutdown();

  








  BackgroundHangMonitor(const char* aName,
                        uint32_t aTimeoutMs,
                        uint32_t aMaxTimeoutMs);

  



  BackgroundHangMonitor();

  



  ~BackgroundHangMonitor();

  




  void NotifyActivity();

  




  void NotifyWait();

  









  static void Prohibit();

  






  static void Allow();
};

} 

#endif 
