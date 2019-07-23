













































#include "nsCOMPtr.h"
#include "nsEventStateManager.h"
#include "nsEventListenerManager.h"
#include "nsIMEStateManager.h"
#include "nsContentEventHandler.h"
#include "nsIContent.h"
#include "nsINodeInfo.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIWidget.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsDOMEvent.h"
#include "nsGkAtoms.h"
#include "nsIEditorDocShell.h"
#include "nsIFormControl.h"
#include "nsIComboboxControlFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMXULControlElement.h"
#include "nsIDOMXULTextboxElement.h"
#include "nsImageMapUtils.h"
#include "nsIHTMLDocument.h"
#include "nsINameSpaceManager.h"
#include "nsIBaseWindow.h"
#include "nsIScrollableView.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsISelection.h"
#include "nsFrameSelection.h"
#include "nsIDeviceContext.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMEventTarget.h"
#include "nsIEnumerator.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIWebNavigation.h"
#include "nsIContentViewer.h"
#include "nsIPrefBranch2.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif

#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"

#include "nsFocusManager.h"

#include "nsIDOMXULElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMKeyEvent.h"
#include "nsIObserverService.h"
#include "nsIDocShell.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIScrollableViewProvider.h"
#include "nsIDOMDocumentRange.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMMouseScrollEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMNSUIEvent.h"
#include "nsDOMDragEvent.h"
#include "nsIDOMNSEditableElement.h"

#include "nsIDOMRange.h"
#include "nsCaret.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"

#include "nsIFrameFrame.h"
#include "nsIFrameTraversal.h"
#include "nsLayoutCID.h"
#include "nsLayoutUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"

#include "imgIContainer.h"
#include "nsIProperties.h"
#include "nsISupportsPrimitives.h"
#include "nsEventDispatcher.h"
#include "nsPresShellIterator.h"

#include "nsServiceManagerUtils.h"
#include "nsITimer.h"
#include "nsIFontMetrics.h"
#include "nsIDOMXULDocument.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsDOMDataTransfer.h"
#include "nsContentAreaDragDrop.h"
#ifdef MOZ_XUL
#include "nsTreeBodyFrame.h"
#endif
#include "nsIFocusController.h"
#include "nsIController.h"

#ifdef XP_MACOSX
#include <Carbon/Carbon.h>
#endif



#define NS_USER_INTERACTION_INTERVAL 5000 // ms

static NS_DEFINE_CID(kFrameTraversalCID, NS_FRAMETRAVERSAL_CID);


#define NON_KEYBINDING 0

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

static PRBool sLeftClickOnly = PR_TRUE;
static PRBool sKeyCausesActivation = PR_TRUE;
static PRUint32 sESMInstanceCount = 0;
static PRInt32 sChromeAccessModifier = 0, sContentAccessModifier = 0;
PRInt32 nsEventStateManager::sUserInputEventDepth = 0;

static PRUint32 gMouseOrKeyboardEventCounter = 0;
static nsITimer* gUserInteractionTimer = nsnull;
static nsITimerCallback* gUserInteractionTimerCallback = nsnull;


static nscoord gPixelScrollDeltaX = 0;
static nscoord gPixelScrollDeltaY = 0;
static PRUint32 gPixelScrollDeltaTimeout = 0;

static nscoord
GetScrollableViewLineHeight(nsPresContext* aPresContext, nsIFrame* aTargetFrame);

#ifdef DEBUG_DOCSHELL_FOCUS
static void
PrintDocTree(nsIDocShellTreeItem* aParentItem, int aLevel)
{
  for (PRInt32 i=0;i<aLevel;i++) printf("  ");

  PRInt32 childWebshellCount;
  aParentItem->GetChildCount(&childWebshellCount);
  nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(aParentItem));
  PRInt32 type;
  aParentItem->GetItemType(&type);
  nsCOMPtr<nsIPresShell> presShell;
  parentAsDocShell->GetPresShell(getter_AddRefs(presShell));
  nsCOMPtr<nsPresContext> presContext;
  parentAsDocShell->GetPresContext(getter_AddRefs(presContext));
  nsCOMPtr<nsIContentViewer> cv;
  parentAsDocShell->GetContentViewer(getter_AddRefs(cv));
  nsCOMPtr<nsIDOMDocument> domDoc;
  if (cv)
    cv->GetDOMDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  nsCOMPtr<nsIDOMWindowInternal> domwin = doc ? doc->GetWindow() : nsnull;
  nsIURI* uri = doc ? doc->GetDocumentURI() : nsnull;

  printf("DS %p  Type %s  Cnt %d  Doc %p  DW %p  EM %p%c",
    static_cast<void*>(parentAsDocShell.get()),
    type==nsIDocShellTreeItem::typeChrome?"Chrome":"Content",
    childWebshellCount, static_cast<void*>(doc.get()),
    static_cast<void*>(domwin.get()),
    static_cast<void*>(presContext ? presContext->EventStateManager() : nsnull),
    uri ? ' ' : '\n');
  if (uri) {
    nsCAutoString spec;
    uri->GetSpec(spec);
    printf("\"%s\"\n", spec.get());
  }

  if (childWebshellCount > 0) {
    for (PRInt32 i = 0; i < childWebshellCount; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      aParentItem->GetChildAt(i, getter_AddRefs(child));
      PrintDocTree(child, aLevel + 1);
    }
  }
}

static void
PrintDocTreeAll(nsIDocShellTreeItem* aItem)
{
  nsCOMPtr<nsIDocShellTreeItem> item = aItem;
  for(;;) {
    nsCOMPtr<nsIDocShellTreeItem> parent;
    item->GetParent(getter_AddRefs(parent));
    if (!parent)
      break;
    item = parent;
  }

  PrintDocTree(item, 0);
}
#endif

class nsUITimerCallback : public nsITimerCallback
{
public:
  nsUITimerCallback() : mPreviousCount(0) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
private:
  PRUint32 mPreviousCount;
};

NS_IMPL_ISUPPORTS1(nsUITimerCallback, nsITimerCallback)



NS_IMETHODIMP
nsUITimerCallback::Notify(nsITimer* aTimer)
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  if ((gMouseOrKeyboardEventCounter == mPreviousCount) || !aTimer) {
    gMouseOrKeyboardEventCounter = 0;
    obs->NotifyObservers(nsnull, "user-interaction-inactive", nsnull);
  } else {
    obs->NotifyObservers(nsnull, "user-interaction-active", nsnull);
  }
  mPreviousCount = gMouseOrKeyboardEventCounter;
  return NS_OK;
}

enum {
 MOUSE_SCROLL_N_LINES,
 MOUSE_SCROLL_PAGE,
 MOUSE_SCROLL_HISTORY,
 MOUSE_SCROLL_ZOOM,
 MOUSE_SCROLL_PIXELS
};


#define NS_MODIFIER_SHIFT    1
#define NS_MODIFIER_CONTROL  2
#define NS_MODIFIER_ALT      4
#define NS_MODIFIER_META     8

static nsIDocument *
GetDocumentFromWindow(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aWindow);
  nsCOMPtr<nsIDocument> doc;

  if (win) {
    doc = do_QueryInterface(win->GetExtantDocument());
  }

  return doc;
}

static PRInt32
GetAccessModifierMaskFromPref(PRInt32 aItemType)
{
  PRInt32 accessKey = nsContentUtils::GetIntPref("ui.key.generalAccessKey", -1);
  switch (accessKey) {
    case -1:                             break; 
    case nsIDOMKeyEvent::DOM_VK_SHIFT:   return NS_MODIFIER_SHIFT;
    case nsIDOMKeyEvent::DOM_VK_CONTROL: return NS_MODIFIER_CONTROL;
    case nsIDOMKeyEvent::DOM_VK_ALT:     return NS_MODIFIER_ALT;
    case nsIDOMKeyEvent::DOM_VK_META:    return NS_MODIFIER_META;
    default:                             return 0;
  }

  switch (aItemType) {
  case nsIDocShellTreeItem::typeChrome:
    return nsContentUtils::GetIntPref("ui.key.chromeAccess", 0);
  case nsIDocShellTreeItem::typeContent:
    return nsContentUtils::GetIntPref("ui.key.contentAccess", 0);
  default:
    return 0;
  }
}

static void
GetBasePrefKeyForMouseWheel(nsMouseScrollEvent* aEvent, nsACString& aPref)
{
  NS_NAMED_LITERAL_CSTRING(prefbase,    "mousewheel");
  NS_NAMED_LITERAL_CSTRING(horizscroll, ".horizscroll");
  NS_NAMED_LITERAL_CSTRING(withshift,   ".withshiftkey");
  NS_NAMED_LITERAL_CSTRING(withalt,     ".withaltkey");
  NS_NAMED_LITERAL_CSTRING(withcontrol, ".withcontrolkey");
  NS_NAMED_LITERAL_CSTRING(withmetakey, ".withmetakey");
  NS_NAMED_LITERAL_CSTRING(withno,      ".withnokey");

  aPref = prefbase;
  if (aEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal) {
    aPref.Append(horizscroll);
  }
  if (aEvent->isShift) {
    aPref.Append(withshift);
  } else if (aEvent->isControl) {
    aPref.Append(withcontrol);
  } else if (aEvent->isAlt) {
    aPref.Append(withalt);
  } else if (aEvent->isMeta) {
    aPref.Append(withmetakey);
  } else {
    aPref.Append(withno);
  }
}

class nsMouseWheelTransaction {
public:
  static nsIFrame* GetTargetFrame() { return sTargetFrame; }
  static void BeginTransaction(nsIFrame* aTargetFrame,
                               PRInt32 aNumLines,
                               PRBool aScrollHorizontal);
  
  
  static PRBool UpdateTransaction(PRInt32 aNumLines,
                                  PRBool aScrollHorizontal);
  static void EndTransaction();
  static void OnEvent(nsEvent* aEvent);
  static void Shutdown();
  static PRUint32 GetTimeoutTime();
  static void AccelerateWheelDelta(PRInt32 &aScrollX, PRInt32 &aScrollY);

  enum {
    kScrollSeriesTimeout = 80
  };
protected:
  static nsIntPoint GetScreenPoint(nsGUIEvent* aEvent);
  static void OnFailToScrollTarget();
  static void OnTimeout(nsITimer *aTimer, void *aClosure);
  static void SetTimeout();
  static PRUint32 GetIgnoreMoveDelayTime();
  static PRInt32 GetAccelerationStart();
  static PRInt32 GetAccelerationFactor();
  static PRInt32 ComputeWheelDelta(PRInt32 aDelta, PRInt32 aFactor);

  static nsWeakFrame sTargetFrame;
  static PRUint32    sTime;        
  static PRUint32    sMouseMoved;  
  static nsITimer*   sTimer;
  static PRInt32     sScrollSeriesCounter;
};

nsWeakFrame nsMouseWheelTransaction::sTargetFrame(nsnull);
PRUint32    nsMouseWheelTransaction::sTime        = 0;
PRUint32    nsMouseWheelTransaction::sMouseMoved  = 0;
nsITimer*   nsMouseWheelTransaction::sTimer       = nsnull;
PRInt32     nsMouseWheelTransaction::sScrollSeriesCounter = 0;

static PRBool
OutOfTime(PRUint32 aBaseTime, PRUint32 aThreshold)
{
  PRUint32 now = PR_IntervalToMilliseconds(PR_IntervalNow());
  return (now - aBaseTime > aThreshold);
}

static PRBool
CanScrollOn(nsIScrollableView* aScrollView, PRInt32 aNumLines,
            PRBool aScrollHorizontal)
{
  NS_PRECONDITION(aScrollView, "aScrollView is null");
  NS_PRECONDITION(aNumLines, "aNumLines must be non-zero");
  PRBool canScroll;
  nsresult rv =
    aScrollView->CanScroll(aScrollHorizontal, aNumLines > 0, canScroll);
  return NS_SUCCEEDED(rv) && canScroll;
}

void
nsMouseWheelTransaction::BeginTransaction(nsIFrame* aTargetFrame,
                                          PRInt32 aNumLines,
                                          PRBool aScrollHorizontal)
{
  NS_ASSERTION(!sTargetFrame, "previous transaction is not finished!");
  sTargetFrame = aTargetFrame;
  sScrollSeriesCounter = 0;
  if (!UpdateTransaction(aNumLines, aScrollHorizontal)) {
    NS_ERROR("BeginTransaction is called even cannot scroll the frame");
    EndTransaction();
  }
}

PRBool
nsMouseWheelTransaction::UpdateTransaction(PRInt32 aNumLines,
                                           PRBool aScrollHorizontal)
{
  nsIScrollableViewProvider* svp = do_QueryFrame(GetTargetFrame());
  NS_ENSURE_TRUE(svp, PR_FALSE);
  nsIScrollableView *scrollView = svp->GetScrollableView();
  NS_ENSURE_TRUE(scrollView, PR_FALSE);

  if (!CanScrollOn(scrollView, aNumLines, aScrollHorizontal)) {
    OnFailToScrollTarget();
    
    
    return PR_FALSE;
  }

  SetTimeout();

  if (sScrollSeriesCounter != 0 && OutOfTime(sTime, kScrollSeriesTimeout))
    sScrollSeriesCounter = 0;
  sScrollSeriesCounter++;

  
  
  
  
  sTime = PR_IntervalToMilliseconds(PR_IntervalNow());
  sMouseMoved = 0;
  return PR_TRUE;
}

void
nsMouseWheelTransaction::EndTransaction()
{
  if (sTimer)
    sTimer->Cancel();
  sTargetFrame = nsnull;
  sScrollSeriesCounter = 0;
}

void
nsMouseWheelTransaction::OnEvent(nsEvent* aEvent)
{
  if (!sTargetFrame)
    return;

  if (OutOfTime(sTime, GetTimeoutTime())) {
    
    
    
    
    OnTimeout(nsnull, nsnull);
    return;
  }

  switch (aEvent->message) {
    case NS_MOUSE_SCROLL:
      if (sMouseMoved != 0 &&
          OutOfTime(sMouseMoved, GetIgnoreMoveDelayTime())) {
        
        
        EndTransaction();
      }
      return;
    case NS_MOUSE_MOVE:
    case NS_DRAGDROP_OVER:
      if (((nsMouseEvent*)aEvent)->reason == nsMouseEvent::eReal) {
        
        
        nsIntPoint pt = GetScreenPoint((nsGUIEvent*)aEvent);
        nsIntRect r = sTargetFrame->GetScreenRectExternal();
        if (!r.Contains(pt)) {
          EndTransaction();
          return;
        }

        
        
        
        
        if (OutOfTime(sTime, GetIgnoreMoveDelayTime())) {
          if (sMouseMoved == 0)
            sMouseMoved = PR_IntervalToMilliseconds(PR_IntervalNow());
        }
      }
      return;
    case NS_KEY_PRESS:
    case NS_KEY_UP:
    case NS_KEY_DOWN:
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_BUTTON_DOWN:
    case NS_MOUSE_DOUBLECLICK:
    case NS_MOUSE_CLICK:
    case NS_CONTEXTMENU:
    case NS_DRAGDROP_DROP:
      EndTransaction();
      return;
  }
}

void
nsMouseWheelTransaction::Shutdown()
{
  NS_IF_RELEASE(sTimer);
}

void
nsMouseWheelTransaction::OnFailToScrollTarget()
{
  NS_PRECONDITION(sTargetFrame, "We don't have mouse scrolling transaction");

  if (nsContentUtils::GetBoolPref("test.mousescroll", PR_FALSE)) {
    
    nsContentUtils::DispatchTrustedEvent(
                      sTargetFrame->GetContent()->GetOwnerDoc(),
                      sTargetFrame->GetContent(),
                      NS_LITERAL_STRING("MozMouseScrollFailed"),
                      PR_TRUE, PR_TRUE);
  }
  
  
  if (!sTargetFrame)
    EndTransaction();
}

void
nsMouseWheelTransaction::OnTimeout(nsITimer* aTimer, void* aClosure)
{
  if (!sTargetFrame) {
    
    EndTransaction();
    return;
  }
  
  nsIFrame* frame = sTargetFrame;
  
  
  EndTransaction();

  if (nsContentUtils::GetBoolPref("test.mousescroll", PR_FALSE)) {
    
    nsContentUtils::DispatchTrustedEvent(
                      frame->GetContent()->GetOwnerDoc(),
                      frame->GetContent(),
                      NS_LITERAL_STRING("MozMouseScrollTransactionTimeout"),
                      PR_TRUE, PR_TRUE);
  }
}

