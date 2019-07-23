














































#ifndef nsCLiveconnect_h___
#define nsCLiveconnect_h___

#include "nsILiveconnect.h"
#include "nsAgg.h"






class nsCLiveconnect :public nsILiveconnect {
public:
    
    

    NS_DECL_AGGREGATED

    
    

    









    NS_IMETHOD	
    GetMember(JNIEnv *jEnv, lcjsobject obj, const jchar *name, jsize length, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports, jobject *pjobj);

    







    NS_IMETHOD	
    GetSlot(JNIEnv *jEnv, lcjsobject obj, jint slot, void* principalsArray[], 
            int numPrincipals, nsISupports *securitySupports,  jobject *pjobj);

    








    NS_IMETHOD	
    SetMember(JNIEnv *jEnv, lcjsobject obj, const jchar *name, jsize length, jobject jobj, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports);

    








    NS_IMETHOD	
    SetSlot(JNIEnv *jEnv, lcjsobject obj, jint slot, jobject jobj,  void* principalsArray[], 
            int numPrincipals, nsISupports *securitySupports);

    





    NS_IMETHOD	
    RemoveMember(JNIEnv *jEnv, lcjsobject obj, const jchar *name, jsize length,  void* principalsArray[], 
                 int numPrincipals, nsISupports *securitySupports);

    







    NS_IMETHOD	
    Call(JNIEnv *jEnv, lcjsobject obj, const jchar *name, jsize length, jobjectArray jobjArr, void* principalsArray[], 
         int numPrincipals, nsISupports *securitySupports, jobject *pjobj);

    








    NS_IMETHOD	
    Eval(JNIEnv *jEnv, lcjsobject obj, const jchar *script, jsize length, void* principalsArray[], 
         int numPrincipals, nsISupports *securitySupports, jobject *pjobj);

    








    NS_IMETHOD	
    GetWindow(JNIEnv *jEnv, void *pJavaObject,  void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports, lcjsobject *pobj);

    





    NS_IMETHOD	
    FinalizeJSObject(JNIEnv *jEnv, lcjsobject obj);

    






    NS_IMETHOD
    ToString(JNIEnv *jEnv, lcjsobject obj, jstring *pjstring);
    
    

    nsCLiveconnect(nsISupports *aOuter);
    virtual ~nsCLiveconnect(void);

protected:
    void* mJavaClient;
};

#endif 
