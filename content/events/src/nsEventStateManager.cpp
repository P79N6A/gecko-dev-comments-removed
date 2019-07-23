













































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
#include "nsIObjectFrame.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif

#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"

#include "nsIFocusController.h"

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

#ifdef XP_MACOSX
#include <Carbon/Carbon.h>
#endif



#define NS_USER_INTERACTION_INTERVAL 5000 // ms

static NS_DEFINE_CID(kFrameTraversalCID, NS_FRAMETRAVERSAL_CID);


#define NON_KEYBINDING 0

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

nsIContent * gLastFocusedContent = 0; 
nsIDocument * gLastFocusedDocument = 0; 
nsPresContext* gLastFocusedPresContextWeak = 0; 

enum nsTextfieldSelectModel {
  eTextfieldSelect_unset = -1,
  eTextfieldSelect_manual = 0,
  eTextfieldSelect_auto = 1   
};



static PRInt8 sTextfieldSelectModel = eTextfieldSelect_unset;
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

  nsCOMPtr<nsIWidget> widget;
  nsIViewManager* vm = presShell ? presShell->GetViewManager() : nsnull;
  if (vm) {
    vm->GetWidget(getter_AddRefs(widget));
  }

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

static PRUint32 gMayNeedFocusSuppression = 0;

class nsSuppressUserFocus
{
public:
  nsSuppressUserFocus() : mActive(PR_FALSE) {}
  ~nsSuppressUserFocus() {
    if (mActive) {
      NS_ASSERTION(gMayNeedFocusSuppression,
                   "Trying to unsuppress too much!");
      --gMayNeedFocusSuppression;
    }
  }
  void MaybeSuppress()
  {
    if (!mActive) {
      mActive = PR_TRUE;
      ++gMayNeedFocusSuppression;
    }
  }
private:
  PRBool mActive;
};

static nsIDocument*
EventHandlingSuppressed(nsPIDOMEventTarget* aTarget)
{
  if (!gMayNeedFocusSuppression) {
    return nsnull;
  }
  nsCOMPtr<nsINode> node = do_QueryInterface(aTarget);
  nsCOMPtr<nsIDocument> doc;
  if (node) {
    doc = node->GetOwnerDoc();
  } else {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aTarget);
    if (window) {
      doc = do_QueryInterface(window->GetExtantDocument());
    }
  }

  return (doc && doc->EventHandlingSuppressed()) ? doc.get() : nsnull;
}

static void
FireFocusOrBlurEvent(nsPIDOMEventTarget* aTarget, nsEvent* aEvent, nsPresContext* aContext)
{
  NS_ASSERTION(aEvent->message == NS_BLUR_CONTENT ||
               aEvent->message == NS_FOCUS_CONTENT,
               "Wrong event!");
  nsIDocument* doc = EventHandlingSuppressed(aTarget);
  if (doc) {
    if (aContext) {
      nsIPresShell* shell = aContext->GetPresShell();
      if (shell) {
        shell->NeedsFocusOrBlurAfterSuppression(aTarget, aEvent->message);
      }
    }
  } else if (aTarget) {
    nsEventDispatcher::Dispatch(aTarget, aContext, aEvent);
  }
}

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

static PRBool GetWindowShowCaret(nsIDocument *aDocument);

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
protected:
  static nsIntPoint GetScreenPoint(nsGUIEvent* aEvent);
  static void OnFailToScrollTarget();
  static void OnTimeout(nsITimer *aTimer, void *aClosure);
  static void SetTimeout();
  static PRUint32 GetIgnoreMoveDelayTime();

  static nsWeakFrame sTargetFrame;
  static PRUint32    sTime;        
  static PRUint32    sMouseMoved;  
  static nsITimer*   sTimer;
};

nsWeakFrame nsMouseWheelTransaction::sTargetFrame(nsnull);
PRUint32    nsMouseWheelTransaction::sTime        = 0;
PRUint32    nsMouseWheelTransaction::sMouseMoved  = 0;
nsITimer*   nsMouseWheelTransaction::sTimer       = nsnull;

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
}

