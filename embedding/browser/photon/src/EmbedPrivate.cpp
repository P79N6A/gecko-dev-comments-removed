





































#include <nsIDocShell.h>
#include <nsIURI.h>
#include <nsIWebProgress.h>
#include <nsIDOMDocument.h>
#include <nsIDOMNodeList.h>
#include <nsISelection.h>
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsIWidget.h"


#include <nsIInterfaceRequestor.h>
#include <nsIInterfaceRequestorUtils.h>

#include <nsIComponentManager.h>

#include <nsIPrintSettings.h>
#include "nsPrintSettingsImpl.h"


#include <nsIWindowWatcher.h>

#include <nsILocalFile.h>
#include <nsEmbedAPI.h>
#include <nsString.h>



#include <nsIDOMWindow.h>
#include <nsPIDOMWindow.h>
#include <nsIDOMWindowInternal.h>
#include <nsIContentViewer.h>
#include <nsIContentViewerEdit.h>
#include <nsIWebBrowserSetup.h>
#include "nsIWebBrowserPrint.h"
#include "nsIClipboardCommands.h"


#include "nsClipboard.h"
#include "nsWidgetsCID.h"


#include <nsIFocusController.h>

#include "nsIWebBrowserPrint.h"
#include "nsIPrintOptions.h"


#include "EmbedPrivate.h"
#include "EmbedWindow.h"
#include "EmbedProgress.h"
#include "EmbedContentListener.h"
#include "EmbedEventListener.h"
#include "EmbedWindowCreator.h"
#include "EmbedStream.h"
#include "EmbedPrintListener.h"

#include "PtMozilla.h"


extern char *g_Print_Left_Header_String, *g_Print_Right_Header_String, *g_Print_Left_Footer_String, *g_Print_Right_Footer_String;

static const char sWatcherContractID[] = "@mozilla.org/embedcomp/window-watcher;1";
static NS_DEFINE_CID(kCClipboardCID, NS_CLIPBOARD_CID);

nsIAppShell *EmbedPrivate::sAppShell    = nsnull;
nsIPref     *EmbedPrivate::sPrefs       = nsnull;
nsVoidArray *EmbedPrivate::sWindowList  = nsnull;
nsClipboard *EmbedPrivate::sClipboard   = nsnull;

EmbedPrivate::EmbedPrivate(void)
{
  mOwningWidget     = nsnull;
  mWindow           = nsnull;
  mProgress         = nsnull;
  mContentListener  = nsnull;
  mEventListener    = nsnull;
  mStream           = nsnull;
  mChromeMask       = 0;
  mIsChrome         = PR_FALSE;
  mChromeLoaded     = PR_FALSE;
  mListenersAttached = PR_FALSE;
  mMozWindowWidget  = 0;

	if (!sWindowList) {
  		sWindowList = new nsVoidArray();
	}
	sWindowList->AppendElement(this);
	if( !sClipboard ) {
		nsresult rv;
		nsCOMPtr<nsClipboard> s;
		s = do_GetService( kCClipboardCID, &rv );
		sClipboard = ( nsClipboard * ) s;
		if( NS_FAILED( rv ) ) sClipboard = 0;
	}
}

EmbedPrivate::~EmbedPrivate()
{
}


static void mozilla_set_default_pref( nsIPref *pref );

