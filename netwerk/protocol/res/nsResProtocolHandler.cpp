




#include "mozilla/chrome/RegistryMessageUtils.h"

#include "nsResProtocolHandler.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "prenv.h"
#include "prprf.h"
#include "nsXPIDLString.h"
#include "nsIFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsNetUtil.h"
#include "nsURLHelper.h"
#include "nsEscape.h"

#include "mozilla/Omnijar.h"

static NS_DEFINE_CID(kResURLCID, NS_RESURL_CID);

static nsResProtocolHandler *gResHandler = nullptr;

#if defined(PR_LOGGING)











static PRLogModuleInfo *gResLog;
#endif

#define kAPP           NS_LITERAL_CSTRING("app")
#define kGRE           NS_LITERAL_CSTRING("gre")





nsresult
nsResURL::EnsureFile()
{
    nsresult rv;

    NS_ENSURE_TRUE(gResHandler, NS_ERROR_NOT_AVAILABLE);

    nsAutoCString spec;
    rv = gResHandler->ResolveURI(this, spec);
    if (NS_FAILED(rv))
        return rv;

    nsAutoCString scheme;
    rv = net_ExtractURLScheme(spec, nullptr, nullptr, &scheme);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    if (!scheme.Equals(NS_LITERAL_CSTRING("file")))
        return NS_ERROR_NO_INTERFACE;

    rv = net_GetFileFromURLSpec(spec, getter_AddRefs(mFile));
#ifdef DEBUG_bsmedberg
    if (NS_SUCCEEDED(rv)) {
        bool exists = true;
        mFile->Exists(&exists);
        if (!exists) {
            printf("resource %s doesn't exist!\n", spec.get());
        }
    }
#endif

    return rv;
}

 nsStandardURL*
nsResURL::StartClone()
{
    nsResURL *clone = new nsResURL();
    return clone;
}

NS_IMETHODIMP 
nsResURL::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    *aClassIDNoAlloc = kResURLCID;
    return NS_OK;
}





nsResProtocolHandler::nsResProtocolHandler()
    : mSubstitutions(32)
{
#if defined(PR_LOGGING)
    gResLog = PR_NewLogModule("nsResProtocol");
#endif

    NS_ASSERTION(!gResHandler, "res handler already created!");
    gResHandler = this;
}

nsResProtocolHandler::~nsResProtocolHandler()
{
    gResHandler = nullptr;
}

nsresult
nsResProtocolHandler::Init()
{
    nsresult rv;

    mIOService = do_GetIOService(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString appURI, greURI;
    rv = mozilla::Omnijar::GetURIString(mozilla::Omnijar::APP, appURI);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mozilla::Omnijar::GetURIString(mozilla::Omnijar::GRE, greURI);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), appURI.Length() ? appURI : greURI);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetSubstitution(EmptyCString(), uri);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    rv = SetSubstitution(kAPP, uri);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    if (appURI.Length()) { 
        rv = NS_NewURI(getter_AddRefs(uri), greURI);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = SetSubstitution(kGRE, uri);
    NS_ENSURE_SUCCESS(rv, rv);

    
    

    
    

    return rv;
}

static PLDHashOperator
EnumerateSubstitution(const nsACString& aKey,
                      nsIURI* aURI,
                      void* aArg)
{
    nsTArray<ResourceMapping>* resources =
            static_cast<nsTArray<ResourceMapping>*>(aArg);
    SerializedURI uri;
    if (aURI) {
        aURI->GetSpec(uri.spec);
        aURI->GetOriginCharset(uri.charset);
    }

    ResourceMapping resource = {
        nsCString(aKey), uri
    };
    resources->AppendElement(resource);
    return (PLDHashOperator)PL_DHASH_NEXT;
}

void
nsResProtocolHandler::CollectSubstitutions(InfallibleTArray<ResourceMapping>& aResources)
{
    mSubstitutions.EnumerateRead(&EnumerateSubstitution, &aResources);
}





NS_IMPL_ISUPPORTS3(nsResProtocolHandler,
                   nsIResProtocolHandler,
                   nsIProtocolHandler,
                   nsISupportsWeakReference)





