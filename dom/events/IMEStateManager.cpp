





#include "prlog.h"

#include "mozilla/IMEStateManager.h"

#include "mozilla/Attributes.h"
#include "mozilla/EventStates.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/TextComposition.h"
#include "mozilla/TextEvents.h"
#include "mozilla/dom/HTMLFormElement.h"
#include "mozilla/dom/TabParent.h"

#include "HTMLInputElement.h"
#include "IMEContentObserver.h"

#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMMouseEvent.h"
#include "nsIEditor.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsINode.h"
#include "nsIObserverService.h"
#include "nsIPresShell.h"
#include "nsISelection.h"
#include "nsISupports.h"
#include "nsPresContext.h"

namespace mozilla {

using namespace dom;
using namespace widget;

#ifdef PR_LOGGING














PRLogModuleInfo* sISMLog = nullptr;

static const char*
GetBoolName(bool aBool)
{
  return aBool ? "true" : "false";
}

static const char*
GetActionCauseName(InputContextAction::Cause aCause)
{
  switch (aCause) {
    case InputContextAction::CAUSE_UNKNOWN:
      return "CAUSE_UNKNOWN";
    case InputContextAction::CAUSE_UNKNOWN_CHROME:
      return "CAUSE_UNKNOWN_CHROME";
    case InputContextAction::CAUSE_KEY:
      return "CAUSE_KEY";
    case InputContextAction::CAUSE_MOUSE:
      return "CAUSE_MOUSE";
    default:
      return "illegal value";
  }
}

static const char*
GetActionFocusChangeName(InputContextAction::FocusChange aFocusChange)
{
  switch (aFocusChange) {
    case InputContextAction::FOCUS_NOT_CHANGED:
      return "FOCUS_NOT_CHANGED";
    case InputContextAction::GOT_FOCUS:
      return "GOT_FOCUS";
    case InputContextAction::LOST_FOCUS:
      return "LOST_FOCUS";
    case InputContextAction::MENU_GOT_PSEUDO_FOCUS:
      return "MENU_GOT_PSEUDO_FOCUS";
    case InputContextAction::MENU_LOST_PSEUDO_FOCUS:
      return "MENU_LOST_PSEUDO_FOCUS";
    default:
      return "illegal value";
  }
}

static const char*
GetIMEStateEnabledName(IMEState::Enabled aEnabled)
{
  switch (aEnabled) {
    case IMEState::DISABLED:
      return "DISABLED";
    case IMEState::ENABLED:
      return "ENABLED";
    case IMEState::PASSWORD:
      return "PASSWORD";
    case IMEState::PLUGIN:
      return "PLUGIN";
    default:
      return "illegal value";
  }
}

static const char*
GetIMEStateSetOpenName(IMEState::Open aOpen)
{
  switch (aOpen) {
    case IMEState::DONT_CHANGE_OPEN_STATE:
      return "DONT_CHANGE_OPEN_STATE";
    case IMEState::OPEN:
      return "OPEN";
    case IMEState::CLOSED:
      return "CLOSED";
    default:
      return "illegal value";
  }
}

static const char*
GetEventMessageName(uint32_t aMessage)
{
  switch (aMessage) {
    case NS_COMPOSITION_START:
      return "NS_COMPOSITION_START";
    case NS_COMPOSITION_END:
      return "NS_COMPOSITION_END";
    case NS_COMPOSITION_UPDATE:
      return "NS_COMPOSITION_UPDATE";
    case NS_COMPOSITION_CHANGE:
      return "NS_COMPOSITION_CHANGE";
    case NS_COMPOSITION_COMMIT_AS_IS:
      return "NS_COMPOSITION_COMMIT_AS_IS";
    case NS_COMPOSITION_COMMIT:
      return "NS_COMPOSITION_COMMIT";
    default:
      return "unacceptable event message";
  }
}

static const char*
GetNotifyIMEMessageName(IMEMessage aMessage)
{
  switch (aMessage) {
    case NOTIFY_IME_OF_FOCUS:
      return "NOTIFY_IME_OF_FOCUS";
    case NOTIFY_IME_OF_BLUR:
      return "NOTIFY_IME_OF_BLUR";
    case NOTIFY_IME_OF_SELECTION_CHANGE:
      return "NOTIFY_IME_OF_SELECTION_CHANGE";
    case NOTIFY_IME_OF_TEXT_CHANGE:
      return "NOTIFY_IME_OF_TEXT_CHANGE";
    case NOTIFY_IME_OF_COMPOSITION_UPDATE:
      return "NOTIFY_IME_OF_COMPOSITION_UPDATE";
    case NOTIFY_IME_OF_POSITION_CHANGE:
      return "NOTIFY_IME_OF_POSITION_CHANGE";
    case REQUEST_TO_COMMIT_COMPOSITION:
      return "REQUEST_TO_COMMIT_COMPOSITION";
    case REQUEST_TO_CANCEL_COMPOSITION:
      return "REQUEST_TO_CANCEL_COMPOSITION";
    default:
      return "unacceptable IME notification message";
  }
}
#endif 

nsIContent* IMEStateManager::sContent = nullptr;
nsPresContext* IMEStateManager::sPresContext = nullptr;
bool IMEStateManager::sInstalledMenuKeyboardListener = false;
bool IMEStateManager::sIsTestingIME = false;
bool IMEStateManager::sIsGettingNewIMEState = false;



IMEContentObserver* IMEStateManager::sActiveIMEContentObserver = nullptr;
TextCompositionArray* IMEStateManager::sTextCompositions = nullptr;


void
IMEStateManager::Init()
{
#ifdef PR_LOGGING
  if (!sISMLog) {
    sISMLog = PR_NewLogModule("IMEStateManager");
  }
#endif
}


void
IMEStateManager::Shutdown()
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::Shutdown(), "
     "sTextCompositions=0x%p, sTextCompositions->Length()=%u",
     sTextCompositions, sTextCompositions ? sTextCompositions->Length() : 0));

  MOZ_ASSERT(!sTextCompositions || !sTextCompositions->Length());
  delete sTextCompositions;
  sTextCompositions = nullptr;
}


