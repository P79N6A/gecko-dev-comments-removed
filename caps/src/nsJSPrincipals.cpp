




































#include "xpcprivate.h"
#include "nsString.h"
#include "nsIObjectOutputStream.h"
#include "nsIObjectInputStream.h"
#include "nsJSPrincipals.h"
#include "plstr.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "jsapi.h"
#include "jsxdrapi.h"
#include "nsIJSRuntimeService.h"
#include "nsIServiceManager.h"
#include "nsMemory.h"
#include "nsStringBuffer.h"

static void *
nsGetPrincipalArray(JSContext *cx, JSPrincipals *prin)
{
    return nsnull;
}

static JSBool
nsGlobalPrivilegesEnabled(JSContext *cx, JSPrincipals *jsprin)
{
    return JS_TRUE;
}

static JSBool
nsJSPrincipalsSubsume(JSPrincipals *jsprin, JSPrincipals *other)
{
    nsJSPrincipals *nsjsprin = static_cast<nsJSPrincipals *>(jsprin);
    nsJSPrincipals *nsother  = static_cast<nsJSPrincipals *>(other);

    bool result;
    nsresult rv = nsjsprin->nsIPrincipalPtr->Subsumes(nsother->nsIPrincipalPtr,
                                                      &result);
    return NS_SUCCEEDED(rv) && result;
}

static void
nsDestroyJSPrincipals(JSContext *cx, struct JSPrincipals *jsprin)
{
    nsJSPrincipals *nsjsprin = static_cast<nsJSPrincipals *>(jsprin);

    
    

    
    
    
#ifdef NS_BUILD_REFCNT_LOGGING
    
    
    
    nsjsprin->refcount++;
    nsjsprin->nsIPrincipalPtr->AddRef();
    nsjsprin->refcount--;
#else
    nsjsprin->refcount++;
#endif
    nsjsprin->nsIPrincipalPtr->Release();
    
    
}

static JSBool
nsTranscodeJSPrincipals(JSXDRState *xdr, JSPrincipals **jsprinp)
{
    nsresult rv;

    if (xdr->mode == JSXDR_ENCODE) {
        nsIObjectOutputStream *stream =
            reinterpret_cast<nsIObjectOutputStream*>(xdr->userdata);

        
        uint32 size;
        char *data = (char*) ::JS_XDRMemGetData(xdr, &size);

        rv = stream->Write32(size);
        if (NS_SUCCEEDED(rv)) {
            rv = stream->WriteBytes(data, size);
            if (NS_SUCCEEDED(rv)) {
                ::JS_XDRMemResetData(xdr);

                
                
                nsJSPrincipals *nsjsprin =
                    static_cast<nsJSPrincipals*>(*jsprinp);

                rv = stream->WriteObject(nsjsprin->nsIPrincipalPtr, true);
            }
        }
    } else {
        NS_ASSERTION(JS_XDRMemDataLeft(xdr) == 0, "XDR out of sync?!");
        nsIObjectInputStream *stream =
            reinterpret_cast<nsIObjectInputStream*>(xdr->userdata);

        nsCOMPtr<nsIPrincipal> prin;
        rv = stream->ReadObject(true, getter_AddRefs(prin));
        if (NS_SUCCEEDED(rv)) {
            PRUint32 size;
            rv = stream->Read32(&size);
            if (NS_SUCCEEDED(rv)) {
                char *data = nsnull;
                if (size != 0)
                    rv = stream->ReadBytes(size, &data);
                if (NS_SUCCEEDED(rv)) {
                    char *olddata;
                    uint32 oldsize;

                    
                    
                    
                    
                    olddata = (char*) ::JS_XDRMemGetData(xdr, &oldsize);
                    nsMemory::Free(olddata);
                    ::JS_XDRMemSetData(xdr, data, size);

                    prin->GetJSPrincipals(xdr->cx, jsprinp);
                }
            }
        }
    }

    if (NS_FAILED(rv)) {
        ::JS_ReportError(xdr->cx, "can't %scode principals (failure code %x)",
                         (xdr->mode == JSXDR_ENCODE) ? "en" : "de",
                         (unsigned int) rv);
        return JS_FALSE;
    }
    return JS_TRUE;
}

nsresult
nsJSPrincipals::Startup()
{
    nsCOMPtr<nsIJSRuntimeService> rtsvc = nsXPConnect::GetXPConnect();
    if (!rtsvc)
        return NS_ERROR_FAILURE;

    JSRuntime *rt;
    rtsvc->GetRuntime(&rt);
    NS_ASSERTION(rt != nsnull, "no JSRuntime?!");

    JSSecurityCallbacks *callbacks = JS_GetRuntimeSecurityCallbacks(rt);
    NS_ASSERTION(callbacks, "Need a callbacks struct by now!");

    NS_ASSERTION(!callbacks->principalsTranscoder,
                 "oops, JS_SetPrincipalsTranscoder wars!");

    callbacks->principalsTranscoder = nsTranscodeJSPrincipals;
    return NS_OK;
}

nsJSPrincipals::nsJSPrincipals()
{
    codebase = nsnull;
    getPrincipalArray = nsGetPrincipalArray;
    globalPrivilegesEnabled = nsGlobalPrivilegesEnabled;
    refcount = 0;
    destroy = nsDestroyJSPrincipals;
    subsume = nsJSPrincipalsSubsume;
    nsIPrincipalPtr = nsnull;
}

nsresult
nsJSPrincipals::Init(nsIPrincipal *aPrincipal, const nsCString& aCodebase)
{
    if (nsIPrincipalPtr) {
        NS_ERROR("Init called twice!");
        return NS_ERROR_UNEXPECTED;
    }

    nsIPrincipalPtr = aPrincipal;
    nsStringBuffer* buf = nsStringBuffer::FromString(aCodebase);
    char* data;
    if (buf) {
        buf->AddRef();
        data = static_cast<char*>(buf->Data());
    } else {
        PRUint32 len = aCodebase.Length();
        buf = nsStringBuffer::Alloc(len + 1); 
        if (!buf) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        data = static_cast<char*>(buf->Data());
        memcpy(data, aCodebase.get(), len);
        data[len] = '\0';
    }
    
    codebase = data;

    return NS_OK;
}

nsJSPrincipals::~nsJSPrincipals()
{
    if (codebase) {
        nsStringBuffer::FromData(codebase)->Release();
    }
}
