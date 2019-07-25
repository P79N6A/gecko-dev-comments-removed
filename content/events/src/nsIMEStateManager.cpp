







































#include "nsIMEStateManager.h"
#include "nsCOMPtr.h"
#include "nsIViewManager.h"
#include "nsIPresShell.h"
#include "nsISupports.h"
#include "nsPIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIEditorDocShell.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIDOMWindow.h"
#include "nsContentUtils.h"
#include "nsINode.h"
#include "nsIFrame.h"
#include "nsRange.h"
#include "nsIDOMRange.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISelectionListener.h"
#include "nsISelectionController.h"
#include "nsIMutationObserver.h"
#include "nsContentEventHandler.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsHTMLFormElement.h"

using namespace mozilla::widget;

static

IMEState::Enabled
GetWidgetStatusFromIMEStatus(PRUint32 aState)
{
  switch (aState & nsIContent::IME_STATUS_MASK_ENABLED) {
    case nsIContent::IME_STATUS_DISABLE:
      return IMEState::DISABLED;
    case nsIContent::IME_STATUS_ENABLE:
      return IMEState::ENABLED;
    case nsIContent::IME_STATUS_PASSWORD:
      return IMEState::PASSWORD;
    case nsIContent::IME_STATUS_PLUGIN:
      return IMEState::PLUGIN;
    default:
      NS_ERROR("The given state doesn't have valid enable state");
      return IMEState::ENABLED;
  }
}





nsIContent*    nsIMEStateManager::sContent      = nsnull;
nsPresContext* nsIMEStateManager::sPresContext  = nsnull;
bool           nsIMEStateManager::sInstalledMenuKeyboardListener = false;
bool           nsIMEStateManager::sInSecureInputMode = false;

nsTextStateManager* nsIMEStateManager::sTextStateObserver = nsnull;

nsresult
nsIMEStateManager::OnDestroyPresContext(nsPresContext* aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  if (aPresContext != sPresContext)
    return NS_OK;
  nsCOMPtr<nsIWidget> widget = GetWidget(sPresContext);
  if (widget) {
    PRUint32 newState = GetNewIMEState(sPresContext, nsnull);
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::LOST_FOCUS);
    SetIMEState(newState, nsnull, widget, action);
  }
  sContent = nsnull;
  sPresContext = nsnull;
  OnTextStateBlur(nsnull, nsnull);
  return NS_OK;
}

nsresult
nsIMEStateManager::OnRemoveContent(nsPresContext* aPresContext,
                                   nsIContent* aContent)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  if (!sPresContext || !sContent ||
      aPresContext != sPresContext ||
      aContent != sContent)
    return NS_OK;

  
  nsCOMPtr<nsIWidget> widget = GetWidget(sPresContext);
  if (widget) {
    nsresult rv = widget->CancelIMEComposition();
    if (NS_FAILED(rv))
      widget->ResetInputState();
    PRUint32 newState = GetNewIMEState(sPresContext, nsnull);
    InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                              InputContextAction::LOST_FOCUS);
    SetIMEState(newState, nsnull, widget, action);
  }

  sContent = nsnull;
  sPresContext = nsnull;

  return NS_OK;
}

nsresult
nsIMEStateManager::OnChangeFocus(nsPresContext* aPresContext,
                                 nsIContent* aContent,
                                 InputContextAction::Cause aCause)
{
  InputContextAction action(aCause);
  return OnChangeFocusInternal(aPresContext, aContent, action);
}

