





































#include "nsUnknownContentTypeHandler.h"
#include "nsIGenericFactory.h"
#include "nsIDOMWindowInternal.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDocShell.h"
#include "mimetype/nsIMIMEInfo.h"
#include "nsIURI.h"
#include "nsILocalFile.h"
#include "nsIComponentRegistrar.h"

#include "EmbedPrivate.h"
#include "PtMozilla.h"

nsUnknownContentTypeHandler::nsUnknownContentTypeHandler( ) {
	NS_INIT_ISUPPORTS();

	}

nsUnknownContentTypeHandler::~nsUnknownContentTypeHandler( )
{

}


NS_IMETHODIMP nsUnknownContentTypeHandler::Show( nsIHelperAppLauncher *aLauncher, nsISupports *aContext, PRUint32 aReason )
{
	return aLauncher->SaveToDisk( nsnull, PR_FALSE );
}

NS_IMETHODIMP nsUnknownContentTypeHandler::PromptForSaveToFile( nsIHelperAppLauncher* aLauncher,
                                                                nsISupports *aWindowContext,
                                                                const PRUnichar *aDefaultFile,
                                                                const PRUnichar *aSuggestedFileExtension,
                                                                nsILocalFile **_retval )
{

	NS_ENSURE_ARG_POINTER(_retval);
	*_retval = nsnull;

	

	nsCOMPtr<nsIDOMWindow> domw( do_GetInterface( aWindowContext ) );
	nsIDOMWindow *parent;
	domw->GetParent( &parent );
	PtWidget_t *w = GetWebBrowser( parent );
	PtMozillaWidget_t *moz = ( PtMozillaWidget_t * ) w;

	
	NS_ConvertUTF16toUTF8 theUnicodeString( aDefaultFile );
	const char *filename = theUnicodeString.get( );

	
	nsCOMPtr<nsIURI> aSourceUrl;
	aLauncher->GetSource( getter_AddRefs(aSourceUrl) );
	const char *url;
	nsCAutoString specString;
	aSourceUrl->GetSpec(specString);
	url = specString.get();

	
	nsCOMPtr<nsIMIMEInfo> mimeInfo;
	aLauncher->GetMIMEInfo( getter_AddRefs(mimeInfo) );
	nsCAutoString mimeType;
	mimeInfo->GetMIMEType( mimeType );

	PtCallbackInfo_t cbinfo;
	PtWebUnknownWithNameCallback_t cb;

	memset( &cbinfo, 0, sizeof( cbinfo ) );
	cbinfo.reason = Pt_CB_MOZ_UNKNOWN;
	cbinfo.cbdata = &cb;
	cb.action = Pt_WEB_ACTION_OK;
	cb.content_type = (char*)mimeType.get();
	cb.url = (char *)url;
	cb.content_length = strlen( cb.url );
	cb.suggested_filename = (char*)filename;
	PtInvokeCallbackList( moz->web_unknown_cb, (PtWidget_t *)moz, &cbinfo );
	

	
	if( moz->moz_unknown_ctrl->response != Pt_WEB_RESPONSE_OK ) return NS_ERROR_ABORT;

	
	nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
	NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

	nsCString s ( moz->moz_unknown_ctrl->filename );
	file->InitWithNativePath( s );
	if( !file ) return NS_ERROR_FAILURE;

	*_retval = file;
	NS_ADDREF( *_retval );

	
	EmbedDownload *download = new EmbedDownload( moz, moz->moz_unknown_ctrl->download_ticket, url );
	download->mLauncher = aLauncher;
	aLauncher->SetWebProgressListener( download );

	return NS_OK;
}


PtWidget_t *nsUnknownContentTypeHandler::GetWebBrowser(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsIWebBrowserChrome> chrome;
  PtWidget_t *val = 0;

  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService("@mozilla.org/embedcomp/window-watcher;1"));
  if (!wwatch) return nsnull;

  if( wwatch ) {
    nsCOMPtr<nsIDOMWindow> fosterParent;
    if (!aWindow) { 
      wwatch->GetActiveWindow(getter_AddRefs(fosterParent));
      aWindow = fosterParent;
    }
    wwatch->GetChromeForWindow(aWindow, getter_AddRefs(chrome));
  }

  if (chrome) {
    nsCOMPtr<nsIEmbeddingSiteWindow> site(do_QueryInterface(chrome));
    if (site) {
      site->GetSiteWindow(reinterpret_cast<void **>(&val));
    }
  }

  return val;
}



#define className             nsUnknownContentTypeHandler
#define interfaceName         nsIHelperAppLauncherDialog
#define contractId            NS_IUNKNOWNCONTENTTYPEHANDLER_CONTRACTID



NS_IMPL_ADDREF( className );  
NS_IMPL_RELEASE( className )



NS_IMETHODIMP className::QueryInterface( REFNSIID anIID, void **anInstancePtr ) { 
	nsresult rv = NS_OK; 


	
	if( !anInstancePtr ) rv = NS_ERROR_NULL_POINTER;
	else { 
		*anInstancePtr = 0; 
		if ( anIID.Equals( NS_GET_IID(nsIHelperAppLauncherDialog) ) ) { 
			*anInstancePtr = (void*) (nsIHelperAppLauncherDialog*)this; 
			NS_ADDREF_THIS();
			}
		else if ( anIID.Equals( NS_GET_IID(nsISupports) ) ) { 
			*anInstancePtr = (void*) ( (nsISupports*) (interfaceName*)this ); 
			NS_ADDREF_THIS();
			}
		else rv = NS_NOINTERFACE;
		} 
	return rv; 
	}

NS_GENERIC_FACTORY_CONSTRUCTOR( nsUnknownContentTypeHandler )


static nsModuleComponentInfo info[] = {
		{
			"nsUnknownContentTypeHandler",
			NS_IHELPERAPPLAUNCHERDIALOG_IID,
			NS_IHELPERAPPLAUNCHERDLG_CONTRACTID,
			nsUnknownContentTypeHandlerConstructor
		}
	};

int Init_nsUnknownContentTypeHandler_Factory( ) {
	nsresult rv;

	
	nsCOMPtr<nsIGenericFactory> factory;
	rv = NS_NewGenericFactory( getter_AddRefs(factory), info );
	if (NS_FAILED(rv))
		return rv;

	
	nsCOMPtr<nsIComponentRegistrar> registrar;
	rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
	if (NS_FAILED(rv))
		return rv;
	rv = registrar->RegisterFactory( kCID, "nsUnknownContentTypeHandler", NS_IHELPERAPPLAUNCHERDLG_CONTRACTID, factory);
	return NS_OK;
	}
