







#include "xpcprivate.h"
#include "xpcpublic.h"
#include "XPCWrapper.h"

bool XPCThrower::sVerbose = true;


void
XPCThrower::Throw(nsresult rv, JSContext* cx)
{
    const char* format;
    if (JS_IsExceptionPending(cx))
        return;
    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format))
        format = "";
    BuildAndThrowException(cx, rv, format);
}

namespace xpc {

bool
Throw(JSContext *cx, nsresult rv)
{
    XPCThrower::Throw(rv, cx);
    return false;
}

} 







bool
XPCThrower::CheckForPendingException(nsresult result, JSContext *cx)
{
    nsCOMPtr<nsIException> e;
    XPCJSRuntime::Get()->GetPendingException(getter_AddRefs(e));
    if (!e)
        return false;
    XPCJSRuntime::Get()->SetPendingException(nullptr);

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

    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format))
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

    

    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format) || !format)
        format = "";

    if (nsXPCException::NameAndFormatForNSResult(result, &name, nullptr) && name)
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
XPCThrower::ThrowBadParam(nsresult rv, unsigned paramNum, XPCCallContext& ccx)
{
    char* sz;
    const char* format;

    if (!nsXPCException::NameAndFormatForNSResult(rv, nullptr, &format))
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
    char* sz = nullptr;

    if (ccx.HasInterfaceAndMember()) {
        XPCNativeInterface* iface = ccx.GetInterface();
        jsid id = ccx.GetMember()->GetName();
        JSAutoByteString bytes;
        const char *name = JSID_IS_VOID(id) ? "Unknown" : bytes.encodeLatin1(ccx, JSID_TO_STRING(id));
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
    bool success = false;

    
    if (rv == NS_ERROR_XPC_SECURITY_MANAGER_VETO && JS_IsExceptionPending(cx))
        return;
    nsCOMPtr<nsIException> finalException;
    nsCOMPtr<nsIException> defaultException;
    nsXPCException::NewException(sz, rv, nullptr, nullptr, getter_AddRefs(defaultException));

    nsIExceptionManager * exceptionManager = XPCJSRuntime::Get()->GetExceptionManager();
    if (exceptionManager) {
        
        
        exceptionManager->GetExceptionFromProvider(rv,
                                                   defaultException,
                                                   getter_AddRefs(finalException));
        
        
        if (finalException == nullptr) {
            finalException = defaultException;
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
    secMan = XPCWrapper::GetSecurityManager();

    if (!secMan)
        return false;

    bool isChrome;
    rv = secMan->SubjectPrincipalIsSystem(&isChrome);
    return NS_SUCCEEDED(rv) && isChrome;
}


bool
XPCThrower::ThrowExceptionObject(JSContext* cx, nsIException* e)
{
    bool success = false;
    if (e) {
        nsCOMPtr<nsIXPCException> xpcEx;
        JS::RootedValue thrown(cx);
        nsXPConnect* xpc;

        
        
        
        if (!IsCallerChrome(cx) &&
            (xpcEx = do_QueryInterface(e)) &&
            NS_SUCCEEDED(xpcEx->StealJSVal(thrown.address()))) {
            if (!JS_WrapValue(cx, thrown.address()))
                return false;
            JS_SetPendingException(cx, thrown);
            success = true;
        } else if ((xpc = nsXPConnect::XPConnect())) {
            JS::RootedObject glob(cx, JS::CurrentGlobalOrNull(cx));
            if (!glob)
                return false;

            nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
            nsresult rv = xpc->WrapNative(cx, glob, e,
                                          NS_GET_IID(nsIException),
                                          getter_AddRefs(holder));
            if (NS_SUCCEEDED(rv) && holder) {
                JS::RootedObject obj(cx, holder->GetJSObject());
                if (obj) {
                    JS_SetPendingException(cx, OBJECT_TO_JSVAL(obj));
                    success = true;
                }
            }
        }
    }
    return success;
}
