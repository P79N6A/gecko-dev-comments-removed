















#include "mozilla/ArrayUtils.h"

#include "prlink.h"
#include "nsCOMPtr.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsNativeCharsetUtils.h"
#include "mozilla/Telemetry.h"

#include "nsAuthGSSAPI.h"

#ifdef XP_MACOSX
#include <Kerberos/Kerberos.h>
#endif

#ifdef XP_MACOSX
typedef KLStatus (*KLCacheHasValidTickets_type)(
    KLPrincipal,
    KLKerberosVersion,
    KLBoolean *,
    KLPrincipal *,
    char **);
#endif

#if defined(HAVE_RES_NINIT)
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#endif

using namespace mozilla;







static gss_OID_desc gss_c_nt_hostbased_service = 
    { 10, (void *) "\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x04" };

static const char kNegotiateAuthGssLib[] =
    "network.negotiate-auth.gsslib";
static const char kNegotiateAuthNativeImp[] = 
   "network.negotiate-auth.using-native-gsslib";

static struct GSSFunction {
    const char *str;
    PRFuncPtr func;
} gssFuncs[] = {
    { "gss_display_status", nullptr },
    { "gss_init_sec_context", nullptr },
    { "gss_indicate_mechs", nullptr },
    { "gss_release_oid_set", nullptr },
    { "gss_delete_sec_context", nullptr },
    { "gss_import_name", nullptr },
    { "gss_release_buffer", nullptr },
    { "gss_release_name", nullptr },
    { "gss_wrap", nullptr },
    { "gss_unwrap", nullptr }
};

static bool      gssNativeImp = true;
static PRLibrary* gssLibrary = nullptr;

#define gss_display_status_ptr      ((gss_display_status_type)*gssFuncs[0].func)
#define gss_init_sec_context_ptr    ((gss_init_sec_context_type)*gssFuncs[1].func)
#define gss_indicate_mechs_ptr      ((gss_indicate_mechs_type)*gssFuncs[2].func)
#define gss_release_oid_set_ptr     ((gss_release_oid_set_type)*gssFuncs[3].func)
#define gss_delete_sec_context_ptr  ((gss_delete_sec_context_type)*gssFuncs[4].func)
#define gss_import_name_ptr         ((gss_import_name_type)*gssFuncs[5].func)
#define gss_release_buffer_ptr      ((gss_release_buffer_type)*gssFuncs[6].func)
#define gss_release_name_ptr        ((gss_release_name_type)*gssFuncs[7].func)
#define gss_wrap_ptr                ((gss_wrap_type)*gssFuncs[8].func)
#define gss_unwrap_ptr              ((gss_unwrap_type)*gssFuncs[9].func)

#ifdef XP_MACOSX
static PRFuncPtr KLCacheHasValidTicketsPtr;
#define KLCacheHasValidTickets_ptr \
        ((KLCacheHasValidTickets_type)*KLCacheHasValidTicketsPtr)
#endif

