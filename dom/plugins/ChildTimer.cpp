







































#include "ChildTimer.h"
#include "PluginInstanceChild.h"
#include "nsComponentManagerUtils.h"

namespace mozilla {
namespace plugins {

ChildTimer::ChildTimer(PluginInstanceChild* instance,
                       uint32_t interval,
                       bool repeat,
                       TimerFunc func)
  : mInstance(instance)
  , mFunc(func)
  , mID(0)
{
  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  if (!mTimer)
    return;

  nsresult rv = mTimer->InitWithFuncCallback(Callback, this, interval,
                                             repeat ? nsITimer::TYPE_REPEATING_SLACK : nsITimer::TYPE_ONE_SHOT);
  if (NS_FAILED(rv)) {
    mTimer = NULL;
    return;
  }

  mID = gNextTimerID++;
}

void
ChildTimer::Destroy()
{
  mTimer->Cancel();
  mTimer = NULL;
  delete this;
}

uint32_t
ChildTimer::gNextTimerID = 1;

void
ChildTimer::Callback(nsITimer* aTimer, void* aClosure)
{
  ChildTimer* self = static_cast<ChildTimer*>(aClosure);
  self->mFunc(self->mInstance->GetNPP(), self->mID);
}

} 
} 
