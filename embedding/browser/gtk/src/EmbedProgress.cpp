




































#include "EmbedProgress.h"

#include <nsXPIDLString.h>
#include <nsIChannel.h>
#include <nsIWebProgress.h>
#include <nsIDOMWindow.h>

#include "nsIURI.h"
#include "nsCRT.h"

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
    gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		    moz_embed_signals[NET_START]);
  }

  
  nsXPIDLCString uriString;
  RequestToURIString(aRequest, getter_Copies(uriString));
  nsString tmpString;
  CopyUTF8toUTF16(uriString, tmpString);

  
  if (mOwner->mURI.Equals(tmpString))
  {
    
    gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		    moz_embed_signals[NET_STATE],
		    aStateFlags, aStatus);
  }
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		  moz_embed_signals[NET_STATE_ALL],
		  (const char *)uriString,
		  (gint)aStateFlags, (gint)aStatus);
  
  if ((aStateFlags & GTK_MOZ_EMBED_FLAG_IS_NETWORK) && 
      (aStateFlags & GTK_MOZ_EMBED_FLAG_STOP))
  {
    gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		    moz_embed_signals[NET_STOP]);
    
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

  nsXPIDLCString uriString;
  RequestToURIString(aRequest, getter_Copies(uriString));
  
  nsString tmpString;
  CopyUTF8toUTF16(uriString, tmpString);

  
  if (mOwner->mURI.Equals(tmpString)) {
    gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		    moz_embed_signals[PROGRESS],
		    aCurTotalProgress, aMaxTotalProgress);
  }

  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		  moz_embed_signals[PROGRESS_ALL],
		  (const char *)uriString,
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
    gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		    moz_embed_signals[LOCATION]);
  }

  return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnStatusChange(nsIWebProgress  *aWebProgress,
			      nsIRequest      *aRequest,
			      nsresult         aStatus,
			      const PRUnichar *aMessage)
{
  
  PRUnichar *tmpString = nsCRT::strdup(aMessage);

  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		  moz_embed_signals[STATUS_CHANGE],
		  static_cast<void *>(aRequest),
		  static_cast<int>(aStatus),
		  static_cast<void *>(tmpString));

  nsMemory::Free(tmpString);

  return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnSecurityChange(nsIWebProgress *aWebProgress,
				nsIRequest     *aRequest,
				PRUint32         aState)
{
  gtk_signal_emit(GTK_OBJECT(mOwner->mOwningWidget),
		  moz_embed_signals[SECURITY_CHANGE],
		  static_cast<void *>(aRequest),
		  aState);
  return NS_OK;
}


void
EmbedProgress::RequestToURIString(nsIRequest *aRequest, char **aString)
{
  
  nsCOMPtr<nsIChannel> channel;
  channel = do_QueryInterface(aRequest);
  if (!channel)
    return;
  
  nsCOMPtr<nsIURI> uri;
  channel->GetURI(getter_AddRefs(uri));
  if (!uri)
    return;
  
  nsCAutoString uriString;
  uri->GetSpec(uriString);

  *aString = strdup(uriString.get());
}
