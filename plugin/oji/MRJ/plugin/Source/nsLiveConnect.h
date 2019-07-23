







































#pragma once

#include "nsILiveconnect.h"
#include "nsIPluginStreamListener.h"
#include "SupportsMixin.h"

class MRJMonitor;

class nsLiveconnect : public nsILiveconnect,
                      public nsIPluginStreamListener,
                      private SupportsMixin {
public:
    DECL_SUPPORTS_MIXIN
    
    nsLiveconnect();
    virtual ~nsLiveconnect();
    
    









    NS_IMETHOD
    GetMember(JNIEnv *env, jsobject jsobj, const jchar *name, jsize length, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports, jobject *pjobj)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    







    NS_IMETHOD
    GetSlot(JNIEnv *env, jsobject jsobj, jint slot, void* principalsArray[], 
            int numPrincipals, nsISupports *securitySupports, jobject *pjobj)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    








    NS_IMETHOD
    SetMember(JNIEnv *env, jsobject jsobj, const jchar* name, jsize length, jobject jobj, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    








    NS_IMETHOD
    SetSlot(JNIEnv *env, jsobject jsobj, jint slot, jobject jobj, void* principalsArray[], 
            int numPrincipals, nsISupports *securitySupports)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    





    NS_IMETHOD
    RemoveMember(JNIEnv *env, jsobject jsobj, const jchar* name, jsize length,  void* principalsArray[], 
                 int numPrincipals, nsISupports *securitySupports)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    







    NS_IMETHOD
    Call(JNIEnv *env, jsobject jsobj, const jchar* name, jsize length, jobjectArray jobjArr,  void* principalsArray[], 
         int numPrincipals, nsISupports *securitySupports, jobject *pjobj)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    








    NS_IMETHOD  
    Eval(JNIEnv *env, jsobject obj, const jchar *script, jsize length, void* principalsArray[], 
         int numPrincipals, nsISupports *securitySupports, jobject *outResult);

    








    NS_IMETHOD
    GetWindow(JNIEnv *env, void *pJavaObject, void* principalsArray[], 
              int numPrincipals, nsISupports *securitySupports, jsobject *pobj)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    





    NS_IMETHOD
    FinalizeJSObject(JNIEnv *env, jsobject jsobj)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    






    NS_IMETHOD
    ToString(JNIEnv *env, jsobject obj, jstring *pjstring)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }

    
    
    






    NS_IMETHOD
    OnStartBinding(nsIPluginStreamInfo* pluginInfo)
    {
        return NS_OK;
    }

    









    NS_IMETHOD
    OnDataAvailable(nsIPluginStreamInfo* pluginInfo, nsIInputStream* input, PRUint32 length);

    NS_IMETHOD
    OnFileAvailable(nsIPluginStreamInfo* pluginInfo, const char* fileName)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
    
    










    NS_IMETHOD
    OnStopBinding(nsIPluginStreamInfo* pluginInfo, nsresult status);

    


    NS_IMETHOD
    GetStreamType(nsPluginStreamType *result)
    {
        *result = nsPluginStreamType_Normal;
        return NS_OK;
    }

private:
    MRJMonitor* mJavaScriptMonitor;
    char* mScript;
    char* mResult;

    
    static const InterfaceInfo sInterfaces[];
    static const UInt32 kInterfaceCount;
};
