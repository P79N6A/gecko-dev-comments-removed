




#ifndef nsIJSNativeInitializer_h__
#define nsIJSNativeInitializer_h__

#include "nsISupports.h"
#include "jsapi.h"

#define NS_IJSNATIVEINITIALIZER_IID \
{ 0xdb48eee5, 0x89a4, 0x4f18,       \
  { 0x86, 0xd0, 0x4c, 0x4e, 0x9d, 0x4b, 0xf8, 0x7e } }









class nsIJSNativeInitializer : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSNATIVEINITIALIZER_IID)

  



  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext *cx, JSObject *obj,
                        const JS::CallArgs& args) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSNativeInitializer,
                              NS_IJSNATIVEINITIALIZER_IID)

#endif 
