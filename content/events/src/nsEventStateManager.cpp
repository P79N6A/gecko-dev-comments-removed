





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
#include "nsIDOMHTMLElement.h"
#include "nsIDOMXULControlElement.h"
#include "nsINameSpaceManager.h"
#include "nsIBaseWindow.h"
#include "nsISelection.h"
#include "nsFrameSelection.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
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
#include "nsIDOMWheelEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMUIEvent.h"
#include "nsDOMDragEvent.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIDOMMozBrowserFrame.h"
#include "nsIMozBrowserFrame.h"

#include "nsCaret.h"

#include "nsSubDocumentFrame.h"
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
#include "mozilla/LookAndFeel.h"
#include "mozilla/Attributes.h"
#include "sampler.h"

#include "nsIDOMClientRect.h"

#ifdef XP_MACOSX
#import <ApplicationServices/ApplicationServices.h>
#endif

using namespace mozilla;
using namespace mozilla::dom;



#define NS_USER_INTERACTION_INTERVAL 5000 // ms

static bool sLeftClickOnly = true;
static bool sKeyCausesActivation = true;
static PRUint32 sESMInstanceCount = 0;
static PRInt32 sChromeAccessModifier = 0, sContentAccessModifier = 0;
PRInt32 nsEventStateManager::sUserInputEventDepth = 0;
bool nsEventStateManager::sNormalLMouseEventInProcess = false;
nsEventStateManager* nsEventStateManager::sActiveESM = nullptr;
nsIDocument* nsEventStateManager::sMouseOverDocument = nullptr;
nsWeakFrame nsEventStateManager::sLastDragOverFrame = nullptr;
nsIntPoint nsEventStateManager::sLastRefPoint = nsIntPoint(0,0);
nsIntPoint nsEventStateManager::sLastScreenPoint = nsIntPoint(0,0);
nsIntPoint nsEventStateManager::sLastClientPoint = nsIntPoint(0,0);
bool nsEventStateManager::sIsPointerLocked = false;

nsWeakPtr nsEventStateManager::sPointerLockedElement;

nsWeakPtr nsEventStateManager::sPointerLockedDoc;
nsCOMPtr<nsIContent> nsEventStateManager::sDragOverContent = nullptr;

static PRUint32 gMouseOrKeyboardEventCounter = 0;
static nsITimer* gUserInteractionTimer = nullptr;
static nsITimerCallback* gUserInteractionTimerCallback = nullptr;

TimeStamp nsEventStateManager::sHandlingInputStart;

nsEventStateManager::WheelPrefs*
  nsEventStateManager::WheelPrefs::sInstance = nullptr;
nsEventStateManager::DeltaAccumulator*
  nsEventStateManager::DeltaAccumulator::sInstance = nullptr;

static inline PRInt32
RoundDown(double aDouble)
{
  return (aDouble > 0) ? static_cast<PRInt32>(floor(aDouble)) :
                         static_cast<PRInt32>(ceil(aDouble));
}

static inline bool
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
  nsCOMPtr<nsIDOMWindow> domwin = doc ? doc->GetWindow() : nullptr;
  nsIURI* uri = doc ? doc->GetDocumentURI() : nullptr;

  printf("DS %p  Type %s  Cnt %d  Doc %p  DW %p  EM %p%c",
    static_cast<void*>(parentAsDocShell.get()),
    type==nsIDocShellTreeItem::typeChrome?"Chrome":"Content",
    childWebshellCount, static_cast<void*>(doc.get()),
    static_cast<void*>(domwin.get()),
    static_cast<void*>(presContext ? presContext->EventStateManager() : nullptr),
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

class nsUITimerCallback MOZ_FINAL : public nsITimerCallback
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
    obs->NotifyObservers(nullptr, "user-interaction-inactive", nullptr);
    if (gUserInteractionTimer) {
      gUserInteractionTimer->Cancel();
      NS_RELEASE(gUserInteractionTimer);
    }
  } else {
    obs->NotifyObservers(nullptr, "user-interaction-active", nullptr);
    nsEventStateManager::UpdateUserActivityTimer();
  }
  mPreviousCount = gMouseOrKeyboardEventCounter;
  return NS_OK;
}


#define NS_MODIFIER_SHIFT    1
#define NS_MODIFIER_CONTROL  2
#define NS_MODIFIER_ALT      4
#define NS_MODIFIER_META     8
#define NS_MODIFIER_OS       16

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
    case nsIDOMKeyEvent::DOM_VK_WIN:     return NS_MODIFIER_OS;
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

struct DeltaValues
{
  DeltaValues() : deltaX(0.0), deltaY(0.0) {}

  DeltaValues(double aDeltaX, double aDeltaY) :
    deltaX(aDeltaX), deltaY(aDeltaY)
  {
  }

  explicit DeltaValues(widget::WheelEvent* aEvent) :
    deltaX(aEvent->deltaX), deltaY(aEvent->deltaY)
  {
  }

  double deltaX;
  double deltaY;
};

class nsMouseWheelTransaction {
public:
  static nsIFrame* GetTargetFrame() { return sTargetFrame; }
  static void BeginTransaction(nsIFrame* aTargetFrame,
                               widget::WheelEvent* aEvent);
  
  
  static bool UpdateTransaction(widget::WheelEvent* aEvent);
  static void EndTransaction();
  static void OnEvent(nsEvent* aEvent);
  static void Shutdown();
  static PRUint32 GetTimeoutTime();


  static DeltaValues AccelerateWheelDelta(widget::WheelEvent* aEvent,
                                          bool aAllowScrollSpeedOverride);

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
  static DeltaValues OverrideSystemScrollSpeed(widget::WheelEvent* aEvent);
  static double ComputeAcceleratedWheelDelta(double aDelta, PRInt32 aFactor);

  static nsWeakFrame sTargetFrame;
  static PRUint32    sTime;        
  static PRUint32    sMouseMoved;  
  static nsITimer*   sTimer;
  static PRInt32     sScrollSeriesCounter;
};

nsWeakFrame nsMouseWheelTransaction::sTargetFrame(nullptr);
PRUint32    nsMouseWheelTransaction::sTime        = 0;
PRUint32    nsMouseWheelTransaction::sMouseMoved  = 0;
nsITimer*   nsMouseWheelTransaction::sTimer       = nullptr;
PRInt32     nsMouseWheelTransaction::sScrollSeriesCounter = 0;

static bool
OutOfTime(PRUint32 aBaseTime, PRUint32 aThreshold)
{
  PRUint32 now = PR_IntervalToMilliseconds(PR_IntervalNow());
  return (now - aBaseTime > aThreshold);
}

static bool
CanScrollInRange(nscoord aMin, nscoord aValue, nscoord aMax, double aDirection)
{
  return aDirection > 0.0 ? aValue < static_cast<double>(aMax) :
                            static_cast<double>(aMin) < aValue;
}

static bool
CanScrollOn(nsIScrollableFrame* aScrollFrame, double aDeltaX, double aDeltaY)
{
  MOZ_ASSERT(aScrollFrame);
  NS_ASSERTION(aDeltaX || aDeltaY,
               "One of the delta values must be non-zero at least");

  nsPoint scrollPt = aScrollFrame->GetScrollPosition();
  nsRect scrollRange = aScrollFrame->GetScrollRange();

  return ((aDeltaX && CanScrollInRange(scrollRange.x, scrollPt.x,
                                       scrollRange.XMost(), aDeltaX)) ||
          (aDeltaY && CanScrollInRange(scrollRange.y, scrollPt.y,
                                       scrollRange.YMost(), aDeltaY)));
}

void
nsMouseWheelTransaction::BeginTransaction(nsIFrame* aTargetFrame,
                                          widget::WheelEvent* aEvent)
{
  NS_ASSERTION(!sTargetFrame, "previous transaction is not finished!");
  sTargetFrame = aTargetFrame;
  sScrollSeriesCounter = 0;
  if (!UpdateTransaction(aEvent)) {
    NS_ERROR("BeginTransaction is called even cannot scroll the frame");
    EndTransaction();
  }
}

bool
nsMouseWheelTransaction::UpdateTransaction(widget::WheelEvent* aEvent)
{
  nsIScrollableFrame* sf = GetTargetFrame()->GetScrollTargetFrame();
  NS_ENSURE_TRUE(sf, false);

  if (!CanScrollOn(sf, aEvent->deltaX, aEvent->deltaY)) {
    OnFailToScrollTarget();
    
    
    return false;
  }

  SetTimeout();

  if (sScrollSeriesCounter != 0 && OutOfTime(sTime, kScrollSeriesTimeout))
    sScrollSeriesCounter = 0;
  sScrollSeriesCounter++;

  
  
  
  
  sTime = PR_IntervalToMilliseconds(PR_IntervalNow());
  sMouseMoved = 0;
  return true;
}

void
nsMouseWheelTransaction::EndTransaction()
{
  if (sTimer)
    sTimer->Cancel();
  sTargetFrame = nullptr;
  sScrollSeriesCounter = 0;
}