nsresult
nsIMEStateManager::OnChangeFocusInternal(nsPresContext* aPresContext,
                                         nsIContent* aContent,
                                         InputContextAction aAction)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  nsCOMPtr<nsIWidget> widget = GetWidget(aPresContext);
  if (!widget) {
    return NS_OK;
  }

  
  bool contentIsPassword = false;
  if (aContent && aContent->GetNameSpaceID() == kNameSpaceID_XHTML) {
    if (aContent->Tag() == nsGkAtoms::input) {
      nsAutoString type;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, type);
      contentIsPassword = type.LowerCaseEqualsLiteral("password");
    }
  }
  if (sInSecureInputMode) {
    if (!contentIsPassword) {
      if (NS_SUCCEEDED(widget->EndSecureKeyboardInput())) {
        sInSecureInputMode = false;
      }
    }
  } else {
    if (contentIsPassword) {
      if (NS_SUCCEEDED(widget->BeginSecureKeyboardInput())) {
        sInSecureInputMode = true;
      }
    }
  }

  PRUint32 newState = GetNewIMEState(aPresContext, aContent);
  if (aPresContext == sPresContext && aContent == sContent) {
    
    
    PRUint32 newEnabledState = newState & nsIContent::IME_STATUS_MASK_ENABLED;
    if (newEnabledState == 0) {
      
      return NS_OK;
    }
    InputContext context = widget->GetInputContext();
    if (context.mIMEState.mEnabled ==
          GetWidgetStatusFromIMEStatus(newEnabledState)) {
      
      return NS_OK;
    }
    aAction.mFocusChange = InputContextAction::FOCUS_NOT_CHANGED;
  } else if (aAction.mFocusChange == InputContextAction::FOCUS_NOT_CHANGED) {
    
    
    bool gotFocus = aContent || (newState & nsIContent::IME_STATUS_ENABLE);
    aAction.mFocusChange =
      gotFocus ? InputContextAction::GOT_FOCUS : InputContextAction::LOST_FOCUS;
  }

  
  if (sPresContext) {
    nsCOMPtr<nsIWidget> oldWidget;
    if (sPresContext == aPresContext)
      oldWidget = widget;
    else
      oldWidget = GetWidget(sPresContext);
    if (oldWidget)
      oldWidget->ResetInputState();
  }

  if (newState != nsIContent::IME_STATUS_NONE) {
    
    SetIMEState(newState, aContent, widget, aAction);
  }

  sPresContext = aPresContext;
  sContent = aContent;

  return NS_OK;
}

void
nsIMEStateManager::OnInstalledMenuKeyboardListener(bool aInstalling)
{
  sInstalledMenuKeyboardListener = aInstalling;

  InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
    aInstalling ? InputContextAction::MENU_GOT_PSEUDO_FOCUS :
                  InputContextAction::MENU_LOST_PSEUDO_FOCUS);
  OnChangeFocusInternal(sPresContext, sContent, action);
}

void
nsIMEStateManager::UpdateIMEState(PRUint32 aNewIMEState, nsIContent* aContent)
{
  if (!sPresContext) {
    NS_WARNING("ISM doesn't know which editor has focus");
    return;
  }
  NS_PRECONDITION(aNewIMEState != 0, "aNewIMEState doesn't specify new state.");
  nsCOMPtr<nsIWidget> widget = GetWidget(sPresContext);
  if (!widget) {
    NS_WARNING("focused widget is not found");
    return;
  }

  
  InputContext context = widget->GetInputContext();
  PRUint32 newEnabledState = aNewIMEState & nsIContent::IME_STATUS_MASK_ENABLED;
  if (context.mIMEState.mEnabled ==
        GetWidgetStatusFromIMEStatus(newEnabledState)) {
    return;
  }

  
  widget->ResetInputState();

  InputContextAction action(InputContextAction::CAUSE_UNKNOWN,
                            InputContextAction::FOCUS_NOT_CHANGED);
  SetIMEState(aNewIMEState, aContent, widget, action);
}

PRUint32
nsIMEStateManager::GetNewIMEState(nsPresContext* aPresContext,
                                  nsIContent*    aContent)
{
  
  if (aPresContext->Type() == nsPresContext::eContext_PrintPreview ||
      aPresContext->Type() == nsPresContext::eContext_Print) {
    return nsIContent::IME_STATUS_DISABLE;
  }

  if (sInstalledMenuKeyboardListener)
    return nsIContent::IME_STATUS_DISABLE;

  if (!aContent) {
    
    
    nsIDocument* doc = aPresContext->Document();
    if (doc && doc->HasFlag(NODE_IS_EDITABLE))
      return nsIContent::IME_STATUS_ENABLE;
    return nsIContent::IME_STATUS_DISABLE;
  }

  return aContent->GetDesiredIMEState();
}


class IMEEnabledStateChangedEvent : public nsRunnable {
public:
  IMEEnabledStateChangedEvent(PRUint32 aState)
    : mState(aState)
  {
  }

  NS_IMETHOD Run() {
    nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
    if (observerService) {
      nsAutoString state;
      state.AppendInt(mState);
      observerService->NotifyObservers(nsnull, "ime-enabled-state-changed", state.get());
    }
    return NS_OK;
  }

private:
  PRUint32 mState;
};