nsresult
IMEStateManager::OnDestroyPresContext(nsPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  
  if (sTextCompositions) {
    TextCompositionArray::index_type i =
      sTextCompositions->IndexOf(aPresContext);
    if (i != TextCompositionArray::NoIndex) {
      PR_LOG(sISMLog, PR_LOG_DEBUG,
        ("ISM:   IMEStateManager::OnDestroyPresContext(), "
         "removing TextComposition instance from the array (index=%u)", i));
      
      sTextCompositions->ElementAt(i)->Destroy();
      sTextCompositions->RemoveElementAt(i);
#if defined DEBUG || PR_LOGGING
      if (sTextCompositions->IndexOf(aPresContext) !=
            TextCompositionArray::NoIndex) {
        PR_LOG(sISMLog, PR_LOG_ERROR,
          ("ISM:   IMEStateManager::OnDestroyPresContext(), FAILED to remove "
           "TextComposition instance from the array"));
        MOZ_CRASH("Failed to remove TextComposition instance from the array");
      }
#endif 
    }
  }

  if (aPresContext != sPresContext) {
    return NS_OK;
  }

  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnDestroyPresContext(aPresContext=0x%p), "
     "sPresContext=0x%p, sContent=0x%p, sTextCompositions=0x%p",
     aPresContext, sPresContext, sContent, sTextCompositions));

  DestroyIMEContentObserver();

  nsCOMPtr<nsIWidget> widget = sPresContext->GetRootWidget();
  if (widget) {
    IMEState newState = GetNewIMEState(sPresContext, nullptr);
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::LOST_FOCUS);
    SetIMEState(newState, nullptr, widget, action);
  }
  NS_IF_RELEASE(sContent);
  sPresContext = nullptr;
  return NS_OK;
}


nsresult
IMEStateManager::OnRemoveContent(nsPresContext* aPresContext,
                                 nsIContent* aContent)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  
  if (sTextCompositions) {
    nsRefPtr<TextComposition> compositionInContent =
      sTextCompositions->GetCompositionInContent(aPresContext, aContent);

    if (compositionInContent) {
      PR_LOG(sISMLog, PR_LOG_DEBUG,
        ("ISM:   IMEStateManager::OnRemoveContent(), "
         "composition is in the content"));

      
      
      
      
      nsCOMPtr<nsIWidget> widget = aPresContext->GetRootWidget();
      MOZ_ASSERT(widget, "Why is there no widget?");
      nsresult rv =
        compositionInContent->NotifyIME(REQUEST_TO_CANCEL_COMPOSITION);
      if (NS_FAILED(rv)) {
        compositionInContent->NotifyIME(REQUEST_TO_COMMIT_COMPOSITION);
      }
    }
  }

  if (!sPresContext || !sContent ||
      !nsContentUtils::ContentIsDescendantOf(sContent, aContent)) {
    return NS_OK;
  }

  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnRemoveContent(aPresContext=0x%p, "
     "aContent=0x%p), sPresContext=0x%p, sContent=0x%p, sTextCompositions=0x%p",
     aPresContext, aContent, sPresContext, sContent, sTextCompositions));

  DestroyIMEContentObserver();

  
  nsCOMPtr<nsIWidget> widget = sPresContext->GetRootWidget();
  if (widget) {
    IMEState newState = GetNewIMEState(sPresContext, nullptr);
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::LOST_FOCUS);
    SetIMEState(newState, nullptr, widget, action);
  }

  NS_IF_RELEASE(sContent);
  sPresContext = nullptr;

  return NS_OK;
}


nsresult
IMEStateManager::OnChangeFocus(nsPresContext* aPresContext,
                               nsIContent* aContent,
                               InputContextAction::Cause aCause)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnChangeFocus(aPresContext=0x%p, "
     "aContent=0x%p, aCause=%s)",
     aPresContext, aContent, GetActionCauseName(aCause)));

  InputContextAction action(aCause);
  return OnChangeFocusInternal(aPresContext, aContent, action);
}


