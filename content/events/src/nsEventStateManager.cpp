













































#include "mozilla/dom/TabParent.h"

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
#include "nsIDOMHTMLLabelElement.h"
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
#include "nsINameSpaceManager.h"
#include "nsIBaseWindow.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsISelection.h"
#include "nsFrameSelection.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
#include "nsIDOMEventTarget.h"
#include "nsIEnumerator.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIWebNavigation.h"
#include "nsIContentViewer.h"
#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif
#include "nsFrameManager.h"

#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"

#include "nsFocusManager.h"

#include "nsIDOMXULElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMKeyEvent.h"
#include "nsIObserverService.h"
#include "nsIDocShell.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIDOMMouseScrollEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSUIEvent.h"
#include "nsDOMDragEvent.h"
#include "nsIDOMNSEditableElement.h"

#include "nsIDOMRange.h"
#include "nsCaret.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"

#include "nsSubDocumentFrame.h"
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

#include "nsServiceManagerUtils.h"
#include "nsITimer.h"
#include "nsFontMetrics.h"
#include "nsIDOMXULDocument.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "nsDOMDataTransfer.h"
#include "nsContentAreaDragDrop.h"
#ifdef MOZ_XUL
#include "nsTreeBodyFrame.h"
#endif
#include "nsIController.h"
#include "nsICommandParams.h"
#include "mozilla/Services.h"
#include "mozAutoDocUpdate.h"
#include "nsHTMLLabelElement.h"

#include "mozilla/Preferences.h"

#ifdef XP_MACOSX
#import <ApplicationServices/ApplicationServices.h>
#endif

using namespace mozilla;
using namespace mozilla::dom;



#define NS_USER_INTERACTION_INTERVAL 5000 // ms

static NS_DEFINE_CID(kFrameTraversalCID, NS_FRAMETRAVERSAL_CID);

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

static PRBool sLeftClickOnly = PR_TRUE;
static PRBool sKeyCausesActivation = PR_TRUE;
static PRUint32 sESMInstanceCount = 0;
static PRInt32 sChromeAccessModifier = 0, sContentAccessModifier = 0;
PRInt32 nsEventStateManager::sUserInputEventDepth = 0;
PRBool nsEventStateManager::sNormalLMouseEventInProcess = PR_FALSE;
nsEventStateManager* nsEventStateManager::sActiveESM = nsnull;
nsIDocument* nsEventStateManager::sMouseOverDocument = nsnull;

static PRUint32 gMouseOrKeyboardEventCounter = 0;
static nsITimer* gUserInteractionTimer = nsnull;
static nsITimerCallback* gUserInteractionTimerCallback = nsnull;


static nscoord gPixelScrollDeltaX = 0;
static nscoord gPixelScrollDeltaY = 0;
static PRUint32 gPixelScrollDeltaTimeout = 0;

static nscoord
GetScrollableLineHeight(nsIFrame* aTargetFrame);

