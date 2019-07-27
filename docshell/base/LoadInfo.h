





#ifndef mozilla_LoadInfo_h
#define mozilla_LoadInfo_h

#include "nsIPrincipal.h"
#include "nsILoadInfo.h"

namespace mozilla {




class LoadInfo MOZ_FINAL : public nsILoadInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSILOADINFO

  enum InheritType
  {
    eInheritPrincipal,
    eDontInheritPrincipal
  };

  enum SandboxType
  {
    eSandboxed,
    eNotSandboxed
  };

  
  
  
  LoadInfo(nsIPrincipal* aPrincipal,
           InheritType aInheritPrincipal,
           SandboxType aSandboxed);

private:
  ~LoadInfo();

  nsCOMPtr<nsIPrincipal> mPrincipal;
  bool mInheritPrincipal;
  bool mSandboxed;
};

} 

#endif

