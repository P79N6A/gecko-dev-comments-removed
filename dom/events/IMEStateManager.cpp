





#include "mozilla/IMEStateManager.h"

#include "mozilla/Attributes.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/TextEvents.h"
#include "mozilla/dom/HTMLFormElement.h"

#include "HTMLInputElement.h"
#include "IMEContentObserver.h"
#include "TextComposition.h"

#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMMouseEvent.h"
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

nsIContent* IMEStateManager::sContent = nullptr;
nsPresContext* IMEStateManager::sPresContext = nullptr;
bool IMEStateManager::sInstalledMenuKeyboardListener = false;
bool IMEStateManager::sIsTestingIME = false;



IMEContentObserver* IMEStateManager::sActiveIMEContentObserver = nullptr;
TextCompositionArray* IMEStateManager::sTextCompositions = nullptr;

void
IMEStateManager::Shutdown()
{
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
      
      sTextCompositions->ElementAt(i)->Destroy();
      sTextCompositions->RemoveElementAt(i);
      MOZ_ASSERT(sTextCompositions->IndexOf(aPresContext) ==
                   TextCompositionArray::NoIndex);
    }
  }

  if (aPresContext != sPresContext) {
    return NS_OK;
  }

  DestroyTextStateManager();

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
      
      
      
      
      nsCOMPtr<nsIWidget> widget = aPresContext->GetRootWidget();
      if (widget) {
        nsresult rv =
          compositionInContent->NotifyIME(REQUEST_TO_CANCEL_COMPOSITION);
        if (NS_FAILED(rv)) {
          compositionInContent->NotifyIME(REQUEST_TO_COMMIT_COMPOSITION);
        }
        
        compositionInContent =
          sTextCompositions->GetCompositionFor(
                               compositionInContent->GetPresContext(),
                               compositionInContent->GetEventTargetNode());
      }
    }

    
    
    if (compositionInContent) {
      compositionInContent->SynthesizeCommit(true);
    }
  }

  if (!sPresContext || !sContent ||
      !nsContentUtils::ContentIsDescendantOf(sContent, aContent)) {
    return NS_OK;
  }

  DestroyTextStateManager();

  
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
  InputContextAction action(aCause);
  return OnChangeFocusInternal(aPresContext, aContent, action);
}

nsresult
IMEStateManager::OnChangeFocusInternal(nsPresContext* aPresContext,
                                       nsIContent* aContent,
                                       InputContextAction aAction)
{
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
    DestroyTextStateManager();
  }

  if (!aPresContext) {
    return NS_OK;
  }

  nsCOMPtr<nsIWidget> widget =
    (sPresContext == aPresContext) ? oldWidget.get() :
                                     aPresContext->GetRootWidget();
  if (!widget) {
    return NS_OK;
  }

  IMEState newState = GetNewIMEState(aPresContext, aContent);
  if (!focusActuallyChanging) {
    
    
    InputContext context = widget->GetInputContext();
    if (context.mIMEState.mEnabled == newState.mEnabled) {
      
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
  sInstalledMenuKeyboardListener = aInstalling;

  InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
    aInstalling ? InputContextAction::MENU_GOT_PSEUDO_FOCUS :
                  InputContextAction::MENU_LOST_PSEUDO_FOCUS);
  OnChangeFocusInternal(sPresContext, sContent, action);
}

void
IMEStateManager::OnClickInEditor(nsPresContext* aPresContext,
                                 nsIContent* aContent,
                                 nsIDOMMouseEvent* aMouseEvent)
{
  if (sPresContext != aPresContext || sContent != aContent) {
    return;
  }

  nsCOMPtr<nsIWidget> widget = aPresContext->GetRootWidget();
  NS_ENSURE_TRUE_VOID(widget);

  bool isTrusted;
  nsresult rv = aMouseEvent->GetIsTrusted(&isTrusted);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (!isTrusted) {
    return; 
  }

  int16_t button;
  rv = aMouseEvent->GetButton(&button);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (button != 0) {
    return; 
  }

  int32_t clickCount;
  rv = aMouseEvent->GetDetail(&clickCount);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (clickCount != 1) {
    return; 
  }

  InputContextAction action(InputContextAction::CAUSE_MOUSE,
                            InputContextAction::FOCUS_NOT_CHANGED);
  IMEState newState = GetNewIMEState(aPresContext, aContent);
  SetIMEState(newState, aContent, widget, action);
}

void
IMEStateManager::OnFocusInEditor(nsPresContext* aPresContext,
                                 nsIContent* aContent)
{
  if (sPresContext != aPresContext || sContent != aContent) {
    return;
  }

  
  
  if (sActiveIMEContentObserver) {
    if (sActiveIMEContentObserver->IsManaging(aPresContext, aContent)) {
      return;
    }
    DestroyTextStateManager();
  }

  CreateIMEContentObserver();
}