nsresult
EmbedPrivate::Init(PtWidget_t *aOwningWidget)
{
	
	if (mOwningWidget)
		return NS_OK;

	
	mOwningWidget = aOwningWidget;

	
	
	
	mWindow = new EmbedWindow();
	mWindowGuard = NS_STATIC_CAST(nsIWebBrowserChrome *, mWindow);
	mWindow->Init(this);

	
	
	
	mProgress = new EmbedProgress();
	mProgressGuard = NS_STATIC_CAST(nsIWebProgressListener *,
					   mProgress);
	mProgress->Init(this);

	
	
	
	mContentListener = new EmbedContentListener();
	mContentListenerGuard = mContentListener;
	mContentListener->Init(this);

	
	
	mEventListener = new EmbedEventListener();
	mEventListenerGuard =
	NS_STATIC_CAST(nsISupports *, NS_STATIC_CAST(nsIDOMKeyListener *,
						 mEventListener));
	mEventListener->Init(this);

	
	
	
	mPrint = new EmbedPrintListener();
	mPrintGuard = NS_STATIC_CAST(nsIWebProgressListener *, mPrint);
	mPrint->Init(this);

	
	static int initialized = PR_FALSE;
	
	if (!initialized) 
	{
		
		
		initialized = PR_TRUE;

		
		EmbedWindowCreator *creator = new EmbedWindowCreator();
		nsCOMPtr<nsIWindowCreator> windowCreator;
		windowCreator = NS_STATIC_CAST(nsIWindowCreator *, creator);

		
		nsCOMPtr<nsIWindowWatcher> watcher = do_GetService(sWatcherContractID);
		if (watcher)
      		watcher->SetWindowCreator(windowCreator);
  }

	if (!sPrefs)
	{
		
		nsresult rv;
		nsCOMPtr<nsIPref> pref(do_GetService(NS_PREF_CONTRACTID, &rv));
		if (pref)
		{
			sPrefs = pref.get();
			NS_ADDREF( sPrefs );
			extern int sProfileDirCreated;
			if( sProfileDirCreated ) mozilla_set_default_pref( pref );
			sPrefs->ReadUserPrefs( nsnull );
		}
	}

  return NS_OK;
}

nsIPref *
EmbedPrivate::GetPrefs()
{
	return (sPrefs);
}

nsresult
EmbedPrivate::Setup()
{
	
	nsCOMPtr<nsIWebBrowser> webBrowser;
	mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

	
	PRBool aAllowPlugins = PR_TRUE;
	nsCOMPtr<nsIWebBrowserSetup> webBrowserAsSetup(do_QueryInterface(webBrowser));
	webBrowserAsSetup->SetProperty(nsIWebBrowserSetup::SETUP_ALLOW_PLUGINS, aAllowPlugins);

	
	mNavigation = do_QueryInterface(webBrowser);

	
	
	
	mSessionHistory = do_CreateInstance(NS_SHISTORY_CONTRACTID);
	mNavigation->SetSessionHistory(mSessionHistory);

	
	mWindow->CreateWindow();

	
	nsCOMPtr<nsISupportsWeakReference> supportsWeak;
	supportsWeak = do_QueryInterface(mProgressGuard);
	nsCOMPtr<nsIWeakReference> weakRef;
	supportsWeak->GetWeakReference(getter_AddRefs(weakRef));
	webBrowser->AddWebBrowserListener(weakRef, NS_GET_IID(nsIWebProgressListener));

	
	nsCOMPtr<nsIURIContentListener> uriListener;
	uriListener = do_QueryInterface(mContentListenerGuard);
	webBrowser->SetParentURIContentListener(uriListener);

	nsCOMPtr<nsIWebBrowserPrint> print(do_GetInterface(webBrowser));



	return NS_OK;
}

void
EmbedPrivate::Show(void)
{
  
  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(webBrowser);
  baseWindow->SetVisibility(PR_TRUE);
}

void
EmbedPrivate::Hide(void)
{
  
  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(webBrowser);
  baseWindow->SetVisibility(PR_FALSE);
}

void
EmbedPrivate::Position(PRUint32 aX, PRUint32 aY)
{
	mWindow->mBaseWindow->SetPosition(aX, aY);
}

void
EmbedPrivate::Size(PRUint32 aWidth, PRUint32 aHeight)
{
	mWindow->mBaseWindow->SetSize(aWidth, aHeight, PR_TRUE);
}



void
EmbedPrivate::Destroy(void)
{
  
  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  
  nsCOMPtr<nsISupportsWeakReference> supportsWeak;
  supportsWeak = do_QueryInterface(mProgressGuard);
  nsCOMPtr<nsIWeakReference> weakRef;
  supportsWeak->GetWeakReference(getter_AddRefs(weakRef));
  webBrowser->RemoveWebBrowserListener(weakRef,
				       NS_GET_IID(nsIWebProgressListener));
  weakRef = nsnull;
  supportsWeak = nsnull;

  
  webBrowser->SetParentURIContentListener(nsnull);
  mContentListenerGuard = nsnull;
  mContentListener = nsnull;

  
  
  mProgressGuard = nsnull;
  mProgress = nsnull;

  
  DetachListeners();
  if (mEventTarget)
    mEventTarget = nsnull;

	
  sWindowList->RemoveElement(this);
  
  
  mWindow->ReleaseChildren();

  
  mNavigation = nsnull;

  

  
  mSessionHistory = nsnull;

  mOwningWidget = nsnull;

  mMozWindowWidget = 0;
}

