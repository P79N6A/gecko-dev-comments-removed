







































#ifndef mozilla_plugins_ChildAsyncCall_h
#define mozilla_plugins_ChildAsyncCall_h

#include "PluginMessageUtils.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace plugins {

typedef void (*PluginThreadCallback)(void*);

class PluginInstanceChild;

class ChildAsyncCall : public nsRunnable
{
public:
  ChildAsyncCall(PluginInstanceChild* instance,
                 PluginThreadCallback aFunc, void* aUserData);

  void Cancel();
  NS_IMETHOD Run();

private:
  PluginInstanceChild* mInstance;
  PluginThreadCallback mFunc;
  void* mData;
};

} 
} 

#endif 
