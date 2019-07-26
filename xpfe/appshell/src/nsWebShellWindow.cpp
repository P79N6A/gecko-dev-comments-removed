





#include "nsWebShellWindow.h"

#include "nsLayoutCID.h"
#include "nsContentCID.h"
#include "nsIWeakReference.h"
#include "nsIContentViewer.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIIOService.h"
#include "nsIURL.h"
#include "nsNetCID.h"
#include "nsIStringBundle.h"
#include "nsReadableUtils.h"

#include "nsEscape.h"
#include "nsPIDOMWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWindowWatcher.h"

#include "nsIDOMXULElement.h"

#include "nsWidgetsCID.h"
#include "nsIWidget.h"
#include "nsIWidgetListener.h"

#include "nsIDOMCharacterData.h"
#include "nsIDOMNodeList.h"

#include "nsITimer.h"
#include "nsXULPopupManager.h"


#include "nsIDOMXULDocument.h"

#include "nsFocusManager.h"

#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"

#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIObserverService.h"
#include "prprf.h"

#include "nsIScreenManager.h"
#include "nsIScreen.h"

#include "nsIContent.h" 
#include "nsIScriptSecurityManager.h"


#include "nsIPresShell.h"
#include "nsPresContext.h"

#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"

#include "nsIMarkupDocumentViewer.h"
#include "mozilla/Attributes.h"
#include "mozilla/MouseEvents.h"

#ifdef XP_MACOSX
#include "nsINativeMenuService.h"
#define USE_NATIVE_MENUS
#endif

using namespace mozilla;
using namespace mozilla::dom;


static NS_DEFINE_CID(kWindowCID,           NS_WINDOW_CID);

#define SIZE_PERSISTENCE_TIMEOUT 500 // msec

nsWebShellWindow::nsWebShellWindow(uint32_t aChromeFlags)
  : nsXULWindow(aChromeFlags)
  , mSPTimerLock("nsWebShellWindow.mSPTimerLock")
{
}


nsWebShellWindow::~nsWebShellWindow()
{
  MutexAutoLock lock(mSPTimerLock);
  if (mSPTimer)
    mSPTimer->Cancel();
}

NS_IMPL_ADDREF_INHERITED(nsWebShellWindow, nsXULWindow)
NS_IMPL_RELEASE_INHERITED(nsWebShellWindow, nsXULWindow)

NS_INTERFACE_MAP_BEGIN(nsWebShellWindow)
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
NS_INTERFACE_MAP_END_INHERITING(nsXULWindow)