void
EmbedPrivate::SetURI(const char *aURI)
{
  mURI.AssignWithConversion(aURI);
}

void
EmbedPrivate::LoadCurrentURI(void)
{
  if (mURI.Length())
	mNavigation->LoadURI(mURI.get(),                        
                     nsIWebNavigation::LOAD_FLAGS_NONE, 
                     nsnull,                            
                     nsnull,                            
                     nsnull);                           
}

void
EmbedPrivate::Stop(void)
{
	if (mNavigation)
		mNavigation->Stop(nsIWebNavigation::STOP_ALL);
}

void
EmbedPrivate::Reload(int32_t flags)
{
	PRUint32 reloadFlags = 0;

	
	switch (flags) 
	{
		case MOZ_EMBED_FLAG_RELOADNORMAL:
		  	reloadFlags = 0;
		  	break;
		case MOZ_EMBED_FLAG_RELOADBYPASSCACHE:
		  	reloadFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE;
		  	break;
		case MOZ_EMBED_FLAG_RELOADBYPASSPROXY:
		  	reloadFlags = nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY;
		  	break;
		case MOZ_EMBED_FLAG_RELOADBYPASSPROXYANDCACHE:
		  	reloadFlags = (nsIWebNavigation::LOAD_FLAGS_BYPASS_PROXY |
				 nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE);
		  	break;
		case MOZ_EMBED_FLAG_RELOADCHARSETCHANGE:
		  	reloadFlags = nsIWebNavigation::LOAD_FLAGS_CHARSET_CHANGE;
		  	break;
		default:
		  	reloadFlags = 0;
		  	break;
	}

	if (mNavigation)
  		mNavigation->Reload(reloadFlags);
}

void
EmbedPrivate::Back(void)
{
	if (mNavigation)
		mNavigation->GoBack();
}

void
EmbedPrivate::Forward(void)
{
	if (mNavigation)
		mNavigation->GoForward();
}

void
EmbedPrivate::ScrollUp(int amount)
{
  	nsCOMPtr<nsIWebBrowser> webBrowser;
  	mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

	nsCOMPtr<nsIDOMWindow> oDomWindow;
	nsresult rv = webBrowser->GetContentDOMWindow(getter_AddRefs(oDomWindow));

	if (oDomWindow)
		rv = oDomWindow->ScrollBy(0, -amount);
}
void
EmbedPrivate::ScrollDown(int amount)
{
  	nsCOMPtr<nsIWebBrowser> webBrowser;
  	mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

	nsCOMPtr<nsIDOMWindow> oDomWindow;
	nsresult rv = webBrowser->GetContentDOMWindow(getter_AddRefs(oDomWindow));

	if (oDomWindow)
		rv = oDomWindow->ScrollBy(0, amount);
}
void
EmbedPrivate::ScrollLeft(int amount)
{
  	nsCOMPtr<nsIWebBrowser> webBrowser;
  	mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

	nsCOMPtr<nsIDOMWindow> oDomWindow;
	nsresult rv = webBrowser->GetContentDOMWindow(getter_AddRefs(oDomWindow));

	if (oDomWindow)
		rv = oDomWindow->ScrollBy(-amount, 0);
}
void
EmbedPrivate::ScrollRight(int amount)
{
  	nsCOMPtr<nsIWebBrowser> webBrowser;
  	mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

	nsCOMPtr<nsIDOMWindow> oDomWindow;
	nsresult rv = webBrowser->GetContentDOMWindow(getter_AddRefs(oDomWindow));

	if (oDomWindow)
		rv = oDomWindow->ScrollBy(amount, 0);
}






PRBool
EmbedPrivate::CanGoBack()
{
	PRBool nsresult = PR_FALSE;

	if (mNavigation)
    	mNavigation->GetCanGoBack(&nsresult);

	return (nsresult);
}

PRBool
EmbedPrivate::CanGoForward()
{
	PRBool nsresult = PR_FALSE;

	if (mNavigation)
    	mNavigation->GetCanGoForward(&nsresult);

	return (nsresult);
}

