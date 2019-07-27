





#ifndef mozilla_net_BaseWebSocketChannel_h
#define mozilla_net_BaseWebSocketChannel_h

#include "nsIWebSocketChannel.h"
#include "nsIWebSocketListener.h"
#include "nsIProtocolHandler.h"
#include "nsIThread.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsCOMPtr.h"
#include "nsString.h"

namespace mozilla {
namespace net {

const static int32_t kDefaultWSPort     = 80;
const static int32_t kDefaultWSSPort    = 443;

class BaseWebSocketChannel : public nsIWebSocketChannel,
                             public nsIProtocolHandler,
                             public nsIThreadRetargetableRequest
{
 public:
  BaseWebSocketChannel();

  NS_DECL_NSIPROTOCOLHANDLER
  NS_DECL_NSITHREADRETARGETABLEREQUEST

  NS_IMETHOD QueryInterface(const nsIID & uuid, void **result) MOZ_OVERRIDE = 0;
  NS_IMETHOD_(MozExternalRefCountType ) AddRef(void) MOZ_OVERRIDE = 0;
  NS_IMETHOD_(MozExternalRefCountType ) Release(void) MOZ_OVERRIDE = 0;

  
  
  NS_IMETHOD GetOriginalURI(nsIURI **aOriginalURI) MOZ_OVERRIDE;
  NS_IMETHOD GetURI(nsIURI **aURI) MOZ_OVERRIDE;
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor **aNotificationCallbacks) MOZ_OVERRIDE;
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor *aNotificationCallbacks) MOZ_OVERRIDE;
  NS_IMETHOD GetLoadGroup(nsILoadGroup **aLoadGroup) MOZ_OVERRIDE;
  NS_IMETHOD SetLoadGroup(nsILoadGroup *aLoadGroup) MOZ_OVERRIDE;
  NS_IMETHOD SetLoadInfo(nsILoadInfo *aLoadInfo) MOZ_OVERRIDE;
  NS_IMETHOD GetLoadInfo(nsILoadInfo **aLoadInfo) MOZ_OVERRIDE;
  NS_IMETHOD GetExtensions(nsACString &aExtensions) MOZ_OVERRIDE;
  NS_IMETHOD GetProtocol(nsACString &aProtocol) MOZ_OVERRIDE;
  NS_IMETHOD SetProtocol(const nsACString &aProtocol) MOZ_OVERRIDE;
  NS_IMETHOD GetPingInterval(uint32_t *aSeconds) MOZ_OVERRIDE;
  NS_IMETHOD SetPingInterval(uint32_t aSeconds) MOZ_OVERRIDE;
  NS_IMETHOD GetPingTimeout(uint32_t *aSeconds) MOZ_OVERRIDE;
  NS_IMETHOD SetPingTimeout(uint32_t aSeconds) MOZ_OVERRIDE;

  
  virtual void GetEffectiveURL(nsAString& aEffectiveURL) const = 0;
  virtual bool IsEncrypted() const = 0;

 protected:
  nsCOMPtr<nsIURI>                mOriginalURI;
  nsCOMPtr<nsIURI>                mURI;
  nsCOMPtr<nsIWebSocketListener>  mListener;
  nsCOMPtr<nsISupports>           mContext;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsCOMPtr<nsILoadGroup>          mLoadGroup;
  nsCOMPtr<nsILoadInfo>           mLoadInfo;
  nsCOMPtr<nsIEventTarget>        mTargetThread;

  nsCString                       mProtocol;
  nsCString                       mOrigin;

  nsCString                       mNegotiatedExtensions;

  uint32_t                        mEncrypted                 : 1;
  uint32_t                        mWasOpened                 : 1;
  uint32_t                        mClientSetPingInterval     : 1;
  uint32_t                        mClientSetPingTimeout      : 1;
  uint32_t                        mPingForced                : 1;

  uint32_t                        mPingInterval;         
  uint32_t                        mPingResponseTimeout;  
};

} 
} 

#endif 