nsresult nsWebShellWindow::Initialize(nsIXULWindow* aParent,
                                      nsIXULWindow* aOpener,
                                      nsIURI* aUrl,
                                      int32_t aInitialWidth,
                                      int32_t aInitialHeight,
                                      bool aIsHiddenWindow,
                                      nsWidgetInitData& widgetInitData)
{
  nsresult rv;
  nsCOMPtr<nsIWidget> parentWidget;

  mIsHiddenWindow = aIsHiddenWindow;

  int32_t initialX = 0, initialY = 0;
  nsCOMPtr<nsIBaseWindow> base(do_QueryInterface(aOpener));
  if (base) {
    rv = base->GetPositionAndSize(&mOpenerScreenRect.x,
                                  &mOpenerScreenRect.y,
                                  &mOpenerScreenRect.width,
                                  &mOpenerScreenRect.height);
    if (NS_FAILED(rv)) {
      mOpenerScreenRect.SetEmpty();
    } else {
      double scale;
      if (NS_SUCCEEDED(base->GetUnscaledDevicePixelsPerCSSPixel(&scale))) {
        mOpenerScreenRect.x = NSToIntRound(mOpenerScreenRect.x / scale);
        mOpenerScreenRect.y = NSToIntRound(mOpenerScreenRect.y / scale);
        mOpenerScreenRect.width = NSToIntRound(mOpenerScreenRect.width / scale);
        mOpenerScreenRect.height = NSToIntRound(mOpenerScreenRect.height / scale);
      }
      initialX = mOpenerScreenRect.x;
      initialY = mOpenerScreenRect.y;
      ConstrainToOpenerScreen(&initialX, &initialY);
    }
  }

  
  
  nsIntRect r(initialX, initialY, aInitialWidth, aInitialHeight);
  
  
  mWindow = do_CreateInstance(kWindowCID, &rv);
  if (NS_OK != rv) {
    return rv;
  }

  










  nsCOMPtr<nsIBaseWindow> parentAsWin(do_QueryInterface(aParent));
  if (parentAsWin) {
    parentAsWin->GetMainWidget(getter_AddRefs(parentWidget));
    mParentWindow = do_GetWeakReference(aParent);
  }

  mWindow->SetWidgetListener(this);
  mWindow->Create((nsIWidget *)parentWidget,          
                  nullptr,                            
                  r,                                  
                  nullptr,                            
                  &widgetInitData);                   
  mWindow->GetClientBounds(r);
  
  
  mWindow->SetBackgroundColor(NS_RGB(255,255,255));

  
  mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  
  
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(EnsureChromeTreeOwner(), NS_ERROR_FAILURE);

  docShellAsItem->SetTreeOwner(mChromeTreeOwner);
  docShellAsItem->SetItemType(nsIDocShellTreeItem::typeChrome);

  r.x = r.y = 0;
  nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
  NS_ENSURE_SUCCESS(docShellAsWin->InitWindow(nullptr, mWindow, 
   r.x, r.y, r.width, r.height), NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(docShellAsWin->Create(), NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWebProgress> webProgress(do_GetInterface(mDocShell, &rv));
  if (webProgress) {
    webProgress->AddProgressListener(this, nsIWebProgress::NOTIFY_STATE_NETWORK);
  }

  
  
  
  
  
  nsCOMPtr<nsIScriptSecurityManager> ssm =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  if (ssm) { 
    nsCOMPtr<nsIPrincipal> principal;
    ssm->GetSubjectPrincipal(getter_AddRefs(principal));
    if (!principal) {
      ssm->GetSystemPrincipal(getter_AddRefs(principal));
    }
    rv = mDocShell->CreateAboutBlankContentViewer(principal);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> doc = do_GetInterface(mDocShell);
    NS_ENSURE_TRUE(!!doc, NS_ERROR_FAILURE);
    doc->SetIsInitialDocument(true);
  }

  if (nullptr != aUrl)  {
    nsCString tmpStr;

    rv = aUrl->GetSpec(tmpStr);
    if (NS_FAILED(rv)) return rv;

    NS_ConvertUTF8toUTF16 urlString(tmpStr);
    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
    NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
    rv = webNav->LoadURI(urlString.get(),
                         nsIWebNavigation::LOAD_FLAGS_NONE,
                         nullptr,
                         nullptr,
                         nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
  }
                     
  return rv;
}

nsIPresShell*
nsWebShellWindow::GetPresShell()
{
  if (!mDocShell)
    return nullptr;

  return mDocShell->GetPresShell();
}

bool
nsWebShellWindow::WindowMoved(nsIWidget* aWidget, int32_t x, int32_t y)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mDocShell);
    pm->AdjustPopupsOnWindowChange(window);
  }

  
  
  SetPersistenceTimer(PAD_POSITION);
  return false;
}

bool
nsWebShellWindow::WindowResized(nsIWidget* aWidget, int32_t aWidth, int32_t aHeight)
{
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (pm) {
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mDocShell);
    pm->AdjustPopupsOnWindowChange(window);
  }

  nsCOMPtr<nsIBaseWindow> shellAsWin(do_QueryInterface(mDocShell));
  if (shellAsWin) {
    shellAsWin->SetPositionAndSize(0, 0, aWidth, aHeight, false);
  }
  
  
  if (!IsLocked())
    SetPersistenceTimer(PAD_POSITION | PAD_SIZE | PAD_MISC);
  return true;
}

