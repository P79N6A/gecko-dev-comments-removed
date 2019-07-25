




































#include "nsIDocShell.h"
#include "nsIWebProgress.h"
#include "nsIWidget.h"
#include "nsCRT.h"
#include "nsNetUtil.h"
#include "nsIWebBrowserStream.h"
#include "nsIWebBrowserFocus.h"
#include "nsIDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"


#include "nsIInterfaceRequestor.h"

#include "nsIComponentManager.h"
#include "nsComponentManagerUtils.h"


#include "nsIWindowWatcher.h"

#include "nsILocalFile.h"

#include "nsXULAppAPI.h"



#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMWindowInternal.h"


#include "nsIDOMBarProp.h"


#include "EmbedPrivate.h"
#include "EmbedWindow.h"
#include "EmbedProgress.h"
#include "EmbedContentListener.h"
#include "EmbedEventListener.h"
#include "EmbedWindowCreator.h"
#include "GtkPromptService.h"

#include "mozilla/ModuleUtils.h"

#ifdef MOZ_ACCESSIBILITY_ATK
#include "nsIAccessibilityService.h"
#include "nsIAccessible.h"
#include "nsIDOMDocument.h"
#endif

PRUint32                 EmbedPrivate::sWidgetCount = 0;

char                    *EmbedPrivate::sPath        = nsnull;
char                    *EmbedPrivate::sCompPath    = nsnull;
nsTArray<EmbedPrivate*> *EmbedPrivate::sWindowList  = nsnull;
nsILocalFile            *EmbedPrivate::sProfileDir  = nsnull;
nsISupports             *EmbedPrivate::sProfileLock = nsnull;
GtkWidget               *EmbedPrivate::sOffscreenWindow = 0;
GtkWidget               *EmbedPrivate::sOffscreenFixed  = 0;

nsIDirectoryServiceProvider *EmbedPrivate::sAppFileLocProvider = nsnull;

class GTKEmbedDirectoryProvider : public nsIDirectoryServiceProvider2
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2
};

static const GTKEmbedDirectoryProvider kDirectoryProvider;

NS_IMPL_QUERY_INTERFACE2(GTKEmbedDirectoryProvider,
                         nsIDirectoryServiceProvider,
                         nsIDirectoryServiceProvider2)

NS_IMETHODIMP_(nsrefcnt)
GTKEmbedDirectoryProvider::AddRef()
{
  return 1;
}

NS_IMETHODIMP_(nsrefcnt)
GTKEmbedDirectoryProvider::Release()
{
  return 1;
}

