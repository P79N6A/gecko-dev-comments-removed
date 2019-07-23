













































#ifndef nsISecureLiveconnect_h___
#define nsISecureLiveconnect_h___

#include "nsISupports.h"
#include "nsIFactory.h"
#include "jni.h"







#if JS_BYTES_PER_WORD == 8
typedef jlong jsobject;
#else
typedef jint jsobject;
#endif 

class nsISecureLiveconnect : public nsISupports {
public:
    










    NS_IMETHOD	
    Eval(JNIEnv *jEnv, jsobject obj, const jchar *script, jsize length, void **pNSIPrincipaArray, 
         int numPrincipals, void *pNSISecurityContext, jobject *pjobj) = 0;
};


#define NS_ISECURELIVECONNECT_IID                          \
{ /* 68190910-3318-11d2-97f0-00805f8a28d0 */         \
    0x68190910,                                      \
    0x3318,                                          \
    0x11d2,                                          \
    {0x97, 0xf0, 0x00, 0x80, 0x5f, 0x8a, 0x28, 0xd0} \
}


#endif 
