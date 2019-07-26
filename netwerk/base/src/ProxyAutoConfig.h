





#ifndef ProxyAutoConfig_h__
#define ProxyAutoConfig_h__

#include "nsString.h"
#include "jsapi.h"
#include "prio.h"
#include "nsITimer.h"
#include "nsAutoPtr.h"
#include "mozilla/net/DNS.h"

namespace mozilla { namespace net {

class JSRuntimeWrapper;





class ProxyAutoConfig  {
public:
  ProxyAutoConfig()
    : mJSRuntime(nullptr)
    , mJSNeedsSetup(false)
    , mShutdown(false)
  {
    MOZ_COUNT_CTOR(ProxyAutoConfig);
  }
  ~ProxyAutoConfig();

  nsresult Init(const nsCString &aPACURI,
                const nsCString &aPACScript);
  void     Shutdown();
  void     GC();
  bool     MyIPAddress(JS::Value *vp);
  bool     ResolveAddress(const nsCString &aHostName,
                          NetAddr *aNetAddr, unsigned int aTimeout);

  

































  nsresult GetProxyForURI(const nsCString &aTestURI,
                          const nsCString &aTestHost,
                          nsACString &result);

private:
  const static unsigned int kTimeout = 1000; 

  
  nsresult SetupJS();

  bool SrcAddress(const NetAddr *remoteAddress, nsCString &localAddress);
  bool MyIPAddressTryHost(const nsCString &hostName, unsigned int timeout,
                          JS::Value *vp);

  JSRuntimeWrapper *mJSRuntime;
  bool              mJSNeedsSetup;
  bool              mShutdown;
  nsCString         mPACScript;
  nsCString         mPACURI;
  nsCString         mRunningHost;
  nsCOMPtr<nsITimer> mTimer;
};

}} 

#endif  
