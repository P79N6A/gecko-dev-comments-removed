





#ifndef ProxyAutoConfig_h__
#define ProxyAutoConfig_h__

#include "nsString.h"
#include "nsCOMPtr.h"

class nsITimer;
namespace JS {
class CallArgs;
}

namespace mozilla { namespace net {

class JSRuntimeWrapper;
union NetAddr;





class ProxyAutoConfig  {
public:
  ProxyAutoConfig();
  ~ProxyAutoConfig();

  nsresult Init(const nsCString &aPACURI,
                const nsCString &aPACScript);
  void     SetThreadLocalIndex(uint32_t index);
  void     Shutdown();
  void     GC();
  bool     MyIPAddress(const JS::CallArgs &aArgs);
  bool     ResolveAddress(const nsCString &aHostName,
                          NetAddr *aNetAddr, unsigned int aTimeout);

  

































  nsresult GetProxyForURI(const nsCString &aTestURI,
                          const nsCString &aTestHost,
                          nsACString &result);

private:
  
  const static unsigned int kTimeout = 665;

  
  nsresult SetupJS();

  bool SrcAddress(const NetAddr *remoteAddress, nsCString &localAddress);
  bool MyIPAddressTryHost(const nsCString &hostName, unsigned int timeout,
                          const JS::CallArgs &aArgs, bool* aResult);

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