bool
nsWebShellWindow::RequestWindowClose(nsIWidget* aWidget)
{
  
  nsCOMPtr<nsIXULWindow> xulWindow(this);

  nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(mDocShell));
  nsCOMPtr<EventTarget> eventTarget = do_QueryInterface(window);

  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();

  if (!presShell) {
    bool dying;
    MOZ_ASSERT(NS_SUCCEEDED(mDocShell->IsBeingDestroyed(&dying)) && dying,
               "No presShell, but window is not being destroyed");
  } else if (eventTarget) {
    nsRefPtr<nsPresContext> presContext = presShell->GetPresContext();

    nsEventStatus status = nsEventStatus_eIgnore;
    WidgetMouseEvent event(true, NS_XUL_CLOSE, nullptr,
                           WidgetMouseEvent::eReal);
    if (NS_SUCCEEDED(eventTarget->DispatchDOMEvent(&event, nullptr, presContext, &status)) &&
        status == nsEventStatus_eConsumeNoDefault)
      return false;
  }

  Destroy();
  return false;
}

void
nsWebShellWindow::SizeModeChanged(nsSizeMode sizeMode)
{
  
  
  
  
  if (sizeMode == nsSizeMode_Maximized || sizeMode == nsSizeMode_Fullscreen) {
    uint32_t zLevel;
    GetZLevel(&zLevel);
    if (zLevel > nsIXULWindow::normalZ)
      SetZLevel(nsIXULWindow::normalZ);
  }
  mWindow->SetSizeMode(sizeMode);

  
  
  
  SetPersistenceTimer(PAD_MISC);
  nsCOMPtr<nsPIDOMWindow> ourWindow = do_GetInterface(mDocShell);
  if (ourWindow) {
    
    
    if (sizeMode == nsSizeMode_Fullscreen) {
      ourWindow->SetFullScreen(true);
    }
    else if (sizeMode != nsSizeMode_Minimized) {
      ourWindow->SetFullScreen(false);
    }

    
    ourWindow->DispatchCustomEvent("sizemodechange");
  }

  
  
  
  
  
}

void
nsWebShellWindow::OSToolbarButtonPressed()
{
  
  nsCOMPtr<nsIXULWindow> xulWindow(this);

  
  
  
  
  uint32_t    chromeMask = (nsIWebBrowserChrome::CHROME_TOOLBAR |
                            nsIWebBrowserChrome::CHROME_LOCATIONBAR |
                            nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);

  nsCOMPtr<nsIWebBrowserChrome> wbc(do_GetInterface(xulWindow));
  if (!wbc)
    return;

  uint32_t    chromeFlags, newChromeFlags = 0;
  wbc->GetChromeFlags(&chromeFlags);
  newChromeFlags = chromeFlags & chromeMask;
  if (!newChromeFlags)    chromeFlags |= chromeMask;
  else                    chromeFlags &= (~newChromeFlags);
  wbc->SetChromeFlags(chromeFlags);
}

bool
nsWebShellWindow::ZLevelChanged(bool aImmediate, nsWindowZ *aPlacement,
                                nsIWidget* aRequestBelow, nsIWidget** aActualBelow)
{
  if (aActualBelow)
    *aActualBelow = nullptr;

  return ConstrainToZLevel(aImmediate, aPlacement, aRequestBelow, aActualBelow);
}

void
nsWebShellWindow::WindowActivated()
{
  nsCOMPtr<nsIXULWindow> xulWindow(this);

  
  nsCOMPtr<nsIDOMWindow> window = do_GetInterface(mDocShell);
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm && window)
    fm->WindowRaised(window);

  if (mChromeLoaded) {
    PersistentAttributesDirty(PAD_POSITION | PAD_SIZE | PAD_MISC);
    SavePersistentAttributes();
   }
}

void
nsWebShellWindow::WindowDeactivated()
{
  nsCOMPtr<nsIXULWindow> xulWindow(this);

  nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mDocShell);
  nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
  if (fm && window)
    fm->WindowLowered(window);
}

#ifdef USE_NATIVE_MENUS
static void LoadNativeMenus(nsIDOMDocument *aDOMDoc, nsIWidget *aParentWindow)
{
  
  
  nsCOMPtr<nsIDOMNodeList> menubarElements;
  aDOMDoc->GetElementsByTagNameNS(NS_LITERAL_STRING("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul"),
                                  NS_LITERAL_STRING("menubar"),
                                  getter_AddRefs(menubarElements));

  nsCOMPtr<nsIDOMNode> menubarNode;
  if (menubarElements)
    menubarElements->Item(0, getter_AddRefs(menubarNode));
  if (!menubarNode)
    return;

  nsCOMPtr<nsINativeMenuService> nms = do_GetService("@mozilla.org/widget/nativemenuservice;1");
  nsCOMPtr<nsIContent> menubarContent(do_QueryInterface(menubarNode));
  if (nms && menubarContent)
    nms->CreateNativeMenuBar(aParentWindow, menubarContent);
}
#endif

