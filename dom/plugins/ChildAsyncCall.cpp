







































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

NS_IMETHODIMP
ChildAsyncCall::Run()
{
  if (mFunc) {
    mInstance->mPendingAsyncCalls.RemoveElement(this);
    mFunc(mData);
  }
  return NS_OK;
}

} 
} 
