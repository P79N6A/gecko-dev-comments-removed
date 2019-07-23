




































#ifndef nsIScriptObjectPrincipal_h__
#define nsIScriptObjectPrincipal_h__

#include "nsISupports.h"

class nsIPrincipal;


#define NS_ISCRIPTOBJECTPRINCIPAL_IID \
{ 0x3eedba38, 0x8d22, 0x41e1,  \
{ 0x81, 0x7a, 0x0e, 0x43, 0xe1, 0x65, 0xb6, 0x64} }




class nsIScriptObjectPrincipal : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTOBJECTPRINCIPAL_IID)

  virtual nsIPrincipal* GetPrincipal() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptObjectPrincipal,
                              NS_ISCRIPTOBJECTPRINCIPAL_IID)

#endif 