static nsresult
gssInit()
{
    nsXPIDLCString libPath;
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        prefs->GetCharPref(kNegotiateAuthGssLib, getter_Copies(libPath)); 
        prefs->GetBoolPref(kNegotiateAuthNativeImp, &gssNativeImp); 
    }

    PRLibrary *lib = nullptr;

    if (!libPath.IsEmpty()) {
        LOG(("Attempting to load user specified library [%s]\n", libPath.get()));
        gssNativeImp = false;
        lib = PR_LoadLibrary(libPath.get());
    }
    else {
#ifdef XP_WIN
        char *libName = PR_GetLibraryName(nullptr, "gssapi32");
        if (libName) {
            lib = PR_LoadLibrary("gssapi32");
            PR_FreeLibraryName(libName);
        }
#elif defined(__OpenBSD__)
        





        const char *const verLibNames[] = {
            "libasn1.so",
            "libcrypto.so",
            "libroken.so",
            "libheimbase.so",
            "libcom_err.so",
            "libkrb5.so",
            "libgssapi.so"
        };

        PRLibSpec libSpec;
        for (size_t i = 0; i < ArrayLength(verLibNames); ++i) {
            libSpec.type = PR_LibSpec_Pathname;
            libSpec.value.pathname = verLibNames[i];
            lib = PR_LoadLibraryWithFlags(libSpec, PR_LD_GLOBAL);
        };

#else
        
        const char *const libNames[] = {
            "gss",
            "gssapi_krb5",
            "gssapi"
        };
        
        const char *const verLibNames[] = {
            "libgssapi_krb5.so.2", 
            "libgssapi.so.4",      
            "libgssapi.so.1"       
        };

        for (size_t i = 0; i < ArrayLength(verLibNames) && !lib; ++i) {
            lib = PR_LoadLibrary(verLibNames[i]);
 
            





            if (lib &&
                PR_FindFunctionSymbol(lib, 
                                      "internal_krb5_gss_initialize") &&
                PR_FindFunctionSymbol(lib, "gssd_pname_to_uid")) {
                LOG(("CITI libgssapi found, which calls exit(). Skipping\n"));
                PR_UnloadLibrary(lib);
                lib = nullptr;
            }
        }

        for (size_t i = 0; i < ArrayLength(libNames) && !lib; ++i) {
            char *libName = PR_GetLibraryName(nullptr, libNames[i]);
            if (libName) {
                lib = PR_LoadLibrary(libName);
                PR_FreeLibraryName(libName);

                if (lib &&
                    PR_FindFunctionSymbol(lib, 
                                          "internal_krb5_gss_initialize") &&
                    PR_FindFunctionSymbol(lib, "gssd_pname_to_uid")) {
                    LOG(("CITI libgssapi found, which calls exit(). Skipping\n"));
                    PR_UnloadLibrary(lib);
                    lib = nullptr;
                } 
            }
        }
#endif
    }
    
    if (!lib) {
        LOG(("Fail to load gssapi library\n"));
        return NS_ERROR_FAILURE;
    }

    LOG(("Attempting to load gss functions\n"));

    for (size_t i = 0; i < ArrayLength(gssFuncs); ++i) {
        gssFuncs[i].func = PR_FindFunctionSymbol(lib, gssFuncs[i].str);
        if (!gssFuncs[i].func) {
            LOG(("Fail to load %s function from gssapi library\n", gssFuncs[i].str));
            PR_UnloadLibrary(lib);
            return NS_ERROR_FAILURE;
        }
    }
#ifdef XP_MACOSX
    if (gssNativeImp &&
            !(KLCacheHasValidTicketsPtr =
               PR_FindFunctionSymbol(lib, "KLCacheHasValidTickets"))) {
        LOG(("Fail to load KLCacheHasValidTickets function from gssapi library\n"));
        PR_UnloadLibrary(lib);
        return NS_ERROR_FAILURE;
    }
#endif

    gssLibrary = lib;
    return NS_OK;
}



void
LogGssError(OM_uint32 maj_stat, OM_uint32 min_stat, const char *prefix)
{
    OM_uint32 new_stat;
    OM_uint32 msg_ctx = 0;
    gss_buffer_desc status1_string;
    gss_buffer_desc status2_string;
    OM_uint32 ret;
    nsAutoCString errorStr;
    errorStr.Assign(prefix);

    if (!gssLibrary)
        return;

    errorStr += ": ";
    do {
        ret = gss_display_status_ptr(&new_stat,
                                     maj_stat,
                                     GSS_C_GSS_CODE,
                                     GSS_C_NULL_OID,
                                     &msg_ctx,
                                     &status1_string);
        errorStr.Append((const char *) status1_string.value, status1_string.length);
        gss_release_buffer_ptr(&new_stat, &status1_string);

        errorStr += '\n';
        ret = gss_display_status_ptr(&new_stat,
                                     min_stat,
                                     GSS_C_MECH_CODE,
                                     GSS_C_NULL_OID,
                                     &msg_ctx,
                                     &status2_string);
        errorStr.Append((const char *) status2_string.value, status2_string.length);
        errorStr += '\n';
    } while (!GSS_ERROR(ret) && msg_ctx != 0);

    LOG(("%s\n", errorStr.get()));
}



