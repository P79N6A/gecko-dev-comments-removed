














































#include "nsAuthSSPI.h"
#include "nsIServiceManager.h"
#include "nsIDNSService.h"
#include "nsIDNSRecord.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"

#include <windows.h>

#define SEC_SUCCESS(Status) ((Status) >= 0)

#ifndef KERB_WRAP_NO_ENCRYPT
#define KERB_WRAP_NO_ENCRYPT 0x80000001
#endif

#ifndef SECBUFFER_PADDING
#define SECBUFFER_PADDING 9
#endif

#ifndef SECBUFFER_STREAM
#define SECBUFFER_STREAM 10
#endif



static const char *const pTypeName [] = {
    "Kerberos",
    "Negotiate",
    "NTLM"
};

#ifdef DEBUG
#define CASE_(_x) case _x: return # _x;
static const char *MapErrorCode(int rc)
{
    switch (rc) {
    CASE_(SEC_E_OK)
    CASE_(SEC_I_CONTINUE_NEEDED)
    CASE_(SEC_I_COMPLETE_NEEDED)
    CASE_(SEC_I_COMPLETE_AND_CONTINUE)
    CASE_(SEC_E_INCOMPLETE_MESSAGE)
    CASE_(SEC_I_INCOMPLETE_CREDENTIALS)
    CASE_(SEC_E_INVALID_HANDLE)
    CASE_(SEC_E_TARGET_UNKNOWN)
    CASE_(SEC_E_LOGON_DENIED)
    CASE_(SEC_E_INTERNAL_ERROR)
    CASE_(SEC_E_NO_CREDENTIALS)
    CASE_(SEC_E_NO_AUTHENTICATING_AUTHORITY)
    CASE_(SEC_E_INSUFFICIENT_MEMORY)
    CASE_(SEC_E_INVALID_TOKEN)
    }
    return "<unknown>";
}
#else
#define MapErrorCode(_rc) ""
#endif



static HINSTANCE                 sspi_lib; 
static PSecurityFunctionTableW   sspi;

