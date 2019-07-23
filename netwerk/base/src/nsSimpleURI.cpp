







































#include "nsSimpleURI.h"
#include "nscore.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "prmem.h"
#include "prprf.h"
#include "nsURLHelper.h"
#include "nsNetCID.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsEscape.h"
#include "nsNetError.h"
#include "nsIProgrammingLanguage.h"

static NS_DEFINE_CID(kThisSimpleURIImplementationCID,
                     NS_THIS_SIMPLEURI_IMPLEMENTATION_CID);
static NS_DEFINE_CID(kSimpleURICID, NS_SIMPLEURI_CID);




nsSimpleURI::nsSimpleURI(nsISupports* outer)
    : mMutable(PR_TRUE)
{
    NS_INIT_AGGREGATED(outer);
}

nsSimpleURI::~nsSimpleURI()
{
}

NS_IMPL_AGGREGATED(nsSimpleURI)

nsresult
nsSimpleURI::AggregatedQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);

    if (aIID.Equals(NS_GET_IID(nsISupports))) {
        *aInstancePtr = InnerObject();
    } else if (aIID.Equals(kThisSimpleURIImplementationCID) || 
               aIID.Equals(NS_GET_IID(nsIURI))) {
        *aInstancePtr = static_cast<nsIURI*>(this);
    } else if (aIID.Equals(NS_GET_IID(nsISerializable))) {
        *aInstancePtr = static_cast<nsISerializable*>(this);
    } else if (aIID.Equals(NS_GET_IID(nsIClassInfo))) {
        *aInstancePtr = static_cast<nsIClassInfo*>(this);
    } else if (aIID.Equals(NS_GET_IID(nsIMutable))) {
        *aInstancePtr = static_cast<nsIMutable*>(this);
    } else {
        *aInstancePtr = nsnull;
        return NS_NOINTERFACE;
    }
    NS_ADDREF((nsISupports*)*aInstancePtr);
    return NS_OK;
}




