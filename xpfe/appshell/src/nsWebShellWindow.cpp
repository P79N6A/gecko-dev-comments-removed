






































#include "nsWebShellWindow.h"

#include "nsLayoutCID.h"
#include "nsContentCID.h"
#include "nsIWeakReference.h"

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
#include "nsIDOMEventTarget.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMFocusListener.h"
#include "nsIWebNavigation.h"
#include "nsIWindowWatcher.h"

#include "nsIDOMXULElement.h"

#include "nsGUIEvent.h"
#include "nsWidgetsCID.h"
#include "nsIWidget.h"
#include "nsIAppShell.h"

#include "nsIAppShellService.h"

#include "nsIDOMCharacterData.h"
#include "nsIDOMNodeList.h"

#include "nsITimer.h"
#include "nsXULPopupManager.h"

#include "prmem.h"
#include "prlock.h"

#include "nsIDOMXULDocument.h"

#include "nsFocusManager.h"

#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"

#include "nsIDocumentViewer.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIObserverService.h"
#include "prprf.h"

#include "nsIContent.h" 


#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"

#include "nsIBaseWindow.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"

#include "nsIMarkupDocumentViewer.h"

#ifdef XP_MACOSX
#include "nsINativeMenuService.h"
#define USE_NATIVE_MENUS
#endif


static NS_DEFINE_CID(kWindowCID,           NS_WINDOW_CID);

#define SIZE_PERSISTENCE_TIMEOUT 500 // msec

nsWebShellWindow::nsWebShellWindow(PRUint32 aChromeFlags)
  : nsXULWindow(aChromeFlags)
{
  mSPTimerLock = PR_NewLock();
}


nsWebShellWindow::~nsWebShellWindow()
{
  if (mWindow) {
    mWindow->SetClientData(0);
    mWindow->Destroy();
    mWindow = nsnull; 
  }

  if (mSPTimerLock) {
    PR_Lock(mSPTimerLock);
    if (mSPTimer)
      mSPTimer->Cancel();
    PR_Unlock(mSPTimerLock);
    PR_DestroyLock(mSPTimerLock);
  }
}

NS_IMPL_ADDREF_INHERITED(nsWebShellWindow, nsXULWindow)
NS_IMPL_RELEASE_INHERITED(nsWebShellWindow, nsXULWindow)

NS_INTERFACE_MAP_BEGIN(nsWebShellWindow)
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
NS_INTERFACE_MAP_END_INHERITING(nsXULWindow)