void
nsIMEStateManager::SetIMEState(PRUint32 aState,
                               nsIContent* aContent,
                               nsIWidget* aWidget,
                               InputContextAction aAction)
{
  NS_ENSURE_TRUE(aWidget, );

  InputContext context;
  context.mIMEState.mEnabled = GetWidgetStatusFromIMEStatus(aState);

  if (aContent && aContent->GetNameSpaceID() == kNameSpaceID_XHTML &&
      (aContent->Tag() == nsGkAtoms::input ||
       aContent->Tag() == nsGkAtoms::textarea)) {
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type,
                      context.mHTMLInputType);
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::moz_action_hint,
                      context.mActionHint);

    
    if (context.mActionHint.IsEmpty() && aContent->Tag() == nsGkAtoms::input) {
      bool willSubmit = false;
      nsCOMPtr<nsIFormControl> control(do_QueryInterface(aContent));
      mozilla::dom::Element* formElement = control->GetFormElement();
      nsCOMPtr<nsIForm> form;
      if (control) {
        
        if ((form = do_QueryInterface(formElement)) && form->GetDefaultSubmitElement()) {
          willSubmit = true;
        
        } else if (formElement && formElement->Tag() == nsGkAtoms::form && formElement->IsHTML() &&
                   static_cast<nsHTMLFormElement*>(formElement)->HasSingleTextControl()) {
          willSubmit = true;
        }
      }
      context.mActionHint.Assign(willSubmit ? control->GetType() == NS_FORM_INPUT_SEARCH
                                                ? NS_LITERAL_STRING("search")
                                                : NS_LITERAL_STRING("go")
                                            : formElement
                                                ? NS_LITERAL_STRING("next")
                                                : EmptyString());
    }
  }

  
  
  if (aAction.mCause == InputContextAction::CAUSE_UNKNOWN &&
      XRE_GetProcessType() != GeckoProcessType_Content) {
    aAction.mCause = InputContextAction::CAUSE_UNKNOWN_CHROME;
  }

  if (aState & nsIContent::IME_STATUS_MASK_OPENED) {
    bool open = !!(aState & nsIContent::IME_STATUS_OPEN);
    context.mIMEState.mOpen = open ? IMEState::OPEN : IMEState::CLOSED;
  }

  aWidget->SetInputContext(context, aAction);
  if (aState & nsIContent::IME_STATUS_MASK_ENABLED) {
    nsContentUtils::AddScriptRunner(
      new IMEEnabledStateChangedEvent(context.mIMEState.mEnabled));
  }
}

nsIWidget*
nsIMEStateManager::GetWidget(nsPresContext* aPresContext)
{
  nsIPresShell* shell = aPresContext->GetPresShell();
  NS_ENSURE_TRUE(shell, nsnull);

  nsIViewManager* vm = shell->GetViewManager();
  if (!vm)
    return nsnull;
  nsCOMPtr<nsIWidget> widget = nsnull;
  nsresult rv = vm->GetRootWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, nsnull);
  return widget;
}







class nsTextStateManager : public nsISelectionListener,
                           public nsStubMutationObserver
{
public:
  nsTextStateManager();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISELECTIONLISTENER
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  nsresult Init(nsIWidget* aWidget,
                nsPresContext* aPresContext,
                nsINode* aNode,
                bool aWantUpdates);
  void     Destroy(void);

  nsCOMPtr<nsIWidget>            mWidget;
  nsCOMPtr<nsISelection>         mSel;
  nsCOMPtr<nsIContent>           mRootContent;
  nsCOMPtr<nsINode>              mEditableNode;
  bool                           mDestroying;

private:
  void NotifyContentAdded(nsINode* aContainer, PRInt32 aStart, PRInt32 aEnd);
};

nsTextStateManager::nsTextStateManager()
{
  mDestroying = false;
}

