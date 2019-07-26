




#ifndef nsIGlobalObject_h__
#define nsIGlobalObject_h__

#include "nsISupports.h"
#include "nsIScriptObjectPrincipal.h"

class JSObject;

#define NS_IGLOBALOBJECT_IID \
{ 0x8503e9a9, 0x530, 0x4b26,  \
{ 0xae, 0x24, 0x18, 0xca, 0x38, 0xe5, 0xed, 0x17 } }

class nsIGlobalObject : public nsIScriptObjectPrincipal
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGLOBALOBJECT_IID)

  virtual JSObject* GetGlobalJSObject() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGlobalObject,
                              NS_IGLOBALOBJECT_IID)

#endif 
