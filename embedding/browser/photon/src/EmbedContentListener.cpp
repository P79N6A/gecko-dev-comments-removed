





































#include <strings.h>
#include <nsXPIDLString.h>

#include "nsIURI.h"

#include "EmbedContentListener.h"
#include "EmbedPrivate.h"

#include "PtMozilla.h"

#include "nsServiceManagerUtils.h"
#include "nsIWebNavigationInfo.h"
#include "nsDocShellCID.h"

EmbedContentListener::EmbedContentListener(void)
{
  mOwner = nsnull;
}

EmbedContentListener::~EmbedContentListener()
{
}

NS_IMPL_ISUPPORTS1(EmbedContentListener,
		   nsIURIContentListener)

nsresult
EmbedContentListener::Init(EmbedPrivate *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::OnStartURIOpen(nsIURI     *aURI,
				     PRBool     *aAbortOpen)
{
	PtMozillaWidget_t   *moz = (PtMozillaWidget_t *) mOwner->mOwningWidget;
	PtCallbackList_t    *cb = NULL;
	PtCallbackInfo_t    cbinfo;
	PtMozillaUrlCb_t    url;
	nsCAutoString specString;

	if (!moz->open_cb)
		return NS_OK;

	memset(&cbinfo, 0, sizeof(cbinfo));
	cbinfo.cbdata = &url;
	cbinfo.reason = Pt_CB_MOZ_OPEN;
	cb = moz->open_cb;

	aURI->GetSpec(specString);
	url.url = (char *) specString.get();

	if (PtInvokeCallbackList(cb, (PtWidget_t *) moz, &cbinfo) == Pt_END)
	{
		*aAbortOpen = PR_TRUE;
		return NS_ERROR_ABORT;
	}

	*aAbortOpen = PR_FALSE;

	return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::DoContent(const char         *aContentType,
				PRBool             aIsContentPreferred,
				nsIRequest         *aRequest,
				nsIStreamListener **aContentHandler,
				PRBool             *aAbortProcess)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::IsPreferred(const char        *aContentType,
				  char             **aDesiredContentType,
				  PRBool            *aCanHandleContent)
{
  return CanHandleContent(aContentType, PR_TRUE, aDesiredContentType,
			  aCanHandleContent);
}

NS_IMETHODIMP
EmbedContentListener::CanHandleContent(const char        *aContentType,
				       PRBool           aIsContentPreferred,
				       char             **aDesiredContentType,
				       PRBool            *_retval)
{
  *_retval = PR_FALSE;
  *aDesiredContentType = nsnull;
  
  if (aContentType) {
    nsCOMPtr<nsIWebNavigationInfo> webNavInfo(
           do_GetService(NS_WEBNAVIGATION_INFO_CONTRACTID));
    if (webNavInfo) {
      PRUint32 canHandle;
      nsresult rv =
	webNavInfo->IsTypeSupported(nsDependentCString(aContentType),
				    mOwner ? mOwner->mNavigation.get() : nsnull,
				    &canHandle);
      NS_ENSURE_SUCCESS(rv, rv);
      *_retval = (canHandle != nsIWebNavigationInfo::UNSUPPORTED);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::GetLoadCookie(nsISupports **aLoadCookie)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::SetLoadCookie(nsISupports *aLoadCookie)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::GetParentContentListener(nsIURIContentListener **aParent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::SetParentContentListener(nsIURIContentListener *aParent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

