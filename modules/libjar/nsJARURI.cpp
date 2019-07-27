





#include "base/basictypes.h"

#include "nsJARURI.h"
#include "nsNetUtil.h"
#include "nsIIOService.h"
#include "nsIStandardURL.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIZipReader.h"
#include "nsReadableUtils.h"
#include "nsAutoPtr.h"
#include "nsNetCID.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "mozilla/ipc/URIUtils.h"

using namespace mozilla::ipc;

static NS_DEFINE_CID(kJARURICID, NS_JARURI_CID);


 
nsJARURI::nsJARURI()
{
}
 
nsJARURI::~nsJARURI()
{
}


NS_IMPL_ADDREF(nsJARURI)
NS_IMPL_RELEASE(nsJARURI)
NS_INTERFACE_MAP_BEGIN(nsJARURI)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIJARURI)
  NS_INTERFACE_MAP_ENTRY(nsIURI)
  NS_INTERFACE_MAP_ENTRY(nsIURL)
  NS_INTERFACE_MAP_ENTRY(nsIJARURI)
  NS_INTERFACE_MAP_ENTRY(nsISerializable)
  NS_INTERFACE_MAP_ENTRY(nsIClassInfo)
  NS_INTERFACE_MAP_ENTRY(nsINestedURI)
  NS_INTERFACE_MAP_ENTRY(nsIIPCSerializableURI)
  
  if (aIID.Equals(NS_GET_IID(nsJARURI)))
      foundInterface = reinterpret_cast<nsISupports*>(this);
  else
NS_INTERFACE_MAP_END

nsresult
nsJARURI::Init(const char *charsetHint)
{
    mCharsetHint = charsetHint;
    return NS_OK;
}

#define NS_JAR_SCHEME           NS_LITERAL_CSTRING("jar:")
#define NS_JAR_DELIMITER        NS_LITERAL_CSTRING("!/")
#define NS_BOGUS_ENTRY_SCHEME   NS_LITERAL_CSTRING("x:///")



nsresult
nsJARURI::FormatSpec(const nsACString &entrySpec, nsACString &result,
                     bool aIncludeScheme)
{
    
    NS_ASSERTION(StringBeginsWith(entrySpec, NS_BOGUS_ENTRY_SCHEME),
                 "bogus entry spec");

    nsAutoCString fileSpec;
    nsresult rv = mJARFile->GetSpec(fileSpec);
    if (NS_FAILED(rv)) return rv;

    if (aIncludeScheme)
        result = NS_JAR_SCHEME;
    else
        result.Truncate();

    result.Append(fileSpec + NS_JAR_DELIMITER +
                  Substring(entrySpec, 5, entrySpec.Length() - 5));
    return NS_OK;
}

nsresult
nsJARURI::CreateEntryURL(const nsACString& entryFilename,
                         const char* charset,
                         nsIURL** url)
{
    *url = nullptr;

    nsCOMPtr<nsIStandardURL> stdURL(do_CreateInstance(NS_STANDARDURL_CONTRACTID));
    if (!stdURL) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    nsAutoCString spec(NS_BOGUS_ENTRY_SCHEME + entryFilename);
    nsresult rv = stdURL->Init(nsIStandardURL::URLTYPE_NO_AUTHORITY, -1,
                               spec, charset, nullptr);
    if (NS_FAILED(rv)) {
        return rv;
    }

    return CallQueryInterface(stdURL, url);
}
    



