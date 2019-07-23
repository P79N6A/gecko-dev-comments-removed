



















































#include "prlink.h"
#include "nsCOMPtr.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsNativeCharsetUtils.h"

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
#include <resolv.h>
#endif







static gss_OID_desc gss_c_nt_hostbased_service = 
    { 10, (void *) "\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x04" };

static const char kNegotiateAuthGssLib[] =
    "network.negotiate-auth.gsslib";
static const char kNegotiateAuthNativeImp[] = 
   "network.negotiate-auth.using-native-gsslib";

static const char *gssFuncStr[] = {
    "gss_display_status", 
    "gss_init_sec_context", 
    "gss_indicate_mechs",
    "gss_release_oid_set",
    "gss_delete_sec_context",
    "gss_import_name",
    "gss_release_buffer",
    "gss_release_name",
    "gss_wrap",
    "gss_unwrap"
};

#define gssFuncItems NS_ARRAY_LENGTH(gssFuncStr)

static PRFuncPtr gssFunPtr[gssFuncItems]; 
static PRBool    gssNativeImp = PR_TRUE;
static PRLibrary* gssLibrary = nsnull;

#define gss_display_status_ptr      ((gss_display_status_type)*gssFunPtr[0])
#define gss_init_sec_context_ptr    ((gss_init_sec_context_type)*gssFunPtr[1])
#define gss_indicate_mechs_ptr      ((gss_indicate_mechs_type)*gssFunPtr[2])
#define gss_release_oid_set_ptr     ((gss_release_oid_set_type)*gssFunPtr[3])
#define gss_delete_sec_context_ptr  ((gss_delete_sec_context_type)*gssFunPtr[4])
#define gss_import_name_ptr         ((gss_import_name_type)*gssFunPtr[5])
#define gss_release_buffer_ptr      ((gss_release_buffer_type)*gssFunPtr[6])
#define gss_release_name_ptr        ((gss_release_name_type)*gssFunPtr[7])
#define gss_wrap_ptr                ((gss_wrap_type)*gssFunPtr[8])
#define gss_unwrap_ptr              ((gss_unwrap_type)*gssFunPtr[9])

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

    PRLibrary *lib = NULL;

    if (!libPath.IsEmpty()) {
        LOG(("Attempting to load user specified library [%s]\n", libPath.get()));
        gssNativeImp = PR_FALSE;
        lib = PR_LoadLibrary(libPath.get());
    }
    else {
#ifdef XP_WIN
        char *libName = PR_GetLibraryName(NULL, "gssapi32");
        if (libName) {
            lib = PR_LoadLibrary("gssapi32");
            PR_FreeLibraryName(libName);
        }
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

        for (size_t i = 0; i < NS_ARRAY_LENGTH(verLibNames) && !lib; ++i) {
            lib = PR_LoadLibrary(verLibNames[i]);
 
            





            if (lib &&
                PR_FindFunctionSymbol(lib, 
                                      "internal_krb5_gss_initialize") &&
                PR_FindFunctionSymbol(lib, "gssd_pname_to_uid")) {
                LOG(("CITI libgssapi found, which calls exit(). Skipping\n"));
                PR_UnloadLibrary(lib);
                lib = NULL;
            }
        }

        for (size_t i = 0; i < NS_ARRAY_LENGTH(libNames) && !lib; ++i) {
            char *libName = PR_GetLibraryName(NULL, libNames[i]);
            if (libName) {
                lib = PR_LoadLibrary(libName);
                PR_FreeLibraryName(libName);

                if (lib &&
                    PR_FindFunctionSymbol(lib, 
                                          "internal_krb5_gss_initialize") &&
                    PR_FindFunctionSymbol(lib, "gssd_pname_to_uid")) {
                    LOG(("CITI libgssapi found, which calls exit(). Skipping\n"));
                    PR_UnloadLibrary(lib);
                    lib = NULL;
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

    for (size_t i = 0; i < gssFuncItems; ++i) {
        gssFunPtr[i] = PR_FindFunctionSymbol(lib, gssFuncStr[i]);
        if (!gssFunPtr[i]) {
            LOG(("Fail to load %s function from gssapi library\n", gssFuncStr[i]));
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

#if defined( PR_LOGGING )



void
LogGssError(OM_uint32 maj_stat, OM_uint32 min_stat, const char *prefix)
{
    OM_uint32 new_stat;
    OM_uint32 msg_ctx = 0;
    gss_buffer_desc status1_string;
    gss_buffer_desc status2_string;
    OM_uint32 ret;
    nsCAutoString errorStr;
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

#else 

#define LogGssError(x,y,z)

#endif 



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

    mComplete = PR_FALSE;

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
    mComplete = PR_FALSE;
}

 void
nsAuthGSSAPI::Shutdown()
{
    if (gssLibrary) {
        PR_UnloadLibrary(gssLibrary);
        gssLibrary = nsnull;
    }
}


NS_IMPL_THREADSAFE_ISUPPORTS1(nsAuthGSSAPI, nsIAuthModule)

NS_IMETHODIMP
nsAuthGSSAPI::Init(const char *serviceName,
                   PRUint32    serviceFlags,
                   const PRUnichar *domain,
                   const PRUnichar *username,
                   const PRUnichar *password)
{
    
    NS_ASSERTION(!domain && !username && !password, "unexpected credentials");

    
    NS_ENSURE_TRUE(serviceName && *serviceName, NS_ERROR_INVALID_ARG);

    LOG(("entering nsAuthGSSAPI::Init()\n"));

    if (!gssLibrary)
       return NS_ERROR_NOT_INITIALIZED;

    mServiceName = serviceName;
    mServiceFlags = serviceFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsAuthGSSAPI::GetNextToken(const void *inToken,
                           PRUint32    inTokenLen,
                           void      **outToken,
                           PRUint32   *outTokenLen)
{
    OM_uint32 major_status, minor_status;
    OM_uint32 req_flags = 0;
    gss_buffer_desc input_token = GSS_C_EMPTY_BUFFER;
    gss_buffer_desc output_token = GSS_C_EMPTY_BUFFER;
    gss_buffer_t  in_token_ptr = GSS_C_NO_BUFFER;
    gss_name_t server;
    nsCAutoString userbuf;
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

    res_ninit(&_res);
    major_status = gss_import_name_ptr(&minor_status,
                                   &input_token,
                                   &gss_c_nt_hostbased_service,
                                   &server);
    input_token.value = NULL;
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
    PRBool doingMailTask = mServiceName.Find("imap@") ||
                           mServiceName.Find("pop@") ||
                           mServiceName.Find("smtp@") ||
                           mServiceName.Find("ldap@");
    
    if (!doingMailTask && (gssNativeImp &&
         (KLCacheHasValidTickets_ptr(NULL, kerberosVersion_V5, &found, NULL, NULL) != klNoErr || !found)))
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
                                            nsnull,
                                            &output_token,
                                            nsnull,
                                            nsnull);

    if (GSS_ERROR(major_status)) {
        LogGssError(major_status, minor_status, "gss_init_sec_context() failed");
        Reset();
        rv = NS_ERROR_FAILURE;
        goto end;
    }
    if (major_status == GSS_S_COMPLETE) {
        
        
        mComplete = PR_TRUE;
    }
    else if (major_status == GSS_S_CONTINUE_NEEDED) {
        
        
        
        
        
    } 
    
    *outTokenLen = output_token.length;
    if (output_token.length != 0)
        *outToken = nsMemory::Clone(output_token.value, output_token.length);
    else
        *outToken = NULL;
    
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
                     PRUint32    inTokenLen,
                     void      **outToken,
                     PRUint32   *outTokenLen)
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
                                  NULL,
                                  NULL);
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
        *outToken = NULL;

    gss_release_buffer_ptr(&minor_status, &output_token);

    return NS_OK;
}
 
NS_IMETHODIMP
nsAuthGSSAPI::Wrap(const void *inToken,
                   PRUint32    inTokenLen,
                   PRBool      confidential,
                   void      **outToken,
                   PRUint32   *outTokenLen)
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
                                NULL,
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

