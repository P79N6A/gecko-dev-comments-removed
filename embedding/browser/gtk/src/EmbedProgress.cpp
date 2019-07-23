




































#include "EmbedProgress.h"

#include "nsIChannel.h"
#include "nsIWebProgress.h"
#include "nsIDOMWindow.h"

#include "nsIURI.h"

EmbedProgress::EmbedProgress(void)
{
  mOwner = nsnull;
}

EmbedProgress::~EmbedProgress()
{
}

NS_IMPL_ISUPPORTS2(EmbedProgress,
		   nsIWebProgressListener,
		   nsISupportsWeakReference)

nsresult
EmbedProgress::Init(EmbedPrivate *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnStateChange(nsIWebProgress *aWebProgress,
			     nsIRequest     *aRequest,
			     PRUint32        aStateFlags,
			     nsresult        aStatus)
{
  
  mOwner->ContentStateChange();
  
  if ((aStateFlags & GTK_MOZ_EMBED_FLAG_IS_NETWORK) && 
      (aStateFlags & GTK_MOZ_EMBED_FLAG_START))
  {
    g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[NET_START], 0);
  }

  
  nsCAutoString uriString;
  RequestToURIString(aRequest, uriString);

  
  if (mOwner->mURI.Equals(uriString))
  {
    
    g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[NET_STATE], 0,
                  aStateFlags, aStatus);
  }
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[NET_STATE_ALL], 0,
                uriString.get(),
                (gint)aStateFlags, (gint)aStatus);
  
  if ((aStateFlags & GTK_MOZ_EMBED_FLAG_IS_NETWORK) &&
      (aStateFlags & GTK_MOZ_EMBED_FLAG_STOP))
  {
    g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[NET_STOP], 0);
    
    mOwner->ContentFinishedLoading();
  }

  return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnProgressChange(nsIWebProgress *aWebProgress,
				nsIRequest     *aRequest,
				PRInt32         aCurSelfProgress,
				PRInt32         aMaxSelfProgress,
				PRInt32         aCurTotalProgress,
				PRInt32         aMaxTotalProgress)
{

  nsCAutoString uriString;
  RequestToURIString(aRequest, uriString);

  
  if (mOwner->mURI.Equals(uriString)) {
    g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[PROGRESS], 0,
                  aCurTotalProgress, aMaxTotalProgress);
  }

  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[PROGRESS_ALL], 0,
                uriString.get(),
                aCurTotalProgress, aMaxTotalProgress);
  return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnLocationChange(nsIWebProgress *aWebProgress,
				nsIRequest     *aRequest,
				nsIURI         *aLocation)
{
  nsCAutoString newURI;
  NS_ENSURE_ARG_POINTER(aLocation);
  aLocation->GetSpec(newURI);

  
  
  PRBool isSubFrameLoad = PR_FALSE;
  if (aWebProgress) {
    nsCOMPtr<nsIDOMWindow> domWindow;
    nsCOMPtr<nsIDOMWindow> topDomWindow;

    aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));

    
    if (domWindow)
      domWindow->GetTop(getter_AddRefs(topDomWindow));

    if (domWindow != topDomWindow)
      isSubFrameLoad = PR_TRUE;
  }

  if (!isSubFrameLoad) {
    mOwner->SetURI(newURI.get());
    g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                  moz_embed_signals[LOCATION], 0);
  }

  return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnStatusChange(nsIWebProgress  *aWebProgress,
			      nsIRequest      *aRequest,
			      nsresult         aStatus,
			      const PRUnichar *aMessage)
{
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[STATUS_CHANGE], 0,
                static_cast<void *>(aRequest),
                static_cast<int>(aStatus),
                static_cast<const void *>(aMessage));

  return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnSecurityChange(nsIWebProgress *aWebProgress,
				nsIRequest     *aRequest,
				PRUint32         aState)
{
  g_signal_emit(G_OBJECT(mOwner->mOwningWidget),
                moz_embed_signals[SECURITY_CHANGE], 0,
                static_cast<void *>(aRequest),
                aState);
  return NS_OK;
}


void
EmbedProgress::RequestToURIString(nsIRequest *aRequest, nsACString &aString)
{
  
  nsCOMPtr<nsIChannel> channel;
  channel = do_QueryInterface(aRequest);
  if (!channel)
    return;
  
  nsCOMPtr<nsIURI> uri;
  channel->GetURI(getter_AddRefs(uri));
  if (!uri)
    return;
  
  uri->GetSpec(aString);
}
