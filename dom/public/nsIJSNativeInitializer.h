




































#ifndef nsIJSNativeInitializer_h__
#define nsIJSNativeInitializer_h__

#include "nsISupports.h"
#include "jsapi.h"

#define NS_IJSNATIVEINITIALIZER_IID \
{0xa6cf90f4, 0x15b3, 0x11d2,        \
 {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}








class nsIJSNativeInitializer : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSNATIVEINITIALIZER_IID)

  



  NS_IMETHOD Initialize(JSContext *cx, JSObject *obj, 
                        PRUint32 argc, jsval *argv) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSNativeInitializer,
                              NS_IJSNATIVEINITIALIZER_IID)

#endif 