nsresult nsWebShellWindow::Initialize(nsIXULWindow* aParent,
                                      nsIAppShell* aShell, nsIURI* aUrl, 
                                      PRInt32 aInitialWidth,
                                      PRInt32 aInitialHeight,
                                      PRBool aIsHiddenWindow,
                                      nsWidgetInitData& widgetInitData)
{
  nsresult rv;
  nsCOMPtr<nsIWidget> parentWidget;

  mIsHiddenWindow = aIsHiddenWindow;
  
  
  
  nsIntRect r(0, 0, aInitialWidth, aInitialHeight);
  
  
  mWindow = do_CreateInstance(kWindowCID, &rv);
  if (NS_OK != rv) {
    return rv;
  }

  










  nsCOMPtr<nsIBaseWindow> parentAsWin(do_QueryInterface(aParent));
  if (parentAsWin) {
    parentAsWin->GetMainWidget(getter_AddRefs(parentWidget));
    mParentWindow = do_GetWeakReference(aParent);
  }

  mWindow->SetClientData(this);
  mWindow->Create((nsIWidget *)parentWidget,          
                  nsnull,                             
                  r,                                  
                  nsWebShellWindow::HandleEvent,      
                  nsnull,                             
                  aShell,                             
                  nsnull,                             
                  &widgetInitData);                   
  mWindow->GetClientBounds(r);
  mWindow->SetBackgroundColor(NS_RGB(192,192,192));

  
  mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  
  
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(EnsureChromeTreeOwner(), NS_ERROR_FAILURE);

  docShellAsItem->SetTreeOwner(mChromeTreeOwner);
  docShellAsItem->SetItemType(nsIDocShellTreeItem::typeChrome);

  r.x = r.y = 0;
  nsCOMPtr<nsIBaseWindow> docShellAsWin(do_QueryInterface(mDocShell));
  NS_ENSURE_SUCCESS(docShellAsWin->InitWindow(nsnull, mWindow, 
   r.x, r.y, r.width, r.height), NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(docShellAsWin->Create(), NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIWebProgress> webProgress(do_GetInterface(mDocShell, &rv));
  if (webProgress) {
    webProgress->AddProgressListener(this, nsIWebProgress::NOTIFY_STATE_NETWORK);
  }

  if (nsnull != aUrl)  {
    nsCString tmpStr;

    rv = aUrl->GetSpec(tmpStr);
    if (NS_FAILED(rv)) return rv;

    NS_ConvertUTF8toUTF16 urlString(tmpStr);
    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
    NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);
    rv = webNav->LoadURI(urlString.get(),
                         nsIWebNavigation::LOAD_FLAGS_NONE,
                         nsnull,
                         nsnull,
                         nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }
                     
  return rv;
}





nsresult
nsWebShellWindow::Toolbar()
{
    nsCOMPtr<nsIXULWindow> kungFuDeathGrip(this);
    nsCOMPtr<nsIWebBrowserChrome> wbc(do_GetInterface(kungFuDeathGrip));
    if (!wbc)
      return NS_ERROR_UNEXPECTED;

    
    
    
    

    PRUint32    chromeMask = (nsIWebBrowserChrome::CHROME_TOOLBAR |
                              nsIWebBrowserChrome::CHROME_LOCATIONBAR |
                              nsIWebBrowserChrome::CHROME_PERSONAL_TOOLBAR);

    PRUint32    chromeFlags, newChromeFlags = 0;
    wbc->GetChromeFlags(&chromeFlags);
    newChromeFlags = chromeFlags & chromeMask;
    if (!newChromeFlags)    chromeFlags |= chromeMask;
    else                    chromeFlags &= (~newChromeFlags);
    wbc->SetChromeFlags(chromeFlags);
    return NS_OK;
}








nsEventStatus
nsWebShellWindow::HandleEvent(nsGUIEvent *aEvent)
{
  nsEventStatus result = nsEventStatus_eIgnore;
  nsIDocShell* docShell = nsnull;
  nsWebShellWindow *eventWindow = nsnull;

  
  if (nsnull != aEvent->widget) {
    void* data;

    aEvent->widget->GetClientData(data);
    if (data != nsnull) {
      eventWindow = reinterpret_cast<nsWebShellWindow *>(data);
      docShell = eventWindow->mDocShell;
    }
  }

  if (docShell) {
    switch(aEvent->message) {
      



      case NS_MOVE: {
#ifndef XP_MACOSX
        
        
        
        
        nsCOMPtr<nsIMenuRollup> pm =
          do_GetService("@mozilla.org/xul/xul-popup-manager;1");
        if (pm)
          pm->AdjustPopupsOnWindowChange();
#endif

        
        
        eventWindow->SetPersistenceTimer(PAD_POSITION);
        break;
      }
      case NS_SIZE: {
#ifndef XP_MACOSX
        nsCOMPtr<nsIMenuRollup> pm =
          do_GetService("@mozilla.org/xul/xul-popup-manager;1");
        if (pm)
          pm->AdjustPopupsOnWindowChange();
#endif
 
        nsSizeEvent* sizeEvent = (nsSizeEvent*)aEvent;
        nsCOMPtr<nsIBaseWindow> shellAsWin(do_QueryInterface(docShell));
        shellAsWin->SetPositionAndSize(0, 0, sizeEvent->windowSize->width, 
          sizeEvent->windowSize->height, PR_FALSE);  
        
        
        if (!eventWindow->IsLocked())
          eventWindow->SetPersistenceTimer(PAD_POSITION | PAD_SIZE | PAD_MISC);
        result = nsEventStatus_eConsumeNoDefault;
        break;
      }
      case NS_SIZEMODE: {
        nsSizeModeEvent* modeEvent = (nsSizeModeEvent*)aEvent;

        
        
        
        
        if (modeEvent->mSizeMode == nsSizeMode_Maximized ||
            modeEvent->mSizeMode == nsSizeMode_Fullscreen) {
          PRUint32 zLevel;
          eventWindow->GetZLevel(&zLevel);
          if (zLevel > nsIXULWindow::normalZ)
            eventWindow->SetZLevel(nsIXULWindow::normalZ);
        }

        aEvent->widget->SetSizeMode(modeEvent->mSizeMode);

        
        
        
        eventWindow->SetPersistenceTimer(PAD_MISC);
        result = nsEventStatus_eConsumeDoDefault;

        
        
        
        
        
        break;
      }
      case NS_OS_TOOLBAR: {
        nsCOMPtr<nsIXULWindow> kungFuDeathGrip(eventWindow);
        eventWindow->Toolbar();
        break;
      }
      case NS_XUL_CLOSE: {
        
        
        
        
        nsCOMPtr<nsIXULWindow> kungFuDeathGrip(eventWindow);
        if (!eventWindow->ExecuteCloseHandler())
          eventWindow->Destroy();
        break;
      }
      


      case NS_DESTROY: {
        eventWindow->Destroy();
        break;
      }

      case NS_SETZLEVEL: {
        nsZLevelEvent *zEvent = (nsZLevelEvent *) aEvent;

        zEvent->mAdjusted = eventWindow->ConstrainToZLevel(zEvent->mImmediate,
                              &zEvent->mPlacement,
                              zEvent->mReqBelow, &zEvent->mActualBelow);
        break;
      }

      case NS_ACTIVATE: {
#if defined(DEBUG_saari) || defined(DEBUG_smaug)
        printf("nsWebShellWindow::NS_ACTIVATE\n");
#endif
        
        nsCOMPtr<nsIXULWindow> kungFuDeathGrip(eventWindow);

        nsCOMPtr<nsIDOMWindow> window = do_GetInterface(docShell);
        nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
        if (fm && window)
          fm->WindowRaised(window);

        if (eventWindow->mChromeLoaded) {
          eventWindow->PersistentAttributesDirty(
                             PAD_POSITION | PAD_SIZE | PAD_MISC);
          eventWindow->SavePersistentAttributes();
        }

        break;
      }

      case NS_DEACTIVATE: {
#if defined(DEBUG_saari) || defined(DEBUG_smaug)
        printf("nsWebShellWindow::NS_DEACTIVATE\n");
#endif

        nsCOMPtr<nsIDOMWindow> window = do_GetInterface(docShell);
        nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
        if (fm && window)
          fm->WindowLowered(window);
        break;
      }
      
      case NS_GETACCESSIBLE: {
        nsCOMPtr<nsIPresShell> presShell;
        docShell->GetPresShell(getter_AddRefs(presShell));
        if (presShell) {
          presShell->HandleEventWithTarget(aEvent, nsnull, nsnull, &result);
        }
        break;
      }
      default:
        break;

    }
  }
  return result;
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

void
nsWebShellWindow::SetPersistenceTimer(PRUint32 aDirtyFlags)
{
  if (!mSPTimerLock)
    return;

  PR_Lock(mSPTimerLock);
  if (!mSPTimer) {
    nsresult rv;
    mSPTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      NS_ADDREF_THIS(); 
    }
  }
  mSPTimer->InitWithFuncCallback(FirePersistenceTimer, this,
                                 SIZE_PERSISTENCE_TIMEOUT, nsITimer::TYPE_ONE_SHOT);
  PersistentAttributesDirty(aDirtyFlags);
  PR_Unlock(mSPTimerLock);
}

void
nsWebShellWindow::FirePersistenceTimer(nsITimer *aTimer, void *aClosure)
{
  nsWebShellWindow *win = static_cast<nsWebShellWindow *>(aClosure);
  if (!win->mSPTimerLock)
    return;
  PR_Lock(win->mSPTimerLock);
  win->SavePersistentAttributes();
  PR_Unlock(win->mSPTimerLock);
}





NS_IMETHODIMP
nsWebShellWindow::OnProgressChange(nsIWebProgress *aProgress,
                                   nsIRequest *aRequest,
                                   PRInt32 aCurSelfProgress,
                                   PRInt32 aMaxSelfProgress,
                                   PRInt32 aCurTotalProgress,
                                   PRInt32 aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnStateChange(nsIWebProgress *aProgress,
                                nsIRequest *aRequest,
                                PRUint32 aStateFlags,
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

  mChromeLoaded = PR_TRUE;
  mLockedUntilChromeLoad = PR_FALSE;

#ifdef USE_NATIVE_MENUS
  
  
  
  nsCOMPtr<nsIDOMDocument> menubarDOMDoc(GetNamedDOMDoc(NS_LITERAL_STRING("this"))); 
  if (menubarDOMDoc)
    LoadNativeMenus(menubarDOMDoc, mWindow);
#endif 

  OnChromeLoaded();
  LoadContentAreas();

  return NS_OK;
}

NS_IMETHODIMP
nsWebShellWindow::OnLocationChange(nsIWebProgress *aProgress,
                                   nsIRequest *aRequest,
                                   nsIURI *aURI)
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
                                   PRUint32 state)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}



nsCOMPtr<nsIDOMDocument> nsWebShellWindow::GetNamedDOMDoc(const nsAString & aDocShellName)
{
  nsCOMPtr<nsIDOMDocument> domDoc; 

  
  nsCOMPtr<nsIDocShell> childDocShell;
  if (aDocShellName.EqualsLiteral("this")) { 
    childDocShell = mDocShell;
  } else {
    nsCOMPtr<nsIDocShellTreeItem> docShellAsItem;
    nsCOMPtr<nsIDocShellTreeNode> docShellAsNode(do_QueryInterface(mDocShell));
    docShellAsNode->FindChildWithName(PromiseFlatString(aDocShellName).get(), 
      PR_TRUE, PR_FALSE, nsnull, nsnull, getter_AddRefs(docShellAsItem));
    childDocShell = do_QueryInterface(docShellAsItem);
    if (!childDocShell)
      return domDoc;
  }
  
  nsCOMPtr<nsIContentViewer> cv;
  childDocShell->GetContentViewer(getter_AddRefs(cv));
  if (!cv)
    return domDoc;
   
  nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(cv));
  if (!docv)
    return domDoc;

  nsCOMPtr<nsIDocument> doc;
  docv->GetDocument(getter_AddRefs(doc));
  if (doc)
    return nsCOMPtr<nsIDOMDocument>(do_QueryInterface(doc));

  return domDoc;
} 




