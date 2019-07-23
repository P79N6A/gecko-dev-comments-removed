






































#include "EmbedProgress.h"

#include "qgeckoembed.h"

#include <nsXPIDLString.h>
#include <nsIChannel.h>
#include <nsIWebProgress.h>
#include <nsIDOMWindow.h>

#include "nsIURI.h"
#include "nsCRT.h"
#include "nsString.h"

EmbedProgress::EmbedProgress(QGeckoEmbed *aOwner)
{
    qDebug("XXX EMBEDPROGRSS");
    mOwner = aOwner;
}

EmbedProgress::~EmbedProgress()
{
    qDebug("#########################################################################################");
}

NS_IMPL_ISUPPORTS2(EmbedProgress,
                   nsIWebProgressListener,
                   nsISupportsWeakReference)

NS_IMETHODIMP
EmbedProgress::OnStateChange(nsIWebProgress *aWebProgress,
                             nsIRequest     *aRequest,
                             PRUint32        aStateFlags,
                             nsresult        aStatus)
{
    
    mOwner->contentStateChanged();
    
    if ((aStateFlags & STATE_IS_NETWORK) &&
        (aStateFlags & STATE_START))
    {
        qDebug("net start");
        emit mOwner->netStart();
    }

    

    if ((aStateFlags & STATE_IS_NETWORK) &&
             (aStateFlags & STATE_STOP)) {
        
        emit mOwner->netStop();
        mOwner->contentFinishedLoading();
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
#if 0
    nsString tmpString;
    RequestToURIString(aRequest, tmpString);
    
    if (mOwner->mURI.Equals(tmpString))
        mOwner->progressAll(QString(tmpString.get()), aCurTotalProgress, aMaxTotalProgress);
#endif
    

    mOwner->progress(aCurTotalProgress, aMaxTotalProgress);
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

    if (!isSubFrameLoad)
        emit mOwner->locationChanged(newURI.get());

    return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnStatusChange(nsIWebProgress  *aWebProgress,
                              nsIRequest      *aRequest,
                              nsresult         aStatus,
                              const PRUnichar *aMessage)
{
    QString message = QString::fromUcs2(aMessage);
    emit mOwner->linkMessage(message);

    return NS_OK;
}

NS_IMETHODIMP
EmbedProgress::OnSecurityChange(nsIWebProgress *aWebProgress,
                                nsIRequest     *aRequest,
                                PRUint32         aState)
{
    
    

    return NS_OK;
}


void
EmbedProgress::RequestToURIString(nsIRequest *aRequest, nsString &aString)
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

    CopyUTF8toUTF16(uriString, aString);
}