void
EmbedPrivate::Cut(int ig)
{
	nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWindow->mWebBrowser));
	if (clipboard) {
		
		
		
		
		if (sClipboard)
			sClipboard->SetInputGroup(ig);
	    clipboard->CutSelection();
		}
}

void
EmbedPrivate::Copy(int ig)
{
	nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWindow->mWebBrowser));
	if (clipboard) {
		
		
		
		
		if (sClipboard)
			sClipboard->SetInputGroup(ig);
	    clipboard->CopySelection();
		}
}

void
EmbedPrivate::Paste(int ig)
{
	nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWindow->mWebBrowser));
	if (clipboard) {
		
		
		
		
		if (sClipboard)
			sClipboard->SetInputGroup(ig);
	    clipboard->Paste();
		}
}

void
EmbedPrivate::SelectAll()
{






	nsCOMPtr<nsIDOMWindow> domWindow;
	mWindow->mWebBrowser->GetContentDOMWindow( getter_AddRefs( domWindow ) );
	if( !domWindow ) {
		NS_WARNING( "no dom window in content finished loading\n" );
		return;
		}

	nsCOMPtr<nsIDOMDocument> domDocument;
	domWindow->GetDocument( getter_AddRefs( domDocument ) );
	if( !domDocument ) {
		NS_WARNING( "no dom document\n" );
		return;
		}

	nsCOMPtr<nsIDOMNodeList> list;
	domDocument->GetElementsByTagName( NS_LITERAL_STRING( "body" ), getter_AddRefs( list ) );
	if( !list ) {
		NS_WARNING( "no list\n" );
		return;
		}

	nsCOMPtr<nsIDOMNode> node;
	list->Item( 0, getter_AddRefs( node ) );
	if( !node ) {
		NS_WARNING( "no node\n" );
		return;
		}

	nsCOMPtr<nsISelection> selection;
	domWindow->GetSelection( getter_AddRefs( selection ) );
	if( !selection ) {
		NS_WARNING( "no selection\n" );
		return;
		}

	selection->SelectAllChildren( node );

}

void
EmbedPrivate::Clear()
{
	nsCOMPtr<nsIClipboardCommands> clipboard(do_GetInterface(mWindow->mWebBrowser));
	if (clipboard)
	    clipboard->SelectNone();
}

void
EmbedPrivate::Print(PpPrintContext_t *pc)
{
    nsCOMPtr<nsIWebBrowserPrint> browserAsPrint = do_GetInterface(mWindow->mWebBrowser);
    NS_ASSERTION(browserAsPrint, "No nsIWebBrowserPrint!");

    nsCOMPtr<nsIPrintSettings> printSettings;
    browserAsPrint->GetGlobalPrintSettings(getter_AddRefs(printSettings));
    if (printSettings) 
    {
    printSettings->SetPrintSilent(PR_TRUE);
		printSettings->SetEndPageRange((PRInt32) pc);

		nsAutoString format_left_footer;
		PrintHeaderFooter_FormatSpecialCodes( g_Print_Left_Footer_String, format_left_footer );
		nsAutoString format_right_footer;
		PrintHeaderFooter_FormatSpecialCodes( g_Print_Right_Footer_String, format_right_footer );
		nsAutoString format_left_header;
		PrintHeaderFooter_FormatSpecialCodes( g_Print_Left_Header_String, format_left_header );
		nsAutoString format_right_header;
		PrintHeaderFooter_FormatSpecialCodes( g_Print_Right_Header_String, format_right_header );

		printSettings->SetFooterStrLeft( format_left_footer.get() );
		printSettings->SetFooterStrRight( format_right_footer.get() );
		printSettings->SetHeaderStrLeft( format_left_header.get() );
		printSettings->SetHeaderStrRight( format_right_header.get() );
    }

		nsIPref *pref = GetPrefs();
		pref->SetBoolPref( "print.show_print_progress", PR_FALSE );

    browserAsPrint->Print(printSettings, mPrint);
}

nsresult
EmbedPrivate::OpenStream(const char *aBaseURI, const char *aContentType)
{
  nsresult rv;

  if (!mStream) {
    mStream = new EmbedStream();
    mStreamGuard = do_QueryInterface(mStream);
    mStream->InitOwner(this);
    rv = mStream->Init();
    if (NS_FAILED(rv))
      return rv;
  }

  rv = mStream->OpenStream(aBaseURI, aContentType);
  return rv;
}

