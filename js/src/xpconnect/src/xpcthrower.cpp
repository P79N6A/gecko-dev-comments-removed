









































#include "xpcprivate.h"

JSBool XPCThrower::sVerbose = JS_TRUE;


void
XPCThrower::Throw(nsresult rv, JSContext* cx)
{
    const char* format;
    if(JS_IsExceptionPending(cx))
        return;
    if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";
    BuildAndThrowException(cx, rv, format);
}







JSBool
XPCThrower::CheckForPendingException(nsresult result, XPCCallContext &ccx)
{
    nsXPConnect* xpc = nsXPConnect::GetXPConnect();
    if(!xpc)
        return JS_FALSE;

    nsCOMPtr<nsIException> e;
    xpc->GetPendingException(getter_AddRefs(e));
    if(!e)
        return JS_FALSE;
    xpc->SetPendingException(nsnull);

    nsresult e_result;
    if(NS_FAILED(e->GetResult(&e_result)) || e_result != result)
        return JS_FALSE;

    if(!ThrowExceptionObject(ccx, e))
        JS_ReportOutOfMemory(ccx);
    return JS_TRUE;
}


void
XPCThrower::Throw(nsresult rv, XPCCallContext& ccx)
{
    char* sz;
    const char* format;

    if(CheckForPendingException(rv, ccx))
        return;

    if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";

    sz = (char*) format;

    if(sz && sVerbose)
        Verbosify(ccx, &sz, PR_FALSE);

    BuildAndThrowException(ccx, rv, sz);

    if(sz && sz != format)
        JS_smprintf_free(sz);
}



void
XPCThrower::ThrowBadResult(nsresult rv, nsresult result, XPCCallContext& ccx)
{
    char* sz;
    const char* format;
    const char* name;

    






    if(CheckForPendingException(result, ccx))
        return;

    

    if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format) || !format)
        format = "";

    if(nsXPCException::NameAndFormatForNSResult(result, &name, nsnull) && name)
        sz = JS_smprintf("%s 0x%x (%s)", format, result, name);
    else
        sz = JS_smprintf("%s 0x%x", format, result);

    if(sz && sVerbose)
        Verbosify(ccx, &sz, PR_TRUE);

    BuildAndThrowException(ccx, result, sz);

    if(sz)
        JS_smprintf_free(sz);
}


void
XPCThrower::ThrowBadParam(nsresult rv, uintN paramNum, XPCCallContext& ccx)
{
    char* sz;
    const char* format;

    if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";

    sz = JS_smprintf("%s arg %d", format, paramNum);

    if(sz && sVerbose)
        Verbosify(ccx, &sz, PR_TRUE);

    BuildAndThrowException(ccx, rv, sz);

    if(sz)
        JS_smprintf_free(sz);
}



void
XPCThrower::Verbosify(XPCCallContext& ccx,
                      char** psz, PRBool own)
{
    char* sz = nsnull;

    if(ccx.HasInterfaceAndMember())
    {
        XPCNativeInterface* iface = ccx.GetInterface();
#ifdef XPC_IDISPATCH_SUPPORT
        NS_ASSERTION(ccx.GetIDispatchMember() == nsnull || 
                        ccx.GetMember() == nsnull,
                     "Both IDispatch member and regular XPCOM member "
                     "were set in XPCCallContext");
        char const * name;
        if(ccx.GetIDispatchMember())
        {
            XPCDispInterface::Member * member = 
                reinterpret_cast<XPCDispInterface::Member*>(ccx.GetIDispatchMember());
            if(member && JSVAL_IS_STRING(member->GetName()))
            {
                name = JS_GetStringBytes(JSVAL_TO_STRING(member->GetName()));
            }
            else
                name = "Unknown";
        }
        else
            name = iface->GetMemberName(ccx, ccx.GetMember());
        sz = JS_smprintf("%s [%s.%s]",
                         *psz,
                         iface->GetNameString(),
                         name);
#else
        sz = JS_smprintf("%s [%s.%s]",
                         *psz,
                         iface->GetNameString(),
                         iface->GetMemberName(ccx, ccx.GetMember()));
#endif
    }

    if(sz)
    {
        if(own)
            JS_smprintf_free(*psz);
        *psz = sz;
    }
}


