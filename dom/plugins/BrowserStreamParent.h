




































#ifndef mozilla_plugins_BrowserStreamParent_h
#define mozilla_plugins_BrowserStreamParent_h

#include "mozilla/plugins/PBrowserStreamParent.h"
#include "mozilla/plugins/AStream.h"

namespace mozilla {
namespace plugins {

class PluginInstanceParent;

class BrowserStreamParent : public PBrowserStreamParent, public AStream
{
  friend class PluginModuleParent;
  friend class PluginInstanceParent;

public:
  BrowserStreamParent(PluginInstanceParent* npp,
                      NPStream* stream);
  virtual ~BrowserStreamParent();

  NS_OVERRIDE virtual bool IsBrowserStream() { return true; }

  virtual bool AnswerNPN_RequestRead(const IPCByteRanges& ranges,
                                     NPError* result);

  int32_t WriteReady();
  int32_t Write(int32_t offset, int32_t len, void* buffer);
  void StreamAsFile(const char* fname);
  NPError NPN_DestroyStream(NPError reason);

private:
  PluginInstanceParent* mNPP;
  NPStream* mStream;
};

} 
} 

#endif