static PRBool
OutOfTime(PRUint32 aBaseTime, PRUint32 aThreshold)
{
  PRUint32 now = PR_IntervalToMilliseconds(PR_IntervalNow());
  return (now - aBaseTime > aThreshold);
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





nsEventStateManager::nsEventStateManager()
  : mLockCursor(0),
    mCurrentTarget(nsnull),
    mLastMouseOverFrame(nsnull),
    mLastDragOverFrame(nsnull),
    
    mGestureDownPoint(0,0),
    mCurrentFocusFrame(nsnull),
    mCurrentTabIndex(0),
    mLastFocusedWith(eEventFocusedByUnknown),
    mPresContext(nsnull),
    mLClickCount(0),
    mMClickCount(0),
    mRClickCount(0),
    mNormalLMouseEventInProcess(PR_FALSE),
    m_haveShutdown(PR_FALSE),
    mBrowseWithCaret(PR_FALSE),
    mTabbedThroughDocument(PR_FALSE),
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

      nsIContent::sTabFocusModelAppliesToXUL =
        nsContentUtils::GetBoolPref("accessibility.tabfocus_applies_to_xul",
                                    nsIContent::sTabFocusModelAppliesToXUL);
    }
    prefBranch->AddObserver("accessibility.accesskeycausesactivation", this, PR_TRUE);
    prefBranch->AddObserver("accessibility.browsewithcaret", this, PR_TRUE);
    prefBranch->AddObserver("accessibility.tabfocus_applies_to_xul", this, PR_TRUE);
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

  if (sTextfieldSelectModel == eTextfieldSelect_unset) {
    nsCOMPtr<nsILookAndFeel> lookNFeel(do_GetService(kLookAndFeelCID));
    PRInt32 selectTextfieldsOnKeyFocus = 0;
    lookNFeel->GetMetric(nsILookAndFeel::eMetric_SelectTextfieldsOnKeyFocus,
                         selectTextfieldsOnKeyFocus);
    sTextfieldSelectModel = selectTextfieldsOnKeyFocus ? eTextfieldSelect_auto:
                                                         eTextfieldSelect_manual;
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
    NS_IF_RELEASE(gLastFocusedContent);
    NS_IF_RELEASE(gLastFocusedDocument);
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
    prefBranch->RemoveObserver("accessibility.browsewithcaret", this);
    prefBranch->RemoveObserver("accessibility.tabfocus_applies_to_xul", this);
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
    } else if (data.EqualsLiteral("accessibility.browsewithcaret")) {
      ResetBrowseWithCaret();
    } else if (data.EqualsLiteral("accessibility.tabfocus_applies_to_xul")) {
      nsIContent::sTabFocusModelAppliesToXUL =
        nsContentUtils::GetBoolPref("accessibility.tabfocus_applies_to_xul",
                                    nsIContent::sTabFocusModelAppliesToXUL);
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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentFocus);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastFocus);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastContentFocus);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstBlurEvent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstDocumentBlurEvent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFirstFocusEvent);
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
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCurrentFocus);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastFocus);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastContentFocus);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstBlurEvent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstDocumentBlurEvent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstFocusEvent);
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

  nsSuppressUserFocus userFocus;

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
  case NS_GOTFOCUS:
    {
#ifdef DEBUG_smaug
      printf("nsEventStateManager::PreHandleEvent, NS_GOTFOCUS \n");
#endif
      userFocus.MaybeSuppress();
      
      
      
      

      EnsureDocument(aPresContext);

      
      
      

      if (gLastFocusedDocument == mDocument)
        break;

      if (mDocument) {
        nsIMEStateManager::OnTextStateBlur(mPresContext, mCurrentFocus);

        if (gLastFocusedDocument && gLastFocusedPresContextWeak) {
          nsCOMPtr<nsPIDOMWindow> ourWindow =
            gLastFocusedDocument->GetWindow();

          
          
          
          
          

          
          
          nsCOMPtr<nsIFocusController> focusController;
          PRBool isAlreadySuppressed = PR_FALSE;

          if (ourWindow) {
            focusController = ourWindow->GetRootFocusController();
            if (focusController) {
              focusController->GetSuppressFocus(&isAlreadySuppressed);
              focusController->SetSuppressFocus(PR_TRUE,
                                                "NS_GOTFOCUS ESM Suppression");
            }
          }

          if (!isAlreadySuppressed) {

            
            nsEvent blurEvent(PR_TRUE, NS_BLUR_CONTENT);
            blurEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

            FireFocusOrBlurEvent(gLastFocusedDocument, &blurEvent,
                                 gLastFocusedPresContextWeak);

            nsCOMPtr<nsIEventStateManager> esm;
            if (!mCurrentFocus && gLastFocusedContent) {
              
              
              
              if (gLastFocusedContent) {
                nsCOMPtr<nsIDocument> doc = gLastFocusedContent->GetCurrentDoc();
                if (doc) {
                  nsIPresShell *shell = doc->GetPrimaryShell();
                  if (shell) {
                    nsCOMPtr<nsPresContext> oldPresContext = shell->GetPresContext();
                    esm = oldPresContext->EventStateManager();
                    if (esm) {
                      esm->SetFocusedContent(gLastFocusedContent);
                    }
                  }
                }
              }
              nsCOMPtr<nsIContent> blurContent = gLastFocusedContent;
              blurEvent.target = nsnull;
              FireFocusOrBlurEvent(gLastFocusedContent, &blurEvent,
                                   gLastFocusedPresContextWeak);
            }
            if (ourWindow) {
              
              blurEvent.target = nsnull;
              nsCOMPtr<nsPIDOMEventTarget> win = do_QueryInterface(ourWindow);
              FireFocusOrBlurEvent(win, &blurEvent, gLastFocusedPresContextWeak);
            }

            if (esm) {
              esm->SetFocusedContent(nsnull);
            }
            NS_IF_RELEASE(gLastFocusedContent);
          }

          if (focusController)
            focusController->SetSuppressFocus(PR_FALSE,
                                              "NS_GOTFOCUS ESM Suppression");
        }

        
        

        nsCOMPtr<nsPIDOMWindow> window(mDocument->GetWindow());

        if (window) {
          
          

          nsCOMPtr<nsIContent> currentFocus = mCurrentFocus;
          
          SetFocusedContent(nsnull);

          nsIMEStateManager::OnChangeFocus(mPresContext, currentFocus);

          nsEvent focusEvent(PR_TRUE, NS_FOCUS_CONTENT);
          focusEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

          if (gLastFocusedDocument != mDocument) {
            FireFocusOrBlurEvent(mDocument, &focusEvent, aPresContext);
            if (currentFocus && currentFocus != gLastFocusedContent) {
              
              focusEvent.target = nsnull;
              FireFocusOrBlurEvent(currentFocus, &focusEvent, aPresContext);
            }
          }

          
          focusEvent.target = nsnull;
          nsCOMPtr<nsPIDOMEventTarget> win = do_QueryInterface(window);
          FireFocusOrBlurEvent(win, &focusEvent, aPresContext);

          SetFocusedContent(currentFocus); 
          NS_IF_RELEASE(gLastFocusedContent);
          gLastFocusedContent = mCurrentFocus;
          NS_IF_ADDREF(gLastFocusedContent);

          nsIMEStateManager::OnTextStateFocus(mPresContext, mCurrentFocus);
        }

        
        if (gLastFocusedDocument && gLastFocusedDocument != mDocument) {

          nsIFocusController *lastController = nsnull;
          nsPIDOMWindow* lastWindow = gLastFocusedDocument->GetWindow();
          if (lastWindow)
            lastController = lastWindow->GetRootFocusController();

          nsIFocusController *nextController = nsnull;
          nsPIDOMWindow* nextWindow = mDocument->GetWindow();
          if (nextWindow)
            nextController = nextWindow->GetRootFocusController();

          if (lastController != nextController && lastController && nextController)
            lastController->SetActive(PR_FALSE);
        }

        NS_IF_RELEASE(gLastFocusedDocument);
        gLastFocusedDocument = mDocument;
        gLastFocusedPresContextWeak = aPresContext;
        NS_IF_ADDREF(gLastFocusedDocument);
      }

      ResetBrowseWithCaret();
    }

    break;

  case NS_LOSTFOCUS:
    {
#ifdef DEBUG_smaug
      printf("nsEventStateManager::PreHandleEvent, NS_LOSTFOCUS \n");
#endif
      userFocus.MaybeSuppress();
      
      if (mPresContext) {
        nsIPresShell *presShell = mPresContext->GetPresShell();
        if (presShell) {
           nsRefPtr<nsCaret> caret;
           presShell->GetCaret(getter_AddRefs(caret));
           if (caret) {
             PRBool caretVisible = PR_FALSE;
             caret->GetCaretVisible(&caretVisible);
             if (caretVisible) {
               SetContentCaretVisible(presShell, mCurrentFocus, PR_FALSE);
             }
           }
        }
      }

      
      
      

#if defined(XP_WIN) || defined(XP_OS2)
      
      
      if (aEvent->eventStructType == NS_FOCUS_EVENT &&
          !static_cast<nsFocusEvent*>(aEvent)->isMozWindowTakingFocus) {

        
        
        

        EnsureDocument(aPresContext);

        if (gLastFocusedContent && !gLastFocusedContent->IsInDoc()) {
          NS_RELEASE(gLastFocusedContent);
        }

        nsIMEStateManager::OnTextStateBlur(nsnull, nsnull);

        
        
        nsEvent blurEvent(PR_TRUE, NS_BLUR_CONTENT);
        blurEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

        if (gLastFocusedDocument && gLastFocusedPresContextWeak) {
          if (gLastFocusedContent) {
            
            
            nsCOMPtr<nsIDocument> doc = gLastFocusedContent->GetDocument();
            if (doc) {
              nsIPresShell *shell = doc->GetPrimaryShell();
              if (shell) {
                nsCOMPtr<nsPresContext> oldPresContext =
                  shell->GetPresContext();

                nsCOMPtr<nsIEventStateManager> esm =
                  oldPresContext->EventStateManager();
                esm->SetFocusedContent(gLastFocusedContent);
                FireFocusOrBlurEvent(gLastFocusedContent, &blurEvent, oldPresContext);
                esm->SetFocusedContent(nsnull);
                NS_IF_RELEASE(gLastFocusedContent);
              }
            }
          }

          
          
          nsCOMPtr<nsIDocument> lastFocusedDocument;
          lastFocusedDocument.swap(gLastFocusedDocument);
          nsCOMPtr<nsPresContext> lastFocusedPresContext = gLastFocusedPresContextWeak;
          gLastFocusedPresContextWeak = nsnull;
          mCurrentTarget = nsnull;

          
          if (lastFocusedDocument) {
            
            

            nsCOMPtr<nsPIDOMWindow> window(lastFocusedDocument->GetWindow());

            blurEvent.target = nsnull;
            FireFocusOrBlurEvent(lastFocusedDocument, &blurEvent, lastFocusedPresContext);

            if (window) {
              blurEvent.target = nsnull;
              nsCOMPtr<nsPIDOMEventTarget> win = do_QueryInterface(window);
              FireFocusOrBlurEvent(win, &blurEvent ,lastFocusedPresContext);
            }
          }
        }
      }
#endif
    }
    break;

 case NS_ACTIVATE:
    {
#ifdef DEBUG_smaug
      printf("nsEventStateManager::PreHandleEvent, NS_ACTIVATE \n");
#endif
      userFocus.MaybeSuppress();
      
      
      

      EnsureDocument(aPresContext);

      nsCOMPtr<nsPIDOMWindow> win = mDocument->GetWindow();

      if (!win) {
        
        return NS_ERROR_NULL_POINTER;
      }

      
      
      nsCOMPtr<nsIFocusController> focusController =
        win->GetRootFocusController();
      nsCOMPtr<nsIDOMElement> focusedElement;
      nsCOMPtr<nsIDOMWindowInternal> focusedWindow;
      nsFocusScrollSuppressor scrollSuppressor;

      if (focusController) {
        
        focusController->GetFocusedWindow(getter_AddRefs(focusedWindow));
        focusController->GetFocusedElement(getter_AddRefs(focusedElement));

        scrollSuppressor.Init(focusController);
        focusController->SetActive(PR_TRUE);
      }

      if (!focusedWindow)
        focusedWindow = win;

      NS_ASSERTION(focusedWindow,"check why focusedWindow is null!!!");

      
      if (focusedWindow) {
        focusedWindow->Focus();

        nsCOMPtr<nsIDocument> document = GetDocumentFromWindow(focusedWindow);

        if (document) {
          
          
          nsCOMPtr<nsIPresShell> shell = document->GetPrimaryShell();
          NS_ASSERTION(shell, "Focus events should not be getting thru when this is null!");
          if (shell) {
            nsPresContext* context = shell->GetPresContext();
            nsIMEStateManager::OnActivate(context);
            if (focusedElement) {
              nsCOMPtr<nsIContent> focusContent = do_QueryInterface(focusedElement);
              focusContent->SetFocus(context);
            } else {
              nsIMEStateManager::OnChangeFocus(context, nsnull);
            }

            
            nsCOMPtr<nsFrameSelection> frameSelection = shell->FrameSelection();
            frameSelection->SetMouseDownState(PR_FALSE);
          }
        }
      }

      if (focusController) {
        
        

        if (gLastFocusedDocument && gLastFocusedDocument == mDocument) {
          nsCOMPtr<nsIDOMElement> focusElement = do_QueryInterface(mCurrentFocus);
          focusController->SetFocusedElement(focusElement);
        }

        PRBool isSuppressed;
        focusController->GetSuppressFocus(&isSuppressed);
        while (isSuppressed) {
          
          focusController->SetSuppressFocus(PR_FALSE,
                                            "Activation Suppression");

          focusController->GetSuppressFocus(&isSuppressed);
        }
      }
    }
    break;

 case NS_DEACTIVATE:
    {
#ifdef DEBUG_smaug
      printf("nsEventStateManager::PreHandleEvent, NS_DEACTIVATE \n");
#endif
      userFocus.MaybeSuppress();
      EnsureDocument(aPresContext);

      nsIMEStateManager::OnDeactivate(aPresContext);

      nsCOMPtr<nsPIDOMWindow> ourWindow(mDocument->GetWindow());

      
      
      
      

      nsCOMPtr<nsIFocusController> focusController =
        GetFocusControllerForDocument(mDocument);

      if (focusController)
        focusController->SetSuppressFocus(PR_TRUE, "Deactivate Suppression");

      nsIMEStateManager::OnTextStateBlur(nsnull, nsnull);

      

      if (gLastFocusedDocument && gLastFocusedDocument == mDocument &&
          gLastFocusedDocument != mFirstDocumentBlurEvent) {

        PRBool clearFirstDocumentBlurEvent = PR_FALSE;
        if (!mFirstDocumentBlurEvent) {
          mFirstDocumentBlurEvent = gLastFocusedDocument;
          clearFirstDocumentBlurEvent = PR_TRUE;
        }

        nsEvent blurEvent(PR_TRUE, NS_BLUR_CONTENT);
        blurEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

        if (gLastFocusedContent) {
          nsIPresShell *shell = gLastFocusedDocument->GetPrimaryShell();
          if (shell) {
            nsCOMPtr<nsPresContext> oldPresContext = shell->GetPresContext();

            nsCOMPtr<nsIDOMElement> focusedElement;
            if (focusController)
              focusController->GetFocusedElement(getter_AddRefs(focusedElement));

            nsCOMPtr<nsIEventStateManager> esm;
            esm = oldPresContext->EventStateManager();
            esm->SetFocusedContent(gLastFocusedContent);

            nsCOMPtr<nsIContent> focusedContent = do_QueryInterface(focusedElement);
            if (focusedContent) {
              
              FireFocusOrBlurEvent(focusedContent, &blurEvent, oldPresContext);
            }

            esm->SetFocusedContent(nsnull);
            NS_IF_RELEASE(gLastFocusedContent);
          }
        }

        
        
        mCurrentTarget = nsnull;
        NS_IF_RELEASE(gLastFocusedDocument);
        gLastFocusedPresContextWeak = nsnull;

        
        blurEvent.target = nsnull;
        FireFocusOrBlurEvent(mDocument, &blurEvent, aPresContext);

        if (ourWindow) {
          blurEvent.target = nsnull;
          nsCOMPtr<nsPIDOMEventTarget> win = do_QueryInterface(ourWindow);
          FireFocusOrBlurEvent(win, &blurEvent, aPresContext);
        }
        if (clearFirstDocumentBlurEvent) {
          mFirstDocumentBlurEvent = nsnull;
        }
      }

      if (focusController) {
        focusController->SetActive(PR_FALSE);
        focusController->SetSuppressFocus(PR_FALSE, "Deactivate Suppression");
      }
    }

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
      if (mCurrentFocus) {
        mCurrentTargetContent = mCurrentFocus;
      }
    }
    break;
  case NS_MOUSE_SCROLL:
    {
      if (mCurrentFocus) {
        mCurrentTargetContent = mCurrentFocus;
      }

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
      if (mCurrentFocus) {
        mCurrentTargetContent = mCurrentFocus;
      }

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
  }

  return PR_FALSE;
}

