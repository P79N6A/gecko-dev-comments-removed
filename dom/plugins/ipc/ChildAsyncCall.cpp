






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
  mInstance = nullptr;
  mFunc = nullptr;
  mData = nullptr;
}

void
ChildAsyncCall::RemoveFromAsyncList()
{
  if (mInstance) {
    MutexAutoLock lock(mInstance->mAsyncCallMutex);
    mInstance->mPendingAsyncCalls.RemoveElement(this);
  }
}

void
ChildAsyncCall::Run()
{
  RemoveFromAsyncList();

  if (mFunc)
    mFunc(mData);
}

} 
} 
