









































#include "xpcprivate.h"
#include "XPCWrapper.h"

JSBool XPCThrower::sVerbose = true;


void
XPCThrower::Throw(nsresult rv, JSContext* cx)
{
    const char* format;
    if (JS_IsExceptionPending(cx))
        return;
    if (!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";
    BuildAndThrowException(cx, rv, format);
}







JSBool
XPCThrower::CheckForPendingException(nsresult result, JSContext *cx)
{
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if (!xpc)
        return false;

    nsCOMPtr<nsIException> e;
    xpc->GetPendingException(getter_AddRefs(e));
    if (!e)
        return false;
    xpc->SetPendingException(nsnull);

    nsresult e_result;
    if (NS_FAILED(e->GetResult(&e_result)) || e_result != result)
        return false;

    if (!ThrowExceptionObject(cx, e))
        JS_ReportOutOfMemory(cx);
    return true;
}


void
XPCThrower::Throw(nsresult rv, XPCCallContext& ccx)
{
    char* sz;
    const char* format;

    if (CheckForPendingException(rv, ccx))
        return;

    if (!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";

    sz = (char*) format;

    if (sz && sVerbose)
        Verbosify(ccx, &sz, false);

    BuildAndThrowException(ccx, rv, sz);

    if (sz && sz != format)
        JS_smprintf_free(sz);
}



void
XPCThrower::ThrowBadResult(nsresult rv, nsresult result, XPCCallContext& ccx)
{
    char* sz;
    const char* format;
    const char* name;

    






    if (CheckForPendingException(result, ccx))
        return;

    

    if (!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format) || !format)
        format = "";

    if (nsXPCException::NameAndFormatForNSResult(result, &name, nsnull) && name)
        sz = JS_smprintf("%s 0x%x (%s)", format, result, name);
    else
        sz = JS_smprintf("%s 0x%x", format, result);

    if (sz && sVerbose)
        Verbosify(ccx, &sz, true);

    BuildAndThrowException(ccx, result, sz);

    if (sz)
        JS_smprintf_free(sz);
}


void
XPCThrower::ThrowBadParam(nsresult rv, uintN paramNum, XPCCallContext& ccx)
{
    char* sz;
    const char* format;

    if (!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";

    sz = JS_smprintf("%s arg %d", format, paramNum);

    if (sz && sVerbose)
        Verbosify(ccx, &sz, true);

    BuildAndThrowException(ccx, rv, sz);

    if (sz)
        JS_smprintf_free(sz);
}



void
XPCThrower::Verbosify(XPCCallContext& ccx,
                      char** psz, bool own)
{
    char* sz = nsnull;

    if (ccx.HasInterfaceAndMember()) {
        XPCNativeInterface* iface = ccx.GetInterface();
        jsid id = ccx.GetMember()->GetName();
        JSAutoByteString bytes;
        const char *name = JSID_IS_VOID(id) ? "Unknown" : bytes.encode(ccx, JSID_TO_STRING(id));
        if (!name) {
            name = "";
        }
        sz = JS_smprintf("%s [%s.%s]", *psz, iface->GetNameString(), name);
    }

    if (sz) {
        if (own)
            JS_smprintf_free(*psz);
        *psz = sz;
    }
}


void
XPCThrower::BuildAndThrowException(JSContext* cx, nsresult rv, const char* sz)
{
    JSBool success = false;

    
    if (rv == NS_ERROR_XPC_SECURITY_MANAGER_VETO && JS_IsExceptionPending(cx))
        return;
    nsCOMPtr<nsIException> finalException;
    nsCOMPtr<nsIException> defaultException;
    nsXPCException::NewException(sz, rv, nsnull, nsnull, getter_AddRefs(defaultException));
    XPCPerThreadData* tls = XPCPerThreadData::GetData(cx);
    if (tls) {
        nsIExceptionManager * exceptionManager = tls->GetExceptionManager();
        if (exceptionManager) {
           
           
            exceptionManager->GetExceptionFromProvider(rv,
                                                       defaultException,
                                                       getter_AddRefs(finalException));
            
            
            if (finalException == nsnull) {
                finalException = defaultException;
            }
        }
    }
    
    
    if (finalException)
        success = ThrowExceptionObject(cx, finalException);
    
    
    if (!success)
        JS_ReportOutOfMemory(cx);
}

static bool
IsCallerChrome(JSContext* cx)
{
    nsresult rv;

    nsCOMPtr<nsIScriptSecurityManager> secMan;
    if (XPCPerThreadData::IsMainThread(cx)) {
        secMan = XPCWrapper::GetSecurityManager();
    } else {
        nsXPConnect* xpc = nsXPConnect::GetXPConnect();
        if (!xpc)
            return false;

        nsCOMPtr<nsIXPCSecurityManager> xpcSecMan;
        PRUint16 flags = 0;
        rv = xpc->GetSecurityManagerForJSContext(cx, getter_AddRefs(xpcSecMan),
                                                 &flags);
        if (NS_FAILED(rv) || !xpcSecMan)
            return false;

        secMan = do_QueryInterface(xpcSecMan);
    }

    if (!secMan)
        return false;

    bool isChrome;
    rv = secMan->SubjectPrincipalIsSystem(&isChrome);
    return NS_SUCCEEDED(rv) && isChrome;
}


JSBool
XPCThrower::ThrowExceptionObject(JSContext* cx, nsIException* e)
{
    JSBool success = false;
    if (e) {
        nsCOMPtr<nsIXPCException> xpcEx;
        jsval thrown;
        nsXPConnect* xpc;

        
        
        
        if (!IsCallerChrome(cx) &&
            (xpcEx = do_QueryInterface(e)) &&
            NS_SUCCEEDED(xpcEx->StealJSVal(&thrown))) {
            if (!JS_WrapValue(cx, &thrown))
                return false;
            JS_SetPendingException(cx, thrown);
            success = true;
        } else if ((xpc = nsXPConnect::GetXPConnect())) {
            JSObject* glob = JS_GetGlobalForScopeChain(cx);
            if (!glob)
                return false;

            nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
            nsresult rv = xpc->WrapNative(cx, glob, e,
                                          NS_GET_IID(nsIException),
                                          getter_AddRefs(holder));
            if (NS_SUCCEEDED(rv) && holder) {
                JSObject* obj;
                if (NS_SUCCEEDED(holder->GetJSObject(&obj))) {
                    JS_SetPendingException(cx, OBJECT_TO_JSVAL(obj));
                    success = true;
                }
            }
        }
    }
    return success;
}