NS_IMETHODIMP
nsResProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("resource");
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::GetDefaultPort(int32_t *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::GetProtocolFlags(uint32_t *result)
{
    
    
    *result = URI_STD | URI_IS_UI_RESOURCE | URI_IS_LOCAL_RESOURCE;
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::NewURI(const nsACString &aSpec,
                             const char *aCharset,
                             nsIURI *aBaseURI,
                             nsIURI **result)
{
    nsresult rv;

    nsResURL *resURL = new nsResURL();
    if (!resURL)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(resURL);

    
    
    
    nsAutoCString spec;
    const char *src = aSpec.BeginReading();
    const char *end = aSpec.EndReading();
    const char *last = src;

    spec.SetCapacity(aSpec.Length()+1);
    for ( ; src < end; ++src) {
        if (*src == '%' && (src < end-2) && *(src+1) == '2') {
           char ch = '\0';
           if (*(src+2) == 'f' || *(src+2) == 'F')
             ch = '/';
           else if (*(src+2) == 'e' || *(src+2) == 'E')
             ch = '.';

           if (ch) {
             if (last < src)
               spec.Append(last, src-last);
             spec.Append(ch);
             src += 2;
             last = src+1; 
           }
        }
    }
    if (last < src)
      spec.Append(last, src-last);

    rv = resURL->Init(nsIStandardURL::URLTYPE_STANDARD, -1, spec, aCharset, aBaseURI);
    if (NS_SUCCEEDED(rv))
        rv = CallQueryInterface(resURL, result);
    NS_RELEASE(resURL);
    return rv;
}

NS_IMETHODIMP
nsResProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    NS_ENSURE_ARG_POINTER(uri);
    nsresult rv;
    nsAutoCString spec;

    rv = ResolveURI(uri, spec);
    if (NS_FAILED(rv)) return rv;

    rv = mIOService->NewChannel(spec, nullptr, nullptr, result);
    if (NS_FAILED(rv)) return rv;

    nsLoadFlags loadFlags = 0;
    (*result)->GetLoadFlags(&loadFlags);
    (*result)->SetLoadFlags(loadFlags & ~nsIChannel::LOAD_REPLACE);
    return (*result)->SetOriginalURI(uri);
}

NS_IMETHODIMP 
nsResProtocolHandler::AllowPort(int32_t port, const char *scheme, bool *_retval)
{
    
    *_retval = false;
    return NS_OK;
}





NS_IMETHODIMP
nsResProtocolHandler::SetSubstitution(const nsACString& root, nsIURI *baseURI)
{
    if (!baseURI) {
        mSubstitutions.Remove(root);
        return NS_OK;
    }

    
    nsAutoCString scheme;
    nsresult rv = baseURI->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!scheme.Equals(NS_LITERAL_CSTRING("resource"))) {
        mSubstitutions.Put(root, baseURI);
        return NS_OK;
    }

    
    nsAutoCString newBase;
    rv = ResolveURI(baseURI, newBase);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> newBaseURI;
    rv = mIOService->NewURI(newBase, nullptr, nullptr,
                            getter_AddRefs(newBaseURI));
    NS_ENSURE_SUCCESS(rv, rv);

    mSubstitutions.Put(root, newBaseURI);
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::GetSubstitution(const nsACString& root, nsIURI **result)
{
    NS_ENSURE_ARG_POINTER(result);

    if (mSubstitutions.Get(root, result))
        return NS_OK;

    

    nsAutoCString key;
    key.AssignLiteral("resource:");
    key.Append(root);

    nsCOMPtr<nsIFile> file;
    nsresult rv = NS_GetSpecialDirectory(key.get(), getter_AddRefs(file));
    if (NS_FAILED(rv))
        return NS_ERROR_NOT_AVAILABLE;
        
    rv = mIOService->NewFileURI(file, result);
    if (NS_FAILED(rv))
        return NS_ERROR_NOT_AVAILABLE;

    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::HasSubstitution(const nsACString& root, bool *result)
{
    NS_ENSURE_ARG_POINTER(result);

    *result = mSubstitutions.Get(root, nullptr);
    return NS_OK;
}

NS_IMETHODIMP
nsResProtocolHandler::ResolveURI(nsIURI *uri, nsACString &result)
{
    nsresult rv;

    nsAutoCString host;
    nsAutoCString path;

    rv = uri->GetAsciiHost(host);
    if (NS_FAILED(rv)) return rv;

    rv = uri->GetPath(path);
    if (NS_FAILED(rv)) return rv;

    
    nsAutoCString unescapedPath(path);
    NS_UnescapeURL(unescapedPath);

    
    if (unescapedPath.FindChar(':') != -1)
        return NS_ERROR_MALFORMED_URI;

    if (unescapedPath.FindChar('\\') != -1)
        return NS_ERROR_MALFORMED_URI;

    const char *p = path.get() + 1; 
    NS_ASSERTION(*(p-1) == '/', "Path did not begin with a slash!");

    if (*p == '/')
        return NS_ERROR_MALFORMED_URI;

    nsCOMPtr<nsIURI> baseURI;
    rv = GetSubstitution(host, getter_AddRefs(baseURI));
    if (NS_FAILED(rv)) return rv;

    rv = baseURI->Resolve(nsDependentCString(p, path.Length()-1), result);

#if defined(PR_LOGGING)
    if (PR_LOG_TEST(gResLog, PR_LOG_DEBUG)) {
        nsAutoCString spec;
        uri->GetAsciiSpec(spec);
        PR_LOG(gResLog, PR_LOG_DEBUG,
               ("%s\n -> %s\n", spec.get(), PromiseFlatCString(result).get()));
    }
#endif
    return rv;
}
