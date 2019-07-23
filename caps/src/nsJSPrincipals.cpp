




































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

JS_STATIC_DLL_CALLBACK(void *)
nsGetPrincipalArray(JSContext *cx, JSPrincipals *prin)
{
    return nsnull;
}

JS_STATIC_DLL_CALLBACK(JSBool)
nsGlobalPrivilegesEnabled(JSContext *cx, JSPrincipals *jsprin)
{
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
nsJSPrincipalsSubsume(JSPrincipals *jsprin, JSPrincipals *other)
{
    nsJSPrincipals *nsjsprin = static_cast<nsJSPrincipals *>(jsprin);
    nsJSPrincipals *nsother  = static_cast<nsJSPrincipals *>(other);

    JSBool result;
    nsresult rv = nsjsprin->nsIPrincipalPtr->Subsumes(nsother->nsIPrincipalPtr,
                                                      &result);
    return NS_SUCCEEDED(rv) && result;
}

JS_STATIC_DLL_CALLBACK(void)
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
    if (nsjsprin->nsIPrincipalPtr)
        nsjsprin->nsIPrincipalPtr->Release();
    
    
}

JS_STATIC_DLL_CALLBACK(JSBool)
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

                rv = stream->WriteObject(nsjsprin->nsIPrincipalPtr, PR_TRUE);
            }
        }
    } else {
        NS_ASSERTION(JS_XDRMemDataLeft(xdr) == 0, "XDR out of sync?!");
        nsIObjectInputStream *stream =
            reinterpret_cast<nsIObjectInputStream*>(xdr->userdata);

        nsCOMPtr<nsIPrincipal> prin;
        rv = stream->ReadObject(PR_TRUE, getter_AddRefs(prin));
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
    static const char rtsvc_id[] = "@mozilla.org/js/xpc/RuntimeService;1";
    nsCOMPtr<nsIJSRuntimeService> rtsvc(do_GetService(rtsvc_id));
    if (!rtsvc)
        return NS_ERROR_FAILURE;

    JSRuntime *rt;
    rtsvc->GetRuntime(&rt);
    NS_ASSERTION(rt != nsnull, "no JSRuntime?!");

    JSPrincipalsTranscoder oldpx;
    oldpx = ::JS_SetPrincipalsTranscoder(rt, nsTranscodeJSPrincipals);
    NS_ASSERTION(oldpx == nsnull, "oops, JS_SetPrincipalsTranscoder wars!");

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
nsJSPrincipals::Init(nsIPrincipal *aPrincipal, const char *aCodebase)
{
    if (nsIPrincipalPtr) {
        NS_ERROR("Init called twice!");
        return NS_ERROR_UNEXPECTED;
    }

    nsIPrincipalPtr = aPrincipal;
    codebase = PL_strdup(aCodebase);
    if (!codebase)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

nsJSPrincipals::~nsJSPrincipals()
{
    if (codebase)
        PL_strfree(codebase);
}
