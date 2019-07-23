







































#ifndef mozilla_plugins_ChildAsyncCall_h
#define mozilla_plugins_ChildAsyncCall_h

#include "PluginMessageUtils.h"
#include "base/task.h"

namespace mozilla {
namespace plugins {

typedef void (*PluginThreadCallback)(void*);

class PluginInstanceChild;

class ChildAsyncCall : public CancelableTask
{
public:
  ChildAsyncCall(PluginInstanceChild* instance,
                 PluginThreadCallback aFunc, void* aUserData);

  NS_OVERRIDE void Run();
  NS_OVERRIDE void Cancel();
  
protected:
  PluginInstanceChild* mInstance;
  PluginThreadCallback mFunc;
  void* mData;

  void RemoveFromAsyncList();
};

} 
} 

#endif 
