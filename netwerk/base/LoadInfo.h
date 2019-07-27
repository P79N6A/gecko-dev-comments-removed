





#ifndef mozilla_LoadInfo_h
#define mozilla_LoadInfo_h

#include "nsIContentPolicy.h"
#include "nsILoadInfo.h"
#include "nsIPrincipal.h"
#include "nsIWeakReferenceUtils.h" 
#include "nsIURI.h"

class nsINode;

namespace mozilla {

namespace net {
class HttpChannelParent;
class FTPChannelParent;
class WebSocketChannelParent;
}









class MOZ_EXPORT LoadInfo final : public nsILoadInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADINFO

  
  LoadInfo(nsIPrincipal* aLoadingPrincipal,
           nsIPrincipal* aTriggeringPrincipal,
           nsINode* aLoadingContext,
           nsSecurityFlags aSecurityFlags,
           nsContentPolicyType aContentPolicyType,
           nsIURI* aBaseURI = nullptr);

private:
  
  
  
  LoadInfo(nsIPrincipal* aLoadingPrincipal,
           nsIPrincipal* aTriggeringPrincipal,
           nsSecurityFlags aSecurityFlags,
           nsContentPolicyType aContentPolicyType,
           uint32_t aInnerWindowID);

  friend class net::HttpChannelParent;
  friend class net::FTPChannelParent;
  friend class net::WebSocketChannelParent;

  ~LoadInfo();

  nsCOMPtr<nsIPrincipal> mLoadingPrincipal;
  nsCOMPtr<nsIPrincipal> mTriggeringPrincipal;
  nsWeakPtr mLoadingContext;
  nsSecurityFlags mSecurityFlags;
  nsContentPolicyType mContentPolicyType;
  nsCOMPtr<nsIURI> mBaseURI;
  uint32_t mInnerWindowID;
};

} 

#endif 

