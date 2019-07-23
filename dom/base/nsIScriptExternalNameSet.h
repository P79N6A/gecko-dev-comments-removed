




































#ifndef nsIScriptExternalNameSet_h__
#define nsIScriptExternalNameSet_h__

#include "nsISupports.h"

#define NS_ISCRIPTEXTERNALNAMESET_IID \
  {0xa6cf90da, 0x15b3, 0x11d2,        \
  {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsIScriptContext;









class nsIScriptExternalNameSet : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTEXTERNALNAMESET_IID)

  


  NS_IMETHOD InitializeNameSet(nsIScriptContext* aScriptContext) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptExternalNameSet,
                              NS_ISCRIPTEXTERNALNAMESET_IID)

#endif 
