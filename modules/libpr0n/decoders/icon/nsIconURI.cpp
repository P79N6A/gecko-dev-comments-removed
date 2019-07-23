






































#include "nsIconURI.h"
#include "nsNetUtil.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "prprf.h"
#include "plstr.h"
#include <stdlib.h>

#define DEFAULT_IMAGE_SIZE          16



static void extractAttributeValue(const char * searchString, const char * attributeName, nsCString& aResult);
 
static const char *kSizeStrings[] =
{
  "button",
  "toolbar",
  "toolbarsmall",
  "menu",
  "dnd",
  "dialog"
};

static const char *kStateStrings[] =
{
  "normal",
  "disabled"
};


 
nsMozIconURI::nsMozIconURI()
  : mSize(DEFAULT_IMAGE_SIZE),
    mIconSize(-1),
    mIconState(-1)
{
}
 
nsMozIconURI::~nsMozIconURI()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsMozIconURI, nsIMozIconURI, nsIURI)

#define NS_MOZICON_SCHEME           "moz-icon:"
#define NS_MOZ_ICON_DELIMITER        '?'





NS_IMETHODIMP
nsMozIconURI::GetSpec(nsACString &aSpec)
{
  aSpec = NS_MOZICON_SCHEME;

  if (mFileIcon)
  {
    nsCAutoString fileIconSpec;
    nsresult rv = mFileIcon->GetSpec(fileIconSpec);
    NS_ENSURE_SUCCESS(rv, rv);
    aSpec += fileIconSpec;
  }
  else if (!mStockIcon.IsEmpty())
  {
    aSpec += "//stock/";
    aSpec += mStockIcon;
  }
  else
  {
    aSpec += "//";
    aSpec += mDummyFilePath;
  }

  aSpec += "?size=";
  if (mIconSize >= 0)
  {
    aSpec += kSizeStrings[mIconSize];
  }
  else
  {
    char buf[20];
    PR_snprintf(buf, sizeof(buf), "%d", mSize);
    aSpec.Append(buf);
  }

  if (mIconState >= 0) {
    aSpec += "&state=";
    aSpec += kStateStrings[mIconState];
  }

  if (!mContentType.IsEmpty())
  {
    aSpec += "&contentType=";
    aSpec += mContentType.get();
  }
  
  return NS_OK;
}





void extractAttributeValue(const char * searchString, const char * attributeName, nsCString& result)
{
  

  result.Truncate();

  if (searchString && attributeName)
  {
    
    PRUint32 attributeNameSize = strlen(attributeName);
    const char * startOfAttribute = PL_strcasestr(searchString, attributeName);
    if (startOfAttribute &&
       ( *(startOfAttribute-1) == '?' || *(startOfAttribute-1) == '&') )
    {
      startOfAttribute += attributeNameSize; 
      if (*startOfAttribute) 
      {
        const char * endofAttribute = strchr(startOfAttribute, '&');
        if (endofAttribute)
          result.Assign(Substring(startOfAttribute, endofAttribute));
        else
          result.Assign(startOfAttribute);
      } 
    } 
  } 
}

