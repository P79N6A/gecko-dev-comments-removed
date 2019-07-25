








































#ifndef nsAXPCNativeCallContext_h__
#define nsAXPCNativeCallContext_h__

class nsIXPConnectWrappedNative;






class nsAXPCNativeCallContext
{
public:
    NS_IMETHOD GetCallee(nsISupports **aResult) = 0;
    NS_IMETHOD GetCalleeMethodIndex(PRUint16 *aResult) = 0;
    NS_IMETHOD GetCalleeWrapper(nsIXPConnectWrappedNative **aResult) = 0;
    NS_IMETHOD GetJSContext(JSContext **aResult) = 0;
    NS_IMETHOD GetArgc(PRUint32 *aResult) = 0;
    NS_IMETHOD GetArgvPtr(jsval **aResult) = 0;

    


    NS_IMETHOD GetRetValPtr(jsval **aResult) = 0;

    




    NS_IMETHOD GetReturnValueWasSet(bool *aResult) = 0;
    NS_IMETHOD SetReturnValueWasSet(bool aValue) = 0;

    

    NS_IMETHOD GetCalleeInterface(nsIInterfaceInfo **aResult) = 0;
    NS_IMETHOD GetCalleeClassInfo(nsIClassInfo **aResult) = 0;

    NS_IMETHOD GetPreviousCallContext(nsAXPCNativeCallContext **aResult) = 0;

    enum { LANG_UNKNOWN = 0,
           LANG_JS      = 1,
           LANG_NATIVE  = 2 };

    NS_IMETHOD GetLanguage(PRUint16 *aResult) = 0;
};

#endif
