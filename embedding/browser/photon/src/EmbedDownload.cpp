





































#include "EmbedDownload.h"

EmbedDownload::EmbedDownload( PtMozillaWidget_t *aMoz, int aDownloadTicket, const char * aURL )
{
	mMozillaWidget = aMoz;
	mDownloadTicket = aDownloadTicket;
	mURL = strdup( aURL );
	mDone = PR_FALSE;
	mLauncher = nsnull;
	mPersist = nsnull;

	
	aMoz->fDownload = ( EmbedDownload ** ) realloc( aMoz->fDownload, ( aMoz->fDownloadCount + 1 ) * sizeof( EmbedDownload * ) );
	if( aMoz->fDownload ) {
		aMoz->fDownload[ aMoz->fDownloadCount ] = this;
		aMoz->fDownloadCount++;
		}
}

EmbedDownload::~EmbedDownload()
{
	int i;



  
  for( i=0; i<mMozillaWidget->fDownloadCount; i++ ) {
    if( mMozillaWidget->fDownload[i] == this ) break;
    }

  if( i<mMozillaWidget->fDownloadCount ) {
    int j;

    for( j=i; j<mMozillaWidget->fDownloadCount-1; j++ )
      mMozillaWidget->fDownload[j] = mMozillaWidget->fDownload[j+1];

    mMozillaWidget->fDownloadCount--;
    if( !mMozillaWidget->fDownloadCount ) {
      free( mMozillaWidget->fDownload );
      mMozillaWidget->fDownload = NULL;
      }

		if( mDone == PR_FALSE ) ReportDownload( Pt_WEB_DOWNLOAD_CANCEL, 0, 0, "" );
    }



	free( mURL );
}

EmbedDownload *FindDownload( PtMozillaWidget_t *moz, int download_ticket )
{
	int i;
  for( i=0; i<moz->fDownloadCount; i++ ) {
    if( moz->fDownload[i]->mDownloadTicket == download_ticket ) return moz->fDownload[i];
    }
	return NULL;
}


void EmbedDownload::ReportDownload( int type, int current, int total, char *message )
{
  PtCallbackInfo_t cbinfo;
  PtWebDownloadCallback_t cb;

  memset( &cbinfo, 0, sizeof( cbinfo ) );
  cbinfo.reason = Pt_CB_MOZ_DOWNLOAD;
  cbinfo.cbdata = &cb;

  cb.download_ticket = mDownloadTicket;
  cb.type = type;
  cb.url = mURL;
  cb.current = current;
  cb.total = total;
  cb.message = message;

	if( type == Pt_WEB_DOWNLOAD_DONE ) mDone = PR_TRUE;






  PtInvokeCallbackList( mMozillaWidget->web_download_cb, (PtWidget_t *)mMozillaWidget, &cbinfo );
  }



NS_IMPL_ISUPPORTS2(EmbedDownload, nsIWebProgressListener, nsIWebProgressListener2)

NS_IMETHODIMP EmbedDownload::OnProgressChange(nsIWebProgress *aProgress, nsIRequest *aRequest, PRInt32 curSelfProgress, PRInt32 maxSelfProgress, PRInt32 curTotalProgress, PRInt32 maxTotalProgress) {




	ReportDownload( Pt_WEB_DOWNLOAD_PROGRESS, curSelfProgress, maxSelfProgress, "" );

	return NS_OK;
	}

NS_IMETHODIMP EmbedDownload::OnProgressChange64(nsIWebProgress *aProgress, nsIRequest *aRequest, PRInt64 curSelfProgress, PRInt64 maxSelfProgress, PRInt64 curTotalProgress, PRInt64 maxTotalProgress) {
  
  return OnProgressChange(aProgress, aRequest,
                          PRInt32(curSelfProgress), PRInt32(maxSelfProgress),
                          PRInt32(curTotalProgress), PRInt32(maxTotalProgress));
  }

NS_IMETHODIMP EmbedDownload::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus) {
	if( aStateFlags & STATE_STOP ) {
		ReportDownload( Pt_WEB_DOWNLOAD_DONE, 0, 0, "" );
		}
	return NS_OK;
	}

NS_IMETHODIMP EmbedDownload::OnLocationChange(nsIWebProgress* aWebProgress, nsIRequest* aRequest, nsIURI *location) {
	return NS_OK;
	}
NS_IMETHODIMP EmbedDownload::OnStatusChange(nsIWebProgress* aWebProgress, nsIRequest* aRequest, nsresult aStatus, const PRUnichar* aMessage) {
	return NS_OK;
	}
NS_IMETHODIMP EmbedDownload::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state) {
	return NS_OK;
	}
NS_IMETHODIMP EmbedDownload::OnRefreshAttempted(nsIWebProgress *aWebProgress, nsIURI *aUri, PRInt32 aDelay, PRBool aSameUri, PRBool *allowRefresh)
{
    *allowRefresh = PR_TRUE;
    return NS_OK;
}

