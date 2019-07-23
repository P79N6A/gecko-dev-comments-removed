





































#include "nsIOUtil.h"
#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsStreamUtils.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsIOUtil, nsIIOUtil)

NS_IMETHODIMP
nsIOUtil::InputStreamIsBuffered(nsIInputStream* aStream, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aStream);
  *_retval = NS_InputStreamIsBuffered(aStream);
  return NS_OK;
}

NS_IMETHODIMP
nsIOUtil::OutputStreamIsBuffered(nsIOutputStream* aStream, PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aStream);
  *_retval = NS_OutputStreamIsBuffered(aStream);
  return NS_OK;
}
