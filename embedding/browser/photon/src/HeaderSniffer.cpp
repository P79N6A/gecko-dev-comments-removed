





































#include "nsILocalFile.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIURL.h"
#include "necko/nsNetUtil.h"
#include "HeaderSniffer.h"
#include "EmbedDownload.h"

const char* const kPersistContractID = "@mozilla.org/embedding/browser/nsWebBrowserPersist;1";

HeaderSniffer::HeaderSniffer( nsIWebBrowserPersist* aPersist, PtMozillaWidget_t *aMoz, nsIURI* aURL, nsIFile* aFile )
: mPersist(aPersist)
, mTmpFile(aFile)
, mURL(aURL)
{
	mMozillaWidget = aMoz;
}

HeaderSniffer::~HeaderSniffer( )
{
}

NS_IMPL_ISUPPORTS1(HeaderSniffer, nsIWebProgressListener)




NS_IMETHODIMP HeaderSniffer::OnProgressChange(nsIWebProgress *aProgress, nsIRequest *aRequest, PRInt32 curSelfProgress, PRInt32 maxSelfProgress, PRInt32 curTotalProgress, PRInt32 maxTotalProgress) {
	return NS_OK;
	}

NS_IMETHODIMP HeaderSniffer::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus) {

  if( aStateFlags & nsIWebProgressListener::STATE_START ) {

		nsCOMPtr<nsIWebBrowserPersist> kungFuDeathGrip(mPersist);   
		                                                            
		nsCOMPtr<nsIWebProgressListener> kungFuSuicideGrip(this);   

    nsresult rv;
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest, &rv);
    if (!channel) return rv;

		nsCAutoString contentType, suggested_filename;
    channel->GetContentType(contentType);

    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
    if (httpChannel) {
			nsCAutoString contentDisposition;
      nsresult rv = httpChannel->GetResponseHeader(nsCAutoString("content-disposition"), contentDisposition);
			if( NS_SUCCEEDED(rv) && !contentDisposition.IsEmpty() ) {
    		PRInt32 index = contentDisposition.Find("filename=");
    		if( index >= 0 ) {
    		  
    		  index += 9;
    		  contentDisposition.Right(suggested_filename, contentDisposition.Length() - index);
					}
				}
			}

		
		PtCallbackInfo_t cbinfo;
		PtWebUnknownWithNameCallback_t cb;

		memset( &cbinfo, 0, sizeof( cbinfo ) );
		cbinfo.reason = Pt_CB_MOZ_UNKNOWN;
		cbinfo.cbdata = &cb;
		cb.action = Pt_WEB_ACTION_OK;
		cb.content_type = (char*)contentType.get();
		nsCAutoString spec; mURL->GetSpec( spec );
		cb.url = (char*)spec.get();
		cb.content_length = strlen( cb.url );
		cb.suggested_filename = (char*)suggested_filename.get();
		PtInvokeCallbackList( mMozillaWidget->web_unknown_cb, (PtWidget_t *)mMozillaWidget, &cbinfo );
			
	

		
		mPersist->CancelSave();
		
		
		PRBool exists;
		mTmpFile->Exists(&exists);
		if(exists)
		    mTmpFile->Remove(PR_FALSE);

		
		if( mMozillaWidget->moz_unknown_ctrl->response == Pt_WEB_RESPONSE_OK ) {
			
			
			
			nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
			NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

			nsCString s ( mMozillaWidget->moz_unknown_ctrl->filename );
			file->InitWithNativePath( s );
			if( !file ) return NS_ERROR_FAILURE;

			
			nsCAutoString spec;
			mURL->GetSpec( spec );

			nsCOMPtr<nsIWebBrowserPersist> webPersist(do_CreateInstance(kPersistContractID, &rv));
			EmbedDownload *download = new EmbedDownload( mMozillaWidget, mMozillaWidget->moz_unknown_ctrl->download_ticket, spec.get() );
			if( webPersist ) {
				download->mPersist = webPersist;
				webPersist->SetProgressListener( download );
				webPersist->SaveURI( mURL, nsnull, nsnull, nsnull, nsnull, file );
				}
			}
  	}
	return NS_OK;
	}

NS_IMETHODIMP HeaderSniffer::OnLocationChange(nsIWebProgress* aWebProgress, nsIRequest* aRequest, nsIURI *location) {
	return NS_OK;
	}
NS_IMETHODIMP HeaderSniffer::OnStatusChange(nsIWebProgress* aWebProgress, nsIRequest* aRequest, nsresult aStatus, const PRUnichar* aMessage) {
	return NS_OK;
	}
NS_IMETHODIMP HeaderSniffer::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state) {
	return NS_OK;
	}