void
XPCThrower::BuildAndThrowException(JSContext* cx, nsresult rv, const char* sz)
{
    JSBool success = JS_FALSE;

    
    if(rv == NS_ERROR_XPC_SECURITY_MANAGER_VETO && JS_IsExceptionPending(cx))
        return;
    nsCOMPtr<nsIException> finalException;
    nsCOMPtr<nsIException> defaultException;
    nsXPCException::NewException(sz, rv, nsnull, nsnull, getter_AddRefs(defaultException));
    XPCPerThreadData* tls = XPCPerThreadData::GetData();
    if(tls)
    {
        nsIExceptionManager * exceptionManager = tls->GetExceptionManager();
        if(exceptionManager)
        {
           
           
            exceptionManager->GetExceptionFromProvider(
               rv,
               defaultException,
               getter_AddRefs(finalException));
            
            
            if(finalException == nsnull)
            {
                finalException = defaultException;
            }
        }
    }
    
    
    if(finalException)
        success = ThrowExceptionObject(cx, finalException);
    
    
    if(!success)
        JS_ReportOutOfMemory(cx);
}

static JSObject*
GetGlobalObject(JSContext* cx, JSObject* start)
{
    JSObject* parent;

    while((parent = JS_GetParent(cx, start)) != nsnull)
        start = parent;
    return start;
}


JSBool
XPCThrower::ThrowExceptionObject(JSContext* cx, nsIException* e)
{
    JSBool success = JS_FALSE;
    if(e)
    {
        nsXPConnect* xpc = nsXPConnect::GetXPConnect();
        if(xpc)
        {
            JSObject* glob = JS_GetScopeChain(cx);
            if(!glob)
                return JS_FALSE;
            glob = GetGlobalObject(cx, glob);

            nsCOMPtr<nsIXPConnectJSObjectHolder> holder;
            nsresult rv = xpc->WrapNative(cx, glob, e,
                                          NS_GET_IID(nsIException),
                                          getter_AddRefs(holder));
            if(NS_SUCCEEDED(rv) && holder)
            {
                JSObject* obj;
                if(NS_SUCCEEDED(holder->GetJSObject(&obj)))
                {
                    JS_SetPendingException(cx, OBJECT_TO_JSVAL(obj));
                    success = JS_TRUE;
                }
            }
        }
    }
    return success;
}

#ifdef XPC_IDISPATCH_SUPPORT

void
XPCThrower::ThrowCOMError(JSContext* cx, unsigned long COMErrorCode,
                          nsresult rv, const EXCEPINFO * exception)
{
    nsCAutoString msg;
    IErrorInfo * pError;
    const char * format;
    if(!nsXPCException::NameAndFormatForNSResult(rv, nsnull, &format))
        format = "";
    msg = format;
    if(exception)
    {
        msg += static_cast<const char *>
                          (_bstr_t(exception->bstrSource, false));
        msg += " : ";
        msg.AppendInt(static_cast<PRUint32>(COMErrorCode));
        msg += " - ";
        msg += static_cast<const char *>
                          (_bstr_t(exception->bstrDescription, false));
    }
    else
    {
        
        unsigned long result = GetErrorInfo(0, &pError);
        if(SUCCEEDED(result) && pError)
        {
            
            BSTR bstrSource = NULL;
            if(SUCCEEDED(pError->GetSource(&bstrSource)) && bstrSource)
            {
                _bstr_t src(bstrSource, false);
                msg += static_cast<const char *>(src);
                msg += " : ";
            }
            msg.AppendInt(static_cast<PRUint32>(COMErrorCode), 16);
            BSTR bstrDesc = NULL;
            if(SUCCEEDED(pError->GetDescription(&bstrDesc)) && bstrDesc)
            {
                msg += " - ";
                _bstr_t desc(bstrDesc, false);
                msg += static_cast<const char *>(desc);
            }
        }
        else
        {
            
            msg += "COM Error Result = ";
            msg.AppendInt(static_cast<PRUint32>(COMErrorCode), 16);
        }
    }
    
    XPCThrower::BuildAndThrowException(cx, rv, msg.get());
}

#endif
