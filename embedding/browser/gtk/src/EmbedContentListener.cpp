






































#include <strings.h>

#include "nsIURI.h"

#include "EmbedContentListener.h"
#include "EmbedPrivate.h"

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

NS_IMPL_ISUPPORTS2(EmbedContentListener,
                   nsIURIContentListener,
                   nsISupportsWeakReference)

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
  nsresult rv;

  if (mOwner->mOpenBlock) {
    *aAbortOpen = mOwner->mOpenBlock;
    mOwner->mOpenBlock = PR_FALSE;
    return NS_OK;
  }
  nsCAutoString specString;
  rv = aURI->GetSpec(specString);

  if (NS_FAILED(rv))
    return rv;

  gint return_val = FALSE;

  
  aURI->SchemeIs("mailto", &return_val);
  if (return_val) {
    
    *aAbortOpen = TRUE;

    gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                    moz_embed_signals[MAILTO],
                    specString.get());

    return NS_OK;
  }

  
  return_val = FALSE;

  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[OPEN_URI],
                  specString.get(), &return_val);

  *aAbortOpen = return_val;

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

