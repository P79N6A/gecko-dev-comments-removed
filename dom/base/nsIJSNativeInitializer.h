




































#ifndef nsIJSNativeInitializer_h__
#define nsIJSNativeInitializer_h__

#include "nsISupports.h"
#include "jsapi.h"

#define NS_IJSNATIVEINITIALIZER_IID \
{ 0x536c5ad2, 0x1275, 0x4706,       \
  { 0x99, 0xbd, 0x4a, 0xef, 0xb2, 0x4a, 0xb7, 0xf7 } }








class nsIJSNativeInitializer : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IJSNATIVEINITIALIZER_IID)

  



  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext *cx, JSObject *obj,
                        PRUint32 argc, jsval *argv) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIJSNativeInitializer,
                              NS_IJSNATIVEINITIALIZER_IID)

#endif 
