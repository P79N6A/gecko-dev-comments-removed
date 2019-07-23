









































#include "xpctest_private.h"

#if defined(WIN32) && !defined(XPCONNECT_STANDALONE)
#define IMPLEMENT_TIMER_STUFF 1
#endif

class xpctestEcho : public nsIEcho
#ifdef IMPLEMENT_TIMER_STUFF
, public nsITimerCallback
#endif 
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIECHO

#ifdef IMPLEMENT_TIMER_STUFF
    NS_DECL_NSITIMERCALLBACK
#endif 

    xpctestEcho();
    virtual ~xpctestEcho();
private:
    nsIEcho* mReceiver;
    char*    mString;
    PRInt32  mSomeValue;
};



#ifdef IMPLEMENT_TIMER_STUFF
NS_IMPL_ISUPPORTS2(xpctestEcho, nsIEcho, nsITimerCallback)
#else
NS_IMPL_ISUPPORTS1(xpctestEcho, nsIEcho)
#endif 

xpctestEcho::xpctestEcho()
    : mReceiver(nsnull), mString(nsnull), mSomeValue(0)
{
    NS_ADDREF_THIS();
}

xpctestEcho::~xpctestEcho()
{
    NS_IF_RELEASE(mReceiver);
    if(mString)
        nsMemory::Free(mString);
}

