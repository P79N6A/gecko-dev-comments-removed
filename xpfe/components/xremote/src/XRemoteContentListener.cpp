



































#include "XRemoteContentListener.h"

XRemoteContentListener::XRemoteContentListener()
{
}

XRemoteContentListener::~XRemoteContentListener()
{
}

NS_IMPL_ISUPPORTS2(XRemoteContentListener,
		   nsIURIContentListener,
		   nsIInterfaceRequestor)



NS_IMETHODIMP
XRemoteContentListener::OnStartURIOpen(nsIURI *aURI, PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
XRemoteContentListener::DoContent(const char *aContentType,
				  PRBool aIsContentPreferred,
				  nsIRequest *request,
				  nsIStreamListener **aContentHandler,
				  PRBool *_retval)
{
  NS_NOTREACHED("XRemoteContentListener::DoContent");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
XRemoteContentListener::IsPreferred(const char *aContentType,
				    char **aDesiredContentType,
				    PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
XRemoteContentListener::CanHandleContent(const char *aContentType,
					 PRBool aIsContentPreferred,
					 char **aDesiredContentType,
					 PRBool *_retval)
{
  NS_NOTREACHED("XRemoteContentListener::CanHandleContent");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
XRemoteContentListener::GetLoadCookie(nsISupports * *aLoadCookie)
{
  *aLoadCookie = mLoadCookie;
  NS_IF_ADDREF(*aLoadCookie);
  return NS_OK;
}

NS_IMETHODIMP
XRemoteContentListener::SetLoadCookie(nsISupports * aLoadCookie)
{
  mLoadCookie = aLoadCookie;
  return NS_OK;
}

NS_IMETHODIMP
XRemoteContentListener::GetParentContentListener(nsIURIContentListener * *aParentContentListener)
{
  *aParentContentListener = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
XRemoteContentListener::SetParentContentListener(nsIURIContentListener * aParentContentListener)
{
  return NS_OK;
}


NS_IMETHODIMP
XRemoteContentListener::GetInterface(const nsIID & uuid, void * *result)
{
  NS_ENSURE_ARG_POINTER(result);
  return QueryInterface(uuid, result);
}