nsAuthGSSAPI::nsAuthGSSAPI(pType package)
    : mServiceFlags(REQ_DEFAULT)
{
    OM_uint32 minstat;
    OM_uint32 majstat;
    gss_OID_set mech_set;
    gss_OID item;

    unsigned int i;
    static gss_OID_desc gss_krb5_mech_oid_desc =
        { 9, (void *) "\x2a\x86\x48\x86\xf7\x12\x01\x02\x02" };
    static gss_OID_desc gss_spnego_mech_oid_desc =
        { 6, (void *) "\x2b\x06\x01\x05\x05\x02" };

    LOG(("entering nsAuthGSSAPI::nsAuthGSSAPI()\n"));

    mComplete = false;

    if (!gssLibrary && NS_FAILED(gssInit()))
        return;

    mCtx = GSS_C_NO_CONTEXT;
    mMechOID = &gss_krb5_mech_oid_desc;

    
    

    if (package == PACKAGE_TYPE_KERBEROS)
        return;

    
    
    
    
    
    
    
    
    
    
    majstat = gss_indicate_mechs_ptr(&minstat, &mech_set);
    if (GSS_ERROR(majstat))
        return;

    if (mech_set) {
        for (i=0; i<mech_set->count; i++) {
            item = &mech_set->elements[i];    
            if (item->length == gss_spnego_mech_oid_desc.length &&
                !memcmp(item->elements, gss_spnego_mech_oid_desc.elements,
                item->length)) {
                
                mMechOID = &gss_spnego_mech_oid_desc;
                break;
            }
        }
        gss_release_oid_set_ptr(&minstat, &mech_set);
    }
}

void
nsAuthGSSAPI::Reset()
{
    if (gssLibrary && mCtx != GSS_C_NO_CONTEXT) {
        OM_uint32 minor_status;
        gss_delete_sec_context_ptr(&minor_status, &mCtx, GSS_C_NO_BUFFER);
    }
    mCtx = GSS_C_NO_CONTEXT;
    mComplete = false;
}

 void
nsAuthGSSAPI::Shutdown()
{
    if (gssLibrary) {
        PR_UnloadLibrary(gssLibrary);
        gssLibrary = nullptr;
    }
}


NS_IMPL_ISUPPORTS(nsAuthGSSAPI, nsIAuthModule)