void
nsMouseWheelTransaction::SetTimeout()
{
  if (!sTimer) {
    nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
    if (!timer)
      return;
    timer.swap(sTimer);
  }
  sTimer->Cancel();
  nsresult rv =
    sTimer->InitWithFuncCallback(OnTimeout, nsnull, GetTimeoutTime(),
                                 nsITimer::TYPE_ONE_SHOT);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "nsITimer::InitWithFuncCallback failed");
}

nsIntPoint
nsMouseWheelTransaction::GetScreenPoint(nsGUIEvent* aEvent)
{
  NS_ASSERTION(aEvent, "aEvent is null");
  NS_ASSERTION(aEvent->widget, "aEvent-widget is null");
  return aEvent->refPoint + aEvent->widget->WidgetToScreenOffset();
}

PRUint32
nsMouseWheelTransaction::GetTimeoutTime()
{
  return (PRUint32)
    nsContentUtils::GetIntPref("mousewheel.transaction.timeout", 1500);
}

PRUint32
nsMouseWheelTransaction::GetIgnoreMoveDelayTime()
{
  return (PRUint32)
    nsContentUtils::GetIntPref("mousewheel.transaction.ignoremovedelay", 100);
}

void
nsMouseWheelTransaction::AccelerateWheelDelta(PRInt32 &aScrollX,
                                              PRInt32 &aScrollY)
{
  PRInt32 start = GetAccelerationStart();
  if (start < 0 || sScrollSeriesCounter < start)
    return;

  PRInt32 factor = GetAccelerationFactor();
  if (factor < 0)
    return;

  aScrollX = ComputeWheelDelta(aScrollX, factor);
  aScrollY = ComputeWheelDelta(aScrollY, factor);
}

PRInt32
nsMouseWheelTransaction::ComputeWheelDelta(PRInt32 aDelta, PRInt32 aFactor)
{
  if (aDelta == 0)
    return 0;

  return PRInt32(0.5 + (aDelta * sScrollSeriesCounter * (double)aFactor / 10));
}

PRInt32
nsMouseWheelTransaction::GetAccelerationStart()
{
  return nsContentUtils::GetIntPref("mousewheel.acceleration.start", -1);
}

PRInt32
nsMouseWheelTransaction::GetAccelerationFactor()
{
  return nsContentUtils::GetIntPref("mousewheel.acceleration.factor", -1);
}





nsEventStateManager::nsEventStateManager()
  : mLockCursor(0),
    mCurrentTarget(nsnull),
    mLastMouseOverFrame(nsnull),
    mLastDragOverFrame(nsnull),
    
    mGestureDownPoint(0,0),
    mPresContext(nsnull),
    mLClickCount(0),
    mMClickCount(0),
    mRClickCount(0),
    mNormalLMouseEventInProcess(PR_FALSE),
    m_haveShutdown(PR_FALSE),
    mLastLineScrollConsumedX(PR_FALSE),
    mLastLineScrollConsumedY(PR_FALSE)
{
  if (sESMInstanceCount == 0) {
    gUserInteractionTimerCallback = new nsUITimerCallback();
    if (gUserInteractionTimerCallback) {
      NS_ADDREF(gUserInteractionTimerCallback);
      CallCreateInstance("@mozilla.org/timer;1", &gUserInteractionTimer);
      if (gUserInteractionTimer) {
        gUserInteractionTimer->InitWithCallback(gUserInteractionTimerCallback,
                                                NS_USER_INTERACTION_INTERVAL,
                                                nsITimer::TYPE_REPEATING_SLACK);
      }
    }
  }
  ++sESMInstanceCount;
}

NS_IMETHODIMP
nsEventStateManager::Init()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
           do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);

  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_QueryInterface(nsContentUtils::GetPrefBranch());

  if (prefBranch) {
    if (sESMInstanceCount == 1) {
      sLeftClickOnly =
        nsContentUtils::GetBoolPref("nglayout.events.dispatchLeftClickOnly",
                                    sLeftClickOnly);

      sChromeAccessModifier =
        GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeChrome);
      sContentAccessModifier =
        GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeContent);
    }
    prefBranch->AddObserver("accessibility.accesskeycausesactivation", this, PR_TRUE);
    prefBranch->AddObserver("nglayout.events.dispatchLeftClickOnly", this, PR_TRUE);
    prefBranch->AddObserver("ui.key.generalAccessKey", this, PR_TRUE);
    prefBranch->AddObserver("ui.key.chromeAccess", this, PR_TRUE);
    prefBranch->AddObserver("ui.key.contentAccess", this, PR_TRUE);
#if 0
    prefBranch->AddObserver("mousewheel.withaltkey.action", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withaltkey.numlines", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withaltkey.sysnumlines", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withcontrolkey.action", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withcontrolkey.numlines", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withcontrolkey.sysnumlines", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withnokey.action", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withnokey.numlines", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withnokey.sysnumlines", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withshiftkey.action", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withshiftkey.numlines", this, PR_TRUE);
    prefBranch->AddObserver("mousewheel.withshiftkey.sysnumlines", this, PR_TRUE);
#endif

    prefBranch->AddObserver("dom.popup_allowed_events", this, PR_TRUE);
  }

  return rv;
}

nsEventStateManager::~nsEventStateManager()
{
#if CLICK_HOLD_CONTEXT_MENUS
  KillClickHoldTimer();
#endif

  --sESMInstanceCount;
  if(sESMInstanceCount == 0) {
    nsMouseWheelTransaction::Shutdown();
    if (gUserInteractionTimerCallback) {
      gUserInteractionTimerCallback->Notify(nsnull);
      NS_RELEASE(gUserInteractionTimerCallback);
    }
    if (gUserInteractionTimer) {
      gUserInteractionTimer->Cancel();
      NS_RELEASE(gUserInteractionTimer);
    }
  }

  if (!m_haveShutdown) {
    Shutdown();

    
    
    

    nsresult rv;

    nsCOMPtr<nsIObserverService> observerService =
             do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    }
  }

}

nsresult
nsEventStateManager::Shutdown()
{
  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_QueryInterface(nsContentUtils::GetPrefBranch());

  if (prefBranch) {
    prefBranch->RemoveObserver("accessibility.accesskeycausesactivation", this);
    prefBranch->RemoveObserver("nglayout.events.dispatchLeftClickOnly", this);
    prefBranch->RemoveObserver("ui.key.generalAccessKey", this);
    prefBranch->RemoveObserver("ui.key.chromeAccess", this);
    prefBranch->RemoveObserver("ui.key.contentAccess", this);
#if 0
    prefBranch->RemoveObserver("mousewheel.withshiftkey.action", this);
    prefBranch->RemoveObserver("mousewheel.withshiftkey.numlines", this);
    prefBranch->RemoveObserver("mousewheel.withshiftkey.sysnumlines", this);
    prefBranch->RemoveObserver("mousewheel.withcontrolkey.action", this);
    prefBranch->RemoveObserver("mousewheel.withcontrolkey.numlines", this);
    prefBranch->RemoveObserver("mousewheel.withcontrolkey.sysnumlines", this);
    prefBranch->RemoveObserver("mousewheel.withaltkey.action", this);
    prefBranch->RemoveObserver("mousewheel.withaltkey.numlines", this);
    prefBranch->RemoveObserver("mousewheel.withaltkey.sysnumlines", this);
    prefBranch->RemoveObserver("mousewheel.withnokey.action", this);
    prefBranch->RemoveObserver("mousewheel.withnokey.numlines", this);
    prefBranch->RemoveObserver("mousewheel.withnokey.sysnumlines", this);
#endif

    prefBranch->RemoveObserver("dom.popup_allowed_events", this);
  }

  m_haveShutdown = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *someData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    Shutdown();
  else if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    if (!someData)
      return NS_OK;

    nsDependentString data(someData);
    if (data.EqualsLiteral("accessibility.accesskeycausesactivation")) {
      sKeyCausesActivation =
        nsContentUtils::GetBoolPref("accessibility.accesskeycausesactivation",
                                    sKeyCausesActivation);
    } else if (data.EqualsLiteral("nglayout.events.dispatchLeftClickOnly")) {
      sLeftClickOnly =
        nsContentUtils::GetBoolPref("nglayout.events.dispatchLeftClickOnly",
                                    sLeftClickOnly);
    } else if (data.EqualsLiteral("ui.key.generalAccessKey")) {
      sChromeAccessModifier =
        GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeChrome);
      sContentAccessModifier =
        GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeContent);
    } else if (data.EqualsLiteral("ui.key.chromeAccess")) {
      sChromeAccessModifier =
        GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeChrome);
    } else if (data.EqualsLiteral("ui.key.contentAccess")) {
      sContentAccessModifier =
        GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeContent);
#if 0
    } else if (data.EqualsLiteral("mousewheel.withaltkey.action")) {
    } else if (data.EqualsLiteral("mousewheel.withaltkey.numlines")) {
    } else if (data.EqualsLiteral("mousewheel.withaltkey.sysnumlines")) {
    } else if (data.EqualsLiteral("mousewheel.withcontrolkey.action")) {
    } else if (data.EqualsLiteral("mousewheel.withcontrolkey.numlines")) {
    } else if (data.EqualsLiteral("mousewheel.withcontrolkey.sysnumlines")) {
    } else if (data.EqualsLiteral("mousewheel.withshiftkey.action")) {
    } else if (data.EqualsLiteral("mousewheel.withshiftkey.numlines")) {
    } else if (data.EqualsLiteral("mousewheel.withshiftkey.sysnumlines")) {
    } else if (data.EqualsLiteral("mousewheel.withnokey.action")) {
    } else if (data.EqualsLiteral("mousewheel.withnokey.numlines")) {
    } else if (data.EqualsLiteral("mousewheel.withnokey.sysnumlines")) {
#endif
    } else if (data.EqualsLiteral("dom.popup_allowed_events")) {
      nsDOMEvent::PopupAllowedEventsChanged();
    }
  }

  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsEventStateManager)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsEventStateManager)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEventStateManager)
   NS_INTERFACE_MAP_ENTRY(nsIEventStateManager)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsEventStateManager, nsIEventStateManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsEventStateManager, nsIEventStateManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsEventStateManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentTargetContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastMouseOverElement);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mGestureDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mGestureDownFrameOwner);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastLeftMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastMiddleMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastRightMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mActiveContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mHoverContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDragOverContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mURLTargetContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstMouseOverEventElement);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstMouseOutEventElement);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mAccessKeys);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsEventStateManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCurrentTargetContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastMouseOverElement);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGestureDownContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mGestureDownFrameOwner);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastLeftMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastMiddleMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastRightMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mActiveContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mHoverContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDragOverContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mURLTargetContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstMouseOverEventElement);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstMouseOutEventElement);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mAccessKeys);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