NS_IMETHODIMP
nsSimpleURI::Read(nsIObjectInputStream* aStream)
{
    nsresult rv;

    rv = aStream->ReadBoolean(&mMutable);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->ReadCString(mScheme);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->ReadCString(mPath);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::Write(nsIObjectOutputStream* aStream)
{
    nsresult rv;

    rv = aStream->WriteBoolean(mMutable);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->WriteStringZ(mScheme.get());
    if (NS_FAILED(rv)) return rv;

    rv = aStream->WriteStringZ(mPath.get());
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}




NS_IMETHODIMP
nsSimpleURI::GetSpec(nsACString &result)
{
    result = mScheme + NS_LITERAL_CSTRING(":") + mPath;
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::SetSpec(const nsACString &aSpec)
{
    NS_ENSURE_STATE(mMutable);
    
    const nsAFlatCString& flat = PromiseFlatCString(aSpec);
    const char* specPtr = flat.get();

    
    nsCAutoString filteredSpec;
    PRInt32 specLen;
    if (net_FilterURIString(specPtr, filteredSpec)) {
        specPtr = filteredSpec.get();
        specLen = filteredSpec.Length();
    } else
        specLen = flat.Length();

    
    nsCAutoString spec;
    NS_EscapeURL(specPtr, specLen, esc_OnlyNonASCII|esc_AlwaysCopy, spec);

    PRInt32 pos = spec.FindChar(':');
    if (pos == -1 || !net_IsValidScheme(spec.get(), pos))
        return NS_ERROR_MALFORMED_URI;

    mScheme.Truncate();
    mPath.Truncate();

    PRInt32 n = spec.Left(mScheme, pos);
    NS_ASSERTION(n == pos, "Left failed");

    PRInt32 count = spec.Length() - pos - 1;
    n = spec.Mid(mPath, pos + 1, count);
    NS_ASSERTION(n == count, "Mid failed");

    ToLowerCase(mScheme);
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::GetScheme(nsACString &result)
{
    result = mScheme;
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::SetScheme(const nsACString &scheme)
{
    NS_ENSURE_STATE(mMutable);

    const nsPromiseFlatCString &flat = PromiseFlatCString(scheme);
    if (!net_IsValidScheme(flat)) {
        NS_ERROR("the given url scheme contains invalid characters");
        return NS_ERROR_MALFORMED_URI;
    }

    mScheme = scheme;
    ToLowerCase(mScheme);
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::GetPrePath(nsACString &result)
{
    result = mScheme + NS_LITERAL_CSTRING(":");
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::GetUserPass(nsACString &result)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::SetUserPass(const nsACString &userPass)
{
    NS_ENSURE_STATE(mMutable);
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::GetUsername(nsACString &result)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::SetUsername(const nsACString &userName)
{
    NS_ENSURE_STATE(mMutable);
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::GetPassword(nsACString &result)
{
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::SetPassword(const nsACString &password)
{
    NS_ENSURE_STATE(mMutable);
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::GetHostPort(nsACString &result)
{
    
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::SetHostPort(const nsACString &result)
{
    NS_ENSURE_STATE(mMutable);
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::GetHost(nsACString &result)
{
    
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::SetHost(const nsACString &host)
{
    NS_ENSURE_STATE(mMutable);
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::GetPort(PRInt32 *result)
{
    
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::SetPort(PRInt32 port)
{
    NS_ENSURE_STATE(mMutable);
    
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSimpleURI::GetPath(nsACString &result)
{
    result = mPath;
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::SetPath(const nsACString &path)
{
    NS_ENSURE_STATE(mMutable);
    
    mPath = path;
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::Equals(nsIURI* other, PRBool *result)
{
    PRBool eq = PR_FALSE;
    if (other) {
        nsSimpleURI* otherUrl;
        nsresult rv =
            other->QueryInterface(kThisSimpleURIImplementationCID,
                                  (void**)&otherUrl);
        if (NS_SUCCEEDED(rv)) {
            eq = PRBool((0 == strcmp(mScheme.get(), otherUrl->mScheme.get())) && 
                        (0 == strcmp(mPath.get(), otherUrl->mPath.get())));
            NS_RELEASE(otherUrl);
        }
    }
    *result = eq;
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::SchemeIs(const char *i_Scheme, PRBool *o_Equals)
{
    NS_ENSURE_ARG_POINTER(o_Equals);
    if (!i_Scheme) return NS_ERROR_NULL_POINTER;

    const char *this_scheme = mScheme.get();

    
    if (*i_Scheme == *this_scheme || *i_Scheme == (*this_scheme - ('a' - 'A')) ) {
        *o_Equals = PL_strcasecmp(this_scheme, i_Scheme) ? PR_FALSE : PR_TRUE;
    } else {
        *o_Equals = PR_FALSE;
    }

    return NS_OK;
}

 nsSimpleURI*
nsSimpleURI::StartClone()
{
    return new nsSimpleURI(nsnull);     
}

NS_IMETHODIMP
nsSimpleURI::Clone(nsIURI* *result)
{
    nsSimpleURI* url = StartClone();
    if (url == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    url->mScheme = mScheme;
    url->mPath = mPath;

    *result = url;
    NS_ADDREF(url);
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::Resolve(const nsACString &relativePath, nsACString &result) 
{
    result = relativePath;
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::GetAsciiSpec(nsACString &result)
{
    nsCAutoString buf;
    nsresult rv = GetSpec(buf);
    if (NS_FAILED(rv)) return rv;
    NS_EscapeURL(buf, esc_OnlyNonASCII|esc_AlwaysCopy, result);
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::GetAsciiHost(nsACString &result)
{
    result.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::GetOriginCharset(nsACString &result)
{
    result.Truncate();
    return NS_OK;
}





NS_IMETHODIMP 
nsSimpleURI::GetInterfaces(PRUint32 *count, nsIID * **array)
{
    *count = 0;
    *array = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSimpleURI::GetHelperForLanguage(PRUint32 language, nsISupports **_retval)
{
    *_retval = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSimpleURI::GetContractID(char * *aContractID)
{
    
    
    *aContractID = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSimpleURI::GetClassDescription(char * *aClassDescription)
{
    *aClassDescription = nsnull;
    return NS_OK;
}

NS_IMETHODIMP 
nsSimpleURI::GetClassID(nsCID * *aClassID)
{
    
    
    *aClassID = (nsCID*) nsMemory::Alloc(sizeof(nsCID));
    if (!*aClassID)
        return NS_ERROR_OUT_OF_MEMORY;
    return GetClassIDNoAlloc(*aClassID);
}

NS_IMETHODIMP 
nsSimpleURI::GetImplementationLanguage(PRUint32 *aImplementationLanguage)
{
    *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
    return NS_OK;
}

NS_IMETHODIMP 
nsSimpleURI::GetFlags(PRUint32 *aFlags)
{
    *aFlags = nsIClassInfo::MAIN_THREAD_ONLY;
    return NS_OK;
}

NS_IMETHODIMP 
nsSimpleURI::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    *aClassIDNoAlloc = kSimpleURICID;
    return NS_OK;
}




NS_IMETHODIMP
nsSimpleURI::GetMutable(PRBool *value)
{
    *value = mMutable;
    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::SetMutable(PRBool value)
{
    NS_ENSURE_ARG(mMutable || !value);

    mMutable = value;
    return NS_OK;
}

