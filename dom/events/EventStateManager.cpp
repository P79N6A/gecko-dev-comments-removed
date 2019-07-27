





#include "mozilla/Attributes.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/IMEStateManager.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/dom/UIEvent.h"

#include "ContentEventHandler.h"
#include "IMEContentObserver.h"
#include "WheelHandlingHelper.h"

#include "nsCOMPtr.h"
#include "nsFocusManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIWidget.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsGkAtoms.h"
#include "nsIFormControl.h"
#include "nsIComboboxControlFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMXULControlElement.h"
#include "nsNameSpaceManager.h"
#include "nsIBaseWindow.h"
#include "nsISelection.h"
#include "nsITextControlElement.h"
#include "nsFrameSelection.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
#include "nsIWebNavigation.h"
#include "nsIContentViewer.h"
#include "nsFrameManager.h"

#include "nsIDOMXULElement.h"
#include "nsIDOMKeyEvent.h"
#include "nsIObserverService.h"
#include "nsIDocShell.h"
#include "nsIDOMWheelEvent.h"
#include "nsIDOMDragEvent.h"
#include "nsIDOMUIEvent.h"
#include "nsIMozBrowserFrame.h"

#include "nsSubDocumentFrame.h"
#include "nsLayoutUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"

#include "imgIContainer.h"
#include "nsIProperties.h"
#include "nsISupportsPrimitives.h"

#include "nsServiceManagerUtils.h"
#include "nsITimer.h"
#include "nsFontMetrics.h"
#include "nsIDOMXULDocument.h"
#include "nsIDragService.h"
#include "nsIDragSession.h"
#include "mozilla/dom/DataTransfer.h"
#include "nsContentAreaDragDrop.h"
#ifdef MOZ_XUL
#include "nsTreeBodyFrame.h"
#endif
#include "nsIController.h"
#include "nsICommandParams.h"
#include "mozilla/Services.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/HTMLLabelElement.h"

#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "GeckoProfiler.h"
#include "Units.h"
#include "mozilla/layers/APZCTreeManager.h"

#ifdef XP_MACOSX
#import <ApplicationServices/ApplicationServices.h>
#endif

namespace mozilla {

using namespace dom;



#define NS_USER_INTERACTION_INTERVAL 5000 // ms

static const LayoutDeviceIntPoint kInvalidRefPoint = LayoutDeviceIntPoint(-1,-1);

static uint32_t gMouseOrKeyboardEventCounter = 0;
static nsITimer* gUserInteractionTimer = nullptr;
static nsITimerCallback* gUserInteractionTimerCallback = nullptr;

static inline int32_t
RoundDown(double aDouble)
{
  return (aDouble > 0) ? static_cast<int32_t>(floor(aDouble)) :
                         static_cast<int32_t>(ceil(aDouble));
}

#ifdef DEBUG_DOCSHELL_FOCUS
static void
PrintDocTree(nsIDocShellTreeItem* aParentItem, int aLevel)
{
  for (int32_t i=0;i<aLevel;i++) printf("  ");

  int32_t childWebshellCount;
  aParentItem->GetChildCount(&childWebshellCount);
  nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(aParentItem));
  int32_t type = aParentItem->ItemType();
  nsCOMPtr<nsIPresShell> presShell = parentAsDocShell->GetPresShell();
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
    nsAutoCString spec;
    uri->GetSpec(spec);
    printf("\"%s\"\n", spec.get());
  }

  if (childWebshellCount > 0) {
    for (int32_t i = 0; i < childWebshellCount; i++) {
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


#define NS_MODIFIER_SHIFT    1
#define NS_MODIFIER_CONTROL  2
#define NS_MODIFIER_ALT      4
#define NS_MODIFIER_META     8
#define NS_MODIFIER_OS       16

static nsIDocument *
GetDocumentFromWindow(nsIDOMWindow *aWindow)
{
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aWindow);
  return win ? win->GetExtantDoc() : nullptr;
}





class UITimerCallback final : public nsITimerCallback
{
public:
  UITimerCallback() : mPreviousCount(0) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
private:
  ~UITimerCallback() {}
  uint32_t mPreviousCount;
};

NS_IMPL_ISUPPORTS(UITimerCallback, nsITimerCallback)



NS_IMETHODIMP
UITimerCallback::Notify(nsITimer* aTimer)
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
    EventStateManager::UpdateUserActivityTimer();
  }
  mPreviousCount = gMouseOrKeyboardEventCounter;
  return NS_OK;
}





OverOutElementsWrapper::OverOutElementsWrapper()
  : mLastOverFrame(nullptr)
{
}

OverOutElementsWrapper::~OverOutElementsWrapper()
{
}

NS_IMPL_CYCLE_COLLECTION(OverOutElementsWrapper,
                         mLastOverElement,
                         mFirstOverEventElement,
                         mFirstOutEventElement)
NS_IMPL_CYCLE_COLLECTING_ADDREF(OverOutElementsWrapper)
NS_IMPL_CYCLE_COLLECTING_RELEASE(OverOutElementsWrapper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(OverOutElementsWrapper)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END





static uint32_t sESMInstanceCount = 0;
static bool sPointerEventEnabled = false;

int32_t EventStateManager::sUserInputEventDepth = 0;
bool EventStateManager::sNormalLMouseEventInProcess = false;
EventStateManager* EventStateManager::sActiveESM = nullptr;
nsIDocument* EventStateManager::sMouseOverDocument = nullptr;
nsWeakFrame EventStateManager::sLastDragOverFrame = nullptr;
LayoutDeviceIntPoint EventStateManager::sLastRefPoint = kInvalidRefPoint;
LayoutDeviceIntPoint EventStateManager::sLastScreenPoint = LayoutDeviceIntPoint(0, 0);
LayoutDeviceIntPoint EventStateManager::sSynthCenteringPoint = kInvalidRefPoint;
CSSIntPoint EventStateManager::sLastClientPoint = CSSIntPoint(0, 0);
bool EventStateManager::sIsPointerLocked = false;

nsWeakPtr EventStateManager::sPointerLockedElement;

nsWeakPtr EventStateManager::sPointerLockedDoc;
nsCOMPtr<nsIContent> EventStateManager::sDragOverContent = nullptr;
TimeStamp EventStateManager::sHandlingInputStart;

EventStateManager::WheelPrefs*
  EventStateManager::WheelPrefs::sInstance = nullptr;
EventStateManager::DeltaAccumulator*
  EventStateManager::DeltaAccumulator::sInstance = nullptr;

EventStateManager::EventStateManager()
  : mLockCursor(0)
  , mPreLockPoint(0,0)
  , mCurrentTarget(nullptr)
    
  , mGestureDownPoint(0,0)
  , mPresContext(nullptr)
  , mLClickCount(0)
  , mMClickCount(0)
  , mRClickCount(0)
  , m_haveShutdown(false)
{
  if (sESMInstanceCount == 0) {
    gUserInteractionTimerCallback = new UITimerCallback();
    if (gUserInteractionTimerCallback)
      NS_ADDREF(gUserInteractionTimerCallback);
    UpdateUserActivityTimer();
  }
  ++sESMInstanceCount;

  static bool sAddedPointerEventEnabled = false;
  if (!sAddedPointerEventEnabled) {
    Preferences::AddBoolVarCache(&sPointerEventEnabled,
                                 "dom.w3c_pointer_events.enabled", false);
    sAddedPointerEventEnabled = true;
  }
}

nsresult
EventStateManager::UpdateUserActivityTimer()
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

nsresult
EventStateManager::Init()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, true);

  if (sESMInstanceCount == 1) {
    Prefs::Init();
  }

  return NS_OK;
}