static nsresult
InitSSPI()
{
    PSecurityFunctionTableW (*initFun)(void);

    LOG(("  InitSSPI\n"));

    sspi_lib = LoadLibraryW(L"secur32.dll");
    if (!sspi_lib) {
        sspi_lib = LoadLibraryW(L"security.dll");
        if (!sspi_lib) {
            LOG(("SSPI library not found"));
            return NS_ERROR_UNEXPECTED;
        }
    }

    initFun = (PSecurityFunctionTableW (*)(void))
            GetProcAddress(sspi_lib, "InitSecurityInterfaceW");
    if (!initFun) {
        LOG(("InitSecurityInterfaceW not found"));
        return NS_ERROR_UNEXPECTED;
    }

    sspi = initFun();
    if (!sspi) {
        LOG(("InitSecurityInterfaceW failed"));
        return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}



static nsresult
MakeSN(const char *principal, nsCString &result)
{
    nsresult rv;

    nsCAutoString buf(principal);

    
    
    
    PRInt32 index = buf.FindChar('@');
    if (index == kNotFound)
        return NS_ERROR_UNEXPECTED;
    
    nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    
    
    
    
    nsCOMPtr<nsIDNSRecord> record;
    rv = dns->Resolve(Substring(buf, index + 1),
                      nsIDNSService::RESOLVE_CANONICAL_NAME,
                      getter_AddRefs(record));
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString cname;
    rv = record->GetCanonicalName(cname);
    if (NS_SUCCEEDED(rv)) {
        result = StringHead(buf, index) + NS_LITERAL_CSTRING("/") + cname;
        LOG(("Using SPN of [%s]\n", result.get()));
    }
    return rv;
}



nsAuthSSPI::nsAuthSSPI(pType package)
    : mServiceFlags(REQ_DEFAULT)
    , mMaxTokenLen(0)
    , mPackage(package)
{
    memset(&mCred, 0, sizeof(mCred));
    memset(&mCtxt, 0, sizeof(mCtxt));
}

nsAuthSSPI::~nsAuthSSPI()
{
    Reset();

    if (mCred.dwLower || mCred.dwUpper) {
#ifdef __MINGW32__
        (sspi->FreeCredentialsHandle)(&mCred);
#else
        (sspi->FreeCredentialHandle)(&mCred);
#endif
        memset(&mCred, 0, sizeof(mCred));
    }
}

void
nsAuthSSPI::Reset()
{
    if (mCtxt.dwLower || mCtxt.dwUpper) {
        (sspi->DeleteSecurityContext)(&mCtxt);
        memset(&mCtxt, 0, sizeof(mCtxt));
    }
}

NS_IMPL_ISUPPORTS1(nsAuthSSPI, nsIAuthModule)

NS_IMETHODIMP
nsAuthSSPI::Init(const char *serviceName,
                 PRUint32    serviceFlags,
                 const PRUnichar *domain,
                 const PRUnichar *username,
                 const PRUnichar *password)
{
    LOG(("  nsAuthSSPI::Init\n"));

    
    NS_ASSERTION(!domain && !username && !password, "unexpected credentials");

    
    
    if (mPackage != PACKAGE_TYPE_NTLM)
        NS_ENSURE_TRUE(serviceName && *serviceName, NS_ERROR_INVALID_ARG);

    nsresult rv;

    
    if (!sspi) {
        rv = InitSSPI();
        if (NS_FAILED(rv))
            return rv;
    }
    SEC_WCHAR *package;

    package = (SEC_WCHAR *) pTypeName[(int)mPackage];
    if (mPackage != PACKAGE_TYPE_NTLM)
    {
        rv = MakeSN(serviceName, mServiceName);
        if (NS_FAILED(rv))
            return rv;
        mServiceFlags = serviceFlags;
    }

    SECURITY_STATUS rc;

    PSecPkgInfoW pinfo;
    rc = (sspi->QuerySecurityPackageInfoW)(package, &pinfo);
    if (rc != SEC_E_OK) {
        LOG(("%s package not found\n", package));
        return NS_ERROR_UNEXPECTED;
    }
    mMaxTokenLen = pinfo->cbMaxToken;
    (sspi->FreeContextBuffer)(pinfo);

    TimeStamp useBefore;

    rc = (sspi->AcquireCredentialsHandleW)(NULL,
                                          package,
                                          SECPKG_CRED_OUTBOUND,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &mCred,
                                          &useBefore);
    if (rc != SEC_E_OK)
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

NS_IMETHODIMP
nsAuthSSPI::GetNextToken(const void *inToken,
                         PRUint32    inTokenLen,
                         void      **outToken,
                         PRUint32   *outTokenLen)
{
    SECURITY_STATUS rc;
    TimeStamp ignored;

    DWORD ctxAttr, ctxReq = 0;
    CtxtHandle *ctxIn;
    SecBufferDesc ibd, obd;
    SecBuffer ib, ob;

    LOG(("entering nsAuthSSPI::GetNextToken()\n"));

    if (mServiceFlags & REQ_DELEGATE)
        ctxReq |= ISC_REQ_DELEGATE;
    if (mServiceFlags & REQ_MUTUAL_AUTH)
        ctxReq |= ISC_REQ_MUTUAL_AUTH;

    if (inToken) {
        ib.BufferType = SECBUFFER_TOKEN;
        ib.cbBuffer = inTokenLen;
        ib.pvBuffer = (void *) inToken;
        ibd.ulVersion = SECBUFFER_VERSION;
        ibd.cBuffers = 1;
        ibd.pBuffers = &ib;
        ctxIn = &mCtxt;
    }
    else {
        
        
        
        
        
        if (mCtxt.dwLower || mCtxt.dwUpper) {
            LOG(("Cannot restart authentication sequence!"));
            return NS_ERROR_UNEXPECTED;
        }

        ctxIn = NULL;
    }

    obd.ulVersion = SECBUFFER_VERSION;
    obd.cBuffers = 1;
    obd.pBuffers = &ob;
    ob.BufferType = SECBUFFER_TOKEN;
    ob.cbBuffer = mMaxTokenLen;
    ob.pvBuffer = nsMemory::Alloc(ob.cbBuffer);
    if (!ob.pvBuffer)
        return NS_ERROR_OUT_OF_MEMORY;
    memset(ob.pvBuffer, 0, ob.cbBuffer);
    SEC_WCHAR *sn;
    if (mPackage == PACKAGE_TYPE_NTLM)
        sn = NULL;
    else
        sn = (SEC_WCHAR *) mServiceName.get();

    rc = (sspi->InitializeSecurityContextW)(&mCred,
                                           ctxIn,
                                           sn,
                                           ctxReq,
                                           0,
                                           SECURITY_NATIVE_DREP,
                                           inToken ? &ibd : NULL,
                                           0,
                                           &mCtxt,
                                           &obd,
                                           &ctxAttr,
                                           &ignored);
    if (rc == SEC_I_CONTINUE_NEEDED || rc == SEC_E_OK) {
        if (!ob.cbBuffer) {
            nsMemory::Free(ob.pvBuffer);
            ob.pvBuffer = NULL;
        }
        *outToken = ob.pvBuffer;
        *outTokenLen = ob.cbBuffer;

        if (rc == SEC_E_OK)
            return NS_SUCCESS_AUTH_FINISHED;

        return NS_OK;
    }

    LOG(("InitializeSecurityContext failed [rc=%d:%s]\n", rc, MapErrorCode(rc)));
    Reset();
    nsMemory::Free(ob.pvBuffer);
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsAuthSSPI::Unwrap(const void *inToken,
                   PRUint32    inTokenLen,
                   void      **outToken,
                   PRUint32   *outTokenLen)
{
    SECURITY_STATUS rc;
    SecBufferDesc ibd;
    SecBuffer ib[2];

    ibd.cBuffers = 2;
    ibd.pBuffers = ib;
    ibd.ulVersion = SECBUFFER_VERSION; 

    
    ib[0].BufferType = SECBUFFER_STREAM;
    ib[0].cbBuffer = inTokenLen;
    ib[0].pvBuffer = nsMemory::Alloc(ib[0].cbBuffer);
    if (!ib[0].pvBuffer)
        return NS_ERROR_OUT_OF_MEMORY;
    
    memcpy(ib[0].pvBuffer, inToken, inTokenLen);

    
    ib[1].BufferType = SECBUFFER_DATA;
    ib[1].cbBuffer = 0;
    ib[1].pvBuffer = NULL;

    rc = (sspi->DecryptMessage)(
                                &mCtxt,
                                &ibd,
                                0, 
                                NULL
                                );

    if (SEC_SUCCESS(rc)) {
        *outToken = ib[1].pvBuffer;
        *outTokenLen = ib[1].cbBuffer;
    }
    else
        nsMemory::Free(ib[1].pvBuffer);

    nsMemory::Free(ib[0].pvBuffer);

    if (!SEC_SUCCESS(rc))
        return NS_ERROR_FAILURE;

    return NS_OK;
}


class secBuffers
{
public:

    SecBuffer ib[3];

    secBuffers() { memset(&ib, 0, sizeof(ib)); }

    ~secBuffers() 
    {
        if (ib[0].pvBuffer)
            nsMemory::Free(ib[0].pvBuffer);

        if (ib[1].pvBuffer)
            nsMemory::Free(ib[1].pvBuffer);

        if (ib[2].pvBuffer)
            nsMemory::Free(ib[2].pvBuffer);
    }
};

NS_IMETHODIMP
nsAuthSSPI::Wrap(const void *inToken,
                 PRUint32    inTokenLen,
                 PRBool      confidential,
                 void      **outToken,
                 PRUint32   *outTokenLen)
{
    SECURITY_STATUS rc;

    SecBufferDesc ibd;
    secBuffers bufs;
    SecPkgContext_Sizes sizes;

    rc = (sspi->QueryContextAttributesW)(
         &mCtxt,
         SECPKG_ATTR_SIZES,
         &sizes);

    if (!SEC_SUCCESS(rc))  
        return NS_ERROR_FAILURE;
    
    ibd.cBuffers = 3;
    ibd.pBuffers = bufs.ib;
    ibd.ulVersion = SECBUFFER_VERSION;
    
    
    bufs.ib[0].cbBuffer = sizes.cbSecurityTrailer;
    bufs.ib[0].BufferType = SECBUFFER_TOKEN;
    bufs.ib[0].pvBuffer = nsMemory::Alloc(sizes.cbSecurityTrailer);

    if (!bufs.ib[0].pvBuffer)
        return NS_ERROR_OUT_OF_MEMORY;

    
    bufs.ib[1].BufferType = SECBUFFER_DATA;
    bufs.ib[1].pvBuffer = nsMemory::Alloc(inTokenLen);
    bufs.ib[1].cbBuffer = inTokenLen;
    
    if (!bufs.ib[1].pvBuffer)
        return NS_ERROR_OUT_OF_MEMORY;

    memcpy(bufs.ib[1].pvBuffer, inToken, inTokenLen);

    
    bufs.ib[2].BufferType = SECBUFFER_PADDING;
    bufs.ib[2].cbBuffer = sizes.cbBlockSize;
    bufs.ib[2].pvBuffer = nsMemory::Alloc(bufs.ib[2].cbBuffer);

    if (!bufs.ib[2].pvBuffer)
        return NS_ERROR_OUT_OF_MEMORY;

    rc = (sspi->EncryptMessage)(&mCtxt,
          confidential ? 0 : KERB_WRAP_NO_ENCRYPT,
         &ibd, 0);

    if (SEC_SUCCESS(rc)) {
        int len  = bufs.ib[0].cbBuffer + bufs.ib[1].cbBuffer + bufs.ib[2].cbBuffer;
        char *p = (char *) nsMemory::Alloc(len);

        if (!p)
            return NS_ERROR_OUT_OF_MEMORY;
				
        *outToken = (void *) p;
        *outTokenLen = len;

        memcpy(p, bufs.ib[0].pvBuffer, bufs.ib[0].cbBuffer);
        p += bufs.ib[0].cbBuffer;

        memcpy(p,bufs.ib[1].pvBuffer, bufs.ib[1].cbBuffer);
        p += bufs.ib[1].cbBuffer;

        memcpy(p,bufs.ib[2].pvBuffer, bufs.ib[2].cbBuffer);
        
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}
