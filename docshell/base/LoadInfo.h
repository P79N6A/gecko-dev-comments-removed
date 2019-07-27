





#ifndef mozilla_LoadInfo_h
#define mozilla_LoadInfo_h

#include "nsIContentPolicy.h"
#include "nsILoadInfo.h"
#include "nsIPrincipal.h"
#include "nsIWeakReferenceUtils.h" 
#include "nsIURI.h"

class nsINode;

namespace mozilla {




class MOZ_EXPORT LoadInfo MOZ_FINAL : public nsILoadInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADINFO

  
  LoadInfo(nsIPrincipal* aPrincipal,
           nsINode* aLoadingContext,
           nsSecurityFlags aSecurityFlags,
           nsContentPolicyType aContentPolicyType,
           nsIURI* aBaseURI = nullptr);

private:
  ~LoadInfo();

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsWeakPtr              mLoadingContext;
  nsSecurityFlags        mSecurityFlags;
  nsContentPolicyType    mContentPolicyType;
  nsCOMPtr<nsIURI>       mBaseURI;
};

} 

#endif 

