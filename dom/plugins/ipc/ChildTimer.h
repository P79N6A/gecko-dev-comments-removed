







































#ifndef mozilla_plugins_ChildTimer_h
#define mozilla_plugins_ChildTimer_h

#include "PluginMessageUtils.h"
#include "npapi.h"
#include "base/timer.h"

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
  ~ChildTimer() { }

  uint32_t ID() const { return mID; }

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
  bool mRepeating;
  uint32_t mID;
  base::RepeatingTimer<ChildTimer> mTimer;

  void Run();

  static uint32_t gNextTimerID;
};

} 
} 

#endif 
