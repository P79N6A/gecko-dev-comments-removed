







































#include "ChildAsyncCall.h"
#include "PluginInstanceChild.h"

namespace mozilla {
namespace plugins {

ChildAsyncCall::ChildAsyncCall(PluginInstanceChild* instance,
                               PluginThreadCallback aFunc, void* aUserData)
  : mInstance(instance)
  , mFunc(aFunc)
  , mData(aUserData)
{
}

void
ChildAsyncCall::Cancel()
{
  mInstance = NULL;
  mFunc = NULL;
  mData = NULL;
}

void
ChildAsyncCall::Run()
{
  if (mFunc) {
    MutexAutoLock lock(mInstance->mAsyncCallMutex);
    mInstance->mPendingAsyncCalls.RemoveElement(this);
  }
  if (mFunc)
    mFunc(mData);
}

} 
} 