NS_IMETHODIMP
nsEventStateManager::PreHandleEvent(nsPresContext* aPresContext,
                                    nsEvent *aEvent,
                                    nsIFrame* aTargetFrame,
                                    nsEventStatus* aStatus,
                                    nsIView* aView)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  NS_ENSURE_ARG(aPresContext);
  if (!aEvent) {
    NS_ERROR("aEvent is null.  This should never happen.");
    return NS_ERROR_NULL_POINTER;
  }

  mCurrentTarget = aTargetFrame;
  mCurrentTargetContent = nsnull;

  
  if (NS_EVENT_NEEDS_FRAME(aEvent)) {
    NS_ASSERTION(mCurrentTarget, "mCurrentTarget is null.  this should not happen.  see bug #13007");
    if (!mCurrentTarget) return NS_ERROR_NULL_POINTER;
  }

  
  
  if (NS_IS_TRUSTED_EVENT(aEvent) &&
      ((aEvent->eventStructType == NS_MOUSE_EVENT  &&
        static_cast<nsMouseEvent*>(aEvent)->reason == nsMouseEvent::eReal &&
        aEvent->message != NS_MOUSE_ENTER &&
        aEvent->message != NS_MOUSE_EXIT) ||
       aEvent->eventStructType == NS_MOUSE_SCROLL_EVENT ||
       aEvent->eventStructType == NS_KEY_EVENT)) {
    if (gMouseOrKeyboardEventCounter == 0) {
      nsCOMPtr<nsIObserverService> obs =
        do_GetService("@mozilla.org/observer-service;1");
      if (obs) {
        obs->NotifyObservers(nsnull, "user-interaction-active", nsnull);
      }
    }
    ++gMouseOrKeyboardEventCounter;
  }

  *aStatus = nsEventStatus_eIgnore;

  nsMouseWheelTransaction::OnEvent(aEvent);

  switch (aEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
    switch (static_cast<nsMouseEvent*>(aEvent)->button) {
    case nsMouseEvent::eLeftButton:
#ifndef XP_OS2
      BeginTrackingDragGesture ( aPresContext, (nsMouseEvent*)aEvent, aTargetFrame );
#endif
      mLClickCount = ((nsMouseEvent*)aEvent)->clickCount;
      SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      mNormalLMouseEventInProcess = PR_TRUE;
      break;
    case nsMouseEvent::eMiddleButton:
      mMClickCount = ((nsMouseEvent*)aEvent)->clickCount;
      SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      break;
    case nsMouseEvent::eRightButton:
#ifdef XP_OS2
      BeginTrackingDragGesture ( aPresContext, (nsMouseEvent*)aEvent, aTargetFrame );
#endif
      mRClickCount = ((nsMouseEvent*)aEvent)->clickCount;
      SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      break;
    }
    break;
  case NS_MOUSE_BUTTON_UP:
    switch (static_cast<nsMouseEvent*>(aEvent)->button) {
      case nsMouseEvent::eLeftButton:
#ifdef CLICK_HOLD_CONTEXT_MENUS
      KillClickHoldTimer();
#endif
#ifndef XP_OS2
      StopTrackingDragGesture();
#endif
      mNormalLMouseEventInProcess = PR_FALSE;
    case nsMouseEvent::eRightButton:
#ifdef XP_OS2
      StopTrackingDragGesture();
#endif
    case nsMouseEvent::eMiddleButton:
      SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      break;
    }
    break;
  case NS_MOUSE_EXIT:
    
    
    
    {
      nsMouseEvent* mouseEvent = static_cast<nsMouseEvent*>(aEvent);
      if (mouseEvent->exit != nsMouseEvent::eTopLevel) {
        
        
        
        mouseEvent->message = NS_MOUSE_MOVE;
        mouseEvent->reason = nsMouseEvent::eSynthesized;
        
      } else {
        GenerateMouseEnterExit((nsGUIEvent*)aEvent);
        
        aEvent->message = 0;
        break;
      }
    }
  case NS_MOUSE_MOVE:
    
    
    
    
    
    
    GenerateDragGesture(aPresContext, (nsMouseEvent*)aEvent);
    UpdateCursor(aPresContext, aEvent, mCurrentTarget, aStatus);
    GenerateMouseEnterExit((nsGUIEvent*)aEvent);
    break;
#ifdef CLICK_HOLD_CONTEXT_MENUS
  case NS_DRAGDROP_GESTURE:
    
    
    KillClickHoldTimer();
    break;
#endif
  case NS_DRAGDROP_DROP:
  case NS_DRAGDROP_OVER:
    
    
    GenerateDragDropEnterExit(aPresContext, (nsGUIEvent*)aEvent);
    break;

  case NS_KEY_PRESS:
    {

      nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;

      PRInt32 modifierMask = 0;
      if (keyEvent->isShift)
        modifierMask |= NS_MODIFIER_SHIFT;
      if (keyEvent->isControl)
        modifierMask |= NS_MODIFIER_CONTROL;
      if (keyEvent->isAlt)
        modifierMask |= NS_MODIFIER_ALT;
      if (keyEvent->isMeta)
        modifierMask |= NS_MODIFIER_META;

      
      if (modifierMask && (modifierMask == sChromeAccessModifier ||
                           modifierMask == sContentAccessModifier))
        HandleAccessKey(aPresContext, keyEvent, aStatus, nsnull,
                        eAccessKeyProcessingNormal, modifierMask);
    }
  case NS_KEY_DOWN:
  case NS_KEY_UP:
    {
      nsIContent* content = GetFocusedContent();
      if (content)
        mCurrentTargetContent = content;
    }
    break;
  case NS_MOUSE_SCROLL:
    {
      nsIContent* content = GetFocusedContent();
      if (content)
        mCurrentTargetContent = content;

      nsMouseScrollEvent* msEvent = static_cast<nsMouseScrollEvent*>(aEvent);

      NS_NAMED_LITERAL_CSTRING(actionslot,      ".action");
      NS_NAMED_LITERAL_CSTRING(numlinesslot,    ".numlines");
      NS_NAMED_LITERAL_CSTRING(sysnumlinesslot, ".sysnumlines");

      nsCAutoString baseKey;
      GetBasePrefKeyForMouseWheel(msEvent, baseKey);

      nsCAutoString sysNumLinesKey(baseKey);
      sysNumLinesKey.Append(sysnumlinesslot);
      PRBool useSysNumLines = nsContentUtils::GetBoolPref(sysNumLinesKey.get());

      nsCAutoString actionKey(baseKey);
      actionKey.Append(actionslot);
      PRInt32 action = nsContentUtils::GetIntPref(actionKey.get());

      if (!useSysNumLines) {
        
        
        
        
        
        
        
        
        
        
        
        

        nsCAutoString numLinesKey(baseKey);
        numLinesKey.Append(numlinesslot);
        PRInt32 numLines = nsContentUtils::GetIntPref(numLinesKey.get());

        PRBool swapDirs = (numLines < 0);
        PRInt32 userSize = swapDirs ? -numLines : numLines;

        PRBool deltaUp = (msEvent->delta < 0);
        if (swapDirs) {
          deltaUp = !deltaUp;
        }

        msEvent->delta = deltaUp ? -userSize : userSize;
      }
      if ((useSysNumLines &&
           (msEvent->scrollFlags & nsMouseScrollEvent::kIsFullPage)) ||
          action == MOUSE_SCROLL_PAGE) {
          msEvent->delta = (msEvent->delta > 0)
            ? PRInt32(nsIDOMNSUIEvent::SCROLL_PAGE_DOWN)
            : PRInt32(nsIDOMNSUIEvent::SCROLL_PAGE_UP);
      }
    }
    break;
  case NS_MOUSE_PIXEL_SCROLL:
    {
      nsIContent* content = GetFocusedContent();
      if (content)
        mCurrentTargetContent = content;

      nsMouseScrollEvent *msEvent = static_cast<nsMouseScrollEvent*>(aEvent);

      
      if (OutOfTime(gPixelScrollDeltaTimeout, nsMouseWheelTransaction::GetTimeoutTime())) {
        gPixelScrollDeltaX = gPixelScrollDeltaY = 0;
      }
      gPixelScrollDeltaTimeout = PR_IntervalToMilliseconds(PR_IntervalNow());

      
      if (msEvent->scrollFlags & nsMouseScrollEvent::kNoLines) {
        nscoord pixelHeight = aPresContext->AppUnitsToIntCSSPixels(
          GetScrollableViewLineHeight(aPresContext, aTargetFrame));

        if (msEvent->scrollFlags & nsMouseScrollEvent::kIsVertical) {
          gPixelScrollDeltaX += msEvent->delta;
          if (!gPixelScrollDeltaX || !pixelHeight)
            break;

          if (PR_ABS(gPixelScrollDeltaX) >= pixelHeight) {
            PRInt32 numLines = (PRInt32)ceil((float)gPixelScrollDeltaX/(float)pixelHeight);

            gPixelScrollDeltaX -= numLines*pixelHeight;

            nsWeakFrame weakFrame(aTargetFrame);
            SendLineScrollEvent(aTargetFrame, msEvent, aPresContext,
              aStatus, numLines);
            NS_ENSURE_STATE(weakFrame.IsAlive());
          }
        } else if (msEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal) {
          gPixelScrollDeltaY += msEvent->delta;
          if (!gPixelScrollDeltaY || !pixelHeight)
            break;

          if (PR_ABS(gPixelScrollDeltaY) >= pixelHeight) {
            PRInt32 numLines = (PRInt32)ceil((float)gPixelScrollDeltaY/(float)pixelHeight);

            gPixelScrollDeltaY -= numLines*pixelHeight;

            nsWeakFrame weakFrame(aTargetFrame);
            SendLineScrollEvent(aTargetFrame, msEvent, aPresContext,
              aStatus, numLines);
            NS_ENSURE_STATE(weakFrame.IsAlive());
          }
        }
      }

      
      if ((msEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal) ?
           mLastLineScrollConsumedX : mLastLineScrollConsumedY) {
        *aStatus = nsEventStatus_eConsumeNoDefault;
      }
    }
    break;
  case NS_QUERY_SELECTED_TEXT:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQuerySelectedText((nsQueryContentEvent*)aEvent);
    }
    break;
  case NS_QUERY_TEXT_CONTENT:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQueryTextContent((nsQueryContentEvent*)aEvent);
    }
    break;
  case NS_QUERY_CARET_RECT:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQueryCaretRect((nsQueryContentEvent*)aEvent);
    }
    break;
  case NS_QUERY_TEXT_RECT:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQueryTextRect((nsQueryContentEvent*)aEvent);
    }
    break;
  case NS_QUERY_EDITOR_RECT:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQueryEditorRect((nsQueryContentEvent*)aEvent);
    }
    break;
  case NS_QUERY_CONTENT_STATE:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQueryContentState(static_cast<nsQueryContentEvent*>(aEvent));
    }
    break;
  case NS_QUERY_SELECTION_AS_TRANSFERABLE:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQuerySelectionAsTransferable(static_cast<nsQueryContentEvent*>(aEvent));
    }
    break;
  case NS_QUERY_CHARACTER_AT_POINT:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnQueryCharacterAtPoint(static_cast<nsQueryContentEvent*>(aEvent));
    }
    break;
  case NS_SELECTION_SET:
    {
      nsContentEventHandler handler(mPresContext);
      handler.OnSelectionEvent((nsSelectionEvent*)aEvent);
    }
    break;
  case NS_CONTENT_COMMAND_CUT:
  case NS_CONTENT_COMMAND_COPY:
  case NS_CONTENT_COMMAND_PASTE:
  case NS_CONTENT_COMMAND_DELETE:
  case NS_CONTENT_COMMAND_UNDO:
  case NS_CONTENT_COMMAND_REDO:
    {
      DoContentCommandEvent(static_cast<nsContentCommandEvent*>(aEvent));
    }
    break;
  }
  return NS_OK;
}

static PRInt32
GetAccessModifierMask(nsISupports* aDocShell)
{
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(aDocShell));
  if (!treeItem)
    return -1; 

  PRInt32 itemType;
  treeItem->GetItemType(&itemType);
  switch (itemType) {

  case nsIDocShellTreeItem::typeChrome:
    return sChromeAccessModifier;

  case nsIDocShellTreeItem::typeContent:
    return sContentAccessModifier;

  default:
    return -1; 
  }
}