void
nsMouseWheelTransaction::OnEvent(nsEvent* aEvent)
{
  if (!sTargetFrame)
    return;

  if (OutOfTime(sTime, GetTimeoutTime())) {
    
    
    
    
    OnTimeout(nullptr, nullptr);
    return;
  }

  switch (aEvent->message) {
    case NS_WHEEL_WHEEL:
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

  if (Preferences::GetBool("test.mousescroll", false)) {
    
    nsContentUtils::DispatchTrustedEvent(
                      sTargetFrame->GetContent()->OwnerDoc(),
                      sTargetFrame->GetContent(),
                      NS_LITERAL_STRING("MozMouseScrollFailed"),
                      true, true);
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

  if (Preferences::GetBool("test.mousescroll", false)) {
    
    nsContentUtils::DispatchTrustedEvent(
                      frame->GetContent()->OwnerDoc(),
                      frame->GetContent(),
                      NS_LITERAL_STRING("MozMouseScrollTransactionTimeout"),
                      true, true);
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
  sTimer->InitWithFuncCallback(OnTimeout, nullptr, GetTimeoutTime(),
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

DeltaValues
nsMouseWheelTransaction::AccelerateWheelDelta(widget::WheelEvent* aEvent,
                                              bool aAllowScrollSpeedOverride)
{
  DeltaValues result(aEvent);

  
  if (aEvent->deltaMode != nsIDOMWheelEvent::DOM_DELTA_LINE) {
    return result;
  }

  if (aAllowScrollSpeedOverride) {
    result = OverrideSystemScrollSpeed(aEvent);
  }

  
  PRInt32 start = GetAccelerationStart();
  if (start >= 0 && sScrollSeriesCounter >= start) {
    PRInt32 factor = GetAccelerationFactor();
    if (factor > 0) {
      result.deltaX = ComputeAcceleratedWheelDelta(result.deltaX, factor);
      result.deltaY = ComputeAcceleratedWheelDelta(result.deltaY, factor);
    }
  }

  return result;
}

double
nsMouseWheelTransaction::ComputeAcceleratedWheelDelta(double aDelta,
                                                      PRInt32 aFactor)
{
  if (aDelta == 0.0) {
    return 0;
  }

  return (aDelta * sScrollSeriesCounter * (double)aFactor / 10);
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

DeltaValues
nsMouseWheelTransaction::OverrideSystemScrollSpeed(widget::WheelEvent* aEvent)
{
  MOZ_ASSERT(sTargetFrame, "We don't have mouse scrolling transaction");
  MOZ_ASSERT(aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_LINE);

  DeltaValues result(aEvent);

  
  
  
  if ((!aEvent->lineOrPageDeltaX && !aEvent->lineOrPageDeltaY) ||
      (static_cast<double>(aEvent->lineOrPageDeltaX) != aEvent->deltaX) ||
      (static_cast<double>(aEvent->lineOrPageDeltaY) != aEvent->deltaY)) {
    return result;
  }

  
  if (sTargetFrame !=
        sTargetFrame->PresContext()->PresShell()->GetRootScrollFrame()) {
    return result;
  }

  
  
  
  
  nsCOMPtr<nsIWidget> widget(sTargetFrame->GetNearestWidget());
  NS_ENSURE_TRUE(widget, result);
  PRInt32 overriddenDeltaX = 0, overriddenDeltaY = 0;
  if (aEvent->lineOrPageDeltaX) {
    nsresult rv =
      widget->OverrideSystemMouseScrollSpeed(aEvent->lineOrPageDeltaX,
                                             true, overriddenDeltaX);
    if (NS_FAILED(rv)) {
      return result;
    }
  }
  if (aEvent->lineOrPageDeltaY) {
    nsresult rv =
      widget->OverrideSystemMouseScrollSpeed(aEvent->lineOrPageDeltaY,
                                             false, overriddenDeltaY);
    if (NS_FAILED(rv)) {
      return result;
    }
  }
  return DeltaValues(overriddenDeltaX, overriddenDeltaY);
}





nsEventStateManager::nsEventStateManager()
  : mLockCursor(0),
    mPreLockPoint(0,0),
    mCurrentTarget(nullptr),
    mLastMouseOverFrame(nullptr),
    
    mGestureDownPoint(0,0),
    mPresContext(nullptr),
    mLClickCount(0),
    mMClickCount(0),
    mRClickCount(0),
    m_haveShutdown(false),
    mClickHoldContextMenu(false)
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
  "dom.popup_allowed_events",
  nullptr
};

nsresult
nsEventStateManager::Init()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, true);

  if (sESMInstanceCount == 1) {
    sKeyCausesActivation =
      Preferences::GetBool("accessibility.accesskeycausesactivation",
                           sKeyCausesActivation);
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
    Preferences::GetBool("ui.click_hold_context_menus", false);

  return NS_OK;
}

nsEventStateManager::~nsEventStateManager()
{
  if (sActiveESM == this) {
    sActiveESM = nullptr;
  }
  if (mClickHoldContextMenu)
    KillClickHoldTimer();

  if (mDocument == sMouseOverDocument)
    sMouseOverDocument = nullptr;

  --sESMInstanceCount;
  if(sESMInstanceCount == 0) {
    nsMouseWheelTransaction::Shutdown();
    if (gUserInteractionTimerCallback) {
      gUserInteractionTimerCallback->Notify(nullptr);
      NS_RELEASE(gUserInteractionTimerCallback);
    }
    if (gUserInteractionTimer) {
      gUserInteractionTimer->Cancel();
      NS_RELEASE(gUserInteractionTimer);
    }
    WheelPrefs::Shutdown();
    DeltaAccumulator::Shutdown();
  }

  if (sDragOverContent && sDragOverContent->OwnerDoc() == mDocument) {
    sDragOverContent = nullptr;
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
  m_haveShutdown = true;
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
        Preferences::GetBool("ui.click_hold_context_menus", false);
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
                                    nsEventStatus* aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  NS_ENSURE_ARG(aPresContext);
  if (!aEvent) {
    NS_ERROR("aEvent is null.  This should never happen.");
    return NS_ERROR_NULL_POINTER;
  }

  mCurrentTarget = aTargetFrame;
  mCurrentTargetContent = nullptr;

  
  if (NS_EVENT_NEEDS_FRAME(aEvent)) {
    NS_ASSERTION(mCurrentTarget, "mCurrentTarget is null.  this should not happen.  see bug #13007");
    if (!mCurrentTarget) return NS_ERROR_NULL_POINTER;
  }
#ifdef DEBUG
  if (NS_IS_DRAG_EVENT(aEvent) && sIsPointerLocked) {
    NS_ASSERTION(sIsPointerLocked,
      "sIsPointerLocked is true. Drag events should be suppressed when the pointer is locked.");
  }
#endif
  
  
  if (NS_IS_TRUSTED_EVENT(aEvent) &&
      ((NS_IS_MOUSE_EVENT_STRUCT(aEvent) &&
       IsMouseEventReal(aEvent)) ||
       aEvent->eventStructType == NS_WHEEL_EVENT)) {
    if (!sIsPointerLocked) {
      sLastScreenPoint = nsDOMUIEvent::CalculateScreenPoint(aPresContext, aEvent);
      sLastClientPoint = nsDOMUIEvent::CalculateClientPoint(aPresContext, aEvent, nullptr);
    }
  }

  
  
  if (NS_IS_TRUSTED_EVENT(aEvent) &&
      ((aEvent->eventStructType == NS_MOUSE_EVENT  &&
        IsMouseEventReal(aEvent) &&
        aEvent->message != NS_MOUSE_ENTER &&
        aEvent->message != NS_MOUSE_EXIT) ||
       aEvent->eventStructType == NS_WHEEL_EVENT ||
       aEvent->eventStructType == NS_KEY_EVENT)) {
    if (gMouseOrKeyboardEventCounter == 0) {
      nsCOMPtr<nsIObserverService> obs =
        mozilla::services::GetObserverService();
      if (obs) {
        obs->NotifyObservers(nullptr, "user-interaction-active", nullptr);
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
      sNormalLMouseEventInProcess = true;
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
        sNormalLMouseEventInProcess = false;
        
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
    
    
    FlushPendingEvents(aPresContext);
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
      if (keyEvent->IsShift())
        modifierMask |= NS_MODIFIER_SHIFT;
      if (keyEvent->IsControl())
        modifierMask |= NS_MODIFIER_CONTROL;
      if (keyEvent->IsAlt())
        modifierMask |= NS_MODIFIER_ALT;
      if (keyEvent->IsMeta())
        modifierMask |= NS_MODIFIER_META;
      if (keyEvent->IsOS())
        modifierMask |= NS_MODIFIER_OS;

      
      if (modifierMask && (modifierMask == sChromeAccessModifier ||
                           modifierMask == sContentAccessModifier))
        HandleAccessKey(aPresContext, keyEvent, aStatus, nullptr,
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
  case NS_WHEEL_WHEEL:
    {
      NS_ASSERTION(NS_IS_TRUSTED_EVENT(aEvent),
                   "Untrusted wheel event shouldn't be here");

      nsIContent* content = GetFocusedContent();
      if (content)
        mCurrentTargetContent = content;

      widget::WheelEvent* wheelEvent = static_cast<widget::WheelEvent*>(aEvent);
      WheelPrefs::GetInstance()->ApplyUserPrefsToDelta(wheelEvent);

      
      
      
      
      DeltaAccumulator::GetInstance()->
        InitLineOrPageDelta(aTargetFrame, this, wheelEvent);
    }
    break;
  case NS_QUERY_SELECTED_TEXT:
    DoQuerySelectedText(static_cast<nsQueryContentEvent*>(aEvent));
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
  case NS_SELECTION_SET:
    {
      nsSelectionEvent *selectionEvent =
          static_cast<nsSelectionEvent*>(aEvent);
      if (IsTargetCrossProcess(selectionEvent)) {
        
        if (GetCrossProcessTarget()->SendSelectionEvent(*selectionEvent))
          selectionEvent->mSucceeded = true;
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
    if (NS_IS_TRUSTED_EVENT(aEvent)) {
      
      
      nsCompositionEvent *compositionEvent =
        static_cast<nsCompositionEvent*>(aEvent);
      nsQueryContentEvent selectedText(true, NS_QUERY_SELECTED_TEXT,
                                       compositionEvent->widget);
      DoQuerySelectedText(&selectedText);
      NS_ASSERTION(selectedText.mSucceeded, "Failed to get selected text");
      compositionEvent->data = selectedText.mReply.mString;
    }
    
  case NS_COMPOSITION_UPDATE:
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

static bool
IsAccessKeyTarget(nsIContent* aContent, nsIFrame* aFrame, nsAString& aKey)
{
  if (!aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::accesskey, aKey,
                             eIgnoreCase))
    return false;

  nsCOMPtr<nsIDOMXULDocument> xulDoc =
    do_QueryInterface(aContent->OwnerDoc());
  if (!xulDoc && !aContent->IsXUL())
    return true;

    
  if (!aFrame)
    return false;

  if (aFrame->IsFocusable())
    return true;

  if (!aFrame->IsVisibleConsideringAncestors())
    return false;

  
  nsCOMPtr<nsIDOMXULControlElement> control(do_QueryInterface(aContent));
  if (control)
    return true;

  if (aContent->IsHTML()) {
    nsIAtom* tag = aContent->Tag();

    
    
    if (tag == nsGkAtoms::area ||
        tag == nsGkAtoms::label ||
        tag == nsGkAtoms::legend)
      return true;

  } else if (aContent->IsXUL()) {
    
    
    if (aContent->Tag() == nsGkAtoms::label)
      return true;
  }

  return false;
}

bool
nsEventStateManager::ExecuteAccessKey(nsTArray<PRUint32>& aAccessCharCodes,
                                      bool aIsTrustedEvent)
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
        bool shouldActivate = sKeyCausesActivation;
        while (shouldActivate && ++count <= length) {
          nsIContent *oc = mAccessKeys[(start + count) % length];
          nsIFrame *of = oc->GetPrimaryFrame();
          if (IsAccessKeyTarget(oc, of, accessKey))
            shouldActivate = false;
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
        return true;
      }
    }
  }
  return false;
}

bool
nsEventStateManager::GetAccessKeyLabelPrefix(nsAString& aPrefix)
{
  aPrefix.Truncate();
  nsAutoString separator, modifierText;
  nsContentUtils::GetModifierSeparatorText(separator);

  nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
  PRInt32 modifier = GetAccessModifierMask(container);

  if (modifier & NS_MODIFIER_CONTROL) {
    nsContentUtils::GetControlText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifier & NS_MODIFIER_META) {
    nsContentUtils::GetMetaText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifier & NS_MODIFIER_OS) {
    nsContentUtils::GetOSText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifier & NS_MODIFIER_ALT) {
    nsContentUtils::GetAltText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifier & NS_MODIFIER_SHIFT) {
    nsContentUtils::GetShiftText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  return !aPrefix.IsEmpty();
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
    
    bool isTrusted = NS_IS_TRUSTED_EVENT(aEvent);
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
          esm->HandleAccessKey(subPC, aEvent, aStatus, nullptr,
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

bool
nsEventStateManager::DispatchCrossProcessEvent(nsEvent* aEvent,
                                               nsFrameLoader* aFrameLoader,
                                               nsEventStatus *aStatus) {
  PBrowserParent* remoteBrowser = aFrameLoader->GetRemoteBrowser();
  TabParent* remote = static_cast<TabParent*>(remoteBrowser);
  if (!remote) {
    return false;
  }

  switch (aEvent->eventStructType) {
  case NS_MOUSE_EVENT: {
    nsMouseEvent* mouseEvent = static_cast<nsMouseEvent*>(aEvent);
    return remote->SendRealMouseEvent(*mouseEvent);
  }
  case NS_KEY_EVENT: {
    nsKeyEvent* keyEvent = static_cast<nsKeyEvent*>(aEvent);
    return remote->SendRealKeyEvent(*keyEvent);
  }
  case NS_WHEEL_EVENT: {
    widget::WheelEvent* wheelEvent = static_cast<widget::WheelEvent*>(aEvent);
    return remote->SendMouseWheelEvent(*wheelEvent);
  }
  case NS_TOUCH_EVENT: {
    
    
    *aStatus = nsEventStatus_eConsumeNoDefault;
    nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
    return remote->SendRealTouchEvent(*touchEvent);
  }
  default: {
    MOZ_NOT_REACHED("Attempt to send non-whitelisted event?");
    return false;
  }
  }
}

bool
nsEventStateManager::IsRemoteTarget(nsIContent* target) {
  if (!target) {
    return false;
  }

  
  if ((target->Tag() == nsGkAtoms::browser ||
       target->Tag() == nsGkAtoms::iframe) &&
      target->IsXUL() &&
      target->AttrValueIs(kNameSpaceID_None, nsGkAtoms::Remote,
                          nsGkAtoms::_true, eIgnoreCase)) {
    return true;
  }

  
  nsCOMPtr<nsIMozBrowserFrame> browserFrame = do_QueryInterface(target);
  if (browserFrame) {
    bool isBrowser = false;
    browserFrame->GetReallyIsBrowser(&isBrowser);
    if (isBrowser) {
      return !!TabParent::GetFrom(target);
    }
  }

  return false;
}

bool
CrossProcessSafeEvent(const nsEvent& aEvent)
{
  switch (aEvent.eventStructType) {
  case NS_KEY_EVENT:
  case NS_WHEEL_EVENT:
    return true;
  case NS_MOUSE_EVENT:
    switch (aEvent.message) {
    case NS_MOUSE_BUTTON_DOWN:
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_MOVE:
      return true;
    default:
      return false;
    }
  case NS_TOUCH_EVENT:
    switch (aEvent.message) {
    case NS_TOUCH_START:
    case NS_TOUCH_MOVE:
    case NS_TOUCH_END:
    case NS_TOUCH_CANCEL:
      return true;
    default:
      return false;
    }
  default:
    return false;
  }
}

bool
nsEventStateManager::HandleCrossProcessEvent(nsEvent *aEvent,
                                             nsIFrame* aTargetFrame,
                                             nsEventStatus *aStatus) {
  if (*aStatus == nsEventStatus_eConsumeNoDefault ||
      aEvent->flags & NS_EVENT_FLAG_DONT_FORWARD_CROSS_PROCESS ||
      !CrossProcessSafeEvent(*aEvent)) {
    return false;
  }

  
  
  
  
  nsAutoTArray<nsCOMPtr<nsIContent>, 1> targets;
  if (aEvent->eventStructType != NS_TOUCH_EVENT ||
      aEvent->message == NS_TOUCH_START) {
    
    
    nsIContent* target = mCurrentTargetContent;
    if (!target && aTargetFrame) {
      target = aTargetFrame->GetContent();
    }
    if (IsRemoteTarget(target)) {
      targets.AppendElement(target);
    }
  } else {
    
    
    
    
    
    
    
    nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
    const nsTArray<nsCOMPtr<nsIDOMTouch> >& touches = touchEvent->touches;
    for (PRUint32 i = 0; i < touches.Length(); ++i) {
      nsIDOMTouch* touch = touches[i];
      
      
      
      
      if (!touch || !touch->mChanged) {
        continue;
      }
      nsCOMPtr<nsIDOMEventTarget> targetPtr;
      touch->GetTarget(getter_AddRefs(targetPtr));
      if (!targetPtr) {
        continue;
      }
      nsCOMPtr<nsIContent> target = do_QueryInterface(targetPtr);
      if (IsRemoteTarget(target) && !targets.Contains(target)) {
        targets.AppendElement(target);
      }
    }
  }

  if (targets.Length() == 0) {
    return false;
  }

  
  
  bool dispatched = false;
  for (PRUint32 i = 0; i < targets.Length(); ++i) {
    nsIContent* target = targets[i];
    nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(target);
    if (!loaderOwner) {
      continue;
    }

    nsRefPtr<nsFrameLoader> frameLoader = loaderOwner->GetFrameLoader();
    if (!frameLoader) {
      continue;
    }

    PRUint32 eventMode;
    frameLoader->GetEventMode(&eventMode);
    if (eventMode == nsIFrameLoader::EVENT_MODE_DONT_FORWARD_TO_CHILD) {
      continue;
    }

    
    
    if (aEvent->eventStructType != NS_TOUCH_EVENT) {
      nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                                aTargetFrame);
      aEvent->refPoint =
        pt.ToNearestPixels(mPresContext->AppUnitsPerDevPixel());
    } else {
      nsIFrame* targetFrame = frameLoader->GetPrimaryFrameOfOwningContent();
      aEvent->refPoint = nsIntPoint();
      
      nsPoint offset =
        nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, targetFrame);
      nsIntPoint intOffset =
        offset.ToNearestPixels(mPresContext->AppUnitsPerDevPixel());
      nsTouchEvent* touchEvent = static_cast<nsTouchEvent*>(aEvent);
      
      
      const nsTArray<nsCOMPtr<nsIDOMTouch> >& touches = touchEvent->touches;
      for (PRUint32 i = 0; i < touches.Length(); ++i) {
        nsIDOMTouch* touch = touches[i];
        if (touch) {
          touch->mRefPoint += intOffset;
        }
      }
    }

    dispatched |= DispatchCrossProcessEvent(aEvent, frameLoader, aStatus);
  }
  return dispatched;
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
    mClickHoldTimer = nullptr;
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
    mClickHoldTimer = nullptr;
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
    bool allowedToDispatch = true;

    if (mGestureDownContent->IsXUL()) {
      if (tag == nsGkAtoms::scrollbar ||
          tag == nsGkAtoms::scrollbarbutton ||
          tag == nsGkAtoms::button)
        allowedToDispatch = false;
      else if (tag == nsGkAtoms::toolbarbutton) {
        
        
        if (nsContentUtils::HasNonEmptyAttr(mGestureDownContent,
                kNameSpaceID_None, nsGkAtoms::container)) {
          allowedToDispatch = false;
        } else {
          
            
          if (mGestureDownContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::open,
                                               nsGkAtoms::_true, eCaseMatters)) {
            allowedToDispatch = false;
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
                             type == NS_FORM_INPUT_NUMBER ||
                             type == NS_FORM_TEXTAREA);
      }
      else if (tag == nsGkAtoms::applet ||
               tag == nsGkAtoms::embed  ||
               tag == nsGkAtoms::object) {
        allowedToDispatch = false;
      }
    }

    if (allowedToDispatch) {
      
      nsCOMPtr<nsIWidget> targetWidget(mCurrentTarget->GetNearestWidget());
      
      nsMouseEvent event(true, NS_CONTEXTMENU,
                         targetWidget,
                         nsMouseEvent::eReal);
      event.clickCount = 1;
      FillInEventFromGestureDown(&event);
        
      
      if (mCurrentTarget)
      {
        nsRefPtr<nsFrameSelection> frameSel =
          mCurrentTarget->GetFrameSelection();
        
        if (frameSel && frameSel->GetMouseDownState()) {
          
          
          frameSel->SetMouseDownState(false);
        }
      }

      
      nsEventDispatcher::Dispatch(mGestureDownContent, mPresContext, &event,
                                  nullptr, &status);

      
      
      
      
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

  inDownFrame->GetContentForEvent(inDownEvent,
                                  getter_AddRefs(mGestureDownContent));

  mGestureDownFrameOwner = inDownFrame->GetContent();
  mGestureModifiers = inDownEvent->modifiers;
  mGestureDownButtons = inDownEvent->buttons;

  if (mClickHoldContextMenu) {
    
    CreateClickHoldTimer(aPresContext, inDownFrame, inDownEvent);
  }
}








void
nsEventStateManager::StopTrackingDragGesture()
{
  mGestureDownContent = nullptr;
  mGestureDownFrameOwner = nullptr;
}

void
nsEventStateManager::FillInEventFromGestureDown(nsMouseEvent* aEvent)
{
  NS_ASSERTION(aEvent->widget == mCurrentTarget->GetNearestWidget(),
               "Incorrect widget in event");

  
  
  
  nsIntPoint tmpPoint = aEvent->widget->WidgetToScreenOffset();
  aEvent->refPoint = mGestureDownPoint - tmpPoint;
  aEvent->modifiers = mGestureModifiers;
  aEvent->buttons = mGestureDownButtons;
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
      pixelThresholdX =
        LookAndFeel::GetInt(LookAndFeel::eIntID_DragThresholdX, 0);
      pixelThresholdY =
        LookAndFeel::GetInt(LookAndFeel::eIntID_DragThresholdY, 0);
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

      nsCOMPtr<nsISelection> selection;
      nsCOMPtr<nsIContent> eventContent, targetContent;
      mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(eventContent));
      if (eventContent)
        DetermineDragTarget(aPresContext, eventContent, dataTransfer,
                            getter_AddRefs(selection), getter_AddRefs(targetContent));

      
      
      StopTrackingDragGesture();

      if (!targetContent)
        return;

      sLastDragOverFrame = nullptr;
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
      nsEventDispatcher::Dispatch(targetContent, aPresContext, &startEvent, nullptr,
                                  &status);

      nsDragEvent* event = &startEvent;
      if (status != nsEventStatus_eConsumeNoDefault) {
        status = nsEventStatus_eIgnore;
        nsEventDispatcher::Dispatch(targetContent, aPresContext, &gestureEvent, nullptr,
                                    &status);
        event = &gestureEvent;
      }

      
      
      
      dataTransfer->SetReadOnly();

      if (status != nsEventStatus_eConsumeNoDefault) {
        bool dragStarted = DoDefaultDragStart(aPresContext, event, dataTransfer,
                                              targetContent, selection);
        if (dragStarted) {
          sActiveESM = nullptr;
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
                                         nsISelection** aSelection,
                                         nsIContent** aTargetNode)
{
  *aTargetNode = nullptr;

  nsCOMPtr<nsISupports> container = aPresContext->GetContainer();
  nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(container);
  if (!window)
    return;

  
  
  
  
  
  bool canDrag;
  nsCOMPtr<nsIContent> dragDataNode;
  bool wasAlt = (mGestureModifiers & widget::MODIFIER_ALT) != 0;
  nsresult rv = nsContentAreaDragDrop::GetDragData(window, mGestureDownContent,
                                                   aSelectionTarget, wasAlt,
                                                   aDataTransfer, &canDrag, aSelection,
                                                   getter_AddRefs(dragDataNode));
  if (NS_FAILED(rv) || !canDrag)
    return;

  
  
  
  nsIContent* dragContent = mGestureDownContent;
  if (dragDataNode)
    dragContent = dragDataNode;
  else if (*aSelection)
    dragContent = aSelectionTarget;

  nsIContent* originalDragContent = dragContent;

  
  
  
  
  if (!*aSelection) {
    while (dragContent) {
      nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(dragContent);
      if (htmlElement) {
        bool draggable = false;
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

bool
nsEventStateManager::DoDefaultDragStart(nsPresContext* aPresContext,
                                        nsDragEvent* aDragEvent,
                                        nsDOMDataTransfer* aDataTransfer,
                                        nsIContent* aDragTarget,
                                        nsISelection* aSelection)
{
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");
  if (!dragService)
    return false;

  
  
  
  
  
  
  
  nsCOMPtr<nsIDragSession> dragSession;
  dragService->GetCurrentSession(getter_AddRefs(dragSession));
  if (dragSession)
    return true;

  
  
  PRUint32 count = 0;
  if (aDataTransfer)
    aDataTransfer->GetMozItemCount(&count);
  if (!count)
    return false;

  
  
  
  
  nsCOMPtr<nsIDOMNode> dragTarget;
  nsCOMPtr<nsIDOMElement> dragTargetElement;
  aDataTransfer->GetDragTarget(getter_AddRefs(dragTargetElement));
  dragTarget = do_QueryInterface(dragTargetElement);
  if (!dragTarget) {
    dragTarget = do_QueryInterface(aDragTarget);
    if (!dragTarget)
      return false;
  }

  
  
  PRUint32 action;
  aDataTransfer->GetEffectAllowedInt(&action);
  if (action == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED)
    action = nsIDragService::DRAGDROP_ACTION_COPY |
             nsIDragService::DRAGDROP_ACTION_MOVE |
             nsIDragService::DRAGDROP_ACTION_LINK;

  
  PRInt32 imageX, imageY;
  nsIDOMElement* dragImage = aDataTransfer->GetDragImage(&imageX, &imageY);

  nsCOMPtr<nsISupportsArray> transArray;
  aDataTransfer->GetTransferables(getter_AddRefs(transArray), dragTarget);
  if (!transArray)
    return false;

  
  
  
  nsCOMPtr<nsIDOMEvent> domEvent;
  NS_NewDOMDragEvent(getter_AddRefs(domEvent), aPresContext, aDragEvent);

  nsCOMPtr<nsIDOMDragEvent> domDragEvent = do_QueryInterface(domEvent);
  
  

  
  
  
  
  if (!dragImage && aSelection) {
    dragService->InvokeDragSessionWithSelection(aSelection, transArray,
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

  return true;
}

nsresult
nsEventStateManager::GetMarkupDocumentViewer(nsIMarkupDocumentViewer** aMv)
{
  *aMv = nullptr;

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if(!fm) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));

  nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(focusedWindow);
  if(!ourWindow) return NS_ERROR_FAILURE;

  nsIDOMWindow *rootWindow = ourWindow->GetPrivateRoot();
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
      !content->OwnerDoc()->IsXUL())
    {
      
      PRInt32 change = (adjustment > 0) ? -1 : 1;

      if (Preferences::GetBool("browser.zoom.full") || content->GetCurrentDoc()->IsSyntheticDocument()) {
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
    return nullptr;

  if (aFrame->GetStyleDisplay()->mPosition == NS_STYLE_POSITION_FIXED &&
      nsLayoutUtils::IsReallyFixedPos(aFrame))
    return aFrame->PresContext()->GetPresShell()->GetRootScrollFrame();

  return aFrame->GetParent();
}

void
nsEventStateManager::DispatchLegacyMouseScrollEvents(nsIFrame* aTargetFrame,
                                                     widget::WheelEvent* aEvent,
                                                     nsEventStatus* aStatus)
{
  MOZ_ASSERT(aEvent);
  MOZ_ASSERT(aStatus);

  if (!aTargetFrame || *aStatus == nsEventStatus_eConsumeNoDefault) {
    return;
  }

  
  
  nsIScrollableFrame* scrollTarget =
    ComputeScrollTarget(aTargetFrame, aEvent, false);

  nsIFrame* scrollFrame = do_QueryFrame(scrollTarget);
  nsPresContext* pc =
    scrollFrame ? scrollFrame->PresContext() : aTargetFrame->PresContext();

  
  nsSize scrollAmount = GetScrollAmount(pc, aEvent, scrollTarget);
  nsIntSize scrollAmountInCSSPixels(
    nsPresContext::AppUnitsToIntCSSPixels(scrollAmount.width),
    nsPresContext::AppUnitsToIntCSSPixels(scrollAmount.height));

  
  
  
  
  
  
  
  PRInt32 scrollDeltaX, scrollDeltaY, pixelDeltaX, pixelDeltaY;
  switch (aEvent->deltaMode) {
    case nsIDOMWheelEvent::DOM_DELTA_PAGE:
      scrollDeltaX =
        !aEvent->lineOrPageDeltaX ? 0 :
          (aEvent->lineOrPageDeltaX > 0  ? nsIDOMUIEvent::SCROLL_PAGE_DOWN :
                                           nsIDOMUIEvent::SCROLL_PAGE_UP);
      scrollDeltaY =
        !aEvent->lineOrPageDeltaY ? 0 :
          (aEvent->lineOrPageDeltaY > 0  ? nsIDOMUIEvent::SCROLL_PAGE_DOWN :
                                           nsIDOMUIEvent::SCROLL_PAGE_UP);
      pixelDeltaX = RoundDown(aEvent->deltaX * scrollAmountInCSSPixels.width);
      pixelDeltaY = RoundDown(aEvent->deltaY * scrollAmountInCSSPixels.height);
      break;

    case nsIDOMWheelEvent::DOM_DELTA_LINE:
      scrollDeltaX = aEvent->lineOrPageDeltaX;
      scrollDeltaY = aEvent->lineOrPageDeltaY;
      pixelDeltaX = RoundDown(aEvent->deltaX * scrollAmountInCSSPixels.width);
      pixelDeltaY = RoundDown(aEvent->deltaY * scrollAmountInCSSPixels.height);
      break;

    case nsIDOMWheelEvent::DOM_DELTA_PIXEL:
      scrollDeltaX = aEvent->lineOrPageDeltaX;
      scrollDeltaY = aEvent->lineOrPageDeltaY;
      pixelDeltaX = RoundDown(aEvent->deltaX);
      pixelDeltaY = RoundDown(aEvent->deltaY);
      break;

    default:
      MOZ_NOT_REACHED("Invalid deltaMode value comes");
      return;
  }

  
  
  
  
  

  nsWeakFrame targetFrame(aTargetFrame);

  nsEventStatus statusX = *aStatus;
  nsEventStatus statusY = *aStatus;
  if (scrollDeltaY) {
    SendLineScrollEvent(aTargetFrame, aEvent, &statusY,
                        scrollDeltaY, DELTA_DIRECTION_Y);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (pixelDeltaY) {
    SendPixelScrollEvent(aTargetFrame, aEvent, &statusY,
                         pixelDeltaY, DELTA_DIRECTION_Y);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (scrollDeltaX) {
    SendLineScrollEvent(aTargetFrame, aEvent, &statusX,
                        scrollDeltaX, DELTA_DIRECTION_X);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (pixelDeltaX) {
    SendPixelScrollEvent(aTargetFrame, aEvent, &statusX,
                         pixelDeltaX, DELTA_DIRECTION_X);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (statusY == nsEventStatus_eConsumeNoDefault ||
      statusX == nsEventStatus_eConsumeNoDefault) {
    *aStatus = nsEventStatus_eConsumeNoDefault;
    return;
  }
  if (statusY == nsEventStatus_eConsumeDoDefault ||
      statusX == nsEventStatus_eConsumeDoDefault) {
    *aStatus = nsEventStatus_eConsumeDoDefault;
  }
}

void
nsEventStateManager::SendLineScrollEvent(nsIFrame* aTargetFrame,
                                         widget::WheelEvent* aEvent,
                                         nsEventStatus* aStatus,
                                         PRInt32 aDelta,
                                         DeltaDirection aDeltaDirection)
{
  nsCOMPtr<nsIContent> targetContent = aTargetFrame->GetContent();
  if (!targetContent)
    targetContent = GetFocusedContent();
  if (!targetContent)
    return;

  while (targetContent->IsNodeOfType(nsINode::eTEXT)) {
    targetContent = targetContent->GetParent();
  }

  nsMouseScrollEvent event(NS_IS_TRUSTED_EVENT(aEvent), NS_MOUSE_SCROLL,
                           aEvent->widget);
  if (*aStatus == nsEventStatus_eConsumeNoDefault) {
    event.flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }
  event.refPoint = aEvent->refPoint;
  event.widget = aEvent->widget;
  event.time = aEvent->time;
  event.modifiers = aEvent->modifiers;
  event.buttons = aEvent->buttons;
  event.isHorizontal = (aDeltaDirection == DELTA_DIRECTION_X);
  event.delta = aDelta;
  event.inputSource = aEvent->inputSource;

  nsEventDispatcher::Dispatch(targetContent, aTargetFrame->PresContext(),
                              &event, nullptr, aStatus);
}

void
nsEventStateManager::SendPixelScrollEvent(nsIFrame* aTargetFrame,
                                          widget::WheelEvent* aEvent,
                                          nsEventStatus* aStatus,
                                          PRInt32 aPixelDelta,
                                          DeltaDirection aDeltaDirection)
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

  nsMouseScrollEvent event(NS_IS_TRUSTED_EVENT(aEvent), NS_MOUSE_PIXEL_SCROLL,
                           aEvent->widget);
  if (*aStatus == nsEventStatus_eConsumeNoDefault) {
    event.flags |= NS_EVENT_FLAG_NO_DEFAULT;
  }
  event.refPoint = aEvent->refPoint;
  event.widget = aEvent->widget;
  event.time = aEvent->time;
  event.modifiers = aEvent->modifiers;
  event.buttons = aEvent->buttons;
  event.isHorizontal = (aDeltaDirection == DELTA_DIRECTION_X);
  event.delta = aPixelDelta;
  event.inputSource = aEvent->inputSource;

  nsEventDispatcher::Dispatch(targetContent, aTargetFrame->PresContext(),
                              &event, nullptr, aStatus);
}

nsIScrollableFrame*
nsEventStateManager::ComputeScrollTarget(nsIFrame* aTargetFrame,
                                         widget::WheelEvent* aEvent,
                                         bool aForDefaultAction)
{
  if (aForDefaultAction) {
    
    
    
    
    
    
    
    
    
    nsIFrame* lastScrollFrame = nsMouseWheelTransaction::GetTargetFrame();
    if (lastScrollFrame) {
      nsIScrollableFrame* frameToScroll =
        lastScrollFrame->GetScrollTargetFrame();
      if (frameToScroll) {
        return frameToScroll;
      }
    }
  }

  
  
  
  if (!aEvent->deltaX && !aEvent->deltaY) {
    return nullptr;
  }

  nsIScrollableFrame* frameToScroll = nullptr;
  for (nsIFrame* scrollFrame = aTargetFrame; scrollFrame;
       scrollFrame = GetParentFrameToScroll(scrollFrame)) {
    
    frameToScroll = scrollFrame->GetScrollTargetFrame();
    if (!frameToScroll) {
      continue;
    }

    
    
    if (!aForDefaultAction) {
      return frameToScroll;
    }

    nsPresContext::ScrollbarStyles ss = frameToScroll->GetScrollbarStyles();
    bool hiddenForV = (NS_STYLE_OVERFLOW_HIDDEN == ss.mVertical);
    bool hiddenForH = (NS_STYLE_OVERFLOW_HIDDEN == ss.mHorizontal);
    if ((hiddenForV && hiddenForH) ||
        (aEvent->deltaY && !aEvent->deltaX && hiddenForV) ||
        (aEvent->deltaX && !aEvent->deltaY && hiddenForH)) {
      continue;
    }

    
    
    bool canScroll = CanScrollOn(frameToScroll,
                                 aEvent->deltaX, aEvent->deltaY);
    
    nsIComboboxControlFrame* comboBox = do_QueryFrame(scrollFrame);
    if (comboBox) {
      if (comboBox->IsDroppedDown()) {
        
        return canScroll ? frameToScroll : nullptr;
      }
      
      continue;
    }

    if (canScroll) {
      return frameToScroll;
    }
  }

  nsIFrame* newFrame = nsLayoutUtils::GetCrossDocParentFrame(
      aTargetFrame->PresContext()->FrameManager()->GetRootFrame());
  return newFrame ?
    ComputeScrollTarget(newFrame, aEvent, aForDefaultAction) : nullptr;
}

nsSize
nsEventStateManager::GetScrollAmount(nsPresContext* aPresContext,
                                     widget::WheelEvent* aEvent,
                                     nsIScrollableFrame* aScrollableFrame)
{
  MOZ_ASSERT(aPresContext);
  MOZ_ASSERT(aEvent);

  bool isPage = (aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_PAGE);
  if (aScrollableFrame) {
    return isPage ? aScrollableFrame->GetPageScrollAmount() :
                    aScrollableFrame->GetLineScrollAmount();
  }

  
  if (isPage) {
    return aPresContext->GetVisibleArea().Size();
  }

  
  nsIFrame* rootFrame = aPresContext->PresShell()->GetRootFrame();
  if (!rootFrame) {
    return nsSize(0, 0);
  }
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(rootFrame, getter_AddRefs(fm),
    nsLayoutUtils::FontSizeInflationFor(rootFrame));
  NS_ENSURE_TRUE(fm, nsSize(0, 0));
  PRInt32 fontHeight = fm->MaxHeight();
  return nsSize(fontHeight, fontHeight);
}

void
nsEventStateManager::DoScrollText(nsIScrollableFrame* aScrollableFrame,
                                  widget::WheelEvent* aEvent)
{
  MOZ_ASSERT(aScrollableFrame);
  MOZ_ASSERT(aEvent);

  nsIFrame* scrollFrame = do_QueryFrame(aScrollableFrame);
  MOZ_ASSERT(scrollFrame);
  nsWeakFrame scrollFrameWeak(scrollFrame);

  nsIFrame* lastScrollFrame = nsMouseWheelTransaction::GetTargetFrame();
  if (!lastScrollFrame) {
    nsMouseWheelTransaction::BeginTransaction(scrollFrame, aEvent);
  } else if (lastScrollFrame != scrollFrame) {
    nsMouseWheelTransaction::EndTransaction();
    nsMouseWheelTransaction::BeginTransaction(scrollFrame, aEvent);
  } else {
    nsMouseWheelTransaction::UpdateTransaction(aEvent);
  }

  
  
  
  
  if (!scrollFrameWeak.IsAlive()) {
    nsMouseWheelTransaction::EndTransaction();
    return;
  }

  
  
  nsPresContext* pc = scrollFrame->PresContext();
  nsSize scrollAmount = GetScrollAmount(pc, aEvent, aScrollableFrame);
  nsIntSize scrollAmountInDevPixels(
    pc->AppUnitsToDevPixels(scrollAmount.width),
    pc->AppUnitsToDevPixels(scrollAmount.height));
  nsIntPoint actualDevPixelScrollAmount =
    DeltaAccumulator::GetInstance()->
      ComputeScrollAmountForDefaultAction(aEvent, scrollAmountInDevPixels);

  nsIAtom* origin = nullptr;
  switch (aEvent->deltaMode) {
    case nsIDOMWheelEvent::DOM_DELTA_LINE:
      origin = nsGkAtoms::mouseWheel;
      break;
    case nsIDOMWheelEvent::DOM_DELTA_PAGE:
      origin = nsGkAtoms::pages;
      break;
    case nsIDOMWheelEvent::DOM_DELTA_PIXEL:
      origin = nsGkAtoms::pixels;
      break;
    default:
      MOZ_NOT_REACHED("Invalid deltaMode value comes");
      return;
  }

  
  nsSize pageSize = aScrollableFrame->GetPageScrollAmount();
  nsIntSize devPixelPageSize(pc->AppUnitsToDevPixels(pageSize.width),
                             pc->AppUnitsToDevPixels(pageSize.height));
  if (NS_ABS(actualDevPixelScrollAmount.x) > devPixelPageSize.width) {
    actualDevPixelScrollAmount.x =
      (actualDevPixelScrollAmount.x >= 0) ? devPixelPageSize.width :
                                            -devPixelPageSize.width;
  }

  if (NS_ABS(actualDevPixelScrollAmount.y) > devPixelPageSize.height) {
    actualDevPixelScrollAmount.y =
      (actualDevPixelScrollAmount.y >= 0) ? devPixelPageSize.height :
                                            -devPixelPageSize.height;
  }

  bool isDeltaModePixel =
    (aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_PIXEL);

  nsIScrollableFrame::ScrollMode mode;
  switch (aEvent->scrollType) {
    case widget::WheelEvent::SCROLL_DEFAULT:
      if (isDeltaModePixel) {
        mode = nsIScrollableFrame::NORMAL;
      } else {
        mode = nsIScrollableFrame::SMOOTH;
      }
      break;
    case widget::WheelEvent::SCROLL_SYNCHRONOUSLY:
      mode = nsIScrollableFrame::INSTANT;
      break;
    case widget::WheelEvent::SCROLL_ASYNCHRONOUSELY:
      mode = nsIScrollableFrame::NORMAL;
      break;
    case widget::WheelEvent::SCROLL_SMOOTHLY:
      mode = nsIScrollableFrame::SMOOTH;
      break;
    default:
      MOZ_NOT_REACHED("Invalid scrollType value comes");
      return;
  }

  nsIntPoint overflow;
  aScrollableFrame->ScrollBy(actualDevPixelScrollAmount,
                             nsIScrollableFrame::DEVICE_PIXELS,
                             mode, &overflow, origin);

  if (isDeltaModePixel) {
    aEvent->overflowDeltaX = overflow.x;
    aEvent->overflowDeltaY = overflow.y;
  } else {
    aEvent->overflowDeltaX =
      static_cast<double>(overflow.x) / scrollAmountInDevPixels.width;
    aEvent->overflowDeltaY =
      static_cast<double>(overflow.y) / scrollAmountInDevPixels.height;
  }
  WheelPrefs::GetInstance()->CancelApplyingUserPrefsFromOverflowDelta(aEvent);
}

void
nsEventStateManager::DecideGestureEvent(nsGestureNotifyEvent* aEvent,
                                        nsIFrame* targetFrame)
{

  NS_ASSERTION(aEvent->message == NS_GESTURENOTIFY_EVENT_START,
               "DecideGestureEvent called with a non-gesture event");

  











  nsGestureNotifyEvent::ePanDirection panDirection = nsGestureNotifyEvent::ePanNone;
  bool displayPanFeedback = false;
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
        displayPanFeedback = true;

        nsRect scrollRange = scrollableFrame->GetScrollRange();
        bool canScrollHorizontally = scrollRange.width > 0;

        if (targetFrame->GetType() == nsGkAtoms::menuFrame) {
          
          
          canScrollHorizontally = false;
          displayPanFeedback = false;
        }

        
        
        if (scrollRange.height > 0) {
          panDirection = nsGestureNotifyEvent::ePanVertical;
          break;
        }

        if (canScrollHorizontally) {
          panDirection = nsGestureNotifyEvent::ePanHorizontal;
          displayPanFeedback = false;
        }
      } else { 
        PRUint32 scrollbarVisibility = scrollableFrame->GetScrollbarVisibility();

        
        if (scrollbarVisibility & nsIScrollableFrame::VERTICAL) {
          panDirection = nsGestureNotifyEvent::ePanVertical;
          displayPanFeedback = true;
          break;
        }

        if (scrollbarVisibility & nsIScrollableFrame::HORIZONTAL) {
          panDirection = nsGestureNotifyEvent::ePanHorizontal;
          displayPanFeedback = true;
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
        {&nsGkAtoms::always, &nsGkAtoms::never, nullptr};
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
                                     nsEventStatus* aStatus)
{
  NS_ENSURE_ARG(aPresContext);
  NS_ENSURE_ARG_POINTER(aStatus);

  HandleCrossProcessEvent(aEvent, aTargetFrame, aStatus);

  mCurrentTarget = aTargetFrame;
  mCurrentTargetContent = nullptr;

  
  
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
        
        
        nsIPresShell::SetCapturingContent(nullptr, 0);
        break;
      }

      nsCOMPtr<nsIContent> activeContent;
      if (nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> newFocus;      
        bool suppressBlur = false;
        if (mCurrentTarget) {
          mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(newFocus));
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
            newFocus = nullptr;
            break;
          }

          PRInt32 tabIndexUnused;
          if (currFrame->IsFocusable(&tabIndexUnused, true)) {
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
        
        
        ret = CheckForAndDispatchClick(presContext, (nsMouseEvent*)aEvent,
                                       aStatus);
      }

      nsIPresShell *shell = presContext->GetPresShell();
      if (shell) {
        nsRefPtr<nsFrameSelection> frameSelection = shell->FrameSelection();
        frameSelection->SetMouseDownState(false);
      }
    }
    break;
  case NS_WHEEL_WHEEL:
    {
      MOZ_ASSERT(NS_IS_TRUSTED_EVENT(aEvent));

      if (*aStatus == nsEventStatus_eConsumeNoDefault) {
        break;
      }

      widget::WheelEvent* wheelEvent = static_cast<widget::WheelEvent*>(aEvent);
      switch (WheelPrefs::GetInstance()->ComputeActionFor(wheelEvent)) {
        case WheelPrefs::ACTION_SCROLL: {
          
          
          nsIScrollableFrame* scrollTarget =
            ComputeScrollTarget(aTargetFrame, wheelEvent, true);
          wheelEvent->overflowDeltaX = wheelEvent->deltaX;
          wheelEvent->overflowDeltaY = wheelEvent->deltaY;
          WheelPrefs::GetInstance()->
            CancelApplyingUserPrefsFromOverflowDelta(wheelEvent);
          if (scrollTarget) {
            DoScrollText(scrollTarget, wheelEvent);
          } else {
            nsMouseWheelTransaction::EndTransaction();
          }
          break;
        }
        case WheelPrefs::ACTION_HISTORY:
          DoScrollHistory(wheelEvent->GetPreferredIntDelta());
          break;

        case WheelPrefs::ACTION_ZOOM:
          DoScrollZoom(aTargetFrame, wheelEvent->GetPreferredIntDelta());
          break;

        default:
          break;
      }
      *aStatus = nsEventStatus_eConsumeNoDefault;
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

      
      dragSession->SetOnlyChromeDrop(false);
      if (mPresContext) {
        EnsureDocument(mPresContext);
      }
      bool isChromeDoc = nsContentUtils::IsChromeDoc(mDocument);

      
      
      nsCOMPtr<nsIDOMDataTransfer> dataTransfer;
      nsCOMPtr<nsIDOMDataTransfer> initialDataTransfer;
      dragSession->GetDataTransfer(getter_AddRefs(initialDataTransfer));

      nsDragEvent *dragEvent = (nsDragEvent*)aEvent;

      
      
      UpdateDragDataTransfer(dragEvent);

      
      
      
      
      
      
      
      
      
      PRUint32 dropEffect = nsIDragService::DRAGDROP_ACTION_NONE;
      if (nsEventStatus_eConsumeNoDefault == *aStatus) {
        
        if (dragEvent->dataTransfer) {
          
          dataTransfer = do_QueryInterface(dragEvent->dataTransfer);
          dataTransfer->GetDropEffectInt(&dropEffect);
        }
        else {
          
          
          
          
          
          
          
          dataTransfer = initialDataTransfer;

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
        
        dragSession->SetOnlyChromeDrop(true);
      }

      
      
      if (initialDataTransfer)
        initialDataTransfer->SetDropEffectInt(dropEffect);
    }
    break;

  case NS_DRAGDROP_DROP:
    {
      
      if (mCurrentTarget && nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> targetContent;
        mCurrentTarget->GetContentForEvent(aEvent,
                                           getter_AddRefs(targetContent));

        nsCOMPtr<nsIWidget> widget = mCurrentTarget->GetNearestWidget();
        nsDragEvent event(NS_IS_TRUSTED_EVENT(aEvent), NS_DRAGDROP_DRAGDROP, widget);

        nsMouseEvent* mouseEvent = static_cast<nsMouseEvent*>(aEvent);
        event.refPoint = mouseEvent->refPoint;
        if (mouseEvent->widget) {
          event.refPoint += mouseEvent->widget->WidgetToScreenOffset();
        }
        event.refPoint -= widget->WidgetToScreenOffset();
        event.modifiers = mouseEvent->modifiers;
        event.buttons = mouseEvent->buttons;
        event.inputSource = mouseEvent->inputSource;

        nsEventStatus status = nsEventStatus_eIgnore;
        nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
        if (presShell) {
          presShell->HandleEventWithTarget(&event, mCurrentTarget,
                                           targetContent, &status);
        }
      }
      sLastDragOverFrame = nullptr;
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
      
      if (!keyEvent->IsAlt()) {
        switch(keyEvent->keyCode) {
          case NS_VK_TAB:
          case NS_VK_F6:
            EnsureDocument(mPresContext);
            nsIFocusManager* fm = nsFocusManager::GetFocusManager();
            if (fm && mDocument) {
              
              bool isDocMove = ((nsInputEvent*)aEvent)->IsControl() ||
                                 (keyEvent->keyCode == NS_VK_F6);
              PRUint32 dir =
                static_cast<nsInputEvent*>(aEvent)->IsShift() ?
                  (isDocMove ? static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_BACKWARDDOC) :
                               static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_BACKWARD)) :
                  (isDocMove ? static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_FORWARDDOC) :
                               static_cast<PRUint32>(nsIFocusManager::MOVEFOCUS_FORWARD));
              nsCOMPtr<nsIDOMElement> result;
              fm->MoveFocus(mDocument->GetWindow(), nullptr, dir,
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
      mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(targetContent));
      SetContentState(targetContent, NS_EVENT_STATE_HOVER);
    }
    break;

#ifdef XP_MACOSX
  case NS_MOUSE_ACTIVATE:
    if (mCurrentTarget) {
      nsCOMPtr<nsIContent> targetContent;
      mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(targetContent));
      if (!NodeAllowsClickThrough(targetContent)) {
        *aStatus = nsEventStatus_eConsumeNoDefault;
      }
    }
    break;
#endif
  }

  
  mCurrentTarget = nullptr;
  mCurrentTargetContent = nullptr;

  return ret;
}

bool
nsEventStateManager::RemoteQueryContentEvent(nsEvent *aEvent)
{
  nsQueryContentEvent *queryEvent =
      static_cast<nsQueryContentEvent*>(aEvent);
  if (!IsTargetCrossProcess(queryEvent)) {
    return false;
  }
  
  GetCrossProcessTarget()->HandleQueryContentEvent(*queryEvent);
  return true;
}

TabParent*
nsEventStateManager::GetCrossProcessTarget()
{
  return TabParent::GetIMETabParent();
}

bool
nsEventStateManager::IsTargetCrossProcess(nsGUIEvent *aEvent)
{
  
  
  nsIContent *focusedContent = GetFocusedContent();
  if (focusedContent && focusedContent->IsEditable())
    return false;
  return TabParent::GetIMETabParent() != nullptr;
}

void
nsEventStateManager::NotifyDestroyPresContext(nsPresContext* aPresContext)
{
  nsIMEStateManager::OnDestroyPresContext(aPresContext);
  if (mHoverContent) {
    
    
    
    
    SetContentState(nullptr, NS_EVENT_STATE_HOVER);
  }
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
  if (aTargetFrame && IsRemoteTarget(aTargetFrame->GetContent())) {
    return;
  }

  PRInt32 cursor = NS_STYLE_CURSOR_DEFAULT;
  imgIContainer* container = nullptr;
  bool haveHotspot = false;
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

  if (Preferences::GetBool("ui.use_activity_cursor", false)) {
    
    nsCOMPtr<nsISupports> pcContainer = aPresContext->GetContainer();
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(pcContainer));
    if (!docShell) return;
    PRUint32 busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
    docShell->GetBusyFlags(&busyFlags);

    
    
    if (busyFlags & nsIDocShell::BUSY_FLAGS_BUSY &&
          (cursor == NS_STYLE_CURSOR_AUTO || cursor == NS_STYLE_CURSOR_DEFAULT))
    {
      cursor = NS_STYLE_CURSOR_SPINNING;
      container = nullptr;
    }
  }

  if (aTargetFrame) {
    SetCursor(cursor, container, haveHotspot, hotspotX, hotspotY,
              aTargetFrame->GetNearestWidget(), false);
  }

  if (mLockCursor || NS_STYLE_CURSOR_AUTO != cursor) {
    *aStatus = nsEventStatus_eConsumeDoDefault;
  }
}

nsresult
nsEventStateManager::SetCursor(PRInt32 aCursor, imgIContainer* aContainer,
                               bool aHaveHotspot,
                               float aHotspotX, float aHotspotY,
                               nsIWidget* aWidget, bool aLockCursor)
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
  
  
  
  if (sIsPointerLocked &&
      (aMessage == NS_MOUSELEAVE ||
       aMessage == NS_MOUSEENTER ||
       aMessage == NS_MOUSE_ENTER_SYNTH ||
       aMessage == NS_MOUSE_EXIT_SYNTH)) {
    mCurrentTargetContent = nullptr;
    nsCOMPtr<Element> pointerLockedElement =
      do_QueryReferent(nsEventStateManager::sPointerLockedElement);
    if (!pointerLockedElement) {
      NS_WARNING("Should have pointer locked element, but didn't.");
      return nullptr;
    }
    nsCOMPtr<nsIContent> content = do_QueryInterface(pointerLockedElement);
    return mPresContext->GetPrimaryFrameFor(content);
  }

  SAMPLE_LABEL("Input", "DispatchMouseEvent");
  nsEventStatus status = nsEventStatus_eIgnore;
  nsMouseEvent event(NS_IS_TRUSTED_EVENT(aEvent), aMessage, aEvent->widget,
                     nsMouseEvent::eReal);
  event.refPoint = aEvent->refPoint;
  event.modifiers = ((nsMouseEvent*)aEvent)->modifiers;
  event.buttons = ((nsMouseEvent*)aEvent)->buttons;
  event.pluginEvent = ((nsMouseEvent*)aEvent)->pluginEvent;
  event.relatedTarget = aRelatedContent;
  event.inputSource = static_cast<nsMouseEvent*>(aEvent)->inputSource;

  nsWeakFrame previousTarget = mCurrentTarget;

  mCurrentTargetContent = aTargetContent;

  nsIFrame* targetFrame = nullptr;
  if (aTargetContent) {
    nsESMEventCB callback(aTargetContent);
    nsEventDispatcher::Dispatch(aTargetContent, mPresContext, &event, nullptr,
                                &status, &callback);

    
    
    
    if (mPresContext) {
      targetFrame = mPresContext->GetPrimaryFrameFor(aTargetContent);
    }
  }

  mCurrentTargetContent = nullptr;
  mCurrentTarget = previousTarget;

  return targetFrame;
}

class MouseEnterLeaveDispatcher
{
public:
  MouseEnterLeaveDispatcher(nsEventStateManager* aESM,
                            nsIContent* aTarget, nsIContent* aRelatedTarget,
                            nsGUIEvent* aEvent, PRUint32 aType)
  : mESM(aESM), mEvent(aEvent), mType(aType)
  {
    nsPIDOMWindow* win =
      aTarget ? aTarget->OwnerDoc()->GetInnerWindow() : nullptr;
    if (win && win->HasMouseEnterLeaveEventListeners()) {
      mRelatedTarget = aRelatedTarget ?
        aRelatedTarget->FindFirstNonNativeAnonymous() : nullptr;
      nsINode* commonParent = nullptr;
      if (aTarget && aRelatedTarget) {
        commonParent =
          nsContentUtils::GetCommonAncestor(aTarget, aRelatedTarget);
      }
      nsIContent* current = aTarget;
      
      while (current && current != commonParent) {
        if (!current->IsInNativeAnonymousSubtree()) {
          mTargets.AppendObject(current);
        }
        
        current = current->GetParent();
      }
    }
  }

  ~MouseEnterLeaveDispatcher()
  {
    if (mType == NS_MOUSEENTER) {
      for (PRInt32 i = mTargets.Count() - 1; i >= 0; --i) {
        mESM->DispatchMouseEvent(mEvent, mType, mTargets[i], mRelatedTarget);
      }
    } else {
      for (PRInt32 i = 0; i < mTargets.Count(); ++i) {
        mESM->DispatchMouseEvent(mEvent, mType, mTargets[i], mRelatedTarget);
      }
    }
  }

  nsEventStateManager*   mESM;
  nsCOMArray<nsIContent> mTargets;
  nsCOMPtr<nsIContent>   mRelatedTarget;
  nsGUIEvent*            mEvent;
  PRUint32               mType;
};

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
          
          kidESM->NotifyMouseOut(aEvent, nullptr);
        }
      }
    }
  }
  
  
  if (!mLastMouseOverElement)
    return;

  
  
  mFirstMouseOutEventElement = mLastMouseOverElement;

  
  
  
  
  if (!aMovingInto) {
    
    SetContentState(nullptr, NS_EVENT_STATE_HOVER);
  }

  MouseEnterLeaveDispatcher leaveDispatcher(this, mLastMouseOverElement, aMovingInto,
                                            aEvent, NS_MOUSELEAVE);

  
  DispatchMouseEvent(aEvent, NS_MOUSE_EXIT_SYNTH,
                     mLastMouseOverElement, aMovingInto);
  
  mLastMouseOverFrame = nullptr;
  mLastMouseOverElement = nullptr;
  
  
  mFirstMouseOutEventElement = nullptr;
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

  MouseEnterLeaveDispatcher enterDispatcher(this, aContent, lastMouseOverElement,
                                            aEvent, NS_MOUSEENTER);
  
  NotifyMouseOut(aEvent, aContent);

  
  
  mFirstMouseOverEventElement = aContent;
  
  SetContentState(aContent, NS_EVENT_STATE_HOVER);
  
  
  mLastMouseOverFrame = DispatchMouseEvent(aEvent, NS_MOUSE_ENTER_SYNTH,
                                           aContent, lastMouseOverElement);
  mLastMouseOverElement = aContent;
  
  
  mFirstMouseOverEventElement = nullptr;
}




static nsIntPoint
GetWindowInnerRectCenter(nsPIDOMWindow* aWindow,
                         nsIWidget* aWidget,
                         nsPresContext* aContext)
{
  NS_ENSURE_TRUE(aWindow && aWidget && aContext, nsIntPoint(0,0));

  float cssInnerX = 0.0;
  aWindow->GetMozInnerScreenX(&cssInnerX);
  PRInt32 innerX = PRInt32(NS_round(aContext->CSSPixelsToDevPixels(cssInnerX)));

  float cssInnerY = 0.0;
  aWindow->GetMozInnerScreenY(&cssInnerY);
  PRInt32 innerY = PRInt32(NS_round(aContext->CSSPixelsToDevPixels(cssInnerY)));
 
  PRInt32 innerWidth = 0;
  aWindow->GetInnerWidth(&innerWidth);

  PRInt32 innerHeight = 0;
  aWindow->GetInnerHeight(&innerHeight);
 
  nsIntRect screen;
  aWidget->GetScreenBounds(screen);

  return nsIntPoint(innerX - screen.x + innerWidth / 2,
                    innerY - screen.y + innerHeight / 2);
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
      if (sIsPointerLocked && aEvent->widget) {
        
        
        nsIntPoint center = GetWindowInnerRectCenter(mDocument->GetWindow(),
                                                     aEvent->widget,
                                                     mPresContext);
        aEvent->lastRefPoint = center;
        if (aEvent->refPoint != center) {
          
          
          
          aEvent->widget->SynthesizeNativeMouseMove(center);
        }
      } else {
        aEvent->lastRefPoint = sLastRefPoint;
      }

      
      sLastRefPoint = aEvent->refPoint;

      
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

      NotifyMouseOut(aEvent, nullptr);
    }
    break;
  }

  
  mCurrentTargetContent = targetBeforeEvent;
}

void
nsEventStateManager::SetPointerLock(nsIWidget* aWidget,
                                    nsIContent* aElement)
{
  
  sIsPointerLocked = !!aElement;

  if (!aWidget) {
    return;
  }

  
  nsMouseWheelTransaction::EndTransaction();

  
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");

  if (sIsPointerLocked) {
    
    mPreLockPoint = sLastRefPoint;

    
    
    
    sLastRefPoint = GetWindowInnerRectCenter(aElement->OwnerDoc()->GetWindow(),
                                             aWidget,
                                             mPresContext);
    aWidget->SynthesizeNativeMouseMove(sLastRefPoint);

    
    nsIPresShell::SetCapturingContent(aElement, CAPTURE_POINTERLOCK);

    
    if (dragService) {
      dragService->Suppress();
    }
  } else {
    
    
    
    
    sLastRefPoint = mPreLockPoint;
    aWidget->SynthesizeNativeMouseMove(mPreLockPoint);

    
    nsIPresShell::SetCapturingContent(nullptr, CAPTURE_POINTERLOCK);

    
    if (dragService) {
      dragService->Unsuppress();
    }
  }
}

void
nsEventStateManager::GenerateDragDropEnterExit(nsPresContext* aPresContext,
                                               nsGUIEvent* aEvent)
{
  
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aEvent->message) {
  case NS_DRAGDROP_OVER:
    {
      
      
      if (sLastDragOverFrame != mCurrentTarget) {
        
        nsCOMPtr<nsIContent> lastContent;
        nsCOMPtr<nsIContent> targetContent;
        mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(targetContent));

        if (sLastDragOverFrame) {
          
          sLastDragOverFrame->GetContentForEvent(aEvent, getter_AddRefs(lastContent));

          FireDragEnterOrExit(sLastDragOverFrame->PresContext(),
                              aEvent, NS_DRAGDROP_EXIT_SYNTH,
                              targetContent, lastContent, sLastDragOverFrame);
        }

        FireDragEnterOrExit(aPresContext, aEvent, NS_DRAGDROP_ENTER,
                            lastContent, targetContent, mCurrentTarget);

        if (sLastDragOverFrame) {
          FireDragEnterOrExit(sLastDragOverFrame->PresContext(),
                              aEvent, NS_DRAGDROP_LEAVE_SYNTH,
                              targetContent, lastContent, sLastDragOverFrame);
        }

        sLastDragOverFrame = mCurrentTarget;
      }
    }
    break;

  case NS_DRAGDROP_EXIT:
    {
      
      if (sLastDragOverFrame) {
        nsCOMPtr<nsIContent> lastContent;
        sLastDragOverFrame->GetContentForEvent(aEvent, getter_AddRefs(lastContent));

        nsRefPtr<nsPresContext> lastDragOverFramePresContext = sLastDragOverFrame->PresContext();
        FireDragEnterOrExit(lastDragOverFramePresContext,
                            aEvent, NS_DRAGDROP_EXIT_SYNTH,
                            nullptr, lastContent, sLastDragOverFrame);
        FireDragEnterOrExit(lastDragOverFramePresContext,
                            aEvent, NS_DRAGDROP_LEAVE_SYNTH,
                            nullptr, lastContent, sLastDragOverFrame);

        sLastDragOverFrame = nullptr;
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
  event.modifiers = ((nsMouseEvent*)aEvent)->modifiers;
  event.buttons = ((nsMouseEvent*)aEvent)->buttons;
  event.relatedTarget = aRelatedTarget;
  event.inputSource = static_cast<nsMouseEvent*>(aEvent)->inputSource;

  mCurrentTargetContent = aTargetContent;

  if (aTargetContent != aRelatedTarget) {
    
    if (aTargetContent)
      nsEventDispatcher::Dispatch(aTargetContent, aPresContext, &event,
                                  nullptr, &status);

    
    if (status == nsEventStatus_eConsumeNoDefault || aMsg == NS_DRAGDROP_EXIT)
      SetContentState((aMsg == NS_DRAGDROP_ENTER) ? aTargetContent : nullptr,
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
    if (initialDataTransfer) {
      
      nsAutoString mozCursor;
      dragEvent->dataTransfer->GetMozCursor(mozCursor);
      initialDataTransfer->SetMozCursor(mozCursor);
    }
  }
}

nsresult
nsEventStateManager::SetClickCount(nsPresContext* aPresContext,
                                   nsMouseEvent *aEvent,
                                   nsEventStatus* aStatus)
{
  nsCOMPtr<nsIContent> mouseContent;
  nsIContent* mouseContentParent = nullptr;
  mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(mouseContent));
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
      mLastLeftMouseDownContent = nullptr;
      mLastLeftMouseDownContentParent = nullptr;
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
      mLastMiddleMouseDownContent = nullptr;
      mLastMiddleMouseDownContentParent = nullptr;
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
      mLastRightMouseDownContent = nullptr;
      mLastRightMouseDownContentParent = nullptr;
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
    
    
    if (aEvent->widget && !aEvent->widget->IsEnabled()) {
      return ret;
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
    event.modifiers = aEvent->modifiers;
    event.buttons = aEvent->buttons;
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
        event2.modifiers = aEvent->modifiers;
        event2.buttons = aEvent->buttons;
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

  nsIContent *content = nullptr;

  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    content = presShell->GetEventTargetContent(aEvent).get();
  }

  
  
  if (!content && mCurrentTarget) {
    mCurrentTarget->GetContentForEvent(aEvent, &content);
  }

  return content;
}

static Element*
GetLabelTarget(nsIContent* aPossibleLabel)
{
  nsHTMLLabelElement* label = nsHTMLLabelElement::FromContent(aPossibleLabel);
  if (!label)
    return nullptr;

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
  return nullptr;
}

static Element*
GetParentElement(Element* aElement)
{
  nsIContent* p = aElement->GetParent();
  return (p && p->IsElement()) ? p->AsElement() : nullptr;
}


void
nsEventStateManager::SetFullScreenState(Element* aElement, bool aIsFullScreen)
{
  DoStateChange(aElement, NS_EVENT_STATE_FULL_SCREEN, aIsFullScreen);
  Element* ancestor = aElement;
  while ((ancestor = GetParentElement(ancestor))) {
    DoStateChange(ancestor, NS_EVENT_STATE_FULL_SCREEN_ANCESTOR, aIsFullScreen);
  }
}


inline void
nsEventStateManager::DoStateChange(Element* aElement, nsEventStates aState,
                                   bool aAddState)
{
  if (aAddState) {
    aElement->AddStates(aState);
  } else {
    aElement->RemoveStates(aState);
  }
}


inline void
nsEventStateManager::DoStateChange(nsIContent* aContent, nsEventStates aState,
                                   bool aStateAdded)
{
  if (aContent->IsElement()) {
    DoStateChange(aContent->AsElement(), aState, aStateAdded);
  }
}


void
nsEventStateManager::UpdateAncestorState(nsIContent* aStartNode,
                                         nsIContent* aStopBefore,
                                         nsEventStates aState,
                                         bool aAddState)
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

  if (aAddState) {
    
    
    
    
    
    
    
    
    
    
    
    
    for ( ; aStartNode; aStartNode = aStartNode->GetParent()) {
      if (!aStartNode->IsElement()) {
        continue;
      }

      Element* labelTarget = GetLabelTarget(aStartNode->AsElement());
      if (labelTarget && !labelTarget->State().HasState(aState)) {
        DoStateChange(labelTarget, aState, true);
      }
    }
  }
}

bool
nsEventStateManager::SetContentState(nsIContent *aContent, nsEventStates aState)
{
  
  
  NS_PRECONDITION(aState == NS_EVENT_STATE_ACTIVE ||
                  aState == NS_EVENT_STATE_HOVER ||
                  aState == NS_EVENT_STATE_DRAGOVER ||
                  aState == NS_EVENT_STATE_URLTARGET,
                  "Unexpected state");

  nsCOMPtr<nsIContent> notifyContent1;
  nsCOMPtr<nsIContent> notifyContent2;
  bool updateAncestors;

  if (aState == NS_EVENT_STATE_HOVER || aState == NS_EVENT_STATE_ACTIVE) {
    
    updateAncestors = true;

    
    
    if (mCurrentTarget)
    {
      const nsStyleUserInterface* ui = mCurrentTarget->GetStyleUserInterface();
      if (ui->mUserInput == NS_STYLE_USER_INPUT_NONE)
        return false;
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
        nsIFrame *frame = aContent ? aContent->GetPrimaryFrame() : nullptr;
        if (frame && nsLayoutUtils::IsViewportScrollbarFrame(frame)) {
          
          
          newHover = aContent;
        } else {
          
          newHover = nullptr;
        }
      }

      if (newHover != mHoverContent) {
        notifyContent1 = newHover;
        notifyContent2 = mHoverContent;
        mHoverContent = newHover;
      }
    }
  } else {
    updateAncestors = false;
    if (aState == NS_EVENT_STATE_DRAGOVER) {
      if (aContent != sDragOverContent) {
        notifyContent1 = aContent;
        notifyContent2 = sDragOverContent;
        sDragOverContent = aContent;
      }
    } else if (aState == NS_EVENT_STATE_URLTARGET) {
      if (aContent != mURLTargetContent) {
        notifyContent1 = aContent;
        notifyContent2 = mURLTargetContent;
        mURLTargetContent = aContent;
      }
    }
  }

  
  
  
  
  
  bool content1StateSet = true;
  if (!notifyContent1) {
    
    
    notifyContent1 = notifyContent2;
    notifyContent2 = nullptr;
    content1StateSet = false;
  }

  if (notifyContent1 && mPresContext) {
    EnsureDocument(mPresContext);
    if (mDocument) {
      nsAutoScriptBlocker scriptBlocker;

      if (updateAncestors) {
        nsCOMPtr<nsIContent> commonAncestor =
          FindCommonAncestor(notifyContent1, notifyContent2);
        if (notifyContent2) {
          
          
          
          
          
          UpdateAncestorState(notifyContent2, commonAncestor, aState, false);
        }
        UpdateAncestorState(notifyContent1, commonAncestor, aState,
                            content1StateSet);
      } else {
        if (notifyContent2) {
          DoStateChange(notifyContent2, aState, false);
        }
        DoStateChange(notifyContent1, aState, content1StateSet);
      }
    }
  }

  return true;
}

void
nsEventStateManager::ContentRemoved(nsIDocument* aDocument, nsIContent* aContent)
{
  




  if (aContent->IsHTML() &&
      (aContent->Tag() == nsGkAtoms::a || aContent->Tag() == nsGkAtoms::area) &&
      (aContent->AsElement()->State().HasAtLeastOneOfStates(NS_EVENT_STATE_FOCUS |
                                                            NS_EVENT_STATE_HOVER))) {
    nsGenericHTMLElement* element = static_cast<nsGenericHTMLElement*>(aContent);
    element->LeaveLink(element->GetPresContext());
  }

  
  
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

  if (sDragOverContent &&
      sDragOverContent->OwnerDoc() == aContent->OwnerDoc() &&
      nsContentUtils::ContentIsDescendantOf(sDragOverContent, aContent)) {
    sDragOverContent = nullptr;
  }

  if (mLastMouseOverElement &&
      nsContentUtils::ContentIsDescendantOf(mLastMouseOverElement, aContent)) {
    
    mLastMouseOverElement = nullptr;
  }
}

bool
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
  NS_PRECONDITION(nullptr != aPresContext, "nullptr ptr");
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
    return nullptr;

  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  return nsFocusManager::GetFocusedDescendant(mDocument->GetWindow(), false,
                                              getter_AddRefs(focusedWindow));
}




bool
nsEventStateManager::IsShellVisible(nsIDocShell* aShell)
{
  NS_ASSERTION(aShell, "docshell is null");

  nsCOMPtr<nsIBaseWindow> basewin = do_QueryInterface(aShell);
  if (!basewin)
    return true;

  bool isVisible = true;
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
    
    
    aEvent->mIsEnabled = false;
  } else {
    bool canDoIt;
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
  aEvent->mSucceeded = true;
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

  aEvent->mSucceeded = true;

  nsIScrollableFrame* sf =
    ps->GetFrameToScrollAsScrollable(nsIPresShell::eEither);
  aEvent->mIsEnabled = sf ?
    (aEvent->mScroll.mIsHorizontal ?
      CanScrollOn(sf, aEvent->mScroll.mAmount, 0) :
      CanScrollOn(sf, 0, aEvent->mScroll.mAmount)) : false;

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
nsEventStateManager::DoQuerySelectedText(nsQueryContentEvent* aEvent)
{
  if (RemoteQueryContentEvent(aEvent)) {
    return;
  }
  nsContentEventHandler handler(mPresContext);
  handler.OnQuerySelectedText(aEvent);
}

void
nsEventStateManager::SetActiveManager(nsEventStateManager* aNewESM,
                                      nsIContent* aContent)
{
  if (sActiveESM && aNewESM != sActiveESM) {
    sActiveESM->SetContentState(nullptr, NS_EVENT_STATE_ACTIVE);
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
    aClearer->SetContentState(nullptr, NS_EVENT_STATE_ACTIVE);
    if (sDragOverContent) {
      aClearer->SetContentState(nullptr, NS_EVENT_STATE_DRAGOVER);
    }
  }
  if (sActiveESM && aClearer != sActiveESM) {
    sActiveESM->SetContentState(nullptr, NS_EVENT_STATE_ACTIVE);
  }
  sActiveESM = nullptr;
}





void
nsEventStateManager::DeltaAccumulator::InitLineOrPageDelta(
                                         nsIFrame* aTargetFrame,
                                         nsEventStateManager* aESM,
                                         widget::WheelEvent* aEvent)
{
  MOZ_ASSERT(aESM);
  MOZ_ASSERT(aEvent);

  
  if (!mLastTime.IsNull()) {
    TimeDuration duration = TimeStamp::Now() - mLastTime;
    if (duration.ToMilliseconds() > nsMouseWheelTransaction::GetTimeoutTime()) {
      Reset();
    }
  }
  
  if (IsInTransaction()) {
    
    if (mHandlingDeltaMode != aEvent->deltaMode ||
        mHandlingPixelOnlyDevice != aEvent->isPixelOnlyDevice) {
      Reset();
    } else {
      
      
      if (mX && aEvent->deltaX && ((aEvent->deltaX > 0.0) != (mX > 0.0))) {
        mX = 0.0;
      }
      if (mY && aEvent->deltaY && ((aEvent->deltaY > 0.0) != (mY > 0.0))) {
        mY = 0.0;
      }
    }
  }

  mHandlingDeltaMode = aEvent->deltaMode;
  mHandlingPixelOnlyDevice = aEvent->isPixelOnlyDevice;

  
  
  
  if (!(mHandlingDeltaMode == nsIDOMWheelEvent::DOM_DELTA_PIXEL &&
        mHandlingPixelOnlyDevice) &&
      !nsEventStateManager::WheelPrefs::GetInstance()->
        NeedToComputeLineOrPageDelta(aEvent)) {
    mLastTime = TimeStamp::Now();
    return;
  }

  mX += aEvent->deltaX;
  mY += aEvent->deltaY;

  if (mHandlingDeltaMode == nsIDOMWheelEvent::DOM_DELTA_PIXEL) {
    
    
    
    
    
    
    
    nsIScrollableFrame* scrollTarget =
      aESM->ComputeScrollTarget(aTargetFrame, aEvent, false);
    nsIFrame* frame = do_QueryFrame(scrollTarget);
    nsPresContext* pc =
      frame ? frame->PresContext() : aTargetFrame->PresContext();
    nsSize scrollAmount = aESM->GetScrollAmount(pc, aEvent, scrollTarget);
    nsIntSize scrollAmountInCSSPixels(
      nsPresContext::AppUnitsToIntCSSPixels(scrollAmount.width),
      nsPresContext::AppUnitsToIntCSSPixels(scrollAmount.height));

    aEvent->lineOrPageDeltaX = RoundDown(mX) / scrollAmountInCSSPixels.width;
    aEvent->lineOrPageDeltaY = RoundDown(mY) / scrollAmountInCSSPixels.height;

    mX -= aEvent->lineOrPageDeltaX * scrollAmountInCSSPixels.width;
    mY -= aEvent->lineOrPageDeltaY * scrollAmountInCSSPixels.height;
  } else {
    aEvent->lineOrPageDeltaX = RoundDown(mX);
    aEvent->lineOrPageDeltaY = RoundDown(mY);
    mX -= aEvent->lineOrPageDeltaX;
    mY -= aEvent->lineOrPageDeltaY;
  }

  mLastTime = TimeStamp::Now();
}

void
nsEventStateManager::DeltaAccumulator::Reset()
{
  mX = mY = 0.0;
  mPendingScrollAmountX = mPendingScrollAmountY = 0.0;
  mHandlingDeltaMode = PR_UINT32_MAX;
  mHandlingPixelOnlyDevice = false;
}

nsIntPoint
nsEventStateManager::DeltaAccumulator::ComputeScrollAmountForDefaultAction(
                       widget::WheelEvent* aEvent,
                       const nsIntSize& aScrollAmountInDevPixels)
{
  MOZ_ASSERT(aEvent);

  
  
  bool allowScrollSpeedOverride =
    (!aEvent->customizedByUserPrefs &&
     aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_LINE);
  DeltaValues acceleratedDelta =
    nsMouseWheelTransaction::AccelerateWheelDelta(aEvent,
                                                  allowScrollSpeedOverride);

  nsIntPoint result(0, 0);
  if (aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_PIXEL) {
    mPendingScrollAmountX += acceleratedDelta.deltaX;
    mPendingScrollAmountY += acceleratedDelta.deltaY;
  } else {
    mPendingScrollAmountX +=
      aScrollAmountInDevPixels.width * acceleratedDelta.deltaX;
    mPendingScrollAmountY +=
      aScrollAmountInDevPixels.height * acceleratedDelta.deltaY;
  }
  result.x = RoundDown(mPendingScrollAmountX);
  result.y = RoundDown(mPendingScrollAmountY);
  mPendingScrollAmountX -= result.x;
  mPendingScrollAmountY -= result.y;

  return result;
}






nsEventStateManager::WheelPrefs*
nsEventStateManager::WheelPrefs::GetInstance()
{
  if (!sInstance) {
    sInstance = new WheelPrefs();
  }
  return sInstance;
}


void
nsEventStateManager::WheelPrefs::Shutdown()
{
  delete sInstance;
  sInstance = nullptr;
}


int
nsEventStateManager::WheelPrefs::OnPrefChanged(const char* aPrefName,
                                               void* aClosure)
{
  
  sInstance->Reset();
  DeltaAccumulator::GetInstance()->Reset();
  return 0;
}

nsEventStateManager::WheelPrefs::WheelPrefs()
{
  Reset();
  Preferences::RegisterCallback(OnPrefChanged, "mousewheel.", nullptr);
}

nsEventStateManager::WheelPrefs::~WheelPrefs()
{
  Preferences::UnregisterCallback(OnPrefChanged, "mousewheel.", nullptr);
}

void
nsEventStateManager::WheelPrefs::Reset()
{
  memset(mInit, 0, sizeof(mInit));

}

nsEventStateManager::WheelPrefs::Index
nsEventStateManager::WheelPrefs::GetIndexFor(widget::WheelEvent* aEvent)
{
  if (!aEvent) {
    return INDEX_DEFAULT;
  }

  widget::Modifiers modifiers =
    (aEvent->modifiers & (widget::MODIFIER_ALT |
                          widget::MODIFIER_CONTROL |
                          widget::MODIFIER_META |
                          widget::MODIFIER_SHIFT |
                          widget::MODIFIER_OS));

  switch (modifiers) {
    case widget::MODIFIER_ALT:
      return INDEX_ALT;
    case widget::MODIFIER_CONTROL:
      return INDEX_CONTROL;
    case widget::MODIFIER_META:
      return INDEX_META;
    case widget::MODIFIER_SHIFT:
      return INDEX_SHIFT;
    case widget::MODIFIER_OS:
      return INDEX_OS;
    default:
      
      
      return INDEX_DEFAULT;
  }
}

void
nsEventStateManager::WheelPrefs::GetBasePrefName(
                       nsEventStateManager::WheelPrefs::Index aIndex,
                       nsACString& aBasePrefName)
{
  aBasePrefName.AssignLiteral("mousewheel.");
  switch (aIndex) {
    case INDEX_ALT:
      aBasePrefName.AppendLiteral("with_alt.");
      break;
    case INDEX_CONTROL:
      aBasePrefName.AppendLiteral("with_control.");
      break;
    case INDEX_META:
      aBasePrefName.AppendLiteral("with_meta.");
      break;
    case INDEX_SHIFT:
      aBasePrefName.AppendLiteral("with_shift.");
      break;
    case INDEX_OS:
      aBasePrefName.AppendLiteral("with_win.");
      break;
    case INDEX_DEFAULT:
    default:
      aBasePrefName.AppendLiteral("default.");
      break;
  }
}

void
nsEventStateManager::WheelPrefs::Init(
                       nsEventStateManager::WheelPrefs::Index aIndex)
{
  if (mInit[aIndex]) {
    return;
  }
  mInit[aIndex] = true;

  nsCAutoString basePrefName;
  GetBasePrefName(aIndex, basePrefName);

  nsCAutoString prefNameX(basePrefName);
  prefNameX.AppendLiteral("delta_multiplier_x");
  mMultiplierX[aIndex] =
    static_cast<double>(Preferences::GetInt(prefNameX.get(), 100)) / 100;
  if (mMultiplierX[aIndex] < 1.0 && mMultiplierX[aIndex] > -1.0) {
    mMultiplierX[aIndex] = mMultiplierX[aIndex] < 0.0 ? -1.0 : 1.0;
  }

  nsCAutoString prefNameY(basePrefName);
  prefNameY.AppendLiteral("delta_multiplier_y");
  mMultiplierY[aIndex] =
    static_cast<double>(Preferences::GetInt(prefNameY.get(), 100)) / 100;
  if (mMultiplierY[aIndex] < 1.0 && mMultiplierY[aIndex] > -1.0) {
    mMultiplierY[aIndex] = mMultiplierY[aIndex] < 0.0 ? -1.0 : 1.0;
  }

  nsCAutoString prefNameZ(basePrefName);
  prefNameZ.AppendLiteral("delta_multiplier_z");
  mMultiplierZ[aIndex] =
    static_cast<double>(Preferences::GetInt(prefNameZ.get(), 100)) / 100;
  if (mMultiplierZ[aIndex] < 1.0 && mMultiplierZ[aIndex] > -1.0) {
    mMultiplierZ[aIndex] = mMultiplierZ[aIndex] < 0.0 ? -1.0 : 1.0;
  }

  nsCAutoString prefNameAction(basePrefName);
  prefNameAction.AppendLiteral("action");
  mActions[aIndex] =
    static_cast<Action>(Preferences::GetInt(prefNameAction.get(),
                                            ACTION_SCROLL));
  if (mActions[aIndex] < ACTION_NONE || mActions[aIndex] > ACTION_LAST) {
    NS_WARNING("Unsupported action pref value, replaced with 'Scroll'.");
    mActions[aIndex] = ACTION_SCROLL;
  }
}

void
nsEventStateManager::WheelPrefs::ApplyUserPrefsToDelta(
                                   widget::WheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  aEvent->deltaX *= mMultiplierX[index];
  aEvent->deltaY *= mMultiplierY[index];
  aEvent->deltaZ *= mMultiplierZ[index];

  
  
  
  if (!NeedToComputeLineOrPageDelta(aEvent)) {
    aEvent->lineOrPageDeltaX *= static_cast<PRInt32>(mMultiplierX[index]);
    aEvent->lineOrPageDeltaY *= static_cast<PRInt32>(mMultiplierY[index]);
  } else {
    aEvent->lineOrPageDeltaX = 0;
    aEvent->lineOrPageDeltaY = 0;
  }

  aEvent->customizedByUserPrefs =
    ((mMultiplierX[index] != 1.0) || (mMultiplierY[index] != 1.0) ||
     (mMultiplierZ[index] != 1.0));
}

void
nsEventStateManager::WheelPrefs::CancelApplyingUserPrefsFromOverflowDelta(
                                                   widget::WheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  NS_ASSERTION(mMultiplierX[index] && mMultiplierY[index],
               "The absolute values of both multipliers must be 1 or larger");
  aEvent->overflowDeltaX /= mMultiplierX[index];
  aEvent->overflowDeltaY /= mMultiplierY[index];
}

nsEventStateManager::WheelPrefs::Action
nsEventStateManager::WheelPrefs::ComputeActionFor(widget::WheelEvent* aEvent)
{
  if (!aEvent->deltaX && !aEvent->deltaY) {
    return ACTION_NONE;
  }

  Index index = GetIndexFor(aEvent);
  Init(index);

  if (mActions[index] == ACTION_NONE || mActions[index] == ACTION_SCROLL) {
    return mActions[index];
  }

  
  if (aEvent->isMomentum) {
    
    Init(INDEX_DEFAULT);
    return (mActions[INDEX_DEFAULT] == ACTION_SCROLL) ? ACTION_SCROLL :
                                                        ACTION_NONE;
  }

  
  
  return !aEvent->GetPreferredIntDelta() ? ACTION_NONE : mActions[index];
}

bool
nsEventStateManager::WheelPrefs::NeedToComputeLineOrPageDelta(
                                   widget::WheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  return (mMultiplierX[index] != 1.0 && mMultiplierX[index] != -1.0) ||
         (mMultiplierY[index] != 1.0 && mMultiplierY[index] != -1.0);
}