NS_IMETHODIMP
nsAuthGSSAPI::Init(const char *serviceName,
                   uint32_t    serviceFlags,
                   const char16_t *domain,
                   const char16_t *username,
                   const char16_t *password)
{
    
    NS_ASSERTION(!domain && !username && !password, "unexpected credentials");

    
    NS_ENSURE_TRUE(serviceName && *serviceName, NS_ERROR_INVALID_ARG);

    LOG(("entering nsAuthGSSAPI::Init()\n"));

    if (!gssLibrary)
       return NS_ERROR_NOT_INITIALIZED;

    mServiceName = serviceName;
    mServiceFlags = serviceFlags;

    static bool sTelemetrySent = false;
    if (!sTelemetrySent) {
        mozilla::Telemetry::Accumulate(
            mozilla::Telemetry::NTLM_MODULE_USED_2,
            serviceFlags & nsIAuthModule::REQ_PROXY_AUTH
                ? NTLM_MODULE_KERBEROS_PROXY
                : NTLM_MODULE_KERBEROS_DIRECT);
        sTelemetrySent = true;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsAuthGSSAPI::GetNextToken(const void *inToken,
                           uint32_t    inTokenLen,
                           void      **outToken,
                           uint32_t   *outTokenLen)
{
    OM_uint32 major_status, minor_status;
    OM_uint32 req_flags = 0;
    gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
    gss_buffer_t  in_token_ptr = GSS_C_NO_BUFFER;
    gss_name_t server;
    nsAutoCString userbuf;
    nsresult rv;

    LOG(("entering nsAuthGSSAPI::GetNextToken()\n"));

    if (!gssLibrary)
       return NS_ERROR_NOT_INITIALIZED;

    
    if (mComplete)
        Reset();
    
    if (mServiceFlags & REQ_DELEGATE)
        req_flags |= GSS_C_DELEG_FLAG;

    if (mServiceFlags & REQ_MUTUAL_AUTH)
        req_flags |= GSS_C_MUTUAL_FLAG;

    input_token.value = (void *)mServiceName.get();
    input_token.length = mServiceName.Length() + 1;

#if defined(HAVE_RES_NINIT)
    res_ninit(&_res);
#endif
    major_status = gss_import_name_ptr(&minor_status,
                                   &input_token,
                                   &gss_c_nt_hostbased_service,
                                   &server);
    input_token.value = nullptr;
    input_token.length = 0;
    if (GSS_ERROR(major_status)) {
        LogGssError(major_status, minor_status, "gss_import_name() failed");
        return NS_ERROR_FAILURE;
    }

    if (inToken) {
        input_token.length = inTokenLen;
        input_token.value = (void *) inToken;
        in_token_ptr = &input_token;
    }
    else if (mCtx != GSS_C_NO_CONTEXT) {
        
        
        
        
        
        LOG(("Cannot restart authentication sequence!"));
        return NS_ERROR_UNEXPECTED; 
    }

#if defined(XP_MACOSX)
    
    
    
    KLBoolean found;    
    bool doingMailTask = mServiceName.Find("imap@") ||
                           mServiceName.Find("pop@") ||
                           mServiceName.Find("smtp@") ||
                           mServiceName.Find("ldap@");
    
    if (!doingMailTask && (gssNativeImp &&
         (KLCacheHasValidTickets_ptr(nullptr, kerberosVersion_V5, &found, nullptr, nullptr) != klNoErr || !found)))
    {
        major_status = GSS_S_FAILURE;
        minor_status = 0;
    }
    else
#endif 
    major_status = gss_init_sec_context_ptr(&minor_status,
                                            GSS_C_NO_CREDENTIAL,
                                            &mCtx,
                                            server,
                                            mMechOID,
                                            req_flags,
                                            GSS_C_INDEFINITE,
                                            GSS_C_NO_CHANNEL_BINDINGS,
                                            in_token_ptr,
                                            nullptr,
                                            &output_token,
                                            nullptr,
                                            nullptr);

    if (GSS_ERROR(major_status)) {
        LogGssError(major_status, minor_status, "gss_init_sec_context() failed");
        Reset();
        rv = NS_ERROR_FAILURE;
        goto end;
    }
    if (major_status == GSS_S_COMPLETE) {
        
        
        mComplete = true;
    }
    else if (major_status == GSS_S_CONTINUE_NEEDED) {
        
        
        
        
        
    } 
    
    *outTokenLen = output_token.length;
    if (output_token.length != 0)
        *outToken = nsMemory::Clone(output_token.value, output_token.length);
    else
        *outToken = nullptr;
    
    gss_release_buffer_ptr(&minor_status, &output_token);

    if (major_status == GSS_S_COMPLETE)
        rv = NS_SUCCESS_AUTH_FINISHED;
    else
        rv = NS_OK;

end:
    gss_release_name_ptr(&minor_status, &server);

    LOG(("  leaving nsAuthGSSAPI::GetNextToken [rv=%x]", rv));
    return rv;
}

NS_IMETHODIMP
nsAuthGSSAPI::Unwrap(const void *inToken,
                     uint32_t    inTokenLen,
                     void      **outToken,
                     uint32_t   *outTokenLen)
{
    OM_uint32 major_status, minor_status;

    gss_buffer_desc input_token;
    gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;

    input_token.value = (void *) inToken;
    input_token.length = inTokenLen;

    major_status = gss_unwrap_ptr(&minor_status,
                                  mCtx,
                                  &input_token,
                                  &output_token,
                                  nullptr,
                                  nullptr);
    if (GSS_ERROR(major_status)) {
        LogGssError(major_status, minor_status, "gss_unwrap() failed");
        Reset();
        gss_release_buffer_ptr(&minor_status, &output_token);
        return NS_ERROR_FAILURE;
    }

    *outTokenLen = output_token.length;

    if (output_token.length)
        *outToken = nsMemory::Clone(output_token.value, output_token.length);
    else
        *outToken = nullptr;

    gss_release_buffer_ptr(&minor_status, &output_token);

    return NS_OK;
}
 
NS_IMETHODIMP
nsAuthGSSAPI::Wrap(const void *inToken,
                   uint32_t    inTokenLen,
                   bool        confidential,
                   void      **outToken,
                   uint32_t   *outTokenLen)
{
    OM_uint32 major_status, minor_status;

    gss_buffer_desc input_token;
    gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;

    input_token.value = (void *) inToken;
    input_token.length = inTokenLen;

    major_status = gss_wrap_ptr(&minor_status,
                                mCtx,
                                confidential,
                                GSS_C_QOP_DEFAULT,
                                &input_token,
                                nullptr,
                                &output_token);
    
    if (GSS_ERROR(major_status)) {
        LogGssError(major_status, minor_status, "gss_wrap() failed");
        Reset();
        gss_release_buffer_ptr(&minor_status, &output_token);
        return NS_ERROR_FAILURE;
    }

    *outTokenLen = output_token.length;

    
    *outToken = nsMemory::Clone(output_token.value, output_token.length);
    gss_release_buffer_ptr(&minor_status, &output_token);

    return NS_OK;
}