nsresult
IMEStateManager::OnChangeFocusInternal(nsPresContext* aPresContext,
                                       nsIContent* aContent,
                                       InputContextAction aAction)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnChangeFocusInternal(aPresContext=0x%p, "
     "aContent=0x%p, aAction={ mCause=%s, mFocusChange=%s }), "
     "sPresContext=0x%p, sContent=0x%p, sActiveIMEContentObserver=0x%p",
     aPresContext, aContent, GetActionCauseName(aAction.mCause),
     GetActionFocusChangeName(aAction.mFocusChange),
     sPresContext, sContent, sActiveIMEContentObserver));

  bool focusActuallyChanging =
    (sContent != aContent || sPresContext != aPresContext);

  nsCOMPtr<nsIWidget> oldWidget =
    sPresContext ? sPresContext->GetRootWidget() : nullptr;
  if (oldWidget && focusActuallyChanging) {
    
    
    if (aPresContext) {
      NotifyIME(REQUEST_TO_COMMIT_COMPOSITION, oldWidget);
    }
  }

  if (sActiveIMEContentObserver &&
      (aPresContext || !sActiveIMEContentObserver->KeepAliveDuringDeactive()) &&
      !sActiveIMEContentObserver->IsManaging(aPresContext, aContent)) {
    DestroyIMEContentObserver();
  }

  if (!aPresContext) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnChangeFocusInternal(), "
       "no nsPresContext is being activated"));
    return NS_OK;
  }

  nsCOMPtr<nsIWidget> widget =
    (sPresContext == aPresContext) ? oldWidget.get() :
                                     aPresContext->GetRootWidget();
  if (NS_WARN_IF(!widget)) {
    PR_LOG(sISMLog, PR_LOG_ERROR,
      ("ISM:   IMEStateManager::OnChangeFocusInternal(), FAILED due to "
       "no widget to manage its IME state"));
    return NS_OK;
  }

  IMEState newState = GetNewIMEState(aPresContext, aContent);

  
  
  
  
  
  
  
  
  
  if ((newState.mEnabled == IMEState::DISABLED) && TabParent::GetIMETabParent()) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnChangeFocusInternal(), "
       "Parent process cancels to set DISABLED state because the content process "
       "has IME focus and has already sets IME state"));  
    MOZ_ASSERT(XRE_IsParentProcess(),
      "TabParent::GetIMETabParent() should never return non-null value "
      "in the content process");
    return NS_OK;
  }

  if (!focusActuallyChanging) {
    
    
    InputContext context = widget->GetInputContext();
    if (context.mIMEState.mEnabled == newState.mEnabled) {
      PR_LOG(sISMLog, PR_LOG_DEBUG,
        ("ISM:   IMEStateManager::OnChangeFocusInternal(), "
         "neither focus nor IME state is changing"));
      return NS_OK;
    }
    aAction.mFocusChange = InputContextAction::FOCUS_NOT_CHANGED;

    
    
    if (sPresContext && oldWidget && !focusActuallyChanging) {
      NotifyIME(REQUEST_TO_COMMIT_COMPOSITION, oldWidget);
    }
  } else if (aAction.mFocusChange == InputContextAction::FOCUS_NOT_CHANGED) {
    
    
    bool gotFocus = aContent || (newState.mEnabled == IMEState::ENABLED);
    aAction.mFocusChange =
      gotFocus ? InputContextAction::GOT_FOCUS : InputContextAction::LOST_FOCUS;
  }

  
  SetIMEState(newState, aContent, widget, aAction);

  sPresContext = aPresContext;
  if (sContent != aContent) {
    NS_IF_RELEASE(sContent);
    NS_IF_ADDREF(sContent = aContent);
  }

  
  

  return NS_OK;
}


void
IMEStateManager::OnInstalledMenuKeyboardListener(bool aInstalling)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnInstalledMenuKeyboardListener(aInstalling=%s), "
     "sInstalledMenuKeyboardListener=%s",
     GetBoolName(aInstalling), GetBoolName(sInstalledMenuKeyboardListener)));

  sInstalledMenuKeyboardListener = aInstalling;

  InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
    aInstalling ? InputContextAction::MENU_GOT_PSEUDO_FOCUS :
                  InputContextAction::MENU_LOST_PSEUDO_FOCUS);
  OnChangeFocusInternal(sPresContext, sContent, action);
}


bool
IMEStateManager::OnMouseButtonEventInEditor(nsPresContext* aPresContext,
                                            nsIContent* aContent,
                                            nsIDOMMouseEvent* aMouseEvent)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnMouseButtonEventInEditor(aPresContext=0x%p, "
     "aContent=0x%p, aMouseEvent=0x%p), sPresContext=0x%p, sContent=0x%p",
     aPresContext, aContent, aMouseEvent, sPresContext, sContent));

  if (sPresContext != aPresContext || sContent != aContent) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnMouseButtonEventInEditor(), "
       "the mouse event isn't fired on the editor managed by ISM"));
    return false;
  }

  if (!sActiveIMEContentObserver) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnMouseButtonEventInEditor(), "
       "there is no active IMEContentObserver"));
    return false;
  }

  if (!sActiveIMEContentObserver->IsManaging(aPresContext, aContent)) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnMouseButtonEventInEditor(), "
       "the active IMEContentObserver isn't managing the editor"));
    return false;
  }

  WidgetMouseEvent* internalEvent =
    aMouseEvent->GetInternalNSEvent()->AsMouseEvent();
  if (NS_WARN_IF(!internalEvent)) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnMouseButtonEventInEditor(), "
       "the internal event of aMouseEvent isn't WidgetMouseEvent"));
    return false;
  }

  bool consumed =
    sActiveIMEContentObserver->OnMouseButtonEvent(aPresContext, internalEvent);