EventStateManager::~EventStateManager()
{
  ReleaseCurrentIMEContentObserver();

  if (sActiveESM == this) {
    sActiveESM = nullptr;
  }
  if (Prefs::ClickHoldContextMenu())
    KillClickHoldTimer();

  if (mDocument == sMouseOverDocument)
    sMouseOverDocument = nullptr;

  --sESMInstanceCount;
  if(sESMInstanceCount == 0) {
    WheelTransaction::Shutdown();
    if (gUserInteractionTimerCallback) {
      gUserInteractionTimerCallback->Notify(nullptr);
      NS_RELEASE(gUserInteractionTimerCallback);
    }
    if (gUserInteractionTimer) {
      gUserInteractionTimer->Cancel();
      NS_RELEASE(gUserInteractionTimer);
    }
    Prefs::Shutdown();
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
EventStateManager::Shutdown()
{
  m_haveShutdown = true;
  return NS_OK;
}

NS_IMETHODIMP
EventStateManager::Observe(nsISupports* aSubject,
                           const char* aTopic,
                           const char16_t *someData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    Shutdown();
  }

  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(EventStateManager)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(EventStateManager)
NS_IMPL_CYCLE_COLLECTING_RELEASE(EventStateManager)

NS_IMPL_CYCLE_COLLECTION(EventStateManager,
                         mCurrentTargetContent,
                         mGestureDownContent,
                         mGestureDownFrameOwner,
                         mLastLeftMouseDownContent,
                         mLastLeftMouseDownContentParent,
                         mLastMiddleMouseDownContent,
                         mLastMiddleMouseDownContentParent,
                         mLastRightMouseDownContent,
                         mLastRightMouseDownContentParent,
                         mActiveContent,
                         mHoverContent,
                         mURLTargetContent,
                         mMouseEnterLeaveHelper,
                         mPointersEnterLeaveHelper,
                         mDocument,
                         mIMEContentObserver,
                         mAccessKeys)

void
EventStateManager::ReleaseCurrentIMEContentObserver()
{
  if (mIMEContentObserver) {
    mIMEContentObserver->DisconnectFromEventStateManager();
  }
  mIMEContentObserver = nullptr;
}

void
EventStateManager::OnStartToObserveContent(
                     IMEContentObserver* aIMEContentObserver)
{
  ReleaseCurrentIMEContentObserver();
  mIMEContentObserver = aIMEContentObserver;
}

void
EventStateManager::OnStopObservingContent(
                     IMEContentObserver* aIMEContentObserver)
{
  aIMEContentObserver->DisconnectFromEventStateManager();
  NS_ENSURE_TRUE_VOID(mIMEContentObserver == aIMEContentObserver);
  mIMEContentObserver = nullptr;
}

nsresult
EventStateManager::PreHandleEvent(nsPresContext* aPresContext,
                                  WidgetEvent* aEvent,
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

  
  
  WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
  if (aEvent->mFlags.mIsTrusted &&
      ((mouseEvent && mouseEvent->IsReal() &&
        mouseEvent->message != NS_MOUSE_ENTER &&
        mouseEvent->message != NS_MOUSE_EXIT) ||
       aEvent->mClass == eWheelEventClass ||
       aEvent->mClass == eKeyboardEventClass)) {
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

  WheelTransaction::OnEvent(aEvent);

  
  if (NS_EVENT_NEEDS_FRAME(aEvent)) {
    if (!mCurrentTarget) {
      return NS_ERROR_NULL_POINTER;
    }
  }
#ifdef DEBUG
  if (aEvent->HasDragEventMessage() && sIsPointerLocked) {
    NS_ASSERTION(sIsPointerLocked,
      "sIsPointerLocked is true. Drag events should be suppressed when "
      "the pointer is locked.");
  }
#endif
  
  
  if (aEvent->mFlags.mIsTrusted &&
      ((mouseEvent && mouseEvent->IsReal()) ||
       aEvent->mClass == eWheelEventClass) &&
      !sIsPointerLocked) {
    sLastScreenPoint =
      UIEvent::CalculateScreenPoint(aPresContext, aEvent);
    sLastClientPoint =
      UIEvent::CalculateClientPoint(aPresContext, aEvent, nullptr);
  }

  *aStatus = nsEventStatus_eIgnore;

  switch (aEvent->message) {
  case NS_CONTEXTMENU:
    if (sIsPointerLocked) {
      return NS_ERROR_DOM_INVALID_STATE_ERR;
    }
    break;
  case NS_MOUSE_BUTTON_DOWN: {
    switch (mouseEvent->button) {
    case WidgetMouseEvent::eLeftButton:
      BeginTrackingDragGesture(aPresContext, mouseEvent, aTargetFrame);
      mLClickCount = mouseEvent->clickCount;
      SetClickCount(aPresContext, mouseEvent, aStatus);
      sNormalLMouseEventInProcess = true;
      break;
    case WidgetMouseEvent::eMiddleButton:
      mMClickCount = mouseEvent->clickCount;
      SetClickCount(aPresContext, mouseEvent, aStatus);
      break;
    case WidgetMouseEvent::eRightButton:
      mRClickCount = mouseEvent->clickCount;
      SetClickCount(aPresContext, mouseEvent, aStatus);
      break;
    }
    break;
  }
  case NS_MOUSE_BUTTON_UP: {
    switch (mouseEvent->button) {
      case WidgetMouseEvent::eLeftButton:
        if (Prefs::ClickHoldContextMenu()) {
          KillClickHoldTimer();
        }
        StopTrackingDragGesture();
        sNormalLMouseEventInProcess = false;
        
      case WidgetMouseEvent::eRightButton:
      case WidgetMouseEvent::eMiddleButton:
        SetClickCount(aPresContext, mouseEvent, aStatus);
        break;
    }
    break;
  }
  case NS_MOUSE_EXIT:
    
    
    
    
    
    
    
    if (XRE_GetProcessType() == GeckoProcessType_Content) {
      ClearCachedWidgetCursor(mCurrentTarget);
    }

    
    
    
    if (mouseEvent->exit != WidgetMouseEvent::eTopLevel) {
      
      
      
      mouseEvent->message = NS_MOUSE_MOVE;
      mouseEvent->reason = WidgetMouseEvent::eSynthesized;
      
    } else {
      if (sPointerEventEnabled) {
        
        GeneratePointerEnterExit(NS_POINTER_LEAVE, mouseEvent);
      }
      GenerateMouseEnterExit(mouseEvent);
      
      aEvent->message = 0;
      break;
    }
  case NS_MOUSE_MOVE:
  case NS_POINTER_DOWN:
  case NS_POINTER_MOVE: {
    
    
    
    
    
    
    GenerateDragGesture(aPresContext, mouseEvent);
    UpdateCursor(aPresContext, aEvent, mCurrentTarget, aStatus);
    GenerateMouseEnterExit(mouseEvent);
    
    
    FlushPendingEvents(aPresContext);
    break;
  }
  case NS_DRAGDROP_GESTURE:
    if (Prefs::ClickHoldContextMenu()) {
      
      
      KillClickHoldTimer();
    }
    break;
  case NS_DRAGDROP_OVER:
    
    
    GenerateDragDropEnterExit(aPresContext, aEvent->AsDragEvent());
    break;

  case NS_KEY_PRESS:
    {
      WidgetKeyboardEvent* keyEvent = aEvent->AsKeyboardEvent();

      int32_t modifierMask = 0;
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

      
      if (modifierMask &&
          (modifierMask == Prefs::ChromeAccessModifierMask() ||
           modifierMask == Prefs::ContentAccessModifierMask())) {
        HandleAccessKey(aPresContext, keyEvent, aStatus, nullptr,
                        eAccessKeyProcessingNormal, modifierMask);
      }
    }
    
  case NS_KEY_BEFORE_DOWN:
  case NS_KEY_DOWN:
  case NS_KEY_AFTER_DOWN:
  case NS_KEY_BEFORE_UP:
  case NS_KEY_UP:
  case NS_KEY_AFTER_UP:
    {
      nsIContent* content = GetFocusedContent();
      if (content)
        mCurrentTargetContent = content;

      
      
      
      
      
      WidgetKeyboardEvent* keyEvent = aEvent->AsKeyboardEvent();
      nsRefPtr<TextComposition> composition =
        IMEStateManager::GetTextCompositionFor(keyEvent);
      keyEvent->mIsComposing = !!composition;
    }
    break;
  case NS_WHEEL_WHEEL:
  case NS_WHEEL_START:
  case NS_WHEEL_STOP:
    {
      NS_ASSERTION(aEvent->mFlags.mIsTrusted,
                   "Untrusted wheel event shouldn't be here");

      nsIContent* content = GetFocusedContent();
      if (content) {
        mCurrentTargetContent = content;
      }

      if (aEvent->message != NS_WHEEL_WHEEL) {
        break;
      }

      WidgetWheelEvent* wheelEvent = aEvent->AsWheelEvent();
      WheelPrefs::GetInstance()->ApplyUserPrefsToDelta(wheelEvent);

      
      if (!wheelEvent->IsAllowedToDispatchDOMEvent()) {
        break;
      }

      
      
      
      
      DeltaAccumulator::GetInstance()->
        InitLineOrPageDelta(aTargetFrame, this, wheelEvent);
    }
    break;
  case NS_QUERY_SELECTED_TEXT:
    DoQuerySelectedText(aEvent->AsQueryContentEvent());
    break;
  case NS_QUERY_TEXT_CONTENT:
    {
      if (RemoteQueryContentEvent(aEvent)) {
        break;
      }
      ContentEventHandler handler(mPresContext);
      handler.OnQueryTextContent(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_QUERY_CARET_RECT:
    {
      if (RemoteQueryContentEvent(aEvent)) {
        break;
      }
      ContentEventHandler handler(mPresContext);
      handler.OnQueryCaretRect(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_QUERY_TEXT_RECT:
    {
      if (RemoteQueryContentEvent(aEvent)) {
        break;
      }
      ContentEventHandler handler(mPresContext);
      handler.OnQueryTextRect(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_QUERY_EDITOR_RECT:
    {
      if (RemoteQueryContentEvent(aEvent)) {
        break;
      }
      ContentEventHandler handler(mPresContext);
      handler.OnQueryEditorRect(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_QUERY_CONTENT_STATE:
    {
      
      ContentEventHandler handler(mPresContext);
      handler.OnQueryContentState(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_QUERY_SELECTION_AS_TRANSFERABLE:
    {
      
      ContentEventHandler handler(mPresContext);
      handler.OnQuerySelectionAsTransferable(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_QUERY_CHARACTER_AT_POINT:
    {
      
      ContentEventHandler handler(mPresContext);
      handler.OnQueryCharacterAtPoint(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_QUERY_DOM_WIDGET_HITTEST:
    {
      
      ContentEventHandler handler(mPresContext);
      handler.OnQueryDOMWidgetHittest(aEvent->AsQueryContentEvent());
    }
    break;
  case NS_SELECTION_SET:
    {
      WidgetSelectionEvent* selectionEvent = aEvent->AsSelectionEvent();
      if (IsTargetCrossProcess(selectionEvent)) {
        
        if (GetCrossProcessTarget()->SendSelectionEvent(*selectionEvent)) {
          selectionEvent->mSucceeded = true;
        }
        break;
      }
      ContentEventHandler handler(mPresContext);
      handler.OnSelectionEvent(selectionEvent);
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
      DoContentCommandEvent(aEvent->AsContentCommandEvent());
    }
    break;
  case NS_CONTENT_COMMAND_SCROLL:
    {
      DoContentCommandScrollEvent(aEvent->AsContentCommandEvent());
    }
    break;
  case NS_COMPOSITION_START:
    if (aEvent->mFlags.mIsTrusted) {
      
      
      WidgetCompositionEvent* compositionEvent = aEvent->AsCompositionEvent();
      WidgetQueryContentEvent selectedText(true, NS_QUERY_SELECTED_TEXT,
                                           compositionEvent->widget);
      DoQuerySelectedText(&selectedText);
      NS_ASSERTION(selectedText.mSucceeded, "Failed to get selected text");
      compositionEvent->mData = selectedText.mReply.mString;
    }
    
  case NS_COMPOSITION_END:
  case NS_COMPOSITION_CHANGE:
  case NS_COMPOSITION_COMMIT_AS_IS:
  case NS_COMPOSITION_COMMIT:
    {
      WidgetCompositionEvent* compositionEvent = aEvent->AsCompositionEvent();
      if (IsTargetCrossProcess(compositionEvent)) {
        
        if (GetCrossProcessTarget()->SendCompositionEvent(*compositionEvent)) {
          
          aEvent->mFlags.mPropagationStopped = true;
        }
      }
    }
    break;
  }
  return NS_OK;
}


int32_t
EventStateManager::GetAccessModifierMaskFor(nsISupports* aDocShell)
{
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryInterface(aDocShell));
  if (!treeItem)
    return -1; 

  switch (treeItem->ItemType()) {
  case nsIDocShellTreeItem::typeChrome:
    return Prefs::ChromeAccessModifierMask();

  case nsIDocShellTreeItem::typeContent:
    return Prefs::ContentAccessModifierMask();

  default:
    return -1; 
  }
}

static bool
IsAccessKeyTarget(nsIContent* aContent, nsIFrame* aFrame, nsAString& aKey)
{
  
  
  nsString contentKey;
  if (!aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, contentKey) ||
      !contentKey.Equals(aKey, nsCaseInsensitiveStringComparator()))
    return false;

  nsCOMPtr<nsIDOMXULDocument> xulDoc =
    do_QueryInterface(aContent->OwnerDoc());
  if (!xulDoc && !aContent->IsXULElement())
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

  
  
  if (aContent->IsAnyOfHTMLElements(nsGkAtoms::area,
                                    nsGkAtoms::label,
                                    nsGkAtoms::legend)) {
    return true;
  }

  
  
  if (aContent->IsXULElement(nsGkAtoms::label)) {
    return true;
  }

  return false;
}

bool
EventStateManager::ExecuteAccessKey(nsTArray<uint32_t>& aAccessCharCodes,
                                    bool aIsTrustedEvent)
{
  int32_t count, start = -1;
  nsIContent* focusedContent = GetFocusedContent();
  if (focusedContent) {
    start = mAccessKeys.IndexOf(focusedContent);
    if (start == -1 && focusedContent->GetBindingParent())
      start = mAccessKeys.IndexOf(focusedContent->GetBindingParent());
  }
  nsIContent *content;
  nsIFrame *frame;
  int32_t length = mAccessKeys.Count();
  for (uint32_t i = 0; i < aAccessCharCodes.Length(); ++i) {
    uint32_t ch = aAccessCharCodes[i];
    nsAutoString accessKey;
    AppendUCS4ToUTF16(ch, accessKey);
    for (count = 1; count <= length; ++count) {
      content = mAccessKeys[(start + count) % length];
      frame = content->GetPrimaryFrame();
      if (IsAccessKeyTarget(content, frame, accessKey)) {
        bool shouldActivate = Prefs::KeyCausesActivation();
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


void
EventStateManager::GetAccessKeyLabelPrefix(Element* aElement, nsAString& aPrefix)
{
  aPrefix.Truncate();
  nsAutoString separator, modifierText;
  nsContentUtils::GetModifierSeparatorText(separator);

  nsCOMPtr<nsISupports> container = aElement->OwnerDoc()->GetDocShell();
  int32_t modifierMask = GetAccessModifierMaskFor(container);

  if (modifierMask == -1) {
    return;
  }

  if (modifierMask & NS_MODIFIER_CONTROL) {
    nsContentUtils::GetControlText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifierMask & NS_MODIFIER_META) {
    nsContentUtils::GetMetaText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifierMask & NS_MODIFIER_OS) {
    nsContentUtils::GetOSText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifierMask & NS_MODIFIER_ALT) {
    nsContentUtils::GetAltText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
  if (modifierMask & NS_MODIFIER_SHIFT) {
    nsContentUtils::GetShiftText(modifierText);
    aPrefix.Append(modifierText + separator);
  }
}

void
EventStateManager::HandleAccessKey(nsPresContext* aPresContext,
                                   WidgetKeyboardEvent* aEvent,
                                   nsEventStatus* aStatus,
                                   nsIDocShellTreeItem* aBubbledFrom,
                                   ProcessingAccessKeyState aAccessKeyState,
                                   int32_t aModifierMask)
{
  nsCOMPtr<nsIDocShell> docShell = aPresContext->GetDocShell();

  
  if (mAccessKeys.Count() > 0 &&
      aModifierMask == GetAccessModifierMaskFor(docShell)) {
    
    nsAutoTArray<uint32_t, 10> accessCharCodes;
    nsContentUtils::GetAccessKeyCandidates(aEvent, accessCharCodes);
    if (ExecuteAccessKey(accessCharCodes, aEvent->mFlags.mIsTrusted)) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  
  if (nsEventStatus_eConsumeNoDefault != *aStatus) {
    

    if (!docShell) {
      NS_WARNING("no docShellTreeNode for presContext");
      return;
    }

    int32_t childCount;
    docShell->GetChildCount(&childCount);
    for (int32_t counter = 0; counter < childCount; counter++) {
      
      nsCOMPtr<nsIDocShellTreeItem> subShellItem;
      docShell->GetChildAt(counter, getter_AddRefs(subShellItem));
      if (aAccessKeyState == eAccessKeyProcessingUp &&
          subShellItem == aBubbledFrom)
        continue;

      nsCOMPtr<nsIDocShell> subDS = do_QueryInterface(subShellItem);
      if (subDS && IsShellVisible(subDS)) {
        nsCOMPtr<nsIPresShell> subPS = subDS->GetPresShell();

        
        
        if (!subPS) {
          
          continue;
        }

        nsPresContext *subPC = subPS->GetPresContext();

        EventStateManager* esm =
          static_cast<EventStateManager*>(subPC->EventStateManager());

        if (esm)
          esm->HandleAccessKey(subPC, aEvent, aStatus, nullptr,
                               eAccessKeyProcessingDown, aModifierMask);

        if (nsEventStatus_eConsumeNoDefault == *aStatus)
          break;
      }
    }
  }

  
  if (eAccessKeyProcessingDown != aAccessKeyState && nsEventStatus_eConsumeNoDefault != *aStatus) {
    if (!docShell) {
      NS_WARNING("no docShellTreeItem for presContext");
      return;
    }

    nsCOMPtr<nsIDocShellTreeItem> parentShellItem;
    docShell->GetParent(getter_AddRefs(parentShellItem));
    nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(parentShellItem);
    if (parentDS) {
      nsCOMPtr<nsIPresShell> parentPS = parentDS->GetPresShell();
      NS_ASSERTION(parentPS, "Our PresShell exists but the parent's does not?");

      nsPresContext *parentPC = parentPS->GetPresContext();
      NS_ASSERTION(parentPC, "PresShell without PresContext");

      EventStateManager* esm =
        static_cast<EventStateManager*>(parentPC->EventStateManager());

      if (esm)
        esm->HandleAccessKey(parentPC, aEvent, aStatus, docShell,
                             eAccessKeyProcessingUp, aModifierMask);
    }
  }
}

bool
EventStateManager::DispatchCrossProcessEvent(WidgetEvent* aEvent,
                                             nsFrameLoader* aFrameLoader,
                                             nsEventStatus *aStatus) {
  TabParent* remote = TabParent::GetFrom(aFrameLoader);
  if (!remote) {
    return false;
  }

  switch (aEvent->mClass) {
  case eMouseEventClass: {
    return remote->SendRealMouseEvent(*aEvent->AsMouseEvent());
  }
  case eKeyboardEventClass: {
    return remote->SendRealKeyEvent(*aEvent->AsKeyboardEvent());
  }
  case eWheelEventClass: {
    return remote->SendMouseWheelEvent(*aEvent->AsWheelEvent());
  }
  case eTouchEventClass: {
    
    
    *aStatus = nsEventStatus_eConsumeNoDefault;
    return remote->SendRealTouchEvent(*aEvent->AsTouchEvent());
  }
  case eDragEventClass: {
    if (remote->Manager()->IsContentParent()) {
      remote->Manager()->AsContentParent()->MaybeInvokeDragSession(remote);
    }

    nsCOMPtr<nsIDragSession> dragSession = nsContentUtils::GetDragSession();
    uint32_t dropEffect = nsIDragService::DRAGDROP_ACTION_NONE;
    uint32_t action = nsIDragService::DRAGDROP_ACTION_NONE;
    if (dragSession) {
      dragSession->DragEventDispatchedToChildProcess();
      dragSession->GetDragAction(&action);
      nsCOMPtr<nsIDOMDataTransfer> initialDataTransfer;
      dragSession->GetDataTransfer(getter_AddRefs(initialDataTransfer));
      if (initialDataTransfer) {
        initialDataTransfer->GetDropEffectInt(&dropEffect);
      }
    }

    bool retval = remote->SendRealDragEvent(*aEvent->AsDragEvent(),
                                            action, dropEffect);

    return retval;
  }
  default: {
    MOZ_CRASH("Attempt to send non-whitelisted event?");
  }
  }
}

bool
EventStateManager::IsRemoteTarget(nsIContent* target) {
  if (!target) {
    return false;
  }

  
  if (target->IsAnyOfXULElements(nsGkAtoms::browser, nsGkAtoms::iframe) &&
      target->AttrValueIs(kNameSpaceID_None, nsGkAtoms::Remote,
                          nsGkAtoms::_true, eIgnoreCase)) {
    return true;
  }

  
  nsCOMPtr<nsIMozBrowserFrame> browserFrame = do_QueryInterface(target);
  if (browserFrame && browserFrame->GetReallyIsBrowserOrApp()) {
    return !!TabParent::GetFrom(target);
  }

  return false;
}

bool
CrossProcessSafeEvent(const WidgetEvent& aEvent)
{
  switch (aEvent.mClass) {
  case eKeyboardEventClass:
  case eWheelEventClass:
    return true;
  case eMouseEventClass:
    switch (aEvent.message) {
    case NS_MOUSE_BUTTON_DOWN:
    case NS_MOUSE_BUTTON_UP:
    case NS_MOUSE_MOVE:
    case NS_CONTEXTMENU:
    case NS_MOUSE_EXIT:
      return true;
    default:
      return false;
    }
  case eTouchEventClass:
    switch (aEvent.message) {
    case NS_TOUCH_START:
    case NS_TOUCH_MOVE:
    case NS_TOUCH_END:
    case NS_TOUCH_CANCEL:
      return true;
    default:
      return false;
    }
  case eDragEventClass:
    switch (aEvent.message) {
    case NS_DRAGDROP_OVER:
    case NS_DRAGDROP_EXIT:
    case NS_DRAGDROP_DROP:
      return true;
    }
  default:
    return false;
  }
}

bool
EventStateManager::HandleCrossProcessEvent(WidgetEvent* aEvent,
                                           nsEventStatus *aStatus) {
  if (*aStatus == nsEventStatus_eConsumeNoDefault ||
      aEvent->mFlags.mNoCrossProcessBoundaryForwarding ||
      !CrossProcessSafeEvent(*aEvent)) {
    return false;
  }

  
  
  
  
  nsAutoTArray<nsCOMPtr<nsIContent>, 1> targets;
  if (aEvent->mClass != eTouchEventClass ||
      aEvent->message == NS_TOUCH_START) {
    
    
    nsIFrame* frame = GetEventTarget();
    nsIContent* target = frame ? frame->GetContent() : nullptr;
    if (IsRemoteTarget(target)) {
      targets.AppendElement(target);
    }
  } else {
    
    
    
    
    
    
    
    const WidgetTouchEvent::TouchArray& touches =
      aEvent->AsTouchEvent()->touches;
    for (uint32_t i = 0; i < touches.Length(); ++i) {
      Touch* touch = touches[i];
      
      
      
      
      if (!touch || !touch->mChanged) {
        continue;
      }
      nsCOMPtr<EventTarget> targetPtr = touch->mTarget;
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
  for (uint32_t i = 0; i < targets.Length(); ++i) {
    nsIContent* target = targets[i];
    nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(target);
    if (!loaderOwner) {
      continue;
    }

    nsRefPtr<nsFrameLoader> frameLoader = loaderOwner->GetFrameLoader();
    if (!frameLoader) {
      continue;
    }

    uint32_t eventMode;
    frameLoader->GetEventMode(&eventMode);
    if (eventMode == nsIFrameLoader::EVENT_MODE_DONT_FORWARD_TO_CHILD) {
      continue;
    }

    dispatched |= DispatchCrossProcessEvent(aEvent, frameLoader, aStatus);
  }
  return dispatched;
}








void
EventStateManager::CreateClickHoldTimer(nsPresContext* inPresContext,
                                        nsIFrame* inDownFrame,
                                        WidgetGUIEvent* inMouseDownEvent)
{
  if (!inMouseDownEvent->mFlags.mIsTrusted ||
      IsRemoteTarget(mGestureDownContent) ||
      sIsPointerLocked) {
    return;
  }

  
  if (mClickHoldTimer) {
    mClickHoldTimer->Cancel();
    mClickHoldTimer = nullptr;
  }

  
  
  if (mGestureDownContent) {
    
    if (nsContentUtils::HasNonEmptyAttr(mGestureDownContent, kNameSpaceID_None,
                                        nsGkAtoms::popup))
      return;
    
    
    if (mGestureDownContent->IsXULElement(nsGkAtoms::menubutton))
      return;
  }

  mClickHoldTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mClickHoldTimer) {
    int32_t clickHoldDelay =
      Preferences::GetInt("ui.click_hold_context_menus.delay", 500);
    mClickHoldTimer->InitWithFuncCallback(sClickHoldCallback, this,
                                          clickHoldDelay,
                                          nsITimer::TYPE_ONE_SHOT);
  }
} 







void
EventStateManager::KillClickHoldTimer()
{
  if (mClickHoldTimer) {
    mClickHoldTimer->Cancel();
    mClickHoldTimer = nullptr;
  }
}







void
EventStateManager::sClickHoldCallback(nsITimer* aTimer, void* aESM)
{
  nsRefPtr<EventStateManager> self = static_cast<EventStateManager*>(aESM);
  if (self) {
    self->FireContextClick();
  }

  

} 
















void
EventStateManager::FireContextClick()
{
  if (!mGestureDownContent || !mPresContext || sIsPointerLocked) {
    return;
  }

#ifdef XP_MACOSX
  
  
  
  if (!CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, kCGMouseButtonLeft))
    return;
#endif

  nsEventStatus status = nsEventStatus_eIgnore;

  
  
  
  
  
  
  mCurrentTarget = mPresContext->GetPrimaryFrameFor(mGestureDownContent);
  
  nsCOMPtr<nsIWidget> targetWidget;
  if (mCurrentTarget && (targetWidget = mCurrentTarget->GetNearestWidget())) {
    NS_ASSERTION(mPresContext == mCurrentTarget->PresContext(),
                 "a prescontext returned a primary frame that didn't belong to it?");

    
    
    bool allowedToDispatch = true;

    if (mGestureDownContent->IsAnyOfXULElements(nsGkAtoms::scrollbar,
                                                nsGkAtoms::scrollbarbutton,
                                                nsGkAtoms::button)) {
      allowedToDispatch = false;
    } else if (mGestureDownContent->IsXULElement(nsGkAtoms::toolbarbutton)) {
      
      
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
    else if (mGestureDownContent->IsHTMLElement()) {
      nsCOMPtr<nsIFormControl> formCtrl(do_QueryInterface(mGestureDownContent));

      if (formCtrl) {
        allowedToDispatch = formCtrl->IsTextControl(false) ||
                            formCtrl->GetType() == NS_FORM_INPUT_FILE;
      }
      else if (mGestureDownContent->IsAnyOfHTMLElements(nsGkAtoms::applet,
                                                        nsGkAtoms::embed,
                                                        nsGkAtoms::object)) {
        allowedToDispatch = false;
      }
    }

    if (allowedToDispatch) {
      
      WidgetMouseEvent event(true, NS_CONTEXTMENU, targetWidget,
                             WidgetMouseEvent::eReal);
      event.clickCount = 1;
      FillInEventFromGestureDown(&event);
        
      
      if (mCurrentTarget)
      {
        nsRefPtr<nsFrameSelection> frameSel =
          mCurrentTarget->GetFrameSelection();
        
        if (frameSel && frameSel->GetDragState()) {
          
          
          frameSel->SetDragState(false);
        }
      }

      nsIDocument* doc = mGestureDownContent->GetCrossShadowCurrentDoc();
      AutoHandlingUserInputStatePusher userInpStatePusher(true, &event, doc);

      
      EventDispatcher::Dispatch(mGestureDownContent, mPresContext, &event,
                                nullptr, &status);

      
      
      
      
    }
  }

  
  if (status == nsEventStatus_eConsumeNoDefault) {
    StopTrackingDragGesture();
  }

  KillClickHoldTimer();

} 














void
EventStateManager::BeginTrackingDragGesture(nsPresContext* aPresContext,
                                            WidgetMouseEvent* inDownEvent,
                                            nsIFrame* inDownFrame)
{
  if (!inDownEvent->widget)
    return;

  
  
  mGestureDownPoint = inDownEvent->refPoint + inDownEvent->widget->WidgetToScreenOffset();

  inDownFrame->GetContentForEvent(inDownEvent,
                                  getter_AddRefs(mGestureDownContent));

  mGestureDownFrameOwner = inDownFrame->GetContent();
  mGestureModifiers = inDownEvent->modifiers;
  mGestureDownButtons = inDownEvent->buttons;

  if (Prefs::ClickHoldContextMenu()) {
    
    CreateClickHoldTimer(aPresContext, inDownFrame, inDownEvent);
  }
}

void
EventStateManager::BeginTrackingRemoteDragGesture(nsIContent* aContent)
{
  mGestureDownContent = aContent;
  mGestureDownFrameOwner = aContent;
}







void
EventStateManager::StopTrackingDragGesture()
{
  mGestureDownContent = nullptr;
  mGestureDownFrameOwner = nullptr;
}

void
EventStateManager::FillInEventFromGestureDown(WidgetMouseEvent* aEvent)
{
  NS_ASSERTION(aEvent->widget == mCurrentTarget->GetNearestWidget(),
               "Incorrect widget in event");

  
  
  
  aEvent->refPoint = mGestureDownPoint - aEvent->widget->WidgetToScreenOffset();
  aEvent->modifiers = mGestureModifiers;
  aEvent->buttons = mGestureDownButtons;
}







void
EventStateManager::GenerateDragGesture(nsPresContext* aPresContext,
                                       WidgetMouseEvent* aEvent)
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
      if (frameSel && frameSel->GetDragState()) {
        StopTrackingDragGesture();
        return;
      }
    }

    
    if (nsIPresShell::IsMouseCapturePreventingDrag()) {
      StopTrackingDragGesture();
      return;
    }

    static int32_t pixelThresholdX = 0;
    static int32_t pixelThresholdY = 0;

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

    
    LayoutDeviceIntPoint pt = aEvent->refPoint + aEvent->widget->WidgetToScreenOffset();
    LayoutDeviceIntPoint distance = pt - mGestureDownPoint;
    if (Abs(distance.x) > AssertedCast<uint32_t>(pixelThresholdX) ||
        Abs(distance.y) > AssertedCast<uint32_t>(pixelThresholdY)) {
      if (Prefs::ClickHoldContextMenu()) {
        
        
        KillClickHoldTimer();
      }

      nsCOMPtr<nsISupports> container = aPresContext->GetContainerWeak();
      nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(container);
      if (!window)
        return;

      nsRefPtr<DataTransfer> dataTransfer =
        new DataTransfer(window, NS_DRAGDROP_START, false, -1);

      nsCOMPtr<nsISelection> selection;
      nsCOMPtr<nsIContent> eventContent, targetContent;
      mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(eventContent));
      if (eventContent)
        DetermineDragTargetAndDefaultData(window, eventContent, dataTransfer,
                                          getter_AddRefs(selection),
                                          getter_AddRefs(targetContent));

      
      
      StopTrackingDragGesture();

      if (!targetContent)
        return;

      
      
      dataTransfer->SetParentObject(targetContent);

      sLastDragOverFrame = nullptr;
      nsCOMPtr<nsIWidget> widget = mCurrentTarget->GetNearestWidget();

      
      WidgetDragEvent startEvent(aEvent->mFlags.mIsTrusted,
                                 NS_DRAGDROP_START, widget);
      FillInEventFromGestureDown(&startEvent);

      WidgetDragEvent gestureEvent(aEvent->mFlags.mIsTrusted,
                                   NS_DRAGDROP_GESTURE, widget);
      FillInEventFromGestureDown(&gestureEvent);

      startEvent.dataTransfer = gestureEvent.dataTransfer = dataTransfer;
      startEvent.inputSource = gestureEvent.inputSource = aEvent->inputSource;

      
      
      
      
      
      
      
      

      
      nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

      
      mCurrentTargetContent = targetContent;

      
      
      
      nsEventStatus status = nsEventStatus_eIgnore;
      EventDispatcher::Dispatch(targetContent, aPresContext, &startEvent,
                                nullptr, &status);

      WidgetDragEvent* event = &startEvent;
      if (status != nsEventStatus_eConsumeNoDefault) {
        status = nsEventStatus_eIgnore;
        EventDispatcher::Dispatch(targetContent, aPresContext, &gestureEvent,
                                  nullptr, &status);
        event = &gestureEvent;
      }

      nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
      
      if (observerService) {
        observerService->NotifyObservers(dataTransfer,
                                         "on-datatransfer-available",
                                         nullptr);
      }

      
      
      
      dataTransfer->SetReadOnly();

      if (status != nsEventStatus_eConsumeNoDefault) {
        bool dragStarted = DoDefaultDragStart(aPresContext, event, dataTransfer,
                                              targetContent, selection);
        if (dragStarted) {
          sActiveESM = nullptr;
          aEvent->mFlags.mPropagationStopped = true;
        }
      }

      
      
      

      
      mCurrentTargetContent = targetBeforeEvent;
    }

    
    
    FlushPendingEvents(aPresContext);
  }
} 

void
EventStateManager::DetermineDragTargetAndDefaultData(nsPIDOMWindow* aWindow,
                                                     nsIContent* aSelectionTarget,
                                                     DataTransfer* aDataTransfer,
                                                     nsISelection** aSelection,
                                                     nsIContent** aTargetNode)
{
  *aTargetNode = nullptr;

  
  
  
  
  
  bool canDrag;
  nsCOMPtr<nsIContent> dragDataNode;
  bool wasAlt = (mGestureModifiers & MODIFIER_ALT) != 0;
  nsresult rv = nsContentAreaDragDrop::GetDragData(aWindow, mGestureDownContent,
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
EventStateManager::DoDefaultDragStart(nsPresContext* aPresContext,
                                      WidgetDragEvent* aDragEvent,
                                      DataTransfer* aDataTransfer,
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

  
  
  uint32_t count = 0;
  if (aDataTransfer)
    aDataTransfer->GetMozItemCount(&count);
  if (!count)
    return false;

  
  
  
  
  nsCOMPtr<nsIContent> dragTarget = aDataTransfer->GetDragTarget();
  if (!dragTarget) {
    dragTarget = aDragTarget;
    if (!dragTarget)
      return false;
  }

  
  
  uint32_t action;
  aDataTransfer->GetEffectAllowedInt(&action);
  if (action == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED)
    action = nsIDragService::DRAGDROP_ACTION_COPY |
             nsIDragService::DRAGDROP_ACTION_MOVE |
             nsIDragService::DRAGDROP_ACTION_LINK;

  
  int32_t imageX, imageY;
  Element* dragImage = aDataTransfer->GetDragImage(&imageX, &imageY);

  nsCOMPtr<nsISupportsArray> transArray =
    aDataTransfer->GetTransferables(dragTarget->AsDOMNode());
  if (!transArray)
    return false;

  
  
  
  nsCOMPtr<nsIDOMEvent> domEvent;
  NS_NewDOMDragEvent(getter_AddRefs(domEvent), dragTarget,
                     aPresContext, aDragEvent);

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
      if (dragTarget->NodeInfo()->Equals(nsGkAtoms::treechildren,
                                         kNameSpaceID_XUL)) {
        nsTreeBodyFrame* treeBody =
          do_QueryFrame(dragTarget->GetPrimaryFrame());
        if (treeBody) {
          treeBody->GetSelectionRegion(getter_AddRefs(region));
        }
      }
    }
#endif

    dragService->InvokeDragSessionWithImage(dragTarget->AsDOMNode(), transArray,
                                            region, action,
                                            dragImage ? dragImage->AsDOMNode() :
                                                        nullptr,
                                            imageX,
                                            imageY, domDragEvent,
                                            aDataTransfer);
  }

  return true;
}

nsresult
EventStateManager::GetContentViewer(nsIContentViewer** aCv)
{
  *aCv = nullptr;

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

  nsCOMPtr<nsIDocShell> docshell(presContext->GetDocShell());
  if(!docshell) return NS_ERROR_FAILURE;

  docshell->GetContentViewer(aCv);
  if(!*aCv) return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult
EventStateManager::ChangeTextSize(int32_t change)
{
  nsCOMPtr<nsIContentViewer> cv;
  nsresult rv = GetContentViewer(getter_AddRefs(cv));
  NS_ENSURE_SUCCESS(rv, rv);

  float textzoom;
  float zoomMin = ((float)Preferences::GetInt("zoom.minPercent", 50)) / 100;
  float zoomMax = ((float)Preferences::GetInt("zoom.maxPercent", 300)) / 100;
  cv->GetTextZoom(&textzoom);
  textzoom += ((float)change) / 10;
  if (textzoom < zoomMin)
    textzoom = zoomMin;
  else if (textzoom > zoomMax)
    textzoom = zoomMax;
  cv->SetTextZoom(textzoom);

  return NS_OK;
}

nsresult
EventStateManager::ChangeFullZoom(int32_t change)
{
  nsCOMPtr<nsIContentViewer> cv;
  nsresult rv = GetContentViewer(getter_AddRefs(cv));
  NS_ENSURE_SUCCESS(rv, rv);

  float fullzoom;
  float zoomMin = ((float)Preferences::GetInt("zoom.minPercent", 50)) / 100;
  float zoomMax = ((float)Preferences::GetInt("zoom.maxPercent", 300)) / 100;
  cv->GetFullZoom(&fullzoom);
  fullzoom += ((float)change) / 10;
  if (fullzoom < zoomMin)
    fullzoom = zoomMin;
  else if (fullzoom > zoomMax)
    fullzoom = zoomMax;
  cv->SetFullZoom(fullzoom);

  return NS_OK;
}

void
EventStateManager::DoScrollHistory(int32_t direction)
{
  nsCOMPtr<nsISupports> pcContainer(mPresContext->GetContainerWeak());
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
EventStateManager::DoScrollZoom(nsIFrame* aTargetFrame,
                                int32_t adjustment)
{
  
  nsIContent *content = aTargetFrame->GetContent();
  if (content &&
      !content->IsNodeOfType(nsINode::eHTML_FORM_CONTROL) &&
      !nsContentUtils::IsInChromeDocshell(content->OwnerDoc()))
    {
      
      int32_t change = (adjustment > 0) ? -1 : 1;

      if (Preferences::GetBool("browser.zoom.full") || content->OwnerDoc()->IsSyntheticDocument()) {
        ChangeFullZoom(change);
      } else {
        ChangeTextSize(change);
      }
      EnsureDocument(mPresContext);
      nsContentUtils::DispatchChromeEvent(mDocument, static_cast<nsIDocument*>(mDocument),
                                          NS_LITERAL_STRING("ZoomChangeUsingMouseWheel"),
                                          true, true);
    }
}

static nsIFrame*
GetParentFrameToScroll(nsIFrame* aFrame)
{
  if (!aFrame)
    return nullptr;

  if (aFrame->StyleDisplay()->mPosition == NS_STYLE_POSITION_FIXED &&
      nsLayoutUtils::IsReallyFixedPos(aFrame))
    return aFrame->PresContext()->GetPresShell()->GetRootScrollFrame();

  return aFrame->GetParent();
}

 bool
EventStateManager::CanVerticallyScrollFrameWithWheel(nsIFrame* aFrame)
{
  nsIContent* c = aFrame->GetContent();
  if (!c) {
    return true;
  }
  nsCOMPtr<nsITextControlElement> ctrl =
    do_QueryInterface(c->IsInAnonymousSubtree() ? c->GetBindingParent() : c);
  if (ctrl && ctrl->IsSingleLineTextControl()) {
    return false;
  }
  return true;
}

void
EventStateManager::DispatchLegacyMouseScrollEvents(nsIFrame* aTargetFrame,
                                                   WidgetWheelEvent* aEvent,
                                                   nsEventStatus* aStatus)
{
  MOZ_ASSERT(aEvent);
  MOZ_ASSERT(aStatus);

  if (!aTargetFrame || *aStatus == nsEventStatus_eConsumeNoDefault) {
    return;
  }

  
  
  nsIScrollableFrame* scrollTarget =
    ComputeScrollTarget(aTargetFrame, aEvent,
                        COMPUTE_LEGACY_MOUSE_SCROLL_EVENT_TARGET);

  nsIFrame* scrollFrame = do_QueryFrame(scrollTarget);
  nsPresContext* pc =
    scrollFrame ? scrollFrame->PresContext() : aTargetFrame->PresContext();

  
  nsSize scrollAmount = GetScrollAmount(pc, aEvent, scrollTarget);
  nsIntSize scrollAmountInCSSPixels(
    nsPresContext::AppUnitsToIntCSSPixels(scrollAmount.width),
    nsPresContext::AppUnitsToIntCSSPixels(scrollAmount.height));

  
  
  
  
  
  
  
  int32_t scrollDeltaX, scrollDeltaY, pixelDeltaX, pixelDeltaY;
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
      MOZ_CRASH("Invalid deltaMode value comes");
  }

  
  
  
  
  

  nsWeakFrame targetFrame(aTargetFrame);

  MOZ_ASSERT(*aStatus != nsEventStatus_eConsumeNoDefault &&
             !aEvent->mFlags.mDefaultPrevented,
             "If you make legacy events dispatched for default prevented wheel "
             "event, you need to initialize stateX and stateY");
  EventState stateX, stateY;
  if (scrollDeltaY) {
    SendLineScrollEvent(aTargetFrame, aEvent, stateY,
                        scrollDeltaY, DELTA_DIRECTION_Y);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (pixelDeltaY) {
    SendPixelScrollEvent(aTargetFrame, aEvent, stateY,
                         pixelDeltaY, DELTA_DIRECTION_Y);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (scrollDeltaX) {
    SendLineScrollEvent(aTargetFrame, aEvent, stateX,
                        scrollDeltaX, DELTA_DIRECTION_X);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (pixelDeltaX) {
    SendPixelScrollEvent(aTargetFrame, aEvent, stateX,
                         pixelDeltaX, DELTA_DIRECTION_X);
    if (!targetFrame.IsAlive()) {
      *aStatus = nsEventStatus_eConsumeNoDefault;
      return;
    }
  }

  if (stateY.mDefaultPrevented || stateX.mDefaultPrevented) {
    *aStatus = nsEventStatus_eConsumeNoDefault;
    aEvent->mFlags.mDefaultPrevented = true;
    aEvent->mFlags.mDefaultPreventedByContent |=
      stateY.mDefaultPreventedByContent || stateX.mDefaultPreventedByContent;
  }
}

void
EventStateManager::SendLineScrollEvent(nsIFrame* aTargetFrame,
                                       WidgetWheelEvent* aEvent,
                                       EventState& aState,
                                       int32_t aDelta,
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

  WidgetMouseScrollEvent event(aEvent->mFlags.mIsTrusted, NS_MOUSE_SCROLL,
                               aEvent->widget);
  event.mFlags.mDefaultPrevented = aState.mDefaultPrevented;
  event.mFlags.mDefaultPreventedByContent = aState.mDefaultPreventedByContent;
  event.refPoint = aEvent->refPoint;
  event.widget = aEvent->widget;
  event.time = aEvent->time;
  event.timeStamp = aEvent->timeStamp;
  event.modifiers = aEvent->modifiers;
  event.buttons = aEvent->buttons;
  event.isHorizontal = (aDeltaDirection == DELTA_DIRECTION_X);
  event.delta = aDelta;
  event.inputSource = aEvent->inputSource;

  nsEventStatus status = nsEventStatus_eIgnore;
  EventDispatcher::Dispatch(targetContent, aTargetFrame->PresContext(),
                            &event, nullptr, &status);
  aState.mDefaultPrevented =
    event.mFlags.mDefaultPrevented || status == nsEventStatus_eConsumeNoDefault;
  aState.mDefaultPreventedByContent = event.mFlags.mDefaultPreventedByContent;
}

void
EventStateManager::SendPixelScrollEvent(nsIFrame* aTargetFrame,
                                        WidgetWheelEvent* aEvent,
                                        EventState& aState,
                                        int32_t aPixelDelta,
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

  WidgetMouseScrollEvent event(aEvent->mFlags.mIsTrusted, NS_MOUSE_PIXEL_SCROLL,
                               aEvent->widget);
  event.mFlags.mDefaultPrevented = aState.mDefaultPrevented;
  event.mFlags.mDefaultPreventedByContent = aState.mDefaultPreventedByContent;
  event.refPoint = aEvent->refPoint;
  event.widget = aEvent->widget;
  event.time = aEvent->time;
  event.timeStamp = aEvent->timeStamp;
  event.modifiers = aEvent->modifiers;
  event.buttons = aEvent->buttons;
  event.isHorizontal = (aDeltaDirection == DELTA_DIRECTION_X);
  event.delta = aPixelDelta;
  event.inputSource = aEvent->inputSource;

  nsEventStatus status = nsEventStatus_eIgnore;
  EventDispatcher::Dispatch(targetContent, aTargetFrame->PresContext(),
                            &event, nullptr, &status);
  aState.mDefaultPrevented =
    event.mFlags.mDefaultPrevented || status == nsEventStatus_eConsumeNoDefault;
  aState.mDefaultPreventedByContent = event.mFlags.mDefaultPreventedByContent;
}

nsIScrollableFrame*
EventStateManager::ComputeScrollTarget(nsIFrame* aTargetFrame,
                                       WidgetWheelEvent* aEvent,
                                       ComputeScrollTargetOptions aOptions)
{
  return ComputeScrollTarget(aTargetFrame, aEvent->deltaX, aEvent->deltaY,
                             aEvent, aOptions);
}




nsIScrollableFrame*
EventStateManager::ComputeScrollTarget(nsIFrame* aTargetFrame,
                                       double aDirectionX,
                                       double aDirectionY,
                                       WidgetWheelEvent* aEvent,
                                       ComputeScrollTargetOptions aOptions)
{
  if (aOptions & PREFER_MOUSE_WHEEL_TRANSACTION) {
    
    
    
    
    
    
    
    
    
    nsIFrame* lastScrollFrame = WheelTransaction::GetTargetFrame();
    if (lastScrollFrame) {
      nsIScrollableFrame* frameToScroll =
        lastScrollFrame->GetScrollTargetFrame();
      if (frameToScroll) {
        return frameToScroll;
      }
    }
  }

  
  
  
  if (!aDirectionX && !aDirectionY) {
    return nullptr;
  }

  bool checkIfScrollableX =
    aDirectionX && (aOptions & PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_X_AXIS);
  bool checkIfScrollableY =
    aDirectionY && (aOptions & PREFER_ACTUAL_SCROLLABLE_TARGET_ALONG_Y_AXIS);

  nsIScrollableFrame* frameToScroll = nullptr;
  nsIFrame* scrollFrame =
    !(aOptions & START_FROM_PARENT) ? aTargetFrame :
                                      GetParentFrameToScroll(aTargetFrame);
  for (; scrollFrame; scrollFrame = GetParentFrameToScroll(scrollFrame)) {
    
    frameToScroll = scrollFrame->GetScrollTargetFrame();
    if (!frameToScroll) {
      continue;
    }

    
    if (checkIfScrollableY) {
      if (!CanVerticallyScrollFrameWithWheel(scrollFrame)) {
        continue;
      }
    }

    if (!checkIfScrollableX && !checkIfScrollableY) {
      return frameToScroll;
    }

    ScrollbarStyles ss = frameToScroll->GetScrollbarStyles();
    bool hiddenForV = (NS_STYLE_OVERFLOW_HIDDEN == ss.mVertical);
    bool hiddenForH = (NS_STYLE_OVERFLOW_HIDDEN == ss.mHorizontal);
    if ((hiddenForV && hiddenForH) ||
        (checkIfScrollableY && !checkIfScrollableX && hiddenForV) ||
        (checkIfScrollableX && !checkIfScrollableY && hiddenForH)) {
      continue;
    }

    
    
    bool canScroll =
      WheelHandlingUtils::CanScrollOn(frameToScroll, aDirectionX, aDirectionY);
    
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
  aOptions =
    static_cast<ComputeScrollTargetOptions>(aOptions & ~START_FROM_PARENT);
  return newFrame ? ComputeScrollTarget(newFrame, aEvent, aOptions) : nullptr;
}

nsSize
EventStateManager::GetScrollAmount(nsPresContext* aPresContext,
                                   WidgetWheelEvent* aEvent,
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
  return nsSize(fm->AveCharWidth(), fm->MaxHeight());
}

void
EventStateManager::DoScrollText(nsIScrollableFrame* aScrollableFrame,
                                WidgetWheelEvent* aEvent)
{
  MOZ_ASSERT(aScrollableFrame);
  MOZ_ASSERT(aEvent);

  nsIFrame* scrollFrame = do_QueryFrame(aScrollableFrame);
  MOZ_ASSERT(scrollFrame);
  nsWeakFrame scrollFrameWeak(scrollFrame);

  nsIFrame* lastScrollFrame = WheelTransaction::GetTargetFrame();
  if (!lastScrollFrame) {
    WheelTransaction::BeginTransaction(scrollFrame, aEvent);
  } else if (lastScrollFrame != scrollFrame) {
    WheelTransaction::EndTransaction();
    WheelTransaction::BeginTransaction(scrollFrame, aEvent);
  } else {
    WheelTransaction::UpdateTransaction(aEvent);
  }

  
  
  
  
  if (!scrollFrameWeak.IsAlive()) {
    WheelTransaction::EndTransaction();
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

  
  ScrollbarStyles overflowStyle = aScrollableFrame->GetScrollbarStyles();
  if (overflowStyle.mHorizontal == NS_STYLE_OVERFLOW_HIDDEN) {
    actualDevPixelScrollAmount.x = 0;
  }
  if (overflowStyle.mVertical == NS_STYLE_OVERFLOW_HIDDEN) {
    actualDevPixelScrollAmount.y = 0;
  }

  nsIScrollbarMediator::ScrollSnapMode snapMode = nsIScrollbarMediator::DISABLE_SNAP;
  nsIAtom* origin = nullptr;
  switch (aEvent->deltaMode) {
    case nsIDOMWheelEvent::DOM_DELTA_LINE:
      origin = nsGkAtoms::mouseWheel;
      snapMode = nsIScrollableFrame::ENABLE_SNAP;
      break;
    case nsIDOMWheelEvent::DOM_DELTA_PAGE:
      origin = nsGkAtoms::pages;
      snapMode = nsIScrollableFrame::ENABLE_SNAP;
      break;
    case nsIDOMWheelEvent::DOM_DELTA_PIXEL:
      origin = nsGkAtoms::pixels;
      break;
    default:
      MOZ_CRASH("Invalid deltaMode value comes");
  }

  
  
  nsSize pageSize = aScrollableFrame->GetPageScrollAmount();
  nsIntSize devPixelPageSize(pc->AppUnitsToDevPixels(pageSize.width),
                             pc->AppUnitsToDevPixels(pageSize.height));
  if (!WheelPrefs::GetInstance()->IsOverOnePageScrollAllowedX(aEvent) &&
      DeprecatedAbs(actualDevPixelScrollAmount.x) > devPixelPageSize.width) {
    actualDevPixelScrollAmount.x =
      (actualDevPixelScrollAmount.x >= 0) ? devPixelPageSize.width :
                                            -devPixelPageSize.width;
  }

  if (!WheelPrefs::GetInstance()->IsOverOnePageScrollAllowedY(aEvent) &&
      DeprecatedAbs(actualDevPixelScrollAmount.y) > devPixelPageSize.height) {
    actualDevPixelScrollAmount.y =
      (actualDevPixelScrollAmount.y >= 0) ? devPixelPageSize.height :
                                            -devPixelPageSize.height;
  }

  bool isDeltaModePixel =
    (aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_PIXEL);

  nsIScrollableFrame::ScrollMode mode;
  switch (aEvent->scrollType) {
    case WidgetWheelEvent::SCROLL_DEFAULT:
      if (isDeltaModePixel) {
        mode = nsIScrollableFrame::NORMAL;
      } else {
        mode = nsIScrollableFrame::SMOOTH;
      }
      break;
    case WidgetWheelEvent::SCROLL_SYNCHRONOUSLY:
      mode = nsIScrollableFrame::INSTANT;
      break;
    case WidgetWheelEvent::SCROLL_ASYNCHRONOUSELY:
      mode = nsIScrollableFrame::NORMAL;
      break;
    case WidgetWheelEvent::SCROLL_SMOOTHLY:
      mode = nsIScrollableFrame::SMOOTH;
      break;
    default:
      MOZ_CRASH("Invalid scrollType value comes");
  }

  nsIScrollableFrame::ScrollMomentum momentum =
    aEvent->isMomentum ? nsIScrollableFrame::SYNTHESIZED_MOMENTUM_EVENT
                       : nsIScrollableFrame::NOT_MOMENTUM;

  nsIntPoint overflow;
  aScrollableFrame->ScrollBy(actualDevPixelScrollAmount,
                             nsIScrollableFrame::DEVICE_PIXELS,
                             mode, &overflow, origin, momentum, snapMode);

  if (!scrollFrameWeak.IsAlive()) {
    
    
    
    aEvent->overflowDeltaX = aEvent->overflowDeltaY = 0;
  } else if (isDeltaModePixel) {
    aEvent->overflowDeltaX = overflow.x;
    aEvent->overflowDeltaY = overflow.y;
  } else {
    aEvent->overflowDeltaX =
      static_cast<double>(overflow.x) / scrollAmountInDevPixels.width;
    aEvent->overflowDeltaY =
      static_cast<double>(overflow.y) / scrollAmountInDevPixels.height;
  }

  
  
  
  
  
  
  if (scrollFrameWeak.IsAlive()) {
    if (aEvent->deltaX &&
        overflowStyle.mHorizontal == NS_STYLE_OVERFLOW_HIDDEN &&
        !ComputeScrollTarget(scrollFrame, aEvent,
                             COMPUTE_SCROLLABLE_ANCESTOR_ALONG_X_AXIS)) {
      aEvent->overflowDeltaX = aEvent->deltaX;
    }
    if (aEvent->deltaY &&
        overflowStyle.mVertical == NS_STYLE_OVERFLOW_HIDDEN &&
        !ComputeScrollTarget(scrollFrame, aEvent,
                             COMPUTE_SCROLLABLE_ANCESTOR_ALONG_Y_AXIS)) {
      aEvent->overflowDeltaY = aEvent->deltaY;
    }
  }

  NS_ASSERTION(aEvent->overflowDeltaX == 0 ||
    (aEvent->overflowDeltaX > 0) == (aEvent->deltaX > 0),
    "The sign of overflowDeltaX is different from the scroll direction");
  NS_ASSERTION(aEvent->overflowDeltaY == 0 ||
    (aEvent->overflowDeltaY > 0) == (aEvent->deltaY > 0),
    "The sign of overflowDeltaY is different from the scroll direction");

  WheelPrefs::GetInstance()->CancelApplyingUserPrefsFromOverflowDelta(aEvent);
}

void
EventStateManager::DecideGestureEvent(WidgetGestureNotifyEvent* aEvent,
                                      nsIFrame* targetFrame)
{

  NS_ASSERTION(aEvent->message == NS_GESTURENOTIFY_EVENT_START,
               "DecideGestureEvent called with a non-gesture event");

  











  WidgetGestureNotifyEvent::ePanDirection panDirection =
    WidgetGestureNotifyEvent::ePanNone;
  bool displayPanFeedback = false;
  for (nsIFrame* current = targetFrame; current;
       current = nsLayoutUtils::GetCrossDocParentFrame(current)) {

    
    
    
    if (current && IsRemoteTarget(current->GetContent())) {
      panDirection = WidgetGestureNotifyEvent::ePanBoth;
      
      displayPanFeedback = false;
      break;
    }

    nsIAtom* currentFrameType = current->GetType();

    
    if (currentFrameType == nsGkAtoms::scrollbarFrame) {
      panDirection = WidgetGestureNotifyEvent::ePanNone;
      break;
    }

#ifdef MOZ_XUL
    
    nsTreeBodyFrame* treeFrame = do_QueryFrame(current);
    if (treeFrame) {
      if (treeFrame->GetHorizontalOverflow()) {
        panDirection = WidgetGestureNotifyEvent::ePanHorizontal;
      }
      if (treeFrame->GetVerticalOverflow()) {
        panDirection = WidgetGestureNotifyEvent::ePanVertical;
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
          panDirection = WidgetGestureNotifyEvent::ePanVertical;
          break;
        }

        if (canScrollHorizontally) {
          panDirection = WidgetGestureNotifyEvent::ePanHorizontal;
          displayPanFeedback = false;
        }
      } else { 
        uint32_t scrollbarVisibility = scrollableFrame->GetScrollbarVisibility();

        
        if (scrollbarVisibility & nsIScrollableFrame::VERTICAL) {
          panDirection = WidgetGestureNotifyEvent::ePanVertical;
          displayPanFeedback = true;
          break;
        }

        if (scrollbarVisibility & nsIScrollableFrame::HORIZONTAL) {
          panDirection = WidgetGestureNotifyEvent::ePanHorizontal;
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
    if (aNode->IsXULElement()) {
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

void
EventStateManager::PostHandleKeyboardEvent(WidgetKeyboardEvent* aKeyboardEvent,
                                           nsEventStatus& aStatus,
                                           bool dispatchedToContentProcess)
{
  if (aStatus == nsEventStatus_eConsumeNoDefault) {
    return;
  }

  
  
  switch(aKeyboardEvent->keyCode) {
    case NS_VK_TAB:
    case NS_VK_F6:
      
      if (!aKeyboardEvent->IsAlt()) {
        
        
        
        
        if (dispatchedToContentProcess)
          break;

        EnsureDocument(mPresContext);
        nsIFocusManager* fm = nsFocusManager::GetFocusManager();
        if (fm && mDocument) {
          
          bool isDocMove =
            aKeyboardEvent->IsControl() || aKeyboardEvent->keyCode == NS_VK_F6;
          uint32_t dir = aKeyboardEvent->IsShift() ?
            (isDocMove ? static_cast<uint32_t>(nsIFocusManager::MOVEFOCUS_BACKWARDDOC) :
                         static_cast<uint32_t>(nsIFocusManager::MOVEFOCUS_BACKWARD)) :
            (isDocMove ? static_cast<uint32_t>(nsIFocusManager::MOVEFOCUS_FORWARDDOC) :
                         static_cast<uint32_t>(nsIFocusManager::MOVEFOCUS_FORWARD));
          nsCOMPtr<nsIDOMElement> result;
          fm->MoveFocus(mDocument->GetWindow(), nullptr, dir,
                        nsIFocusManager::FLAG_BYKEY,
                        getter_AddRefs(result));
        }
        aStatus = nsEventStatus_eConsumeNoDefault;
      }
      return;
    case 0:
      
      break;
    default:
      return;
  }

  switch(aKeyboardEvent->mKeyNameIndex) {
    case KEY_NAME_INDEX_ZoomIn:
    case KEY_NAME_INDEX_ZoomOut:
      ChangeFullZoom(
        aKeyboardEvent->mKeyNameIndex == KEY_NAME_INDEX_ZoomIn ? 1 : -1);
      aStatus = nsEventStatus_eConsumeNoDefault;
      break;
    default:
      break;
  }
}

nsresult
EventStateManager::PostHandleEvent(nsPresContext* aPresContext,
                                   WidgetEvent* aEvent,
                                   nsIFrame* aTargetFrame,
                                   nsEventStatus* aStatus)
{
  NS_ENSURE_ARG(aPresContext);
  NS_ENSURE_ARG_POINTER(aStatus);

  bool dispatchedToContentProcess = HandleCrossProcessEvent(aEvent,
                                                            aStatus);

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
      WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
      if (mouseEvent->button == WidgetMouseEvent::eLeftButton &&
          !sNormalLMouseEventInProcess) {
        
        
        nsIPresShell::SetCapturingContent(nullptr, 0);
        break;
      }

      
      
      
      
      if (dispatchedToContentProcess && !nsIPresShell::GetCapturingContent()) {
        nsIContent* content = mCurrentTarget ? mCurrentTarget->GetContent() : nullptr;
        nsIPresShell::SetCapturingContent(content, 0);
      }

      nsCOMPtr<nsIContent> activeContent;
      if (nsEventStatus_eConsumeNoDefault != *aStatus) {
        nsCOMPtr<nsIContent> newFocus;      
        bool suppressBlur = false;
        if (mCurrentTarget) {
          mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(newFocus));
          const nsStyleUserInterface* ui = mCurrentTarget->StyleUserInterface();
          activeContent = mCurrentTarget->GetContent();

          
          
          
          
          
          
          
          
          
          
          
          
          
          suppressBlur = (ui->mUserFocus == NS_STYLE_USER_FOCUS_IGNORE);

          if (!suppressBlur) {
            nsCOMPtr<Element> element = do_QueryInterface(aEvent->target);
            suppressBlur = element &&
                           element->State().HasState(NS_EVENT_STATE_DISABLED);
          }

          if (!suppressBlur) {
            nsCOMPtr<nsIDOMXULControlElement> xulControl =
              do_QueryInterface(aEvent->target);
            if (xulControl) {
              bool disabled;
              xulControl->GetDisabled(&disabled);
              suppressBlur = disabled;
            }
          }
        }

        if (!suppressBlur) {
          suppressBlur = nsContentUtils::IsUserFocusIgnored(activeContent);
        }

        nsIFrame* currFrame = mCurrentTarget;

        
        
        
        
        
        
        
        
        if (newFocus && !newFocus->IsEditable()) {
          nsIDocument *doc = newFocus->GetCrossShadowCurrentDoc();
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
          
          
          const nsStyleDisplay* display = currFrame->StyleDisplay();
          if (display->mDisplay == NS_STYLE_DISPLAY_POPUP) {
            newFocus = nullptr;
            break;
          }

          int32_t tabIndexUnused;
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
              if (!activeContent || !activeContent->IsXULElement())
#endif
                fm->ClearFocus(mDocument->GetWindow());
              fm->SetFocusedWindow(mDocument->GetWindow());
            }
          }
        }

        
        if (mouseEvent->button != WidgetMouseEvent::eLeftButton) {
          break;
        }

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
          if (currentWindow && mDocument->GetWindow() &&
              currentWindow != mDocument->GetWindow() &&
              !nsContentUtils::IsChromeDoc(mDocument)) {
            nsCOMPtr<nsIDOMWindow> currentTop;
            nsCOMPtr<nsIDOMWindow> newTop;
            currentWindow->GetTop(getter_AddRefs(currentTop));
            mDocument->GetWindow()->GetTop(getter_AddRefs(newTop));
            nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(currentWindow);
            nsCOMPtr<nsIDocument> currentDoc = win->GetExtantDoc();
            if (nsContentUtils::IsChromeDoc(currentDoc) ||
                (currentTop && newTop && currentTop != newTop)) {
              fm->SetFocusedWindow(mDocument->GetWindow());
            }
          }
        }
      }
      SetActiveManager(this, activeContent);
    }
    break;
  case NS_POINTER_CANCEL: {
    if(WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent()) {
      GenerateMouseEnterExit(mouseEvent);
    }
    
    
  }
  case NS_POINTER_UP: {
    WidgetPointerEvent* pointerEvent = aEvent->AsPointerEvent();
    
    
    if (pointerEvent->inputSource == nsIDOMMouseEvent::MOZ_SOURCE_TOUCH) {
      mPointersEnterLeaveHelper.Remove(pointerEvent->pointerId);
      GenerateMouseEnterExit(pointerEvent);
    }
    break;
  }
  case NS_MOUSE_BUTTON_UP:
    {
      ClearGlobalActiveContent(this);
      WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
      if (mouseEvent && mouseEvent->IsReal()) {
        if (!mCurrentTarget) {
          GetEventTarget();
        }
        
        
        ret = CheckForAndDispatchClick(presContext, mouseEvent, aStatus);
      }

      nsIPresShell *shell = presContext->GetPresShell();
      if (shell) {
        nsRefPtr<nsFrameSelection> frameSelection = shell->FrameSelection();
        frameSelection->SetDragState(false);
      }
    }
    break;
  case NS_WHEEL_STOP:
    {
      MOZ_ASSERT(aEvent->mFlags.mIsTrusted);
      ScrollbarsForWheel::MayInactivate();
      WidgetWheelEvent* wheelEvent = aEvent->AsWheelEvent();
      nsIScrollableFrame* scrollTarget =
        ComputeScrollTarget(aTargetFrame, wheelEvent,
                            COMPUTE_DEFAULT_ACTION_TARGET);
      if (scrollTarget) {
        scrollTarget->ScrollSnap();
      }
    }
    break;
  case NS_WHEEL_WHEEL:
  case NS_WHEEL_START:
    {
      MOZ_ASSERT(aEvent->mFlags.mIsTrusted);

      if (*aStatus == nsEventStatus_eConsumeNoDefault) {
        ScrollbarsForWheel::Inactivate();
        break;
      }

      WidgetWheelEvent* wheelEvent = aEvent->AsWheelEvent();

      
      
      WheelPrefs::Action action;
      if (gfxPrefs::AsyncPanZoomEnabled() &&
          layers::APZCTreeManager::WillHandleWheelEvent(wheelEvent))
      {
        action = WheelPrefs::ACTION_NONE;
      } else {
        action = WheelPrefs::GetInstance()->ComputeActionFor(wheelEvent);
      }
      switch (action) {
        case WheelPrefs::ACTION_SCROLL: {
          
          

          ScrollbarsForWheel::PrepareToScrollText(this, aTargetFrame, wheelEvent);

          if (aEvent->message != NS_WHEEL_WHEEL ||
              (!wheelEvent->deltaX && !wheelEvent->deltaY)) {
            break;
          }

          nsIScrollableFrame* scrollTarget =
            ComputeScrollTarget(aTargetFrame, wheelEvent,
                                COMPUTE_DEFAULT_ACTION_TARGET);

          ScrollbarsForWheel::SetActiveScrollTarget(scrollTarget);

          nsIFrame* rootScrollFrame = !aTargetFrame ? nullptr :
            aTargetFrame->PresContext()->PresShell()->GetRootScrollFrame();
          nsIScrollableFrame* rootScrollableFrame = nullptr;
          if (rootScrollFrame) {
            rootScrollableFrame = do_QueryFrame(rootScrollFrame);
          }
          if (!scrollTarget || scrollTarget == rootScrollableFrame) {
            wheelEvent->mViewPortIsOverscrolled = true;
          }
          wheelEvent->overflowDeltaX = wheelEvent->deltaX;
          wheelEvent->overflowDeltaY = wheelEvent->deltaY;
          WheelPrefs::GetInstance()->
            CancelApplyingUserPrefsFromOverflowDelta(wheelEvent);
          if (scrollTarget) {
            DoScrollText(scrollTarget, wheelEvent);
          } else {
            WheelTransaction::EndTransaction();
            ScrollbarsForWheel::Inactivate();
          }
          break;
        }
        case WheelPrefs::ACTION_HISTORY: {
          
          
          int32_t intDelta = wheelEvent->GetPreferredIntDelta();
          if (!intDelta) {
            break;
          }
          DoScrollHistory(intDelta);
          break;
        }
        case WheelPrefs::ACTION_ZOOM: {
          
          
          int32_t intDelta = wheelEvent->GetPreferredIntDelta();
          if (!intDelta) {
            break;
          }
          DoScrollZoom(aTargetFrame, intDelta);
          break;
        }
        case WheelPrefs::ACTION_NONE:
        default:
          
          
          wheelEvent->overflowDeltaX = wheelEvent->deltaX;
          wheelEvent->overflowDeltaY = wheelEvent->deltaY;
          WheelPrefs::GetInstance()->
            CancelApplyingUserPrefsFromOverflowDelta(wheelEvent);
          break;
      }
      *aStatus = nsEventStatus_eConsumeNoDefault;
    }
    break;

  case NS_GESTURENOTIFY_EVENT_START:
    {
      if (nsEventStatus_eConsumeNoDefault != *aStatus) {
        DecideGestureEvent(aEvent->AsGestureNotifyEvent(), mCurrentTarget);
      }
    }
    break;

  case NS_DRAGDROP_ENTER:
  case NS_DRAGDROP_OVER:
    {
      NS_ASSERTION(aEvent->mClass == eDragEventClass, "Expected a drag event");

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

      WidgetDragEvent *dragEvent = aEvent->AsDragEvent();

      
      
      UpdateDragDataTransfer(dragEvent);

      
      
      
      
      
      
      
      
      
      uint32_t dropEffect = nsIDragService::DRAGDROP_ACTION_NONE;
      uint32_t action = nsIDragService::DRAGDROP_ACTION_NONE;
      if (nsEventStatus_eConsumeNoDefault == *aStatus) {
        
        if (dragEvent->dataTransfer) {
          
          dataTransfer = do_QueryInterface(dragEvent->dataTransfer);
          dataTransfer->GetDropEffectInt(&dropEffect);
        }
        else {
          
          
          
          
          
          
          
          dataTransfer = initialDataTransfer;

          dragSession->GetDragAction(&action);

          
          
          dropEffect = nsContentUtils::FilterDropEffect(action,
                         nsIDragService::DRAGDROP_ACTION_UNINITIALIZED);
        }

        
        
        
        uint32_t effectAllowed = nsIDragService::DRAGDROP_ACTION_UNINITIALIZED;
        if (dataTransfer)
          dataTransfer->GetEffectAllowedInt(&effectAllowed);

        
        
        
        
        
        
        if (effectAllowed == nsIDragService::DRAGDROP_ACTION_UNINITIALIZED ||
            dropEffect & effectAllowed)
          action = dropEffect;

        if (action == nsIDragService::DRAGDROP_ACTION_NONE)
          dropEffect = nsIDragService::DRAGDROP_ACTION_NONE;

        
        dragSession->SetDragAction(action);
        dragSession->SetCanDrop(action != nsIDragService::DRAGDROP_ACTION_NONE);

        
        
        if (aEvent->message == NS_DRAGDROP_OVER && !isChromeDoc) {
          
          
          dragSession->SetOnlyChromeDrop(
            !dragEvent->mDefaultPreventedOnContent);
        }
      } else if (aEvent->message == NS_DRAGDROP_OVER && !isChromeDoc) {
        
        dragSession->SetOnlyChromeDrop(true);
      }
      if (ContentChild* child = ContentChild::GetSingleton()) {
        child->SendUpdateDropEffect(action, dropEffect);
      }
      if (dispatchedToContentProcess) {
        dragSession->SetCanDrop(true);
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
        WidgetDragEvent event(aEvent->mFlags.mIsTrusted,
                              NS_DRAGDROP_DRAGDROP, widget);

        WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
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
     
     
    GenerateDragDropEnterExit(presContext, aEvent->AsDragEvent());
    break;

  case NS_KEY_BEFORE_UP:
  case NS_KEY_UP:
  case NS_KEY_AFTER_UP:
    break;

  case NS_KEY_PRESS:
    {
      WidgetKeyboardEvent* keyEvent = aEvent->AsKeyboardEvent();
      PostHandleKeyboardEvent(keyEvent, *aStatus, dispatchedToContentProcess);
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
EventStateManager::RemoteQueryContentEvent(WidgetEvent* aEvent)
{
  WidgetQueryContentEvent* queryEvent = aEvent->AsQueryContentEvent();
  if (!IsTargetCrossProcess(queryEvent)) {
    return false;
  }
  
  GetCrossProcessTarget()->HandleQueryContentEvent(*queryEvent);
  return true;
}

TabParent*
EventStateManager::GetCrossProcessTarget()
{
  return TabParent::GetIMETabParent();
}

bool
EventStateManager::IsTargetCrossProcess(WidgetGUIEvent* aEvent)
{
  
  
  nsIContent *focusedContent = GetFocusedContent();
  if (focusedContent && focusedContent->IsEditable())
    return false;
  return TabParent::GetIMETabParent() != nullptr;
}

void
EventStateManager::NotifyDestroyPresContext(nsPresContext* aPresContext)
{
  IMEStateManager::OnDestroyPresContext(aPresContext);
  if (mHoverContent) {
    
    
    
    
    SetContentState(nullptr, NS_EVENT_STATE_HOVER);
  }
  mPointersEnterLeaveHelper.Clear();
}

void
EventStateManager::SetPresContext(nsPresContext* aPresContext)
{
  mPresContext = aPresContext;
}

void
EventStateManager::ClearFrameRefs(nsIFrame* aFrame)
{
  if (aFrame && aFrame == mCurrentTarget) {
    mCurrentTargetContent = aFrame->GetContent();
  }
}

void
EventStateManager::UpdateCursor(nsPresContext* aPresContext,
                                WidgetEvent* aEvent,
                                nsIFrame* aTargetFrame,
                                nsEventStatus* aStatus)
{
  if (aTargetFrame && IsRemoteTarget(aTargetFrame->GetContent())) {
    return;
  }

  int32_t cursor = NS_STYLE_CURSOR_DEFAULT;
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
    
    nsCOMPtr<nsIDocShell> docShell(aPresContext->GetDocShell());
    if (!docShell) return;
    uint32_t busyFlags = nsIDocShell::BUSY_FLAGS_NONE;
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

void
EventStateManager::ClearCachedWidgetCursor(nsIFrame* aTargetFrame)
{
  if (!aTargetFrame) {
    return;
  }
  nsIWidget* aWidget = aTargetFrame->GetNearestWidget();
  if (!aWidget) {
    return;
  }
  aWidget->ClearCachedCursor();
}

nsresult
EventStateManager::SetCursor(int32_t aCursor, imgIContainer* aContainer,
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
  case NS_STYLE_CURSOR_ZOOM_IN:
    c = eCursor_zoom_in;
    break;
  case NS_STYLE_CURSOR_ZOOM_OUT:
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
    uint32_t hotspotX, hotspotY;

    
    
    
    if (aHaveHotspot) {
      int32_t imgWidth, imgHeight;
      aContainer->GetWidth(&imgWidth);
      aContainer->GetHeight(&imgHeight);

      
      hotspotX = aHotspotX > 0.0f
                   ? uint32_t(aHotspotX + 0.5f) : uint32_t(0);
      if (hotspotX >= uint32_t(imgWidth))
        hotspotX = imgWidth - 1;
      hotspotY = aHotspotY > 0.0f
                   ? uint32_t(aHotspotY + 0.5f) : uint32_t(0);
      if (hotspotY >= uint32_t(imgHeight))
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

class MOZ_STACK_CLASS ESMEventCB : public EventDispatchingCallback
{
public:
  explicit ESMEventCB(nsIContent* aTarget) : mTarget(aTarget) {}

  virtual void HandleEvent(EventChainPostVisitor& aVisitor)
  {
    if (aVisitor.mPresContext) {
      nsIFrame* frame = aVisitor.mPresContext->GetPrimaryFrameFor(mTarget);
      if (frame) {
        frame->HandleEvent(aVisitor.mPresContext,
                           aVisitor.mEvent->AsGUIEvent(),
                           &aVisitor.mEventStatus);
      }
    }
  }

  nsCOMPtr<nsIContent> mTarget;
};

 bool
EventStateManager::IsHandlingUserInput()
{
  if (sUserInputEventDepth <= 0) {
    return false;
  }

  TimeDuration timeout = nsContentUtils::HandlingUserInputTimeout();
  return timeout <= TimeDuration(0) ||
         (TimeStamp::Now() - sHandlingInputStart) <= timeout;
}

static void
CreateMouseOrPointerWidgetEvent(WidgetMouseEvent* aMouseEvent,
                                uint32_t aMessage,
                                nsIContent* aRelatedContent,
                                nsAutoPtr<WidgetMouseEvent>& aNewEvent)
{
  WidgetPointerEvent* sourcePointer = aMouseEvent->AsPointerEvent();
  if (sourcePointer) {
    PROFILER_LABEL("Input", "DispatchPointerEvent",
      js::ProfileEntry::Category::EVENTS);

    nsAutoPtr<WidgetPointerEvent> newPointerEvent;
    newPointerEvent =
      new WidgetPointerEvent(aMouseEvent->mFlags.mIsTrusted, aMessage,
                             aMouseEvent->widget);
    newPointerEvent->isPrimary = sourcePointer->isPrimary;
    newPointerEvent->pointerId = sourcePointer->pointerId;
    newPointerEvent->width = sourcePointer->width;
    newPointerEvent->height = sourcePointer->height;
    newPointerEvent->inputSource = sourcePointer->inputSource;
    newPointerEvent->relatedTarget =
      nsIPresShell::GetPointerCapturingContent(sourcePointer->pointerId)
        ? nullptr
        : aRelatedContent;
    aNewEvent = newPointerEvent.forget();
  } else {
    aNewEvent =
      new WidgetMouseEvent(aMouseEvent->mFlags.mIsTrusted, aMessage,
                           aMouseEvent->widget, WidgetMouseEvent::eReal);
    aNewEvent->relatedTarget = aRelatedContent;
  }
  aNewEvent->refPoint = aMouseEvent->refPoint;
  aNewEvent->modifiers = aMouseEvent->modifiers;
  aNewEvent->button = aMouseEvent->button;
  aNewEvent->buttons = aMouseEvent->buttons;
  aNewEvent->pressure = aMouseEvent->pressure;
  aNewEvent->mPluginEvent = aMouseEvent->mPluginEvent;
  aNewEvent->inputSource = aMouseEvent->inputSource;
}

nsIFrame*
EventStateManager::DispatchMouseOrPointerEvent(WidgetMouseEvent* aMouseEvent,
                                               uint32_t aMessage,
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
      do_QueryReferent(EventStateManager::sPointerLockedElement);
    if (!pointerLockedElement) {
      NS_WARNING("Should have pointer locked element, but didn't.");
      return nullptr;
    }
    nsCOMPtr<nsIContent> content = do_QueryInterface(pointerLockedElement);
    return mPresContext->GetPrimaryFrameFor(content);
  }

  mCurrentTargetContent = nullptr;

  if (!aTargetContent) {
    return nullptr;
  }

  nsAutoPtr<WidgetMouseEvent> dispatchEvent;
  CreateMouseOrPointerWidgetEvent(aMouseEvent, aMessage,
                                  aRelatedContent, dispatchEvent);

  nsWeakFrame previousTarget = mCurrentTarget;
  mCurrentTargetContent = aTargetContent;

  nsIFrame* targetFrame = nullptr;

  if (aMouseEvent->AsMouseEvent()) {
    PROFILER_LABEL("Input", "DispatchMouseEvent",
      js::ProfileEntry::Category::EVENTS);
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  ESMEventCB callback(aTargetContent);
  EventDispatcher::Dispatch(aTargetContent, mPresContext, dispatchEvent, nullptr,
                            &status, &callback);

  if (mPresContext) {
    
    
    targetFrame = mPresContext->GetPrimaryFrameFor(aTargetContent);

    
    
    if (aMessage == NS_MOUSE_EXIT_SYNTH && IsRemoteTarget(aTargetContent)) {
      
      nsAutoPtr<WidgetMouseEvent> remoteEvent;
      CreateMouseOrPointerWidgetEvent(aMouseEvent, NS_MOUSE_EXIT,
                                      aRelatedContent, remoteEvent);

      
      
      
      
      mCurrentTarget = targetFrame;
      HandleCrossProcessEvent(remoteEvent, &status);
    }
  }

  mCurrentTargetContent = nullptr;
  mCurrentTarget = previousTarget;

  return targetFrame;
}

class EnterLeaveDispatcher
{
public:
  EnterLeaveDispatcher(EventStateManager* aESM,
                       nsIContent* aTarget, nsIContent* aRelatedTarget,
                       WidgetMouseEvent* aMouseEvent, uint32_t aType)
    : mESM(aESM)
    , mMouseEvent(aMouseEvent)
    , mType(aType)
  {
    nsPIDOMWindow* win =
      aTarget ? aTarget->OwnerDoc()->GetInnerWindow() : nullptr;
    if (aMouseEvent->AsPointerEvent() ? win && win->HasPointerEnterLeaveEventListeners() :
                                        win && win->HasMouseEnterLeaveEventListeners()) {
      mRelatedTarget = aRelatedTarget ?
        aRelatedTarget->FindFirstNonChromeOnlyAccessContent() : nullptr;
      nsINode* commonParent = nullptr;
      if (aTarget && aRelatedTarget) {
        commonParent =
          nsContentUtils::GetCommonAncestor(aTarget, aRelatedTarget);
      }
      nsIContent* current = aTarget;
      
      while (current && current != commonParent) {
        if (!current->ChromeOnlyAccess()) {
          mTargets.AppendObject(current);
        }
        
        current = current->GetParent();
      }
    }
  }

  ~EnterLeaveDispatcher()
  {
    if (mType == NS_MOUSEENTER ||
        mType == NS_POINTER_ENTER) {
      for (int32_t i = mTargets.Count() - 1; i >= 0; --i) {
        mESM->DispatchMouseOrPointerEvent(mMouseEvent, mType, mTargets[i],
                                          mRelatedTarget);
      }
    } else {
      for (int32_t i = 0; i < mTargets.Count(); ++i) {
        mESM->DispatchMouseOrPointerEvent(mMouseEvent, mType, mTargets[i],
                                          mRelatedTarget);
      }
    }
  }

  EventStateManager* mESM;
  nsCOMArray<nsIContent> mTargets;
  nsCOMPtr<nsIContent> mRelatedTarget;
  WidgetMouseEvent* mMouseEvent;
  uint32_t mType;
};

void
EventStateManager::NotifyMouseOut(WidgetMouseEvent* aMouseEvent,
                                  nsIContent* aMovingInto)
{
  OverOutElementsWrapper* wrapper = GetWrapperByEventID(aMouseEvent);

  if (!wrapper->mLastOverElement)
    return;
  
  if (wrapper->mLastOverElement == wrapper->mFirstOutEventElement)
    return;

  if (wrapper->mLastOverFrame) {
    
    
    nsSubDocumentFrame* subdocFrame = do_QueryFrame(wrapper->mLastOverFrame.GetFrame());
    if (subdocFrame) {
      nsCOMPtr<nsIDocShell> docshell;
      subdocFrame->GetDocShell(getter_AddRefs(docshell));
      if (docshell) {
        nsRefPtr<nsPresContext> presContext;
        docshell->GetPresContext(getter_AddRefs(presContext));

        if (presContext) {
          EventStateManager* kidESM = presContext->EventStateManager();
          
          kidESM->NotifyMouseOut(aMouseEvent, nullptr);
        }
      }
    }
  }
  
  
  if (!wrapper->mLastOverElement)
    return;

  
  
  wrapper->mFirstOutEventElement = wrapper->mLastOverElement;

  
  
  
  
  bool isPointer = aMouseEvent->mClass == ePointerEventClass;
  if (!aMovingInto && !isPointer) {
    
    SetContentState(nullptr, NS_EVENT_STATE_HOVER);
  }

  EnterLeaveDispatcher leaveDispatcher(this, wrapper->mLastOverElement,
                                       aMovingInto, aMouseEvent,
                                       isPointer ? NS_POINTER_LEAVE :
                                                   NS_MOUSELEAVE);

  
  DispatchMouseOrPointerEvent(aMouseEvent, isPointer ? NS_POINTER_OUT : NS_MOUSE_EXIT_SYNTH,
                              wrapper->mLastOverElement, aMovingInto);

  wrapper->mLastOverFrame = nullptr;
  wrapper->mLastOverElement = nullptr;

  
  wrapper->mFirstOutEventElement = nullptr;
}

void
EventStateManager::NotifyMouseOver(WidgetMouseEvent* aMouseEvent,
                                   nsIContent* aContent)
{
  NS_ASSERTION(aContent, "Mouse must be over something");

  
  
  bool dispatch = !aMouseEvent->retargetedByPointerCapture;

  OverOutElementsWrapper* wrapper = GetWrapperByEventID(aMouseEvent);

  if (wrapper->mLastOverElement == aContent && dispatch)
    return;

  
  if (aContent == wrapper->mFirstOverEventElement)
    return;

  
  
  
  EnsureDocument(mPresContext);
  if (nsIDocument *parentDoc = mDocument->GetParentDocument()) {
    if (nsIContent *docContent = parentDoc->FindContentForSubDocument(mDocument)) {
      if (nsIPresShell *parentShell = parentDoc->GetShell()) {
        EventStateManager* parentESM =
          parentShell->GetPresContext()->EventStateManager();
        parentESM->NotifyMouseOver(aMouseEvent, docContent);
      }
    }
  }
  
  
  if (wrapper->mLastOverElement == aContent && dispatch)
    return;

  
  
  nsCOMPtr<nsIContent> lastOverElement = wrapper->mLastOverElement;

  bool isPointer = aMouseEvent->mClass == ePointerEventClass;
  
  Maybe<EnterLeaveDispatcher> enterDispatcher;
  if (dispatch) {
    enterDispatcher.emplace(this, aContent, lastOverElement, aMouseEvent,
                            isPointer ? NS_POINTER_ENTER : NS_MOUSEENTER);
  }

  NotifyMouseOut(aMouseEvent, aContent);

  
  
  wrapper->mFirstOverEventElement = aContent;

  if (!isPointer) {
    SetContentState(aContent, NS_EVENT_STATE_HOVER);
  }

  if (dispatch) {
    
    wrapper->mLastOverFrame = 
      DispatchMouseOrPointerEvent(aMouseEvent,
                                  isPointer ? NS_POINTER_OVER : NS_MOUSE_ENTER_SYNTH,
                                  aContent, lastOverElement);
    wrapper->mLastOverElement = aContent;
  } else {
    wrapper->mLastOverFrame = nullptr;
    wrapper->mLastOverElement = nullptr;
  }

  
  wrapper->mFirstOverEventElement = nullptr;
}












static LayoutDeviceIntPoint
GetWindowInnerRectCenter(nsPIDOMWindow* aWindow,
                         nsIWidget* aWidget,
                         nsPresContext* aContext)
{
  NS_ENSURE_TRUE(aWindow && aWidget && aContext, LayoutDeviceIntPoint(0, 0));

  float cssInnerX = 0.0;
  aWindow->GetMozInnerScreenX(&cssInnerX);
  int32_t innerX = int32_t(NS_round(cssInnerX));

  float cssInnerY = 0.0;
  aWindow->GetMozInnerScreenY(&cssInnerY);
  int32_t innerY = int32_t(NS_round(cssInnerY));
 
  int32_t innerWidth = 0;
  aWindow->GetInnerWidth(&innerWidth);

  int32_t innerHeight = 0;
  aWindow->GetInnerHeight(&innerHeight);

  nsIntRect screen;
  aWidget->GetScreenBounds(screen);

  int32_t cssScreenX = aContext->DevPixelsToIntCSSPixels(screen.x);
  int32_t cssScreenY = aContext->DevPixelsToIntCSSPixels(screen.y);

  return LayoutDeviceIntPoint(
    aContext->CSSPixelsToDevPixels(innerX - cssScreenX + innerWidth / 2),
    aContext->CSSPixelsToDevPixels(innerY - cssScreenY + innerHeight / 2));
}

void
EventStateManager::GeneratePointerEnterExit(uint32_t aMessage, WidgetMouseEvent* aEvent)
{
  WidgetPointerEvent pointerEvent(*aEvent);
  pointerEvent.message = aMessage;
  GenerateMouseEnterExit(&pointerEvent);
}

void
EventStateManager::GenerateMouseEnterExit(WidgetMouseEvent* aMouseEvent)
{
  EnsureDocument(mPresContext);
  if (!mDocument)
    return;

  
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aMouseEvent->message) {
  case NS_MOUSE_MOVE:
    {
      
      
      
      if (sIsPointerLocked && aMouseEvent->widget) {
        
        
        
        
        
        
        LayoutDeviceIntPoint center =
          GetWindowInnerRectCenter(mDocument->GetWindow(), aMouseEvent->widget,
                                   mPresContext);
        aMouseEvent->lastRefPoint = center;
        if (aMouseEvent->refPoint != center) {
          
          
          
          
          
          sSynthCenteringPoint = center;
          aMouseEvent->widget->SynthesizeNativeMouseMove(
            center + aMouseEvent->widget->WidgetToScreenOffset());
        } else if (aMouseEvent->refPoint == sSynthCenteringPoint) {
          
          
          aMouseEvent->mFlags.mPropagationStopped = true;
          
          
          sSynthCenteringPoint = kInvalidRefPoint;
        }
      } else if (sLastRefPoint == kInvalidRefPoint) {
        
        
        
        
        aMouseEvent->lastRefPoint = aMouseEvent->refPoint;
      } else {
        aMouseEvent->lastRefPoint = sLastRefPoint;
      }

      
      sLastRefPoint = aMouseEvent->refPoint;

    }
  case NS_POINTER_MOVE:
  case NS_POINTER_DOWN:
    {
      
      nsCOMPtr<nsIContent> targetElement = GetEventTargetContent(aMouseEvent);
      if (!targetElement) {
        
        
        
        targetElement = mDocument->GetRootElement();
      }
      if (targetElement) {
        NotifyMouseOver(aMouseEvent, targetElement);
      }
    }
    break;
  case NS_POINTER_UP:
    {
      
      nsCOMPtr<nsIContent> targetElement = GetEventTargetContent(aMouseEvent);
      if (!targetElement) {
        
        
        
        targetElement = mDocument->GetRootElement();
      }
      if (targetElement) {
        OverOutElementsWrapper* helper = GetWrapperByEventID(aMouseEvent);
        if (helper) {
          helper->mLastOverElement = targetElement;
        }
        NotifyMouseOut(aMouseEvent, nullptr);
      }
    }
    break;
  case NS_POINTER_LEAVE:
  case NS_POINTER_CANCEL:
  case NS_MOUSE_EXIT:
    {
      
      

      OverOutElementsWrapper* helper = GetWrapperByEventID(aMouseEvent);
      if (helper->mLastOverFrame &&
          nsContentUtils::GetTopLevelWidget(aMouseEvent->widget) !=
          nsContentUtils::GetTopLevelWidget(helper->mLastOverFrame->GetNearestWidget())) {
        
        
        break;
      }

      
      
      sLastRefPoint = kInvalidRefPoint;

      NotifyMouseOut(aMouseEvent, nullptr);
    }
    break;
  }

  
  mCurrentTargetContent = targetBeforeEvent;
}

OverOutElementsWrapper*
EventStateManager::GetWrapperByEventID(WidgetMouseEvent* aEvent)
{
  WidgetPointerEvent* pointer = aEvent->AsPointerEvent();
  if (!pointer) {
    MOZ_ASSERT(aEvent->AsMouseEvent() != nullptr);
    if (!mMouseEnterLeaveHelper) {
      mMouseEnterLeaveHelper = new OverOutElementsWrapper();
    }
    return mMouseEnterLeaveHelper;
  }
  nsRefPtr<OverOutElementsWrapper> helper;
  if (!mPointersEnterLeaveHelper.Get(pointer->pointerId, getter_AddRefs(helper))) {
    helper = new OverOutElementsWrapper();
    mPointersEnterLeaveHelper.Put(pointer->pointerId, helper);
  }

  return helper;
}

void
EventStateManager::SetPointerLock(nsIWidget* aWidget,
                                  nsIContent* aElement)
{
  
  sIsPointerLocked = !!aElement;

  if (!aWidget) {
    return;
  }

  
  WheelTransaction::EndTransaction();

  
  nsCOMPtr<nsIDragService> dragService =
    do_GetService("@mozilla.org/widget/dragservice;1");

  if (sIsPointerLocked) {
    
    mPreLockPoint = sLastRefPoint;

    
    
    
    sLastRefPoint = GetWindowInnerRectCenter(aElement->OwnerDoc()->GetWindow(),
                                             aWidget,
                                             mPresContext);
    aWidget->SynthesizeNativeMouseMove(sLastRefPoint + aWidget->WidgetToScreenOffset());

    
    nsIPresShell::SetCapturingContent(aElement, CAPTURE_POINTERLOCK);

    
    if (dragService) {
      dragService->Suppress();
    }
  } else {
    
    
    
    
    sLastRefPoint = mPreLockPoint;
    aWidget->SynthesizeNativeMouseMove(mPreLockPoint + aWidget->WidgetToScreenOffset());

    
    nsIPresShell::SetCapturingContent(nullptr, CAPTURE_POINTERLOCK);

    
    if (dragService) {
      dragService->Unsuppress();
    }
  }
}

void
EventStateManager::GenerateDragDropEnterExit(nsPresContext* aPresContext,
                                             WidgetDragEvent* aDragEvent)
{
  
  nsCOMPtr<nsIContent> targetBeforeEvent = mCurrentTargetContent;

  switch(aDragEvent->message) {
  case NS_DRAGDROP_OVER:
    {
      
      
      if (sLastDragOverFrame != mCurrentTarget) {
        
        nsCOMPtr<nsIContent> lastContent;
        nsCOMPtr<nsIContent> targetContent;
        mCurrentTarget->GetContentForEvent(aDragEvent,
                                           getter_AddRefs(targetContent));

        if (sLastDragOverFrame) {
          
          sLastDragOverFrame->GetContentForEvent(aDragEvent,
                                                 getter_AddRefs(lastContent));

          FireDragEnterOrExit(sLastDragOverFrame->PresContext(),
                              aDragEvent, NS_DRAGDROP_EXIT_SYNTH,
                              targetContent, lastContent, sLastDragOverFrame);
        }

        FireDragEnterOrExit(aPresContext, aDragEvent, NS_DRAGDROP_ENTER,
                            lastContent, targetContent, mCurrentTarget);

        if (sLastDragOverFrame) {
          FireDragEnterOrExit(sLastDragOverFrame->PresContext(),
                              aDragEvent, NS_DRAGDROP_LEAVE_SYNTH,
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
        sLastDragOverFrame->GetContentForEvent(aDragEvent,
                                               getter_AddRefs(lastContent));

        nsRefPtr<nsPresContext> lastDragOverFramePresContext = sLastDragOverFrame->PresContext();
        FireDragEnterOrExit(lastDragOverFramePresContext,
                            aDragEvent, NS_DRAGDROP_EXIT_SYNTH,
                            nullptr, lastContent, sLastDragOverFrame);
        FireDragEnterOrExit(lastDragOverFramePresContext,
                            aDragEvent, NS_DRAGDROP_LEAVE_SYNTH,
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
EventStateManager::FireDragEnterOrExit(nsPresContext* aPresContext,
                                       WidgetDragEvent* aDragEvent,
                                       uint32_t aMsg,
                                       nsIContent* aRelatedTarget,
                                       nsIContent* aTargetContent,
                                       nsWeakFrame& aTargetFrame)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  WidgetDragEvent event(aDragEvent->mFlags.mIsTrusted, aMsg,
                        aDragEvent->widget);
  event.refPoint = aDragEvent->refPoint;
  event.modifiers = aDragEvent->modifiers;
  event.buttons = aDragEvent->buttons;
  event.relatedTarget = aRelatedTarget;
  event.inputSource = aDragEvent->inputSource;

  mCurrentTargetContent = aTargetContent;

  if (aTargetContent != aRelatedTarget) {
    
    if (aTargetContent) {
      EventDispatcher::Dispatch(aTargetContent, aPresContext, &event,
                                nullptr, &status);
    }

    
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
EventStateManager::UpdateDragDataTransfer(WidgetDragEvent* dragEvent)
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
EventStateManager::SetClickCount(nsPresContext* aPresContext,
                                 WidgetMouseEvent* aEvent,
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
  case WidgetMouseEvent::eLeftButton:
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

  case WidgetMouseEvent::eMiddleButton:
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

  case WidgetMouseEvent::eRightButton:
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
EventStateManager::CheckForAndDispatchClick(nsPresContext* aPresContext,
                                            WidgetMouseEvent* aEvent,
                                            nsEventStatus* aStatus)
{
  nsresult ret = NS_OK;

  
  
  if (0 != aEvent->clickCount) {
    
    
    if (aEvent->widget && !aEvent->widget->IsEnabled()) {
      return ret;
    }
    
    bool notDispatchToContents =
     (aEvent->button == WidgetMouseEvent::eMiddleButton ||
      aEvent->button == WidgetMouseEvent::eRightButton);

    WidgetMouseEvent event(aEvent->mFlags.mIsTrusted, NS_MOUSE_CLICK,
                           aEvent->widget, WidgetMouseEvent::eReal);
    event.refPoint = aEvent->refPoint;
    event.clickCount = aEvent->clickCount;
    event.modifiers = aEvent->modifiers;
    event.buttons = aEvent->buttons;
    event.time = aEvent->time;
    event.timeStamp = aEvent->timeStamp;
    event.mFlags.mNoContentDispatch = notDispatchToContents;
    event.button = aEvent->button;
    event.inputSource = aEvent->inputSource;

    nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
    if (presShell) {
      nsCOMPtr<nsIContent> mouseContent = GetEventTargetContent(aEvent);
      
      
      
      
      while (mouseContent && !mouseContent->IsElement()) {
        mouseContent = mouseContent->GetParent();
      }

      if (!mouseContent && !mCurrentTarget) {
        return NS_OK;
      }

      
      nsWeakFrame currentTarget = mCurrentTarget;
      ret = presShell->HandleEventWithTarget(&event, currentTarget,
                                             mouseContent, aStatus);
      if (NS_SUCCEEDED(ret) && aEvent->clickCount == 2) {
        
        WidgetMouseEvent event2(aEvent->mFlags.mIsTrusted, NS_MOUSE_DOUBLECLICK,
                                aEvent->widget, WidgetMouseEvent::eReal);
        event2.refPoint = aEvent->refPoint;
        event2.clickCount = aEvent->clickCount;
        event2.modifiers = aEvent->modifiers;
        event2.buttons = aEvent->buttons;
        event2.mFlags.mNoContentDispatch = notDispatchToContents;
        event2.button = aEvent->button;
        event2.inputSource = aEvent->inputSource;

        ret = presShell->HandleEventWithTarget(&event2, currentTarget,
                                               mouseContent, aStatus);
      }
    }
  }
  return ret;
}

nsIFrame*
EventStateManager::GetEventTarget()
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
EventStateManager::GetEventTargetContent(WidgetEvent* aEvent)
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

  nsCOMPtr<nsIContent> content;

  nsIPresShell *presShell = mPresContext->GetPresShell();
  if (presShell) {
    content = presShell->GetEventTargetContent(aEvent);
  }

  
  
  if (!content && mCurrentTarget) {
    mCurrentTarget->GetContentForEvent(aEvent, getter_AddRefs(content));
  }

  return content.forget();
}

static Element*
GetLabelTarget(nsIContent* aPossibleLabel)
{
  mozilla::dom::HTMLLabelElement* label =
    mozilla::dom::HTMLLabelElement::FromContent(aPossibleLabel);
  if (!label)
    return nullptr;

  return label->GetLabeledElement();
}

static nsIContent* FindCommonAncestor(nsIContent *aNode1, nsIContent *aNode2)
{
  
  if (aNode1 && aNode2) {
    
    
    int32_t offset = 0;
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
EventStateManager::SetFullScreenState(Element* aElement, bool aIsFullScreen)
{
  DoStateChange(aElement, NS_EVENT_STATE_FULL_SCREEN, aIsFullScreen);
  Element* ancestor = aElement;
  while ((ancestor = GetParentElement(ancestor))) {
    DoStateChange(ancestor, NS_EVENT_STATE_FULL_SCREEN_ANCESTOR, aIsFullScreen);
  }
}


inline void
EventStateManager::DoStateChange(Element* aElement, EventStates aState,
                                 bool aAddState)
{
  if (aAddState) {
    aElement->AddStates(aState);
  } else {
    aElement->RemoveStates(aState);
  }
}


inline void
EventStateManager::DoStateChange(nsIContent* aContent, EventStates aState,
                                 bool aStateAdded)
{
  if (aContent->IsElement()) {
    DoStateChange(aContent->AsElement(), aState, aStateAdded);
  }
}


void
EventStateManager::UpdateAncestorState(nsIContent* aStartNode,
                                       nsIContent* aStopBefore,
                                       EventStates aState,
                                       bool aAddState)
{
  for (; aStartNode && aStartNode != aStopBefore;
       aStartNode = aStartNode->GetParentElementCrossingShadowRoot()) {
    
    
    
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
    
    
    
    
    
    
    
    
    
    
    
    
    for ( ; aStartNode; aStartNode = aStartNode->GetParentElementCrossingShadowRoot()) {
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
EventStateManager::SetContentState(nsIContent* aContent, EventStates aState)
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
      const nsStyleUserInterface* ui = mCurrentTarget->StyleUserInterface();
      if (ui->mUserInput == NS_STYLE_USER_INPUT_NONE)
        return false;
    }

    if (aState == NS_EVENT_STATE_ACTIVE) {
      
      
      if (aContent && aContent->IsEditable()) {
        aContent = nullptr;
      }
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
                     aContent->GetCrossShadowCurrentDoc() ==
                       mPresContext->PresShell()->GetDocument(),
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

PLDHashOperator
EventStateManager::ResetLastOverForContent(
                     const uint32_t& aIdx,
                     nsRefPtr<OverOutElementsWrapper>& aElemWrapper,
                     void* aClosure)
{
  nsIContent* content = static_cast<nsIContent*>(aClosure);
  if (aElemWrapper && aElemWrapper->mLastOverElement &&
      nsContentUtils::ContentIsDescendantOf(aElemWrapper->mLastOverElement, content)) {
    aElemWrapper->mLastOverElement = nullptr;
  }

  return PL_DHASH_NEXT;
}

void
EventStateManager::ContentRemoved(nsIDocument* aDocument, nsIContent* aContent)
{
  




  if (aContent->IsAnyOfHTMLElements(nsGkAtoms::a, nsGkAtoms::area) &&
      (aContent->AsElement()->State().HasAtLeastOneOfStates(NS_EVENT_STATE_FOCUS |
                                                            NS_EVENT_STATE_HOVER))) {
    nsGenericHTMLElement* element = static_cast<nsGenericHTMLElement*>(aContent);
    element->LeaveLink(
      element->GetPresContext(nsGenericHTMLElement::eForComposedDoc));
  }

  IMEStateManager::OnRemoveContent(mPresContext, aContent);

  
  
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

  
  ResetLastOverForContent(0, mMouseEnterLeaveHelper, aContent);
  mPointersEnterLeaveHelper.Enumerate(
    &EventStateManager::ResetLastOverForContent, aContent);
}

bool
EventStateManager::EventStatusOK(WidgetGUIEvent* aEvent)
{
  return !(aEvent->message == NS_MOUSE_BUTTON_DOWN &&
           aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton &&
           !sNormalLMouseEventInProcess);
}




void
EventStateManager::RegisterAccessKey(nsIContent* aContent, uint32_t aKey)
{
  if (aContent && mAccessKeys.IndexOf(aContent) == -1)
    mAccessKeys.AppendObject(aContent);
}

void
EventStateManager::UnregisterAccessKey(nsIContent* aContent, uint32_t aKey)
{
  if (aContent)
    mAccessKeys.RemoveObject(aContent);
}

uint32_t
EventStateManager::GetRegisteredAccessKey(nsIContent* aContent)
{
  MOZ_ASSERT(aContent);

  if (mAccessKeys.IndexOf(aContent) == -1)
    return 0;

  nsAutoString accessKey;
  aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);
  return accessKey.First();
}

void
EventStateManager::EnsureDocument(nsPresContext* aPresContext)
{
  if (!mDocument)
    mDocument = aPresContext->Document();
}

void
EventStateManager::FlushPendingEvents(nsPresContext* aPresContext)
{
  NS_PRECONDITION(nullptr != aPresContext, "nullptr ptr");
  nsIPresShell *shell = aPresContext->GetPresShell();
  if (shell) {
    shell->FlushPendingNotifications(Flush_InterruptibleLayout);
  }
}

nsIContent*
EventStateManager::GetFocusedContent()
{
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm || !mDocument)
    return nullptr;

  nsCOMPtr<nsPIDOMWindow> focusedWindow;
  return nsFocusManager::GetFocusedDescendant(mDocument->GetWindow(), false,
                                              getter_AddRefs(focusedWindow));
}




bool
EventStateManager::IsShellVisible(nsIDocShell* aShell)
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
EventStateManager::DoContentCommandEvent(WidgetContentCommandEvent* aEvent)
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
EventStateManager::DoContentCommandScrollEvent(
                     WidgetContentCommandEvent* aEvent)
{
  NS_ENSURE_TRUE(mPresContext, NS_ERROR_NOT_AVAILABLE);
  nsIPresShell* ps = mPresContext->GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(aEvent->mScroll.mAmount != 0, NS_ERROR_INVALID_ARG);

  nsIScrollableFrame::ScrollUnit scrollUnit;
  switch (aEvent->mScroll.mUnit) {
    case WidgetContentCommandEvent::eCmdScrollUnit_Line:
      scrollUnit = nsIScrollableFrame::LINES;
      break;
    case WidgetContentCommandEvent::eCmdScrollUnit_Page:
      scrollUnit = nsIScrollableFrame::PAGES;
      break;
    case WidgetContentCommandEvent::eCmdScrollUnit_Whole:
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
      WheelHandlingUtils::CanScrollOn(sf, aEvent->mScroll.mAmount, 0) :
      WheelHandlingUtils::CanScrollOn(sf, 0, aEvent->mScroll.mAmount)) : false;

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
EventStateManager::DoQuerySelectedText(WidgetQueryContentEvent* aEvent)
{
  if (RemoteQueryContentEvent(aEvent)) {
    return;
  }
  ContentEventHandler handler(mPresContext);
  handler.OnQuerySelectedText(aEvent);
}

void
EventStateManager::SetActiveManager(EventStateManager* aNewESM,
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
EventStateManager::ClearGlobalActiveContent(EventStateManager* aClearer)
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
EventStateManager::DeltaAccumulator::InitLineOrPageDelta(
                                       nsIFrame* aTargetFrame,
                                       EventStateManager* aESM,
                                       WidgetWheelEvent* aEvent)
{
  MOZ_ASSERT(aESM);
  MOZ_ASSERT(aEvent);

  
  if (!mLastTime.IsNull()) {
    TimeDuration duration = TimeStamp::Now() - mLastTime;
    if (duration.ToMilliseconds() > WheelTransaction::GetTimeoutTime()) {
      Reset();
    }
  }
  
  if (IsInTransaction()) {
    
    if (mHandlingDeltaMode != aEvent->deltaMode ||
        mIsNoLineOrPageDeltaDevice != aEvent->mIsNoLineOrPageDelta) {
      Reset();
    } else {
      
      
      if (mX && aEvent->deltaX && ((aEvent->deltaX > 0.0) != (mX > 0.0))) {
        mX = mPendingScrollAmountX = 0.0;
      }
      if (mY && aEvent->deltaY && ((aEvent->deltaY > 0.0) != (mY > 0.0))) {
        mY = mPendingScrollAmountY = 0.0;
      }
    }
  }

  mHandlingDeltaMode = aEvent->deltaMode;
  mIsNoLineOrPageDeltaDevice = aEvent->mIsNoLineOrPageDelta;

  
  
  
  if (!mIsNoLineOrPageDeltaDevice &&
      !EventStateManager::WheelPrefs::GetInstance()->
        NeedToComputeLineOrPageDelta(aEvent)) {
    
    
    
    
    
    if (aEvent->deltaX) {
      mX = aEvent->deltaX;
    }
    if (aEvent->deltaY) {
      mY = aEvent->deltaY;
    }
    mLastTime = TimeStamp::Now();
    return;
  }

  mX += aEvent->deltaX;
  mY += aEvent->deltaY;

  if (mHandlingDeltaMode == nsIDOMWheelEvent::DOM_DELTA_PIXEL) {
    
    
    
    
    
    
    
    nsIScrollableFrame* scrollTarget =
      aESM->ComputeScrollTarget(aTargetFrame, aEvent,
                                COMPUTE_LEGACY_MOUSE_SCROLL_EVENT_TARGET);
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
EventStateManager::DeltaAccumulator::Reset()
{
  mX = mY = 0.0;
  mPendingScrollAmountX = mPendingScrollAmountY = 0.0;
  mHandlingDeltaMode = UINT32_MAX;
  mIsNoLineOrPageDeltaDevice = false;
}

nsIntPoint
EventStateManager::DeltaAccumulator::ComputeScrollAmountForDefaultAction(
                     WidgetWheelEvent* aEvent,
                     const nsIntSize& aScrollAmountInDevPixels)
{
  MOZ_ASSERT(aEvent);

  
  
  bool allowScrollSpeedOverride =
    (!aEvent->customizedByUserPrefs &&
     aEvent->deltaMode == nsIDOMWheelEvent::DOM_DELTA_LINE);
  DeltaValues acceleratedDelta =
    WheelTransaction::AccelerateWheelDelta(aEvent, allowScrollSpeedOverride);

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






EventStateManager::WheelPrefs*
EventStateManager::WheelPrefs::GetInstance()
{
  if (!sInstance) {
    sInstance = new WheelPrefs();
  }
  return sInstance;
}


void
EventStateManager::WheelPrefs::Shutdown()
{
  delete sInstance;
  sInstance = nullptr;
}


void
EventStateManager::WheelPrefs::OnPrefChanged(const char* aPrefName,
                                             void* aClosure)
{
  
  sInstance->Reset();
  DeltaAccumulator::GetInstance()->Reset();
}

EventStateManager::WheelPrefs::WheelPrefs()
{
  Reset();
  Preferences::RegisterCallback(OnPrefChanged, "mousewheel.", nullptr);
}

EventStateManager::WheelPrefs::~WheelPrefs()
{
  Preferences::UnregisterCallback(OnPrefChanged, "mousewheel.", nullptr);
}

void
EventStateManager::WheelPrefs::Reset()
{
  memset(mInit, 0, sizeof(mInit));

}

EventStateManager::WheelPrefs::Index
EventStateManager::WheelPrefs::GetIndexFor(WidgetWheelEvent* aEvent)
{
  if (!aEvent) {
    return INDEX_DEFAULT;
  }

  Modifiers modifiers =
    (aEvent->modifiers & (MODIFIER_ALT |
                          MODIFIER_CONTROL |
                          MODIFIER_META |
                          MODIFIER_SHIFT |
                          MODIFIER_OS));

  switch (modifiers) {
    case MODIFIER_ALT:
      return INDEX_ALT;
    case MODIFIER_CONTROL:
      return INDEX_CONTROL;
    case MODIFIER_META:
      return INDEX_META;
    case MODIFIER_SHIFT:
      return INDEX_SHIFT;
    case MODIFIER_OS:
      return INDEX_OS;
    default:
      
      
      return INDEX_DEFAULT;
  }
}

void
EventStateManager::WheelPrefs::GetBasePrefName(
                     EventStateManager::WheelPrefs::Index aIndex,
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
EventStateManager::WheelPrefs::Init(EventStateManager::WheelPrefs::Index aIndex)
{
  if (mInit[aIndex]) {
    return;
  }
  mInit[aIndex] = true;

  nsAutoCString basePrefName;
  GetBasePrefName(aIndex, basePrefName);

  nsAutoCString prefNameX(basePrefName);
  prefNameX.AppendLiteral("delta_multiplier_x");
  mMultiplierX[aIndex] =
    static_cast<double>(Preferences::GetInt(prefNameX.get(), 100)) / 100;

  nsAutoCString prefNameY(basePrefName);
  prefNameY.AppendLiteral("delta_multiplier_y");
  mMultiplierY[aIndex] =
    static_cast<double>(Preferences::GetInt(prefNameY.get(), 100)) / 100;

  nsAutoCString prefNameZ(basePrefName);
  prefNameZ.AppendLiteral("delta_multiplier_z");
  mMultiplierZ[aIndex] =
    static_cast<double>(Preferences::GetInt(prefNameZ.get(), 100)) / 100;

  nsAutoCString prefNameAction(basePrefName);
  prefNameAction.AppendLiteral("action");
  int32_t action = Preferences::GetInt(prefNameAction.get(), ACTION_SCROLL);
  if (action < int32_t(ACTION_NONE) || action > int32_t(ACTION_LAST)) {
    NS_WARNING("Unsupported action pref value, replaced with 'Scroll'.");
    action = ACTION_SCROLL;
  }
  mActions[aIndex] = static_cast<Action>(action);

  
  
  
  prefNameAction.AppendLiteral(".override_x");
  int32_t actionOverrideX = Preferences::GetInt(prefNameAction.get(), -1);
  if (actionOverrideX < -1 || actionOverrideX > int32_t(ACTION_LAST)) {
    NS_WARNING("Unsupported action override pref value, didn't override.");
    actionOverrideX = -1;
  }
  mOverriddenActionsX[aIndex] = (actionOverrideX == -1)
                              ? static_cast<Action>(action)
                              : static_cast<Action>(actionOverrideX);
}

void
EventStateManager::WheelPrefs::ApplyUserPrefsToDelta(WidgetWheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  aEvent->deltaX *= mMultiplierX[index];
  aEvent->deltaY *= mMultiplierY[index];
  aEvent->deltaZ *= mMultiplierZ[index];

  
  
  
  if (!NeedToComputeLineOrPageDelta(aEvent)) {
    aEvent->lineOrPageDeltaX *= static_cast<int32_t>(mMultiplierX[index]);
    aEvent->lineOrPageDeltaY *= static_cast<int32_t>(mMultiplierY[index]);
  } else {
    aEvent->lineOrPageDeltaX = 0;
    aEvent->lineOrPageDeltaY = 0;
  }

  aEvent->customizedByUserPrefs =
    ((mMultiplierX[index] != 1.0) || (mMultiplierY[index] != 1.0) ||
     (mMultiplierZ[index] != 1.0));
}

void
EventStateManager::WheelPrefs::CancelApplyingUserPrefsFromOverflowDelta(
                                 WidgetWheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  
  
  
  
  

  if (mMultiplierX[index]) {
    aEvent->overflowDeltaX /= mMultiplierX[index];
  }
  if (mMultiplierY[index]) {
    aEvent->overflowDeltaY /= mMultiplierY[index];
  }
}

EventStateManager::WheelPrefs::Action
EventStateManager::WheelPrefs::ComputeActionFor(WidgetWheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  bool deltaXPreferred =
    (Abs(aEvent->deltaX) > Abs(aEvent->deltaY) &&
     Abs(aEvent->deltaX) > Abs(aEvent->deltaZ));
  Action* actions = deltaXPreferred ? mOverriddenActionsX : mActions;
  if (actions[index] == ACTION_NONE || actions[index] == ACTION_SCROLL) {
    return actions[index];
  }

  
  if (aEvent->isMomentum) {
    
    Init(INDEX_DEFAULT);
    return (actions[INDEX_DEFAULT] == ACTION_SCROLL) ? ACTION_SCROLL :
                                                       ACTION_NONE;
  }

  return actions[index];
}

bool
EventStateManager::WheelPrefs::NeedToComputeLineOrPageDelta(
                                 WidgetWheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  return (mMultiplierX[index] != 1.0 && mMultiplierX[index] != -1.0) ||
         (mMultiplierY[index] != 1.0 && mMultiplierY[index] != -1.0);
}

bool
EventStateManager::WheelPrefs::HasUserPrefsForDelta(WidgetWheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);

  return mMultiplierX[index] != 1.0 ||
         mMultiplierY[index] != 1.0;
}

bool
EventStateManager::WheelEventIsScrollAction(WidgetWheelEvent* aEvent)
{
  return WheelPrefs::GetInstance()->ComputeActionFor(aEvent) == WheelPrefs::ACTION_SCROLL;
}

bool
EventStateManager::WheelEventNeedsDeltaMultipliers(WidgetWheelEvent* aEvent)
{
  return WheelPrefs::GetInstance()->HasUserPrefsForDelta(aEvent);
}

bool
EventStateManager::WheelPrefs::IsOverOnePageScrollAllowedX(
                                 WidgetWheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);
  return Abs(mMultiplierX[index]) >=
           MIN_MULTIPLIER_VALUE_ALLOWING_OVER_ONE_PAGE_SCROLL;
}

bool
EventStateManager::WheelPrefs::IsOverOnePageScrollAllowedY(
                                 WidgetWheelEvent* aEvent)
{
  Index index = GetIndexFor(aEvent);
  Init(index);
  return Abs(mMultiplierY[index]) >=
           MIN_MULTIPLIER_VALUE_ALLOWING_OVER_ONE_PAGE_SCROLL;
}





bool EventStateManager::Prefs::sKeyCausesActivation = true;
bool EventStateManager::Prefs::sClickHoldContextMenu = false;
int32_t EventStateManager::Prefs::sGenericAccessModifierKey = -1;
int32_t EventStateManager::Prefs::sChromeAccessModifierMask = 0;
int32_t EventStateManager::Prefs::sContentAccessModifierMask = 0;


void
EventStateManager::Prefs::Init()
{
  DebugOnly<nsresult> rv = Preferences::RegisterCallback(OnChange, "dom.popup_allowed_events");
  MOZ_ASSERT(NS_SUCCEEDED(rv),
             "Failed to observe \"dom.popup_allowed_events\"");

  static bool sPrefsAlreadyCached = false;
  if (sPrefsAlreadyCached) {
    return;
  }

  rv = Preferences::AddBoolVarCache(&sKeyCausesActivation,
                                    "accessibility.accesskeycausesactivation",
                                    sKeyCausesActivation);
  MOZ_ASSERT(NS_SUCCEEDED(rv),
             "Failed to observe \"accessibility.accesskeycausesactivation\"");
  rv = Preferences::AddBoolVarCache(&sClickHoldContextMenu,
                                    "ui.click_hold_context_menus",
                                    sClickHoldContextMenu);
  MOZ_ASSERT(NS_SUCCEEDED(rv),
             "Failed to observe \"ui.click_hold_context_menus\"");
  rv = Preferences::AddIntVarCache(&sGenericAccessModifierKey,
                                   "ui.key.generalAccessKey",
                                   sGenericAccessModifierKey);
  MOZ_ASSERT(NS_SUCCEEDED(rv),
             "Failed to observe \"ui.key.generalAccessKey\"");
  rv = Preferences::AddIntVarCache(&sChromeAccessModifierMask,
                                   "ui.key.chromeAccess",
                                   sChromeAccessModifierMask);
  MOZ_ASSERT(NS_SUCCEEDED(rv),
             "Failed to observe \"ui.key.chromeAccess\"");
  rv = Preferences::AddIntVarCache(&sContentAccessModifierMask,
                                   "ui.key.contentAccess",
                                   sContentAccessModifierMask);
  MOZ_ASSERT(NS_SUCCEEDED(rv),
             "Failed to observe \"ui.key.contentAccess\"");
  sPrefsAlreadyCached = true;
}


void
EventStateManager::Prefs::OnChange(const char* aPrefName, void*)
{
  nsDependentCString prefName(aPrefName);
  if (prefName.EqualsLiteral("dom.popup_allowed_events")) {
    Event::PopupAllowedEventsChanged();
  }
}


void
EventStateManager::Prefs::Shutdown()
{
  Preferences::UnregisterCallback(OnChange, "dom.popup_allowed_events");
}


int32_t
EventStateManager::Prefs::ChromeAccessModifierMask()
{
  return GetAccessModifierMask(nsIDocShellTreeItem::typeChrome);
}


int32_t
EventStateManager::Prefs::ContentAccessModifierMask()
{
  return GetAccessModifierMask(nsIDocShellTreeItem::typeContent);
}


int32_t
EventStateManager::Prefs::GetAccessModifierMask(int32_t aItemType)
{
  switch (sGenericAccessModifierKey) {
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
      return sChromeAccessModifierMask;
    case nsIDocShellTreeItem::typeContent:
      return sContentAccessModifierMask;
    default:
      return 0;
  }
}





AutoHandlingUserInputStatePusher::AutoHandlingUserInputStatePusher(
                                    bool aIsHandlingUserInput,
                                    WidgetEvent* aEvent,
                                    nsIDocument* aDocument) :
  mIsHandlingUserInput(aIsHandlingUserInput),
  mIsMouseDown(aEvent && aEvent->message == NS_MOUSE_BUTTON_DOWN),
  mResetFMMouseButtonHandlingState(false)
{
  if (!aIsHandlingUserInput) {
    return;
  }
  EventStateManager::StartHandlingUserInput();
  if (mIsMouseDown) {
    nsIPresShell::SetCapturingContent(nullptr, 0);
    nsIPresShell::AllowMouseCapture(true);
  }
  if (!aDocument || !aEvent || !aEvent->mFlags.mIsTrusted) {
    return;
  }
  mResetFMMouseButtonHandlingState = (aEvent->message == NS_MOUSE_BUTTON_DOWN ||
                                      aEvent->message == NS_MOUSE_BUTTON_UP);
  if (mResetFMMouseButtonHandlingState) {
    nsFocusManager* fm = nsFocusManager::GetFocusManager();
    NS_ENSURE_TRUE_VOID(fm);
    
    
    
    
    mMouseButtonEventHandlingDocument =
      fm->SetMouseButtonHandlingDocument(aDocument);
  }
}

AutoHandlingUserInputStatePusher::~AutoHandlingUserInputStatePusher()
{
  if (!mIsHandlingUserInput) {
    return;
  }
  EventStateManager::StopHandlingUserInput();
  if (mIsMouseDown) {
    nsIPresShell::AllowMouseCapture(false);
  }
  if (mResetFMMouseButtonHandlingState) {
    nsFocusManager* fm = nsFocusManager::GetFocusManager();
    NS_ENSURE_TRUE_VOID(fm);
    nsCOMPtr<nsIDocument> handlingDocument =
      fm->SetMouseButtonHandlingDocument(mMouseButtonEventHandlingDocument);
  }
}

} 