static PRBool
IsAccessKeyTarget(nsIContent* aContent, nsIFrame* aFrame, nsAString& aKey)
{
  if (!aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::accesskey, aKey,
                             eIgnoreCase))
    return PR_FALSE;

  nsCOMPtr<nsIDOMXULDocument> xulDoc =
    do_QueryInterface(aContent->GetOwnerDoc());
  if (!xulDoc && !aContent->IsNodeOfType(nsINode::eXUL))
    return PR_TRUE;

    
  if (!aFrame)
    return PR_FALSE;

  if (aFrame->IsFocusable())
    return PR_TRUE;

  if (!aFrame->GetStyleVisibility()->IsVisible())
    return PR_FALSE;

  if (!aFrame->AreAncestorViewsVisible())
    return PR_FALSE;

  
  nsCOMPtr<nsIDOMXULControlElement> control(do_QueryInterface(aContent));
  if (control)
    return PR_TRUE;

  if (aContent->IsNodeOfType(nsINode::eHTML)) {
    nsIAtom* tag = aContent->Tag();

    
    
    if (tag == nsGkAtoms::area ||
        tag == nsGkAtoms::label ||
        tag == nsGkAtoms::legend)
      return PR_TRUE;

  } else if (aContent->IsNodeOfType(nsINode::eXUL)) {
    
    
    if (aContent->Tag() == nsGkAtoms::label)
      return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
nsEventStateManager::ExecuteAccessKey(nsTArray<PRUint32>& aAccessCharCodes,
                                      PRBool aIsTrustedEvent)
{
  PRInt32 count, start = -1;
  nsIContent* focusedContent = GetFocusedContent();
  if (focusedContent) {
    start = mAccessKeys.IndexOf(focusedContent);
    if (start == -1 && focusedContent->GetBindingParent())
      start = mAccessKeys.IndexOf(focusedContent->GetBindingParent());
  }
  nsIContent *content;
  nsIFrame *frame;
  PRInt32 length = mAccessKeys.Count();
  for (PRUint32 i = 0; i < aAccessCharCodes.Length(); ++i) {
    PRUint32 ch = aAccessCharCodes[i];
    nsAutoString accessKey;
    AppendUCS4ToUTF16(ch, accessKey);
    for (count = 1; count <= length; ++count) {
      content = mAccessKeys[(start + count) % length];
      frame = mPresContext->PresShell()->GetPrimaryFrameFor(content);
      if (IsAccessKeyTarget(content, frame, accessKey)) {
        PRBool shouldActivate = sKeyCausesActivation;
        while (shouldActivate && ++count <= length) {
          nsIContent *oc = mAccessKeys[(start + count) % length];
          nsIFrame *of = mPresContext->PresShell()->GetPrimaryFrameFor(oc);
          if (IsAccessKeyTarget(oc, of, accessKey))
            shouldActivate = PR_FALSE;
        }
        if (shouldActivate)
          content->PerformAccesskey(shouldActivate, aIsTrustedEvent);
        else {
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            nsCOMPtr<nsIDOMElement> element = do_QueryInterface(content);
            fm->SetFocus(element, nsIFocusManager::FLAG_BYKEY);
          }
        }
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

void
nsEventStateManager::HandleAccessKey(nsPresContext* aPresContext,
                                     nsKeyEvent *aEvent,
                                     nsEventStatus* aStatus,
                                     nsIDocShellTreeItem* aBubbledFrom,
                                     ProcessingAccessKeyState aAccessKeyState,
                                     PRInt32 aModifierMask)
{
  nsCOMPtr<nsISupports> pcContainer = aPresContext->GetContainer();

  
  if (mAccessKeys.Count() > 0 &&
      aModifierMask == GetAccessModifierMask(pcContainer)) {
    
    PRBool isTrusted = NS_IS_TRUSTED_EVENT(aEvent);
    nsAutoTArray<PRUint32, 10> accessCharCodes;
    nsContentUtils::GetAccessKeyCandidates(aEvent, accessCharCodes);
    if (ExecuteAccessKey(accessCharCodes, isTrusted)) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  
  if (nsEventStatus_eConsumeNoDefault != *aStatus) {
    

    nsCOMPtr<nsIDocShellTreeNode> docShell(do_QueryInterface(pcContainer));
    if (!docShell) {
      NS_WARNING("no docShellTreeNode for presContext");
      return;
    }

    PRInt32 childCount;
    docShell->GetChildCount(&childCount);
    for (PRInt32 counter = 0; counter < childCount; counter++) {
      
      nsCOMPtr<nsIDocShellTreeItem> subShellItem;
      docShell->GetChildAt(counter, getter_AddRefs(subShellItem));
      if (aAccessKeyState == eAccessKeyProcessingUp &&
          subShellItem == aBubbledFrom)
        continue;

      nsCOMPtr<nsIDocShell> subDS = do_QueryInterface(subShellItem);
      if (subDS && IsShellVisible(subDS)) {
        nsCOMPtr<nsIPresShell> subPS;
        subDS->GetPresShell(getter_AddRefs(subPS));

        
        
        if (!subPS) {
          
          continue;
        }

        nsPresContext *subPC = subPS->GetPresContext();

        nsEventStateManager* esm =
          static_cast<nsEventStateManager *>(subPC->EventStateManager());

        if (esm)
          esm->HandleAccessKey(subPC, aEvent, aStatus, nsnull,
                               eAccessKeyProcessingDown, aModifierMask);

        if (nsEventStatus_eConsumeNoDefault == *aStatus)
          break;
      }
    }
  }

  
  if (eAccessKeyProcessingDown != aAccessKeyState && nsEventStatus_eConsumeNoDefault != *aStatus) {
    nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryInterface(pcContainer));
    if (!docShell) {
      NS_WARNING("no docShellTreeItem for presContext");
      return;
    }

    nsCOMPtr<nsIDocShellTreeItem> parentShellItem;
    docShell->GetParent(getter_AddRefs(parentShellItem));
    nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(parentShellItem);
    if (parentDS) {
      nsCOMPtr<nsIPresShell> parentPS;

      parentDS->GetPresShell(getter_AddRefs(parentPS));
      NS_ASSERTION(parentPS, "Our PresShell exists but the parent's does not?");

      nsPresContext *parentPC = parentPS->GetPresContext();
      NS_ASSERTION(parentPC, "PresShell without PresContext");

      nsEventStateManager* esm =
        static_cast<nsEventStateManager *>(parentPC->EventStateManager());

      if (esm)
        esm->HandleAccessKey(parentPC, aEvent, aStatus, docShell,
                             eAccessKeyProcessingUp, aModifierMask);
    }
  }
}


#ifdef CLICK_HOLD_CONTEXT_MENUS









void
nsEventStateManager::CreateClickHoldTimer(nsPresContext* inPresContext,
                                          nsIFrame* inDownFrame,
                                          nsGUIEvent* inMouseDownEvent)
{
  if (!NS_IS_TRUSTED_EVENT(inMouseDownEvent))
    return;

  
  if (mClickHoldTimer) {
    mClickHoldTimer->Cancel();
    mClickHoldTimer = nsnull;
  }

  
  
  if (mGestureDownContent) {
    
    if (nsContentUtils::HasNonEmptyAttr(mGestureDownContent, kNameSpaceID_None,
                                        nsGkAtoms::popup))
      return;
    
    
    if (mGestureDownContent->Tag() == nsGkAtoms::menubutton)
      return;
  }

  mClickHoldTimer = do_CreateInstance("@mozilla.org/timer;1");
  if ( mClickHoldTimer )
    mClickHoldTimer->InitWithFuncCallback(sClickHoldCallback, this,
                                          kClickHoldDelay,
                                          nsITimer::TYPE_ONE_SHOT);
} 







void
nsEventStateManager::KillClickHoldTimer()
{
  if (mClickHoldTimer) {
    mClickHoldTimer->Cancel();
    mClickHoldTimer = nsnull;
  }
}







void
nsEventStateManager::sClickHoldCallback(nsITimer *aTimer, void* aESM)
{
  nsEventStateManager* self = static_cast<nsEventStateManager*>(aESM);
  if ( self )
    self->FireContextClick();

  

} 
















void
nsEventStateManager::FireContextClick()
{
  if ( !mGestureDownContent )
    return;

#ifdef XP_MACOSX
  
  
  
  if (!::StillDown())
    return;
#endif

  nsEventStatus status = nsEventStatus_eIgnore;

  
  
  
  
  
  
  mCurrentTarget = nsnull;
  nsIPresShell *shell = mPresContext->GetPresShell();
  if ( shell ) {
    mCurrentTarget = shell->GetPrimaryFrameFor(mGestureDownFrameOwner);

    if ( mCurrentTarget ) {
      NS_ASSERTION(mPresContext == mCurrentTarget->PresContext(),
                   "a prescontext returned a primary frame that didn't belong to it?");

      
      
      nsIAtom *tag = mGestureDownContent->Tag();
      PRBool allowedToDispatch = PR_TRUE;

      if (mGestureDownContent->IsNodeOfType(nsINode::eXUL)) {
        if (tag == nsGkAtoms::scrollbar ||
            tag == nsGkAtoms::scrollbarbutton ||
            tag == nsGkAtoms::button)
          allowedToDispatch = PR_FALSE;
        else if (tag == nsGkAtoms::toolbarbutton) {
          
          
          if (nsContentUtils::HasNonEmptyAttr(mGestureDownContent,
                  kNameSpaceID_None, nsGkAtoms::container)) {
            allowedToDispatch = PR_FALSE;
          } else {
            
            
            if (mGestureDownContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::open,
                                                 nsGkAtoms::_true, eCaseMatters)) {
              allowedToDispatch = PR_FALSE;
            }
          }
        }
      }
      else if (mGestureDownContent->IsNodeOfType(nsINode::eHTML)) {
        nsCOMPtr<nsIFormControl> formCtrl(do_QueryInterface(mGestureDownContent));

        if (formCtrl) {
          
          
          PRInt32 type = formCtrl->GetType();

          allowedToDispatch = (type == NS_FORM_INPUT_TEXT ||
                               type == NS_FORM_INPUT_PASSWORD ||
                               type == NS_FORM_INPUT_FILE ||
                               type == NS_FORM_TEXTAREA);
        }
        else if (tag == nsGkAtoms::applet ||
                 tag == nsGkAtoms::embed  ||
                 tag == nsGkAtoms::object) {
          allowedToDispatch = PR_FALSE;
        }
      }

      if (allowedToDispatch) {
        
        nsCOMPtr<nsIWidget> targetWidget(mCurrentTarget->GetWindow());
        
        nsMouseEvent event(PR_TRUE, NS_CONTEXTMENU,
                           targetWidget,
                           nsMouseEvent::eReal);
        event.clickCount = 1;
        FillInEventFromGestureDown(&event);
        
        
        if (mCurrentTarget)
        {
          nsCOMPtr<nsFrameSelection> frameSel =
            mCurrentTarget->GetFrameSelection();
        
          if (frameSel && frameSel->GetMouseDownState()) {
            
            
            frameSel->SetMouseDownState(PR_FALSE);
          }
        }

        
        nsEventDispatcher::Dispatch(mGestureDownContent, mPresContext, &event,
                                    nsnull, &status);

        
        
        
        
      }
    }
  }

  
  if ( status == nsEventStatus_eConsumeNoDefault ) {
    StopTrackingDragGesture();
  }

  KillClickHoldTimer();

} 

#endif














void
nsEventStateManager::BeginTrackingDragGesture(nsPresContext* aPresContext,
                                              nsMouseEvent* inDownEvent,
                                              nsIFrame* inDownFrame)
{
  
  
  mGestureDownPoint = inDownEvent->refPoint + 
    inDownEvent->widget->WidgetToScreenOffset();

  inDownFrame->GetContentForEvent(aPresContext, inDownEvent,
                                  getter_AddRefs(mGestureDownContent));

  mGestureDownFrameOwner = inDownFrame->GetContent();
  mGestureDownShift = inDownEvent->isShift;
  mGestureDownControl = inDownEvent->isControl;
  mGestureDownAlt = inDownEvent->isAlt;
  mGestureDownMeta = inDownEvent->isMeta;

#ifdef CLICK_HOLD_CONTEXT_MENUS
  
  if (nsContentUtils::GetBoolPref("ui.click_hold_context_menus", PR_TRUE))
    CreateClickHoldTimer ( aPresContext, inDownFrame, inDownEvent );
#endif
}








void
nsEventStateManager::StopTrackingDragGesture()
{
  mGestureDownContent = nsnull;
  mGestureDownFrameOwner = nsnull;
}

void
nsEventStateManager::FillInEventFromGestureDown(nsMouseEvent* aEvent)
{
  NS_ASSERTION(aEvent->widget == mCurrentTarget->GetWindow(),
               "Incorrect widget in event");

  
  
  
  nsIntPoint tmpPoint = aEvent->widget->WidgetToScreenOffset();
  aEvent->refPoint = mGestureDownPoint - tmpPoint;
  aEvent->isShift = mGestureDownShift;
  aEvent->isControl = mGestureDownControl;
  aEvent->isAlt = mGestureDownAlt;
  aEvent->isMeta = mGestureDownMeta;
}















void
nsEventStateManager::GenerateDragGesture(nsPresContext* aPresContext,
                                         nsMouseEvent *aEvent)
{
  NS_ASSERTION(aPresContext, "This shouldn't happen.");
  if ( IsTrackingDragGesture() ) {
    mCurrentTarget = aPresContext->GetPresShell()->GetPrimaryFrameFor(mGestureDownFrameOwner);

    if (!mCurrentTarget) {
      StopTrackingDragGesture();
      return;
    }

    
    
    if (mCurrentTarget)
    {
      nsCOMPtr<nsFrameSelection> frameSel = mCurrentTarget->GetFrameSelection();
      if (frameSel && frameSel->GetMouseDownState()) {
        StopTrackingDragGesture();
        return;
      }
    }

    static PRInt32 pixelThresholdX = 0;
    static PRInt32 pixelThresholdY = 0;

    if (!pixelThresholdX) {
      nsILookAndFeel *lf = aPresContext->LookAndFeel();
      lf->GetMetric(nsILookAndFeel::eMetric_DragThresholdX, pixelThresholdX);
      lf->GetMetric(nsILookAndFeel::eMetric_DragThresholdY, pixelThresholdY);
      if (!pixelThresholdX)
        pixelThresholdX = 5;
      if (!pixelThresholdY)
        pixelThresholdY = 5;
    }

    
    nsIntPoint pt = aEvent->refPoint + aEvent->widget->WidgetToScreenOffset();
    if (PR_ABS(pt.x - mGestureDownPoint.x) > pixelThresholdX ||
        PR_ABS(pt.y - mGestureDownPoint.y) > pixelThresholdY) {
#ifdef CLICK_HOLD_CONTEXT_MENUS
      
      
      KillClickHoldTimer();
#endif

      nsRefPtr<nsDOMDataTransfer> dataTransfer = new nsDOMDataTransfer();
      if (!dataTransfer)
        return;

      PRBool isInEditor = PR_FALSE;
      PRBool isSelection = PR_FALSE;
      nsCOMPtr<nsIContent> eventContent, targetContent;
      mCurrentTarget->GetContentForEvent(aPresContext, aEvent,
                                         getter_AddRefs(eventContent));
      if (eventContent)
        DetermineDragTarget(aPresContext, eventContent, dataTransfer,
                            &isSelection, &isInEditor,
                            getter_AddRefs(targetContent));

      
      
      StopTrackingDragGesture();

      if (!targetContent)
        return;

      nsCOMPtr<nsIWidget> widget = mCurrentTarget->GetWindow();

      
      nsDragEvent startEvent(NS_IS_TRUSTED_EVENT(aEvent), NS_DRAGDROP_START, widget);
      FillInEventFromGestureDown(&startEvent);

      nsDragEvent gestureEvent(NS_IS_TRUSTED_EVENT(aEvent), NS_DRAGDROP_GESTURE, widget);
      FillInEventFromGestureDown(&gestureEvent);

      startEvent.dataTransfer = gestureEvent.dataTransfer = dataTransfer;

      
      
      
      
      
      
      
      

      
      nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

      
      mCurrentTargetContent = targetContent;

      
      
      
      nsEventStatus status = nsEventStatus_eIgnore;
      if (!isInEditor)
        nsEventDispatcher::Dispatch(targetContent, aPresContext, &startEvent, nsnull,
                                    &status);

      nsDragEvent* event = &startEvent;
      if (status != nsEventStatus_eConsumeNoDefault) {
        status = nsEventStatus_eIgnore;
        nsEventDispatcher::Dispatch(targetContent, aPresContext, &gestureEvent, nsnull,
                                    &status);
        event = &gestureEvent;
      }

      
      
      
      dataTransfer->SetReadOnly();

      if (status != nsEventStatus_eConsumeNoDefault)
        DoDefaultDragStart(aPresContext, event, dataTransfer,
                           targetContent, isSelection);

      
      
      

      
      mCurrentTargetContent = targetBeforeEvent;
    }

    
    
    FlushPendingEvents(aPresContext);
  }
} 

void
nsEventStateManager::DetermineDragTarget(nsPresContext* aPresContext,
                                         nsIContent* aSelectionTarget,
                                         nsDOMDataTransfer* aDataTransfer,
                                         PRBool* aIsSelection,
                                         PRBool* aIsInEditor,
                                         nsIContent** aTargetNode)
{
  *aTargetNode = nsnull;
  *aIsInEditor = PR_FALSE;

  nsCOMPtr<nsISupports> container = aPresContext->GetContainer();
  nsCOMPtr<nsIDOMWindow> window = do_GetInterface(container);

  
  
  
  PRBool canDrag;
  nsCOMPtr<nsIContent> dragDataNode;
  nsCOMPtr<nsIDocShellTreeItem> dsti = do_QueryInterface(container);
  if (dsti) {
    PRInt32 type = -1;
    if (NS_SUCCEEDED(dsti->GetItemType(&type)) &&
        type != nsIDocShellTreeItem::typeChrome) {
      
      
      nsresult rv =
        nsContentAreaDragDrop::GetDragData(window, mGestureDownContent,
                                           aSelectionTarget, mGestureDownAlt,
                                           aDataTransfer, &canDrag, aIsSelection,
                                           getter_AddRefs(dragDataNode));
      if (NS_FAILED(rv) || !canDrag)
        return;
    }
  }

  
  
  
  nsIContent* dragContent = mGestureDownContent;
  if (dragDataNode)
    dragContent = dragDataNode;
  else if (*aIsSelection)
    dragContent = aSelectionTarget;

  nsIContent* originalDragContent = dragContent;

  
  
  
  
  if (!*aIsSelection) {
    while (dragContent) {
      nsCOMPtr<nsIDOMNSHTMLElement> htmlElement = do_QueryInterface(dragContent);
      if (htmlElement) {
        PRBool draggable = PR_FALSE;
        htmlElement->GetDraggable(&draggable);
        if (draggable)
          break;
      }
      else {
        nsCOMPtr<nsIDOMXULElement> xulElement = do_QueryInterface(dragContent);
        if (xulElement) {
          
          
          
          
          
          
          
          dragContent = mGestureDownContent;
          break;
        }
        
      }
      dragContent = dragContent->GetParent();

      
      
      
      
      
      nsCOMPtr<nsIDOMNSEditableElement> editableElement = do_QueryInterface(dragContent);
      if (editableElement) {
        *aIsInEditor = PR_TRUE;
        break;
      }
    }
  }

  
  
  if (!dragContent && dragDataNode)
    dragContent = dragDataNode;

  if (dragContent) {
    
    
    if (dragContent != originalDragContent)
      aDataTransfer->ClearAll();
    *aTargetNode = dragContent;
    NS_ADDREF(*aTargetNode);
  }
}

void
nsEventStateManager::DoDefaultDragStart(nsPresContext* aPresContext,
                                        nsDragEvent* aDragEvent,
                                        nsDOMDataTransfer* aDataTransfer,
                                        nsIContent* aDragTarget,
                                        PRBool aIsSelection)
{
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (!dragService)
    return;

  
  
  
  
  
  
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (dragSession)
    return; 

  
  
  PRUint32 count = 0;
  if (aDataTransfer)
    aDataTransfer->GetMozItemCount(&count);
  if (!count)
    return;

  
  
  
  
  nsCOMPtr<nsIDOMNode> dragTarget;
  nsCOMPtr<nsIDOMElement> dragTargetElement;
  aDataTransfer->GetDragTarget(getter_AddRefs(dragTargetElement));
  dragTarget = do_QueryInterface(dragTargetElement);
  if (!dragTarget) {
    dragTarget = do_QueryInterface(aDragTarget);
    if (!dragTarget)
      return;
  }

  
  
  PRUint32 action;
  aDataTransfer->GetEffectAllowedInt(&action);
  if (action == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED)
    action = nsIDragService::DRAGDROP_ACTION_COPY |
             nsIDragService::DRAGDROP_ACTION_MOVE |
             nsIDragService::DRAGDROP_ACTION_LINK;

  
  PRInt32 imageX, imageY;
  nsIDOMElement* dragImage = aDataTransfer->GetDragImage(&imageX, &imageY);

  
  
  
  
  nsISelection* selection = nsnull;
  if (aIsSelection && !dragImage) {
    nsIDocument* doc = aDragTarget->GetCurrentDoc();
    if (doc) {
      nsIPresShell* presShell = doc->GetPrimaryShell();
      if (presShell) {
        selection = presShell->GetCurrentSelection(
                      nsISelectionController::SELECTION_NORMAL);
      }
    }
  }

  nsCOMPtr<nsISupportsArray> transArray;
  aDataTransfer->GetTransferables(getter_AddRefs(transArray));
  if (!transArray)
    return;

  
  
  
  nsCOMPtr<nsIDOMEvent> domEvent;
  NS_NewDOMDragEvent(getter_AddRefs(domEvent), aPresContext, aDragEvent);

  nsCOMPtr<nsIDOMDragEvent> domDragEvent = do_QueryInterface(domEvent);
  
  
  if (selection) {
    dragService->InvokeDragSessionWithSelection(selection, transArray,
                                                action, domDragEvent,
                                                aDataTransfer);
  }
  else {
    
    
    
    
    
    nsCOMPtr<nsIScriptableRegion> region;
#ifdef MOZ_XUL
    if (dragTarget && !dragImage) {
      nsCOMPtr<nsIContent> content = do_QueryInterface(dragTarget);
      if (content->NodeInfo()->Equals(nsGkAtoms::treechildren,
                                      kNameSpaceID_XUL)) {
        nsIDocument* doc = content->GetCurrentDoc();
        if (doc) {
          nsIPresShell* presShell = doc->GetPrimaryShell();
          if (presShell) {
            nsIFrame* frame = presShell->GetPrimaryFrameFor(content);
            if (frame) {
              nsTreeBodyFrame* treeBody = do_QueryFrame(frame);
              treeBody->GetSelectionRegion(getter_AddRefs(region));
            }
          }
        }
      }
    }
#endif

    dragService->InvokeDragSessionWithImage(dragTarget, transArray,
                                            region, action, dragImage,
                                            imageX, imageY, domDragEvent,
                                            aDataTransfer);
  }
}

nsresult
nsEventStateManager::GetMarkupDocumentViewer(nsIMarkupDocumentViewer** aMv)
{
  *aMv = nsnull;

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if(!fm) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));

  nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(focusedWindow);
  if(!ourWindow) return NS_ERROR_FAILURE;

  nsIDOMWindowInternal *rootWindow = ourWindow->GetPrivateRoot();
  if(!rootWindow) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> contentWindow;
  rootWindow->GetContent(getter_AddRefs(contentWindow));
  if(!contentWindow) return NS_ERROR_FAILURE;

  nsIDocument *doc = GetDocumentFromWindow(contentWindow);
  if(!doc) return NS_ERROR_FAILURE;

  nsIPresShell *presShell = doc->GetPrimaryShell();
  if(!presShell) return NS_ERROR_FAILURE;
  nsPresContext *presContext = presShell->GetPresContext();
  if(!presContext) return NS_ERROR_FAILURE;

  nsCOMPtr<nsISupports> pcContainer = presContext->GetContainer();
  if(!pcContainer) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDocShell> docshell(do_QueryInterface(pcContainer));
  if(!docshell) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContentViewer> cv;
  docshell->GetContentViewer(getter_AddRefs(cv));
  if(!cv) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIMarkupDocumentViewer> mv(do_QueryInterface(cv));
  if(!mv) return NS_ERROR_FAILURE;

  *aMv = mv;
  NS_IF_ADDREF(*aMv);

  return NS_OK;
}

nsresult
nsEventStateManager::ChangeTextSize(PRInt32 change)
{
  nsCOMPtr<nsIMarkupDocumentViewer> mv;
  nsresult rv = GetMarkupDocumentViewer(getter_AddRefs(mv));
  NS_ENSURE_SUCCESS(rv, rv);

  float textzoom;
  float zoomMin = ((float)nsContentUtils::GetIntPref("zoom.minPercent", 50)) / 100;
  float zoomMax = ((float)nsContentUtils::GetIntPref("zoom.maxPercent", 300)) / 100;
  mv->GetTextZoom(&textzoom);
  textzoom += ((float)change) / 10;
  if (textzoom < zoomMin)
    textzoom = zoomMin;
  else if (textzoom > zoomMax)
    textzoom = zoomMax;
  mv->SetTextZoom(textzoom);

  return NS_OK;
}

nsresult
nsEventStateManager::ChangeFullZoom(PRInt32 change)
{
  nsCOMPtr<nsIMarkupDocumentViewer> mv;
  nsresult rv = GetMarkupDocumentViewer(getter_AddRefs(mv));
  NS_ENSURE_SUCCESS(rv, rv);

  float fullzoom;
  float zoomMin = ((float)nsContentUtils::GetIntPref("zoom.minPercent", 50)) / 100;
  float zoomMax = ((float)nsContentUtils::GetIntPref("zoom.maxPercent", 300)) / 100;
  mv->GetFullZoom(&fullzoom);
  fullzoom += ((float)change) / 10;
  if (fullzoom < zoomMin)
    fullzoom = zoomMin;
  else if (fullzoom > zoomMax)
    fullzoom = zoomMax;
  mv->SetFullZoom(fullzoom);

  return NS_OK;
}

void
nsEventStateManager::DoScrollHistory(PRInt32 direction)
{
  nsCOMPtr<nsISupports> pcContainer(mPresContext->GetContainer());
  if (pcContainer) {
    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(pcContainer));
    if (webNav) {
      
      if (direction > 0)
        webNav->GoBack();
      else
        webNav->GoForward();
    }
  }
}

