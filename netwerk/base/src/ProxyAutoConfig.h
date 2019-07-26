





#ifndef ProxyAutoConfig_h__
#define ProxyAutoConfig_h__

#include "nsString.h"

namespace mozilla { namespace net {

class JSRuntimeWrapper;





class ProxyAutoConfig  {
public:
  ProxyAutoConfig()
    : mJSRuntime(nullptr)
    , mRunning(false)
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

  

































  nsresult GetProxyForURI(const nsCString &aTestURI,
                          const nsCString &aTestHost,
                          nsACString &result);

private:
  
  nsresult SetupJS();

  JSRuntimeWrapper *mJSRuntime;
  bool              mRunning;
  bool              mJSNeedsSetup;
  bool              mShutdown;
  nsCString         mPACScript;
  nsCString         mPACURI;
};

}} 

#endif  
