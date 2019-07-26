




#include "nsIOUtil.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsStreamUtils.h"

NS_IMPL_ISUPPORTS(nsIOUtil, nsIIOUtil)

NS_IMETHODIMP
nsIOUtil::InputStreamIsBuffered(nsIInputStream* aStream, bool* aResult)
{
  if (NS_WARN_IF(!aStream)) {
    return NS_ERROR_INVALID_ARG;
  }
  *aResult = NS_InputStreamIsBuffered(aStream);
  return NS_OK;
}

NS_IMETHODIMP
nsIOUtil::OutputStreamIsBuffered(nsIOutputStream* aStream, bool* aResult)
{
  if (NS_WARN_IF(!aStream)) {
    return NS_ERROR_INVALID_ARG;
  }
  *aResult = NS_OutputStreamIsBuffered(aStream);
  return NS_OK;
}