int
EmbedPrivate::SaveAs(char *fname, char *dirname)
{	
	if (mWindow)
		return (mWindow->SaveAs(fname, dirname));
	return (1);
}

nsresult
EmbedPrivate::AppendToStream(const char *aData, PRInt32 aLen)
{
  if (!mStream)
    return NS_ERROR_FAILURE;

  
  
  ContentStateChange();

  return mStream->AppendToStream(aData, aLen);
}

nsresult
EmbedPrivate::CloseStream(void)
{
  nsresult rv;

  if (!mStream)
    return NS_ERROR_FAILURE;
  rv = mStream->CloseStream();

  
  mStream = 0;
  mStreamGuard = 0;

  return rv;
}


EmbedPrivate *
EmbedPrivate::FindPrivateForBrowser(nsIWebBrowserChrome *aBrowser)
{
	if (!sWindowList)
	  return nsnull;

	
	PRInt32 count = sWindowList->Count();
	
	
	
	for (int i = 0; i < count; i++) 
	{
	  EmbedPrivate *tmpPrivate = NS_STATIC_CAST(EmbedPrivate *,
							sWindowList->ElementAt(i));
	  
	  nsIWebBrowserChrome *chrome = NS_STATIC_CAST(nsIWebBrowserChrome *,
						   tmpPrivate->mWindow);
	  if (chrome == aBrowser)
		return tmpPrivate;
	}

  return nsnull;
}

void
EmbedPrivate::ContentStateChange(void)
{

  
  if (mListenersAttached && !mIsChrome)
    return;

  GetListener();

  if (!mEventTarget)
    return;
  
  AttachListeners();

}

void
EmbedPrivate::ContentFinishedLoading(void)
{
  if (mIsChrome) {
    
    mChromeLoaded = PR_TRUE;

    
    nsCOMPtr<nsIWebBrowser> webBrowser;
    mWindow->GetWebBrowser(getter_AddRefs(webBrowser));
    
    
    nsCOMPtr<nsIDOMWindow> domWindow;
    webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
    if (!domWindow) {
      NS_WARNING("no dom window in content finished loading\n");
      return;
    }
    
    
    domWindow->SizeToContent();

    
    
    PRBool visibility;
    mWindow->GetVisibility(&visibility);
    if (visibility)
      mWindow->SetVisibility(PR_TRUE);
  }
}



#if 0









void
EmbedPrivate::TopLevelFocusIn(void)
{
  nsCOMPtr<nsPIDOMWindow> piWin;
  GetPIDOMWindow(getter_AddRefs(piWin));

  if (!piWin)
    return;

  nsIFocusController *focusController = piWin->GetRootFocusController();
  if (focusController)
    focusController->SetActive(PR_TRUE);
}

void
EmbedPrivate::TopLevelFocusOut(void)
{
  nsCOMPtr<nsPIDOMWindow> piWin;
  GetPIDOMWindow(getter_AddRefs(piWin));

  if (!piWin)
    return;

  nsIFocusController *focusController = piWin->GetRootFocusController();
  if (focusController)
    focusController->SetActive(PR_FALSE);
}

void
EmbedPrivate::ChildFocusIn(void)
{
  nsCOMPtr<nsPIDOMWindow> piWin;
  GetPIDOMWindow(getter_AddRefs(piWin));

  if (!piWin)
    return;

  piWin->Activate();
}

void
EmbedPrivate::ChildFocusOut(void)
{
  nsCOMPtr<nsPIDOMWindow> piWin;
  GetPIDOMWindow(getter_AddRefs(piWin));

  if (!piWin)
    return;

  piWin->Deactivate();

  
  
  nsIFocusController *focusController = piWin->GetRootFocusController();
  if (focusController)
    focusController->SetActive(PR_TRUE);

}

#endif




void
EmbedPrivate::GetListener(void)
{
  if (mEventTarget)
    return;

  nsCOMPtr<nsPIDOMWindow> piWin;
  GetPIDOMWindow(getter_AddRefs(piWin));

  if (!piWin)
    return;

  mEventTarget = do_QueryInterface(piWin->GetChromeEventHandler());
}