void
IMEStateManager::UpdateIMEState(const IMEState& aNewIMEState,
                                nsIContent* aContent)
{
  if (!sPresContext) {
    NS_WARNING("ISM doesn't know which editor has focus");
    return;
  }
  nsCOMPtr<nsIWidget> widget = sPresContext->GetRootWidget();
  if (!widget) {
    NS_WARNING("focused widget is not found");
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
    DestroyTextStateManager();
  }

  if (updateIMEState) {
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::FOCUS_NOT_CHANGED);
    SetIMEState(aNewIMEState, aContent, widget, action);
  }

  if (createTextStateManager) {
    CreateIMEContentObserver();
  }
}

IMEState
IMEStateManager::GetNewIMEState(nsPresContext* aPresContext,
                                nsIContent*    aContent)
{
  
  if (aPresContext->Type() == nsPresContext::eContext_PrintPreview ||
      aPresContext->Type() == nsPresContext::eContext_Print) {
    return IMEState(IMEState::DISABLED);
  }

  if (sInstalledMenuKeyboardListener) {
    return IMEState(IMEState::DISABLED);
  }

  if (!aContent) {
    
    
    nsIDocument* doc = aPresContext->Document();
    if (doc && doc->HasFlag(NODE_IS_EDITABLE)) {
      return IMEState(IMEState::ENABLED);
    }
    return IMEState(IMEState::DISABLED);
  }

  return aContent->GetDesiredIMEState();
}


