












































#ifndef nsILiveconnect_h___
#define nsILiveconnect_h___

#include "nsISupports.h"
#include "nsIFactory.h"
#include "jni.h"
#include "jsjava.h" 

#define NS_ILIVECONNECT_IID                          \
{ /* 68190910-3318-11d2-97f0-00805f8a28d0 */         \
    0x68190910,                                      \
    0x3318,                                          \
    0x11d2,                                          \
    {0x97, 0xf0, 0x00, 0x80, 0x5f, 0x8a, 0x28, 0xd0} \
}

#define NS_CLIVECONNECT_CID                          \
{ /* b8f0cef0-3931-11d2-97f0-00805f8a28d0 */         \
    0xb8f0cef0,                                      \
    0x3931,                                          \
    0x11d2,                                          \
    {0x97, 0xf0, 0x00, 0x80, 0x5f, 0x8a, 0x28, 0xd0} \
}

class nsILiveconnect : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILIVECONNECT_IID)
	NS_DEFINE_STATIC_CID_ACCESSOR(NS_CLIVECONNECT_CID)
	
    









    NS_IMETHOD
    GetMember(JNIEnv *jEnv, lcjsobject jsobj, const jchar *name, jsize length, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports, jobject *pjobj) = 0;

    







    NS_IMETHOD
    GetSlot(JNIEnv *jEnv, lcjsobject jsobj, jint slot, void* principalsArray[], 
            int numPrincipals, nsISupports *securitySupports, jobject *pjobj) = 0;

    








    NS_IMETHOD
    SetMember(JNIEnv *jEnv, lcjsobject jsobj, const jchar* name, jsize length, jobject jobj, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports) = 0;

    








    NS_IMETHOD
    SetSlot(JNIEnv *jEnv, lcjsobject jsobj, jint slot, jobject jobj, void* principalsArray[], 
            int numPrincipals, nsISupports *securitySupports) = 0;

    





    NS_IMETHOD
    RemoveMember(JNIEnv *jEnv, lcjsobject jsobj, const jchar* name, jsize length,  void* principalsArray[], 
                 int numPrincipals, nsISupports *securitySupports) = 0;

    







    NS_IMETHOD
    Call(JNIEnv *jEnv, lcjsobject jsobj, const jchar* name, jsize length, jobjectArray jobjArr,  void* principalsArray[], 
         int numPrincipals, nsISupports *securitySupports, jobject *pjobj) = 0;

    








    NS_IMETHOD	
    Eval(JNIEnv *jEnv, lcjsobject obj, const jchar *script, jsize length, void* principalsArray[], 
         int numPrincipals, nsISupports *securitySupports, jobject *pjobj) = 0;

    








    NS_IMETHOD
    GetWindow(JNIEnv *jEnv, void *pJavaObject, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports, lcjsobject *pobj) = 0;

    





    NS_IMETHOD
    FinalizeJSObject(JNIEnv *jEnv, lcjsobject jsobj) = 0;

    






    NS_IMETHOD
    ToString(JNIEnv *jEnv, lcjsobject obj, jstring *pjstring) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILiveconnect, NS_ILIVECONNECT_IID)

#endif 
