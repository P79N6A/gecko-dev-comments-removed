




#include "nsIOUtil.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsStreamUtils.h"

NS_IMPL_ISUPPORTS1(nsIOUtil, nsIIOUtil)

NS_IMETHODIMP
nsIOUtil::InputStreamIsBuffered(nsIInputStream* aStream, bool* _retval)
{
  if (NS_WARN_IF(!aStream))
    return NS_ERROR_INVALID_ARG;
  *_retval = NS_InputStreamIsBuffered(aStream);
  return NS_OK;
}

NS_IMETHODIMP
nsIOUtil::OutputStreamIsBuffered(nsIOutputStream* aStream, bool* _retval)
{
  if (NS_WARN_IF(!aStream))
    return NS_ERROR_INVALID_ARG;
  *_retval = NS_OutputStreamIsBuffered(aStream);
  return NS_OK;
}
