





#ifndef mozilla_dom_ChannelInfo_h
#define mozilla_dom_ChannelInfo_h

#include "nsString.h"

class nsIChannel;

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
  {
  }

  ChannelInfo(const ChannelInfo& aRHS)
    : mSecurityInfo(aRHS.mSecurityInfo)
    , mInited(aRHS.mInited)
  {
  }

  ChannelInfo&
  operator=(const ChannelInfo& aRHS)
  {
    mSecurityInfo = aRHS.mSecurityInfo;
    mInited = aRHS.mInited;
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
  bool mInited;
};

} 
} 

#endif 