namespace mozilla {

class WebShellWindowTimerCallback MOZ_FINAL : public nsITimerCallback
{
public:
  WebShellWindowTimerCallback(nsWebShellWindow* aWindow)
    : mWindow(aWindow)
  {}

  NS_DECL_THREADSAFE_ISUPPORTS

  NS_IMETHOD Notify(nsITimer* aTimer)
  {
    
    
    

    mWindow->FirePersistenceTimer();
    return NS_OK;
  }

private:
  nsRefPtr<nsWebShellWindow> mWindow;
};

NS_IMPL_ISUPPORTS1(WebShellWindowTimerCallback, nsITimerCallback)

} 

void
nsWebShellWindow::SetPersistenceTimer(uint32_t aDirtyFlags)
{
  MutexAutoLock lock(mSPTimerLock);
  if (!mSPTimer) {
    mSPTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mSPTimer) {
      NS_WARNING("Couldn't create @mozilla.org/timer;1 instance?");
      return;
    }
  }

  nsRefPtr<WebShellWindowTimerCallback> callback =
    new WebShellWindowTimerCallback(this);
  mSPTimer->InitWithCallback(callback, SIZE_PERSISTENCE_TIMEOUT,
                             nsITimer::TYPE_ONE_SHOT);

  PersistentAttributesDirty(aDirtyFlags);
}

void
nsWebShellWindow::FirePersistenceTimer()
{
  MutexAutoLock lock(mSPTimerLock);
  SavePersistentAttributes();
}





NS_IMETHODIMP
nsWebShellWindow::OnProgressChange(nsIWebProgress *aProgress,
                                   nsIRequest *aRequest,
                                   int32_t aCurSelfProgress,
                                   int32_t aMaxSelfProgress,
                                   int32_t aCurTotalProgress,
                                   int32_t aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnStateChange(nsIWebProgress *aProgress,
                                nsIRequest *aRequest,
                                uint32_t aStateFlags,
                                nsresult aStatus)
{
  
  
  if (!(aStateFlags & nsIWebProgressListener::STATE_STOP) || 
      !(aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK)) {
    return NS_OK;
  }

  if (mChromeLoaded)
    return NS_OK;

  
  nsCOMPtr<nsIDOMWindow> eventWin;
  aProgress->GetDOMWindow(getter_AddRefs(eventWin));
  nsCOMPtr<nsPIDOMWindow> eventPWin(do_QueryInterface(eventWin));
  if (eventPWin) {
    nsPIDOMWindow *rootPWin = eventPWin->GetPrivateRoot();
    if (eventPWin != rootPWin)
      return NS_OK;
  }

  mChromeLoaded = true;
  mLockedUntilChromeLoad = false;

#ifdef USE_NATIVE_MENUS
  
  
  
  nsCOMPtr<nsIContentViewer> cv;
  mDocShell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
    nsCOMPtr<nsIDOMDocument> menubarDOMDoc(do_QueryInterface(cv->GetDocument()));
    if (menubarDOMDoc)
      LoadNativeMenus(menubarDOMDoc, mWindow);
  }
#endif 

  OnChromeLoaded();
  LoadContentAreas();

  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnLocationChange(nsIWebProgress *aProgress,
                                   nsIRequest *aRequest,
                                   nsIURI *aURI,
                                   uint32_t aFlags)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP 
nsWebShellWindow::OnStatusChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnSecurityChange(nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   uint32_t state)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}