void
EmbedPrivate::AttachListeners(void)
{
  if (!mEventTarget || mListenersAttached)
    return;

  nsIDOMEventListener *eventListener =
    NS_STATIC_CAST(nsIDOMEventListener *,
		   NS_STATIC_CAST(nsIDOMKeyListener *, mEventListener));

  
  nsresult rv;
  rv = mEventTarget->AddEventListenerByIID(eventListener,
                                           NS_GET_IID(nsIDOMKeyListener));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to add key listener\n");
    return;
  }

  rv = mEventTarget->AddEventListenerByIID(eventListener,
                                           NS_GET_IID(nsIDOMMouseListener));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to add mouse listener\n");
    return;
  }

  
  mListenersAttached = PR_TRUE;
}

void
EmbedPrivate::DetachListeners(void)
{
  if (!mListenersAttached || !mEventTarget)
    return;

  nsIDOMEventListener *eventListener =
    NS_STATIC_CAST(nsIDOMEventListener *,
		   NS_STATIC_CAST(nsIDOMKeyListener *, mEventListener));

  nsresult rv;
  rv = mEventTarget->RemoveEventListenerByIID(eventListener,
                                              NS_GET_IID(nsIDOMKeyListener));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to remove key listener\n");
    return;
  }

  rv =
    mEventTarget->RemoveEventListenerByIID(eventListener,
                                           NS_GET_IID(nsIDOMMouseListener));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to remove mouse listener\n");
    return;
  }


  mListenersAttached = PR_FALSE;
}