NS_IMETHODIMP
GTKEmbedDirectoryProvider::GetFile(const char *aKey, PRBool *aPersist,
                                   nsIFile* *aResult)
{
  if (EmbedPrivate::sAppFileLocProvider) {
    nsresult rv = EmbedPrivate::sAppFileLocProvider->GetFile(aKey, aPersist,
                                                             aResult);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  if (EmbedPrivate::sProfileDir && (!strcmp(aKey, NS_APP_USER_PROFILE_50_DIR)
                                 || !strcmp(aKey, NS_APP_PROFILE_DIR_STARTUP))) {
    *aPersist = PR_TRUE;
    return EmbedPrivate::sProfileDir->Clone(aResult);
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
GTKEmbedDirectoryProvider::GetFiles(const char *aKey,
                                    nsISimpleEnumerator* *aResult)
{
  nsCOMPtr<nsIDirectoryServiceProvider2>
    dp2(do_QueryInterface(EmbedPrivate::sAppFileLocProvider));

  if (!dp2)
    return NS_ERROR_FAILURE;

  return dp2->GetFiles(aKey, aResult);
}

#define NS_PROMPTSERVICE_CID \
 {0x95611356, 0xf583, 0x46f5, {0x81, 0xff, 0x4b, 0x3e, 0x01, 0x62, 0xc6, 0x19}}

NS_GENERIC_FACTORY_CONSTRUCTOR(GtkPromptService)
NS_DEFINE_NAMED_CID(NS_PROMPTSERVICE_CID);

static const mozilla::Module::CIDEntry kDefaultPromptCIDs[] = {
  { &kNS_PROMPTSERVICE_CID, false, NULL, GtkPromptServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDefaultPromptContracts[] = {
  { "@mozilla.org/embedcomp/prompt-service;1", &kNS_PROMPTSERVICE_CID },
  { NULL }
};

static const mozilla::Module kDefaultPromptModule = {
  mozilla::Module::kVersion,
  kDefaultPromptCIDs,
  kDefaultPromptContracts
};

EmbedPrivate::EmbedPrivate(void)
{
  mOwningWidget     = nsnull;
  mWindow           = nsnull;
  mProgress         = nsnull;
  mContentListener  = nsnull;
  mEventListener    = nsnull;
  mChromeMask       = nsIWebBrowserChrome::CHROME_ALL;
  mIsChrome         = PR_FALSE;
  mChromeLoaded     = PR_FALSE;
  mListenersAttached = PR_FALSE;
  mMozWindowWidget  = 0;
  mIsDestroyed      = PR_FALSE;

  PushStartup();
  if (!sWindowList) {
    sWindowList = new nsTArray<EmbedPrivate*>();
  }
  sWindowList->AppendElement(this);
}

EmbedPrivate::~EmbedPrivate()
{
  sWindowList->RemoveElement(this);
  PopStartup();
}

nsresult
EmbedPrivate::Init(GtkMozEmbed *aOwningWidget)
{
  
  if (mOwningWidget)
    return NS_OK;

  
  mOwningWidget = aOwningWidget;

  
  
  
  mWindow = new EmbedWindow();
  mWindowGuard = static_cast<nsIWebBrowserChrome *>(mWindow);
  mWindow->Init(this);

  
  
  
  mProgress = new EmbedProgress();
  mProgressGuard = static_cast<nsIWebProgressListener *>
                                (mProgress);
  mProgress->Init(this);

  
  
  
  mContentListener = new EmbedContentListener();
  mContentListenerGuard = static_cast<nsISupports*>(static_cast<nsIURIContentListener*>(mContentListener));
  mContentListener->Init(this);

  
  
  mEventListener = new EmbedEventListener();
  mEventListenerGuard =
    static_cast<nsISupports *>(static_cast<nsIDOMKeyListener *>(mEventListener));
  mEventListener->Init(this);

  
  static int initialized = PR_FALSE;
  
  if (!initialized) {
    
    
    initialized = PR_TRUE;

    
    EmbedWindowCreator *creator = new EmbedWindowCreator();
    nsCOMPtr<nsIWindowCreator> windowCreator;
    windowCreator = static_cast<nsIWindowCreator *>(creator);

    
    nsCOMPtr<nsIWindowWatcher> watcher = do_GetService(NS_WINDOWWATCHER_CONTRACTID);
    if (watcher)
      watcher->SetWindowCreator(windowCreator);
  }
  return NS_OK;
}

nsresult
EmbedPrivate::Realize(PRBool *aAlreadyRealized)
{

  *aAlreadyRealized = PR_FALSE;

  
  EnsureOffscreenWindow();

  
  
  if (mMozWindowWidget) {
    gtk_widget_reparent(mMozWindowWidget, GTK_WIDGET(mOwningWidget));
    *aAlreadyRealized = PR_TRUE;
    return NS_OK;
  }

  
  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  
  mNavigation = do_QueryInterface(webBrowser);

  
  
  
  mSessionHistory = do_CreateInstance(NS_SHISTORY_CONTRACTID);
  mNavigation->SetSessionHistory(mSessionHistory);

  
  mWindow->CreateWindow();

  
  nsCOMPtr<nsISupportsWeakReference> supportsWeak;
  supportsWeak = do_QueryInterface(mProgressGuard);
  nsCOMPtr<nsIWeakReference> weakRef;
  supportsWeak->GetWeakReference(getter_AddRefs(weakRef));
  webBrowser->AddWebBrowserListener(weakRef,
				    NS_GET_IID(nsIWebProgressListener));

  
  nsCOMPtr<nsIURIContentListener> uriListener;
  uriListener = do_QueryInterface(mContentListenerGuard);
  webBrowser->SetParentURIContentListener(uriListener);

  
  nsCOMPtr<nsIWidget> mozWidget;
  mWindow->mBaseWindow->GetMainWidget(getter_AddRefs(mozWidget));
  
  GdkWindow *tmp_window =
    static_cast<GdkWindow *>(
		   mozWidget->GetNativeData(NS_NATIVE_WINDOW));
  
  tmp_window = gdk_window_get_parent(tmp_window);
  
  gpointer data = nsnull;
  gdk_window_get_user_data(tmp_window, &data);
  mMozWindowWidget = static_cast<GtkWidget *>(data);

  
  ApplyChromeMask();

  return NS_OK;
}

void
EmbedPrivate::Unrealize(void)
{
  
  gtk_widget_reparent(mMozWindowWidget, sOffscreenFixed);
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
EmbedPrivate::Resize(PRUint32 aWidth, PRUint32 aHeight)
{
  mWindow->SetDimensions(nsIEmbeddingSiteWindow::DIM_FLAGS_POSITION |
			 nsIEmbeddingSiteWindow::DIM_FLAGS_SIZE_INNER,
			 0, 0, aWidth, aHeight);
}

void
EmbedPrivate::Destroy(void)
{
  
  
  
  
  mIsDestroyed = PR_TRUE;

  
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

  
  mWindow->ReleaseChildren();

  
  mNavigation = nsnull;

  
  mSessionHistory = nsnull;

  mOwningWidget = nsnull;

  mMozWindowWidget = 0;
}

void
EmbedPrivate::SetURI(const char *aURI)
{
  mURI.Assign(aURI);
}

void
EmbedPrivate::LoadCurrentURI(void)
{
  if (mURI.Length()) {
    nsCOMPtr<nsPIDOMWindow> piWin;
    GetPIDOMWindow(getter_AddRefs(piWin));

    nsAutoPopupStatePusher popupStatePusher(piWin, openAllowed);

    mNavigation->LoadURI(NS_ConvertUTF8toUTF16(mURI).get(), 
                         nsIWebNavigation::LOAD_FLAGS_NONE | 
                         nsIWebNavigation::LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP,  
                         nsnull,                            
                         nsnull,                            
                         nsnull);                           
  }
}

void
EmbedPrivate::Reload(PRUint32 reloadFlags)
{
  

  nsCOMPtr<nsIWebNavigation> wn;

  if (mSessionHistory) {
    wn = do_QueryInterface(mSessionHistory);
  }
  if (!wn)
    wn = mNavigation;

  if (wn)
    wn->Reload(reloadFlags);
}


void
EmbedPrivate::ApplyChromeMask()
{
   if (mWindow) {
      nsCOMPtr<nsIWebBrowser> webBrowser;
      mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

      nsCOMPtr<nsIDOMWindow> domWindow;
      webBrowser->GetContentDOMWindow(getter_AddRefs(domWindow));
      if (domWindow) {

         nsCOMPtr<nsIDOMBarProp> scrollbars;
         domWindow->GetScrollbars(getter_AddRefs(scrollbars));
         if (scrollbars) {

            scrollbars->SetVisible
               (mChromeMask & nsIWebBrowserChrome::CHROME_SCROLLBARS ?
                PR_TRUE : PR_FALSE);
         }
      }
   }
}


void
EmbedPrivate::SetChromeMask(PRUint32 aChromeMask)
{
   mChromeMask = aChromeMask;

   ApplyChromeMask();
}



void
EmbedPrivate::PushStartup(void)
{
  
  sWidgetCount++;

  
  if (sWidgetCount == 1) {
    nsresult rv;

    nsCOMPtr<nsILocalFile> binDir;
    if (sCompPath) {
      rv = NS_NewNativeLocalFile(nsDependentCString(sCompPath), 1, getter_AddRefs(binDir));
      if (NS_FAILED(rv))
	return;
    }

    const char *grePath = sPath;

    if (!grePath)
      grePath = getenv("MOZILLA_FIVE_HOME");

    if (!grePath)
      return;

    nsCOMPtr<nsILocalFile> greDir;
    rv = NS_NewNativeLocalFile(nsDependentCString(grePath), PR_TRUE,
                               getter_AddRefs(greDir));
    if (NS_FAILED(rv))
      return;

    if (sProfileDir && !sProfileLock) {
      rv = XRE_LockProfileDirectory(sProfileDir,
                                    &sProfileLock);
      if (NS_FAILED(rv)) return;
    }

    rv = XRE_InitEmbedding2(greDir, binDir,
                            const_cast<GTKEmbedDirectoryProvider*>(&kDirectoryProvider));
    if (NS_FAILED(rv))
      return;

    if (sProfileDir)
      XRE_NotifyProfile();

    RegisterAppComponents();
  }
}


void
EmbedPrivate::PopStartup(void)
{
  sWidgetCount--;
  if (sWidgetCount == 0) {

    
    DestroyOffscreenWindow();

    
    if (sAppFileLocProvider) {
      NS_RELEASE(sAppFileLocProvider);
      sAppFileLocProvider = nsnull;
    }

    
    XRE_TermEmbedding();

    NS_IF_RELEASE(sProfileLock);
    NS_IF_RELEASE(sProfileDir);
  }
}


void EmbedPrivate::SetPath(const char *aPath)
{
  if (sPath)
    free(sPath);
  if (aPath)
    sPath = strdup(aPath);
  else
    sPath = nsnull;
}


void
EmbedPrivate::SetCompPath(const char *aPath)
{
  if (sCompPath)
    free(sCompPath);
  if (aPath)
    sCompPath = strdup(aPath);
  else
    sCompPath = nsnull;
}


void
EmbedPrivate::SetProfilePath(const char *aDir, const char *aName)
{
  if (sProfileDir) {
    if (sWidgetCount) {
      NS_ERROR("Cannot change profile directory during run.");
      return;
    }

    NS_RELEASE(sProfileDir);
    NS_RELEASE(sProfileLock);
  }

  nsresult rv =
    NS_NewNativeLocalFile(nsDependentCString(aDir), PR_TRUE, &sProfileDir);

  if (NS_SUCCEEDED(rv) && aName)
    rv = sProfileDir->AppendNative(nsDependentCString(aName));

  if (NS_SUCCEEDED(rv)) {
    PRBool exists = PR_FALSE;
    rv = sProfileDir->Exists(&exists);
    if (!exists)
      rv = sProfileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    rv = XRE_LockProfileDirectory(sProfileDir, &sProfileLock);
  }

  if (NS_SUCCEEDED(rv)) {
    if (sWidgetCount)
      XRE_NotifyProfile();

    return;
  }

  NS_WARNING("Failed to lock profile.");

  
  NS_IF_RELEASE(sProfileDir);
  NS_IF_RELEASE(sProfileLock);
}

void
EmbedPrivate::SetDirectoryServiceProvider(nsIDirectoryServiceProvider * appFileLocProvider)
{
  if (sAppFileLocProvider)
    NS_RELEASE(sAppFileLocProvider);

  if (appFileLocProvider) {
    sAppFileLocProvider = appFileLocProvider;
    NS_ADDREF(sAppFileLocProvider);
  }
}

nsresult
EmbedPrivate::OpenStream(const char *aBaseURI, const char *aContentType)
{
  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  nsCOMPtr<nsIWebBrowserStream> wbStream = do_QueryInterface(webBrowser);
  if (!wbStream) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aBaseURI);
  if (NS_FAILED(rv))
    return rv;

  rv = wbStream->OpenStream(uri, nsDependentCString(aContentType));
  return rv;
}

nsresult
EmbedPrivate::AppendToStream(const PRUint8 *aData, PRUint32 aLen)
{
  
  
  ContentStateChange();

  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  nsCOMPtr<nsIWebBrowserStream> wbStream = do_QueryInterface(webBrowser);
  if (!wbStream) return NS_ERROR_FAILURE;

  return wbStream->AppendToStream(aData, aLen);
}

nsresult
EmbedPrivate::CloseStream(void)
{
  nsCOMPtr<nsIWebBrowser> webBrowser;
  mWindow->GetWebBrowser(getter_AddRefs(webBrowser));

  nsCOMPtr<nsIWebBrowserStream> wbStream = do_QueryInterface(webBrowser);
  if (!wbStream) return NS_ERROR_FAILURE;

  return wbStream->CloseStream();
}


EmbedPrivate *
EmbedPrivate::FindPrivateForBrowser(nsIWebBrowserChrome *aBrowser)
{
  if (!sWindowList)
    return nsnull;

  
  PRInt32 count = sWindowList->Length();
  
  
  
  for (int i = 0; i < count; i++) {
    EmbedPrivate *tmpPrivate = sWindowList->ElementAt(i);
    
    nsIWebBrowserChrome *chrome = static_cast<nsIWebBrowserChrome *>(
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

void
EmbedPrivate::ChildFocusIn(void)
{
  if (mIsDestroyed)
    return;

  nsresult rv;
  nsCOMPtr<nsIWebBrowser> webBrowser;
  rv = mWindow->GetWebBrowser(getter_AddRefs(webBrowser));
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsIWebBrowserFocus> webBrowserFocus(do_QueryInterface(webBrowser));
  if (!webBrowserFocus)
    return;
  
  webBrowserFocus->Activate();
}

void
EmbedPrivate::ChildFocusOut(void)
{
  if (mIsDestroyed)
    return;

  nsresult rv;
  nsCOMPtr<nsIWebBrowser> webBrowser;
  rv = mWindow->GetWebBrowser(getter_AddRefs(webBrowser));
  if (NS_FAILED(rv))
	  return;

  nsCOMPtr<nsIWebBrowserFocus> webBrowserFocus(do_QueryInterface(webBrowser));
  if (!webBrowserFocus)
	  return;
  
  webBrowserFocus->Deactivate();
}



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
    static_cast<nsIDOMEventListener *>(static_cast<nsIDOMKeyListener *>(mEventListener));

  
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

  rv = mEventTarget->AddEventListenerByIID(eventListener,
                                             NS_GET_IID(nsIDOMUIListener));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to add UI listener\n");
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
    static_cast<nsIDOMEventListener *>(static_cast<nsIDOMKeyListener *>(mEventListener));

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

  rv = mEventTarget->RemoveEventListenerByIID(eventListener,
						NS_GET_IID(nsIDOMUIListener));
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to remove UI listener\n");
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

#ifdef MOZ_ACCESSIBILITY_ATK
void *
EmbedPrivate::GetAtkObjectForCurrentDocument()
{
  if (!mNavigation)
    return nsnull;

  nsCOMPtr<nsIAccessibilityService> accService =
    do_GetService("@mozilla.org/accessibilityService;1");
  if (accService) {
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    mNavigation->GetDocument(getter_AddRefs(domDoc));
    NS_ENSURE_TRUE(domDoc, nsnull);

    nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(domDoc));
    NS_ENSURE_TRUE(domNode, nsnull);

    nsCOMPtr<nsIAccessible> acc;
    accService->GetAccessibleFor(domNode, getter_AddRefs(acc));
    NS_ENSURE_TRUE(acc, nsnull);

    void *atkObj = nsnull;
    if (NS_SUCCEEDED(acc->GetNativeInterface(&atkObj)))
      return atkObj;
  }
  return nsnull;
}
#endif 


void
EmbedPrivate::RegisterAppComponents(void)
{
  XRE_AddStaticComponent(&kDefaultPromptModule);
}


void
EmbedPrivate::EnsureOffscreenWindow(void)
{
  if (sOffscreenWindow)
    return;
  sOffscreenWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize(sOffscreenWindow);
  sOffscreenFixed = gtk_fixed_new();
  gtk_container_add(GTK_CONTAINER(sOffscreenWindow), sOffscreenFixed);
  gtk_widget_realize(sOffscreenFixed);
}


void
EmbedPrivate::DestroyOffscreenWindow(void)
{
  if (!sOffscreenWindow)
    return;
  gtk_widget_destroy(sOffscreenWindow);
  sOffscreenWindow = 0;
}
