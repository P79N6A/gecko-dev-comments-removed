







































#include "IPCMessageUtils.h"

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




nsSimpleURI::nsSimpleURI()
    : mMutable(PR_TRUE),
      mIsRefValid(PR_FALSE)
{
}

nsSimpleURI::~nsSimpleURI()
{
}

NS_IMPL_ADDREF(nsSimpleURI)
NS_IMPL_RELEASE(nsSimpleURI)
NS_INTERFACE_TABLE_HEAD(nsSimpleURI)
NS_INTERFACE_TABLE5(nsSimpleURI, nsIURI, nsISerializable, nsIIPCSerializable, nsIClassInfo, nsIMutable)
NS_INTERFACE_TABLE_TO_MAP_SEGUE
  if (aIID.Equals(kThisSimpleURIImplementationCID))
    foundInterface = static_cast<nsIURI*>(this);
  else
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSimpleURI::Read(nsIObjectInputStream* aStream)
{
    nsresult rv;

    PRBool isMutable; 
    rv = aStream->ReadBoolean(&isMutable);
    if (NS_FAILED(rv)) return rv;
    if (isMutable != PR_TRUE && isMutable != PR_FALSE) {
        NS_WARNING("Unexpected boolean value");
        return NS_ERROR_UNEXPECTED;
    }
    mMutable = isMutable;

    rv = aStream->ReadCString(mScheme);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->ReadCString(mPath);
    if (NS_FAILED(rv)) return rv;

    PRBool isRefValid;
    rv = aStream->ReadBoolean(&isRefValid);
    if (NS_FAILED(rv)) return rv;
    if (isRefValid != PR_TRUE && isRefValid != PR_FALSE) {
        NS_WARNING("Unexpected boolean value");
        return NS_ERROR_UNEXPECTED;
    }
    mIsRefValid = isRefValid;

    if (isRefValid) {
        rv = aStream->ReadCString(mRef);
        if (NS_FAILED(rv)) return rv;
    } else {
        mRef.Truncate(); 
    }

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

    rv = aStream->WriteBoolean(mIsRefValid);
    if (NS_FAILED(rv)) return rv;

    if (mIsRefValid) {
        rv = aStream->WriteStringZ(mRef.get());
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}




PRBool
nsSimpleURI::Read(const IPC::Message *aMsg, void **aIter)
{
    bool isMutable, isRefValid;
    if (!ReadParam(aMsg, aIter, &isMutable) ||
        !ReadParam(aMsg, aIter, &mScheme) ||
        !ReadParam(aMsg, aIter, &mPath) ||
        !ReadParam(aMsg, aIter, &isRefValid))
        return PR_FALSE;

    mMutable = isMutable;
    mIsRefValid = isRefValid;

    if (mIsRefValid) {
        return ReadParam(aMsg, aIter, &mRef);
    }
    mRef.Truncate(); 

    return PR_TRUE;
}

void
nsSimpleURI::Write(IPC::Message *aMsg)
{
    WriteParam(aMsg, bool(mMutable));
    WriteParam(aMsg, mScheme);
    WriteParam(aMsg, mPath);
    WriteParam(aMsg, mIsRefValid);
    if (mIsRefValid) {
        WriteParam(aMsg, mRef);
    }
}




NS_IMETHODIMP
nsSimpleURI::GetSpec(nsACString &result)
{
    result = mScheme + NS_LITERAL_CSTRING(":") + mPath;
    if (mIsRefValid) {
        result += NS_LITERAL_CSTRING("#") + mRef;
    } else {
        NS_ABORT_IF_FALSE(mRef.IsEmpty(), "mIsRefValid/mRef invariant broken");
    }
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

    PRInt32 colonPos = spec.FindChar(':');
    if (colonPos < 0 || !net_IsValidScheme(spec.get(), colonPos))
        return NS_ERROR_MALFORMED_URI;

    mScheme.Truncate();
    PRInt32 n = spec.Left(mScheme, colonPos);
    NS_ASSERTION(n == colonPos, "Left failed");
    ToLowerCase(mScheme);

    
    return SetPath(Substring(spec, colonPos + 1));
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
    if (mIsRefValid) {
        result += NS_LITERAL_CSTRING("#") + mRef;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::SetPath(const nsACString &path)
{
    NS_ENSURE_STATE(mMutable);
    
    PRInt32 hashPos = path.FindChar('#');
    if (hashPos < 0) {
        mIsRefValid = PR_FALSE;
        mRef.Truncate(); 
        mPath = path;
        return NS_OK;
    }

    mPath = StringHead(path, hashPos);
    return SetRef(Substring(path, PRUint32(hashPos)));
}

NS_IMETHODIMP
nsSimpleURI::GetRef(nsACString &result)
{
    if (!mIsRefValid) {
      NS_ABORT_IF_FALSE(mRef.IsEmpty(), "mIsRefValid/mRef invariant broken");
      result.Truncate();
    } else {
      result = mRef;
    }

    return NS_OK;
}



NS_IMETHODIMP
nsSimpleURI::SetRef(const nsACString &aRef)
{
    NS_ENSURE_STATE(mMutable);

    if (aRef.IsEmpty()) {
      
      mIsRefValid = PR_FALSE;
      mRef.Truncate(); 
      return NS_OK;
    }

    mIsRefValid = PR_TRUE;

    
    if (aRef[0] == '#') {
        mRef = Substring(aRef, 1);
    } else {
        mRef = aRef;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsSimpleURI::Equals(nsIURI* other, PRBool *result)
{
    return EqualsInternal(other, eHonorRef, result);
}

NS_IMETHODIMP
nsSimpleURI::EqualsExceptRef(nsIURI* other, PRBool *result)
{
    return EqualsInternal(other, eIgnoreRef, result);
}

 nsresult
nsSimpleURI::EqualsInternal(nsIURI* other,
                            nsSimpleURI::RefHandlingEnum refHandlingMode,
                            PRBool* result)
{
    NS_ENSURE_ARG_POINTER(other);
    NS_PRECONDITION(result, "null pointer");

    nsRefPtr<nsSimpleURI> otherUri;
    nsresult rv = other->QueryInterface(kThisSimpleURIImplementationCID,
                                        getter_AddRefs(otherUri));
    if (NS_FAILED(rv)) {
        *result = PR_FALSE;
        return NS_OK;
    }

    *result = (mScheme == otherUri->mScheme &&
               mPath   == otherUri->mPath);

    if (*result && refHandlingMode == eHonorRef) {
        *result = (mIsRefValid == otherUri->mIsRefValid &&
                   (!mIsRefValid || mRef == otherUri->mRef));
    }

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
nsSimpleURI::StartClone(nsSimpleURI::RefHandlingEnum )
{
    return new nsSimpleURI();
}

NS_IMETHODIMP
nsSimpleURI::Clone(nsIURI** result)
{
    return CloneInternal(eHonorRef, result);
}

NS_IMETHODIMP
nsSimpleURI::CloneIgnoringRef(nsIURI** result)
{
    return CloneInternal(eIgnoreRef, result);
}

nsresult
nsSimpleURI::CloneInternal(nsSimpleURI::RefHandlingEnum refHandlingMode,
                           nsIURI** result)
{
    nsRefPtr<nsSimpleURI> url = StartClone(refHandlingMode);
    if (!url)
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    url->mScheme = mScheme;
    url->mPath = mPath;
    if (refHandlingMode == eHonorRef) {
        url->mRef = mRef;
        url->mIsRefValid = mIsRefValid;
    }

    url.forget(result);
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
