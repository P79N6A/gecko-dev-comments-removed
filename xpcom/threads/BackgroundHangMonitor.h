





#ifndef mozilla_BackgroundHangMonitor_h
#define mozilla_BackgroundHangMonitor_h

#include "mozilla/RefPtr.h"
#include "mozilla/Monitor.h"

#include "nsString.h"

#include <stdint.h>

namespace mozilla {

namespace Telemetry {
class ThreadHangStats;
};

class BackgroundHangThread;
class BackgroundHangManager;




















































































class BackgroundHangMonitor
{
private:
  friend BackgroundHangManager;

  RefPtr<BackgroundHangThread> mThread;

  static bool ShouldDisableOnBeta(const nsCString &);
  static bool DisableOnBeta();

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

  


  static bool IsDisabled();

  








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
