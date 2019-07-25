





































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
  StreamNotifyChild(const nsCString& aURL)
    : mURL(aURL)
    , mClosure(NULL)
    , mBrowserStream(NULL)
  { }

  NS_OVERRIDE virtual void ActorDestroy(ActorDestroyReason why);

  void SetValid(void* aClosure) {
    mClosure = aClosure;
  }

  void NPP_URLNotify(NPReason reason);

private:
  NS_OVERRIDE virtual bool Recv__delete__(const NPReason& reason);

  bool RecvRedirectNotify(const nsCString& url, const int32_t& status);

  





  void SetAssociatedStream(BrowserStreamChild* bs);

  nsCString mURL;
  void* mClosure;

  




  BrowserStreamChild* mBrowserStream;
};

} 
} 

#endif
