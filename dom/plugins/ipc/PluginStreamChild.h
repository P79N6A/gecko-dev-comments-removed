




































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
  PluginStreamChild();
  virtual ~PluginStreamChild() { }

  NS_OVERRIDE virtual bool IsBrowserStream() { return false; }

  virtual bool Answer__delete__(const NPReason& reason,
                                const bool& artificial);

  int32_t NPN_Write(int32_t length, void* buffer);
  void NPP_DestroyStream(NPError reason);

  void EnsureCorrectInstance(PluginInstanceChild* i)
  {
    if (i != Instance())
      NS_RUNTIMEABORT("Incorrect stream instance");
  }
  void EnsureCorrectStream(NPStream* s)
  {
    if (s != &mStream)
      NS_RUNTIMEABORT("Incorrect stream data");
  }

private:
  PluginInstanceChild* Instance();

  NPStream mStream;
  bool mClosed;
};


} 
} 

#endif