void
nsEventStateManager::DoScrollZoom(nsIFrame *aTargetFrame,
                                  PRInt32 adjustment)
{
  
  nsIContent *content = aTargetFrame->GetContent();
  if (content &&
      !content->IsNodeOfType(nsINode::eHTML_FORM_CONTROL) &&
      !content->IsNodeOfType(nsINode::eXUL))
    {
      
      PRInt32 change = (adjustment > 0) ? -1 : 1;

      if (nsContentUtils::GetBoolPref("browser.zoom.full"))
        ChangeFullZoom(change);
      else
        ChangeTextSize(change);
    }
}

static nsIFrame*
GetParentFrameToScroll(nsPresContext* aPresContext, nsIFrame* aFrame)
{
  if (!aPresContext || !aFrame)
    return nsnull;

  if (aFrame->GetStyleDisplay()->mPosition == NS_STYLE_POSITION_FIXED &&
      nsLayoutUtils::IsReallyFixedPos(aFrame))
    return aPresContext->GetPresShell()->GetRootScrollFrame();

  return aFrame->GetParent();
}

static nsIScrollableView*
GetScrollableViewForFrame(nsPresContext* aPresContext, nsIFrame* aFrame)
{
  for (; aFrame; aFrame = GetParentFrameToScroll(aPresContext, aFrame)) {
    nsIScrollableViewProvider* svp = do_QueryFrame(aFrame);
    if (svp) {
      nsIScrollableView* scrollView = svp->GetScrollableView();
      if (scrollView)
        return scrollView;
    }
  }
  return nsnull;
}

static nscoord
GetScrollableViewLineHeight(nsPresContext* aPresContext, nsIFrame* aTargetFrame)
{
  nsIScrollableView* scrollView = GetScrollableViewForFrame(aPresContext, aTargetFrame);
  nscoord lineHeight = 0;
  if (scrollView) {
    scrollView->GetLineHeight(&lineHeight);
  } else {
    
    const nsStyleFont* font = aTargetFrame->GetStyleFont();
    const nsFont& f = font->mFont;
    nsCOMPtr<nsIFontMetrics> fm = aPresContext->GetMetricsFor(f);
    NS_ASSERTION(fm, "FontMetrics is null!");
    if (fm)
      fm->GetHeight(lineHeight);
  }
  return lineHeight;
}

void
nsEventStateManager::SendLineScrollEvent(nsIFrame* aTargetFrame,
                                         nsMouseScrollEvent* aEvent,
                                         nsPresContext* aPresContext,
                                         nsEventStatus* aStatus,
                                         PRInt32 aNumLines)
{
  nsCOMPtr<nsIContent> targetContent = aTargetFrame->GetContent();
  if (!targetContent)
    targetContent = GetFocusedContent();
  if (!targetContent)
    return;

  while (targetContent->IsNodeOfType(nsINode::eTEXT)) {
    targetContent = targetContent->GetParent();
  }

  PRBool isTrusted = (aEvent->flags & NS_EVENT_FLAG_TRUSTED) != 0;
  nsMouseScrollEvent event(isTrusted, NS_MOUSE_SCROLL, nsnull);
  event.refPoint = aEvent->refPoint;
  event.widget = aEvent->widget;
  event.time = aEvent->time;
  event.isShift = aEvent->isShift;
  event.isControl = aEvent->isControl;
  event.isAlt = aEvent->isAlt;
  event.isMeta = aEvent->isMeta;
  event.scrollFlags = aEvent->scrollFlags;
  event.delta = aNumLines;

  nsEventDispatcher::Dispatch(targetContent, aPresContext, &event, nsnull, aStatus);
}

void
nsEventStateManager::SendPixelScrollEvent(nsIFrame* aTargetFrame,
                                          nsMouseScrollEvent* aEvent,
                                          nsPresContext* aPresContext,
                                          nsEventStatus* aStatus)
{
  nsCOMPtr<nsIContent> targetContent = aTargetFrame->GetContent();
  if (!targetContent) {
    targetContent = GetFocusedContent();
    if (!targetContent)
      return;
  }

  while (targetContent->IsNodeOfType(nsINode::eTEXT)) {
    targetContent = targetContent->GetParent();
  }

  nscoord lineHeight = GetScrollableViewLineHeight(aPresContext, aTargetFrame);

  PRBool isTrusted = (aEvent->flags & NS_EVENT_FLAG_TRUSTED) != 0;
  nsMouseScrollEvent event(isTrusted, NS_MOUSE_PIXEL_SCROLL, nsnull);
  event.refPoint = aEvent->refPoint;
  event.widget = aEvent->widget;
  event.time = aEvent->time;
  event.isShift = aEvent->isShift;
  event.isControl = aEvent->isControl;
  event.isAlt = aEvent->isAlt;
  event.isMeta = aEvent->isMeta;
  event.scrollFlags = aEvent->scrollFlags;
  event.delta = aPresContext->AppUnitsToIntCSSPixels(aEvent->delta * lineHeight);

  nsEventDispatcher::Dispatch(targetContent, aPresContext, &event, nsnull, aStatus);
}

nsresult
nsEventStateManager::DoScrollText(nsPresContext* aPresContext,
                                  nsIFrame* aTargetFrame,
                                  nsMouseScrollEvent* aMouseEvent,
                                  ScrollQuantity aScrollQuantity)
{
  nsIScrollableView* scrollView = nsnull;
  nsIFrame* scrollFrame = aTargetFrame;
  PRInt32 numLines = aMouseEvent->delta;
  PRBool isHorizontal = aMouseEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal;
  aMouseEvent->scrollOverflow = 0;

  
  
  
  
  
  
  
  
  nsIFrame* lastScrollFrame = nsMouseWheelTransaction::GetTargetFrame();
  if (lastScrollFrame) {
    nsIScrollableViewProvider* svp = do_QueryFrame(lastScrollFrame);
    if (svp && (scrollView = svp->GetScrollableView())) {
      nsMouseWheelTransaction::UpdateTransaction(numLines, isHorizontal);
      
      
      
      
      if (!nsMouseWheelTransaction::GetTargetFrame())
        return NS_OK;
    } else {
      nsMouseWheelTransaction::EndTransaction();
      lastScrollFrame = nsnull;
    }
  }
  PRBool passToParent = lastScrollFrame ? PR_FALSE : PR_TRUE;

  for (; scrollFrame && passToParent;
       scrollFrame = GetParentFrameToScroll(aPresContext, scrollFrame)) {
    
    scrollView = nsnull;
    nsIScrollableViewProvider* svp = do_QueryFrame(scrollFrame);
    if (svp) {
      scrollView = svp->GetScrollableView();
    }
    if (!scrollView) {
      continue;
    }

    nsPresContext::ScrollbarStyles ss =
      nsLayoutUtils::ScrollbarStylesOfView(scrollView);
    if (NS_STYLE_OVERFLOW_HIDDEN ==
        (isHorizontal ? ss.mHorizontal : ss.mVertical)) {
      continue;
    }

    
    nscoord lineHeight;
    scrollView->GetLineHeight(&lineHeight);

    if (lineHeight != 0) {
      if (CanScrollOn(scrollView, numLines, isHorizontal)) {
        passToParent = PR_FALSE;
        nsMouseWheelTransaction::BeginTransaction(scrollFrame,
                                                  numLines, isHorizontal);
      }

      
      nsIComboboxControlFrame* comboBox = do_QueryFrame(scrollFrame);
      if (comboBox) {
        if (comboBox->IsDroppedDown()) {
          
          if (passToParent) {
            passToParent = PR_FALSE;
            scrollView = nsnull;
            nsMouseWheelTransaction::EndTransaction();
          }
        } else {
          
          if (!passToParent) {
            passToParent = PR_TRUE;
            nsMouseWheelTransaction::EndTransaction();
          }
        }
      }
    }
  }

  if (!passToParent && scrollView) {
    if (aScrollQuantity == eScrollByLine) {
      
      
      nscoord lineHeight = 0;
      scrollView->GetLineHeight(&lineHeight);
      if (lineHeight) {
        nsSize pageScrollDistances(0, 0);
        scrollView->GetPageScrollDistances(&pageScrollDistances);
        nscoord pageScroll = isHorizontal ?
          pageScrollDistances.width : pageScrollDistances.height;

        if (PR_ABS(numLines) * lineHeight > pageScroll) {
          nscoord maxLines = (pageScroll / lineHeight);
          if (maxLines >= 1) {
            numLines = ((numLines < 0) ? -1 : 1) * maxLines;
          } else {
            aScrollQuantity = eScrollByPage;
          }
        }
      }
    }

    PRInt32 scrollX = 0;
    PRInt32 scrollY = numLines;

    if (aScrollQuantity == eScrollByPage)
      scrollY = (scrollY > 0) ? 1 : -1;
      
    if (isHorizontal) {
      scrollX = scrollY;
      scrollY = 0;
    }
    
    PRBool noDefer = aMouseEvent->scrollFlags & nsMouseScrollEvent::kNoDefer;
    PRInt32 overflowX = 0, overflowY = 0;
    
    if (aScrollQuantity == eScrollByPage) {
      scrollView->ScrollByPages(scrollX, scrollY,
        (noDefer ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_SMOOTHSCROLL));
    }
    else if (aScrollQuantity == eScrollByPixel) {
      scrollView->ScrollByPixels(scrollX, scrollY, overflowX, overflowY,
        (noDefer ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_DEFERRED));
    }
    else {
      nsMouseWheelTransaction::AccelerateWheelDelta(scrollX, scrollY);
      scrollView->ScrollByLinesWithOverflow(scrollX, scrollY, overflowX, overflowY,
        (noDefer ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_SMOOTHSCROLL));
    }
    
    if (isHorizontal)
      aMouseEvent->scrollOverflow = overflowX;
    else
      aMouseEvent->scrollOverflow = overflowY;

    return NS_OK;
  }
  
  if (passToParent) {
    nsresult rv;
    nsIFrame* newFrame = nsnull;
    nsCOMPtr<nsPresContext> newPresContext;
    rv = GetParentScrollingView(aMouseEvent, aPresContext, newFrame,
                                *getter_AddRefs(newPresContext));
    if (NS_SUCCEEDED(rv) && newFrame)
      return DoScrollText(newPresContext, newFrame, aMouseEvent, aScrollQuantity);
  }

  aMouseEvent->scrollOverflow = numLines;

  return NS_OK;
}

void
nsEventStateManager::DecideGestureEvent(nsGestureNotifyEvent* aEvent,
                                        nsIFrame* targetFrame)
{

  NS_ASSERTION(aEvent->message == NS_GESTURENOTIFY_EVENT_START,
               "DecideGestureEvent called with a non-gesture event");

  











  nsGestureNotifyEvent::ePanDirection panDirection = nsGestureNotifyEvent::ePanNone;
  PRBool displayPanFeedback = PR_FALSE;
  for (nsIFrame* current = targetFrame; current;
       current = nsLayoutUtils::GetCrossDocParentFrame(current)) {

    nsIAtom* currentFrameType = current->GetType();

    
    if (currentFrameType == nsGkAtoms::scrollbarFrame) {
      panDirection = nsGestureNotifyEvent::ePanNone;
      break;
    }

#ifdef MOZ_XUL
    
    nsTreeBodyFrame* treeFrame = do_QueryFrame(current);
    if (treeFrame) {
      if (treeFrame->GetHorizontalOverflow()) {
        panDirection = nsGestureNotifyEvent::ePanHorizontal;
      }
      if (treeFrame->GetVerticalOverflow()) {
        panDirection = nsGestureNotifyEvent::ePanVertical;
      }
      break;
    }
#endif

    nsIScrollableFrame* scrollableFrame = do_QueryFrame(current);
    if (scrollableFrame) {
      if (current->IsFrameOfType(nsIFrame::eXULBox)) {

        nsIScrollableView* scrollableView = scrollableFrame->GetScrollableView();
        if (scrollableView) {

          displayPanFeedback = PR_TRUE;

          PRBool canScrollUp, canScrollDown, canScrollLeft, canScrollRight;
          scrollableView->CanScroll(PR_FALSE, PR_TRUE,  canScrollDown);
          scrollableView->CanScroll(PR_FALSE, PR_FALSE, canScrollUp);
          scrollableView->CanScroll(PR_TRUE,  PR_TRUE,  canScrollRight);
          scrollableView->CanScroll(PR_TRUE,  PR_FALSE, canScrollLeft);

          if (targetFrame->GetType() == nsGkAtoms::menuFrame) {
            
            
            canScrollRight = PR_FALSE;
            canScrollLeft  = PR_FALSE;
            displayPanFeedback = PR_FALSE;
          }

          
          
          if (canScrollUp || canScrollDown) {
            panDirection = nsGestureNotifyEvent::ePanVertical;
            break;
          }

          if (canScrollLeft || canScrollRight) {
            panDirection = nsGestureNotifyEvent::ePanHorizontal;
            displayPanFeedback = PR_FALSE;
          }
        }

      } else { 

        nsMargin scrollbarSizes = scrollableFrame->GetActualScrollbarSizes();

        
        if (scrollbarSizes.LeftRight()) {
          panDirection = nsGestureNotifyEvent::ePanVertical;
          displayPanFeedback = PR_TRUE;
          break;
        }

        if (scrollbarSizes.TopBottom()) {
          panDirection = nsGestureNotifyEvent::ePanHorizontal;
          displayPanFeedback = PR_TRUE;
        }

      }

    } 
  } 

  aEvent->displayPanFeedback = displayPanFeedback;
  aEvent->panDirection = panDirection;

}

