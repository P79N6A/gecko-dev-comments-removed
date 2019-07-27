





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
class OptionalLoadInfoArgs;
} 

namespace ipc {

nsresult
LoadInfoArgsToLoadInfo(const mozilla::net::OptionalLoadInfoArgs& aLoadInfoArgs,
                       nsILoadInfo** outLoadInfo);
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
           bool aUpgradeInsecureRequests,
           uint64_t aInnerWindowID,
           uint64_t aOuterWindowID,
           uint64_t aParentOuterWindowID);

  friend nsresult
  mozilla::ipc::LoadInfoArgsToLoadInfo(
    const mozilla::net::OptionalLoadInfoArgs& aLoadInfoArgs,
    nsILoadInfo** outLoadInfo);

  ~LoadInfo();

  nsCOMPtr<nsIPrincipal> mLoadingPrincipal;
  nsCOMPtr<nsIPrincipal> mTriggeringPrincipal;
  nsWeakPtr              mLoadingContext;
  nsSecurityFlags        mSecurityFlags;
  nsContentPolicyType    mContentPolicyType;
  nsCOMPtr<nsIURI>       mBaseURI;
  bool                   mUpgradeInsecureRequests;
  uint64_t               mInnerWindowID;
  uint64_t               mOuterWindowID;
  uint64_t               mParentOuterWindowID;
};

} 

#endif 

