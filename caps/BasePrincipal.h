





#ifndef mozilla_BasePrincipal_h
#define mozilla_BasePrincipal_h

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"

namespace mozilla {








class BasePrincipal : public nsJSPrincipals
{
public:
  BasePrincipal() {}
  NS_IMETHOD GetCsp(nsIContentSecurityPolicy** aCsp);
  NS_IMETHOD SetCsp(nsIContentSecurityPolicy* aCsp);

protected:
  virtual ~BasePrincipal() {}

  nsCOMPtr<nsIContentSecurityPolicy> mCSP;
};

} 

#endif 