NS_IMETHODIMP
nsJARURI::Read(nsIObjectInputStream* aInputStream)
{
    nsresult rv;

    nsCOMPtr<nsISupports> supports;
    rv = aInputStream->ReadObject(true, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    mJARFile = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aInputStream->ReadObject(true, getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);

    mJAREntry = do_QueryInterface(supports);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aInputStream->ReadCString(mCharsetHint);
    return rv;
}

NS_IMETHODIMP
nsJARURI::Write(nsIObjectOutputStream* aOutputStream)
{
    nsresult rv;
    
    rv = aOutputStream->WriteCompoundObject(mJARFile, NS_GET_IID(nsIURI),
                                            true);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aOutputStream->WriteCompoundObject(mJAREntry, NS_GET_IID(nsIURL),
                                            true);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aOutputStream->WriteStringZ(mCharsetHint.get());
    return rv;
}




NS_IMETHODIMP 
nsJARURI::GetInterfaces(uint32_t *count, nsIID * **array)
{
    *count = 0;
    *array = nullptr;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARURI::GetScriptableHelper(nsIXPCScriptable **_retval)
{
    *_retval = nullptr;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARURI::GetContractID(char * *aContractID)
{
    *aContractID = nullptr;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARURI::GetClassDescription(char * *aClassDescription)
{
    *aClassDescription = nullptr;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARURI::GetClassID(nsCID * *aClassID)
{
    *aClassID = (nsCID*) moz_xmalloc(sizeof(nsCID));
    if (!*aClassID)
        return NS_ERROR_OUT_OF_MEMORY;
    return GetClassIDNoAlloc(*aClassID);
}

NS_IMETHODIMP 
nsJARURI::GetFlags(uint32_t *aFlags)
{
    
    *aFlags = nsIClassInfo::MAIN_THREAD_ONLY;
    return NS_OK;
}

NS_IMETHODIMP 
nsJARURI::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    *aClassIDNoAlloc = kJARURICID;
    return NS_OK;
}




NS_IMETHODIMP
nsJARURI::GetSpec(nsACString &aSpec)
{
    nsAutoCString entrySpec;
    mJAREntry->GetSpec(entrySpec);
    return FormatSpec(entrySpec, aSpec);
}

NS_IMETHODIMP
nsJARURI::GetSpecIgnoringRef(nsACString &aSpec)
{
    nsAutoCString entrySpec;
    mJAREntry->GetSpecIgnoringRef(entrySpec);
    return FormatSpec(entrySpec, aSpec);
}

NS_IMETHODIMP
nsJARURI::GetHasRef(bool *result)
{
    return mJAREntry->GetHasRef(result);
}

NS_IMETHODIMP
nsJARURI::SetSpec(const nsACString& aSpec)
{
    return SetSpecWithBase(aSpec, nullptr);
}

nsresult
nsJARURI::SetSpecWithBase(const nsACString &aSpec, nsIURI* aBaseURL)
{
    nsresult rv;

    nsCOMPtr<nsIIOService> ioServ(do_GetIOService(&rv));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString scheme;
    rv = ioServ->ExtractScheme(aSpec, scheme);
    if (NS_FAILED(rv)) {
        
        if (!aBaseURL)
            return NS_ERROR_MALFORMED_URI;

        nsRefPtr<nsJARURI> otherJAR;
        aBaseURL->QueryInterface(NS_GET_IID(nsJARURI), getter_AddRefs(otherJAR));
        NS_ENSURE_TRUE(otherJAR, NS_NOINTERFACE);

        mJARFile = otherJAR->mJARFile;

        nsCOMPtr<nsIStandardURL> entry(do_CreateInstance(NS_STANDARDURL_CONTRACTID));
        if (!entry)
            return NS_ERROR_OUT_OF_MEMORY;

        rv = entry->Init(nsIStandardURL::URLTYPE_NO_AUTHORITY, -1,
                         aSpec, mCharsetHint.get(), otherJAR->mJAREntry);
        if (NS_FAILED(rv))
            return rv;

        mJAREntry = do_QueryInterface(entry);
        if (!mJAREntry)
            return NS_NOINTERFACE;

        return NS_OK;
    }

    NS_ENSURE_TRUE(scheme.EqualsLiteral("jar"), NS_ERROR_MALFORMED_URI);

    nsACString::const_iterator begin, end;
    aSpec.BeginReading(begin);
    aSpec.EndReading(end);

    while (begin != end && *begin != ':')
        ++begin;

    ++begin; 

    
    
    
    
    
    
    

    nsACString::const_iterator delim_begin (begin),
                               delim_end   (end);

    if (!RFindInReadable(NS_JAR_DELIMITER, delim_begin, delim_end))
        return NS_ERROR_MALFORMED_URI;

    rv = ioServ->NewURI(Substring(begin, delim_begin), mCharsetHint.get(),
                        aBaseURL, getter_AddRefs(mJARFile));
    if (NS_FAILED(rv)) return rv;

    NS_TryToSetImmutable(mJARFile);

    
    while (*delim_end == '/')
        ++delim_end;

    return SetJAREntry(Substring(delim_end, end));
}

NS_IMETHODIMP
nsJARURI::GetPrePath(nsACString &prePath)
{
    prePath = NS_JAR_SCHEME;
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::GetScheme(nsACString &aScheme)
{
    aScheme = "jar";
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::SetScheme(const nsACString &aScheme)
{
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetUserPass(nsACString &aUserPass)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetUserPass(const nsACString &aUserPass)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetUsername(nsACString &aUsername)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetUsername(const nsACString &aUsername)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetPassword(nsACString &aPassword)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetPassword(const nsACString &aPassword)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetHostPort(nsACString &aHostPort)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetHostPort(const nsACString &aHostPort)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetHost(nsACString &aHost)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::SetHost(const nsACString &aHost)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetPort(int32_t *aPort)
{
    return NS_ERROR_FAILURE;
}
 
NS_IMETHODIMP
nsJARURI::SetPort(int32_t aPort)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetPath(nsACString &aPath)
{
    nsAutoCString entrySpec;
    mJAREntry->GetSpec(entrySpec);
    return FormatSpec(entrySpec, aPath, false);
}

NS_IMETHODIMP
nsJARURI::SetPath(const nsACString &aPath)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetAsciiSpec(nsACString &aSpec)
{
    
    return GetSpec(aSpec);
}

NS_IMETHODIMP
nsJARURI::GetAsciiHost(nsACString &aHost)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsJARURI::GetOriginCharset(nsACString &aOriginCharset)
{
    aOriginCharset = mCharsetHint;
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::Equals(nsIURI *other, bool *result)
{
    return EqualsInternal(other, eHonorRef, result);
}

NS_IMETHODIMP
nsJARURI::EqualsExceptRef(nsIURI *other, bool *result)
{
    return EqualsInternal(other, eIgnoreRef, result);
}


 nsresult
nsJARURI::EqualsInternal(nsIURI *other,
                         nsJARURI::RefHandlingEnum refHandlingMode,
                         bool *result)
{
    *result = false;

    if (!other)
        return NS_OK;	

    nsRefPtr<nsJARURI> otherJAR;
    other->QueryInterface(NS_GET_IID(nsJARURI), getter_AddRefs(otherJAR));
    if (!otherJAR)
        return NS_OK;   

    bool equal;
    nsresult rv = mJARFile->Equals(otherJAR->mJARFile, &equal);
    if (NS_FAILED(rv) || !equal) {
        return rv;   
    }

    return refHandlingMode == eHonorRef ?
        mJAREntry->Equals(otherJAR->mJAREntry, result) :
        mJAREntry->EqualsExceptRef(otherJAR->mJAREntry, result);
}

NS_IMETHODIMP
nsJARURI::SchemeIs(const char *i_Scheme, bool *o_Equals)
{
    NS_ENSURE_ARG_POINTER(o_Equals);
    if (!i_Scheme) return NS_ERROR_INVALID_ARG;

    if (*i_Scheme == 'j' || *i_Scheme == 'J') {
        *o_Equals = PL_strcasecmp("jar", i_Scheme) ? false : true;
    } else {
        *o_Equals = false;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::Clone(nsIURI **result)
{
    nsresult rv;

    nsCOMPtr<nsIJARURI> uri;
    rv = CloneWithJARFileInternal(mJARFile, eHonorRef, getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    uri.forget(result);
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::CloneIgnoringRef(nsIURI **result)
{
    nsresult rv;

    nsCOMPtr<nsIJARURI> uri;
    rv = CloneWithJARFileInternal(mJARFile, eIgnoreRef, getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    uri.forget(result);
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::Resolve(const nsACString &relativePath, nsACString &result)
{
    nsresult rv;

    nsCOMPtr<nsIIOService> ioServ(do_GetIOService(&rv));
    if (NS_FAILED(rv))
      return rv;

    nsAutoCString scheme;
    rv = ioServ->ExtractScheme(relativePath, scheme);
    if (NS_SUCCEEDED(rv)) {
        
        result = relativePath;
        return NS_OK;
    }

    nsAutoCString resolvedPath;
    mJAREntry->Resolve(relativePath, resolvedPath);
    
    return FormatSpec(resolvedPath, result);
}




NS_IMETHODIMP
nsJARURI::GetFilePath(nsACString& filePath)
{
    return mJAREntry->GetFilePath(filePath);
}

NS_IMETHODIMP
nsJARURI::SetFilePath(const nsACString& filePath)
{
    return mJAREntry->SetFilePath(filePath);
}

NS_IMETHODIMP
nsJARURI::GetQuery(nsACString& query)
{
    return mJAREntry->GetQuery(query);
}

NS_IMETHODIMP
nsJARURI::SetQuery(const nsACString& query)
{
    return mJAREntry->SetQuery(query);
}

NS_IMETHODIMP
nsJARURI::GetRef(nsACString& ref)
{
    return mJAREntry->GetRef(ref);
}

NS_IMETHODIMP
nsJARURI::SetRef(const nsACString& ref)
{
    return mJAREntry->SetRef(ref);
}

NS_IMETHODIMP
nsJARURI::GetDirectory(nsACString& directory)
{
    return mJAREntry->GetDirectory(directory);
}

NS_IMETHODIMP
nsJARURI::SetDirectory(const nsACString& directory)
{
    return mJAREntry->SetDirectory(directory);
}

NS_IMETHODIMP
nsJARURI::GetFileName(nsACString& fileName)
{
    return mJAREntry->GetFileName(fileName);
}

NS_IMETHODIMP
nsJARURI::SetFileName(const nsACString& fileName)
{
    return mJAREntry->SetFileName(fileName);
}

NS_IMETHODIMP
nsJARURI::GetFileBaseName(nsACString& fileBaseName)
{
    return mJAREntry->GetFileBaseName(fileBaseName);
}

NS_IMETHODIMP
nsJARURI::SetFileBaseName(const nsACString& fileBaseName)
{
    return mJAREntry->SetFileBaseName(fileBaseName);
}

NS_IMETHODIMP
nsJARURI::GetFileExtension(nsACString& fileExtension)
{
    return mJAREntry->GetFileExtension(fileExtension);
}

NS_IMETHODIMP
nsJARURI::SetFileExtension(const nsACString& fileExtension)
{
    return mJAREntry->SetFileExtension(fileExtension);
}

NS_IMETHODIMP
nsJARURI::GetCommonBaseSpec(nsIURI* uriToCompare, nsACString& commonSpec)
{
    commonSpec.Truncate();

    NS_ENSURE_ARG_POINTER(uriToCompare);
    
    commonSpec.Truncate();
    nsCOMPtr<nsIJARURI> otherJARURI(do_QueryInterface(uriToCompare));
    if (!otherJARURI) {
        
        return NS_OK;
    }

    nsCOMPtr<nsIURI> otherJARFile;
    nsresult rv = otherJARURI->GetJARFile(getter_AddRefs(otherJARFile));
    if (NS_FAILED(rv)) return rv;

    bool equal;
    rv = mJARFile->Equals(otherJARFile, &equal);
    if (NS_FAILED(rv)) return rv;

    if (!equal) {
        
        nsCOMPtr<nsIURL> ourJARFileURL(do_QueryInterface(mJARFile));
        if (!ourJARFileURL) {
            
            return NS_OK;
        }
        nsAutoCString common;
        rv = ourJARFileURL->GetCommonBaseSpec(otherJARFile, common);
        if (NS_FAILED(rv)) return rv;

        commonSpec = NS_JAR_SCHEME + common;
        return NS_OK;
        
    }
    
    
    nsAutoCString otherEntry;
    rv = otherJARURI->GetJAREntry(otherEntry);
    if (NS_FAILED(rv)) return rv;

    nsAutoCString otherCharset;
    rv = uriToCompare->GetOriginCharset(otherCharset);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> url;
    rv = CreateEntryURL(otherEntry, otherCharset.get(), getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsAutoCString common;
    rv = mJAREntry->GetCommonBaseSpec(url, common);
    if (NS_FAILED(rv)) return rv;

    rv = FormatSpec(common, commonSpec);
    return rv;
}

NS_IMETHODIMP
nsJARURI::GetRelativeSpec(nsIURI* uriToCompare, nsACString& relativeSpec)
{
    GetSpec(relativeSpec);

    NS_ENSURE_ARG_POINTER(uriToCompare);

    nsCOMPtr<nsIJARURI> otherJARURI(do_QueryInterface(uriToCompare));
    if (!otherJARURI) {
        
        return NS_OK;
    }

    nsCOMPtr<nsIURI> otherJARFile;
    nsresult rv = otherJARURI->GetJARFile(getter_AddRefs(otherJARFile));
    if (NS_FAILED(rv)) return rv;

    bool equal;
    rv = mJARFile->Equals(otherJARFile, &equal);
    if (NS_FAILED(rv)) return rv;

    if (!equal) {
        
        return rv;
    }

    
    nsAutoCString otherEntry;
    rv = otherJARURI->GetJAREntry(otherEntry);
    if (NS_FAILED(rv)) return rv;

    nsAutoCString otherCharset;
    rv = uriToCompare->GetOriginCharset(otherCharset);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> url;
    rv = CreateEntryURL(otherEntry, otherCharset.get(), getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;

    nsAutoCString relativeEntrySpec;
    rv = mJAREntry->GetRelativeSpec(url, relativeEntrySpec);
    if (NS_FAILED(rv)) return rv;

    if (!StringBeginsWith(relativeEntrySpec, NS_BOGUS_ENTRY_SCHEME)) {
        
        relativeSpec = relativeEntrySpec;
    }
    return rv;
}




NS_IMETHODIMP
nsJARURI::GetJARFile(nsIURI* *jarFile)
{
    return GetInnerURI(jarFile);
}

NS_IMETHODIMP
nsJARURI::GetJAREntry(nsACString &entryPath)
{
    nsAutoCString filePath;
    mJAREntry->GetFilePath(filePath);
    NS_ASSERTION(filePath.Length() > 0, "path should never be empty!");
    
    entryPath = Substring(filePath, 1, filePath.Length() - 1);
    return NS_OK;
}

NS_IMETHODIMP
nsJARURI::SetJAREntry(const nsACString &entryPath)
{
    return CreateEntryURL(entryPath, mCharsetHint.get(),
                          getter_AddRefs(mJAREntry));
}

NS_IMETHODIMP
nsJARURI::CloneWithJARFile(nsIURI *jarFile, nsIJARURI **result)
{
    return CloneWithJARFileInternal(jarFile, eHonorRef, result);
}

nsresult
nsJARURI::CloneWithJARFileInternal(nsIURI *jarFile,
                                   nsJARURI::RefHandlingEnum refHandlingMode,
                                   nsIJARURI **result)
{
    if (!jarFile) {
        return NS_ERROR_INVALID_ARG;
    }

    nsresult rv;

    nsCOMPtr<nsIURI> newJARFile;
    rv = jarFile->Clone(getter_AddRefs(newJARFile));
    if (NS_FAILED(rv)) return rv;

    NS_TryToSetImmutable(newJARFile);

    nsCOMPtr<nsIURI> newJAREntryURI;
    rv = refHandlingMode == eHonorRef ?
        mJAREntry->Clone(getter_AddRefs(newJAREntryURI)) :
        mJAREntry->CloneIgnoringRef(getter_AddRefs(newJAREntryURI));

    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIURL> newJAREntry(do_QueryInterface(newJAREntryURI));
    NS_ASSERTION(newJAREntry, "This had better QI to nsIURL!");
    
    nsJARURI* uri = new nsJARURI();
    NS_ADDREF(uri);
    uri->mJARFile = newJARFile;
    uri->mJAREntry = newJAREntry;
    *result = uri;

    return NS_OK;
}



NS_IMETHODIMP
nsJARURI::GetInnerURI(nsIURI **uri)
{
    return NS_EnsureSafeToReturn(mJARFile, uri);
}

NS_IMETHODIMP
nsJARURI::GetInnermostURI(nsIURI** uri)
{
    return NS_ImplGetInnermostURI(this, uri);
}




void
nsJARURI::Serialize(URIParams& aParams)
{
    JARURIParams params;

    SerializeURI(mJARFile, params.jarFile());
    SerializeURI(mJAREntry, params.jarEntry());
    params.charset() = mCharsetHint;

    aParams = params;
}

bool
nsJARURI::Deserialize(const URIParams& aParams)
{
    if (aParams.type() != URIParams::TJARURIParams) {
        NS_ERROR("Received unknown parameters from the other process!");
        return false;
    }

    const JARURIParams& params = aParams.get_JARURIParams();

    nsCOMPtr<nsIURI> file = DeserializeURI(params.jarFile());
    if (!file) {
        NS_ERROR("Couldn't deserialize jar file URI!");
        return false;
    }

    nsCOMPtr<nsIURI> entry = DeserializeURI(params.jarEntry());
    if (!entry) {
        NS_ERROR("Couldn't deserialize jar entry URI!");
        return false;
    }

    nsCOMPtr<nsIURL> entryURL = do_QueryInterface(entry);
    if (!entryURL) {
        NS_ERROR("Couldn't QI jar entry URI to nsIURL!");
        return false;
    }

    mJARFile.swap(file);
    mJAREntry.swap(entryURL);
    mCharsetHint = params.charset();

    return true;
}