static inline PRBool
IsMouseEventReal(nsEvent* aEvent)
{
  NS_ABORT_IF_FALSE(NS_IS_MOUSE_EVENT_STRUCT(aEvent), "Not a mouse event");
  
  return static_cast<nsMouseEvent*>(aEvent)->reason == nsMouseEvent::eReal;
}

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
  nsRefPtr<nsPresContext> presContext;
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
  nsCOMPtr<nsIObserverService> obs =
    mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;
  if ((gMouseOrKeyboardEventCounter == mPreviousCount) || !aTimer) {
    gMouseOrKeyboardEventCounter = 0;
    obs->NotifyObservers(nsnull, "user-interaction-inactive", nsnull);
    if (gUserInteractionTimer) {
      gUserInteractionTimer->Cancel();
      NS_RELEASE(gUserInteractionTimer);
    }
  } else {
    obs->NotifyObservers(nsnull, "user-interaction-active", nsnull);
    nsEventStateManager::UpdateUserActivityTimer();
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
  PRInt32 accessKey = Preferences::GetInt("ui.key.generalAccessKey", -1);
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
    return Preferences::GetInt("ui.key.chromeAccess", 0);
  case nsIDocShellTreeItem::typeContent:
    return Preferences::GetInt("ui.key.contentAccess", 0);
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
  static PRInt32 AccelerateWheelDelta(PRInt32 aScrollLines,
                   PRBool aIsHorizontal, PRBool aAllowScrollSpeedOverride,
                   nsIScrollableFrame::ScrollUnit *aScrollQuantity,
                   PRBool aLimitToMaxOnePageScroll = PR_TRUE);
  static PRBool IsAccelerationEnabled();

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
  static PRInt32 OverrideSystemScrollSpeed(PRInt32 aScrollLines,
                                           PRBool aIsHorizontal);
  static PRInt32 ComputeAcceleratedWheelDelta(PRInt32 aDelta, PRInt32 aFactor);
  static PRInt32 LimitToOnePageScroll(PRInt32 aScrollLines,
                   PRBool aIsHorizontal,
                   nsIScrollableFrame::ScrollUnit *aScrollQuantity);

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
CanScrollInRange(nscoord aMin, nscoord aValue, nscoord aMax, PRInt32 aDirection)
{
  return aDirection > 0 ? aValue < aMax : aMin < aValue;
}

static PRBool
CanScrollOn(nsIScrollableFrame* aScrollFrame, PRInt32 aNumLines,
            PRBool aScrollHorizontal)
{
  NS_PRECONDITION(aScrollFrame, "aScrollFrame is null");
  NS_PRECONDITION(aNumLines, "aNumLines must be non-zero");
  nsPoint scrollPt = aScrollFrame->GetScrollPosition();
  nsRect scrollRange = aScrollFrame->GetScrollRange();

  return aScrollHorizontal
    ? CanScrollInRange(scrollRange.x, scrollPt.x, scrollRange.XMost(), aNumLines)
    : CanScrollInRange(scrollRange.y, scrollPt.y, scrollRange.YMost(), aNumLines);
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
  nsIScrollableFrame* sf = GetTargetFrame()->GetScrollTargetFrame();
  NS_ENSURE_TRUE(sf, PR_FALSE);

  if (!CanScrollOn(sf, aNumLines, aScrollHorizontal)) {
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

  PRInt32 message = aEvent->message;
  
  
  
  if (message == NS_QUERY_SCROLL_TARGET_INFO) {
    nsQueryContentEvent* queryEvent = static_cast<nsQueryContentEvent*>(aEvent);
    message = queryEvent->mInput.mMouseScrollEvent->message;
  }

  switch (message) {
    case NS_MOUSE_SCROLL:
    case NS_MOUSE_PIXEL_SCROLL:
      if (sMouseMoved != 0 &&
          OutOfTime(sMouseMoved, GetIgnoreMoveDelayTime())) {
        
        
        EndTransaction();
      }
      return;
    case NS_MOUSE_MOVE:
    case NS_DRAGDROP_OVER:
      if (IsMouseEventReal(aEvent)) {
        
        
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

  if (Preferences::GetBool("test.mousescroll", PR_FALSE)) {
    
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

  if (Preferences::GetBool("test.mousescroll", PR_FALSE)) {
    
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
#ifdef DEBUG
  nsresult rv =
#endif
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
  return Preferences::GetUint("mousewheel.transaction.timeout", 1500);
}

PRUint32
nsMouseWheelTransaction::GetIgnoreMoveDelayTime()
{
  return Preferences::GetUint("mousewheel.transaction.ignoremovedelay", 100);
}

PRBool
nsMouseWheelTransaction::IsAccelerationEnabled()
{
  return GetAccelerationStart() >= 0 && GetAccelerationFactor() > 0;
}

PRInt32
nsMouseWheelTransaction::AccelerateWheelDelta(PRInt32 aScrollLines,
                           PRBool aIsHorizontal,
                           PRBool aAllowScrollSpeedOverride,
                           nsIScrollableFrame::ScrollUnit *aScrollQuantity,
                           PRBool aLimitToMaxOnePageScroll)
{
  if (aAllowScrollSpeedOverride) {
    aScrollLines = OverrideSystemScrollSpeed(aScrollLines, aIsHorizontal);
  }

  
  PRInt32 start = GetAccelerationStart();
  if (start >= 0 && sScrollSeriesCounter >= start) {
    PRInt32 factor = GetAccelerationFactor();
    if (factor > 0) {
      aScrollLines = ComputeAcceleratedWheelDelta(aScrollLines, factor);
    }
  }

  
  
  return !aLimitToMaxOnePageScroll ? aScrollLines :
    LimitToOnePageScroll(aScrollLines, aIsHorizontal, aScrollQuantity);
}

PRInt32
nsMouseWheelTransaction::ComputeAcceleratedWheelDelta(PRInt32 aDelta,
                                                      PRInt32 aFactor)
{
  if (aDelta == 0)
    return 0;

  return PRInt32(NS_round(aDelta * sScrollSeriesCounter *
                          (double)aFactor / 10));
}

PRInt32
nsMouseWheelTransaction::GetAccelerationStart()
{
  return Preferences::GetInt("mousewheel.acceleration.start", -1);
}

PRInt32
nsMouseWheelTransaction::GetAccelerationFactor()
{
  return Preferences::GetInt("mousewheel.acceleration.factor", -1);
}

PRInt32
nsMouseWheelTransaction::OverrideSystemScrollSpeed(PRInt32 aScrollLines,
                                                   PRBool aIsHorizontal)
{
  NS_PRECONDITION(sTargetFrame, "We don't have mouse scrolling transaction");

  if (aScrollLines == 0) {
    return 0;
  }

  
  if (sTargetFrame !=
        sTargetFrame->PresContext()->PresShell()->GetRootScrollFrame()) {
    return aScrollLines;
  }

  
  
  
  
  nsCOMPtr<nsIWidget> widget(sTargetFrame->GetNearestWidget());
  NS_ENSURE_TRUE(widget, aScrollLines);
  PRInt32 overriddenDelta;
  nsresult rv = widget->OverrideSystemMouseScrollSpeed(aScrollLines,
                                                       aIsHorizontal,
                                                       overriddenDelta);
  NS_ENSURE_SUCCESS(rv, aScrollLines);
  return overriddenDelta;
}

PRInt32
nsMouseWheelTransaction::LimitToOnePageScroll(PRInt32 aScrollLines,
                           PRBool aIsHorizontal,
                           nsIScrollableFrame::ScrollUnit *aScrollQuantity)
{
  NS_ENSURE_TRUE(aScrollQuantity, aScrollLines);
  NS_PRECONDITION(*aScrollQuantity == nsIScrollableFrame::LINES,
                  "aScrollQuantity isn't by line");

  NS_ENSURE_TRUE(sTargetFrame, aScrollLines);
  nsIScrollableFrame* sf = sTargetFrame->GetScrollTargetFrame();
  NS_ENSURE_TRUE(sf, aScrollLines);

  
  
  nsSize lineAmount = sf->GetLineScrollAmount();
  nscoord lineScroll = aIsHorizontal ? lineAmount.width : lineAmount.height;

  if (lineScroll == 0)
    return aScrollLines;

  nsSize pageAmount = sf->GetPageScrollAmount();
  nscoord pageScroll = aIsHorizontal ? pageAmount.width : pageAmount.height;

  if (NS_ABS(aScrollLines) * lineScroll < pageScroll)
    return aScrollLines;

  nscoord maxLines = (pageScroll / lineScroll);
  if (maxLines >= 1)
    return ((aScrollLines < 0) ? -1 : 1) * maxLines;

  *aScrollQuantity = nsIScrollableFrame::PAGES;
  return (aScrollLines < 0) ? -1 : 1;
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
    m_haveShutdown(PR_FALSE),
    mLastLineScrollConsumedX(PR_FALSE),
    mLastLineScrollConsumedY(PR_FALSE),
    mClickHoldContextMenu(PR_FALSE)
{
  if (sESMInstanceCount == 0) {
    gUserInteractionTimerCallback = new nsUITimerCallback();
    if (gUserInteractionTimerCallback)
      NS_ADDREF(gUserInteractionTimerCallback);
    UpdateUserActivityTimer();
  }
  ++sESMInstanceCount;
}

nsresult
nsEventStateManager::UpdateUserActivityTimer(void)
{
  if (!gUserInteractionTimerCallback)
    return NS_OK;

  if (!gUserInteractionTimer)
    CallCreateInstance("@mozilla.org/timer;1", &gUserInteractionTimer);

  if (gUserInteractionTimer) {
    gUserInteractionTimer->InitWithCallback(gUserInteractionTimerCallback,
                                            NS_USER_INTERACTION_INTERVAL,
                                            nsITimer::TYPE_ONE_SHOT);
  }
  return NS_OK;
}

static const char* kObservedPrefs[] = {
  "accessibility.accesskeycausesactivation",
  "nglayout.events.dispatchLeftClickOnly",
  "ui.key.generalAccessKey",
  "ui.key.chromeAccess",
  "ui.key.contentAccess",
  "ui.click_hold_context_menus",
#if 0
  "mousewheel.withaltkey.action",
  "mousewheel.withaltkey.numlines",
  "mousewheel.withaltkey.sysnumlines",
  "mousewheel.withcontrolkey.action",
  "mousewheel.withcontrolkey.numlines",
  "mousewheel.withcontrolkey.sysnumlines",
  "mousewheel.withnokey.action",
  "mousewheel.withnokey.numlines",
  "mousewheel.withnokey.sysnumlines",
  "mousewheel.withshiftkey.action",
  "mousewheel.withshiftkey.numlines",
  "mousewheel.withshiftkey.sysnumlines",
#endif
  "dom.popup_allowed_events",
  nsnull
};

nsresult
nsEventStateManager::Init()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);

  if (sESMInstanceCount == 1) {
    sLeftClickOnly =
      Preferences::GetBool("nglayout.events.dispatchLeftClickOnly",
                           sLeftClickOnly);
    sChromeAccessModifier =
      GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeChrome);
    sContentAccessModifier =
      GetAccessModifierMaskFromPref(nsIDocShellTreeItem::typeContent);
  }
  Preferences::AddWeakObservers(this, kObservedPrefs);

  mClickHoldContextMenu =
    Preferences::GetBool("ui.click_hold_context_menus", PR_FALSE);

  return NS_OK;
}

nsEventStateManager::~nsEventStateManager()
{
  if (sActiveESM == this) {
    sActiveESM = nsnull;
  }
  if (mClickHoldContextMenu)
    KillClickHoldTimer();

  if (mDocument == sMouseOverDocument)
    sMouseOverDocument = nsnull;

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

    
    
    

    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (observerService) {
      observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    }
  }

}

nsresult
nsEventStateManager::Shutdown()
{
  Preferences::RemoveObservers(this, kObservedPrefs);
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
        Preferences::GetBool("accessibility.accesskeycausesactivation",
                             sKeyCausesActivation);
    } else if (data.EqualsLiteral("nglayout.events.dispatchLeftClickOnly")) {
      sLeftClickOnly =
        Preferences::GetBool("nglayout.events.dispatchLeftClickOnly",
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
    } else if (data.EqualsLiteral("ui.click_hold_context_menus")) {
      mClickHoldContextMenu =
        Preferences::GetBool("ui.click_hold_context_menus", PR_FALSE);
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
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsEventStateManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsEventStateManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsEventStateManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentTargetContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastMouseOverElement);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mGestureDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mGestureDownFrameOwner);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastLeftMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastLeftMouseDownContentParent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastMiddleMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastMiddleMouseDownContentParent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastRightMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mLastRightMouseDownContentParent);
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
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastLeftMouseDownContentParent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastMiddleMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastMiddleMouseDownContentParent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastRightMouseDownContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mLastRightMouseDownContentParent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mActiveContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mHoverContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDragOverContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mURLTargetContent);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstMouseOverEventElement);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFirstMouseOutEventElement);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mAccessKeys);
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