PRBool
nsEventStateManager::ExecuteAccessKey(nsTArray<PRUint32>& aAccessCharCodes,
                                      PRBool aIsTrustedEvent)
{
  PRInt32 count, start = -1;
  if (mCurrentFocus) {
    start = mAccessKeys.IndexOf(mCurrentFocus);
    if (start == -1 && mCurrentFocus->GetBindingParent())
      start = mAccessKeys.IndexOf(mCurrentFocus->GetBindingParent());
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
        else if (frame && frame->IsFocusable())
          ChangeFocusWith(content, eEventFocusedByKey);
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

  if(!gLastFocusedDocument) return NS_ERROR_FAILURE;

  nsPIDOMWindow* ourWindow = gLastFocusedDocument->GetWindow();
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
    GetFocusedContent(getter_AddRefs(targetContent));
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
  if (!targetContent)
    GetFocusedContent(getter_AddRefs(targetContent));
  if (!targetContent)
    return;

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
                                  nsInputEvent* aEvent,
                                  PRInt32 aNumLines,
                                  PRBool aScrollHorizontal,
                                  ScrollQuantity aScrollQuantity)
{
  nsIScrollableView* scrollView = nsnull;
  nsIFrame* scrollFrame = aTargetFrame;

  
  
  
  
  
  
  
  
  nsIFrame* lastScrollFrame = nsMouseWheelTransaction::GetTargetFrame();
  if (lastScrollFrame) {
    nsIScrollableViewProvider* svp = do_QueryFrame(lastScrollFrame);
    if (svp && (scrollView = svp->GetScrollableView())) {
      nsMouseWheelTransaction::UpdateTransaction(aNumLines, aScrollHorizontal);
      
      
      
      
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
        (aScrollHorizontal ? ss.mHorizontal : ss.mVertical)) {
      continue;
    }

    
    nscoord lineHeight;
    scrollView->GetLineHeight(&lineHeight);

    if (lineHeight != 0) {
      if (CanScrollOn(scrollView, aNumLines, aScrollHorizontal)) {
        passToParent = PR_FALSE;
        nsMouseWheelTransaction::BeginTransaction(scrollFrame,
                                                  aNumLines, aScrollHorizontal);
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
        nscoord pageScroll = aScrollHorizontal ?
          pageScrollDistances.width : pageScrollDistances.height;

        if (PR_ABS(aNumLines) * lineHeight > pageScroll) {
          nscoord maxLines = (pageScroll / lineHeight);
          if (maxLines >= 1) {
            aNumLines = ((aNumLines < 0) ? -1 : 1) * maxLines;
          } else {
            aScrollQuantity = eScrollByPage;
          }
        }
      }
    }

    PRInt32 scrollX = 0;
    PRInt32 scrollY = aNumLines;

    if (aScrollQuantity == eScrollByPage)
      scrollY = (scrollY > 0) ? 1 : -1;
      
    if (aScrollHorizontal) {
      scrollX = scrollY;
      scrollY = 0;
    }
    
    if (aScrollQuantity == eScrollByPage)
      scrollView->ScrollByPages(scrollX, scrollY, NS_VMREFRESH_SMOOTHSCROLL);
    else if (aScrollQuantity == eScrollByPixel)
      scrollView->ScrollByPixels(scrollX, scrollY, NS_VMREFRESH_DEFERRED);
    else
      scrollView->ScrollByLines(scrollX, scrollY, NS_VMREFRESH_SMOOTHSCROLL);
  }
  if (passToParent) {
    nsresult rv;
    nsIFrame* newFrame = nsnull;
    nsCOMPtr<nsPresContext> newPresContext;
    rv = GetParentScrollingView(aEvent, aPresContext, newFrame,
                                *getter_AddRefs(newPresContext));
    if (NS_SUCCEEDED(rv) && newFrame)
      return DoScrollText(newPresContext, newFrame, aEvent, aNumLines,
                          aScrollHorizontal, aScrollQuantity);
  }

  return NS_OK;
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
        
        
        if (aView) {
          nsIViewManager* viewMan = aView->GetViewManager();
          if (viewMan) {
            nsIView* grabbingView;
            viewMan->GetMouseEventGrabber(grabbingView);
            if (grabbingView == aView) {
              PRBool result;
              viewMan->GrabMouseEvents(nsnull, result);
            }
          }
        }
        break;
      }

      if (nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> newFocus;
        PRBool suppressBlur = PR_FALSE;
        if (mCurrentTarget) {
          mCurrentTarget->GetContentForEvent(mPresContext, aEvent, getter_AddRefs(newFocus));
          const nsStyleUserInterface* ui = mCurrentTarget->GetStyleUserInterface();
          suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);
        }

        nsIFrame* currFrame = mCurrentTarget;
        nsIContent* activeContent = nsnull;
        if (mCurrentTarget)
          activeContent = mCurrentTarget->GetContent();

        
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

        if (newFocus && currFrame)
          ChangeFocusWith(newFocus, eEventFocusedByMouse);
        else if (!suppressBlur) {
          SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
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
        nsIViewManager* viewMan = shell->GetViewManager();
        if (viewMan) {
          nsIView* grabbingView = nsnull;
          viewMan->GetMouseEventGrabber(grabbingView);
          if (grabbingView == aView) {
            PRBool result;
            viewMan->GrabMouseEvents(nsnull, result);
          }
        }
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
          {
            DoScrollText(presContext, aTargetFrame, msEvent, msEvent->delta,
                         !!(msEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal),
                         eScrollByLine);
          }
          break;

        case MOUSE_SCROLL_PAGE:
          {
            DoScrollText(presContext, aTargetFrame, msEvent, msEvent->delta,
                         !!(msEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal),
                         eScrollByPage);
          }
          break;

        case MOUSE_SCROLL_PIXELS:
          {
            DoScrollText(presContext, aTargetFrame, msEvent, msEvent->delta,
                         !!(msEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal),
                         eScrollByPixel);
          }
          break;

        case MOUSE_SCROLL_HISTORY:
          {
            DoScrollHistory(msEvent->delta);
          }
          break;

        case MOUSE_SCROLL_ZOOM:
          {
            DoScrollZoom(aTargetFrame, msEvent->delta);
          }
          break;

        default:  
          break;
        }
        *aStatus = nsEventStatus_eConsumeNoDefault;
      }
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

          
          
          dropEffect = nsDOMDragEvent::FilterDropEffect(action,
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
            if (!((nsInputEvent*)aEvent)->isControl) {
              
              ShiftFocus(!((nsInputEvent*)aEvent)->isShift);
            } else {
              ShiftFocusByDoc(!((nsInputEvent*)aEvent)->isShift);
            }
            *aStatus = nsEventStatus_eConsumeNoDefault;
            break;

          case NS_VK_F6:
            
            ShiftFocusByDoc(!((nsInputEvent*)aEvent)->isShift);
            *aStatus = nsEventStatus_eConsumeNoDefault;
            break;


#if NON_KEYBINDING
          case NS_VK_PAGE_DOWN:
          case NS_VK_PAGE_UP:
            if (!mCurrentFocus) {
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
            if (!mCurrentFocus) {
              nsIScrollableView* sv = nsLayoutUtils::GetNearestScrollingView(aView, nsLayoutUtils::eVertical);
              if (sv) {
                nsKeyEvent * keyEvent = (nsKeyEvent *)aEvent;
                sv->ScrollByWhole((keyEvent->keyCode != NS_VK_HOME) ? PR_FALSE : PR_TRUE);
              }
            }
            break;
          case NS_VK_DOWN:
          case NS_VK_UP:
            if (!mCurrentFocus) {
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
            if (!mCurrentFocus) {
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
              if (!mCurrentFocus) {
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
  if (aPresContext == nsnull) {
    
    
    if (mPresContext == gLastFocusedPresContextWeak) {
      gLastFocusedPresContextWeak = nsnull;
      NS_IF_RELEASE(gLastFocusedDocument);
      NS_IF_RELEASE(gLastFocusedContent);
    }
  }

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
nsEventStateManager::ChangeFocusWith(nsIContent* aFocusContent,
                                     EFocusedWithType aFocusedWith)
{
  mLastFocusedWith = aFocusedWith;
  if (!aFocusContent) {
    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
    return NS_OK;
  }

  
  EnsureDocument(mPresContext);
  nsCOMPtr<nsIFocusController> focusController = nsnull;
  nsCOMPtr<nsPIDOMWindow> window(mDocument->GetWindow());
  if (window)
    focusController = window->GetRootFocusController();

  
  
  nsFocusScrollSuppressor scrollSuppressor;
  if (aFocusedWith == eEventFocusedByMouse) {
    scrollSuppressor.Init(focusController);
  }

  aFocusContent->SetFocus(mPresContext);
  if (aFocusedWith != eEventFocusedByMouse) {
    MoveCaretToFocus();
    
    if (sTextfieldSelectModel == eTextfieldSelect_auto &&
        mCurrentFocus &&
        mCurrentFocus->IsNodeOfType(nsINode::eHTML_FORM_CONTROL)) {
      nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(mCurrentFocus));
      PRInt32 controlType = formControl->GetType();
      if (controlType == NS_FORM_INPUT_TEXT ||
          controlType == NS_FORM_INPUT_PASSWORD) {
        nsCOMPtr<nsIDOMHTMLInputElement> inputElement =
          do_QueryInterface(mCurrentFocus);
        if (inputElement) {
          inputElement->Select();
        }
      }
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsEventStateManager::ShiftFocus(PRBool aForward, nsIContent* aStart)
{
  nsCOMPtr<nsILookAndFeel> lookNFeel(do_GetService(kLookAndFeelCID));
  lookNFeel->GetMetric(nsILookAndFeel::eMetric_TabFocusModel,
                       nsIContent::sTabFocusModel);

  
  
  
  
  

  mTabbedThroughDocument = PR_FALSE;
  return ShiftFocusInternal(aForward, aStart);
}

nsresult
nsEventStateManager::ShiftFocusInternal(PRBool aForward, nsIContent* aStart)
{
#ifdef DEBUG_DOCSHELL_FOCUS
  printf("[%p] ShiftFocusInternal: aForward=%d, aStart=%p, mCurrentFocus=%p\n",
         static_cast<void*>(this), aForward, static_cast<void*>(aStart),
         static_cast<void*>(mCurrentFocus.get()));
#endif
  NS_ASSERTION(mPresContext, "no pres context");
  EnsureDocument(mPresContext);
  NS_ASSERTION(mDocument, "no document");

  nsCOMPtr<nsIContent> rootContent = mDocument->GetRootContent();

  nsCOMPtr<nsISupports> pcContainer = mPresContext->GetContainer();
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(pcContainer));
  NS_ENSURE_STATE(docShell);
  PRBool docHasFocus = PR_FALSE;

  
  
  
  PRBool ignoreTabIndex = PR_FALSE;

  if (!aStart && !mCurrentFocus) {
    
    
    
    
    
    

    docShell->GetHasFocus(&docHasFocus);
  }

  nsIFrame* selectionFrame = nsnull;
  nsIFrame* curFocusFrame = nsnull;   

  
  
  nsCOMPtr<nsIPresShell> presShell = mPresContext->PresShell();

  
  PRInt32 itemType;
  nsCOMPtr<nsIDocShellTreeItem> shellItem(do_QueryInterface(docShell));
  shellItem->GetItemType(&itemType);

  
  
  if (!aStart && itemType != nsIDocShellTreeItem::typeChrome) {
    
    if (!mCurrentFocus || (mLastFocusedWith != eEventFocusedByMouse && mCurrentFocus->Tag() != nsGkAtoms::area)) {
      nsCOMPtr<nsIContent> selectionContent, endSelectionContent;  
      PRUint32 selectionOffset; 
      GetDocSelectionLocation(getter_AddRefs(selectionContent), getter_AddRefs(endSelectionContent), &selectionFrame, &selectionOffset);
      if (selectionContent == rootContent)  
        selectionFrame = nsnull;
      
      
      nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
      if (htmlDoc &&
          htmlDoc->GetEditingState() == nsIHTMLDocument::eContentEditable &&
          selectionContent && selectionContent->HasFlag(NODE_IS_EDITABLE))
        selectionFrame = nsnull;
      
      
      
      if (selectionFrame) {
        PRBool selectionWithFocus;
        MoveFocusToCaret(PR_FALSE, &selectionWithFocus);
        ignoreTabIndex = !selectionWithFocus;
        
        
        GetDocSelectionLocation(getter_AddRefs(selectionContent),
                                getter_AddRefs(endSelectionContent),
                                &selectionFrame, &selectionOffset);
      }
    }
  }

  nsIContent *startContent = nsnull;

  if (aStart) {
    curFocusFrame = presShell->GetPrimaryFrameFor(aStart);

    
    
    if (curFocusFrame)
      startContent = aStart;
  } else if (selectionFrame) {
    
    
    startContent = mCurrentFocus;
    curFocusFrame = selectionFrame;
  } else if (!docHasFocus) {
    startContent = mCurrentFocus;
    GetFocusedFrame(&curFocusFrame);
  }

  if (aStart) {
    if (aStart->HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
      aStart->IsFocusable(&mCurrentTabIndex);
    } else {
      ignoreTabIndex = PR_TRUE; 
    }
  } else if (!mCurrentFocus) {  
    if (aForward) {
      mCurrentTabIndex = docHasFocus && selectionFrame ? 0 : 1;
    } else if (!docHasFocus) {
      mCurrentTabIndex = 0;
    } else if (selectionFrame) {
      mCurrentTabIndex = 1;   
    }
  }

  
  
  
  nsIFrame* popupFrame = nsnull;
  if (curFocusFrame) {
    
    
    
    popupFrame = nsLayoutUtils::GetClosestFrameOfType(curFocusFrame,
                                                      nsGkAtoms::menuPopupFrame);
  }
#ifdef MOZ_XUL
  else {
    
    
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm) {
      popupFrame = pm->GetTopPopup(ePopupTypePanel);
    }
  }
#endif

  if (popupFrame) {
    
    
    rootContent = popupFrame->GetContent();
    NS_ASSERTION(rootContent, "Popup frame doesn't have a content node");
  }

  nsCOMPtr<nsIContent> nextFocus;
  nsIFrame* nextFocusFrame;
  if (aForward || !docHasFocus || selectionFrame)
    GetNextTabbableContent(rootContent, startContent, curFocusFrame,
                           aForward, ignoreTabIndex || mCurrentTabIndex < 0,
                           getter_AddRefs(nextFocus), &nextFocusFrame);

  if (popupFrame && !nextFocus) {
    
    
    
    mCurrentTabIndex = aForward ? 1 : 0;
    GetNextTabbableContent(rootContent, rootContent, nsnull,
                           aForward, ignoreTabIndex,
                           getter_AddRefs(nextFocus), &nextFocusFrame);
    
    if (startContent == nextFocus) {
      nextFocus = nsnull;
    }
  }

  
  
  mCurrentTabIndex = 0;

  if (nextFocus) {
    
    
    

    nsCOMPtr<nsIDocShell> sub_shell;
    nsCOMPtr<nsIDocument> doc = nextFocus->GetDocument();

    if (doc) {
      nsIDocument *sub_doc = doc->GetSubDocumentFor(nextFocus);

      if (sub_doc && !sub_doc->EventHandlingSuppressed()) {
        nsCOMPtr<nsISupports> container = sub_doc->GetContainer();
        sub_shell = do_QueryInterface(container);
      }
    }

    if (sub_shell) {
      
      presShell->ScrollContentIntoView(nextFocus,
                                       NS_PRESSHELL_SCROLL_ANYWHERE,
                                       NS_PRESSHELL_SCROLL_ANYWHERE);

      SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

      
      
      
      if (mTabbingFromDocShells.IndexOf(sub_shell) != -1)
        return NS_OK;

      TabIntoDocument(sub_shell, aForward);
    } else {
      
#ifdef DEBUG_DOCSHELL_FOCUS
      printf("focusing next focusable content: %p\n",
             static_cast<void*>(nextFocus.get()));
#endif
      mCurrentTarget = nextFocusFrame;

      nsCOMPtr<nsIContent> oldFocus(mCurrentFocus);
      ChangeFocusWith(nextFocus, eEventFocusedByKey);
      if (!mCurrentFocus && oldFocus) {
        
        
        
        if (oldFocus != aStart && oldFocus->GetDocument()) {
          mCurrentTarget = nsnull;
          return ShiftFocusInternal(aForward, oldFocus);
        } else {
          return NS_OK;
        }
      } else {
        if (mCurrentFocus != nextFocus) {
          
          
          

          return NS_OK;
        }
        nsIFrame* focusedFrame = nsnull;
        GetFocusedFrame(&focusedFrame);
        mCurrentTarget = focusedFrame;
      }

      
      
      
      

      if (oldFocus && doc != nextFocus->GetDocument()) {
        mCurrentTarget = nsnull;
        return ShiftFocusInternal(aForward, oldFocus);
      }

      if (!docHasFocus)
        docShell->SetHasFocus(PR_TRUE);
    }
  } else {

    
    

    PRBool focusDocument;
    if (itemType == nsIDocShellTreeItem::typeChrome)
      focusDocument = PR_FALSE;
    else {
      
      focusDocument = !(IsFrameSetDoc(docShell));
    }

    if (!aForward && !docHasFocus && focusDocument) {
#ifdef DEBUG_DOCSHELL_FOCUS
      printf("Focusing document\n");
#endif
      SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
      docShell->SetHasFocus(PR_TRUE);
      docShell->SetCanvasHasFocus(PR_TRUE);
      
      
      
      
      SetFocusedContent(rootContent);
      MoveCaretToFocus();
      SetFocusedContent(nsnull);

    } else {
      
      
      
      
      

      if (mTabbedThroughDocument)
        return NS_OK;

      SetFocusedContent(rootContent);
      mCurrentTabIndex = 0;
      MoveCaretToFocus();
      SetFocusedContent(nsnull);

      mTabbedThroughDocument = PR_TRUE;

      nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(pcContainer);
      nsCOMPtr<nsIDocShellTreeItem> treeParent;
      treeItem->GetParent(getter_AddRefs(treeParent));
      if (treeParent) {
        nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(treeParent);
        if (parentDS) {
          nsCOMPtr<nsIPresShell> parentShell;
          parentDS->GetPresShell(getter_AddRefs(parentShell));

          nsCOMPtr<nsIDocument> parent_doc = parentShell->GetDocument();
          nsCOMPtr<nsIContent> docContent = parent_doc->FindContentForSubDocument(mDocument);

          nsCOMPtr<nsPresContext> parentPC = parentShell->GetPresContext();
          nsIEventStateManager *parentESM = parentPC->EventStateManager();

          SetContentState(nsnull, NS_EVENT_STATE_FOCUS);

          nsCOMPtr<nsISupports> parentContainer = parentPC->GetContainer();
          if (parentContainer && docContent && docContent->GetCurrentDoc() == parent_doc) {
#ifdef DEBUG_DOCSHELL_FOCUS
            printf("popping out focus to parent docshell\n");
#endif
            parentESM->MoveCaretToFocus();
            parentESM->ShiftFocus(aForward, docContent);
#ifdef DEBUG_DOCSHELL_FOCUS
          } else {
            printf("can't pop out focus to parent docshell\n"); 
#endif
          }
        }
      } else {
        PRBool tookFocus = PR_FALSE;
        nsCOMPtr<nsIDocShell> subShell = do_QueryInterface(pcContainer);
        if (subShell) {
          subShell->TabToTreeOwner(aForward, &tookFocus);
        }

#ifdef DEBUG_DOCSHEL_FOCUS
        printf("offered focus to tree owner, tookFocus=%d\n",
               tookFocus);
#endif

        if (tookFocus) {
          SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
          docShell->SetHasFocus(PR_FALSE);
        } else {
          
          
          

#ifdef DEBUG_DOCSHELL_FOCUS
          printf("wrapping around within this document\n");
#endif

          SetFocusedContent(nsnull);
          docShell->SetHasFocus(PR_FALSE);
          ShiftFocusInternal(aForward);
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsEventStateManager::GetNextTabbableContent(nsIContent* aRootContent,
                                            nsIContent* aStartContent,
                                            nsIFrame* aStartFrame,
                                            PRBool forward,
                                            PRBool aIgnoreTabIndex,
                                            nsIContent** aResultNode,
                                            nsIFrame** aResultFrame)
{
  *aResultNode = nsnull;
  *aResultFrame = nsnull;

  nsresult rv;
  nsCOMPtr<nsIFrameTraversal> trav(do_CreateInstance(kFrameTraversalCID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFrameEnumerator> frameTraversal;

  
  if (!aStartFrame) {
    
    NS_ENSURE_TRUE(mPresContext, NS_ERROR_FAILURE);
    nsIPresShell *presShell = mPresContext->GetPresShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    aStartFrame = presShell->GetPrimaryFrameFor(aRootContent);
    NS_ENSURE_TRUE(aStartFrame, NS_ERROR_FAILURE);
    rv = trav->NewFrameTraversal(getter_AddRefs(frameTraversal),
                                mPresContext, aStartFrame,
                                ePreOrder,
                                PR_FALSE, 
                                PR_FALSE, 
                                PR_TRUE   
                                );
    NS_ENSURE_SUCCESS(rv, rv);
    if (!forward) {
      frameTraversal->Last();
    }
  }
  else {
    rv = trav->NewFrameTraversal(getter_AddRefs(frameTraversal),
                                 mPresContext, aStartFrame,
                                 ePreOrder,
                                 PR_FALSE, 
                                 PR_FALSE, 
                                 PR_TRUE   
                                 );
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aStartContent || aStartContent->Tag() != nsGkAtoms::area ||
        !aStartContent->IsNodeOfType(nsINode::eHTML)) {
      
      
      if (forward)
        frameTraversal->Next();
      else
        frameTraversal->Prev();
    }
  }

  
  while (1) {
    *aResultFrame = frameTraversal->CurrentItem();
    if (!*aResultFrame) {
      break;
    }

    
    
    
    
    
    
    
    PRInt32 tabIndex;
    nsIContent* currentContent = (*aResultFrame)->GetContent();
    (*aResultFrame)->IsFocusable(&tabIndex);
    if (tabIndex >= 0) {
      if (currentContent->Tag() == nsGkAtoms::img &&
          currentContent->HasAttr(kNameSpaceID_None, nsGkAtoms::usemap)) {
        
        
        nsIContent *areaContent = GetNextTabbableMapArea(forward, currentContent);
        if (areaContent) {
          NS_ADDREF(*aResultNode = areaContent);
          return NS_OK;
        }
      }
      else if ((aIgnoreTabIndex || mCurrentTabIndex == tabIndex) &&
          currentContent != aStartContent) {
        NS_ADDREF(*aResultNode = currentContent);
        return NS_OK;
      }
    }
    if (forward)
      frameTraversal->Next();
    else
      frameTraversal->Prev();
  }

  

  
  
  if (mCurrentTabIndex == (forward? 0: 1)) {
    return NS_OK;
  }

  
  mCurrentTabIndex = GetNextTabIndex(aRootContent, forward);
  return GetNextTabbableContent(aRootContent, aStartContent, nsnull, forward,
                                aIgnoreTabIndex, aResultNode, aResultFrame);
}

nsIContent*
nsEventStateManager::GetNextTabbableMapArea(PRBool aForward,
                                            nsIContent *aImageContent)
{
  nsAutoString useMap;
  aImageContent->GetAttr(kNameSpaceID_None, nsGkAtoms::usemap, useMap);

  nsCOMPtr<nsIDocument> doc = aImageContent->GetDocument();
  if (doc) {
    nsCOMPtr<nsIDOMHTMLMapElement> imageMap = nsImageMapUtils::FindImageMap(doc, useMap);
    if (!imageMap)
      return nsnull;
    nsCOMPtr<nsIContent> mapContent = do_QueryInterface(imageMap);
    PRUint32 count = mapContent->GetChildCount();
    
    PRInt32 index = mapContent->IndexOf(mCurrentFocus);
    PRInt32 tabIndex;
    if (index < 0 || (mCurrentFocus->IsFocusable(&tabIndex) &&
                      tabIndex != mCurrentTabIndex)) {
      
      
      
      
      index = aForward ? -1 : (PRInt32)count;
    }

    
    nsCOMPtr<nsIContent> areaContent;
    while ((areaContent = mapContent->GetChildAt(aForward? ++index : --index)) != nsnull) {
      if (areaContent->IsFocusable(&tabIndex) && tabIndex == mCurrentTabIndex) {
        return areaContent;
      }
    }
  }

  return nsnull;
}

PRInt32
nsEventStateManager::GetNextTabIndex(nsIContent* aParent, PRBool forward)
{
  PRInt32 tabIndex, childTabIndex;
  nsIContent *child;

  PRUint32 count = aParent->GetChildCount();

  if (forward) {
    tabIndex = 0;
    for (PRUint32 index = 0; index < count; index++) {
      child = aParent->GetChildAt(index);
      childTabIndex = GetNextTabIndex(child, forward);
      if (childTabIndex > mCurrentTabIndex && childTabIndex != tabIndex) {
        tabIndex = (tabIndex == 0 || childTabIndex < tabIndex) ? childTabIndex : tabIndex;
      }

      nsAutoString tabIndexStr;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_SUCCEEDED (ec) && val > mCurrentTabIndex && val != tabIndex) {
        tabIndex = (tabIndex == 0 || val < tabIndex) ? val : tabIndex;
      }
    }
  }
  else { 
    tabIndex = 1;
    for (PRUint32 index = 0; index < count; index++) {
      child = aParent->GetChildAt(index);
      childTabIndex = GetNextTabIndex(child, forward);
      if ((mCurrentTabIndex == 0 && childTabIndex > tabIndex) ||
          (childTabIndex < mCurrentTabIndex && childTabIndex > tabIndex)) {
        tabIndex = childTabIndex;
      }

      nsAutoString tabIndexStr;
      child->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
      PRInt32 ec, val = tabIndexStr.ToInteger(&ec);
      if (NS_SUCCEEDED (ec)) {
        if ((mCurrentTabIndex == 0 && val > tabIndex) ||
            (val < mCurrentTabIndex && val > tabIndex) ) {
          tabIndex = val;
        }
      }
    }
  }
  return tabIndex;
}

NS_IMETHODIMP
nsEventStateManager::GetEventTarget(nsIFrame **aFrame)
{
  if (!mCurrentTarget && mCurrentTargetContent) {
    if (mPresContext) {
      nsIPresShell *shell = mPresContext->GetPresShell();
      if (shell) {
        mCurrentTarget = shell->GetPrimaryFrameFor(mCurrentTargetContent);
      }
    }
  }

  if (!mCurrentTarget) {
    nsIPresShell *presShell = mPresContext->GetPresShell();
    if (presShell) {
      nsIFrame* frame = nsnull;
      presShell->GetEventTargetFrame(&frame);
      mCurrentTarget = frame;
    }
  }

  *aFrame = mCurrentTarget;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetEventTargetContent(nsEvent* aEvent,
                                           nsIContent** aContent)
{
  if (aEvent &&
      (aEvent->message == NS_FOCUS_CONTENT ||
       aEvent->message == NS_BLUR_CONTENT)) {
    *aContent = mCurrentFocus;
    NS_IF_ADDREF(*aContent);
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

  if (aContent == mCurrentFocus) {
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

  PRBool didContentChangeAllStates = PR_TRUE;

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
    EnsureDocument(mPresContext);
    nsIMEStateManager::OnChangeFocus(mPresContext, aContent);
    if (aContent && (aContent == mCurrentFocus) && gLastFocusedDocument == mDocument) {
      
      
      NS_IF_RELEASE(gLastFocusedContent);
      gLastFocusedContent = mCurrentFocus;
      NS_IF_ADDREF(gLastFocusedContent);
      
      
      if (!(aState & ~NS_EVENT_STATE_FOCUS)) {
        aContent = nsnull;
      }
    } else {
      
      PRBool fcActive = PR_FALSE;
      if (mDocument) {
        nsIFocusController *fc = GetFocusControllerForDocument(mDocument);
        if (fc)
          fc->GetActive(&fcActive);
      }
      notifyContent[2] = gLastFocusedContent;
      NS_IF_ADDREF(gLastFocusedContent);
      
      SendFocusBlur(mPresContext, aContent, fcActive);
      if (mCurrentFocus != aContent) {
        didContentChangeAllStates = PR_FALSE;
      }

#ifdef DEBUG_aleventhal
      nsPIDOMWindow *currentWindow = mDocument->GetWindow();
      if (currentWindow) {
        nsIFocusController *fc = currentWindow->GetRootFocusController();
        if (fc) {
          nsCOMPtr<nsIDOMElement> focusedElement;
          fc->GetFocusedElement(getter_AddRefs(focusedElement));
          if (!SameCOMIdentity(mCurrentFocus, focusedElement)) {
            printf("\n\nFocus out of whack!!!\n\n");
          }
        }
      }
#endif
      
      
      if (mDocument) {
        nsCOMPtr<nsIDocShell> docShell =
          do_QueryInterface(nsCOMPtr<nsISupports>(mDocument->GetContainer()));

        if (docShell && mCurrentFocus)
          docShell->SetCanvasHasFocus(PR_FALSE);
      }
    }
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

  return didContentChangeAllStates;
}

static PRBool
IsFocusable(nsIPresShell* aPresShell, nsIContent* aContent)
{
  
  aPresShell->FlushPendingNotifications(Flush_Frames);

  nsIFrame* focusFrame = aPresShell->GetPrimaryFrameFor(aContent);
  if (!focusFrame) {
    return PR_FALSE;
  }

  
  
  
  
  
  if (aContent->IsNodeOfType(nsINode::eXUL)) {
    
    
    
    return focusFrame->AreAncestorViewsVisible();
  }

  if (aContent->Tag() != nsGkAtoms::area) {
    return focusFrame->IsFocusable();
  }
  
  
  
  return focusFrame->AreAncestorViewsVisible() &&
         focusFrame->GetStyleVisibility()->IsVisible() &&
         aContent->IsFocusable();
}

nsresult
nsEventStateManager::SendFocusBlur(nsPresContext* aPresContext,
                                   nsIContent *aContent,
                                   PRBool aEnsureWindowHasFocus)
{
  
  
  nsCOMPtr<nsIPresShell> presShell = aPresContext->GetPresShell();
  if (!presShell)
    return NS_OK;

  nsCOMPtr<nsIContent> previousFocus = mCurrentFocus;

  
  
  

  if (previousFocus && !previousFocus->GetDocument())
    previousFocus = nsnull;

  
  nsFocusSuppressor oldFocusSuppressor;
  
  nsIMEStateManager::OnTextStateBlur(aPresContext, aContent);

  if (nsnull != gLastFocusedPresContextWeak) {

    nsCOMPtr<nsIContent> focusAfterBlur;

    if (gLastFocusedContent && gLastFocusedContent != mFirstBlurEvent) {

      
      
      PRBool clearFirstBlurEvent = PR_FALSE;
      if (!mFirstBlurEvent) {
        mFirstBlurEvent = gLastFocusedContent;
        clearFirstBlurEvent = PR_TRUE;
      }

      
      
      nsCOMPtr<nsIDocument> doc = gLastFocusedContent->GetDocument();
      if (doc) {
        
        
        
        
        nsCOMPtr<nsIViewManager> kungFuDeathGrip;
        nsIPresShell *shell = doc->GetPrimaryShell();
        if (shell) {
          kungFuDeathGrip = shell->GetViewManager();

          nsCOMPtr<nsPresContext> oldPresContext = shell->GetPresContext();

          
          nsEvent blurEvent(PR_TRUE, NS_BLUR_CONTENT);
          blurEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

          EnsureDocument(presShell);

          
          
          if(gLastFocusedDocument && mDocument) {
            nsPIDOMWindow *newWindow = mDocument->GetWindow();
            if (newWindow) {
              nsIFocusController *newFocusController =
                newWindow->GetRootFocusController();
              nsPIDOMWindow *oldWindow = gLastFocusedDocument->GetWindow();
              if (oldWindow) {
                nsIFocusController *suppressed =
                  oldWindow->GetRootFocusController();

                if (suppressed != newFocusController) {
                  oldFocusSuppressor.Suppress(suppressed, "SendFocusBlur Window Switch #1");
                }
              }
            }
          }

          nsCOMPtr<nsIEventStateManager> esm;
          esm = oldPresContext->EventStateManager();
          esm->SetFocusedContent(gLastFocusedContent);
          nsCOMPtr<nsIContent> temp = gLastFocusedContent;
          NS_RELEASE(gLastFocusedContent); 

          nsCxPusher pusher;
          if (pusher.Push(temp)) {
            FireFocusOrBlurEvent(temp, &blurEvent, oldPresContext);
            pusher.Pop();
          }

          focusAfterBlur = mCurrentFocus;
          if (!previousFocus || previousFocus == focusAfterBlur)
            esm->SetFocusedContent(nsnull);
        }
      }

      if (clearFirstBlurEvent) {
        mFirstBlurEvent = nsnull;
      }

      if (previousFocus && previousFocus != focusAfterBlur) {
        
        
        EnsureFocusSynchronization();
        return NS_OK;
      }
    }

    
    nsCOMPtr<nsPIDOMWindow> window;

    if(gLastFocusedDocument)
      window = gLastFocusedDocument->GetWindow();

    EnsureDocument(presShell);

    if (gLastFocusedDocument && (gLastFocusedDocument != mDocument) &&
        window) {
      nsEvent blurEvent(PR_TRUE, NS_BLUR_CONTENT);
      blurEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

      
      
      if (mDocument && !oldFocusSuppressor.Suppressing()) {
        nsCOMPtr<nsPIDOMWindow> newWindow(mDocument->GetWindow());

        if (newWindow) {
          nsCOMPtr<nsPIDOMWindow> oldWindow(gLastFocusedDocument->GetWindow());
          nsIFocusController *newFocusController =
            newWindow->GetRootFocusController();
          if (oldWindow) {
            nsIFocusController *suppressed =
              oldWindow->GetRootFocusController();
            if (suppressed != newFocusController) {
              oldFocusSuppressor.Suppress(suppressed, "SendFocusBlur Window Switch #2");
            }
          }
        }
      }

      gLastFocusedPresContextWeak->EventStateManager()->SetFocusedContent(nsnull);
      nsCOMPtr<nsIDocument> temp = gLastFocusedDocument;
      NS_RELEASE(gLastFocusedDocument);
      gLastFocusedDocument = nsnull;

      nsCxPusher pusher;
      if (pusher.Push(temp)) {
        FireFocusOrBlurEvent(temp, &blurEvent, gLastFocusedPresContextWeak);
        pusher.Pop();
      }

      if (previousFocus && mCurrentFocus != previousFocus) {
        
        
        
        
        EnsureFocusSynchronization();
        return NS_OK;
      }

      nsCOMPtr<nsPIDOMEventTarget> target = do_QueryInterface(window);
      if (pusher.Push(target)) {
        blurEvent.target = nsnull;
        FireFocusOrBlurEvent(target, &blurEvent, gLastFocusedPresContextWeak);
  
        if (previousFocus && mCurrentFocus != previousFocus) {
          
          
          EnsureFocusSynchronization();
          return NS_OK;
        }
      }
    }
  }

  
  
  if (aContent && !::IsFocusable(presShell, aContent)) {
    aContent = nsnull;
  }

  NS_IF_RELEASE(gLastFocusedContent);
  gLastFocusedContent = aContent;
  NS_IF_ADDREF(gLastFocusedContent);
  SetFocusedContent(aContent);
  EnsureFocusSynchronization();

  
  
  
  if (aEnsureWindowHasFocus) {
    nsCOMPtr<nsIWidget> widget;
    
    nsIFrame* currentFocusFrame = nsnull;
    if (mCurrentFocus)
      currentFocusFrame = presShell->GetPrimaryFrameFor(mCurrentFocus);
    if (!currentFocusFrame)
      currentFocusFrame = mCurrentTarget;
    nsIObjectFrame* objFrame = do_QueryFrame(currentFocusFrame);
    if (objFrame) {
      nsIView* view = currentFocusFrame->GetViewExternal();
      NS_ASSERTION(view, "Object frames must have views");
      widget = view->GetWidget();
    }
    if (!widget) {
      
      
      
      nsIViewManager* vm = presShell->GetViewManager();
      if (vm) {
        vm->GetWidget(getter_AddRefs(widget));
      }
    }
    if (widget)
      widget->SetFocus(PR_TRUE);
  }

  if (nsnull != aContent && aContent != mFirstFocusEvent &&
      !EventHandlingSuppressed(aContent)) {

    
    
    PRBool clearFirstFocusEvent = PR_FALSE;
    if (!mFirstFocusEvent) {
      mFirstFocusEvent = aContent;
      clearFirstFocusEvent = PR_TRUE;
    }

    
    nsEvent focusEvent(PR_TRUE, NS_FOCUS_CONTENT);
    focusEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

    if (nsnull != mPresContext) {
      nsCxPusher pusher;
      if (pusher.Push(aContent)) {
        FireFocusOrBlurEvent(aContent, &focusEvent, mPresContext);
      }
    }

    nsAutoString tabIndex;
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndex);
    PRInt32 ec, val = tabIndex.ToInteger(&ec);
    if (NS_SUCCEEDED (ec)) {
      mCurrentTabIndex = val;
    }

    if (clearFirstFocusEvent) {
      mFirstFocusEvent = nsnull;
    }

    nsIMEStateManager::OnTextStateFocus(mPresContext, mCurrentFocus);
  } else if (!aContent && !EventHandlingSuppressed(mDocument)) {
    
    
    nsEvent focusEvent(PR_TRUE, NS_FOCUS_CONTENT);
    focusEvent.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

    if (nsnull != mPresContext && mDocument) {
      nsCxPusher pusher;
      if (pusher.Push(mDocument)) {
        FireFocusOrBlurEvent(mDocument, &focusEvent, mPresContext);
      }
    }
  }

  if (mBrowseWithCaret || GetWindowShowCaret(mDocument))
    SetContentCaretVisible(presShell, aContent, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetFocusedContent(nsIContent** aContent)
{
  *aContent = mCurrentFocus;
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

void nsEventStateManager::EnsureFocusSynchronization()
{
  
  
  
  
  
  
  
  
  nsPIDOMWindow *currentWindow = mDocument->GetWindow();
  if (currentWindow) {
    nsIFocusController *fc = currentWindow->GetRootFocusController();
    if (fc) {
      nsCOMPtr<nsIDOMElement> focusedElement = do_QueryInterface(mCurrentFocus);
      fc->SetFocusedElement(focusedElement);
    }
  }
}

NS_IMETHODIMP
nsEventStateManager::SetFocusedContent(nsIContent* aContent)
{

  if (aContent &&
      (!mPresContext || mPresContext->Type() == nsPresContext::eContext_PrintPreview)) {
    return NS_OK;
  }

  mCurrentFocus = aContent;
  if (mCurrentFocus)
    mLastFocus = mCurrentFocus;
  mCurrentFocusFrame = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetLastFocusedContent(nsIContent** aContent)
{
  *aContent = mLastFocus;
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::GetFocusedFrame(nsIFrame** aFrame)
{
  if (!mCurrentFocusFrame && mCurrentFocus) {
    nsIDocument* doc = mCurrentFocus->GetDocument();
    if (doc) {
      nsIPresShell *shell = doc->GetPrimaryShell();
      if (shell) {
        mCurrentFocusFrame = shell->GetPrimaryFrameFor(mCurrentFocus);
      }
    }
  }

  *aFrame = mCurrentFocusFrame;
  return NS_OK;
}

NS_IMETHODIMP
nsEventStateManager::ContentRemoved(nsIContent* aContent)
{
  if (mCurrentFocus &&
      nsContentUtils::ContentIsDescendantOf(mCurrentFocus, aContent)) {
    
    
    
    nsIMEStateManager::OnRemoveContent(mPresContext, mCurrentFocus);
    SetFocusedContent(nsnull);
  }

  if (mLastFocus &&
      nsContentUtils::ContentIsDescendantOf(mLastFocus, aContent)) {
    mLastFocus = nsnull;
  }

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
nsEventStateManager::EnsureDocument(nsIPresShell* aPresShell)
{
  if (!mDocument && aPresShell)
    mDocument = aPresShell->GetDocument();
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

nsresult
nsEventStateManager::GetDocSelectionLocation(nsIContent **aStartContent,
                                             nsIContent **aEndContent,
                                             nsIFrame **aStartFrame,
                                             PRUint32* aStartOffset)
{
  
  

  *aStartOffset = 0;
  *aStartFrame = nsnull;
  *aStartContent = *aEndContent = nsnull;
  nsresult rv = NS_ERROR_FAILURE;

  NS_ASSERTION(mPresContext, "mPresContent is null!!");
  EnsureDocument(mPresContext);
  if (!mDocument)
    return rv;
  nsIPresShell *shell;
  shell = mPresContext->GetPresShell();

  nsCOMPtr<nsFrameSelection> frameSelection;
  if (shell)
    frameSelection = shell->FrameSelection();

  nsCOMPtr<nsISelection> domSelection;
  if (frameSelection) {
    domSelection = frameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
  }

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRBool isCollapsed = PR_FALSE;
  nsCOMPtr<nsIContent> startContent, endContent;
  if (domSelection) {
    domSelection->GetIsCollapsed(&isCollapsed);
    nsCOMPtr<nsIDOMRange> domRange;
    rv = domSelection->GetRangeAt(0, getter_AddRefs(domRange));
    if (domRange) {
      domRange->GetStartContainer(getter_AddRefs(startNode));
      domRange->GetEndContainer(getter_AddRefs(endNode));
      domRange->GetStartOffset(reinterpret_cast<PRInt32 *>(aStartOffset));

      nsIContent *childContent = nsnull;

      startContent = do_QueryInterface(startNode);
      if (startContent && startContent->IsNodeOfType(nsINode::eELEMENT)) {
        NS_ASSERTION(*aStartOffset >= 0, "Start offset cannot be negative");  
        childContent = startContent->GetChildAt(*aStartOffset);
        if (childContent) {
          startContent = childContent;
        }
      }

      endContent = do_QueryInterface(endNode);
      if (endContent && endContent->IsNodeOfType(nsINode::eELEMENT)) {
        PRInt32 endOffset = 0;
        domRange->GetEndOffset(&endOffset);
        NS_ASSERTION(endOffset >= 0, "End offset cannot be negative");
        childContent = endContent->GetChildAt(endOffset);
        if (childContent) {
          endContent = childContent;
        }
      }
    }
  }
  else {
    rv = NS_ERROR_INVALID_ARG;
  }

  nsIFrame *startFrame = nsnull;
  if (startContent) {
    startFrame = shell->GetPrimaryFrameFor(startContent);
    if (isCollapsed) {
      
      
      
      

      nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(startContent));
      PRUint16 nodeType;
      domNode->GetNodeType(&nodeType);

      if (nodeType == nsIDOMNode::TEXT_NODE) {
        nsAutoString nodeValue;
        domNode->GetNodeValue(nodeValue);

        PRBool isFormControl =
          startContent->IsNodeOfType(nsINode::eHTML_FORM_CONTROL);

        if (nodeValue.Length() == *aStartOffset && !isFormControl &&
            startContent != mDocument->GetRootContent()) {
          
          nsCOMPtr<nsIFrameEnumerator> frameTraversal;

          nsCOMPtr<nsIFrameTraversal> trav(do_CreateInstance(kFrameTraversalCID,
                                                             &rv));
          NS_ENSURE_SUCCESS(rv, rv);

          rv = trav->NewFrameTraversal(getter_AddRefs(frameTraversal),
                                       mPresContext, startFrame,
                                       eLeaf,
                                       PR_FALSE, 
                                       PR_FALSE, 
                                       PR_FALSE  
                                       );
          NS_ENSURE_SUCCESS(rv, rv);

          nsIFrame *newCaretFrame = nsnull;
          nsCOMPtr<nsIContent> newCaretContent = startContent;
          PRBool endOfSelectionInStartNode(startContent == endContent);
          do {
            
            
            frameTraversal->Next();
            newCaretFrame = frameTraversal->CurrentItem();
            if (nsnull == newCaretFrame) {
              break;
            }
            newCaretContent = newCaretFrame->GetContent();            
          } while (!newCaretContent || newCaretContent == startContent);

          if (newCaretFrame && newCaretContent) {
            
            
            nsRefPtr<nsCaret> caret;
            shell->GetCaret(getter_AddRefs(caret));
            nsRect caretRect;
            nsIView *caretView;
            caret->GetCaretCoordinates(nsCaret::eClosestViewCoordinates, 
                                       domSelection, &caretRect,
                                       &isCollapsed, &caretView);
            nsPoint framePt;
            nsIView *frameClosestView = newCaretFrame->GetClosestView(&framePt);
            if (caretView == frameClosestView && caretRect.y == framePt.y &&
                caretRect.x == framePt.x) {
              
              startFrame = newCaretFrame;
              startContent = newCaretContent;
              if (endOfSelectionInStartNode) {
                endContent = newCaretContent; 
              }
            }
          }
        }
      }
    }
  }

  *aStartFrame = startFrame;
  *aStartContent = startContent;
  *aEndContent = endContent;
  NS_IF_ADDREF(*aStartContent);
  NS_IF_ADDREF(*aEndContent);

  return rv;
}

void
nsEventStateManager::FocusElementButNotDocument(nsIContent *aContent)
{
  

  if (gLastFocusedDocument == mDocument) {
    
    
    if (mCurrentFocus != aContent) {
      if (aContent)
        aContent->SetFocus(mPresContext);
      else
        SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
    }
    return;
  }

  




  nsIFocusController *focusController =
    GetFocusControllerForDocument(mDocument);
  if (!focusController)
      return;

  
  nsCOMPtr<nsIDOMElement> oldFocusedElement;
  focusController->GetFocusedElement(getter_AddRefs(oldFocusedElement));
  nsCOMPtr<nsIContent> oldFocusedContent(do_QueryInterface(oldFocusedElement));

  
  
  SetFocusedContent(aContent);  
  mDocument->BeginUpdate(UPDATE_CONTENT_STATE);
  mDocument->ContentStatesChanged(oldFocusedContent, aContent,
                                  NS_EVENT_STATE_FOCUS);
  mDocument->EndUpdate(UPDATE_CONTENT_STATE);

  
  
  
  SetFocusedContent(nsnull);

}

NS_IMETHODIMP
nsEventStateManager::MoveFocusToCaret(PRBool aCanFocusDoc,
                                      PRBool *aIsSelectionWithFocus)
{
  
  
  

  

  *aIsSelectionWithFocus= PR_FALSE;
  nsCOMPtr<nsIContent> selectionContent, endSelectionContent;
  nsIFrame *selectionFrame;
  PRUint32 selectionOffset;
  GetDocSelectionLocation(getter_AddRefs(selectionContent), getter_AddRefs(endSelectionContent),
    &selectionFrame, &selectionOffset);

  if (!selectionContent)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> testContent(selectionContent);
  nsCOMPtr<nsIContent> nextTestContent(endSelectionContent);

  
  

  
  
  
  
  while (testContent) {
    
    

    if (testContent == mCurrentFocus) {
      *aIsSelectionWithFocus = PR_TRUE;
      return NS_OK;  
    }

    nsIAtom *tag = testContent->Tag();

    
    if (tag == nsGkAtoms::a &&
        testContent->IsNodeOfType(nsINode::eHTML)) {
      *aIsSelectionWithFocus = PR_TRUE;
    }
    else {
      
      *aIsSelectionWithFocus =
        testContent->HasAttr(kNameSpaceID_XLink, nsGkAtoms::href) &&
        testContent->AttrValueIs(kNameSpaceID_XLink, nsGkAtoms::type,
                                 nsGkAtoms::simple, eCaseMatters);
    }

    if (*aIsSelectionWithFocus) {
      FocusElementButNotDocument(testContent);
      return NS_OK;
    }

    
    testContent = testContent->GetParent();

    if (!testContent) {
      
      testContent = nextTestContent;
      nextTestContent = nsnull;
    }
  }

  
  

  
  nsCOMPtr<nsIDOMNode> selectionNode(do_QueryInterface(selectionContent));
  nsCOMPtr<nsIDOMNode> endSelectionNode(do_QueryInterface(endSelectionContent));
  nsCOMPtr<nsIDOMNode> testNode;

  do {
    testContent = do_QueryInterface(selectionNode);

    
    
    
    
    if (testContent) {
      if (testContent->Tag() == nsGkAtoms::a &&
          testContent->IsNodeOfType(nsINode::eHTML)) {
        *aIsSelectionWithFocus = PR_TRUE;
        FocusElementButNotDocument(testContent);
        return NS_OK;
      }
    }

    selectionNode->GetFirstChild(getter_AddRefs(testNode));
    if (testNode) {
      selectionNode = testNode;
      continue;
    }

    if (selectionNode == endSelectionNode)
      break;
    selectionNode->GetNextSibling(getter_AddRefs(testNode));
    if (testNode) {
      selectionNode = testNode;
      continue;
    }

    do {
      selectionNode->GetParentNode(getter_AddRefs(testNode));
      if (!testNode || testNode == endSelectionNode) {
        selectionNode = nsnull;
        break;
      }
      testNode->GetNextSibling(getter_AddRefs(selectionNode));
      if (selectionNode)
        break;
      selectionNode = testNode;
    } while (PR_TRUE);
  }
  while (selectionNode && selectionNode != endSelectionNode);

  if (aCanFocusDoc)
    FocusElementButNotDocument(nsnull);

  return NS_OK; 
}



NS_IMETHODIMP
nsEventStateManager::MoveCaretToFocus()
{
  
  

  PRInt32 itemType = nsIDocShellTreeItem::typeChrome;

  if (mPresContext) {
    nsCOMPtr<nsISupports> pcContainer = mPresContext->GetContainer();
    nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(pcContainer));
    if (treeItem)
      treeItem->GetItemType(&itemType);
    nsCOMPtr<nsIEditorDocShell> editorDocShell(do_QueryInterface(treeItem));
    if (editorDocShell) {
      PRBool isEditable;
      editorDocShell->GetEditable(&isEditable);
      if (isEditable) {
        return NS_OK;  
      }
    }
  }

  if (itemType != nsIDocShellTreeItem::typeChrome) {
    nsIPresShell *shell = mPresContext->GetPresShell();
    if (shell) {
      
      nsCOMPtr<nsIDOMDocumentRange> rangeDoc(do_QueryInterface(mDocument));

      if (rangeDoc) {
        nsCOMPtr<nsFrameSelection> frameSelection = shell->FrameSelection();
        nsCOMPtr<nsISelection> domSelection = frameSelection->
          GetSelection(nsISelectionController::SELECTION_NORMAL);
        if (domSelection) {
          nsCOMPtr<nsIDOMNode> currentFocusNode(do_QueryInterface(mCurrentFocus));
          
          domSelection->RemoveAllRanges();
          if (currentFocusNode && !mCurrentFocus->IsNodeOfType(nsINode::eXUL)) {
            nsCOMPtr<nsIDOMRange> newRange;
            nsresult rv = rangeDoc->CreateRange(getter_AddRefs(newRange));
            if (NS_SUCCEEDED(rv)) {
              
              
              newRange->SelectNodeContents(currentFocusNode);
              nsCOMPtr<nsIDOMNode> firstChild;
              currentFocusNode->GetFirstChild(getter_AddRefs(firstChild));
              if (!firstChild ||
                  mCurrentFocus->IsNodeOfType(nsINode::eHTML_FORM_CONTROL)) {
                
                
                
                newRange->SetStartBefore(currentFocusNode);
                newRange->SetEndBefore(currentFocusNode);
              }
              domSelection->AddRange(newRange);
              domSelection->CollapseToStart();
            }
          }
        }
      }
    }
  }
  return NS_OK;
}

nsresult
nsEventStateManager::SetCaretEnabled(nsIPresShell *aPresShell, PRBool aEnabled)
{
  nsRefPtr<nsCaret> caret;
  aPresShell->GetCaret(getter_AddRefs(caret));

  nsCOMPtr<nsISelectionController> selCon(do_QueryInterface(aPresShell));
  if (!selCon || !caret)
    return NS_ERROR_FAILURE;

  selCon->SetCaretEnabled(aEnabled);
  caret->SetCaretVisible(aEnabled);
  caret->SetIgnoreUserModify(aEnabled);

  return NS_OK;
}

nsresult
nsEventStateManager::SetContentCaretVisible(nsIPresShell* aPresShell,
                                            nsIContent *aFocusedContent,
                                            PRBool aVisible)
{
  
  nsRefPtr<nsCaret> caret;
  aPresShell->GetCaret(getter_AddRefs(caret));

  nsCOMPtr<nsFrameSelection> frameSelection;
  if (aFocusedContent) {
    nsIFrame *focusFrame = aPresShell->GetPrimaryFrameFor(aFocusedContent);
    
    if (focusFrame)
      frameSelection = focusFrame->GetFrameSelection();
  }

  nsCOMPtr<nsFrameSelection> docFrameSelection = aPresShell->FrameSelection();

  if (docFrameSelection && caret &&
     (frameSelection == docFrameSelection || !aFocusedContent)) {
    nsISelection* domSelection = docFrameSelection->
      GetSelection(nsISelectionController::SELECTION_NORMAL);
    if (domSelection) {
      
      caret->SetCaretDOMSelection(domSelection);

      
      
      

      
      return SetCaretEnabled(aPresShell, aVisible);
    }
  }

  return NS_OK;
}

PRBool
nsEventStateManager::GetBrowseWithCaret()
{
  return mBrowseWithCaret;
}



static PRBool
GetWindowShowCaret(nsIDocument *aDocument)
{
  if (!aDocument) return PR_FALSE;

  nsPIDOMWindow* window = aDocument->GetWindow();
  if (!window) return PR_FALSE;

  nsCOMPtr<nsIContent> docContent =
    do_QueryInterface(window->GetFrameElementInternal());
  if (!docContent) return PR_FALSE;

  return docContent->AttrValueIs(kNameSpaceID_None,
                                 nsGkAtoms::showcaret,
                                 NS_LITERAL_STRING("true"),
                                 eCaseMatters);
}

void
nsEventStateManager::ResetBrowseWithCaret()
{
  
  

  if (!mPresContext)
    return;

  nsCOMPtr<nsISupports> pcContainer = mPresContext->GetContainer();
  PRInt32 itemType;
  nsCOMPtr<nsIDocShellTreeItem> shellItem(do_QueryInterface(pcContainer));
  if (!shellItem)
    return;

  shellItem->GetItemType(&itemType);

  if (itemType == nsIDocShellTreeItem::typeChrome)
    return;  

  PRPackedBool browseWithCaret =
    nsContentUtils::GetBoolPref("accessibility.browsewithcaret");
  mBrowseWithCaret = browseWithCaret;

  nsIPresShell *presShell = mPresContext->GetPresShell();

  
  
  
  nsCOMPtr<nsIEditorDocShell> editorDocShell(do_QueryInterface(shellItem));
  if (editorDocShell) {
    PRBool isEditable;
    editorDocShell->GetEditable(&isEditable);
    if (presShell && isEditable) {
      nsCOMPtr<nsIHTMLDocument> doc =
        do_QueryInterface(presShell->GetDocument());

      PRBool isContentEditableDoc =
        doc && doc->GetEditingState() == nsIHTMLDocument::eContentEditable;

      PRBool isFocusEditable =
        mCurrentFocus && mCurrentFocus->HasFlag(NODE_IS_EDITABLE);

      if (!isContentEditableDoc || isFocusEditable)
        return;
    }
  }

  
  
  
  if (presShell && gLastFocusedDocument && gLastFocusedDocument == mDocument) {

    PRBool caretShouldBeVisible = browseWithCaret ||
                                  GetWindowShowCaret(mDocument);

    SetContentCaretVisible(presShell, mCurrentFocus, caretShouldBeVisible);
  }
}







PRBool
nsEventStateManager::IsFrameSetDoc(nsIDocShell* aDocShell)
{
  NS_ASSERTION(aDocShell, "docshell is null");
  PRBool isFrameSet = PR_FALSE;

  
  
  nsCOMPtr<nsIPresShell> presShell;
  aDocShell->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    nsIDocument *doc = presShell->GetDocument();
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(doc);
    if (htmlDoc) {
      nsIContent *rootContent = doc->GetRootContent();
      if (rootContent) {
        PRUint32 childCount = rootContent->GetChildCount();
        for (PRUint32 i = 0; i < childCount; ++i) {
          nsIContent *childContent = rootContent->GetChildAt(i);

          nsINodeInfo *ni = childContent->NodeInfo();

          if (childContent->IsNodeOfType(nsINode::eHTML) &&
              ni->Equals(nsGkAtoms::frameset)) {
            isFrameSet = PR_TRUE;
            break;
          }
        }
      }
    }
  }

  return isFrameSet;
}




PRBool
nsEventStateManager::IsIFrameDoc(nsIDocShell* aDocShell)
{
  NS_ASSERTION(aDocShell, "docshell is null");

  nsCOMPtr<nsPIDOMWindow> domWindow(do_GetInterface(aDocShell));
  if (!domWindow) {
    NS_ERROR("We're a child of a docshell without a window?");
    return PR_FALSE;
  }

  nsCOMPtr<nsIContent> docContent =
    do_QueryInterface(domWindow->GetFrameElementInternal());

  if (!docContent) {
    return PR_FALSE;
  }

  return docContent->Tag() == nsGkAtoms::iframe;
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








void
nsEventStateManager::TabIntoDocument(nsIDocShell* aDocShell,
                                     PRBool aForward)
{
  NS_ASSERTION(aDocShell, "null docshell");
  nsCOMPtr<nsIDOMWindowInternal> domwin = do_GetInterface(aDocShell);
  if (domwin)
    domwin->Focus();

  PRInt32 itemType;
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(aDocShell);
  treeItem->GetItemType(&itemType);

  nsCOMPtr<nsPresContext> presContext;
  aDocShell->GetPresContext(getter_AddRefs(presContext));
  PRBool focusDocument;
  if (presContext &&
      presContext->Type() == nsPresContext::eContext_PrintPreview) {
    
    focusDocument = PR_TRUE;
  } else {
    if (!aForward || (itemType == nsIDocShellTreeItem::typeChrome))
      focusDocument = PR_FALSE;
    else {
      
      focusDocument = !(IsFrameSetDoc(aDocShell));
    }
  }

  if (focusDocument) {
    
    aDocShell->SetCanvasHasFocus(PR_TRUE);
  }
  else {
    aDocShell->SetHasFocus(PR_FALSE);

    if (presContext) {
      nsIEventStateManager *docESM = presContext->EventStateManager();

      
      
      mTabbingFromDocShells.AppendObject(aDocShell);

      
      docESM->SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
      
      docESM->ShiftFocus(aForward, nsnull);

      
      mTabbingFromDocShells.RemoveObject(aDocShell);
    }
  }
}

void
nsEventStateManager::GetLastChildDocShell(nsIDocShellTreeItem* aItem,
                                          nsIDocShellTreeItem** aResult)
{
  NS_ASSERTION(aItem, "null docshell");
  NS_ASSERTION(aResult, "null out pointer");

  nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(aItem);
  while (1) {
    nsCOMPtr<nsIDocShellTreeNode> curNode = do_QueryInterface(curItem);
    PRInt32 childCount = 0;
    curNode->GetChildCount(&childCount);
    if (!childCount) {
      *aResult = curItem;
      NS_ADDREF(*aResult);
      return;
    }

    curNode->GetChildAt(childCount - 1, getter_AddRefs(curItem));
  }
}

void
nsEventStateManager::GetNextDocShell(nsIDocShellTreeNode* aNode,
                                     nsIDocShellTreeItem** aResult)
{
  NS_ASSERTION(aNode, "null docshell");
  NS_ASSERTION(aResult, "null out pointer");

  *aResult = nsnull;

  PRInt32 childCount = 0;
  aNode->GetChildCount(&childCount);
  if (childCount) {
    aNode->GetChildAt(0, aResult);
    if (*aResult)
      return;
  }

  nsCOMPtr<nsIDocShellTreeNode> curNode = aNode;
  while (curNode) {
    nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(curNode);
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    curItem->GetParent(getter_AddRefs(parentItem));
    if (!parentItem) {
      *aResult = nsnull;
      return;
    }

    
    
    nsCOMPtr<nsIDocShellTreeNode> parentNode = do_QueryInterface(parentItem);
    nsCOMPtr<nsIDocShellTreeItem> iterItem;
    childCount = 0;
    parentNode->GetChildCount(&childCount);
    for (PRInt32 index = 0; index < childCount; ++index) {
      parentNode->GetChildAt(index, getter_AddRefs(iterItem));
      if (iterItem == curItem) {
        ++index;
        if (index < childCount) {
          parentNode->GetChildAt(index, aResult);
          if (*aResult)
            return;
        }
        break;
      }
    }

    curNode = do_QueryInterface(parentItem);
  }
}

void
nsEventStateManager::GetPrevDocShell(nsIDocShellTreeNode* aNode,
                                     nsIDocShellTreeItem** aResult)
{
  NS_ASSERTION(aNode, "null docshell");
  NS_ASSERTION(aResult, "null out pointer");

  nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(aNode);
  nsCOMPtr<nsIDocShellTreeItem> parentItem;

  curItem->GetParent(getter_AddRefs(parentItem));
  if (!parentItem) {
    *aResult = nsnull;
    return;
  }

  
  
  nsCOMPtr<nsIDocShellTreeNode> parentNode = do_QueryInterface(parentItem);
  PRInt32 childCount = 0;
  parentNode->GetChildCount(&childCount);
  nsCOMPtr<nsIDocShellTreeItem> prevItem, iterItem;
  for (PRInt32 index = 0; index < childCount; ++index) {
    parentNode->GetChildAt(index, getter_AddRefs(iterItem));
    if (iterItem == curItem)
      break;
    prevItem = iterItem;
  }

  if (prevItem) {
    curItem = prevItem;
    nsCOMPtr<nsIDocShellTreeNode> curNode;
    
    while (1) {
      curNode = do_QueryInterface(curItem);
      childCount = 0;
      curNode->GetChildCount(&childCount);
      if (!childCount)
        break;

      curNode->GetChildAt(childCount - 1, getter_AddRefs(curItem));
    }

    *aResult = curItem;
    NS_ADDREF(*aResult);
    return;
  }

  *aResult = parentItem;
  NS_ADDREF(*aResult);
  return;
}





void
nsEventStateManager::ShiftFocusByDoc(PRBool aForward)
{
  
  
  
  

  NS_ASSERTION(mPresContext, "no prescontext");

  nsCOMPtr<nsISupports> pcContainer = mPresContext->GetContainer();
  nsCOMPtr<nsIDocShellTreeNode> curNode = do_QueryInterface(pcContainer);
  if (!curNode) {
    return;
  }

  
  

  nsCOMPtr<nsIDocShellTreeItem> nextItem;
  nsCOMPtr<nsIDocShell> nextShell;
  do {
    if (aForward) {
      GetNextDocShell(curNode, getter_AddRefs(nextItem));
      if (!nextItem) {
        nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(pcContainer);
        
        curItem->GetRootTreeItem(getter_AddRefs(nextItem));
      }
    }
    else {
      GetPrevDocShell(curNode, getter_AddRefs(nextItem));
      if (!nextItem) {
        nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(pcContainer);
        
        nsCOMPtr<nsIDocShellTreeItem> rootItem;
        curItem->GetRootTreeItem(getter_AddRefs(rootItem));
        GetLastChildDocShell(rootItem, getter_AddRefs(nextItem));
      }
    }

    curNode = do_QueryInterface(nextItem);
    nextShell = do_QueryInterface(nextItem);
  } while (IsFrameSetDoc(nextShell) || IsIFrameDoc(nextShell) || !IsShellVisible(nextShell));

  if (nextShell) {
    
    
    
    SetContentState(nsnull, NS_EVENT_STATE_FOCUS);
    TabIntoDocument(nextShell, PR_TRUE);
  }
}


nsIFocusController*
nsEventStateManager::GetFocusControllerForDocument(nsIDocument* aDocument)
{
  nsCOMPtr<nsISupports> container = aDocument->GetContainer();
  nsCOMPtr<nsPIDOMWindow> windowPrivate = do_GetInterface(container);

  return windowPrivate ? windowPrivate->GetRootFocusController() : nsnull;
}

