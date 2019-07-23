





































#ifndef nsJSPrincipals_h__
#define nsJSPrincipals_h__
#include "jsapi.h"
#include "nsIPrincipal.h"

struct nsJSPrincipals : JSPrincipals
{
  static nsresult Startup();
  nsJSPrincipals();
  nsresult Init(nsIPrincipal* aPrincipal, const char *aCodebase);
  ~nsJSPrincipals(void);

  nsIPrincipal *nsIPrincipalPtr; 
};

#endif 