void nsWebShellWindow::LoadContentAreas() {

  nsAutoString searchSpec;

  
  nsCOMPtr<nsIContentViewer> contentViewer;
  
  
  if (mDocShell)
    mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (contentViewer) {
    nsIDocument* doc = contentViewer->GetDocument();
    if (doc) {
      nsIURI* mainURL = doc->GetDocumentURI();

      nsCOMPtr<nsIURL> url = do_QueryInterface(mainURL);
      if (url) {
        nsAutoCString search;
        url->GetQuery(search);

        AppendUTF8toUTF16(search, searchSpec);
      }
    }
  }

  
  
  if (!searchSpec.IsEmpty()) {
    int32_t     begPos,
                eqPos,
                endPos;
    nsString    contentAreaID,
                contentURL;
    char        *urlChar;
    nsresult rv;
    for (endPos = 0; endPos < (int32_t)searchSpec.Length(); ) {
      
      begPos = endPos;
      eqPos = searchSpec.FindChar('=', begPos);
      if (eqPos < 0)
        break;

      endPos = searchSpec.FindChar(';', eqPos);
      if (endPos < 0)
        endPos = searchSpec.Length();
      searchSpec.Mid(contentAreaID, begPos, eqPos-begPos);
      searchSpec.Mid(contentURL, eqPos+1, endPos-eqPos-1);
      endPos++;

      
      nsCOMPtr<nsIDocShellTreeItem> content;
      rv = GetContentShellById(contentAreaID.get(), getter_AddRefs(content));
      if (NS_SUCCEEDED(rv) && content) {
        nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(content));
        if (webNav) {
          urlChar = ToNewCString(contentURL);
          if (urlChar) {
            nsUnescape(urlChar);
            contentURL.AssignWithConversion(urlChar);
            webNav->LoadURI(contentURL.get(),
                          nsIWebNavigation::LOAD_FLAGS_NONE,
                          nullptr,
                          nullptr,
                          nullptr);
            nsMemory::Free(urlChar);
          }
        }
      }
    }
  }
}





bool nsWebShellWindow::ExecuteCloseHandler()
{
  




  nsCOMPtr<nsIXULWindow> kungFuDeathGrip(this);

  nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(mDocShell));
  nsCOMPtr<EventTarget> eventTarget = do_QueryInterface(window);

  if (eventTarget) {
    nsCOMPtr<nsIContentViewer> contentViewer;
    mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
    if (contentViewer) {
      nsRefPtr<nsPresContext> presContext;
      contentViewer->GetPresContext(getter_AddRefs(presContext));

      nsEventStatus status = nsEventStatus_eIgnore;
      WidgetMouseEvent event(true, NS_XUL_CLOSE, nullptr,
                             WidgetMouseEvent::eReal);

      nsresult rv =
        eventTarget->DispatchDOMEvent(&event, nullptr, presContext, &status);
      if (NS_SUCCEEDED(rv) && status == nsEventStatus_eConsumeNoDefault)
        return true;
      
    }
  }

  return false;
} 

void nsWebShellWindow::ConstrainToOpenerScreen(int32_t* aX, int32_t* aY)
{
  if (mOpenerScreenRect.IsEmpty()) {
    *aX = *aY = 0;
    return;
  }

  int32_t left, top, width, height;
  
  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService("@mozilla.org/gfx/screenmanager;1");
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    screenmgr->ScreenForRect(mOpenerScreenRect.x, mOpenerScreenRect.y,
                             mOpenerScreenRect.width, mOpenerScreenRect.height,
                             getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailRectDisplayPix(&left, &top, &width, &height);
      if (*aX < left || *aX > left + width) {
        *aX = left;
      }
      if (*aY < top || *aY > top + height) {
        *aY = top;
      }
    }
  }
}


NS_IMETHODIMP nsWebShellWindow::Destroy()
{
  nsresult rv;
  nsCOMPtr<nsIWebProgress> webProgress(do_GetInterface(mDocShell, &rv));
  if (webProgress) {
    webProgress->RemoveProgressListener(this);
  }

  nsCOMPtr<nsIXULWindow> kungFuDeathGrip(this);
  {
    MutexAutoLock lock(mSPTimerLock);
    if (mSPTimer) {
      mSPTimer->Cancel();
      SavePersistentAttributes();
      mSPTimer = nullptr;
    }
  }
  return nsXULWindow::Destroy();
}
