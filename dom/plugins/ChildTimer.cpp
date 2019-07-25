







































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
  , mRepeating(repeat)
  , mID(gNextTimerID++)
{
  mTimer.Start(base::TimeDelta::FromMilliseconds(interval),
               this, &ChildTimer::Run);
}

uint32_t
ChildTimer::gNextTimerID = 1;

void
ChildTimer::Run()
{
  if (!mRepeating)
    mTimer.Stop();
  mFunc(mInstance->GetNPP(), mID);
}

} 
} 