#ifdef PR_LOGGING
  nsAutoString eventType;
  aMouseEvent->GetType(eventType);
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM:   IMEStateManager::OnMouseButtonEventInEditor(), "
     "mouse event (type=%s, button=%d) is %s",
     NS_ConvertUTF16toUTF8(eventType).get(), internalEvent->button,
     consumed ? "consumed" : "not consumed"));
#endif

  return consumed;
}


void
IMEStateManager::OnClickInEditor(nsPresContext* aPresContext,
                                 nsIContent* aContent,
                                 nsIDOMMouseEvent* aMouseEvent)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnClickInEditor(aPresContext=0x%p, aContent=0x%p, "
     "aMouseEvent=0x%p), sPresContext=0x%p, sContent=0x%p",
     aPresContext, aContent, aMouseEvent, sPresContext, sContent));

  if (sPresContext != aPresContext || sContent != aContent) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnClickInEditor(), "
       "the mouse event isn't fired on the editor managed by ISM"));
    return;
  }

  nsCOMPtr<nsIWidget> widget = aPresContext->GetRootWidget();
  NS_ENSURE_TRUE_VOID(widget);

  bool isTrusted;
  nsresult rv = aMouseEvent->GetIsTrusted(&isTrusted);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (!isTrusted) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnClickInEditor(), "
       "the mouse event isn't a trusted event"));
    return; 
  }

  int16_t button;
  rv = aMouseEvent->GetButton(&button);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (button != 0) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnClickInEditor(), "
       "the mouse event isn't a left mouse button event"));
    return; 
  }

  int32_t clickCount;
  rv = aMouseEvent->GetDetail(&clickCount);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (clickCount != 1) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnClickInEditor(), "
       "the mouse event isn't a single click event"));
    return; 
  }

  InputContextAction action(InputContextAction::CAUSE_MOUSE,
                            InputContextAction::FOCUS_NOT_CHANGED);
  IMEState newState = GetNewIMEState(aPresContext, aContent);
  SetIMEState(newState, aContent, widget, action);
}


void
IMEStateManager::OnFocusInEditor(nsPresContext* aPresContext,
                                 nsIContent* aContent,
                                 nsIEditor* aEditor)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnFocusInEditor(aPresContext=0x%p, aContent=0x%p, "
     "aEditor=0x%p), sPresContext=0x%p, sContent=0x%p, "
     "sActiveIMEContentObserver=0x%p",
     aPresContext, aContent, aEditor, sPresContext, sContent,
     sActiveIMEContentObserver));

  if (sPresContext != aPresContext || sContent != aContent) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::OnFocusInEditor(), "
       "an editor not managed by ISM gets focus"));
    return;
  }

  
  
  if (sActiveIMEContentObserver) {
    if (sActiveIMEContentObserver->IsManaging(aPresContext, aContent)) {
      PR_LOG(sISMLog, PR_LOG_DEBUG,
        ("ISM:   IMEStateManager::OnFocusInEditor(), "
         "the editor is already being managed by sActiveIMEContentObserver"));
      return;
    }
    DestroyIMEContentObserver();
  }

  CreateIMEContentObserver(aEditor);
}


void
IMEStateManager::UpdateIMEState(const IMEState& aNewIMEState,
                                nsIContent* aContent,
                                nsIEditor* aEditor)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::UpdateIMEState(aNewIMEState={ mEnabled=%s, "
     "mOpen=%s }, aContent=0x%p, aEditor=0x%p), "
     "sPresContext=0x%p, sContent=0x%p, sActiveIMEContentObserver=0x%p, "
     "sIsGettingNewIMEState=%s",
     GetIMEStateEnabledName(aNewIMEState.mEnabled),
     GetIMEStateSetOpenName(aNewIMEState.mOpen), aContent, aEditor,
     sPresContext, sContent, sActiveIMEContentObserver,
     GetBoolName(sIsGettingNewIMEState)));

  if (sIsGettingNewIMEState) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::UpdateIMEState(), "
       "does nothing because of called while getting new IME state"));
    return;
  }

  if (NS_WARN_IF(!sPresContext)) {
    PR_LOG(sISMLog, PR_LOG_ERROR,
      ("ISM:   IMEStateManager::UpdateIMEState(), FAILED due to "
       "no managing nsPresContext"));
    return;
  }
  nsCOMPtr<nsIWidget> widget = sPresContext->GetRootWidget();
  if (NS_WARN_IF(!widget)) {
    PR_LOG(sISMLog, PR_LOG_ERROR,
      ("ISM:   IMEStateManager::UpdateIMEState(), FAILED due to "
       "no widget for the managing nsPresContext"));
    return;
  }

  
  
  
  bool createTextStateManager =
    (!sActiveIMEContentObserver ||
     !sActiveIMEContentObserver->IsManaging(sPresContext, aContent));

  bool updateIMEState =
    (widget->GetInputContext().mIMEState.mEnabled != aNewIMEState.mEnabled);

  if (updateIMEState) {
    
    NotifyIME(REQUEST_TO_COMMIT_COMPOSITION, widget);
  }

  if (createTextStateManager) {
    DestroyIMEContentObserver();
  }

  if (updateIMEState) {
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::FOCUS_NOT_CHANGED);
    SetIMEState(aNewIMEState, aContent, widget, action);
  }

  if (createTextStateManager) {
    CreateIMEContentObserver(aEditor);
  }
}