class IMEEnabledStateChangedEvent : public nsRunnable {
public:
  IMEEnabledStateChangedEvent(uint32_t aState)
    : mState(aState)
  {
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIObserverService> observerService =
      services::GetObserverService();
    if (observerService) {
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
  NS_ENSURE_TRUE_VOID(aWidget);

  InputContext oldContext = aWidget->GetInputContext();

  InputContext context;
  context.mIMEState = aState;

  if (aContent && aContent->GetNameSpaceID() == kNameSpaceID_XHTML &&
      (aContent->Tag() == nsGkAtoms::input ||
       aContent->Tag() == nsGkAtoms::textarea)) {
    if (aContent->Tag() != nsGkAtoms::textarea) {
      
      
      
      
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

    
    if (context.mActionHint.IsEmpty() && aContent->Tag() == nsGkAtoms::input) {
      bool willSubmit = false;
      nsCOMPtr<nsIFormControl> control(do_QueryInterface(aContent));
      mozilla::dom::Element* formElement = control->GetFormElement();
      nsCOMPtr<nsIForm> form;
      if (control) {
        
        if ((form = do_QueryInterface(formElement)) &&
            form->GetDefaultSubmitElement()) {
          willSubmit = true;
        
        } else if (formElement && formElement->Tag() == nsGkAtoms::form &&
                   formElement->IsHTML() &&
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
IMEStateManager::DispatchCompositionEvent(nsINode* aEventTargetNode,
                                          nsPresContext* aPresContext,
                                          WidgetEvent* aEvent,
                                          nsEventStatus* aStatus,
                                          EventDispatchingCallback* aCallBack)
{
  MOZ_ASSERT(aEvent->eventStructType == NS_COMPOSITION_EVENT ||
             aEvent->eventStructType == NS_TEXT_EVENT);
  if (!aEvent->mFlags.mIsTrusted || aEvent->mFlags.mPropagationStopped) {
    return;
  }

  EnsureTextCompositionArray();

  WidgetGUIEvent* GUIEvent = aEvent->AsGUIEvent();

  nsRefPtr<TextComposition> composition =
    sTextCompositions->GetCompositionFor(GUIEvent->widget);
  if (!composition) {
    MOZ_ASSERT(GUIEvent->message == NS_COMPOSITION_START);
    composition = new TextComposition(aPresContext, aEventTargetNode, GUIEvent);
    sTextCompositions->AppendElement(composition);
  }
#ifdef DEBUG
  else {
    MOZ_ASSERT(GUIEvent->message != NS_COMPOSITION_START);
  }
#endif 

  
  composition->DispatchEvent(GUIEvent, aStatus, aCallBack);

  

  
  if (aEvent->message == NS_COMPOSITION_END) {
    TextCompositionArray::index_type i =
      sTextCompositions->IndexOf(GUIEvent->widget);
    if (i != TextCompositionArray::NoIndex) {
      sTextCompositions->ElementAt(i)->Destroy();
      sTextCompositions->RemoveElementAt(i);
    }
  }
}


nsresult
IMEStateManager::NotifyIME(IMEMessage aMessage,
                           nsIWidget* aWidget)
{
  NS_ENSURE_TRUE(aWidget, NS_ERROR_INVALID_ARG);

  nsRefPtr<TextComposition> composition;
  if (sTextCompositions) {
    composition = sTextCompositions->GetCompositionFor(aWidget);
  }
  if (!composition || !composition->IsSynthesizedForTests()) {
    switch (aMessage) {
      case NOTIFY_IME_OF_CURSOR_POS_CHANGED:
        return aWidget->NotifyIME(IMENotification(aMessage));
      case REQUEST_TO_COMMIT_COMPOSITION:
      case REQUEST_TO_CANCEL_COMPOSITION:
      case NOTIFY_IME_OF_COMPOSITION_UPDATE:
        return composition ?
          aWidget->NotifyIME(IMENotification(aMessage)) : NS_OK;
      default:
        MOZ_CRASH("Unsupported notification");
    }
    MOZ_CRASH(
      "Failed to handle the notification for non-synthesized composition");
  }

  
  
  
  switch (aMessage) {
    case REQUEST_TO_COMMIT_COMPOSITION: {
      nsCOMPtr<nsIWidget> widget(aWidget);
      nsEventStatus status = nsEventStatus_eIgnore;
      if (!composition->LastData().IsEmpty()) {
        WidgetTextEvent textEvent(true, NS_TEXT_TEXT, widget);
        textEvent.theText = composition->LastData();
        textEvent.mFlags.mIsSynthesizedForTests = true;
        widget->DispatchEvent(&textEvent, status);
        if (widget->Destroyed()) {
          return NS_OK;
        }
      }

      status = nsEventStatus_eIgnore;
      WidgetCompositionEvent endEvent(true, NS_COMPOSITION_END, widget);
      endEvent.data = composition->LastData();
      endEvent.mFlags.mIsSynthesizedForTests = true;
      widget->DispatchEvent(&endEvent, status);

      return NS_OK;
    }
    case REQUEST_TO_CANCEL_COMPOSITION: {
      nsCOMPtr<nsIWidget> widget(aWidget);
      nsEventStatus status = nsEventStatus_eIgnore;
      if (!composition->LastData().IsEmpty()) {
        WidgetCompositionEvent updateEvent(true, NS_COMPOSITION_UPDATE, widget);
        updateEvent.data = composition->LastData();
        updateEvent.mFlags.mIsSynthesizedForTests = true;
        widget->DispatchEvent(&updateEvent, status);
        if (widget->Destroyed()) {
          return NS_OK;
        }

        status = nsEventStatus_eIgnore;
        WidgetTextEvent textEvent(true, NS_TEXT_TEXT, widget);
        textEvent.theText = composition->LastData();
        textEvent.mFlags.mIsSynthesizedForTests = true;
        widget->DispatchEvent(&textEvent, status);
        if (widget->Destroyed()) {
          return NS_OK;
        }
      }

      status = nsEventStatus_eIgnore;
      WidgetCompositionEvent endEvent(true, NS_COMPOSITION_END, widget);
      endEvent.data = composition->LastData();
      endEvent.mFlags.mIsSynthesizedForTests = true;
      widget->DispatchEvent(&endEvent, status);

      return NS_OK;
    }
    default:
      return NS_OK;
  }
}


nsresult
IMEStateManager::NotifyIME(IMEMessage aMessage,
                           nsPresContext* aPresContext)
{
  NS_ENSURE_TRUE(aPresContext, NS_ERROR_INVALID_ARG);

  nsIWidget* widget = aPresContext->GetRootWidget();
  if (!widget) {
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
IMEStateManager::DestroyTextStateManager()
{
  if (!sActiveIMEContentObserver) {
    return;
  }

  nsRefPtr<IMEContentObserver> tsm;
  tsm.swap(sActiveIMEContentObserver);
  tsm->Destroy();
}

void
IMEStateManager::CreateIMEContentObserver()
{
  if (sActiveIMEContentObserver) {
    NS_WARNING("text state observer has been there already");
    MOZ_ASSERT(sActiveIMEContentObserver->IsManaging(sPresContext, sContent));
    return;
  }

  nsCOMPtr<nsIWidget> widget = sPresContext->GetRootWidget();
  if (!widget) {
    return; 
  }

  
  if (!IsEditableIMEState(widget)) {
    return;
  }

  static bool sInitializeIsTestingIME = true;
  if (sInitializeIsTestingIME) {
    Preferences::AddBoolVarCache(&sIsTestingIME, "test.IME", false);
    sInitializeIsTestingIME = false;
  }

  sActiveIMEContentObserver = new IMEContentObserver();
  NS_ADDREF(sActiveIMEContentObserver);

  
  
  
  nsRefPtr<IMEContentObserver> kungFuDeathGrip(sActiveIMEContentObserver);
  sActiveIMEContentObserver->Init(widget, sPresContext, sContent);
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
IMEStateManager::GetTextCompositionFor(WidgetGUIEvent* aEvent)
{
  MOZ_ASSERT(aEvent->AsCompositionEvent() || aEvent->AsTextEvent(),
             "aEvent has to be WidgetCompositionEvent or WidgetTextEvent");
  return GetTextCompositionFor(aEvent->widget);
}

} 
