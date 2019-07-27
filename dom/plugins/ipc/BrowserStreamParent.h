




#ifndef mozilla_plugins_BrowserStreamParent_h
#define mozilla_plugins_BrowserStreamParent_h

#include "mozilla/plugins/PBrowserStreamParent.h"
#include "mozilla/plugins/AStream.h"
#include "nsNPAPIPluginStreamListener.h"
#include "nsPluginStreamListenerPeer.h"

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

  virtual bool IsBrowserStream() override { return true; }

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  virtual bool RecvAsyncNPP_NewStreamResult(
            const NPError& rv,
            const uint16_t& stype) override;

  virtual bool AnswerNPN_RequestRead(const IPCByteRanges& ranges,
                                     NPError* result) override;

  virtual bool RecvNPN_DestroyStream(const NPReason& reason) override;

  virtual bool RecvStreamDestroyed() override;

  int32_t WriteReady();
  int32_t Write(int32_t offset, int32_t len, void* buffer);
  void StreamAsFile(const char* fname);

  void NPP_DestroyStream(NPReason reason);

  void SetAlive()
  {
    if (mState == INITIALIZING) {
      mState = ALIVE;
    }
  }

private:
  using PBrowserStreamParent::SendNPP_DestroyStream;

  PluginInstanceParent* mNPP;
  NPStream* mStream;
  nsCOMPtr<nsISupports> mStreamPeer;
  nsRefPtr<nsNPAPIPluginStreamListener> mStreamListener;
  NPReason mDeferredDestroyReason;

  enum {
    INITIALIZING,
    DEFERRING_DESTROY,
    ALIVE,
    DYING,
    DELETING
  } mState;
};

} 
} 

#endif