nsresult
nsEventStateManager::GetParentScrollingView(nsInputEvent *aEvent,
                                            nsPresContext* aPresContext,
                                            nsIFrame* &targetOuterFrame,
                                            nsPresContext* &presCtxOuter)
{
  targetOuterFrame = nsnull;

  if (!aEvent) return NS_ERROR_FAILURE;
  if (!aPresContext) return NS_ERROR_FAILURE;

  nsIDocument *doc = aPresContext->PresShell()->GetDocument();
  NS_ASSERTION(doc, "No document in prescontext!");

  nsIDocument *parentDoc = doc->GetParentDocument();

  if (!parentDoc) {
    return NS_OK;
  }

  nsIPresShell *pPresShell = nsnull;
  nsPresShellIterator iter(parentDoc);
  nsCOMPtr<nsIPresShell> tmpPresShell;
  while ((tmpPresShell = iter.GetNextShell())) {
    NS_ENSURE_TRUE(tmpPresShell->GetPresContext(), NS_ERROR_FAILURE);
    if (tmpPresShell->GetPresContext()->Type() == aPresContext->Type()) {
      pPresShell = tmpPresShell;
      break;
    }
  }
  if (!pPresShell)
    return NS_ERROR_FAILURE;

  


  nsIContent *frameContent = parentDoc->FindContentForSubDocument(doc);
  NS_ENSURE_TRUE(frameContent, NS_ERROR_FAILURE);

  






  nsIFrame* frameFrame = pPresShell->GetPrimaryFrameFor(frameContent);
  if (!frameFrame) return NS_ERROR_FAILURE;

  NS_IF_ADDREF(presCtxOuter = pPresShell->GetPresContext());
  targetOuterFrame = frameFrame;

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::PostHandleEvent(nsPresContext* aPresContext,
                                     nsEvent *aEvent,
                                     nsIFrame* aTargetFrame,
                                     nsEventStatus* aStatus,
                                     nsIView* aView)
{
  NS_ENSURE_ARG(aPresContext);
  NS_ENSURE_ARG_POINTER(aStatus);

  mCurrentTarget = aTargetFrame;
  mCurrentTargetContent = nsnull;

  
  
  if (!mCurrentTarget &&
      aEvent->message != NS_MOUSE_BUTTON_UP) {
    return NS_OK;
  }

  
  nsRefPtr<nsPresContext> presContext = aPresContext;
  nsresult ret = NS_OK;

  switch (aEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
    {
      if (static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton &&
          !mNormalLMouseEventInProcess) {
        
        
        nsIPresShell::SetCapturingContent(nsnull, 0);
        break;
      }

      if (nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> newFocus;
        nsIContent* activeContent = nsnull;
        PRBool suppressBlur = PR_FALSE;
        if (mCurrentTarget) {
          mCurrentTarget->GetContentForEvent(mPresContext, aEvent, getter_AddRefs(newFocus));
          const nsStyleUserInterface* ui = mCurrentTarget->GetStyleUserInterface();
          suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);
          activeContent = mCurrentTarget->GetContent();
        }

        
        
        nsIFrame* currFrame = mCurrentTarget;
        while (currFrame) {
          
          
          const nsStyleDisplay* display = currFrame->GetStyleDisplay();
          if (display->mDisplay == NS_STYLE_DISPLAY_POPUP) {
            newFocus = nsnull;
            break;
          }

          PRInt32 tabIndexUnused;
          if (currFrame->IsFocusable(&tabIndexUnused, PR_TRUE)) {
            newFocus = currFrame->GetContent();
            nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(newFocus));
            if (domElement)
              break;
          }
          currFrame = currFrame->GetParent();
        }

        nsIFocusManager* fm = nsFocusManager::GetFocusManager();
        if (fm) {
          
          
          
          
          
          
          
          
          
          if (newFocus && currFrame) {
            
            
            
            nsCOMPtr<nsIDOMElement> newFocusElement = do_QueryInterface(newFocus);
            fm->SetFocus(newFocusElement, nsIFocusManager::FLAG_BYMOUSE |
                                          nsIFocusManager::FLAG_NOSCROLL);
          }
          else if (!suppressBlur) {
            
            
            EnsureDocument(mPresContext);
            if (mDocument) {
              fm->ClearFocus(mDocument->GetWindow());
              fm->SetFocusedWindow(mDocument->GetWindow());
            }
          }
        }

        
        if (static_cast<nsMouseEvent*>(aEvent)->button !=
            nsMouseEvent::eLeftButton)
          break;

        if (activeContent) {
          
          
          
          
          
          nsCOMPtr<nsIDOMElement> elt(do_QueryInterface(activeContent));
          if (!elt) {
            nsIContent* par = activeContent->GetParent();
            if (par)
              activeContent = par;
          }
          SetContentState(activeContent, NS_EVENT_STATE_ACTIVE);
        }
      }
      else {
        
        
        StopTrackingDragGesture();
      }
    }
    break;
  case NS_MOUSE_BUTTON_UP:
    {
      SetContentState(nsnull, NS_EVENT_STATE_ACTIVE);
      if (!mCurrentTarget) {
        nsIFrame* targ;
        GetEventTarget(&targ);
      }
      if (mCurrentTarget) {
        ret =
          CheckForAndDispatchClick(presContext, (nsMouseEvent*)aEvent, aStatus);
      }

      nsIPresShell *shell = presContext->GetPresShell();
      if (shell) {
        nsCOMPtr<nsFrameSelection> frameSelection = shell->FrameSelection();
        frameSelection->SetMouseDownState(PR_FALSE);
      }
    }
    break;
  case NS_MOUSE_SCROLL:
  case NS_MOUSE_PIXEL_SCROLL:
    {
      nsMouseScrollEvent *msEvent = static_cast<nsMouseScrollEvent*>(aEvent);

      if (aEvent->message == NS_MOUSE_SCROLL) {
        
        
        if (msEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal) {
          mLastLineScrollConsumedX = (nsEventStatus_eConsumeNoDefault == *aStatus);
        } else if (msEvent->scrollFlags & nsMouseScrollEvent::kIsVertical) {
          mLastLineScrollConsumedY = (nsEventStatus_eConsumeNoDefault == *aStatus);
        }
        if (!(msEvent->scrollFlags & nsMouseScrollEvent::kHasPixels)) {
          
          
          nsWeakFrame weakFrame(aTargetFrame);
          SendPixelScrollEvent(aTargetFrame, msEvent, presContext, aStatus);
          NS_ENSURE_STATE(weakFrame.IsAlive());
        }
      }

      if (*aStatus != nsEventStatus_eConsumeNoDefault) {
        
        NS_NAMED_LITERAL_CSTRING(actionslot,      ".action");
        NS_NAMED_LITERAL_CSTRING(sysnumlinesslot, ".sysnumlines");

        nsCAutoString baseKey;
        GetBasePrefKeyForMouseWheel(msEvent, baseKey);

        
        nsCAutoString actionKey(baseKey);
        actionKey.Append(actionslot);

        nsCAutoString sysNumLinesKey(baseKey);
        sysNumLinesKey.Append(sysnumlinesslot);

        PRInt32 action = nsContentUtils::GetIntPref(actionKey.get());
        PRBool useSysNumLines =
          nsContentUtils::GetBoolPref(sysNumLinesKey.get());

        if (useSysNumLines) {
          if (msEvent->scrollFlags & nsMouseScrollEvent::kIsFullPage)
            action = MOUSE_SCROLL_PAGE;
        }

        if (aEvent->message == NS_MOUSE_PIXEL_SCROLL) {
          if (action == MOUSE_SCROLL_N_LINES) {
             action = MOUSE_SCROLL_PIXELS;
          } else {
            
            action = -1;
          }
        } else if (msEvent->scrollFlags & nsMouseScrollEvent::kHasPixels) {
          if (action == MOUSE_SCROLL_N_LINES) {
            
            action = -1;
          }
        }

        switch (action) {
        case MOUSE_SCROLL_N_LINES:
          DoScrollText(presContext, aTargetFrame, msEvent, eScrollByLine);
          break;

        case MOUSE_SCROLL_PAGE:
          DoScrollText(presContext, aTargetFrame, msEvent, eScrollByPage);
          break;

        case MOUSE_SCROLL_PIXELS:
          DoScrollText(presContext, aTargetFrame, msEvent, eScrollByPixel);
          break;

        case MOUSE_SCROLL_HISTORY:
          DoScrollHistory(msEvent->delta);
          break;

        case MOUSE_SCROLL_ZOOM:
          DoScrollZoom(aTargetFrame, msEvent->delta);
          break;

        default:  
          break;
        }
        *aStatus = nsEventStatus_eConsumeNoDefault;
      }
    }
    break;

  case NS_GESTURENOTIFY_EVENT_START:
    {
      if (nsEventStatus_eConsumeNoDefault != *aStatus)
        DecideGestureEvent(static_cast<nsGestureNotifyEvent*>(aEvent), mCurrentTarget);
    }
    break;

  case NS_DRAGDROP_ENTER:
  case NS_DRAGDROP_OVER:
    {
      NS_ASSERTION(aEvent->eventStructType == NS_DRAG_EVENT, "Expected a drag event");

      nsCOMPtr<nsIDragSession> dragSession = nsContentUtils::GetDragSession();
      if (!dragSession)
        break;

      
      
      nsCOMPtr<nsIDOMNSDataTransfer> dataTransfer;
      nsCOMPtr<nsIDOMDataTransfer> initialDataTransfer;
      dragSession->GetDataTransfer(getter_AddRefs(initialDataTransfer));

      nsCOMPtr<nsIDOMNSDataTransfer> initialDataTransferNS = 
        do_QueryInterface(initialDataTransfer);

      
      
      
      
      
      
      
      
      
      PRUint32 dropEffect = nsIDragService::DRAGDROP_ACTION_NONE;
      if (nsEventStatus_eConsumeNoDefault == *aStatus) {
        
        nsDragEvent *dragEvent = (nsDragEvent*)aEvent;
        if (dragEvent->dataTransfer) {
          
          dataTransfer = do_QueryInterface(dragEvent->dataTransfer);
          dataTransfer->GetDropEffectInt(&dropEffect);
        }
        else {
          
          
          
          
          
          
          
          dataTransfer = initialDataTransferNS;

          PRUint32 action;
          dragSession->GetDragAction(&action);

          
          
          dropEffect = nsContentUtils::FilterDropEffect(action,
                         nsIDragService::DRAGDROP_ACTION_UNINITIALIZED);
        }

        
        
        
        PRUint32 effectAllowed = nsIDragService::DRAGDROP_ACTION_UNINITIALIZED;
        if (dataTransfer)
          dataTransfer->GetEffectAllowedInt(&effectAllowed);

        
        
        
        
        
        
        PRUint32 action = nsIDragService::DRAGDROP_ACTION_NONE;
        if (effectAllowed == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED ||
            dropEffect & effectAllowed)
          action = dropEffect;

        if (action == nsIDragService::DRAGDROP_ACTION_NONE)
          dropEffect = nsIDragService::DRAGDROP_ACTION_NONE;

        
        dragSession->SetDragAction(action);
        dragSession->SetCanDrop(action != nsIDragService::DRAGDROP_ACTION_NONE);
      }

      
      
      if (initialDataTransferNS)
        initialDataTransferNS->SetDropEffectInt(dropEffect);
    }
    break;

  case NS_DRAGDROP_DROP:
    {
      
      if (mCurrentTarget && nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> targetContent;
        mCurrentTarget->GetContentForEvent(presContext, aEvent,
                                           getter_AddRefs(targetContent));

        nsCOMPtr<nsIWidget> widget = mCurrentTarget->GetWindow();
        nsDragEvent event(NS_IS_TRUSTED_EVENT(aEvent), NS_DRAGDROP_DRAGDROP, widget);

        nsMouseEvent* mouseEvent = static_cast<nsMouseEvent*>(aEvent);
        event.refPoint = mouseEvent->refPoint;
        event.isShift = mouseEvent->isShift;
        event.isControl = mouseEvent->isControl;
        event.isAlt = mouseEvent->isAlt;
        event.isMeta = mouseEvent->isMeta;

        nsEventStatus status = nsEventStatus_eIgnore;
        nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
        if (presShell) {
          presShell->HandleEventWithTarget(&event, mCurrentTarget,
                                           targetContent, &status);
        }
      }
      break;
    }
  case NS_DRAGDROP_EXIT:
     
     
    GenerateDragDropEnterExit(presContext, (nsGUIEvent*)aEvent);
    break;

  case NS_KEY_UP:
    break;

  case NS_KEY_PRESS:
    if (nsEventStatus_eConsumeNoDefault != *aStatus) {
      nsKeyEvent* keyEvent = (nsKeyEvent*)aEvent;
      
      if (!keyEvent->isAlt) {
        switch(keyEvent->keyCode) {
          case NS_VK_TAB:
          case NS_VK_F6:
            EnsureDocument(mPresContext);
            nsIFocusManager* fm = nsFocusManager::GetFocusManager();
            if (fm && mDocument) {
              
              PRBool isDocMove = ((nsInputEvent*)aEvent)->isControl ||
                                 (keyEvent->keyCode == NS_VK_F6);
              PRUint32 dir = ((nsInputEvent*)aEvent)->isShift ?
                             (isDocMove ? nsIFocusManager::MOVEFOCUS_BACKWARDDOC :
                                          nsIFocusManager::MOVEFOCUS_BACKWARD) :
                             (isDocMove ? nsIFocusManager::MOVEFOCUS_FORWARDDOC :
                                          nsIFocusManager::MOVEFOCUS_FORWARD);
              nsCOMPtr<nsIDOMElement> result;
              fm->MoveFocus(mDocument->GetWindow(), nsnull, dir,
                            nsIFocusManager::FLAG_BYKEY,
                            getter_AddRefs(result));
            }
            *aStatus = nsEventStatus_eConsumeNoDefault;
            break;


#if NON_KEYBINDING
          case NS_VK_PAGE_DOWN:
          case NS_VK_PAGE_UP:
            if (!focusedContent) {
              nsIScrollableView* sv = nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eVertical);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByPages(0, (keyEvent->keyCode != NS_VK_PAGE_UP) ? 1 : -1,
                                  NS_VMREFRESH_SMOOTHSCROLL);
              }
            }
            break;
          case NS_VK_HOME:
          case NS_VK_END:
            if (!focusedContent) {
              nsIScrollableView* sv = nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eVertical);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByWhole((keyEvent->keyCode != NS_VK_HOME) ? PR_FALSE : PR_TRUE);
              }
            }
            break;
          case NS_VK_DOWN:
          case NS_VK_UP:
            if (!focusedContent) {
              nsIScrollableView* sv = nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eVertical);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByLines(0, (keyEvent->keyCode == NS_VK_DOWN) ? 1 : -1,
                                  NS_VMREFRESH_SMOOTHSCROLL);

                
                
                nsIViewManager* vm = aView->GetViewManager();
                if (vm) {
                  
                  
                  vm->ForceUpdate();
                }
              }
            }
            break;
          case NS_VK_LEFT:
          case NS_VK_RIGHT:
            if (!focusedContent) {
              nsIScrollableView* sv = nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eHorizontal);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByLines((keyEvent->keyCode == NS_VK_RIGHT) ? 1 : -1, 0,
                                  NS_VMREFRESH_SMOOTHSCROLL);

                
                
                nsIViewManager* vm = aView->GetViewManager();
                if (vm) {
                  
                  
                  vm->ForceUpdate();
                }
              }
            }
            break;
        case 0: 
          {
          
            nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
            if (keyEvent->charCode == 0x20) {
              if (!focusedContent) {
                nsIScrollableView* sv = nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eVertical);
                if (sv) {
                  sv->ScrollByPages(0, 1, NS_VMREFRESH_SMOOTHSCROLL);
                }
              }
            }
          }
          break;
#endif 
        }
      }
    }
    break;

  case NS_MOUSE_ENTER:
    if (mCurrentTarget) {
      nsCOMPtr<nsIContent> targetContent;
      mCurrentTarget->GetContentForEvent(presContext, aEvent,
                                         getter_AddRefs(targetContent));
      SetContentState(targetContent, NS_EVENT_STATE_HOVER);
    }
    break;
  }

  
  mCurrentTarget = nsnull;

  return ret;
}

NS_IMETHODIMP
nsEventStateManager::NotifyDestroyPresContext(nsPresContext* aPresContext)
{
  nsIMEStateManager::OnDestroyPresContext(aPresContext);
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::SetPresContext(nsPresContext* aPresContext)
{
  mPresContext = aPresContext;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::ClearFrameRefs(nsIFrame* aFrame)
{
  if (aFrame && aFrame == mCurrentTarget) {
    mCurrentTargetContent = aFrame->GetContent();
  }

  return NS_OK;
}

void
nsEventStateManager::UpdateCursor(nsPresContext* aPresContext,
                                  nsEvent* aEvent, nsIFrame* aTargetFrame,
                                  nsEventStatus* aStatus)
{
  PRInt32 cursor = NS_STYLE_CURSOR_DEFAULT;
  imgIContainer* container = nsnull;
  PRBool haveHotspot = PR_FALSE;
  float hotspotX = 0.0f, hotspotY = 0.0f;

  
  if (mLockCursor) {
    cursor = mLockCursor;
  }
  
  else if (aTargetFrame) {
      nsIFrame::Cursor framecursor;
      nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                                aTargetFrame);
      if (NS_FAILED(aTargetFrame->GetCursor(pt, framecursor)))
        return;  
      cursor = framecursor.mCursor;
      container = framecursor.mContainer;
      haveHotspot = framecursor.mHaveHotspot;
      hotspotX = framecursor.mHotspotX;
      hotspotY = framecursor.mHotspotY;
  }

  if (nsContentUtils::GetBoolPref("ui.use_activity_cursor", PR_FALSE)) {
    
    nsCOMPtr<nsISupports> pcContainer = aPresContext->GetContainer();
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(pcContainer));
    if (!docShell) return;
    PRUint32 busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
    docShell->GetBusyFlags(&busyFlags);

    
    
    if (busyFlags & nsIDocShell::BUSY_FLAGS_BUSY &&
          (cursor == NS_STYLE_CURSOR_AUTO || cursor == NS_STYLE_CURSOR_DEFAULT))
    {
      cursor = NS_STYLE_CURSOR_SPINNING;
      container = nsnull;
    }
  }

  if (aTargetFrame) {
    SetCursor(cursor, container, haveHotspot, hotspotX, hotspotY,
              aTargetFrame->GetWindow(), PR_FALSE);
  }

  if (mLockCursor || NS_STYLE_CURSOR_AUTO != cursor) {
    *aStatus = nsEventStatus_eConsumeDoDefault;
  }
}