IMEState
IMEStateManager::GetNewIMEState(nsPresContext* aPresContext,
                                nsIContent*    aContent)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::GetNewIMEState(aPresContext=0x%p, aContent=0x%p), "
     "sInstalledMenuKeyboardListener=%s",
     aPresContext, aContent, GetBoolName(sInstalledMenuKeyboardListener)));

  
  if (aPresContext->Type() == nsPresContext::eContext_PrintPreview ||
      aPresContext->Type() == nsPresContext::eContext_Print) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::GetNewIMEState() returns DISABLED because "
       "the nsPresContext is for print or print preview"));
    return IMEState(IMEState::DISABLED);
  }

  if (sInstalledMenuKeyboardListener) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::GetNewIMEState() returns DISABLED because "
       "menu keyboard listener was installed"));
    return IMEState(IMEState::DISABLED);
  }

  if (!aContent) {
    
    
    nsIDocument* doc = aPresContext->Document();
    if (doc && doc->HasFlag(NODE_IS_EDITABLE)) {
      PR_LOG(sISMLog, PR_LOG_DEBUG,
        ("ISM:   IMEStateManager::GetNewIMEState() returns ENABLED because "
         "design mode editor has focus"));
      return IMEState(IMEState::ENABLED);
    }
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::GetNewIMEState() returns DISABLED because "
       "no content has focus"));
    return IMEState(IMEState::DISABLED);
  }

  
  
  
  
  
  GettingNewIMEStateBlocker blocker;

  IMEState newIMEState = aContent->GetDesiredIMEState();
  PR_LOG(sISMLog, PR_LOG_DEBUG,
    ("ISM:   IMEStateManager::GetNewIMEState() returns { mEnabled=%s, "
     "mOpen=%s }",
     GetIMEStateEnabledName(newIMEState.mEnabled),
     GetIMEStateSetOpenName(newIMEState.mOpen)));
  return newIMEState;
}


class IMEEnabledStateChangedEvent : public nsRunnable {
public:
  explicit IMEEnabledStateChangedEvent(uint32_t aState)
    : mState(aState)
  {
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIObserverService> observerService =
      services::GetObserverService();
    if (observerService) {
      PR_LOG(sISMLog, PR_LOG_ALWAYS,
        ("ISM: IMEEnabledStateChangedEvent::Run(), notifies observers of "
         "\"ime-enabled-state-changed\""));
      nsAutoString state;
      state.AppendInt(mState);
      observerService->NotifyObservers(nullptr, "ime-enabled-state-changed",
                                       state.get());
    }
    return NS_OK;
  }

private:
  uint32_t mState;
};


