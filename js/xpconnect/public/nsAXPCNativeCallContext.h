





#ifndef nsAXPCNativeCallContext_h__
#define nsAXPCNativeCallContext_h__






class nsAXPCNativeCallContext
{
public:
    NS_IMETHOD GetCallee(nsISupports** aResult) = 0;
    NS_IMETHOD GetCalleeMethodIndex(uint16_t* aResult) = 0;
    NS_IMETHOD GetJSContext(JSContext** aResult) = 0;
    NS_IMETHOD GetArgc(uint32_t* aResult) = 0;
    NS_IMETHOD GetArgvPtr(JS::Value** aResult) = 0;

    

    NS_IMETHOD GetCalleeInterface(nsIInterfaceInfo** aResult) = 0;
    NS_IMETHOD GetCalleeClassInfo(nsIClassInfo** aResult) = 0;

    NS_IMETHOD GetPreviousCallContext(nsAXPCNativeCallContext** aResult) = 0;

    enum { LANG_UNKNOWN = 0,
           LANG_JS      = 1,
           LANG_NATIVE  = 2 };

    NS_IMETHOD GetLanguage(uint16_t* aResult) = 0;
};

#endif
