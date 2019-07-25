




































#include "nsCharDetDll.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsDocumentCharsetInfo.h"
#include "nsCOMPtr.h"



NS_IMPL_THREADSAFE_ISUPPORTS1(nsDocumentCharsetInfo, nsIDocumentCharsetInfo)

nsDocumentCharsetInfo::nsDocumentCharsetInfo() 
{
  mParentCharsetSource = 0;
}

nsDocumentCharsetInfo::~nsDocumentCharsetInfo() 
{
}

NS_IMETHODIMP nsDocumentCharsetInfo::SetForcedCharset(nsIAtom * aCharset)
{
  mForcedCharset = aCharset;
  return NS_OK;
}

NS_IMETHODIMP nsDocumentCharsetInfo::GetForcedCharset(nsIAtom ** aResult)
{
  *aResult = mForcedCharset;
  if (mForcedCharset) NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentCharsetInfo::SetForcedDetector(PRBool aForced)
{
  
  return NS_OK;
}

NS_IMETHODIMP nsDocumentCharsetInfo::GetForcedDetector(PRBool * aResult)
{
  
  return NS_OK;
}

NS_IMETHODIMP nsDocumentCharsetInfo::SetParentCharset(nsIAtom * aCharset)
{
  mParentCharset = aCharset;
  return NS_OK;
}

NS_IMETHODIMP nsDocumentCharsetInfo::GetParentCharset(nsIAtom ** aResult)
{
  *aResult = mParentCharset;
  if (mParentCharset) NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsDocumentCharsetInfo::SetParentCharsetSource(PRInt32 aCharsetSource)
{
  mParentCharsetSource = aCharsetSource;
  return NS_OK;
}

NS_IMETHODIMP nsDocumentCharsetInfo::GetParentCharsetSource(PRInt32 * aParentCharsetSource)
{
  *aParentCharsetSource = mParentCharsetSource;
  return NS_OK;
}