void
IMEStateManager::SetIMEState(const IMEState& aState,
                             nsIContent* aContent,
                             nsIWidget* aWidget,
                             InputContextAction aAction)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::SetIMEState(aState={ mEnabled=%s, mOpen=%s }, "
     "aContent=0x%p, aWidget=0x%p, aAction={ mCause=%s, mFocusChange=%s })",
     GetIMEStateEnabledName(aState.mEnabled),
     GetIMEStateSetOpenName(aState.mOpen), aContent, aWidget,
     GetActionCauseName(aAction.mCause),
     GetActionFocusChangeName(aAction.mFocusChange)));

  NS_ENSURE_TRUE_VOID(aWidget);

  InputContext oldContext = aWidget->GetInputContext();

  InputContext context;
  context.mIMEState = aState;

  if (aContent &&
      aContent->IsAnyOfHTMLElements(nsGkAtoms::input, nsGkAtoms::textarea)) {
    if (!aContent->IsHTMLElement(nsGkAtoms::textarea)) {
      
      
      
      
      nsIContent* content = aContent;
      HTMLInputElement* inputElement =
        HTMLInputElement::FromContentOrNull(aContent);
      if (inputElement) {
        HTMLInputElement* ownerNumberControl =
          inputElement->GetOwnerNumberControl();
        if (ownerNumberControl) {
          content = ownerNumberControl; 
        }
      }
      content->GetAttr(kNameSpaceID_None, nsGkAtoms::type,
                       context.mHTMLInputType);
    } else {
      context.mHTMLInputType.Assign(nsGkAtoms::textarea->GetUTF16String());
    }

    if (Preferences::GetBool("dom.forms.inputmode", false)) {
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::inputmode,
                        context.mHTMLInputInputmode);
    }

    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::moz_action_hint,
                      context.mActionHint);

    
    
    nsIContent* inputContent = aContent->FindFirstNonChromeOnlyAccessContent();

    
    
    if (context.mActionHint.IsEmpty() &&
        inputContent->IsHTMLElement(nsGkAtoms::input)) {
      bool willSubmit = false;
      nsCOMPtr<nsIFormControl> control(do_QueryInterface(inputContent));
      mozilla::dom::Element* formElement = control->GetFormElement();
      nsCOMPtr<nsIForm> form;
      if (control) {
        
        if ((form = do_QueryInterface(formElement)) &&
            form->GetDefaultSubmitElement()) {
          willSubmit = true;
        
        } else if (formElement && formElement->IsHTMLElement(nsGkAtoms::form) &&
                   !static_cast<dom::HTMLFormElement*>(formElement)->
                     ImplicitSubmissionIsDisabled()) {
          willSubmit = true;
        }
      }
      context.mActionHint.Assign(
        willSubmit ? (control->GetType() == NS_FORM_INPUT_SEARCH ?
                       NS_LITERAL_STRING("search") : NS_LITERAL_STRING("go")) :
                     (formElement ?
                       NS_LITERAL_STRING("next") : EmptyString()));
    }
  }

  
  
  if (aAction.mCause == InputContextAction::CAUSE_UNKNOWN &&
      XRE_GetProcessType() != GeckoProcessType_Content) {
    aAction.mCause = InputContextAction::CAUSE_UNKNOWN_CHROME;
  }


  PR_LOG(sISMLog, PR_LOG_DEBUG,
    ("ISM:   IMEStateManager::SetIMEState(), "
     "calling nsIWidget::SetInputContext(context={ mIMEState={ mEnabled=%s, "
     "mOpen=%s }, mHTMLInputType=\"%s\", mHTMLInputInputmode=\"%s\", "
     "mActionHint=\"%s\" }, aAction={ mCause=%s, mAction=%s })",
     GetIMEStateEnabledName(context.mIMEState.mEnabled),
     GetIMEStateSetOpenName(context.mIMEState.mOpen),
     NS_ConvertUTF16toUTF8(context.mHTMLInputType).get(),
     NS_ConvertUTF16toUTF8(context.mHTMLInputInputmode).get(),
     NS_ConvertUTF16toUTF8(context.mActionHint).get(),
     GetActionCauseName(aAction.mCause),
     GetActionFocusChangeName(aAction.mFocusChange)));

  aWidget->SetInputContext(context, aAction);
  if (oldContext.mIMEState.mEnabled == context.mIMEState.mEnabled) {
    return;
  }

  nsContentUtils::AddScriptRunner(
    new IMEEnabledStateChangedEvent(context.mIMEState.mEnabled));
}


void
IMEStateManager::EnsureTextCompositionArray()
{
  if (sTextCompositions) {
    return;
  }
  sTextCompositions = new TextCompositionArray();
}


void
IMEStateManager::DispatchCompositionEvent(
                   nsINode* aEventTargetNode,
                   nsPresContext* aPresContext,
                   WidgetCompositionEvent* aCompositionEvent,
                   nsEventStatus* aStatus,
                   EventDispatchingCallback* aCallBack,
                   bool aIsSynthesized)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::DispatchCompositionEvent(aNode=0x%p, "
     "aPresContext=0x%p, aCompositionEvent={ message=%s, "
     "mFlags={ mIsTrusted=%s, mPropagationStopped=%s } }, "
     "aIsSynthesized=%s)",
     aEventTargetNode, aPresContext,
     GetEventMessageName(aCompositionEvent->message),
     GetBoolName(aCompositionEvent->mFlags.mIsTrusted),
     GetBoolName(aCompositionEvent->mFlags.mPropagationStopped),
     GetBoolName(aIsSynthesized)));

  if (!aCompositionEvent->mFlags.mIsTrusted ||
      aCompositionEvent->mFlags.mPropagationStopped) {
    return;
  }

  MOZ_ASSERT(aCompositionEvent->message != NS_COMPOSITION_UPDATE,
             "compositionupdate event shouldn't be dispatched manually");

  EnsureTextCompositionArray();

  nsRefPtr<TextComposition> composition =
    sTextCompositions->GetCompositionFor(aCompositionEvent->widget);
  if (!composition) {
    
    
    if (NS_WARN_IF(aIsSynthesized)) {
      return;
    }
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::DispatchCompositionEvent(), "
       "adding new TextComposition to the array"));
    MOZ_ASSERT(aCompositionEvent->message == NS_COMPOSITION_START);
    composition =
      new TextComposition(aPresContext, aEventTargetNode, aCompositionEvent);
    sTextCompositions->AppendElement(composition);
  }