nsresult
nsTextStateManager::Init(nsIWidget* aWidget,
                         nsPresContext* aPresContext,
                         nsINode* aNode,
                         bool aWantUpdates)
{
  mWidget = aWidget;
  MOZ_ASSERT(mWidget);
  if (!aWantUpdates) {
    mEditableNode = aNode;
    return NS_OK;
  }

  nsIPresShell* presShell = aPresContext->PresShell();

  
  nsCOMPtr<nsISelectionController> selCon;
  if (aNode->IsNodeOfType(nsINode::eCONTENT)) {
    nsIFrame* frame = static_cast<nsIContent*>(aNode)->GetPrimaryFrame();
    NS_ENSURE_TRUE(frame, NS_ERROR_UNEXPECTED);

    frame->GetSelectionController(aPresContext,
                                  getter_AddRefs(selCon));
  } else {
    
    selCon = do_QueryInterface(presShell);
  }
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> sel;
  nsresult rv = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                     getter_AddRefs(sel));
  NS_ENSURE_TRUE(sel, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMRange> selDomRange;
  rv = sel->GetRangeAt(0, getter_AddRefs(selDomRange));

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIRange> selRange(do_QueryInterface(selDomRange));
    NS_ENSURE_TRUE(selRange && selRange->GetStartParent(),
                   NS_ERROR_UNEXPECTED);

    mRootContent = selRange->GetStartParent()->
                     GetSelectionRootContent(presShell);
  } else {
    mRootContent = aNode->GetSelectionRootContent(presShell);
  }
  if (!mRootContent && aNode->IsNodeOfType(nsINode::eDOCUMENT)) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }
  NS_ENSURE_TRUE(mRootContent, NS_ERROR_UNEXPECTED);

  
  mRootContent->AddMutationObserver(this);

  
  nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(sel));
  NS_ENSURE_TRUE(selPrivate, NS_ERROR_UNEXPECTED);
  rv = selPrivate->AddSelectionListener(this);
  NS_ENSURE_SUCCESS(rv, rv);
  mSel = sel;

  mEditableNode = aNode;
  return NS_OK;
}

void
nsTextStateManager::Destroy(void)
{
  if (mSel) {
    nsCOMPtr<nsISelectionPrivate> selPrivate(do_QueryInterface(mSel));
    if (selPrivate)
      selPrivate->RemoveSelectionListener(this);
    mSel = nsnull;
  }
  if (mRootContent) {
    mRootContent->RemoveMutationObserver(this);
    mRootContent = nsnull;
  }
  mEditableNode = nsnull;
  mWidget = nsnull;
}

NS_IMPL_ISUPPORTS2(nsTextStateManager,
                   nsIMutationObserver,
                   nsISelectionListener)


class SelectionChangeEvent : public nsRunnable {
public:
  SelectionChangeEvent(nsIWidget *widget)
    : mWidget(widget)
  {
    MOZ_ASSERT(mWidget);
  }

  NS_IMETHOD Run() {
    if(mWidget) {
        mWidget->OnIMESelectionChange();
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIWidget> mWidget;
};

nsresult
nsTextStateManager::NotifySelectionChanged(nsIDOMDocument* aDoc,
                                           nsISelection* aSel,
                                           PRInt16 aReason)
{
  PRInt32 count = 0;
  nsresult rv = aSel->GetRangeCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count > 0 && mWidget) {
    nsContentUtils::AddScriptRunner(new SelectionChangeEvent(mWidget));
  }
  return NS_OK;
}


class TextChangeEvent : public nsRunnable {
public:
  TextChangeEvent(nsIWidget *widget,
                  PRUint32 start, PRUint32 oldEnd, PRUint32 newEnd)
    : mWidget(widget)
    , mStart(start)
    , mOldEnd(oldEnd)
    , mNewEnd(newEnd)
  {
    MOZ_ASSERT(mWidget);
  }

  NS_IMETHOD Run() {
    if(mWidget) {
        mWidget->OnIMETextChange(mStart, mOldEnd, mNewEnd);
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIWidget> mWidget;
  PRUint32 mStart, mOldEnd, mNewEnd;
};

void
nsTextStateManager::CharacterDataChanged(nsIDocument* aDocument,
                                         nsIContent* aContent,
                                         CharacterDataChangeInfo* aInfo)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eTEXT),
               "character data changed for non-text node");

  PRUint32 offset = 0;
  
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, aContent, aInfo->mChangeStart, &offset)))
    return;

  PRUint32 oldEnd = offset + aInfo->mChangeEnd - aInfo->mChangeStart;
  PRUint32 newEnd = offset + aInfo->mReplaceLength;

  nsContentUtils::AddScriptRunner(
      new TextChangeEvent(mWidget, offset, oldEnd, newEnd));
}

void
nsTextStateManager::NotifyContentAdded(nsINode* aContainer,
                                       PRInt32 aStartIndex,
                                       PRInt32 aEndIndex)
{
  PRUint32 offset = 0, newOffset = 0;
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, aContainer, aStartIndex, &offset)))
    return;

  
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    aContainer->GetChildAt(aStartIndex),
                    aContainer, aEndIndex, &newOffset)))
    return;

  
  if (newOffset)
    nsContentUtils::AddScriptRunner(
        new TextChangeEvent(mWidget, offset, offset, offset + newOffset));
}

void
nsTextStateManager::ContentAppended(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aFirstNewContent,
                                    PRInt32 aNewIndexInContainer)
{
  NotifyContentAdded(aContainer, aNewIndexInContainer,
                     aContainer->GetChildCount());
}