nsresult
EmbedPrivate::GetPIDOMWindow(nsPIDOMWindow **aPIWin)
{
  *aPIWin = nsnull;

  
  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  
  nsCOMPtr<nsIDOMWindow> domWindow;
  webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
  if (!domWindow)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsPIDOMWindow> domWindowPrivate = do_QueryInterface(domWindow);
  
	*aPIWin = domWindowPrivate->GetPrivateRoot();
  
  if (*aPIWin) {
    NS_ADDREF(*aPIWin);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;

}


static void mozilla_set_default_pref( nsIPref *pref )
{





	
	pref->SetUnicharPref( "intl.charset.default", NS_ConvertASCIItoUTF16("iso8859-1").get());

	
	pref->SetUnicharPref( "browser.visited_color", NS_ConvertASCIItoUTF16("#008080").get() );
	pref->SetUnicharPref( "browser.anchor_color", NS_ConvertASCIItoUTF16("#0000ff").get() );
	pref->SetUnicharPref( "browser.display.foreground_color", NS_ConvertASCIItoUTF16("#000000").get() );
	pref->SetUnicharPref( "browser.display.background_color", NS_ConvertASCIItoUTF16("#ffffff").get() );

	pref->SetCharPref( "font.name.serif.x-western", "serif" );
	pref->SetCharPref( "font.name.sans-serif.x-western", "sans-serif" );
	pref->SetCharPref( "font.name.monospace.x-western", "monospace" );
	pref->SetCharPref( "font.name.cursive.x-western", "cursive" );
	pref->SetCharPref( "font.name.fantasy.x-western", "fantasy" );

	pref->SetBoolPref( "browser.display.use_document_colors", PR_TRUE );
	pref->SetBoolPref( "browser.underline_anchors", PR_TRUE );
	pref->SetIntPref( "font.size.variable.x-western", 14 );
	pref->SetIntPref( "font.size.fixed.x-western", 12 );
	pref->SetIntPref( "browser.history_expire_days", 4 );
	pref->SetIntPref( "browser.sessionhistory.max_entries", 50 );

	pref->SetBoolPref( "browser.cache.disk.enable", PR_TRUE );
	pref->SetIntPref( "browser.cache.disk.capacity", 5000 );
	pref->SetIntPref( "network.http.max-connections", 4 );
	pref->SetIntPref( "network.proxy.http_port", 80 );
	pref->SetIntPref( "network.proxy.ftp_port", 80 );
	pref->SetIntPref( "network.proxy.gopher_port", 80 );

	pref->SetCharPref( "general.skins.selectedSkin", "classic/1.0" );
	pref->SetIntPref( "browser.cache.memory.capacity", 100 ); 
	pref->SetCharPref( "user.print.print_frame", "print_frame_selected" );

	pref->SetCharPref( "print.print_headercenter", "" );
	pref->SetCharPref( "print.print_footercenter", "" );

	pref->SavePrefFile( nsnull );
}



#define FORMAT_ESCAPE_CHARACTER                '&'
void EmbedPrivate::PrintHeaderFooter_FormatSpecialCodes(const char *original, nsString& aNewStr)
{
	

	const char *szPattern = original;

	time_t aclock;
	struct tm *tm;

	char workBuffer[20], *sz;

	nsAutoString result;

	while ( *szPattern )
	{
		if (*szPattern != FORMAT_ESCAPE_CHARACTER)
		{
			workBuffer[0] = *szPattern;
			szPattern++;
			workBuffer[1] = 0;

			nsAutoString ss;
			ss.AssignWithConversion( workBuffer );
			result += ss;
		}
		else
		{
			szPattern++;				
			switch (*szPattern)
			{
			case 'w':					
			case 'W':
				szPattern++;			

				
				PRUnichar *uTitle;
				mWindow->GetTitle( &uTitle );
				result += uTitle;
				break;


			case 'u':					
			case 'U':					
				szPattern++;			

				
				result += mURI.get();
				break;


			case 'd':		
			case 'D':		
				szPattern++;
				
				(void) time(&aclock);
				tm = localtime(&aclock);
				sz = asctime(tm);

				
				
				

				if (szPattern[1] == 'd')
				{
					workBuffer[0] = sz[4];	
					workBuffer[1] = sz[5];	
					workBuffer[2] = sz[6];	
					workBuffer[3] = sz[7];	
					workBuffer[4] = sz[8];	
					workBuffer[5] = sz[9];	
				}
				else
				{
					workBuffer[0] = sz[8];	
					workBuffer[1] = sz[9];	
					workBuffer[2] = sz[7];	
					workBuffer[3] = sz[4];	
					workBuffer[4] = sz[5];	
					workBuffer[5] = sz[6];	
				}
				workBuffer[6] = sz[10];	
				workBuffer[7] = sz[20];	
				workBuffer[8] = sz[21];	
				workBuffer[9] = sz[22];	
				workBuffer[10] = sz[23];	
				workBuffer[11] = 0;

				
				{
				nsAutoString ss;
				ss.AssignWithConversion( workBuffer );
				result += ss;
				}

				break;


			case 't':					
				szPattern++;

				(void) time(&aclock);
				tm = localtime(&aclock);

				strftime(workBuffer, sizeof(workBuffer), "%I:%M %p", tm);

				{
				
				nsAutoString ss;
				ss.AssignWithConversion( workBuffer );
				result += ss;
				}

				break;

				
			case 'T':					
				szPattern++;

				(void) time(&aclock);
				tm = localtime(&aclock);

				strftime(workBuffer, sizeof(workBuffer), "%H:%M", tm);

				
				{
				nsAutoString ss;
				ss.AssignWithConversion( workBuffer );
				result += ss;
				}
				break;

			case 'p':					
				szPattern++;

				{
				
				const PRUnichar * uStr = NS_LITERAL_STRING( "&P" ).get();
				result += uStr;
				}
				break;

			case 'P': 
				
				{
				const PRUnichar * uStr = NS_LITERAL_STRING( "&PT" ).get();
				result += uStr;
				}
				break;

			case FORMAT_ESCAPE_CHARACTER:	

				workBuffer[0] = *szPattern;
				szPattern++;
				workBuffer[1] = 0;
				{
				nsAutoString ss;
				ss.AssignWithConversion( workBuffer );
				result += ss;
				}
				break;

			case '\0':					

				workBuffer[0] = FORMAT_ESCAPE_CHARACTER;
				workBuffer[1] = 0;
				{
				nsAutoString ss;
				ss.AssignWithConversion( workBuffer );
				result += ss;
				}
				break;
				
			default:					
#if 0
				SM_STRNCPY(p,(const char *) &szPattern[-1],lenCopy);
#endif
				szPattern++;

				
				{
				nsAutoString ss;
				ss.AssignWithConversion( &szPattern[-1] );
				result += ss;
				}
				break;
			}
		}
	}

	aNewStr.Assign( result );
}