#ifdef DEBUG
  else {
    MOZ_ASSERT(aCompositionEvent->message != NS_COMPOSITION_START);
  }
#endif 

  
  composition->DispatchCompositionEvent(aCompositionEvent, aStatus, aCallBack,
                                        aIsSynthesized);

  

  
  
  
  
  
  
  
  
  
  
  if ((!aIsSynthesized ||
       composition->WasNativeCompositionEndEventDiscarded()) &&
      aCompositionEvent->CausesDOMCompositionEndEvent()) {
    TextCompositionArray::index_type i =
      sTextCompositions->IndexOf(aCompositionEvent->widget);
    if (i != TextCompositionArray::NoIndex) {
      PR_LOG(sISMLog, PR_LOG_DEBUG,
        ("ISM:   IMEStateManager::DispatchCompositionEvent(), "
         "removing TextComposition from the array since NS_COMPOSTION_END "
         "was dispatched"));
      sTextCompositions->ElementAt(i)->Destroy();
      sTextCompositions->RemoveElementAt(i);
    }
  }
}


void
IMEStateManager::OnCompositionEventDiscarded(
                   const WidgetCompositionEvent* aCompositionEvent)
{
  
  

  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::OnCompositionEventDiscarded(aCompositionEvent={ "
     "message=%s, mFlags={ mIsTrusted=%s } })",
     GetEventMessageName(aCompositionEvent->message),
     GetBoolName(aCompositionEvent->mFlags.mIsTrusted)));

  if (!aCompositionEvent->mFlags.mIsTrusted) {
    return;
  }

  
  
  if (aCompositionEvent->message == NS_COMPOSITION_START) {
    return;
  }

  nsRefPtr<TextComposition> composition =
    sTextCompositions->GetCompositionFor(aCompositionEvent->widget);
  if (!composition) {
    
    
    
    
    PR_LOG(sISMLog, PR_LOG_ALWAYS,
      ("ISM:   IMEStateManager::OnCompositionEventDiscarded(), "
       "TextComposition instance for the widget has already gone"));
    return;
  }
  composition->OnCompositionEventDiscarded(aCompositionEvent);
}


nsresult
IMEStateManager::NotifyIME(IMEMessage aMessage,
                           nsIWidget* aWidget)
{
  nsRefPtr<TextComposition> composition;
  if (aWidget && sTextCompositions) {
    composition = sTextCompositions->GetCompositionFor(aWidget);
  }

  bool isSynthesizedForTests =
    composition && composition->IsSynthesizedForTests();

  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::NotifyIME(aMessage=%s, aWidget=0x%p), "
     "composition=0x%p, composition->IsSynthesizedForTests()=%s",
     GetNotifyIMEMessageName(aMessage), aWidget, composition.get(),
     GetBoolName(isSynthesizedForTests)));

  if (NS_WARN_IF(!aWidget)) {
    PR_LOG(sISMLog, PR_LOG_ERROR,
      ("ISM:   IMEStateManager::NotifyIME(), FAILED due to no widget"));
    return NS_ERROR_INVALID_ARG;
  }

  switch (aMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION:
      return composition ?
        composition->RequestToCommit(aWidget, false) : NS_OK;
    case REQUEST_TO_CANCEL_COMPOSITION:
      return composition ?
        composition->RequestToCommit(aWidget, true) : NS_OK;
    case NOTIFY_IME_OF_COMPOSITION_UPDATE:
      return composition && !isSynthesizedForTests ?
        aWidget->NotifyIME(IMENotification(aMessage)) : NS_OK;
    default:
      MOZ_CRASH("Unsupported notification");
  }
  MOZ_CRASH(
    "Failed to handle the notification for non-synthesized composition");
  return NS_ERROR_FAILURE;
}


nsresult
IMEStateManager::NotifyIME(IMEMessage aMessage,
                           nsPresContext* aPresContext)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::NotifyIME(aMessage=%s, aPresContext=0x%p)",
     GetNotifyIMEMessageName(aMessage), aPresContext));

  NS_ENSURE_TRUE(aPresContext, NS_ERROR_INVALID_ARG);

  nsIWidget* widget = aPresContext->GetRootWidget();
  if (NS_WARN_IF(!widget)) {
    PR_LOG(sISMLog, PR_LOG_ERROR,
      ("ISM:   IMEStateManager::NotifyIME(), FAILED due to no widget for the "
       "nsPresContext"));
    return NS_ERROR_NOT_AVAILABLE;
  }
  return NotifyIME(aMessage, widget);
}


bool
IMEStateManager::IsEditable(nsINode* node)
{
  if (node->IsEditable()) {
    return true;
  }
  
  if (node->IsElement() &&
      node->AsElement()->State().HasState(NS_EVENT_STATE_MOZ_READWRITE)) {
    return true;
  }
  return false;
}