nsresult
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
        IsMouseEventReal(aEvent) &&
        aEvent->message != NS_MOUSE_ENTER &&
        aEvent->message != NS_MOUSE_EXIT) ||
       aEvent->eventStructType == NS_MOUSE_SCROLL_EVENT ||
       aEvent->eventStructType == NS_KEY_EVENT)) {
    if (gMouseOrKeyboardEventCounter == 0) {
      nsCOMPtr<nsIObserverService> obs =
        mozilla::services::GetObserverService();
      if (obs) {
        obs->NotifyObservers(nsnull, "user-interaction-active", nsnull);
        UpdateUserActivityTimer();
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
      BeginTrackingDragGesture(aPresContext, (nsMouseEvent*)aEvent, aTargetFrame);
#endif
      mLClickCount = ((nsMouseEvent*)aEvent)->clickCount;
      SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      sNormalLMouseEventInProcess = PR_TRUE;
      break;
    case nsMouseEvent::eMiddleButton:
      mMClickCount = ((nsMouseEvent*)aEvent)->clickCount;
      SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      break;
    case nsMouseEvent::eRightButton:
#ifdef XP_OS2
      BeginTrackingDragGesture(aPresContext, (nsMouseEvent*)aEvent, aTargetFrame);
#endif
      mRClickCount = ((nsMouseEvent*)aEvent)->clickCount;
      SetClickCount(aPresContext, (nsMouseEvent*)aEvent, aStatus);
      break;
    }
    break;
  case NS_MOUSE_BUTTON_UP:
    switch (static_cast<nsMouseEvent*>(aEvent)->button) {
      case nsMouseEvent::eLeftButton:
        if (mClickHoldContextMenu) {
          KillClickHoldTimer();
        }
#ifndef XP_OS2
        StopTrackingDragGesture();
#endif
        sNormalLMouseEventInProcess = PR_FALSE;
        
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
  case NS_DRAGDROP_GESTURE:
    if (mClickHoldContextMenu) {
      
      
      KillClickHoldTimer();
    }
    break;
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

      PRBool useSysNumLines = UseSystemScrollSettingFor(msEvent);
      PRInt32 action = GetWheelActionFor(msEvent);

      if (!useSysNumLines) {
        
        
        
        
        
        
        
        
        
        
        
        

        PRInt32 numLines = GetScrollLinesFor(msEvent);

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
          GetScrollableLineHeight(aTargetFrame));

        if (msEvent->scrollFlags & nsMouseScrollEvent::kIsVertical) {
          gPixelScrollDeltaX += msEvent->delta;
          if (!gPixelScrollDeltaX || !pixelHeight)
            break;

          if (NS_ABS(gPixelScrollDeltaX) >= pixelHeight) {
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

          if (NS_ABS(gPixelScrollDeltaY) >= pixelHeight) {
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
      if (RemoteQueryContentEvent(aEvent))
        break;
      nsContentEventHandler handler(mPresContext);
      handler.OnQuerySelectedText((nsQueryContentEvent*)aEvent);
    }
    break;
  case NS_QUERY_TEXT_CONTENT:
    {
      if (RemoteQueryContentEvent(aEvent))
        break;
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
  case NS_QUERY_DOM_WIDGET_HITTEST:
    {
      
      nsContentEventHandler handler(mPresContext);
      handler.OnQueryDOMWidgetHittest(static_cast<nsQueryContentEvent*>(aEvent));
    }
    break;
  case NS_QUERY_SCROLL_TARGET_INFO:
    {
      DoQueryScrollTargetInfo(static_cast<nsQueryContentEvent*>(aEvent),
                              aTargetFrame);
      break;
    }
  case NS_SELECTION_SET:
    {
      nsSelectionEvent *selectionEvent =
          static_cast<nsSelectionEvent*>(aEvent);
      if (IsTargetCrossProcess(selectionEvent)) {
        
        if (GetCrossProcessTarget()->SendSelectionEvent(*selectionEvent))
          selectionEvent->mSucceeded = PR_TRUE;
        break;
      }
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
  case NS_CONTENT_COMMAND_PASTE_TRANSFERABLE:
    {
      DoContentCommandEvent(static_cast<nsContentCommandEvent*>(aEvent));
    }
    break;
  case NS_CONTENT_COMMAND_SCROLL:
    {
      DoContentCommandScrollEvent(static_cast<nsContentCommandEvent*>(aEvent));
    }
    break;
  case NS_TEXT_TEXT:
    {
      nsTextEvent *textEvent = static_cast<nsTextEvent*>(aEvent);
      if (IsTargetCrossProcess(textEvent)) {
        
        if (GetCrossProcessTarget()->SendTextEvent(*textEvent)) {
          
          aEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
        }
      }
    }
    break;
  case NS_COMPOSITION_START:
  case NS_COMPOSITION_END:
    {
      nsCompositionEvent *compositionEvent =
          static_cast<nsCompositionEvent*>(aEvent);
      if (IsTargetCrossProcess(compositionEvent)) {
        
        if (GetCrossProcessTarget()->SendCompositionEvent(*compositionEvent)) {
          
          aEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
        }
      }
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
  if (!xulDoc && !aContent->IsXUL())
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

  if (aContent->IsHTML()) {
    nsIAtom* tag = aContent->Tag();

    
    
    if (tag == nsGkAtoms::area ||
        tag == nsGkAtoms::label ||
        tag == nsGkAtoms::legend)
      return PR_TRUE;

  } else if (aContent->IsXUL()) {
    
    
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
      frame = content->GetPrimaryFrame();
      if (IsAccessKeyTarget(content, frame, accessKey)) {
        PRBool shouldActivate = sKeyCausesActivation;
        while (shouldActivate && ++count <= length) {
          nsIContent *oc = mAccessKeys[(start + count) % length];
          nsIFrame *of = oc->GetPrimaryFrame();
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
  if (mClickHoldTimer) {
    PRInt32 clickHoldDelay =
      Preferences::GetInt("ui.click_hold_context_menus.delay", 500);
    mClickHoldTimer->InitWithFuncCallback(sClickHoldCallback, this,
                                          clickHoldDelay,
                                          nsITimer::TYPE_ONE_SHOT);
  }
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
  if (self)
    self->FireContextClick();

  

} 
















void
nsEventStateManager::FireContextClick()
{
  if (!mGestureDownContent)
    return;

#ifdef XP_MACOSX
  
  
  
  if (!CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft))
    return;
#endif

  nsEventStatus status = nsEventStatus_eIgnore;

  
  
  
  
  
  
  mCurrentTarget = mPresContext->GetPrimaryFrameFor(mGestureDownContent);
  if (mCurrentTarget) {
    NS_ASSERTION(mPresContext == mCurrentTarget->PresContext(),
                 "a prescontext returned a primary frame that didn't belong to it?");

    
    
    nsIAtom *tag = mGestureDownContent->Tag();
    PRBool allowedToDispatch = PR_TRUE;

    if (mGestureDownContent->IsXUL()) {
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
    else if (mGestureDownContent->IsHTML()) {
      nsCOMPtr<nsIFormControl> formCtrl(do_QueryInterface(mGestureDownContent));

      if (formCtrl) {
        
        
        PRInt32 type = formCtrl->GetType();

        allowedToDispatch = (type == NS_FORM_INPUT_TEXT ||
                             type == NS_FORM_INPUT_EMAIL ||
                             type == NS_FORM_INPUT_SEARCH ||
                             type == NS_FORM_INPUT_TEL ||
                             type == NS_FORM_INPUT_URL ||
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
      
      nsCOMPtr<nsIWidget> targetWidget(mCurrentTarget->GetNearestWidget());
      
      nsMouseEvent event(PR_TRUE, NS_CONTEXTMENU,
                         targetWidget,
                         nsMouseEvent::eReal);
      event.clickCount = 1;
      FillInEventFromGestureDown(&event);
        
      
      if (mCurrentTarget)
      {
        nsRefPtr<nsFrameSelection> frameSel =
          mCurrentTarget->GetFrameSelection();
        
        if (frameSel && frameSel->GetMouseDownState()) {
          
          
          frameSel->SetMouseDownState(PR_FALSE);
        }
      }

      
      nsEventDispatcher::Dispatch(mGestureDownContent, mPresContext, &event,
                                  nsnull, &status);

      
      
      
      
    }
  }

  
  if (status == nsEventStatus_eConsumeNoDefault) {
    StopTrackingDragGesture();
  }

  KillClickHoldTimer();

} 














void
nsEventStateManager::BeginTrackingDragGesture(nsPresContext* aPresContext,
                                              nsMouseEvent* inDownEvent,
                                              nsIFrame* inDownFrame)
{
  if (!inDownEvent->widget)
    return;

  
  
  mGestureDownPoint = inDownEvent->refPoint +
    inDownEvent->widget->WidgetToScreenOffset();

  inDownFrame->GetContentForEvent(aPresContext, inDownEvent,
                                  getter_AddRefs(mGestureDownContent));

  mGestureDownFrameOwner = inDownFrame->GetContent();
  mGestureDownShift = inDownEvent->isShift;
  mGestureDownControl = inDownEvent->isControl;
  mGestureDownAlt = inDownEvent->isAlt;
  mGestureDownMeta = inDownEvent->isMeta;

  if (mClickHoldContextMenu) {
    
    CreateClickHoldTimer(aPresContext, inDownFrame, inDownEvent);
  }
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
  NS_ASSERTION(aEvent->widget == mCurrentTarget->GetNearestWidget(),
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
  if (IsTrackingDragGesture()) {
    mCurrentTarget = mGestureDownFrameOwner->GetPrimaryFrame();

    if (!mCurrentTarget) {
      StopTrackingDragGesture();
      return;
    }

    
    
    if (mCurrentTarget)
    {
      nsRefPtr<nsFrameSelection> frameSel = mCurrentTarget->GetFrameSelection();
      if (frameSel && frameSel->GetMouseDownState()) {
        StopTrackingDragGesture();
        return;
      }
    }

    
    if (nsIPresShell::IsMouseCapturePreventingDrag()) {
      StopTrackingDragGesture();
      return;
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
    if (NS_ABS(pt.x - mGestureDownPoint.x) > pixelThresholdX ||
        NS_ABS(pt.y - mGestureDownPoint.y) > pixelThresholdY) {
      if (mClickHoldContextMenu) {
        
        
        KillClickHoldTimer();
      }

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

      nsCOMPtr<nsIWidget> widget = mCurrentTarget->GetNearestWidget();

      
      nsDragEvent startEvent(NS_IS_TRUSTED_EVENT(aEvent), NS_DRAGDROP_START, widget);
      FillInEventFromGestureDown(&startEvent);

      nsDragEvent gestureEvent(NS_IS_TRUSTED_EVENT(aEvent), NS_DRAGDROP_GESTURE, widget);
      FillInEventFromGestureDown(&gestureEvent);

      startEvent.dataTransfer = gestureEvent.dataTransfer = dataTransfer;
      startEvent.inputSource = gestureEvent.inputSource = aEvent->inputSource;

      
      
      
      
      
      
      
      

      
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

      if (status != nsEventStatus_eConsumeNoDefault) {
        PRBool dragStarted = DoDefaultDragStart(aPresContext, event, dataTransfer,
                                                targetContent, isSelection);
        if (dragStarted) {
          sActiveESM = nsnull;
          aEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
        }
      }

      
      
      

      
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

PRBool
nsEventStateManager::DoDefaultDragStart(nsPresContext* aPresContext,
                                        nsDragEvent* aDragEvent,
                                        nsDOMDataTransfer* aDataTransfer,
                                        nsIContent* aDragTarget,
                                        PRBool aIsSelection)
{
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (!dragService)
    return PR_FALSE;

  
  
  
  
  
  
  
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (dragSession)
    return PR_TRUE;

  
  
  PRUint32 count = 0;
  if (aDataTransfer)
    aDataTransfer->GetMozItemCount(&count);
  if (!count)
    return PR_FALSE;

  
  
  
  
  nsCOMPtr<nsIDOMNode> dragTarget;
  nsCOMPtr<nsIDOMElement> dragTargetElement;
  aDataTransfer->GetDragTarget(getter_AddRefs(dragTargetElement));
  dragTarget = do_QueryInterface(dragTargetElement);
  if (!dragTarget) {
    dragTarget = do_QueryInterface(aDragTarget);
    if (!dragTarget)
      return PR_FALSE;
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
      nsIPresShell* presShell = doc->GetShell();
      if (presShell) {
        selection = presShell->GetCurrentSelection(
                      nsISelectionController::SELECTION_NORMAL);
      }
    }
  }

  nsCOMPtr<nsISupportsArray> transArray;
  aDataTransfer->GetTransferables(getter_AddRefs(transArray));
  if (!transArray)
    return PR_FALSE;

  
  
  
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
        nsTreeBodyFrame* treeBody = do_QueryFrame(content->GetPrimaryFrame());
        if (treeBody) {
          treeBody->GetSelectionRegion(getter_AddRefs(region));
        }
      }
    }
#endif

    dragService->InvokeDragSessionWithImage(dragTarget, transArray,
                                            region, action, dragImage,
                                            imageX, imageY, domDragEvent,
                                            aDataTransfer);
  }

  return PR_TRUE;
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

  nsIPresShell *presShell = doc->GetShell();
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
  float zoomMin = ((float)Preferences::GetInt("zoom.minPercent", 50)) / 100;
  float zoomMax = ((float)Preferences::GetInt("zoom.maxPercent", 300)) / 100;
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
  float zoomMin = ((float)Preferences::GetInt("zoom.minPercent", 50)) / 100;
  float zoomMax = ((float)Preferences::GetInt("zoom.maxPercent", 300)) / 100;
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
      !content->IsXUL())
    {
      
      PRInt32 change = (adjustment > 0) ? -1 : 1;

      if (Preferences::GetBool("browser.zoom.full")) {
        ChangeFullZoom(change);
      } else {
        ChangeTextSize(change);
      }
    }
}

static nsIFrame*
GetParentFrameToScroll(nsIFrame* aFrame)
{
  if (!aFrame)
    return nsnull;

  if (aFrame->GetStyleDisplay()->mPosition == NS_STYLE_POSITION_FIXED &&
      nsLayoutUtils::IsReallyFixedPos(aFrame))
    return aFrame->PresContext()->GetPresShell()->GetRootScrollFrame();

  return aFrame->GetParent();
}

static nscoord
GetScrollableLineHeight(nsIFrame* aTargetFrame)
{
  for (nsIFrame* f = aTargetFrame; f; f = GetParentFrameToScroll(f)) {
    nsIScrollableFrame* sf = f->GetScrollTargetFrame();
    if (sf)
      return sf->GetLineScrollAmount().height;
  }

  
  const nsStyleFont* font = aTargetFrame->GetStyleFont();
  const nsFont& f = font->mFont;
  nsRefPtr<nsFontMetrics> fm = aTargetFrame->PresContext()->GetMetricsFor(f);
  NS_ASSERTION(fm, "FontMetrics is null!");
  if (fm)
    return fm->MaxHeight();
  return 0;
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
  event.inputSource = static_cast<nsMouseEvent_base*>(aEvent)->inputSource;

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

  nscoord lineHeight = GetScrollableLineHeight(aTargetFrame);

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
  event.inputSource = static_cast<nsMouseEvent_base*>(aEvent)->inputSource;
  event.delta = aPresContext->AppUnitsToIntCSSPixels(aEvent->delta * lineHeight);

  nsEventDispatcher::Dispatch(targetContent, aPresContext, &event, nsnull, aStatus);
}

PRInt32
nsEventStateManager::ComputeWheelActionFor(nsMouseScrollEvent* aMouseEvent,
                                           PRBool aUseSystemSettings)
{
  PRInt32 action = GetWheelActionFor(aMouseEvent);
  if (aUseSystemSettings &&
      (aMouseEvent->scrollFlags & nsMouseScrollEvent::kIsFullPage)) {
    action = MOUSE_SCROLL_PAGE;
  }

  if (aMouseEvent->message == NS_MOUSE_PIXEL_SCROLL) {
    if (action == MOUSE_SCROLL_N_LINES || action == MOUSE_SCROLL_PAGE ||
        (aMouseEvent->scrollFlags & nsMouseScrollEvent::kIsMomentum)) {
      action = MOUSE_SCROLL_PIXELS;
    } else {
      
      action = -1;
    }
  } else if (aMouseEvent->scrollFlags & nsMouseScrollEvent::kHasPixels) {
    if (aUseSystemSettings ||
        action == MOUSE_SCROLL_N_LINES || action == MOUSE_SCROLL_PAGE ||
        (aMouseEvent->scrollFlags & nsMouseScrollEvent::kIsMomentum)) {
      
      
      action = -1;
    }
  }

  return action;
}

PRInt32
nsEventStateManager::GetWheelActionFor(nsMouseScrollEvent* aMouseEvent)
{
  nsCAutoString prefName;
  GetBasePrefKeyForMouseWheel(aMouseEvent, prefName);
  prefName.Append(".action");
  return Preferences::GetInt(prefName.get());
}

PRInt32
nsEventStateManager::GetScrollLinesFor(nsMouseScrollEvent* aMouseEvent)
{
  NS_ASSERTION(!UseSystemScrollSettingFor(aMouseEvent),
    "GetScrollLinesFor() called when should use system settings");
  nsCAutoString prefName;
  GetBasePrefKeyForMouseWheel(aMouseEvent, prefName);
  prefName.Append(".numlines");
  return Preferences::GetInt(prefName.get());
}

PRBool
nsEventStateManager::UseSystemScrollSettingFor(nsMouseScrollEvent* aMouseEvent)
{
  nsCAutoString prefName;
  GetBasePrefKeyForMouseWheel(aMouseEvent, prefName);
  prefName.Append(".sysnumlines");
  return Preferences::GetBool(prefName.get());
}

nsresult
nsEventStateManager::DoScrollText(nsIFrame* aTargetFrame,
                                  nsMouseScrollEvent* aMouseEvent,
                                  nsIScrollableFrame::ScrollUnit aScrollQuantity,
                                  PRBool aAllowScrollSpeedOverride,
                                  nsQueryContentEvent* aQueryEvent)
{
  nsIScrollableFrame* frameToScroll = nsnull;
  nsIFrame* scrollFrame = aTargetFrame;
  PRInt32 numLines = aMouseEvent->delta;
  PRBool isHorizontal = aMouseEvent->scrollFlags & nsMouseScrollEvent::kIsHorizontal;
  aMouseEvent->scrollOverflow = 0;

  
  
  
  
  
  
  
  
  nsIFrame* lastScrollFrame = nsMouseWheelTransaction::GetTargetFrame();
  if (lastScrollFrame) {
    frameToScroll = lastScrollFrame->GetScrollTargetFrame();
    if (frameToScroll) {
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
       scrollFrame = GetParentFrameToScroll(scrollFrame)) {
    
    frameToScroll = scrollFrame->GetScrollTargetFrame();
    if (!frameToScroll) {
      continue;
    }

    nsPresContext::ScrollbarStyles ss = frameToScroll->GetScrollbarStyles();
    if (NS_STYLE_OVERFLOW_HIDDEN ==
        (isHorizontal ? ss.mHorizontal : ss.mVertical)) {
      continue;
    }

    
    nscoord lineHeight = frameToScroll->GetLineScrollAmount().height;
    if (lineHeight != 0) {
      if (CanScrollOn(frameToScroll, numLines, isHorizontal)) {
        passToParent = PR_FALSE;
        nsMouseWheelTransaction::BeginTransaction(scrollFrame,
                                                  numLines, isHorizontal);
      }

      
      nsIComboboxControlFrame* comboBox = do_QueryFrame(scrollFrame);
      if (comboBox) {
        if (comboBox->IsDroppedDown()) {
          
          if (passToParent) {
            passToParent = PR_FALSE;
            frameToScroll = nsnull;
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

  if (!passToParent && frameToScroll) {
    if (aScrollQuantity == nsIScrollableFrame::LINES) {
      
      
      
      numLines =
        nsMouseWheelTransaction::AccelerateWheelDelta(numLines, isHorizontal,
                                                      aAllowScrollSpeedOverride,
                                                      &aScrollQuantity,
                                                      !aQueryEvent);
    }
#ifdef DEBUG
    else {
      NS_ASSERTION(!aAllowScrollSpeedOverride,
        "aAllowScrollSpeedOverride is true but the quantity isn't by-line scrolling.");
    }
#endif

    if (aScrollQuantity == nsIScrollableFrame::PAGES) {
      numLines = (numLines > 0) ? 1 : -1;
    }

    if (aQueryEvent) {
      
      
      if (nsMouseWheelTransaction::IsAccelerationEnabled()) {
        return NS_OK;
      }

      nscoord appUnitsPerDevPixel =
        aTargetFrame->PresContext()->AppUnitsPerDevPixel();
      aQueryEvent->mReply.mLineHeight =
        frameToScroll->GetLineScrollAmount().height / appUnitsPerDevPixel;
      aQueryEvent->mReply.mPageHeight =
        frameToScroll->GetPageScrollAmount().height / appUnitsPerDevPixel;
      aQueryEvent->mReply.mPageWidth =
        frameToScroll->GetPageScrollAmount().width / appUnitsPerDevPixel;

      
      
      aQueryEvent->mReply.mComputedScrollAmount = numLines;

      aQueryEvent->mSucceeded = PR_TRUE;
      return NS_OK;
    }

    PRInt32 scrollX = 0;
    PRInt32 scrollY = numLines;

    if (isHorizontal) {
      scrollX = scrollY;
      scrollY = 0;
    }

    nsIScrollableFrame::ScrollMode mode;
    if (aMouseEvent->scrollFlags & nsMouseScrollEvent::kNoDefer) {
      mode = nsIScrollableFrame::INSTANT;
    } else if (aScrollQuantity != nsIScrollableFrame::DEVICE_PIXELS ||
               (aMouseEvent->scrollFlags &
                  nsMouseScrollEvent::kAllowSmoothScroll) != 0) {
      mode = nsIScrollableFrame::SMOOTH;
    } else {
      mode = nsIScrollableFrame::NORMAL;
    }

    

    nsIntPoint overflow;
    frameToScroll->ScrollBy(nsIntPoint(scrollX, scrollY), aScrollQuantity,
                            mode, &overflow);
    aMouseEvent->scrollOverflow = isHorizontal ? overflow.x : overflow.y;
    return NS_OK;
  }
  
  if (passToParent) {
    nsIFrame* newFrame = nsLayoutUtils::GetCrossDocParentFrame(
        aTargetFrame->PresContext()->FrameManager()->GetRootFrame());
    if (newFrame)
      return DoScrollText(newFrame, aMouseEvent, aScrollQuantity,
                          aAllowScrollSpeedOverride, aQueryEvent);
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
        displayPanFeedback = PR_TRUE;

        nsRect scrollRange = scrollableFrame->GetScrollRange();
        PRBool canScrollHorizontally = scrollRange.width > 0;

        if (targetFrame->GetType() == nsGkAtoms::menuFrame) {
          
          
          canScrollHorizontally = PR_FALSE;
          displayPanFeedback = PR_FALSE;
        }

        
        
        if (scrollRange.height > 0) {
          panDirection = nsGestureNotifyEvent::ePanVertical;
          break;
        }

        if (canScrollHorizontally) {
          panDirection = nsGestureNotifyEvent::ePanHorizontal;
          displayPanFeedback = PR_FALSE;
        }
      } else { 
        PRUint32 scrollbarVisibility = scrollableFrame->GetScrollbarVisibility();

        
        if (scrollbarVisibility & nsIScrollableFrame::VERTICAL) {
          panDirection = nsGestureNotifyEvent::ePanVertical;
          displayPanFeedback = PR_TRUE;
          break;
        }

        if (scrollbarVisibility & nsIScrollableFrame::HORIZONTAL) {
          panDirection = nsGestureNotifyEvent::ePanHorizontal;
          displayPanFeedback = PR_TRUE;
        }
      }
    } 
  } 

  aEvent->displayPanFeedback = displayPanFeedback;
  aEvent->panDirection = panDirection;
}

#ifdef XP_MACOSX
static bool
NodeAllowsClickThrough(nsINode* aNode)
{
  while (aNode) {
    if (aNode->IsElement() && aNode->AsElement()->IsXUL()) {
      mozilla::dom::Element* element = aNode->AsElement();
      static nsIContent::AttrValuesArray strings[] =
        {&nsGkAtoms::always, &nsGkAtoms::never, nsnull};
      switch (element->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::clickthrough,
                                       strings, eCaseMatters)) {
        case 0:
          return true;
        case 1:
          return false;
      }
    }
    aNode = nsContentUtils::GetCrossDocParentNode(aNode);
  }
  return true;
}
#endif

nsresult
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

  
  
  if (!mCurrentTarget && aEvent->message != NS_MOUSE_BUTTON_UP &&
      aEvent->message != NS_MOUSE_BUTTON_DOWN) {
    return NS_OK;
  }

  
  nsRefPtr<nsPresContext> presContext = aPresContext;
  nsresult ret = NS_OK;

  switch (aEvent->message) {
  case NS_MOUSE_BUTTON_DOWN:
    {
      if (static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton &&
          !sNormalLMouseEventInProcess) {
        
        
        nsIPresShell::SetCapturingContent(nsnull, 0);
        break;
      }

      nsCOMPtr<nsIContent> activeContent;
      if (nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> newFocus;      
        PRBool suppressBlur = PR_FALSE;
        if (mCurrentTarget) {
          mCurrentTarget->GetContentForEvent(mPresContext, aEvent, getter_AddRefs(newFocus));
          const nsStyleUserInterface* ui = mCurrentTarget->GetStyleUserInterface();
          suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);
          activeContent = mCurrentTarget->GetContent();
        }

        nsIFrame* currFrame = mCurrentTarget;

        
        
        
        
        
        
        
        
        if (newFocus && !newFocus->IsEditable()) {
          nsIDocument *doc = newFocus->GetCurrentDoc();
          if (doc && newFocus == doc->GetRootElement()) {
            nsIContent *bodyContent =
              nsLayoutUtils::GetEditableRootContentByContentEditable(doc);
            if (bodyContent) {
              nsIFrame* bodyFrame = bodyContent->GetPrimaryFrame();
              if (bodyFrame) {
                currFrame = bodyFrame;
                newFocus = bodyContent;
              }
            }
          }
        }

        
        
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
#ifdef XP_MACOSX
              if (!activeContent || !activeContent->IsXUL())
#endif
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
        }
      }
      else {
        
        
        StopTrackingDragGesture();

        
        
        
        EnsureDocument(mPresContext);
        nsIFocusManager* fm = nsFocusManager::GetFocusManager();
        if (mDocument && fm) {
          nsCOMPtr<nsIDOMWindow> currentWindow;
          fm->GetFocusedWindow(getter_AddRefs(currentWindow));
          if (currentWindow && currentWindow != mDocument->GetWindow() &&
              !nsContentUtils::IsChromeDoc(mDocument)) {
            nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(currentWindow);
            nsCOMPtr<nsIDocument> currentDoc = do_QueryInterface(win->GetExtantDocument());
            if (nsContentUtils::IsChromeDoc(currentDoc)) {
              fm->SetFocusedWindow(mDocument->GetWindow());
            }
          }
        }
      }
      SetActiveManager(this, activeContent);
    }
    break;
  case NS_MOUSE_BUTTON_UP:
    {
      ClearGlobalActiveContent(this);
      if (IsMouseEventReal(aEvent)) {
        if (!mCurrentTarget) {
          GetEventTarget();
        }
        if (mCurrentTarget) {
          ret = CheckForAndDispatchClick(presContext, (nsMouseEvent*)aEvent,
                                         aStatus);
        }
      }

      nsIPresShell *shell = presContext->GetPresShell();
      if (shell) {
        nsRefPtr<nsFrameSelection> frameSelection = shell->FrameSelection();
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
        PRBool useSysNumLines = UseSystemScrollSettingFor(msEvent);
        PRInt32 action = ComputeWheelActionFor(msEvent, useSysNumLines);

        switch (action) {
        case MOUSE_SCROLL_N_LINES:
          DoScrollText(aTargetFrame, msEvent, nsIScrollableFrame::LINES,
                       useSysNumLines);
          break;

        case MOUSE_SCROLL_PAGE:
          DoScrollText(aTargetFrame, msEvent, nsIScrollableFrame::PAGES,
                       PR_FALSE);
          break;

        case MOUSE_SCROLL_PIXELS:
          DoScrollText(aTargetFrame, msEvent, nsIScrollableFrame::DEVICE_PIXELS,
                       PR_FALSE);
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

      
      dragSession->SetOnlyChromeDrop(PR_FALSE);
      if (mPresContext) {
        EnsureDocument(mPresContext);
      }
      PRBool isChromeDoc = nsContentUtils::IsChromeDoc(mDocument);

      
      
      nsCOMPtr<nsIDOMNSDataTransfer> dataTransfer;
      nsCOMPtr<nsIDOMDataTransfer> initialDataTransfer;
      dragSession->GetDataTransfer(getter_AddRefs(initialDataTransfer));

      nsCOMPtr<nsIDOMNSDataTransfer> initialDataTransferNS = 
        do_QueryInterface(initialDataTransfer);

      nsDragEvent *dragEvent = (nsDragEvent*)aEvent;

      
      
      UpdateDragDataTransfer(dragEvent);

      
      
      
      
      
      
      
      
      
      PRUint32 dropEffect = nsIDragService::DRAGDROP_ACTION_NONE;
      if (nsEventStatus_eConsumeNoDefault == *aStatus) {
        
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

        
        
        if (aEvent->message == NS_DRAGDROP_OVER && !isChromeDoc) {
          
          dragSession->SetOnlyChromeDrop(
            !(aEvent->flags & NS_EVENT_FLAG_NO_DEFAULT_CALLED_IN_CONTENT));
        }
      } else if (aEvent->message == NS_DRAGDROP_OVER && !isChromeDoc) {
        
        dragSession->SetOnlyChromeDrop(PR_TRUE);
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

        nsCOMPtr<nsIWidget> widget = mCurrentTarget->GetNearestWidget();
        nsDragEvent event(NS_IS_TRUSTED_EVENT(aEvent), NS_DRAGDROP_DRAGDROP, widget);

        nsMouseEvent* mouseEvent = static_cast<nsMouseEvent*>(aEvent);
        event.refPoint = mouseEvent->refPoint;
        if (mouseEvent->widget) {
          event.refPoint += mouseEvent->widget->WidgetToScreenOffset();
        }
        event.refPoint -= widget->WidgetToScreenOffset();
        event.isShift = mouseEvent->isShift;
        event.isControl = mouseEvent->isControl;
        event.isAlt = mouseEvent->isAlt;
        event.isMeta = mouseEvent->isMeta;
        event.inputSource = mouseEvent->inputSource;

        nsEventStatus status = nsEventStatus_eIgnore;
        nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
        if (presShell) {
          presShell->HandleEventWithTarget(&event, mCurrentTarget,
                                           targetContent, &status);
        }
      }
      ClearGlobalActiveContent(this);
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
              PRUint32 dir =
                static_cast<nsInputEvent*>(aEvent)->isShift ?
                  (isDocMove ? static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_BACKWARDDOC) :
                               static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_BACKWARD)) :
                  (isDocMove ? static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_FORWARDDOC) :
                               static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_FORWARD));
              nsCOMPtr<nsIDOMElement> result;
              fm->MoveFocus(mDocument->GetWindow(), nsnull, dir,
                            nsIFocusManager::FLAG_BYKEY,
                            getter_AddRefs(result));
            }
            *aStatus = nsEventStatus_eConsumeNoDefault;
            break;
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

#ifdef XP_MACOSX
  case NS_MOUSE_ACTIVATE:
    if (mCurrentTarget) {
      nsCOMPtr<nsIContent> targetContent;
      mCurrentTarget->GetContentForEvent(presContext, aEvent,
                                         getter_AddRefs(targetContent));
      if (!NodeAllowsClickThrough(targetContent)) {
        *aStatus = nsEventStatus_eConsumeNoDefault;
      }
    }
    break;
#endif
  }

  
  mCurrentTarget = nsnull;
  mCurrentTargetContent = nsnull;

  return ret;
}

PRBool
nsEventStateManager::RemoteQueryContentEvent(nsEvent *aEvent)
{
  nsQueryContentEvent *queryEvent =
      static_cast<nsQueryContentEvent*>(aEvent);
  if (!IsTargetCrossProcess(queryEvent)) {
    return PR_FALSE;
  }
  
  GetCrossProcessTarget()->HandleQueryContentEvent(*queryEvent);
  return PR_TRUE;
}

TabParent*
nsEventStateManager::GetCrossProcessTarget()
{
  return TabParent::GetIMETabParent();
}

PRBool
nsEventStateManager::IsTargetCrossProcess(nsGUIEvent *aEvent)
{
  
  
  nsIContent *focusedContent = GetFocusedContent();
  if (focusedContent && focusedContent->IsEditable())
    return PR_FALSE;
  return TabParent::GetIMETabParent() != nsnull;
}

void
nsEventStateManager::NotifyDestroyPresContext(nsPresContext* aPresContext)
{
  nsIMEStateManager::OnDestroyPresContext(aPresContext);
}

void
nsEventStateManager::SetPresContext(nsPresContext* aPresContext)
{
  mPresContext = aPresContext;
}

void
nsEventStateManager::ClearFrameRefs(nsIFrame* aFrame)
{
  if (aFrame && aFrame == mCurrentTarget) {
    mCurrentTargetContent = aFrame->GetContent();
  }
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

  if (Preferences::GetBool("ui.use_activity_cursor", PR_FALSE)) {
    
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
              aTargetFrame->GetNearestWidget(), PR_FALSE);
  }

  if (mLockCursor || NS_STYLE_CURSOR_AUTO != cursor) {
    *aStatus = nsEventStatus_eConsumeDoDefault;
  }
}

nsresult
nsEventStateManager::SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                               PRBool aHaveHotspot,
                               float aHotspotX, float aHotspotY,
                               nsIWidget* aWidget, PRBool aLockCursor)
{
  EnsureDocument(mPresContext);
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);
  sMouseOverDocument = mDocument.get();

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
      nsIFrame* frame = aVisitor.mPresContext->GetPrimaryFrameFor(mTarget);
      if (frame) {
        frame->HandleEvent(aVisitor.mPresContext,
                           (nsGUIEvent*) aVisitor.mEvent,
                           &aVisitor.mEventStatus);
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
  event.pluginEvent = ((nsMouseEvent*)aEvent)->pluginEvent;
  event.relatedTarget = aRelatedContent;
  event.inputSource = static_cast<nsMouseEvent*>(aEvent)->inputSource;

  nsWeakFrame previousTarget = mCurrentTarget;

  mCurrentTargetContent = aTargetContent;

  nsIFrame* targetFrame = nsnull;
  if (aTargetContent) {
    nsESMEventCB callback(aTargetContent);
    nsEventDispatcher::Dispatch(aTargetContent, mPresContext, &event, nsnull,
                                &status, &callback);

    
    
    
    if (mPresContext) {
      targetFrame = mPresContext->GetPrimaryFrameFor(aTargetContent);
    }
  }

  mCurrentTargetContent = nsnull;
  mCurrentTarget = previousTarget;

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
    
    
    nsSubDocumentFrame* subdocFrame = do_QueryFrame(mLastMouseOverFrame.GetFrame());
    if (subdocFrame) {
      nsCOMPtr<nsIDocShell> docshell;
      subdocFrame->GetDocShell(getter_AddRefs(docshell));
      if (docshell) {
        nsRefPtr<nsPresContext> presContext;
        docshell->GetPresContext(getter_AddRefs(presContext));

        if (presContext) {
          nsEventStateManager* kidESM = presContext->EventStateManager();
          
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
      nsIPresShell *parentShell = parentDoc->GetShell();
      if (parentShell) {
        nsEventStateManager* parentESM = parentShell->GetPresContext()->EventStateManager();
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
      
      nsCOMPtr<nsIContent> targetElement = GetEventTargetContent(aEvent);
      if (!targetElement) {
        
        
        
        targetElement = mDocument->GetRootElement();
      }
      if (targetElement) {
        NotifyMouseOver(aEvent, targetElement);
      }
    }
    break;
  case NS_MOUSE_EXIT:
    {
      
      

      if (mLastMouseOverFrame &&
          nsContentUtils::GetTopLevelWidget(aEvent->widget) !=
          nsContentUtils::GetTopLevelWidget(mLastMouseOverFrame->GetNearestWidget())) {
        
        
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

        if (mLastDragOverFrame) {
          
          mLastDragOverFrame->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(lastContent));

          FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_EXIT_SYNTH,
                              targetContent, lastContent, mLastDragOverFrame);
        }

        FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_ENTER,
                            lastContent, targetContent, mCurrentTarget);

        if (mLastDragOverFrame) {
          FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_LEAVE_SYNTH,
                              targetContent, lastContent, mLastDragOverFrame);
        }

        mLastDragOverFrame = mCurrentTarget;
      }
    }
    break;

  case NS_DRAGDROP_EXIT:
    {
      
      if (mLastDragOverFrame) {
        nsCOMPtr<nsIContent> lastContent;
        mLastDragOverFrame->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(lastContent));

        FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_EXIT_SYNTH,
                            nsnull, lastContent, mLastDragOverFrame);
        FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_LEAVE_SYNTH,
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
  event.inputSource = static_cast<nsMouseEvent*>(aEvent)->inputSource;

  mCurrentTargetContent = aTargetContent;

  if (aTargetContent != aRelatedTarget) {
    
    if (aTargetContent)
      nsEventDispatcher::Dispatch(aTargetContent, aPresContext, &event,
                                  nsnull, &status);

    
    if (status == nsEventStatus_eConsumeNoDefault || aMsg == NS_DRAGDROP_EXIT)
      SetContentState((aMsg == NS_DRAGDROP_ENTER) ? aTargetContent : nsnull,
                      NS_EVENT_STATE_DRAGOVER);

    
    
    if (aMsg == NS_DRAGDROP_LEAVE_SYNTH || aMsg == NS_DRAGDROP_EXIT_SYNTH ||
        aMsg == NS_DRAGDROP_ENTER)
      UpdateDragDataTransfer(&event);
  }

  
  if (aTargetFrame)
    aTargetFrame->HandleEvent(aPresContext, &event, &status);
}

void
nsEventStateManager::UpdateDragDataTransfer(nsDragEvent* dragEvent)
{
  NS_ASSERTION(dragEvent, "drag event is null in UpdateDragDataTransfer!");
  if (!dragEvent->dataTransfer)
    return;

  nsCOMPtr<nsIDragSession> dragSession = nsContentUtils::GetDragSession();

  if (dragSession) {
    
    
    nsCOMPtr<nsIDOMDataTransfer> initialDataTransfer;
    dragSession->GetDataTransfer(getter_AddRefs(initialDataTransfer));

    
    nsCOMPtr<nsIDOMNSDataTransfer> initialDataTransferNS = 
      do_QueryInterface(initialDataTransfer);
    nsCOMPtr<nsIDOMNSDataTransfer> eventTransferNS = 
      do_QueryInterface(dragEvent->dataTransfer);

    if (initialDataTransferNS && eventTransferNS) {
      
      nsAutoString mozCursor;
      eventTransferNS->GetMozCursor(mozCursor);
      initialDataTransferNS->SetMozCursor(mozCursor);
    }
  }
}

nsresult
nsEventStateManager::SetClickCount(nsPresContext* aPresContext,
                                   nsMouseEvent *aEvent,
                                   nsEventStatus* aStatus)
{
  nsCOMPtr<nsIContent> mouseContent;
  nsIContent* mouseContentParent = nsnull;
  mCurrentTarget->GetContentForEvent(aPresContext, aEvent, getter_AddRefs(mouseContent));
  if (mouseContent) {
    if (mouseContent->IsNodeOfType(nsINode::eTEXT)) {
      mouseContent = mouseContent->GetParent();
    }
    if (mouseContent && mouseContent->IsRootOfNativeAnonymousSubtree()) {
      mouseContentParent = mouseContent->GetParent();
    }
  }

  switch (aEvent->button) {
  case nsMouseEvent::eLeftButton:
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      mLastLeftMouseDownContent = mouseContent;
      mLastLeftMouseDownContentParent = mouseContentParent;
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      if (mLastLeftMouseDownContent == mouseContent ||
          mLastLeftMouseDownContentParent == mouseContent ||
          mLastLeftMouseDownContent == mouseContentParent) {
        aEvent->clickCount = mLClickCount;
        mLClickCount = 0;
      } else {
        aEvent->clickCount = 0;
      }
      mLastLeftMouseDownContent = nsnull;
      mLastLeftMouseDownContentParent = nsnull;
    }
    break;

  case nsMouseEvent::eMiddleButton:
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      mLastMiddleMouseDownContent = mouseContent;
      mLastMiddleMouseDownContentParent = mouseContentParent;
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      if (mLastMiddleMouseDownContent == mouseContent ||
          mLastMiddleMouseDownContentParent == mouseContent ||
          mLastMiddleMouseDownContent == mouseContentParent) {
        aEvent->clickCount = mMClickCount;
        mMClickCount = 0;
      } else {
        aEvent->clickCount = 0;
      }
      mLastMiddleMouseDownContent = nsnull;
      mLastMiddleMouseDownContentParent = nsnull;
    }
    break;

  case nsMouseEvent::eRightButton:
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      mLastRightMouseDownContent = mouseContent;
      mLastRightMouseDownContentParent = mouseContentParent;
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      if (mLastRightMouseDownContent == mouseContent ||
          mLastRightMouseDownContentParent == mouseContent ||
          mLastRightMouseDownContent == mouseContentParent) {
        aEvent->clickCount = mRClickCount;
        mRClickCount = 0;
      } else {
        aEvent->clickCount = 0;
      }
      mLastRightMouseDownContent = nsnull;
      mLastRightMouseDownContentParent = nsnull;
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
    event.inputSource = aEvent->inputSource;

    nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
    if (presShell) {
      nsCOMPtr<nsIContent> mouseContent = GetEventTargetContent(aEvent);

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
        event2.inputSource = aEvent->inputSource;

        ret = presShell->HandleEventWithTarget(&event2, mCurrentTarget,
                                               mouseContent, aStatus);
      }
    }
  }
  return ret;
}

nsIFrame*
nsEventStateManager::GetEventTarget()
{
  nsIPresShell *shell;
  if (mCurrentTarget ||
      !mPresContext ||
      !(shell = mPresContext->GetPresShell())) {
    return mCurrentTarget;
  }

  if (mCurrentTargetContent) {
    mCurrentTarget = mPresContext->GetPrimaryFrameFor(mCurrentTargetContent);
    if (mCurrentTarget) {
      return mCurrentTarget;
    }
  }

  nsIFrame* frame = shell->GetEventTargetFrame();
  return (mCurrentTarget = frame);
}

already_AddRefed<nsIContent>
nsEventStateManager::GetEventTargetContent(nsEvent* aEvent)
{
  if (aEvent &&
      (aEvent->message == NS_FOCUS_CONTENT ||
       aEvent->message == NS_BLUR_CONTENT)) {
    nsCOMPtr<nsIContent> content = GetFocusedContent();
    return content.forget();
  }

  if (mCurrentTargetContent) {
    nsCOMPtr<nsIContent> content = mCurrentTargetContent;
    return content.forget();
  }

  nsIContent *content = nsnull;

  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    content = presShell->GetEventTargetContent(aEvent).get();
  }

  
  
  if (!content && mCurrentTarget) {
    mCurrentTarget->GetContentForEvent(mPresContext, aEvent, &content);
  }

  return content;
}

static Element*
GetLabelTarget(nsIContent* aPossibleLabel)
{
  nsHTMLLabelElement* label = nsHTMLLabelElement::FromContent(aPossibleLabel);
  if (!label)
    return nsnull;

  return label->GetLabeledElement();
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


inline void
nsEventStateManager::DoStateChange(Element* aElement, nsEventStates aState,
                                   PRBool aAddState)
{
  if (aAddState) {
    aElement->AddStates(aState);
  } else {
    aElement->RemoveStates(aState);
  }
}


inline void
nsEventStateManager::DoStateChange(nsIContent* aContent, nsEventStates aState,
                                   PRBool aStateAdded)
{
  if (aContent->IsElement()) {
    DoStateChange(aContent->AsElement(), aState, aStateAdded);
  }
}


void
nsEventStateManager::UpdateAncestorState(nsIContent* aStartNode,
                                         nsIContent* aStopBefore,
                                         nsEventStates aState,
                                         PRBool aAddState)
{
  for (; aStartNode && aStartNode != aStopBefore;
       aStartNode = aStartNode->GetParent()) {
    
    
    
    if (!aStartNode->IsElement()) {
      continue;
    }
    Element* element = aStartNode->AsElement();
    DoStateChange(element, aState, aAddState);
    Element* labelTarget = GetLabelTarget(element);
    if (labelTarget) {
      DoStateChange(labelTarget, aState, aAddState);
    }
  }
}

PRBool
nsEventStateManager::SetContentState(nsIContent *aContent, nsEventStates aState)
{
  
  
  NS_PRECONDITION(aState == NS_EVENT_STATE_ACTIVE ||
                  aState == NS_EVENT_STATE_HOVER ||
                  aState == NS_EVENT_STATE_DRAGOVER ||
                  aState == NS_EVENT_STATE_URLTARGET,
                  "Unexpected state");

  nsCOMPtr<nsIContent> notifyContent1;
  nsCOMPtr<nsIContent> notifyContent2;
  PRBool updateAncestors;

  if (aState == NS_EVENT_STATE_HOVER || aState == NS_EVENT_STATE_ACTIVE) {
    
    updateAncestors = PR_TRUE;

    
    
    if (mCurrentTarget)
    {
      const nsStyleUserInterface* ui = mCurrentTarget->GetStyleUserInterface();
      if (ui->mUserInput == NS_STYLE_USER_INPUT_NONE)
        return PR_FALSE;
    }

    if (aState == NS_EVENT_STATE_ACTIVE) {
      if (aContent != mActiveContent) {
        notifyContent1 = aContent;
        notifyContent2 = mActiveContent;
        mActiveContent = aContent;
      }
    } else {
      NS_ASSERTION(aState == NS_EVENT_STATE_HOVER, "How did that happen?");
      nsIContent* newHover;
      
      if (mPresContext->IsDynamic()) {
        newHover = aContent;
      } else {
        NS_ASSERTION(!aContent ||
                     aContent->GetCurrentDoc() == mPresContext->PresShell()->GetDocument(),
                     "Unexpected document");
        nsIFrame *frame = aContent ? aContent->GetPrimaryFrame() : nsnull;
        if (frame && nsLayoutUtils::IsViewportScrollbarFrame(frame)) {
          
          
          newHover = aContent;
        } else {
          
          newHover = nsnull;
        }
      }

      if (newHover != mHoverContent) {
        notifyContent1 = newHover;
        notifyContent2 = mHoverContent;
        mHoverContent = newHover;
      }
    }
  } else {
    updateAncestors = PR_FALSE;
    if (aState == NS_EVENT_STATE_DRAGOVER) {
      if (aContent != mDragOverContent) {
        notifyContent1 = aContent;
        notifyContent2 = mDragOverContent;
        mDragOverContent = aContent;
      }
    } else if (aState == NS_EVENT_STATE_URLTARGET) {
      if (aContent != mURLTargetContent) {
        notifyContent1 = aContent;
        notifyContent2 = mURLTargetContent;
        mURLTargetContent = aContent;
      }
    }
  }

  
  
  
  
  
  PRBool content1StateSet = PR_TRUE;
  if (!notifyContent1) {
    
    
    notifyContent1 = notifyContent2;
    notifyContent2 = nsnull;
    content1StateSet = PR_FALSE;
  }

  if (notifyContent1 && mPresContext) {
    EnsureDocument(mPresContext);
    if (mDocument) {
      nsAutoScriptBlocker scriptBlocker;

      if (updateAncestors) {
        nsCOMPtr<nsIContent> commonAncestor =
          FindCommonAncestor(notifyContent1, notifyContent2);
        if (notifyContent2) {
          
          
          
          
          
          UpdateAncestorState(notifyContent2, commonAncestor, aState, PR_FALSE);
        }
        UpdateAncestorState(notifyContent1, commonAncestor, aState,
                            content1StateSet);
      } else {
        if (notifyContent2) {
          DoStateChange(notifyContent2, aState, PR_FALSE);
        }
        DoStateChange(notifyContent1, aState, content1StateSet);
      }
    }
  }

  return PR_TRUE;
}

void
nsEventStateManager::ContentRemoved(nsIDocument* aDocument, nsIContent* aContent)
{
  
  
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm)
    fm->ContentRemoved(aDocument, aContent);

  if (mHoverContent &&
      nsContentUtils::ContentIsDescendantOf(mHoverContent, aContent)) {
    
    
    SetContentState(aContent->GetParent(), NS_EVENT_STATE_HOVER);
  }

  if (mActiveContent &&
      nsContentUtils::ContentIsDescendantOf(mActiveContent, aContent)) {
    
    
    SetContentState(aContent->GetParent(), NS_EVENT_STATE_ACTIVE);
  }

  if (mDragOverContent &&
      nsContentUtils::ContentIsDescendantOf(mDragOverContent, aContent)) {
    mDragOverContent = nsnull;
  }

  if (mLastMouseOverElement &&
      nsContentUtils::ContentIsDescendantOf(mLastMouseOverElement, aContent)) {
    
    mLastMouseOverElement = nsnull;
  }
}

PRBool
nsEventStateManager::EventStatusOK(nsGUIEvent* aEvent)
{
  return !(aEvent->message == NS_MOUSE_BUTTON_DOWN &&
      static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton  && 
      !sNormalLMouseEventInProcess);
}




void
nsEventStateManager::RegisterAccessKey(nsIContent* aContent, PRUint32 aKey)
{
  if (aContent && mAccessKeys.IndexOf(aContent) == -1)
    mAccessKeys.AppendObject(aContent);
}

void
nsEventStateManager::UnregisterAccessKey(nsIContent* aContent, PRUint32 aKey)
{
  if (aContent)
    mAccessKeys.RemoveObject(aContent);
}

PRUint32
nsEventStateManager::GetRegisteredAccessKey(nsIContent* aContent)
{
  NS_ENSURE_ARG(aContent);

  if (mAccessKeys.IndexOf(aContent) == -1)
    return 0;

  nsAutoString accessKey;
  aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);
  return accessKey.First();
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
  nsCOMPtr<nsPIDOMWindow> window(mDocument->GetWindow());
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

  nsCOMPtr<nsPIWindowRoot> root = window->GetTopWindowRoot();
  NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);
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
    case NS_CONTENT_COMMAND_PASTE_TRANSFERABLE:
      cmd = "cmd_pasteTransferable";
      break;
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
  nsCOMPtr<nsIController> controller;
  nsresult rv = root->GetControllerForCommand(cmd, getter_AddRefs(controller));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!controller) {
    
    
    aEvent->mIsEnabled = PR_FALSE;
  } else {
    PRBool canDoIt;
    rv = controller->IsCommandEnabled(cmd, &canDoIt);
    NS_ENSURE_SUCCESS(rv, rv);
    aEvent->mIsEnabled = canDoIt;
    if (canDoIt && !aEvent->mOnlyEnabledCheck) {
      switch (aEvent->message) {
        case NS_CONTENT_COMMAND_PASTE_TRANSFERABLE: {
          nsCOMPtr<nsICommandController> commandController = do_QueryInterface(controller);
          NS_ENSURE_STATE(commandController);

          nsCOMPtr<nsICommandParams> params = do_CreateInstance("@mozilla.org/embedcomp/command-params;1", &rv);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = params->SetISupportsValue("transferable", aEvent->mTransferable);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = commandController->DoCommandWithParams(cmd, params);
          break;
        }
        
        default:
          rv = controller->DoCommand(cmd);
          break;
      }
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  aEvent->mSucceeded = PR_TRUE;
  return NS_OK;
}

nsresult
nsEventStateManager::DoContentCommandScrollEvent(nsContentCommandEvent* aEvent)
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_AVAILABLE);
  nsIPresShell* ps = mPresContext->GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(aEvent->mScroll.mAmount != 0, NS_ERROR_INVALID_ARG);

  nsIScrollableFrame::ScrollUnit scrollUnit;
  switch (aEvent->mScroll.mUnit) {
    case nsContentCommandEvent::eCmdScrollUnit_Line:
      scrollUnit = nsIScrollableFrame::LINES;
      break;
    case nsContentCommandEvent::eCmdScrollUnit_Page:
      scrollUnit = nsIScrollableFrame::PAGES;
      break;
    case nsContentCommandEvent::eCmdScrollUnit_Whole:
      scrollUnit = nsIScrollableFrame::WHOLE;
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }

  aEvent->mSucceeded = PR_TRUE;

  nsIScrollableFrame* sf =
    ps->GetFrameToScrollAsScrollable(nsIPresShell::eEither);
  aEvent->mIsEnabled = sf ? CanScrollOn(sf, aEvent->mScroll.mAmount,
                                        aEvent->mScroll.mIsHorizontal) :
                            PR_FALSE;

  if (!aEvent->mIsEnabled || aEvent->mOnlyEnabledCheck) {
    return NS_OK;
  }

  nsIntPoint pt(0, 0);
  if (aEvent->mScroll.mIsHorizontal) {
    pt.x = aEvent->mScroll.mAmount;
  } else {
    pt.y = aEvent->mScroll.mAmount;
  }

  
  sf->ScrollBy(pt, scrollUnit, nsIScrollableFrame::INSTANT);
  return NS_OK;
}

