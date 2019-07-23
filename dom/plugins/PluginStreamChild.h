




































#ifndef mozilla_plugins_PluginStreamChild_h
#define mozilla_plugins_PluginStreamChild_h

#include "mozilla/plugins/PPluginStreamChild.h"
#include "mozilla/plugins/AStream.h"

namespace mozilla {
namespace plugins {

class PluginInstanceChild;

class PluginStreamChild : public PPluginStreamChild, public AStream
{
  friend class PluginInstanceChild;

public:
  PluginStreamChild(PluginInstanceChild* instance);
  virtual ~PluginStreamChild() { }

  NS_OVERRIDE virtual bool IsBrowserStream() { return false; }

  int32_t NPN_Write(int32_t length, void* buffer);
  void NPP_DestroyStream(NPError reason);

  void EnsureCorrectInstance(PluginInstanceChild* i)
  {
    if (i != mInstance)
      NS_RUNTIMEABORT("Incorrect stream instance");
  }
  void EnsureCorrectStream(NPStream* s)
  {
    if (s != &mStream)
      NS_RUNTIMEABORT("Incorrect stream data");
  }

private:
  PluginInstanceChild* mInstance;
  NPStream mStream;
  bool mClosed;
};


} 
} 

#endif