nsINode*
IMEStateManager::GetRootEditableNode(nsPresContext* aPresContext,
                                     nsIContent* aContent)
{
  if (aContent) {
    nsINode* root = nullptr;
    nsINode* node = aContent;
    while (node && IsEditable(node)) {
      
      
      
      
      
      
      if (node->IsContent() &&
          node->AsContent()->HasIndependentSelection()) {
        return node;
      }
      root = node;
      node = node->GetParentNode();
    }
    return root;
  }
  if (aPresContext) {
    nsIDocument* document = aPresContext->Document();
    if (document && document->IsEditable()) {
      return document;
    }
  }
  return nullptr;
}


bool
IMEStateManager::IsEditableIMEState(nsIWidget* aWidget)
{
  switch (aWidget->GetInputContext().mIMEState.mEnabled) {
    case IMEState::ENABLED:
    case IMEState::PASSWORD:
      return true;
    case IMEState::PLUGIN:
    case IMEState::DISABLED:
      return false;
    default:
      MOZ_CRASH("Unknown IME enable state");
  }
}


void
IMEStateManager::DestroyIMEContentObserver()
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::DestroyIMEContentObserver(), "
     "sActiveIMEContentObserver=0x%p",
     sActiveIMEContentObserver));

  if (!sActiveIMEContentObserver) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::DestroyIMEContentObserver() does nothing"));
    return;
  }

  PR_LOG(sISMLog, PR_LOG_DEBUG,
    ("ISM:   IMEStateManager::DestroyIMEContentObserver(), destroying "
     "the active IMEContentObserver..."));
  nsRefPtr<IMEContentObserver> tsm;
  tsm.swap(sActiveIMEContentObserver);
  tsm->Destroy();
}


void
IMEStateManager::CreateIMEContentObserver(nsIEditor* aEditor)
{
  PR_LOG(sISMLog, PR_LOG_ALWAYS,
    ("ISM: IMEStateManager::CreateIMEContentObserver(aEditor=0x%p), "
     "sPresContext=0x%p, sContent=0x%p, sActiveIMEContentObserver=0x%p, "
     "sActiveIMEContentObserver->IsManaging(sPresContext, sContent)=%s",
     aEditor, sPresContext, sContent, sActiveIMEContentObserver,
     GetBoolName(sActiveIMEContentObserver ?
       sActiveIMEContentObserver->IsManaging(sPresContext, sContent) : false)));

  if (NS_WARN_IF(sActiveIMEContentObserver)) {
    PR_LOG(sISMLog, PR_LOG_ERROR,
      ("ISM:   IMEStateManager::CreateIMEContentObserver(), FAILED due to "
       "there is already an active IMEContentObserver"));
    MOZ_ASSERT(sActiveIMEContentObserver->IsManaging(sPresContext, sContent));
    return;
  }

  nsCOMPtr<nsIWidget> widget = sPresContext->GetRootWidget();
  if (!widget) {
    PR_LOG(sISMLog, PR_LOG_ERROR,
      ("ISM:   IMEStateManager::CreateIMEContentObserver(), FAILED due to "
       "there is a root widget for the nsPresContext"));
    return; 
  }

  
  if (!IsEditableIMEState(widget)) {
    PR_LOG(sISMLog, PR_LOG_DEBUG,
      ("ISM:   IMEStateManager::CreateIMEContentObserver() doesn't create "
       "IMEContentObserver because of non-editable IME state"));
    return;
  }

  static bool sInitializeIsTestingIME = true;
  if (sInitializeIsTestingIME) {
    Preferences::AddBoolVarCache(&sIsTestingIME, "test.IME", false);
    sInitializeIsTestingIME = false;
  }

  PR_LOG(sISMLog, PR_LOG_DEBUG,
    ("ISM:   IMEStateManager::CreateIMEContentObserver() is creating an "
     "IMEContentObserver instance..."));
  sActiveIMEContentObserver = new IMEContentObserver();
  NS_ADDREF(sActiveIMEContentObserver);

  
  
  
  nsRefPtr<IMEContentObserver> kungFuDeathGrip(sActiveIMEContentObserver);
  sActiveIMEContentObserver->Init(widget, sPresContext, sContent, aEditor);
}


nsresult
IMEStateManager::GetFocusSelectionAndRoot(nsISelection** aSelection,
                                          nsIContent** aRootContent)
{
  if (!sActiveIMEContentObserver) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  return sActiveIMEContentObserver->GetSelectionAndRoot(aSelection,
                                                        aRootContent);
}


already_AddRefed<TextComposition>
IMEStateManager::GetTextCompositionFor(nsIWidget* aWidget)
{
  if (!sTextCompositions) {
    return nullptr;
  }
  nsRefPtr<TextComposition> textComposition =
    sTextCompositions->GetCompositionFor(aWidget);
  return textComposition.forget();
}


already_AddRefed<TextComposition>
IMEStateManager::GetTextCompositionFor(WidgetGUIEvent* aGUIEvent)
{
  MOZ_ASSERT(aGUIEvent->AsCompositionEvent() || aGUIEvent->AsKeyboardEvent(),
    "aGUIEvent has to be WidgetCompositionEvent or WidgetKeyboardEvent");
  return GetTextCompositionFor(aGUIEvent->widget);
}

} 
