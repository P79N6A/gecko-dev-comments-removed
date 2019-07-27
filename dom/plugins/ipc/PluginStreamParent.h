




#ifndef mozilla_plugins_PluginStreamParent_h
#define mozilla_plugins_PluginStreamParent_h

#include "mozilla/plugins/PPluginStreamParent.h"
#include "mozilla/plugins/AStream.h"

namespace mozilla {
namespace plugins {

class PluginInstanceParent;

class PluginStreamParent : public PPluginStreamParent, public AStream
{
  friend class PluginModuleParent;
  friend class PluginInstanceParent;

public:
  PluginStreamParent(PluginInstanceParent* npp, const nsCString& mimeType,
                     const nsCString& target, NPError* result);
  virtual ~PluginStreamParent() { }

  virtual bool IsBrowserStream() override { return false; }

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool AnswerNPN_Write(const Buffer& data, int32_t* written) override;

  virtual bool Answer__delete__(const NPError& reason, const bool& artificial) override;

private:
  void NPN_DestroyStream(NPReason reason);

  PluginInstanceParent* mInstance;
  NPStream* mStream;
  bool mClosed;
};

} 
} 

#endif
