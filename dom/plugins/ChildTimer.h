







































#ifndef mozilla_plugins_ChildTimer_h
#define mozilla_plugins_ChildTimer_h

#include "PluginMessageUtils.h"
#include "npapi.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;
typedef void (*TimerFunc)(NPP npp, uint32_t timerID);

class ChildTimer
{
public:
  


  ChildTimer(PluginInstanceChild* instance,
             uint32_t interval,
             bool repeat,
             TimerFunc func);
  ~ChildTimer() {
    NS_ASSERTION(!mTimer, "Timer should already be cancelled");
  }

  uint32_t ID() const { return mID; }

  


  void Destroy();

  class IDComparator
  {
  public:
    PRBool Equals(ChildTimer* t, uint32_t id) const {
      return t->ID() == id;
    }
  };

private:
  PluginInstanceChild* mInstance;
  TimerFunc mFunc;
  uint32_t mID;
  nsCOMPtr<nsITimer> mTimer;

  static uint32_t gNextTimerID;
  static void Callback(nsITimer* aTimer, void* aClosure);
};

} 
} 

#endif 