NS_IMETHODIMP
nsMozIconURI::SetSpec(const nsACString &aSpec)
{
  nsresult rv;
  nsCOMPtr<nsIIOService> ioService (do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString scheme;
  rv = ioService->ExtractScheme(aSpec, scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  if (strcmp("moz-icon", scheme.get()) != 0) 
    return NS_ERROR_MALFORMED_URI;

  nsCAutoString sizeString;
  nsCAutoString stateString;
  nsCAutoString mozIconPath(aSpec);

  
  const char *path = strchr(mozIconPath.get(), ':') + 1;
  const char *question = strchr(mozIconPath.get(), NS_MOZ_ICON_DELIMITER);

  if (!question) 
  {
    mDummyFilePath.Assign(path);
  }
  else
  {
    mDummyFilePath.Assign(Substring(path, question));

    
    extractAttributeValue(question, "size=", sizeString);
    extractAttributeValue(question, "state=", stateString);
    extractAttributeValue(question, "contentType=", mContentType);
  }

  if (!sizeString.IsEmpty())
  {
    const char *sizeStr = sizeString.get();
    for (PRInt32 i = 0; i < NS_ARRAY_LENGTH(kSizeStrings); i++)
    {
      if (PL_strcasecmp(sizeStr, kSizeStrings[i]) == 0)
      {
        mIconSize = i;
        break;
      }
    }
  }

  if (!stateString.IsEmpty())
  {
    const char *stateStr = stateString.get();
    for (PRInt32 i = 0; i < NS_ARRAY_LENGTH(kStateStrings); i++)
    {
      if (PL_strcasecmp(stateStr, kStateStrings[i]) == 0)
      {
        mIconState = i;
        break;
      }
    }
  }

  
  
  
  
  
  if (mDummyFilePath.Length() > 2)
  {
    if (!strncmp("//stock/", mDummyFilePath.get(), 8))
    {
      
      mStockIcon = Substring(mDummyFilePath, 8);
    }
    else
    {
      if (!strncmp("//", mDummyFilePath.get(), 2))
      {
        
        
        mDummyFilePath.Cut(0, 2); 
      }
      if (!strncmp("file://", mDummyFilePath.get(), 7))
      { 
        
        nsCOMPtr<nsIURI> tmpURI;
        rv = ioService->NewURI(mDummyFilePath, nsnull, nsnull, getter_AddRefs(tmpURI));
        if (NS_SUCCEEDED(rv) && tmpURI)
        {
          mFileIcon = tmpURI;
        }
      }
      if (!sizeString.IsEmpty())
      {
        PRInt32 sizeValue = atoi(sizeString.get());
        
        if (sizeValue)
          mSize = sizeValue;
      }
    }
  }
  else
    rv = NS_ERROR_MALFORMED_URI; 
  return rv;
}

NS_IMETHODIMP
nsMozIconURI::GetPrePath(nsACString &prePath)
{
  prePath = NS_MOZICON_SCHEME;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::GetScheme(nsACString &aScheme)
{
  aScheme = "moz-icon";
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::SetScheme(const nsACString &aScheme)
{
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::GetUsername(nsACString &aUsername)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::SetUsername(const nsACString &aUsername)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::GetPassword(nsACString &aPassword)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::SetPassword(const nsACString &aPassword)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::GetUserPass(nsACString &aUserPass)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::SetUserPass(const nsACString &aUserPass)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::GetHostPort(nsACString &aHostPort)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::SetHostPort(const nsACString &aHostPort)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::GetHost(nsACString &aHost)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::SetHost(const nsACString &aHost)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::GetPort(PRInt32 *aPort)
{
  return NS_ERROR_FAILURE;
}
 
NS_IMETHODIMP
nsMozIconURI::SetPort(PRInt32 aPort)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::GetPath(nsACString &aPath)
{
  aPath.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::SetPath(const nsACString &aPath)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsMozIconURI::Equals(nsIURI *other, PRBool *result)
{
  NS_ENSURE_ARG_POINTER(other);
  NS_PRECONDITION(result, "null pointer");

  nsCAutoString spec1;
  nsCAutoString spec2;

  other->GetSpec(spec2);
  GetSpec(spec1);
  if (!PL_strcasecmp(spec1.get(), spec2.get()))
    *result = PR_TRUE;
  else
    *result = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::SchemeIs(const char *i_Scheme, PRBool *o_Equals)
{
  NS_ENSURE_ARG_POINTER(o_Equals);
  if (!i_Scheme) return NS_ERROR_INVALID_ARG;
  
  *o_Equals = PL_strcasecmp("moz-icon", i_Scheme) ? PR_FALSE : PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::Clone(nsIURI **result)
{
  nsCOMPtr<nsIURI> newFileIcon;
  if (mFileIcon)
  {
    nsresult rv = mFileIcon->Clone(getter_AddRefs(newFileIcon));
    if (NS_FAILED(rv)) 
      return rv;
  }

  nsMozIconURI *uri = new nsMozIconURI();
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
 
  newFileIcon.swap(uri->mFileIcon);
  uri->mSize = mSize;
  uri->mContentType = mContentType;
  uri->mDummyFilePath = mDummyFilePath;
  uri->mStockIcon = mStockIcon;
  uri->mIconSize = mIconSize;
  uri->mIconState = mIconState;
  NS_ADDREF(*result = uri);

  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::Resolve(const nsACString &relativePath, nsACString &result)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMozIconURI::GetAsciiSpec(nsACString &aSpecA)
{
  return GetSpec(aSpecA);
}

NS_IMETHODIMP
nsMozIconURI::GetAsciiHost(nsACString &aHostA)
{
  return GetHost(aHostA);
}

NS_IMETHODIMP
nsMozIconURI::GetOriginCharset(nsACString &result)
{
  result.Truncate();
  return NS_OK;
}




NS_IMETHODIMP
nsMozIconURI::GetIconFile(nsIURI* * aFileUrl)
{
  *aFileUrl = mFileIcon;
  NS_IF_ADDREF(*aFileUrl);
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::SetIconFile(nsIURI* aFileUrl)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsMozIconURI::GetImageSize(PRUint32 * aImageSize)  
{
  *aImageSize = mSize;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::SetImageSize(PRUint32 aImageSize)  
{
  mSize = aImageSize;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::GetContentType(nsACString &aContentType)  
{
  aContentType = mContentType;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::SetContentType(const nsACString &aContentType) 
{
  mContentType = aContentType;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::GetFileExtension(nsACString &aFileExtension)  
{
  nsCAutoString fileExtension;
  nsresult rv = NS_OK;

  
  if (mFileIcon)
  {
    nsCAutoString fileExt;
    nsCOMPtr<nsIURL> url (do_QueryInterface(mFileIcon, &rv));
    if (NS_SUCCEEDED(rv) && url)
    {
      rv = url->GetFileExtension(fileExt);
      if (NS_SUCCEEDED(rv))
      {
        
        
        aFileExtension.Assign('.');
        aFileExtension.Append(fileExt);
        return NS_OK;
      }
    }
    
    mFileIcon->GetSpec(fileExt);
    fileExtension = fileExt;
  }
  else
  {
    fileExtension = mDummyFilePath;
  }

  
  const char * chFileName = fileExtension.get(); 
  const char * fileExt = strrchr(chFileName, '.');
  if (!fileExt) return NS_ERROR_FAILURE; 

  aFileExtension = fileExt;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::GetStockIcon(nsACString &aStockIcon)
{
  aStockIcon = mStockIcon;
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::GetIconSize(nsACString &aSize)
{
  if (mIconSize >= 0)
    aSize = kSizeStrings[mIconSize];
  else
    aSize.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsMozIconURI::GetIconState(nsACString &aState)
{
  if (mIconState)
    aState = kStateStrings[mIconState];
  else
    aState.Truncate();
  return NS_OK;
}