void
nsEventStateManager::DoQueryScrollTargetInfo(nsQueryContentEvent* aEvent,
                                             nsIFrame* aTargetFrame)
{
  nsMouseScrollEvent* msEvent = aEvent->mInput.mMouseScrollEvent;

  
  
  if (!UseSystemScrollSettingFor(msEvent)) {
    return;
  }

  nsIScrollableFrame::ScrollUnit unit;
  PRBool allowOverrideSystemSettings;
  switch (ComputeWheelActionFor(msEvent, PR_TRUE)) {
    case MOUSE_SCROLL_N_LINES:
      unit = nsIScrollableFrame::LINES;
      allowOverrideSystemSettings = PR_TRUE;
      break;
    case MOUSE_SCROLL_PAGE:
      unit = nsIScrollableFrame::PAGES;
      allowOverrideSystemSettings = PR_FALSE;
      break;
    case MOUSE_SCROLL_PIXELS:
      unit = nsIScrollableFrame::DEVICE_PIXELS;
      allowOverrideSystemSettings = PR_FALSE;
    default:
      
      
      return;
  }

  DoScrollText(aTargetFrame, msEvent, unit,
               allowOverrideSystemSettings, aEvent);
}

void
nsEventStateManager::SetActiveManager(nsEventStateManager* aNewESM,
                                      nsIContent* aContent)
{
  if (sActiveESM && aNewESM != sActiveESM) {
    sActiveESM->SetContentState(nsnull, NS_EVENT_STATE_ACTIVE);
  }
  sActiveESM = aNewESM;
  if (sActiveESM && aContent) {
    sActiveESM->SetContentState(aContent, NS_EVENT_STATE_ACTIVE);
  }
}

void
nsEventStateManager::ClearGlobalActiveContent(nsEventStateManager* aClearer)
{
  if (aClearer) {
    aClearer->SetContentState(nsnull, NS_EVENT_STATE_ACTIVE);
  }
  if (sActiveESM && aClearer != sActiveESM) {
    sActiveESM->SetContentState(nsnull, NS_EVENT_STATE_ACTIVE);
  }
  sActiveESM = nsnull;
}