void nsWebShellWindow::LoadContentAreas() {

  nsAutoString searchSpec;

  
  nsCOMPtr<nsIContentViewer> contentViewer;
  
  
  if (mDocShell)
    mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (contentViewer) {
    nsCOMPtr<nsIDocumentViewer> docViewer = do_QueryInterface(contentViewer);
    if (docViewer) {
      nsCOMPtr<nsIDocument> doc;
      docViewer->GetDocument(getter_AddRefs(doc));
      nsIURI *mainURL = doc->GetDocumentURI();

      nsCOMPtr<nsIURL> url = do_QueryInterface(mainURL);
      if (url) {
        nsCAutoString search;
        url->GetQuery(search);

        AppendUTF8toUTF16(search, searchSpec);
      }
    }
  }

  
  
  if (!searchSpec.IsEmpty()) {
    PRInt32     begPos,
                eqPos,
                endPos;
    nsString    contentAreaID,
                contentURL;
    char        *urlChar;
    nsresult rv;
    for (endPos = 0; endPos < (PRInt32)searchSpec.Length(); ) {
      
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
                          nsnull,
                          nsnull,
                          nsnull);
            nsMemory::Free(urlChar);
          }
        }
      }
    }
  }
}





PRBool nsWebShellWindow::ExecuteCloseHandler()
{
  




  nsCOMPtr<nsIXULWindow> kungFuDeathGrip(this);

  nsCOMPtr<nsPIDOMWindow> window(do_GetInterface(mDocShell));
  nsCOMPtr<nsPIDOMEventTarget> eventTarget = do_QueryInterface(window);

  if (eventTarget) {
    nsCOMPtr<nsIContentViewer> contentViewer;
    mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
    nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(contentViewer));

    if (docViewer) {
      nsCOMPtr<nsPresContext> presContext;
      docViewer->GetPresContext(getter_AddRefs(presContext));

      nsEventStatus status = nsEventStatus_eIgnore;
      nsMouseEvent event(PR_TRUE, NS_XUL_CLOSE, nsnull,
                         nsMouseEvent::eReal);

      nsresult rv =
        eventTarget->DispatchDOMEvent(&event, nsnull, presContext, &status);
      if (NS_SUCCEEDED(rv) && status == nsEventStatus_eConsumeNoDefault)
        return PR_TRUE;
      
    }
  }

  return PR_FALSE;
} 


NS_IMETHODIMP nsWebShellWindow::Destroy()
{
  nsresult rv;
  nsCOMPtr<nsIWebProgress> webProgress(do_GetInterface(mDocShell, &rv));
  if (webProgress) {
    webProgress->RemoveProgressListener(this);
  }

  nsCOMPtr<nsIXULWindow> kungFuDeathGrip(this);
  if (mSPTimerLock) {
  PR_Lock(mSPTimerLock);
  if (mSPTimer) {
    mSPTimer->Cancel();
    SavePersistentAttributes();
    mSPTimer = nsnull;
    NS_RELEASE_THIS(); 
  }
  PR_Unlock(mSPTimerLock);
  PR_DestroyLock(mSPTimerLock);
  mSPTimerLock = nsnull;
  }
  return nsXULWindow::Destroy();
}