NS_IMETHODIMP
nsEventStateManager::SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                               PRBool aHaveHotspot,
                               float aHotspotX, float aHotspotY,
                               nsIWidget* aWidget, PRBool aLockCursor)
{
  nsCursor c;

  NS_ENSURE_TRUE(aWidget, NS_ERROR_FAILURE);
  if (aLockCursor) {
    if (NS_STYLE_CURSOR_AUTO != aCursor) {
      mLockCursor = aCursor;
    }
    else {
      
      mLockCursor = 0;
    }
  }
  switch (aCursor) {
  default:
  case NS_STYLE_CURSOR_AUTO:
  case NS_STYLE_CURSOR_DEFAULT:
    c = eCursor_standard;
    break;
  case NS_STYLE_CURSOR_POINTER:
    c = eCursor_hyperlink;
    break;
  case NS_STYLE_CURSOR_CROSSHAIR:
    c = eCursor_crosshair;
    break;
  case NS_STYLE_CURSOR_MOVE:
    c = eCursor_move;
    break;
  case NS_STYLE_CURSOR_TEXT:
    c = eCursor_select;
    break;
  case NS_STYLE_CURSOR_WAIT:
    c = eCursor_wait;
    break;
  case NS_STYLE_CURSOR_HELP:
    c = eCursor_help;
    break;
  case NS_STYLE_CURSOR_N_RESIZE:
    c = eCursor_n_resize;
    break;
  case NS_STYLE_CURSOR_S_RESIZE:
    c = eCursor_s_resize;
    break;
  case NS_STYLE_CURSOR_W_RESIZE:
    c = eCursor_w_resize;
    break;
  case NS_STYLE_CURSOR_E_RESIZE:
    c = eCursor_e_resize;
    break;
  case NS_STYLE_CURSOR_NW_RESIZE:
    c = eCursor_nw_resize;
    break;
  case NS_STYLE_CURSOR_SE_RESIZE:
    c = eCursor_se_resize;
    break;
  case NS_STYLE_CURSOR_NE_RESIZE:
    c = eCursor_ne_resize;
    break;
  case NS_STYLE_CURSOR_SW_RESIZE:
    c = eCursor_sw_resize;
    break;
  case NS_STYLE_CURSOR_COPY: 
    c = eCursor_copy;
    break;
  case NS_STYLE_CURSOR_ALIAS:
    c = eCursor_alias;
    break;
  case NS_STYLE_CURSOR_CONTEXT_MENU:
    c = eCursor_context_menu;
    break;
  case NS_STYLE_CURSOR_CELL:
    c = eCursor_cell;
    break;
  case NS_STYLE_CURSOR_GRAB:
    c = eCursor_grab;
    break;
  case NS_STYLE_CURSOR_GRABBING:
    c = eCursor_grabbing;
    break;
  case NS_STYLE_CURSOR_SPINNING:
    c = eCursor_spinning;
    break;
  case NS_STYLE_CURSOR_MOZ_ZOOM_IN:
    c = eCursor_zoom_in;
    break;
  case NS_STYLE_CURSOR_MOZ_ZOOM_OUT:
    c = eCursor_zoom_out;
    break;
  case NS_STYLE_CURSOR_NOT_ALLOWED:
    c = eCursor_not_allowed;
    break;
  case NS_STYLE_CURSOR_COL_RESIZE:
    c = eCursor_col_resize;
    break;
  case NS_STYLE_CURSOR_ROW_RESIZE:
    c = eCursor_row_resize;
    break;
  case NS_STYLE_CURSOR_NO_DROP:
    c = eCursor_no_drop;
    break;
  case NS_STYLE_CURSOR_VERTICAL_TEXT:
    c = eCursor_vertical_text;
    break;
  case NS_STYLE_CURSOR_ALL_SCROLL:
    c = eCursor_all_scroll;
    break;
  case NS_STYLE_CURSOR_NESW_RESIZE:
    c = eCursor_nesw_resize;
    break;
  case NS_STYLE_CURSOR_NWSE_RESIZE:
    c = eCursor_nwse_resize;
    break;
  case NS_STYLE_CURSOR_NS_RESIZE:
    c = eCursor_ns_resize;
    break;
  case NS_STYLE_CURSOR_EW_RESIZE:
    c = eCursor_ew_resize;
    break;
  case NS_STYLE_CURSOR_NONE:
    c = eCursor_none;
    break;
  }

  
  nsresult rv = NS_ERROR_FAILURE;
  if (aContainer) {
    PRUint32 hotspotX, hotspotY;

    
    
    
    if (aHaveHotspot) {
      PRInt32 imgWidth, imgHeight;
      aContainer->GetWidth(&imgWidth);
      aContainer->GetHeight(&imgHeight);

      
      hotspotX = aHotspotX > 0.0f
                   ? PRUint32(aHotspotX + 0.5f) : PRUint32(0);
      if (hotspotX >= PRUint32(imgWidth))
        hotspotX = imgWidth - 1;
      hotspotY = aHotspotY > 0.0f
                   ? PRUint32(aHotspotY + 0.5f) : PRUint32(0);
      if (hotspotY >= PRUint32(imgHeight))
        hotspotY = imgHeight - 1;
    } else {
      hotspotX = 0;
      hotspotY = 0;
      nsCOMPtr<nsIProperties> props(do_QueryInterface(aContainer));
      if (props) {
        nsCOMPtr<nsISupportsPRUint32> hotspotXWrap, hotspotYWrap;

        props->Get("hotspotX", NS_GET_IID(nsISupportsPRUint32), getter_AddRefs(hotspotXWrap));
        props->Get("hotspotY", NS_GET_IID(nsISupportsPRUint32), getter_AddRefs(hotspotYWrap));

        if (hotspotXWrap)
          hotspotXWrap->GetData(&hotspotX);
        if (hotspotYWrap)
          hotspotYWrap->GetData(&hotspotY);
      }
    }

    rv = aWidget->SetCursor(aContainer, hotspotX, hotspotY);
  }

  if (NS_FAILED(rv))
    aWidget->SetCursor(c);

  return NS_OK;
}

class NS_STACK_CLASS nsESMEventCB : public nsDispatchingCallback
{
public:
  nsESMEventCB(nsIContent* aTarget) : mTarget(aTarget) {}

  virtual void HandleEvent(nsEventChainPostVisitor& aVisitor)
  {
    if (aVisitor.mPresContext) {
      nsIPresShell* shell = aVisitor.mPresContext->GetPresShell();
      if (shell) {
        nsIFrame* frame = shell->GetPrimaryFrameFor(mTarget);
        if (frame) {
          frame->HandleEvent(aVisitor.mPresContext,
                             (nsGUIEvent*) aVisitor.mEvent,
                             &aVisitor.mEventStatus);
        }
      }
    }
  }

  nsCOMPtr<nsIContent> mTarget;
};

nsIFrame*
nsEventStateManager::DispatchMouseEvent(nsGUIEvent* aEvent, PRUint32 aMessage,
                                        nsIContent* aTargetContent,
                                        nsIContent* aRelatedContent)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(NS_IS_TRUSTED_EVENT(aEvent), aMessage, aEvent->widget,
                     nsMouseEvent::eReal);
  event.refPoint = aEvent->refPoint;
  event.isShift = ((nsMouseEvent*)aEvent)->isShift;
  event.isControl = ((nsMouseEvent*)aEvent)->isControl;
  event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
  event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;
  event.nativeMsg = ((nsMouseEvent*)aEvent)->nativeMsg;
  event.relatedTarget = aRelatedContent;

  mCurrentTargetContent = aTargetContent;

  nsIFrame* targetFrame = nsnull;
  if (aTargetContent) {
    nsESMEventCB callback(aTargetContent);
    nsEventDispatcher::Dispatch(aTargetContent, mPresContext, &event, nsnull,
                                &status, &callback);

    nsIPresShell *shell = mPresContext ? mPresContext->GetPresShell() : nsnull;
    if (shell) {
      
      
      
      targetFrame = shell->GetPrimaryFrameFor(aTargetContent);
    }
  }

  mCurrentTargetContent = nsnull;

  return targetFrame;
}

void
nsEventStateManager::NotifyMouseOut(nsGUIEvent* aEvent, nsIContent* aMovingInto)
{
  if (!mLastMouseOverElement)
    return;
  
  if (mLastMouseOverElement == mFirstMouseOutEventElement)
    return;

  if (mLastMouseOverFrame) {
    
    
    nsIFrameFrame* subdocFrame = do_QueryFrame(mLastMouseOverFrame.GetFrame());
    if (subdocFrame) {
      nsCOMPtr<nsIDocShell> docshell;
      subdocFrame->GetDocShell(getter_AddRefs(docshell));
      if (docshell) {
        nsCOMPtr<nsPresContext> presContext;
        docshell->GetPresContext(getter_AddRefs(presContext));
        
        if (presContext) {
          nsEventStateManager* kidESM =
            static_cast<nsEventStateManager*>(presContext->EventStateManager());
          
          kidESM->NotifyMouseOut(aEvent, nsnull);
        }
      }
    }
  }
  
  
  if (!mLastMouseOverElement)
    return;

  
  
  mFirstMouseOutEventElement = mLastMouseOverElement;

  
  
  
  
  if (!aMovingInto) {
    
    SetContentState(nsnull, NS_EVENT_STATE_HOVER);
  }
  
  
  DispatchMouseEvent(aEvent, NS_MOUSE_EXIT_SYNTH,
                     mLastMouseOverElement, aMovingInto);
  
  mLastMouseOverFrame = nsnull;
  mLastMouseOverElement = nsnull;
  
  
  mFirstMouseOutEventElement = nsnull;
}

void
nsEventStateManager::NotifyMouseOver(nsGUIEvent* aEvent, nsIContent* aContent)
{
  NS_ASSERTION(aContent, "Mouse must be over something");

  if (mLastMouseOverElement == aContent)
    return;

  
  if (aContent == mFirstMouseOverEventElement)
    return;

  
  
  
  EnsureDocument(mPresContext);
  nsIDocument *parentDoc = mDocument->GetParentDocument();
  if (parentDoc) {
    nsIContent *docContent = parentDoc->FindContentForSubDocument(mDocument);
    if (docContent) {
      nsIPresShell *parentShell = parentDoc->GetPrimaryShell();
      if (parentShell) {
        nsEventStateManager* parentESM =
          static_cast<nsEventStateManager*>
                     (parentShell->GetPresContext()->EventStateManager());
        parentESM->NotifyMouseOver(aEvent, docContent);
      }
    }
  }
  
  
  if (mLastMouseOverElement == aContent)
    return;

  
  
  nsCOMPtr<nsIContent> lastMouseOverElement = mLastMouseOverElement;

  NotifyMouseOut(aEvent, aContent);

  
  
  mFirstMouseOverEventElement = aContent;
  
  SetContentState(aContent, NS_EVENT_STATE_HOVER);
  
  
  mLastMouseOverFrame = DispatchMouseEvent(aEvent, NS_MOUSE_ENTER_SYNTH,
                                           aContent, lastMouseOverElement);
  mLastMouseOverElement = aContent;
  
  
  mFirstMouseOverEventElement = nsnull;
}

void
nsEventStateManager::GenerateMouseEnterExit(nsGUIEvent* aEvent)
{
  EnsureDocument(mPresContext);
  if (!mDocument)
    return;

  
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aEvent->message) {
  case NS_MOUSE_MOVE:
    {
      
      nsCOMPtr<nsIContent> targetElement;
      GetEventTargetContent(aEvent, getter_AddRefs(targetElement));
      if (!targetElement) {
        
        
        
        targetElement = mDocument->GetRootContent();
      }
      NS_ASSERTION(targetElement, "Mouse move must have some target content");
      if (targetElement) {
        NotifyMouseOver(aEvent, targetElement);
      }
    }
    break;
  case NS_MOUSE_EXIT:
    {
      
      

      if (mLastMouseOverFrame &&
          nsContentUtils::GetTopLevelWidget(aEvent->widget) !=
          nsContentUtils::GetTopLevelWidget(mLastMouseOverFrame->GetWindow())) {
        
        
        break;
      }

      NotifyMouseOut(aEvent, nsnull);
    }
    break;
  }

  
  mCurrentTargetContent = targetBeforeEvent;
}

void
nsEventStateManager::GenerateDragDropEnterExit(nsPresContext* aPresContext,
                                               nsGUIEvent* aEvent)
{
  
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aEvent->message) {
  case NS_DRAGDROP_OVER:
    {
      if (mLastDragOverFrame != mCurrentTarget) {
        
        nsCOMPtr<nsIContent> lastContent;
        nsCOMPtr<nsIContent> targetContent;
        mCurrentTarget->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(targetContent));

        if ( mLastDragOverFrame ) {
          
          mLastDragOverFrame->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(lastContent));

          FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_LEAVE_SYNTH,
                              targetContent, lastContent, mLastDragOverFrame);
          FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_EXIT_SYNTH,
                              targetContent, lastContent, mLastDragOverFrame);
        }

        FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_ENTER,
                            lastContent, targetContent, mCurrentTarget);

        mLastDragOverFrame = mCurrentTarget;
      }
    }
    break;

  case NS_DRAGDROP_DROP:
  case NS_DRAGDROP_EXIT:
    {
      
      if ( mLastDragOverFrame ) {
        nsCOMPtr<nsIContent> lastContent;
        mLastDragOverFrame->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(lastContent));

        FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_LEAVE_SYNTH,
                            nsnull, lastContent, mLastDragOverFrame);
        FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_EXIT_SYNTH,
                            nsnull, lastContent, mLastDragOverFrame);

        mLastDragOverFrame = nsnull;
      }
    }
    break;
  }

  
  mCurrentTargetContent = targetBeforeEvent;

  
  FlushPendingEvents(aPresContext);
}

void
nsEventStateManager::FireDragEnterOrExit(nsPresContext* aPresContext,
                                         nsGUIEvent* aEvent,
                                         PRUint32 aMsg,
                                         nsIContent* aRelatedTarget,
                                         nsIContent* aTargetContent,
                                         nsWeakFrame& aTargetFrame)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsDragEvent event(NS_IS_TRUSTED_EVENT(aEvent), aMsg, aEvent->widget);
  event.refPoint = aEvent->refPoint;
  event.isShift = ((nsMouseEvent*)aEvent)->isShift;
  event.isControl = ((nsMouseEvent*)aEvent)->isControl;
  event.isAlt = ((nsMouseEvent*)aEvent)->isAlt;
  event.isMeta = ((nsMouseEvent*)aEvent)->isMeta;
  event.relatedTarget = aRelatedTarget;

  mCurrentTargetContent = aTargetContent;

  if (aTargetContent != aRelatedTarget) {
    
    if (aTargetContent)
      nsEventDispatcher::Dispatch(aTargetContent, aPresContext, &event,
                                  nsnull, &status);

    
    if (status == nsEventStatus_eConsumeNoDefault || aMsg == NS_DRAGDROP_EXIT)
      SetContentState((aMsg == NS_DRAGDROP_ENTER) ? aTargetContent : nsnull,
                      NS_EVENT_STATE_DRAGOVER);
  }

  
  if (aTargetFrame)
    aTargetFrame->HandleEvent(aPresContext, &event, &status);
}

nsresult
nsEventStateManager::SetClickCount(nsPresContext* aPresContext,
                                   nsMouseEvent *aEvent,
                                   nsEventStatus* aStatus)
{
  nsCOMPtr<nsIContent> mouseContent;
  mCurrentTarget->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(mouseContent));

  switch (aEvent->button) {
  case nsMouseEvent::eLeftButton:
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      mLastLeftMouseDownContent = mouseContent;
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      if (mLastLeftMouseDownContent == mouseContent) {
        aEvent->clickCount = mLClickCount;
        mLClickCount = 0;
      } else {
        aEvent->clickCount = 0;
      }
      mLastLeftMouseDownContent = nsnull;
    }
    break;

  case nsMouseEvent::eMiddleButton:
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      mLastMiddleMouseDownContent = mouseContent;
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      if (mLastMiddleMouseDownContent == mouseContent) {
        aEvent->clickCount = mMClickCount;
        mMClickCount = 0;
      } else {
        aEvent->clickCount = 0;
      }
      
    }
    break;

  case nsMouseEvent::eRightButton:
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      mLastRightMouseDownContent = mouseContent;
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      if (mLastRightMouseDownContent == mouseContent) {
        aEvent->clickCount = mRClickCount;
        mRClickCount = 0;
      } else {
        aEvent->clickCount = 0;
      }
      
    }
    break;
  }

  return NS_OK;
}