NS_IMETHODIMP xpctestEcho::SetReceiver(nsIEcho* aReceiver)
{
    NS_IF_ADDREF(aReceiver);
    NS_IF_RELEASE(mReceiver);
    mReceiver = aReceiver;
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::SendOneString(const char* str)
{
    if(mReceiver)
        return mReceiver->SendOneString(str);
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::In2OutOneInt(int input, int* output)
{
    *output = input;
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::In2OutOneDOMString(const nsAString & input, 
                                              nsAString & _retval)
{
    _retval.Assign(input);
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::EchoIn2OutOneDOMString(const nsAString & input, nsAString & _retval)
{
    if(mReceiver)
        return mReceiver->EchoIn2OutOneDOMString(input, _retval);
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::In2OutOneAString(const nsAString & input, 
                                              nsAString & _retval)
{
    _retval.Assign(input);
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::EchoIn2OutOneAString(const nsAString & input, nsAString & _retval)
{
    if(mReceiver)
        return mReceiver->EchoIn2OutOneAString(input, _retval);
    return NS_OK;
}



NS_IMETHODIMP xpctestEcho::In2OutOneUTF8String(const nsACString & input, 
                                              nsACString & _retval)
{
    _retval.Assign(input);
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::EchoIn2OutOneUTF8String(const nsACString & input, 
                                                   nsACString & _retval)
{
    if(mReceiver)
        return mReceiver->EchoIn2OutOneUTF8String(input, _retval);
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::In2OutOneCString(const nsACString & input, 
                                            nsACString & _retval)
{
    _retval.Assign(input);
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::EchoIn2OutOneCString(const nsACString & input, 
                                                nsACString & _retval)
{
    if(mReceiver)
        return mReceiver->EchoIn2OutOneCString(input, _retval);
    return NS_OK;
}


NS_IMETHODIMP xpctestEcho::In2OutAddTwoInts(int input1,
                                       int input2,
                                       int* output1,
                                       int* output2,
                                       int* result)
{
    *output1 = input1;
    *output2 = input2;
    *result = input1+input2;
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::In2OutOneString(const char* input, char** output)
{
    char* p;
    int len;
    if(input && output &&
       (nsnull != (p = (char*)nsMemory::Alloc(len=strlen(input)+1))))
    {
        memcpy(p, input, len);
        *output = p;
        return NS_OK;
    }
    if(output)
        *output = nsnull;
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP xpctestEcho::SimpleCallNoEcho()
{
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::SendManyTypes(PRUint8              p1,
                      PRInt16             p2,
                      PRInt32             p3,
                      PRInt64             p4,
                      PRUint8              p5,
                      PRUint16            p6,
                      PRUint32            p7,
                      PRUint64            p8,
                      float             p9,
                      double            p10,
                      PRBool            p11,
                      char              p12,
                      PRUnichar            p13,
                      const nsID*       p14,
                      const char*       p15,
                      const PRUnichar*  p16)
{
    if(mReceiver)
        return mReceiver->SendManyTypes(p1, p2, p3, p4, p5, p6, p7, p8, p9,
                                        p10, p11, p12, p13, p14, p15, p16);
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::SendInOutManyTypes(PRUint8*    p1,
                           PRInt16*   p2,
                           PRInt32*   p3,
                           PRInt64*   p4,
                           PRUint8*    p5,
                           PRUint16*  p6,
                           PRUint32*  p7,
                           PRUint64*  p8,
                           float*   p9,
                           double*  p10,
                           PRBool*  p11,
                           char*    p12,
                           PRUnichar*  p13,
                           nsID**   p14,
                           char**   p15,
                           PRUnichar** p16)
{
    if(mReceiver)
        return mReceiver->SendInOutManyTypes(p1, p2, p3, p4, p5, p6, p7, p8, p9,
                                             p10, p11, p12, p13, p14, p15, p16);
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::MethodWithNative(int p1, void* p2)
{
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::ReturnCode(int code)
{
    return (nsresult) code;
}

NS_IMETHODIMP
xpctestEcho::FailInJSTest(int fail)
{
    if(mReceiver)
        return mReceiver->FailInJSTest(fail);
    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::SharedString(const char **str)
{
    *str = "a static string";







    return NS_OK;
}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_OK()
{return NS_OK;}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_ERROR_NULL_POINTER()
{return NS_ERROR_NULL_POINTER;}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_ERROR_UNEXPECTED()
{return NS_ERROR_UNEXPECTED;}

NS_IMETHODIMP
xpctestEcho::ReturnCode_NS_ERROR_OUT_OF_MEMORY()
{return NS_ERROR_OUT_OF_MEMORY;}

NS_IMETHODIMP
xpctestEcho::ReturnInterface(nsISupports *obj, nsISupports **_retval)
{
    if(!_retval)
        return NS_ERROR_NULL_POINTER;
    if(obj)
        NS_ADDREF(obj);
    *_retval = obj;
    return NS_OK;
}


NS_IMETHODIMP
xpctestEcho::GetStack(nsIStackFrame **_retval)
{
    nsIStackFrame* stack = nsnull;
    if(!_retval)
        return NS_ERROR_NULL_POINTER;

    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if(NS_SUCCEEDED(rv))
    {
        nsIStackFrame* jsstack;
        if(NS_SUCCEEDED(xpc->GetCurrentJSStack(&jsstack)) && jsstack)
        {
            xpc->CreateStackFrameLocation(nsIProgrammingLanguage::CPLUSPLUS,
                                          __FILE__,
                                          "xpctestEcho::GetStack",
                                          __LINE__,
                                          jsstack,
                                          &stack);
            NS_RELEASE(jsstack);
        }
    }

    if(stack)
    {
        *_retval = stack;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
xpctestEcho::SetReceiverReturnOldReceiver(nsIEcho **aReceiver)
{
    if(!aReceiver)
        return NS_ERROR_NULL_POINTER;

    nsIEcho* oldReceiver = mReceiver;
    mReceiver = *aReceiver;
    if(mReceiver)
        NS_ADDREF(mReceiver);

    
    *aReceiver = oldReceiver;
    return NS_OK;
}


NS_IMETHODIMP
xpctestEcho::MethodWithForwardDeclaredParam(nsITestXPCSomeUselessThing *sut)
{
    return NS_OK;
}


NS_IMETHODIMP
xpctestEcho::PseudoQueryInterface(const nsIID & uuid, void * *result)
{
    if(!result)
        return NS_ERROR_NULL_POINTER;
    if(mReceiver)
        return mReceiver->PseudoQueryInterface(uuid, result);
    return NS_OK;
}        


NS_IMETHODIMP
xpctestEcho::DebugDumpJSStack()
{
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
    if(NS_SUCCEEDED(rv))
    {
        rv = xpc->DebugDumpJSStack(JS_TRUE, JS_TRUE, JS_TRUE);
    }
    return rv;
}        


NS_IMETHODIMP 
xpctestEcho::GetAString(char * *aAString)
{
    printf(">>>> xpctestEcho::GetAString called\n");
    if(mString)
        *aAString = (char*) nsMemory::Clone(mString, strlen(mString)+1);
    else
        *aAString = nsnull;
    return NS_OK;
}
NS_IMETHODIMP 
xpctestEcho::SetAString(const char * aAString)
{
    printf("<<<< xpctestEcho::SetAString called\n");
    if(mString)
        nsMemory::Free(mString);
    if(aAString)
        mString = (char*) nsMemory::Clone(aAString, strlen(aAString)+1);
    else
        mString = nsnull;
    return NS_OK;
}







#define GET_CALL_CONTEXT \
  nsresult rv; \
  nsAXPCNativeCallContext *cc = nsnull; \
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv)); \
  if(NS_SUCCEEDED(rv)) \
    rv = xpc->GetCurrentNativeCallContext(&cc) /* no ';' */        


NS_IMETHODIMP
xpctestEcho::PrintArgTypes(void)
{
    GET_CALL_CONTEXT;
    if(NS_FAILED(rv) || !cc)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupports> callee;
    if(NS_FAILED(cc->GetCallee(getter_AddRefs(callee))) ||
       callee != static_cast<nsIEcho*>(this))
        return NS_ERROR_FAILURE;

    PRUint32 argc;
    if(NS_SUCCEEDED(cc->GetArgc(&argc)))
        printf("argc = %d  ", (int)argc);
    else
        return NS_ERROR_FAILURE;

    jsval* argv;
    if(NS_FAILED(cc->GetArgvPtr(&argv)))
        return NS_ERROR_FAILURE;

    printf("argv types = [");

    for(PRUint32 i = 0; i < argc; i++)
    {
        const char* type = "<unknown>";
        if(JSVAL_IS_OBJECT(argv[i]))
        {
            if(JSVAL_IS_NULL(argv[i]))
                type = "null";
            else
                type = "object";
        }
        else if (JSVAL_IS_BOOLEAN(argv[i]))
            type = "boolean";
        else if (JSVAL_IS_STRING(argv[i]))
            type = "string";
        else if (JSVAL_IS_DOUBLE(argv[i]))
            type = "double";
        else if (JSVAL_IS_INT(argv[i]))
            type = "int";
        else if (JSVAL_IS_VOID(argv[i]))
            type = "void";

        fputs(type, stdout);

        if(i < argc-1)
            printf(", ");
    }
    printf("]\n");
    
    return NS_OK;
}


NS_IMETHODIMP
xpctestEcho::ThrowArg(void)
{
    GET_CALL_CONTEXT;
    if(NS_FAILED(rv) || !cc)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupports> callee;
    if(NS_FAILED(cc->GetCallee(getter_AddRefs(callee))) || 
       callee != static_cast<nsIEcho*>(this))
        return NS_ERROR_FAILURE;

    PRUint32 argc;
    if(NS_FAILED(cc->GetArgc(&argc)) || !argc)
        return NS_OK;

    jsval* argv;
    JSContext* cx;
    if(NS_FAILED(cc->GetArgvPtr(&argv)) ||
       NS_FAILED(cc->GetJSContext(&cx)))
        return NS_ERROR_FAILURE;

    JS_SetPendingException(cx, argv[0]);
    return NS_OK;
}


NS_IMETHODIMP
xpctestEcho::CallReceiverSometimeLater(void)
{
    
    
    
    

#ifdef IMPLEMENT_TIMER_STUFF
    nsCOMPtr<nsITimer> timer;
    nsresult rv;
    timer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if(NS_FAILED(rv))
        return NS_ERROR_FAILURE;
    timer->InitWithCallback(static_cast<nsITimerCallback*>(this), 2000,
                            nsITimer::TYPE_ONE_SHOT);
    return NS_OK;
#else
    return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}

#ifdef IMPLEMENT_TIMER_STUFF
NS_IMETHODIMP
xpctestEcho::Notify(nsITimer *timer)
{
    if(mReceiver)
        mReceiver->CallReceiverSometimeLater();
    NS_RELEASE(timer);
    return NS_OK;
}        
#endif 


NS_IMETHODIMP
xpctestEcho::GetThrowInGetter(PRInt16 *aThrowInGetter)
{
    return NS_ERROR_FAILURE;
}        


NS_IMETHODIMP 
xpctestEcho::CallFunction(nsITestXPCFunctionCallback *callback, const char *s)
{
    return callback->Call(s);
}


NS_IMETHODIMP 
xpctestEcho::CallFunctionWithThis(nsITestXPCFunctionCallback *callback, nsISupports* self, const char *s)
{
    return callback->CallWithThis(self, s);
}


NS_IMETHODIMP 
xpctestEcho::GetSomeValue(PRInt32 *aSomeValue)
{
    *aSomeValue = mSomeValue;
    return NS_OK;
}

NS_IMETHODIMP xpctestEcho::SetSomeValue(PRInt32 aSomeValue)

{
    mSomeValue = aSomeValue;
    return NS_OK;
}




NS_IMETHODIMP
xpctest::ConstructEcho(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpctestEcho* obj = new xpctestEcho();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}
