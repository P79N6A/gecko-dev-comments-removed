





#ifndef mozilla_dom_ChannelInfo_h
#define mozilla_dom_ChannelInfo_h

#include "nsString.h"
#include "nsCOMPtr.h"

class nsIChannel;
class nsIURI;

namespace mozilla {
namespace ipc {
class IPCChannelInfo;
} 

namespace dom {


















class ChannelInfo final
{
public:
  typedef mozilla::ipc::IPCChannelInfo IPCChannelInfo;

  ChannelInfo()
    : mInited(false)
    , mRedirected(false)
  {
  }

  ChannelInfo(const ChannelInfo& aRHS)
    : mSecurityInfo(aRHS.mSecurityInfo)
    , mRedirectedURISpec(aRHS.mRedirectedURISpec)
    , mInited(aRHS.mInited)
    , mRedirected(aRHS.mRedirected)
  {
  }

  ChannelInfo&
  operator=(const ChannelInfo& aRHS)
  {
    mSecurityInfo = aRHS.mSecurityInfo;
    mRedirectedURISpec = aRHS.mRedirectedURISpec;
    mInited = aRHS.mInited;
    mRedirected = aRHS.mRedirected;
    return *this;
  }

  void InitFromChannel(nsIChannel* aChannel);
  void InitFromIPCChannelInfo(const IPCChannelInfo& aChannelInfo);

  
  
  nsresult ResurrectInfoOnChannel(nsIChannel* aChannel);

  bool IsInitialized() const
  {
    return mInited;
  }

  IPCChannelInfo AsIPCChannelInfo() const;

private:
  void SetSecurityInfo(nsISupports* aSecurityInfo);

private:
  nsCString mSecurityInfo;
  nsCString mRedirectedURISpec;
  bool mInited;
  bool mRedirected;
};

} 
} 

#endif 
