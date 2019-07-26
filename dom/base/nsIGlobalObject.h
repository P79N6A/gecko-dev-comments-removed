




#ifndef nsIGlobalObject_h__
#define nsIGlobalObject_h__

#include "nsISupports.h"
#include "js/TypeDecls.h"

#define NS_IGLOBALOBJECT_IID \
{ 0xe2538ded, 0x13ef, 0x4f4d, \
{ 0x94, 0x6b, 0x65, 0xd3, 0x33, 0xb4, 0xf0, 0x3c } }

class nsIGlobalObject : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGLOBALOBJECT_IID)

  virtual JSObject* GetGlobalJSObject() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGlobalObject,
                              NS_IGLOBALOBJECT_IID)

#endif 
