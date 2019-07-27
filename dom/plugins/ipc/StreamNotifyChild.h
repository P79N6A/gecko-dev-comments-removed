





#ifndef mozilla_plugins_StreamNotifyChild_h
#define mozilla_plugins_StreamNotifyChild_h

#include "mozilla/plugins/PStreamNotifyChild.h"

namespace mozilla {
namespace plugins {

class BrowserStreamChild;

class StreamNotifyChild : public PStreamNotifyChild
{
  friend class PluginInstanceChild;
  friend class BrowserStreamChild;

public:
  explicit StreamNotifyChild(const nsCString& aURL)
    : mURL(aURL)
    , mClosure(nullptr)
    , mBrowserStream(nullptr)
  { }

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  void SetValid(void* aClosure) {
    mClosure = aClosure;
  }

  void NPP_URLNotify(NPReason reason);

private:
  virtual bool Recv__delete__(const NPReason& reason) MOZ_OVERRIDE;

  bool RecvRedirectNotify(const nsCString& url, const int32_t& status) MOZ_OVERRIDE;

  





  void SetAssociatedStream(BrowserStreamChild* bs);

  nsCString mURL;
  void* mClosure;

  




  BrowserStreamChild* mBrowserStream;
};

} 
} 

#endif
