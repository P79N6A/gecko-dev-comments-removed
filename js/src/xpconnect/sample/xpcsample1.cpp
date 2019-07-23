








































#include "xpcsample1.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"



class nsXPCSample_ClassA;
class nsXPCSample_ClassB;
class nsXPCSample_ClassC;




class nsXPCSample_ClassA : public nsIXPCSample_ClassA
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSAMPLE_CLASSA

    nsXPCSample_ClassA(PRInt32 aValue);
    virtual ~nsXPCSample_ClassA();

private:
    PRInt32 mValue;
    nsCOMPtr<nsIXPCSample_ClassB> mClassB;
};



class nsXPCSample_ClassB : public nsIXPCSample_ClassB
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSAMPLE_CLASSB

    nsXPCSample_ClassB(PRInt32 aValue);
    virtual ~nsXPCSample_ClassB();

private:
    PRInt32 mValue;
    nsCOMPtr<nsIXPCSample_ClassC> mClassC;
};



class nsXPCSample_ClassC : public nsIXPCSample_ClassC
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSAMPLE_CLASSC

    nsXPCSample_ClassC(PRInt32 aValue);
    virtual ~nsXPCSample_ClassC();

private:
    PRInt32 mValue;
};




NS_IMPL_ISUPPORTS1(nsXPCSample_ClassA, nsIXPCSample_ClassA)

nsXPCSample_ClassA::nsXPCSample_ClassA(PRInt32 aValue)
    :   mValue(aValue)
{
}

nsXPCSample_ClassA::~nsXPCSample_ClassA()
{
    
}


NS_IMETHODIMP nsXPCSample_ClassA::GetSomeValue(PRInt32 *aSomeValue)
{
    *aSomeValue = mValue;
    return NS_OK;
}

NS_IMETHODIMP nsXPCSample_ClassA::SetSomeValue(PRInt32 aSomeValue)
{
    mValue = aSomeValue;
    return NS_OK;
}


NS_IMETHODIMP nsXPCSample_ClassA::GetB(nsIXPCSample_ClassB * *aB)
{
    if(!mClassB && !(mClassB = new nsXPCSample_ClassB(mValue)))
        return NS_ERROR_FAILURE;
    *aB = mClassB;
    NS_ADDREF(*aB);
    return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsXPCSample_ClassB, nsIXPCSample_ClassB)

nsXPCSample_ClassB::nsXPCSample_ClassB(PRInt32 aValue)
    :   mValue(aValue)
{
}

nsXPCSample_ClassB::~nsXPCSample_ClassB()
{
    
}


NS_IMETHODIMP nsXPCSample_ClassB::GetSomeValue(PRInt32 *aSomeValue)
{
    *aSomeValue = mValue;
    return NS_OK;
}

NS_IMETHODIMP nsXPCSample_ClassB::SetSomeValue(PRInt32 aSomeValue)
{
    mValue = aSomeValue;
    return NS_OK;
}


NS_IMETHODIMP nsXPCSample_ClassB::GetC(nsIXPCSample_ClassC * *aC)
{
    if(!mClassC && !(mClassC = new nsXPCSample_ClassC(mValue)))
        return NS_ERROR_FAILURE;
    *aC = mClassC;
    NS_ADDREF(*aC);
    return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsXPCSample_ClassC, nsIXPCSample_ClassC)

nsXPCSample_ClassC::nsXPCSample_ClassC(PRInt32 aValue)
    :   mValue(aValue)
{
}

nsXPCSample_ClassC::~nsXPCSample_ClassC()
{
    
}


NS_IMETHODIMP nsXPCSample_ClassC::GetSomeValue(PRInt32 *aSomeValue)
{
    *aSomeValue = mValue;
    return NS_OK;
}

NS_IMETHODIMP nsXPCSample_ClassC::SetSomeValue(PRInt32 aSomeValue)
{
    mValue = aSomeValue;
    return NS_OK;
}




class nsXPCSample_HookerUpper : public nsIXPCSample_HookerUpper
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSAMPLE_HOOKERUPPER

    nsXPCSample_HookerUpper();
    virtual ~nsXPCSample_HookerUpper();
};

NS_IMPL_ISUPPORTS1(nsXPCSample_HookerUpper, nsIXPCSample_HookerUpper)

nsXPCSample_HookerUpper::nsXPCSample_HookerUpper()
{
}

nsXPCSample_HookerUpper::~nsXPCSample_HookerUpper()
{
    
}


NS_IMETHODIMP
nsXPCSample_HookerUpper::CreateSampleObjectAtGlobalScope(const char *name, PRInt32 value)
{
    
    
    
    

    
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if(NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIXPCNativeCallContext> callContext;
    xpc->GetCurrentNativeCallContext(getter_AddRefs(callContext));
    if(!callContext)
        return NS_ERROR_FAILURE;

    
    
    nsCOMPtr<nsISupports> callee;
    callContext->GetCallee(getter_AddRefs(callee));
    if(!callee || callee.get() != (nsISupports*)this)
        return NS_ERROR_FAILURE;

    
    JSContext* cx;
    rv = callContext->GetJSContext(&cx);
    if(NS_FAILED(rv) || !cx)
        return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIXPConnectWrappedNative> calleeWrapper;
    callContext->GetCalleeWrapper(getter_AddRefs(calleeWrapper));
    if(!calleeWrapper)
        return NS_ERROR_FAILURE;

    
    JSObject* calleeJSObject;
    rv = calleeWrapper->GetJSObject(&calleeJSObject);
    if(NS_FAILED(rv) || !calleeJSObject)
        return NS_ERROR_FAILURE;

    
    
    
    JSObject* tempJSObject;
    JSObject* globalJSObject = calleeJSObject;
    while(tempJSObject = JS_GetParent(cx, globalJSObject))
        globalJSObject = tempJSObject;

    
    

    
    nsXPCSample_ClassA* classA = new nsXPCSample_ClassA(value);
    if(!classA)
        return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
    rv = xpc->WrapNative(cx, globalJSObject, classA,
                         NS_GET_IID(nsXPCSample_ClassA),
                         getter_AddRefs(wrapper));
    if(NS_FAILED(rv) || !wrapper)
        return NS_ERROR_FAILURE;

    
    JSObject* ourJSObject;
    rv = wrapper->GetJSObject(&ourJSObject);
    if(NS_FAILED(rv) || !ourJSObject)
        return NS_ERROR_FAILURE;

    
    

    if(!JS_DefineProperty(cx,
                          globalJSObject,       
                          "A",                  
                          OBJECT_TO_JSVAL(ourJSObject),
                          nsnull, nsnull,
                          JSPROP_PERMANENT |    
                          JSPROP_READONLY |
                          JSPROP_ENUMERATE))
        return NS_ERROR_FAILURE;

    
    return NS_OK;
}




NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPCSample_HookerUpper)

static const nsModuleComponentInfo components[] = {
{ "sample xpc component",
  { 0x97380cf0, 0xc21b, 0x11d3, { 0x98, 0xc9, 0x0, 0x60, 0x8, 0x96, 0x24, 0x22 } },
  NS_NSXPCSAMPLE_HOOKERUPPER_CONTRACTID,
  nsXPCSample_HookerUpperConstructor }
};

NS_IMPL_NSGETMODULE(xpconnect_samples, components)