nsresult
nsEventStateManager::CheckForAndDispatchClick(nsPresContext* aPresContext,
                                              nsMouseEvent *aEvent,
                                              nsEventStatus* aStatus)
{
  nsresult ret = NS_OK;
  PRInt32 flags = NS_EVENT_FLAG_NONE;

  
  
  if (0 != aEvent->clickCount) {
    
    
    if (aEvent->widget) {
      PRBool enabled;
      aEvent->widget->IsEnabled(&enabled);
      if (!enabled) {
        return ret;
      }
    }
    
    if (aEvent->button == nsMouseEvent::eMiddleButton ||
        aEvent->button == nsMouseEvent::eRightButton) {
      flags |=
        sLeftClickOnly ? NS_EVENT_FLAG_NO_CONTENT_DISPATCH : NS_EVENT_FLAG_NONE;
    }

    nsMouseEvent event(NS_IS_TRUSTED_EVENT(aEvent), NS_MOUSE_CLICK, aEvent->widget,
                       nsMouseEvent::eReal);
    event.refPoint = aEvent->refPoint;
    event.clickCount = aEvent->clickCount;
    event.isShift = aEvent->isShift;
    event.isControl = aEvent->isControl;
    event.isAlt = aEvent->isAlt;
    event.isMeta = aEvent->isMeta;
    event.time = aEvent->time;
    event.flags |= flags;
    event.button = aEvent->button;

    nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
    if (presShell) {
      nsCOMPtr<nsIContent> mouseContent;
      GetEventTargetContent(aEvent, getter_AddRefs(mouseContent));

      ret = presShell->HandleEventWithTarget(&event, mCurrentTarget,
                                             mouseContent, aStatus);
      if (NS_SUCCEEDED(ret) && aEvent->clickCount == 2) {
        
        nsMouseEvent event2(NS_IS_TRUSTED_EVENT(aEvent), NS_MOUSE_DOUBLECLICK,
                            aEvent->widget, nsMouseEvent::eReal);
        event2.refPoint = aEvent->refPoint;
        event2.clickCount = aEvent->clickCount;
        event2.isShift = aEvent->isShift;
        event2.isControl = aEvent->isControl;
        event2.isAlt = aEvent->isAlt;
        event2.isMeta = aEvent->isMeta;
        event2.flags |= flags;
        event2.button = aEvent->button;

        ret = presShell->HandleEventWithTarget(&event2, mCurrentTarget,
                                               mouseContent, aStatus);
      }
    }
  }
  return ret;
}

NS_IMETHODIMP
nsEventStateManager::GetEventTarget(nsIFrame **aFrame)
{
  nsIPresShell *shell;
  if (mCurrentTarget ||
      !mPresContext ||
      !(shell = mPresContext->GetPresShell())) {
    *aFrame = mCurrentTarget;
    return NS_OK;
  }

  if (mCurrentTargetContent) {
    mCurrentTarget = shell->GetPrimaryFrameFor(mCurrentTargetContent);
    if (mCurrentTarget) {
      *aFrame = mCurrentTarget;
      return NS_OK;
    }
  }

  nsIFrame* frame = nsnull;
  shell->GetEventTargetFrame(&frame);
  *aFrame = mCurrentTarget = frame;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetEventTargetContent(nsEvent* aEvent,
                                           nsIContent** aContent)
{
  if (aEvent &&
      (aEvent->message == NS_FOCUS_CONTENT ||
       aEvent->message == NS_BLUR_CONTENT)) {
    NS_IF_ADDREF(*aContent = GetFocusedContent());
    return NS_OK;
  }

  if (mCurrentTargetContent) {
    *aContent = mCurrentTargetContent;
    NS_IF_ADDREF(*aContent);
    return NS_OK;
  }

  *aContent = nsnull;

  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    presShell->GetEventTargetContent(aEvent, aContent);
  }

  
  
  if (!*aContent && mCurrentTarget) {
    mCurrentTarget->GetContentForEvent(mPresContext, aEvent, aContent);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetContentState(nsIContent *aContent, PRInt32& aState)
{
  aState = aContent->IntrinsicState();

  
  
  for (nsIContent* activeContent = mActiveContent; activeContent;
       activeContent = activeContent->GetParent()) {
    if (aContent == activeContent) {
      aState |= NS_EVENT_STATE_ACTIVE;
      break;
    }
  }
  
  
  for (nsIContent* hoverContent = mHoverContent; hoverContent;
       hoverContent = hoverContent->GetParent()) {
    if (aContent == hoverContent) {
      aState |= NS_EVENT_STATE_HOVER;
      break;
    }
  }

  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm && aContent == fm->GetFocusedContent()) {
    aState |= NS_EVENT_STATE_FOCUS;
  }
  if (aContent == mDragOverContent) {
    aState |= NS_EVENT_STATE_DRAGOVER;
  }
  if (aContent == mURLTargetContent) {
    aState |= NS_EVENT_STATE_URLTARGET;
  }
  return NS_OK;
}

static nsIContent* FindCommonAncestor(nsIContent *aNode1, nsIContent *aNode2)
{
  
  if (aNode1 && aNode2) {
    
    
    PRInt32 offset = 0;
    nsIContent *anc1 = aNode1;
    for (;;) {
      ++offset;
      nsIContent* parent = anc1->GetParent();
      if (!parent)
        break;
      anc1 = parent;
    }
    nsIContent *anc2 = aNode2;
    for (;;) {
      --offset;
      nsIContent* parent = anc2->GetParent();
      if (!parent)
        break;
      anc2 = parent;
    }
    if (anc1 == anc2) {
      anc1 = aNode1;
      anc2 = aNode2;
      while (offset > 0) {
        anc1 = anc1->GetParent();
        --offset;
      }
      while (offset < 0) {
        anc2 = anc2->GetParent();
        ++offset;
      }
      while (anc1 != anc2) {
        anc1 = anc1->GetParent();
        anc2 = anc2->GetParent();
      }
      return anc1;
    }
  }
  return nsnull;
}

PRBool
nsEventStateManager::SetContentState(nsIContent *aContent, PRInt32 aState)
{
  const PRInt32 maxNotify = 5;
  
  
  nsIContent  *notifyContent[maxNotify];
  memset(notifyContent, 0, sizeof(notifyContent));

  
  
  
  if (mCurrentTarget && (aState == NS_EVENT_STATE_ACTIVE || aState == NS_EVENT_STATE_HOVER))
  {
    const nsStyleUserInterface* ui = mCurrentTarget->GetStyleUserInterface();
    if (ui->mUserInput == NS_STYLE_USER_INPUT_NONE)
      return PR_FALSE;
  }

  if ((aState & NS_EVENT_STATE_DRAGOVER) && (aContent != mDragOverContent)) {
    notifyContent[3] = mDragOverContent; 
    NS_IF_ADDREF(notifyContent[3]);
    mDragOverContent = aContent;
  }

  if ((aState & NS_EVENT_STATE_URLTARGET) && (aContent != mURLTargetContent)) {
    notifyContent[4] = mURLTargetContent;
    NS_IF_ADDREF(notifyContent[4]);
    mURLTargetContent = aContent;
  }

  nsCOMPtr<nsIContent> commonActiveAncestor, oldActive, newActive;
  if ((aState & NS_EVENT_STATE_ACTIVE) && (aContent != mActiveContent)) {
    oldActive = mActiveContent;
    newActive = aContent;
    commonActiveAncestor = FindCommonAncestor(mActiveContent, aContent);
    mActiveContent = aContent;
  }

  nsCOMPtr<nsIContent> commonHoverAncestor, oldHover, newHover;
  if ((aState & NS_EVENT_STATE_HOVER) && (aContent != mHoverContent)) {
    oldHover = mHoverContent;

    if (!mPresContext || mPresContext->IsDynamic()) {
      newHover = aContent;
    } else {
      nsIFrame *frame = aContent ?
        mPresContext->PresShell()->GetPrimaryFrameFor(aContent) : nsnull;
      if (frame && nsLayoutUtils::IsViewportScrollbarFrame(frame)) {
        
        
        newHover = aContent;
      } else {
        
        newHover = nsnull;
      }
    }

    commonHoverAncestor = FindCommonAncestor(mHoverContent, aContent);
    mHoverContent = aContent;
  }

  if ((aState & NS_EVENT_STATE_FOCUS)) {
    notifyContent[2] = aContent;
    NS_IF_ADDREF(notifyContent[2]);
  }

  PRInt32 simpleStates = aState & ~(NS_EVENT_STATE_ACTIVE|NS_EVENT_STATE_HOVER);

  if (aContent && simpleStates != 0) {
    
    notifyContent[0] = aContent;
    NS_ADDREF(aContent);  
  }

  
  if ((notifyContent[4] == notifyContent[3]) || (notifyContent[4] == notifyContent[2]) || (notifyContent[4] == notifyContent[1])) {
    NS_IF_RELEASE(notifyContent[4]);
  }
  
  if ((notifyContent[3] == notifyContent[2]) || (notifyContent[3] == notifyContent[1])) {
    NS_IF_RELEASE(notifyContent[3]);
  }
  if (notifyContent[2] == notifyContent[1]) {
    NS_IF_RELEASE(notifyContent[2]);
  }

  
  
  for  (int i = 0; i < maxNotify; i++) {
    if (notifyContent[i] &&
        !notifyContent[i]->GetDocument()) {
      NS_RELEASE(notifyContent[i]);
    }
  }

  
  nsIContent** from = &(notifyContent[0]);
  nsIContent** to   = &(notifyContent[0]);
  nsIContent** end  = &(notifyContent[maxNotify]);

  while (from < end) {
    if (! *from) {
      while (++from < end) {
        if (*from) {
          *to++ = *from;
          *from = nsnull;
          break;
        }
      }
    }
    else {
      if (from == to) {
        to++;
        from++;
      }
      else {
        *to++ = *from;
        *from++ = nsnull;
      }
    }
  }

  if (notifyContent[0] || newHover || oldHover || newActive || oldActive) {
    
    nsCOMPtr<nsIDocument> doc1, doc2;  
    if (notifyContent[0]) {
      doc1 = notifyContent[0]->GetDocument();
      if (notifyContent[1]) {
        
        doc2 = notifyContent[1]->GetDocument();
        if (doc1 == doc2) {
          doc2 = nsnull;
        }
      }
    }
    else {
      EnsureDocument(mPresContext);
      doc1 = mDocument;
    }
    if (doc1) {
      doc1->BeginUpdate(UPDATE_CONTENT_STATE);

      
      while (newActive && newActive != commonActiveAncestor) {
        doc1->ContentStatesChanged(newActive, nsnull, NS_EVENT_STATE_ACTIVE);
        newActive = newActive->GetParent();
      }
      
      while (oldActive && oldActive != commonActiveAncestor) {
        doc1->ContentStatesChanged(oldActive, nsnull, NS_EVENT_STATE_ACTIVE);
        oldActive = oldActive->GetParent();
      }

      
      while (newHover && newHover != commonHoverAncestor) {
        doc1->ContentStatesChanged(newHover, nsnull, NS_EVENT_STATE_HOVER);
        newHover = newHover->GetParent();
      }
      
      while (oldHover && oldHover != commonHoverAncestor) {
        doc1->ContentStatesChanged(oldHover, nsnull, NS_EVENT_STATE_HOVER);
        oldHover = oldHover->GetParent();
      }

      if (notifyContent[0]) {
        doc1->ContentStatesChanged(notifyContent[0], notifyContent[1],
                                   simpleStates);
        if (notifyContent[2]) {
          
          
          
          
          
          doc1->ContentStatesChanged(notifyContent[2], notifyContent[3],
                                     simpleStates);
          if (notifyContent[4]) {
            
            doc1->ContentStatesChanged(notifyContent[4], nsnull,
                                       simpleStates);
          }
        }
      }
      doc1->EndUpdate(UPDATE_CONTENT_STATE);

      if (doc2) {
        doc2->BeginUpdate(UPDATE_CONTENT_STATE);
        doc2->ContentStatesChanged(notifyContent[1], notifyContent[2],
                                   simpleStates);
        if (notifyContent[3]) {
          doc1->ContentStatesChanged(notifyContent[3], notifyContent[4],
                                     simpleStates);
        }
        doc2->EndUpdate(UPDATE_CONTENT_STATE);
      }
    }

    from = &(notifyContent[0]);
    while (from < to) {  
      nsIContent* notify = *from++;
      NS_RELEASE(notify);
    }
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsEventStateManager::ContentRemoved(nsIDocument* aDocument, nsIContent* aContent)
{
  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm)
    fm->ContentRemoved(aDocument, aContent);

  if (mHoverContent &&
      nsContentUtils::ContentIsDescendantOf(mHoverContent, aContent)) {
    
    
    mHoverContent = aContent->GetParent();
  }

  if (mActiveContent &&
      nsContentUtils::ContentIsDescendantOf(mActiveContent, aContent)) {
    
    
    mActiveContent = aContent->GetParent();
  }

  if (mDragOverContent &&
      nsContentUtils::ContentIsDescendantOf(mDragOverContent, aContent)) {
    mDragOverContent = nsnull;
  }

  if (mLastMouseOverElement &&
      nsContentUtils::ContentIsDescendantOf(mLastMouseOverElement, aContent)) {
    
    mLastMouseOverElement = nsnull;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::EventStatusOK(nsGUIEvent* aEvent, PRBool *aOK)
{
  *aOK = PR_TRUE;
  if (aEvent->message == NS_MOUSE_BUTTON_DOWN &&
      static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton) {
    if (!mNormalLMouseEventInProcess) {
      *aOK = PR_FALSE;
    }
  }
  return NS_OK;
}




NS_IMETHODIMP
nsEventStateManager::RegisterAccessKey(nsIContent* aContent, PRUint32 aKey)
{
  if (aContent && mAccessKeys.IndexOf(aContent) == -1)
    mAccessKeys.AppendObject(aContent);

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey)
{
  if (aContent)
    mAccessKeys.RemoveObject(aContent);

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetRegisteredAccessKey(nsIContent* aContent,
                                            PRUint32* aKey)
{
  NS_ENSURE_ARG(aContent);
  NS_ENSURE_ARG_POINTER(aKey);
  *aKey = 0;

  if (mAccessKeys.IndexOf(aContent) == -1)
    return NS_OK;

  nsAutoString accessKey;
  aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);
  *aKey = accessKey.First();
  return NS_OK;
}

void
nsEventStateManager::EnsureDocument(nsPresContext* aPresContext)
{
  if (!mDocument)
    mDocument = aPresContext->Document();
}

void
nsEventStateManager::FlushPendingEvents(nsPresContext* aPresContext)
{
  NS_PRECONDITION(nsnull != aPresContext, "nsnull ptr");
  nsIPresShell *shell = aPresContext->GetPresShell();
  if (shell) {
    shell->FlushPendingNotifications(Flush_InterruptibleLayout);
  }
}

nsIContent*
nsEventStateManager::GetFocusedContent()
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm || !mDocument)
    return nsnull;

  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  return nsFocusManager::GetFocusedDescendant(mDocument->GetWindow(), PR_FALSE,
                                              getter_AddRefs(focusedWindow));
}




PRBool
nsEventStateManager::IsShellVisible(nsIDocShell* aShell)
{
  NS_ASSERTION(aShell, "docshell is null");

  nsCOMPtr<nsIBaseWindow> basewin = do_QueryInterface(aShell);
  if (!basewin)
    return PR_TRUE;

  PRBool isVisible = PR_TRUE;
  basewin->GetVisibility(&isVisible);

  
  

  return isVisible;
}

nsresult
nsEventStateManager::DoContentCommandEvent(nsContentCommandEvent* aEvent)
{
  EnsureDocument(mPresContext);
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mDocument->GetWindow()));
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
  nsIFocusController* fc = window->GetRootFocusController();
  NS_ENSURE_TRUE(fc, NS_ERROR_FAILURE);
  const char* cmd;
  switch (aEvent->message) {
    case NS_CONTENT_COMMAND_CUT:
      cmd = "cmd_cut";
      break;
    case NS_CONTENT_COMMAND_COPY:
      cmd = "cmd_copy";
      break;
    case NS_CONTENT_COMMAND_PASTE:
      cmd = "cmd_paste";
      break;
    case NS_CONTENT_COMMAND_DELETE:
      cmd = "cmd_delete";
      break;
    case NS_CONTENT_COMMAND_UNDO:
      cmd = "cmd_undo";
      break;
    case NS_CONTENT_COMMAND_REDO:
      cmd = "cmd_redo";
      break;
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
  nsCOMPtr<nsIController> controller;
  nsresult rv = fc->GetControllerForCommand(cmd, getter_AddRefs(controller));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!controller) {
    
    
    aEvent->mIsEnabled = PR_FALSE;
  } else {
    PRBool canDoIt;
    rv = controller->IsCommandEnabled(cmd, &canDoIt);
    NS_ENSURE_SUCCESS(rv, rv);
    aEvent->mIsEnabled = canDoIt;
    if (canDoIt && !aEvent->mOnlyEnabledCheck) {
      rv = controller->DoCommand(cmd);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  aEvent->mSucceeded = PR_TRUE;
  return NS_OK;
}
