











































#ifndef nsNullPrincipal_h__
#define nsNullPrincipal_h__

#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsCOMPtr.h"

class nsIURI;

#define NS_NULLPRINCIPAL_CLASSNAME "nullprincipal"
#define NS_NULLPRINCIPAL_CID \
{ 0xdd156d62, 0xd26f, 0x4441, \
 { 0x9c, 0xdb, 0xe8, 0xf0, 0x91, 0x07, 0xc2, 0x73 } }
#define NS_NULLPRINCIPAL_CONTRACTID "@mozilla.org/nullprincipal;1"

#define NS_NULLPRINCIPAL_SCHEME "moz-nullprincipal"

class nsNullPrincipal : public nsIPrincipal
{
public:
  nsNullPrincipal();
  
  
  

  
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPRINCIPAL
  NS_DECL_NSISERIALIZABLE

  nsresult Init();

protected:
  virtual ~nsNullPrincipal();

  nsJSPrincipals mJSPrincipals;
  nsCOMPtr<nsIURI> mURI;
};

#endif 
