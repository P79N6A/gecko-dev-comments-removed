





































#ifndef nsJSPrincipals_h__
#define nsJSPrincipals_h__
#include "jsapi.h"
#include "nsIPrincipal.h"

class nsCString;

struct nsJSPrincipals : JSPrincipals
{
  static nsresult Startup();
  nsJSPrincipals();
  nsresult Init(nsIPrincipal* aPrincipal, const nsCString& aCodebase);
  ~nsJSPrincipals(void);

  nsIPrincipal *nsIPrincipalPtr; 
};

#endif 