void
nsTextStateManager::ContentInserted(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aChild,
                                     PRInt32 aIndexInContainer)
{
  NotifyContentAdded(NODE_FROM(aContainer, aDocument),
                     aIndexInContainer, aIndexInContainer + 1);
}

void
nsTextStateManager::ContentRemoved(nsIDocument* aDocument,
                                   nsIContent* aContainer,
                                   nsIContent* aChild,
                                   PRInt32 aIndexInContainer,
                                   nsIContent* aPreviousSibling)
{
  PRUint32 offset = 0, childOffset = 1;
  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    mRootContent, NODE_FROM(aContainer, aDocument),
                    aIndexInContainer, &offset)))
    return;

  
  if (aChild->IsNodeOfType(nsINode::eTEXT))
    childOffset = aChild->TextLength();
  else if (0 < aChild->GetChildCount())
    childOffset = aChild->GetChildCount();

  if (NS_FAILED(nsContentEventHandler::GetFlatTextOffsetOfRange(
                    aChild, aChild, childOffset, &childOffset)))
    return;

  
  if (childOffset)
    nsContentUtils::AddScriptRunner(
        new TextChangeEvent(mWidget, offset, offset + childOffset, offset));
}

static nsINode* GetRootEditableNode(nsPresContext* aPresContext,
                                    nsIContent* aContent)
{
  if (aContent) {
    nsINode* root = nsnull;
    nsINode* node = aContent;
    while (node && node->IsEditable()) {
      root = node;
      node = node->GetNodeParent();
    }
    return root;
  }
  if (aPresContext) {
    nsIDocument* document = aPresContext->Document();
    if (document && document->IsEditable())
      return document;
  }
  return nsnull;
}

nsresult
nsIMEStateManager::OnTextStateBlur(nsPresContext* aPresContext,
                                   nsIContent* aContent)
{
  if (!sTextStateObserver || sTextStateObserver->mDestroying ||
      sTextStateObserver->mEditableNode ==
          GetRootEditableNode(aPresContext, aContent))
    return NS_OK;

  sTextStateObserver->mDestroying = true;
  sTextStateObserver->mWidget->OnIMEFocusChange(false);
  sTextStateObserver->Destroy();
  NS_RELEASE(sTextStateObserver);
  return NS_OK;
}

nsresult
nsIMEStateManager::OnTextStateFocus(nsPresContext* aPresContext,
                                    nsIContent* aContent)
{
  if (sTextStateObserver) return NS_OK;

  nsINode *editableNode = GetRootEditableNode(aPresContext, aContent);
  if (!editableNode) return NS_OK;

  nsIPresShell* shell = aPresContext->GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_NOT_AVAILABLE);
  
  nsIViewManager* vm = shell->GetViewManager();
  NS_ENSURE_TRUE(vm, NS_ERROR_NOT_AVAILABLE);

  nsCOMPtr<nsIWidget> widget;
  nsresult rv = vm->GetRootWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NOT_AVAILABLE);
  if (!widget) {
    return NS_OK; 
  }

  rv = widget->OnIMEFocusChange(true);
  if (rv == NS_ERROR_NOT_IMPLEMENTED)
    return NS_OK;
  NS_ENSURE_SUCCESS(rv, rv);

  bool wantUpdates = rv != NS_SUCCESS_IME_NO_UPDATES;

  
  
  NS_ENSURE_TRUE(!sTextStateObserver, NS_OK);

  sTextStateObserver = new nsTextStateManager();
  NS_ENSURE_TRUE(sTextStateObserver, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(sTextStateObserver);
  rv = sTextStateObserver->Init(widget, aPresContext,
                                editableNode, wantUpdates);
  if (NS_FAILED(rv)) {
    sTextStateObserver->mDestroying = true;
    sTextStateObserver->Destroy();
    NS_RELEASE(sTextStateObserver);
    widget->OnIMEFocusChange(false);
    return rv;
  }
  return NS_OK;
}

nsresult
nsIMEStateManager::GetFocusSelectionAndRoot(nsISelection** aSel,
                                            nsIContent** aRoot)
{
  if (!sTextStateObserver || !sTextStateObserver->mEditableNode ||
      !sTextStateObserver->mSel)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ASSERTION(sTextStateObserver->mSel && sTextStateObserver->mRootContent,
               "uninitialized text state observer");
  NS_ADDREF(*aSel = sTextStateObserver->mSel);
  NS_ADDREF(*aRoot = sTextStateObserver->mRootContent);
  return NS_OK;
}
