





#include "nsTextEditorState.h"

#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsView.h"
#include "nsCaret.h"
#include "nsEditorCID.h"
#include "nsLayoutCID.h"
#include "nsITextControlFrame.h" 
#include "nsIPlaintextEditor.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsContentCreatorFunctions.h"
#include "nsTextControlFrame.h"
#include "nsIControllers.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsITransactionManager.h"
#include "nsIControllerContext.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMEventListener.h"
#include "nsIEditorObserver.h"
#include "nsIWidget.h"
#include "nsIDocumentEncoder.h"
#include "nsISelectionPrivate.h"
#include "nsPIDOMWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsIEditor.h"
#include "nsTextEditRules.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/EventListenerManager.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"
#include "nsTextNode.h"
#include "nsIController.h"
#include "mozilla/TextEvents.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/HTMLInputElement.h"
#include "nsNumberControlFrame.h"
#include "nsFrameSelection.h"

using namespace mozilla;
using namespace mozilla::dom;

static NS_DEFINE_CID(kTextEditorCID, NS_TEXTEDITOR_CID);

class MOZ_STACK_CLASS ValueSetter
{
public:
  explicit ValueSetter(nsIEditor* aEditor)
    : mEditor(aEditor)
  {
    MOZ_ASSERT(aEditor);
  
    
    
    
    mEditor->GetSuppressDispatchingInputEvent(&mOuterTransaction);
  }
  ~ValueSetter()
  {
    mEditor->SetSuppressDispatchingInputEvent(mOuterTransaction);
  }
  void Init()
  {
    mEditor->SetSuppressDispatchingInputEvent(true);
  }

private:
  nsCOMPtr<nsIEditor> mEditor;
  bool mOuterTransaction;
};

class RestoreSelectionState : public nsRunnable {
public:
  RestoreSelectionState(nsTextEditorState *aState, nsTextControlFrame *aFrame)
    : mFrame(aFrame),
      mTextEditorState(aState)
  {
  }

  NS_IMETHOD Run() {
    if (!mTextEditorState) {
      return NS_OK;
    }

    if (mFrame) {
      
      
      nsAutoScriptBlocker scriptBlocker;
       nsTextEditorState::SelectionProperties& properties =
         mTextEditorState->GetSelectionProperties();
       mFrame->SetSelectionRange(properties.mStart,
                                 properties.mEnd,
                                 properties.mDirection);
      if (!mTextEditorState->mSelectionRestoreEagerInit) {
        mTextEditorState->HideSelectionIfBlurred();
      }
      mTextEditorState->mSelectionRestoreEagerInit = false;
    }
    mTextEditorState->FinishedRestoringSelection();
    return NS_OK;
  }

  
  void Revoke() {
    mFrame = nullptr;
    mTextEditorState = nullptr;
  }

private:
  nsTextControlFrame* mFrame;
  nsTextEditorState* mTextEditorState;
};


bool
nsITextControlElement::GetWrapPropertyEnum(nsIContent* aContent,
  nsITextControlElement::nsHTMLTextWrap& aWrapProp)
{
  
  
  
  
  aWrapProp = eHTMLTextWrap_Soft; 

  nsAutoString wrap;
  if (aContent->IsHTMLElement()) {
    static nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::HARD, &nsGkAtoms::OFF, nullptr};

    switch (aContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::wrap,
                                      strings, eIgnoreCase)) {
      case 0: aWrapProp = eHTMLTextWrap_Hard; break;
      case 1: aWrapProp = eHTMLTextWrap_Off; break;
    }

    return true;
  }

  return false;
}


already_AddRefed<nsITextControlElement>
nsITextControlElement::GetTextControlElementFromEditingHost(nsIContent* aHost)
{
  if (!aHost) {
    return nullptr;
  }

  nsCOMPtr<nsITextControlElement> parent =
    do_QueryInterface(aHost->GetParent());

  return parent.forget();
}

static bool
SuppressEventHandlers(nsPresContext* aPresContext)
{
  bool suppressHandlers = false;

  if (aPresContext)
  {
    
    

    
    

    suppressHandlers = aPresContext->IsPaginated();
  }

  return suppressHandlers;
}

class nsAnonDivObserver final : public nsStubMutationObserver
{
public:
  explicit nsAnonDivObserver(nsTextEditorState* aTextEditorState)
  : mTextEditorState(aTextEditorState) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

private:
  ~nsAnonDivObserver() {}
  nsTextEditorState* mTextEditorState;
};

class nsTextInputSelectionImpl final : public nsSupportsWeakReference
                                     , public nsISelectionController
{
  ~nsTextInputSelectionImpl(){}

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTextInputSelectionImpl, nsISelectionController)

  nsTextInputSelectionImpl(nsFrameSelection *aSel, nsIPresShell *aShell, nsIContent *aLimiter);

  void SetScrollableFrame(nsIScrollableFrame *aScrollableFrame);
  nsFrameSelection* GetConstFrameSelection()
    { return mFrameSelection; }

  
  NS_IMETHOD SetDisplaySelection(int16_t toggle) override;
  NS_IMETHOD GetDisplaySelection(int16_t* _retval) override;
  NS_IMETHOD SetSelectionFlags(int16_t aInEnable) override;
  NS_IMETHOD GetSelectionFlags(int16_t *aOutEnable) override;
  NS_IMETHOD GetSelection(int16_t type, nsISelection** _retval) override;
  NS_IMETHOD ScrollSelectionIntoView(int16_t aType, int16_t aRegion, int16_t aFlags) override;
  NS_IMETHOD RepaintSelection(int16_t type) override;
  NS_IMETHOD RepaintSelection(nsPresContext* aPresContext, SelectionType aSelectionType);
  NS_IMETHOD SetCaretEnabled(bool enabled) override;
  NS_IMETHOD SetCaretReadOnly(bool aReadOnly) override;
  NS_IMETHOD GetCaretEnabled(bool* _retval) override;
  NS_IMETHOD GetCaretVisible(bool* _retval) override;
  NS_IMETHOD SetCaretVisibilityDuringSelection(bool aVisibility) override;
  NS_IMETHOD PhysicalMove(int16_t aDirection, int16_t aAmount, bool aExtend) override;
  NS_IMETHOD CharacterMove(bool aForward, bool aExtend) override;
  NS_IMETHOD CharacterExtendForDelete() override;
  NS_IMETHOD CharacterExtendForBackspace() override;
  NS_IMETHOD WordMove(bool aForward, bool aExtend) override;
  NS_IMETHOD WordExtendForDelete(bool aForward) override;
  NS_IMETHOD LineMove(bool aForward, bool aExtend) override;
  NS_IMETHOD IntraLineMove(bool aForward, bool aExtend) override;
  NS_IMETHOD PageMove(bool aForward, bool aExtend) override;
  NS_IMETHOD CompleteScroll(bool aForward) override;
  NS_IMETHOD CompleteMove(bool aForward, bool aExtend) override;
  NS_IMETHOD ScrollPage(bool aForward) override;
  NS_IMETHOD ScrollLine(bool aForward) override;
  NS_IMETHOD ScrollCharacter(bool aRight) override;
  NS_IMETHOD SelectAll(void) override;
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, int16_t startOffset, int16_t EndOffset, bool* _retval) override;
  virtual nsresult CheckVisibilityContent(nsIContent* aNode, int16_t aStartOffset, int16_t aEndOffset, bool* aRetval) override;

private:
  nsRefPtr<nsFrameSelection> mFrameSelection;
  nsCOMPtr<nsIContent>       mLimiter;
  nsIScrollableFrame        *mScrollFrame;
  nsWeakPtr mPresShellWeak;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTextInputSelectionImpl)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTextInputSelectionImpl)
NS_INTERFACE_TABLE_HEAD(nsTextInputSelectionImpl)
  NS_INTERFACE_TABLE(nsTextInputSelectionImpl,
                     nsISelectionController,
                     nsISelectionDisplay,
                     nsISupportsWeakReference)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsTextInputSelectionImpl)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION(nsTextInputSelectionImpl, mFrameSelection, mLimiter)




nsTextInputSelectionImpl::nsTextInputSelectionImpl(nsFrameSelection *aSel,
                                                   nsIPresShell *aShell,
                                                   nsIContent *aLimiter)
  : mScrollFrame(nullptr)
{
  if (aSel && aShell)
  {
    mFrameSelection = aSel;
    mLimiter = aLimiter;
    mFrameSelection->Init(aShell, mLimiter);
    mPresShellWeak = do_GetWeakReference(aShell);
  }
}

void
nsTextInputSelectionImpl::SetScrollableFrame(nsIScrollableFrame *aScrollableFrame)
{
  mScrollFrame = aScrollableFrame;
  if (!mScrollFrame && mFrameSelection) {
    mFrameSelection->DisconnectFromPresShell();
    mFrameSelection = nullptr;
  }
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetDisplaySelection(int16_t aToggle)
{
  if (!mFrameSelection)
    return NS_ERROR_NULL_POINTER;
  
  mFrameSelection->SetDisplaySelection(aToggle);
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetDisplaySelection(int16_t *aToggle)
{
  if (!mFrameSelection)
    return NS_ERROR_NULL_POINTER;

  *aToggle = mFrameSelection->GetDisplaySelection();
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetSelectionFlags(int16_t aToggle)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetSelectionFlags(int16_t *aOutEnable)
{
  *aOutEnable = nsISelectionDisplay::DISPLAY_TEXT;
  return NS_OK; 
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetSelection(int16_t type, nsISelection **_retval)
{
  if (!mFrameSelection)
    return NS_ERROR_NULL_POINTER;
    
  *_retval = mFrameSelection->GetSelection(type);
  
  if (!(*_retval))
    return NS_ERROR_FAILURE;

  NS_ADDREF(*_retval);
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::ScrollSelectionIntoView(int16_t aType, int16_t aRegion, int16_t aFlags)
{
  if (!mFrameSelection) 
    return NS_ERROR_FAILURE; 

  return mFrameSelection->ScrollSelectionIntoView(aType, aRegion, aFlags);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::RepaintSelection(int16_t type)
{
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;

  return mFrameSelection->RepaintSelection(type);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::RepaintSelection(nsPresContext* aPresContext, SelectionType aSelectionType)
{
  if (!mFrameSelection)
    return NS_ERROR_FAILURE;

  return mFrameSelection->RepaintSelection(aSelectionType);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetCaretEnabled(bool enabled)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak);
  if (!shell) return NS_ERROR_FAILURE;

  
  
  nsCOMPtr<nsISelectionController> selCon = do_QueryInterface(shell);
  if (!selCon) return NS_ERROR_NO_INTERFACE;
  selCon->SetCaretEnabled(enabled);

  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetCaretReadOnly(bool aReadOnly)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    nsRefPtr<nsCaret> caret = shell->GetCaret();
    if (caret) {
      nsISelection* domSel = mFrameSelection->
        GetSelection(nsISelectionController::SELECTION_NORMAL);
      if (domSel)
        caret->SetCaretReadOnly(aReadOnly);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetCaretEnabled(bool *_retval)
{
  return GetCaretVisible(_retval);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetCaretVisible(bool *_retval)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    nsRefPtr<nsCaret> caret = shell->GetCaret();
    if (caret) {
      *_retval = caret->IsVisible();
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetCaretVisibilityDuringSelection(bool aVisibility)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsIPresShell> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    nsRefPtr<nsCaret> caret = shell->GetCaret();
    if (caret) {
      nsISelection* domSel = mFrameSelection->
        GetSelection(nsISelectionController::SELECTION_NORMAL);
      if (domSel)
        caret->SetVisibilityDuringSelection(aVisibility);
      return NS_OK;
    }
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::PhysicalMove(int16_t aDirection, int16_t aAmount,
                                       bool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->PhysicalMove(aDirection, aAmount, aExtend);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::CharacterMove(bool aForward, bool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->CharacterMove(aForward, aExtend);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::CharacterExtendForDelete()
{
  if (mFrameSelection)
    return mFrameSelection->CharacterExtendForDelete();
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::CharacterExtendForBackspace()
{
  if (mFrameSelection)
    return mFrameSelection->CharacterExtendForBackspace();
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::WordMove(bool aForward, bool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->WordMove(aForward, aExtend);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::WordExtendForDelete(bool aForward)
{
  if (mFrameSelection)
    return mFrameSelection->WordExtendForDelete(aForward);
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::LineMove(bool aForward, bool aExtend)
{
  if (mFrameSelection)
  {
    nsresult result = mFrameSelection->LineMove(aForward, aExtend);
    if (NS_FAILED(result))
      result = CompleteMove(aForward,aExtend);
    return result;
  }
  return NS_ERROR_NULL_POINTER;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::IntraLineMove(bool aForward, bool aExtend)
{
  if (mFrameSelection)
    return mFrameSelection->IntraLineMove(aForward, aExtend);
  return NS_ERROR_NULL_POINTER;
}


NS_IMETHODIMP
nsTextInputSelectionImpl::PageMove(bool aForward, bool aExtend)
{
  
  
  if (mScrollFrame)
  {
    mFrameSelection->CommonPageMove(aForward, aExtend, mScrollFrame);
  }
  
  
  return ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL, nsISelectionController::SELECTION_FOCUS_REGION,
                                 nsISelectionController::SCROLL_SYNCHRONOUS);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::CompleteScroll(bool aForward)
{
  if (!mScrollFrame)
    return NS_ERROR_NOT_INITIALIZED;

  mScrollFrame->ScrollBy(nsIntPoint(0, aForward ? 1 : -1),
                         nsIScrollableFrame::WHOLE,
                         nsIScrollableFrame::INSTANT);
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::CompleteMove(bool aForward, bool aExtend)
{
  
  nsIContent* parentDIV = mFrameSelection->GetLimiter();
  if (!parentDIV)
    return NS_ERROR_UNEXPECTED;

  
  int32_t offset = 0;
  CaretAssociationHint hint = CARET_ASSOCIATE_BEFORE;
  if (aForward)
  {
    offset = parentDIV->GetChildCount();

    
    

    if (offset > 0)
    {
      nsIContent *child = parentDIV->GetLastChild();

      if (child->IsHTMLElement(nsGkAtoms::br))
      {
        --offset;
        hint = CARET_ASSOCIATE_AFTER; 
      }
    }
  }

  mFrameSelection->HandleClick(parentDIV, offset, offset, aExtend,
                               false, hint);

  
  return CompleteScroll(aForward);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::ScrollPage(bool aForward)
{
  if (!mScrollFrame)
    return NS_ERROR_NOT_INITIALIZED;

  mScrollFrame->ScrollBy(nsIntPoint(0, aForward ? 1 : -1),
                         nsIScrollableFrame::PAGES,
                         nsIScrollableFrame::SMOOTH);
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::ScrollLine(bool aForward)
{
  if (!mScrollFrame)
    return NS_ERROR_NOT_INITIALIZED;

  mScrollFrame->ScrollBy(nsIntPoint(0, aForward ? 1 : -1),
                         nsIScrollableFrame::LINES,
                         nsIScrollableFrame::SMOOTH);
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::ScrollCharacter(bool aRight)
{
  if (!mScrollFrame)
    return NS_ERROR_NOT_INITIALIZED;

  mScrollFrame->ScrollBy(nsIntPoint(aRight ? 1 : -1, 0),
                         nsIScrollableFrame::LINES,
                         nsIScrollableFrame::SMOOTH);
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SelectAll()
{
  if (mFrameSelection)
    return mFrameSelection->SelectAll();
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::CheckVisibility(nsIDOMNode *node, int16_t startOffset, int16_t EndOffset, bool *_retval)
{
  if (!mPresShellWeak) return NS_ERROR_NOT_INITIALIZED;
  nsresult result;
  nsCOMPtr<nsISelectionController> shell = do_QueryReferent(mPresShellWeak, &result);
  if (shell)
  {
    return shell->CheckVisibility(node,startOffset,EndOffset, _retval);
  }
  return NS_ERROR_FAILURE;

}

nsresult
nsTextInputSelectionImpl::CheckVisibilityContent(nsIContent* aNode,
                                                 int16_t aStartOffset,
                                                 int16_t aEndOffset,
                                                 bool* aRetval)
{
  if (!mPresShellWeak) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsCOMPtr<nsISelectionController> shell = do_QueryReferent(mPresShellWeak);
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  return shell->CheckVisibilityContent(aNode, aStartOffset, aEndOffset, aRetval);
}

class nsTextInputListener : public nsISelectionListener,
                            public nsIDOMEventListener,
                            public nsIEditorObserver,
                            public nsSupportsWeakReference
{
public:
  
 
  explicit nsTextInputListener(nsITextControlElement* aTxtCtrlElement);

  


  void SetFrame(nsTextControlFrame *aFrame){mFrame = aFrame;}

  void SettingValue(bool aValue) { mSettingValue = aValue; }
  void SetValueChanged(bool aSetValueChanged) { mSetValueChanged = aSetValueChanged; }

  NS_DECL_ISUPPORTS

  NS_DECL_NSISELECTIONLISTENER

  NS_DECL_NSIDOMEVENTLISTENER

  NS_DECL_NSIEDITOROBSERVER

protected:
  

  virtual ~nsTextInputListener();

  nsresult  UpdateTextInputCommands(const nsAString& commandsToUpdate,
                                    nsISelection* sel = nullptr,
                                    int16_t reason = 0);

protected:

  nsIFrame* mFrame;

  nsITextControlElement* const mTxtCtrlElement;

  bool            mSelectionWasCollapsed;
  



  bool            mHadUndoItems;
  



  bool            mHadRedoItems;
  



  bool mSettingValue;
  



  bool mSetValueChanged;
};






nsTextInputListener::nsTextInputListener(nsITextControlElement* aTxtCtrlElement)
: mFrame(nullptr)
, mTxtCtrlElement(aTxtCtrlElement)
, mSelectionWasCollapsed(true)
, mHadUndoItems(false)
, mHadRedoItems(false)
, mSettingValue(false)
, mSetValueChanged(true)
{
}

nsTextInputListener::~nsTextInputListener() 
{
}

NS_IMPL_ISUPPORTS(nsTextInputListener,
                  nsISelectionListener,
                  nsIEditorObserver,
                  nsISupportsWeakReference,
                  nsIDOMEventListener)



NS_IMETHODIMP
nsTextInputListener::NotifySelectionChanged(nsIDOMDocument* aDoc, nsISelection* aSel, int16_t aReason)
{
  bool collapsed;
  nsWeakFrame weakFrame = mFrame;

  if (!aDoc || !aSel || NS_FAILED(aSel->GetIsCollapsed(&collapsed)))
    return NS_OK;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (!collapsed && (aReason & (nsISelectionListener::MOUSEUP_REASON | 
                                nsISelectionListener::KEYPRESS_REASON |
                                nsISelectionListener::SELECTALL_REASON)))
  {
    nsIContent* content = mFrame->GetContent();
    if (content) 
    {
      nsCOMPtr<nsIDocument> doc = content->GetComposedDoc();
      if (doc) 
      {
        nsCOMPtr<nsIPresShell> presShell = doc->GetShell();
        if (presShell) 
        {
          nsEventStatus status = nsEventStatus_eIgnore;
          WidgetEvent event(true, NS_FORM_SELECTED);

          presShell->HandleEventWithTarget(&event, mFrame, content, &status);
        }
      }
    }
  }

  
  if (collapsed == mSelectionWasCollapsed)
    return NS_OK;

  mSelectionWasCollapsed = collapsed;

  if (!weakFrame.IsAlive() || !nsContentUtils::IsFocusedContent(mFrame->GetContent()))
    return NS_OK;

  return UpdateTextInputCommands(NS_LITERAL_STRING("select"), aSel, aReason);
}



static void
DoCommandCallback(Command aCommand, void* aData)
{
  nsTextControlFrame *frame = static_cast<nsTextControlFrame*>(aData);
  nsIContent *content = frame->GetContent();

  nsCOMPtr<nsIControllers> controllers;
  nsCOMPtr<nsIDOMHTMLInputElement> input = do_QueryInterface(content);
  if (input) {
    input->GetControllers(getter_AddRefs(controllers));
  } else {
    nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea =
      do_QueryInterface(content);

    if (textArea) {
      textArea->GetControllers(getter_AddRefs(controllers));
    }
  }

  if (!controllers) {
    NS_WARNING("Could not get controllers");
    return;
  }

  const char* commandStr = WidgetKeyboardEvent::GetCommandStr(aCommand);

  nsCOMPtr<nsIController> controller;
  controllers->GetControllerForCommand(commandStr, getter_AddRefs(controller));
  if (!controller) {
    return;
  }

  bool commandEnabled;
  nsresult rv = controller->IsCommandEnabled(commandStr, &commandEnabled);
  NS_ENSURE_SUCCESS_VOID(rv);
  if (commandEnabled) {
    controller->DoCommand(commandStr);
  }
}

NS_IMETHODIMP
nsTextInputListener::HandleEvent(nsIDOMEvent* aEvent)
{
  bool defaultPrevented = false;
  nsresult rv = aEvent->GetDefaultPrevented(&defaultPrevented);
  NS_ENSURE_SUCCESS(rv, rv);
  if (defaultPrevented) {
    return NS_OK;
  }

  bool isTrusted = false;
  rv = aEvent->GetIsTrusted(&isTrusted);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isTrusted) {
    return NS_OK;
  }

  WidgetKeyboardEvent* keyEvent =
    aEvent->GetInternalNSEvent()->AsKeyboardEvent();
  if (!keyEvent) {
    return NS_ERROR_UNEXPECTED;
  }

  if (keyEvent->message != NS_KEY_PRESS) {
    return NS_OK;
  }

  nsIWidget::NativeKeyBindingsType nativeKeyBindingsType =
    mTxtCtrlElement->IsTextArea() ?
      nsIWidget::NativeKeyBindingsForMultiLineEditor :
      nsIWidget::NativeKeyBindingsForSingleLineEditor;
  nsIWidget* widget = keyEvent->widget;
  
  if (!widget) {
    widget = mFrame->GetNearestWidget();
    NS_ENSURE_TRUE(widget, NS_OK);
  }
                                         
  if (widget->ExecuteNativeKeyBinding(nativeKeyBindingsType,
                                      *keyEvent, DoCommandCallback, mFrame)) {
    aEvent->PreventDefault();
  }
  return NS_OK;
}



NS_IMETHODIMP
nsTextInputListener::EditAction()
{
  nsWeakFrame weakFrame = mFrame;

  nsITextControlFrame* frameBase = do_QueryFrame(mFrame);
  nsTextControlFrame* frame = static_cast<nsTextControlFrame*> (frameBase);
  NS_ASSERTION(frame, "Where is our frame?");
  
  
  
  nsCOMPtr<nsIEditor> editor;
  frame->GetEditor(getter_AddRefs(editor));

  
  int32_t numUndoItems = 0;
  int32_t numRedoItems = 0;
  editor->GetNumberOfUndoItems(&numUndoItems);
  editor->GetNumberOfRedoItems(&numRedoItems);
  if ((numUndoItems && !mHadUndoItems) || (!numUndoItems && mHadUndoItems) ||
      (numRedoItems && !mHadRedoItems) || (!numRedoItems && mHadRedoItems)) {
    
    UpdateTextInputCommands(NS_LITERAL_STRING("undo"));

    mHadUndoItems = numUndoItems != 0;
    mHadRedoItems = numRedoItems != 0;
  }

  if (!weakFrame.IsAlive()) {
    return NS_OK;
  }

  
  
  if (mSetValueChanged) {
    frame->SetValueChanged(true);
  }

  if (!mSettingValue) {
    mTxtCtrlElement->OnValueChanged(true);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTextInputListener::BeforeEditAction()
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputListener::CancelEditAction()
{
  return NS_OK;
}




nsresult
nsTextInputListener::UpdateTextInputCommands(const nsAString& commandsToUpdate,
                                             nsISelection* sel,
                                             int16_t reason)
{
  nsIContent* content = mFrame->GetContent();
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIDocument> doc = content->GetComposedDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsPIDOMWindow *domWindow = doc->GetWindow();
  NS_ENSURE_TRUE(domWindow, NS_ERROR_FAILURE);

  return domWindow->UpdateCommands(commandsToUpdate, sel, reason);
}





nsTextEditorState::nsTextEditorState(nsITextControlElement* aOwningElement)
  : mTextCtrlElement(aOwningElement),
    mBoundFrame(nullptr),
    mEverInited(false),
    mEditorInitialized(false),
    mInitializing(false),
    mValueTransferInProgress(false),
    mSelectionCached(true),
    mSelectionRestoreEagerInit(false),
    mPlaceholderVisibility(false)
{
  MOZ_COUNT_CTOR(nsTextEditorState);
}

nsTextEditorState::~nsTextEditorState()
{
  MOZ_COUNT_DTOR(nsTextEditorState);
  Clear();
}

void
nsTextEditorState::Clear()
{
  if (mBoundFrame) {
    
    
    
    
    UnbindFromFrame(mBoundFrame);
    mEditor = nullptr;
  } else {
    
    
    DestroyEditor();
  }
  mTextListener = nullptr;
}

void nsTextEditorState::Unlink()
{
  nsTextEditorState* tmp = this;
  tmp->Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSelCon)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mEditor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRootNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPlaceholderDiv)
}

void nsTextEditorState::Traverse(nsCycleCollectionTraversalCallback& cb)
{
  nsTextEditorState* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSelCon)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEditor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRootNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPlaceholderDiv)
}

nsFrameSelection*
nsTextEditorState::GetConstFrameSelection() {
  if (mSelCon)
    return mSelCon->GetConstFrameSelection();
  return nullptr;
}

nsIEditor*
nsTextEditorState::GetEditor()
{
  if (!mEditor) {
    nsresult rv = PrepareEditor();
    NS_ENSURE_SUCCESS(rv, nullptr);
  }
  return mEditor;
}

nsISelectionController*
nsTextEditorState::GetSelectionController() const
{
  return mSelCon;
}


class PrepareEditorEvent : public nsRunnable {
public:
  PrepareEditorEvent(nsTextEditorState &aState,
                     nsIContent *aOwnerContent,
                     const nsAString &aCurrentValue)
    : mState(&aState)
    , mOwnerContent(aOwnerContent)
    , mCurrentValue(aCurrentValue)
  {
    aState.mValueTransferInProgress = true;
  }

  NS_IMETHOD Run() {
    NS_ENSURE_TRUE(mState, NS_ERROR_NULL_POINTER);

    
    const nsAString *value = nullptr;
    if (!mCurrentValue.IsEmpty()) {
      value = &mCurrentValue;
    }

    nsAutoScriptBlocker scriptBlocker;

    mState->PrepareEditor(value);

    mState->mValueTransferInProgress = false;

    return NS_OK;
  }

private:
  WeakPtr<nsTextEditorState> mState;
  nsCOMPtr<nsIContent> mOwnerContent; 
  nsAutoString mCurrentValue;
};

nsresult
nsTextEditorState::BindToFrame(nsTextControlFrame* aFrame)
{
  NS_ASSERTION(aFrame, "The frame to bind to should be valid");
  NS_ENSURE_ARG_POINTER(aFrame);

  NS_ASSERTION(!mBoundFrame, "Cannot bind twice, need to unbind first");
  NS_ENSURE_TRUE(!mBoundFrame, NS_ERROR_FAILURE);

  
  
  nsAutoString currentValue;
  if (mEditor) {
    GetValue(currentValue, true);
  }

  mBoundFrame = aFrame;

  nsIContent *rootNode = GetRootNode();

  nsresult rv = InitializeRootNode();
  NS_ENSURE_SUCCESS(rv, rv);

  nsIPresShell *shell = mBoundFrame->PresContext()->GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  
  nsRefPtr<nsFrameSelection> frameSel = new nsFrameSelection();

  
  mSelCon = new nsTextInputSelectionImpl(frameSel, shell, rootNode);
  MOZ_ASSERT(!mTextListener, "Should not overwrite the object");
  mTextListener = new nsTextInputListener(mTextCtrlElement);

  mTextListener->SetFrame(mBoundFrame);
  mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);

  
  nsRefPtr<nsISelection> domSelection;
  if (NS_SUCCEEDED(mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                         getter_AddRefs(domSelection))) &&
      domSelection) {
    nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(domSelection));
    nsRefPtr<nsCaret> caret = shell->GetCaret();
    nsCOMPtr<nsISelectionListener> listener;
    if (caret) {
      listener = do_QueryInterface(caret);
      if (listener) {
        selPriv->AddSelectionListener(listener);
      }
    }

    selPriv->AddSelectionListener(static_cast<nsISelectionListener*>
                                             (mTextListener));
  }

  
  if (mEditor) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
    NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);

    
    uint32_t flags;
    nsresult rv = mEditor->GetFlags(&flags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (flags & nsIPlaintextEditor::eEditorRightToLeft) {
      rootNode->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("rtl"), false);
    } else if (flags & nsIPlaintextEditor::eEditorLeftToRight) {
      rootNode->SetAttr(kNameSpaceID_None, nsGkAtoms::dir, NS_LITERAL_STRING("ltr"), false);
    } else {
      
    }

    if (!nsContentUtils::AddScriptRunner(
          new PrepareEditorEvent(*this, content, currentValue)))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

struct PreDestroyer
{
  void Init(nsIEditor* aEditor)
  {
    mNewEditor = aEditor;
  }
  ~PreDestroyer()
  {
    if (mNewEditor) {
      mNewEditor->PreDestroy(true);
    }
  }
  void Swap(nsCOMPtr<nsIEditor>& aEditor)
  {
    return mNewEditor.swap(aEditor);
  }
private:
  nsCOMPtr<nsIEditor> mNewEditor;
};

nsresult
nsTextEditorState::PrepareEditor(const nsAString *aValue)
{
  if (!mBoundFrame) {
    
    
    return NS_OK;
  }

  if (mEditorInitialized) {
    
    return NS_OK;
  }

  
  InitializationGuard guard(*this);
  if (guard.IsInitializingRecursively()) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  
  

  nsPresContext *presContext = mBoundFrame->PresContext();
  nsIPresShell *shell = presContext->GetPresShell();

  
  uint32_t editorFlags = 0;
  if (IsPlainTextControl())
    editorFlags |= nsIPlaintextEditor::eEditorPlaintextMask;
  if (IsSingleLineTextControl())
    editorFlags |= nsIPlaintextEditor::eEditorSingleLineMask;
  if (IsPasswordTextControl())
    editorFlags |= nsIPlaintextEditor::eEditorPasswordMask;

  
  editorFlags |= nsIPlaintextEditor::eEditorWidgetMask;

  
  
  editorFlags |= nsIPlaintextEditor::eEditorSkipSpellCheck;

  bool shouldInitializeEditor = false;
  nsCOMPtr<nsIEditor> newEditor; 
  nsresult rv = NS_OK;
  PreDestroyer preDestroyer;
  if (!mEditor) {
    shouldInitializeEditor = true;

    
    newEditor = do_CreateInstance(kTextEditorCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    preDestroyer.Init(newEditor);

    
    rv = mBoundFrame->UpdateValueDisplay(true, true);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    if (aValue || !mEditorInitialized) {
      
      rv = mBoundFrame->UpdateValueDisplay(true, !mEditorInitialized, aValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    newEditor = mEditor; 
  }

  
  
  
  nsAutoString defaultValue;
  if (aValue) {
    defaultValue = *aValue;
  } else {
    GetValue(defaultValue, true);
  }

  if (!mEditorInitialized) {
    
    
    
    

    
    nsCOMPtr<nsIDOMDocument> domdoc = do_QueryInterface(shell->GetDocument());
    if (!domdoc)
      return NS_ERROR_FAILURE;

    
    
    
    
    
    
    
    AutoNoJSAPI nojsapi;

    rv = newEditor->Init(domdoc, GetRootNode(), mSelCon, editorFlags,
                         defaultValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  

  if (!SuppressEventHandlers(presContext)) {
    nsCOMPtr<nsIControllers> controllers;
    nsCOMPtr<nsIDOMHTMLInputElement> inputElement =
      do_QueryInterface(mTextCtrlElement);
    if (inputElement) {
      rv = inputElement->GetControllers(getter_AddRefs(controllers));
    } else {
      nsCOMPtr<nsIDOMHTMLTextAreaElement> textAreaElement =
        do_QueryInterface(mTextCtrlElement);

      if (!textAreaElement)
        return NS_ERROR_FAILURE;

      rv = textAreaElement->GetControllers(getter_AddRefs(controllers));
    }

    NS_ENSURE_SUCCESS(rv, rv);

    if (controllers) {
      uint32_t numControllers;
      bool found = false;
      rv = controllers->GetControllerCount(&numControllers);
      for (uint32_t i = 0; i < numControllers; i ++) {
        nsCOMPtr<nsIController> controller;
        rv = controllers->GetControllerAt(i, getter_AddRefs(controller));
        if (NS_SUCCEEDED(rv) && controller) {
          nsCOMPtr<nsIControllerContext> editController =
            do_QueryInterface(controller);
          if (editController) {
            editController->SetCommandContext(newEditor);
            found = true;
          }
        }
      }
      if (!found)
        rv = NS_ERROR_FAILURE;
    }
  }

  if (shouldInitializeEditor) {
    
    nsCOMPtr<nsIPlaintextEditor> textEditor(do_QueryInterface(newEditor));
    if (textEditor) {
      
      textEditor->SetWrapColumn(GetWrapCols());

      
      int32_t maxLength;
      if (GetMaxLength(&maxLength)) { 
        textEditor->SetMaxTextLength(maxLength);
      }
    }
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
  if (content) {
    rv = newEditor->GetFlags(&editorFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (content->HasAttr(kNameSpaceID_None, nsGkAtoms::readonly))
      editorFlags |= nsIPlaintextEditor::eEditorReadonlyMask;

    
    
    if (content->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) 
      editorFlags |= nsIPlaintextEditor::eEditorDisabledMask;

    
    if (editorFlags & nsIPlaintextEditor::eEditorDisabledMask)
      mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_OFF);

    newEditor->SetFlags(editorFlags);
  }

  if (shouldInitializeEditor) {
    
    preDestroyer.Swap(mEditor);
  }

  
  
  
  

  if (!defaultValue.IsEmpty()) {
    rv = newEditor->SetFlags(editorFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    

    rv = newEditor->EnableUndo(false);
    NS_ENSURE_SUCCESS(rv, rv);

    bool success = SetValue(defaultValue, false, false);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    rv = newEditor->EnableUndo(true);
    NS_ASSERTION(NS_SUCCEEDED(rv),"Transaction Manager must have failed");

    
    rv = newEditor->SetFlags(editorFlags);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsITransactionManager> transMgr;
  newEditor->GetTransactionManager(getter_AddRefs(transMgr));
  NS_ENSURE_TRUE(transMgr, NS_ERROR_FAILURE);

  transMgr->SetMaxTransactionCount(nsITextControlElement::DEFAULT_UNDO_CAP);

  if (IsPasswordTextControl()) {
    
    
    
    
    
    newEditor->EnableUndo(false);
  }

  if (!mEditorInitialized) {
    newEditor->PostCreate();
    mEverInited = true;
    mEditorInitialized = true;
  }

  if (mTextListener)
    newEditor->AddEditorObserver(mTextListener);

  
  HTMLInputElement* number = GetParentNumberControl(mBoundFrame);
  if (number ? number->IsSelectionCached() : mSelectionCached) {
    if (mRestoringSelection) 
      mRestoringSelection->Revoke();
    mRestoringSelection = new RestoreSelectionState(this, mBoundFrame);
    if (mRestoringSelection) {
      nsContentUtils::AddScriptRunner(mRestoringSelection);
    }
  }

  
  if (number) {
    number->ClearSelectionCached();
  } else {
    mSelectionCached = false;
  }

  return rv;
}

void
nsTextEditorState::FinishedRestoringSelection()
{
  mRestoringSelection = nullptr;
}

bool
nsTextEditorState::IsSelectionCached() const
{
  if (mBoundFrame) {
    HTMLInputElement* number = GetParentNumberControl(mBoundFrame);
    if (number) {
      return number->IsSelectionCached();
    }
  }
  return mSelectionCached;
}

nsTextEditorState::SelectionProperties&
nsTextEditorState::GetSelectionProperties()
{
  if (mBoundFrame) {
    HTMLInputElement* number = GetParentNumberControl(mBoundFrame);
    if (number) {
      return number->GetSelectionProperties();
    }
  }
  return mSelectionProperties;
}

HTMLInputElement*
nsTextEditorState::GetParentNumberControl(nsFrame* aFrame) const
{
  MOZ_ASSERT(aFrame);
  nsIContent* content = aFrame->GetContent();
  MOZ_ASSERT(content);
  nsIContent* parent = content->GetParent();
  if (!parent) {
    return nullptr;
  }
  nsIContent* parentOfParent = parent->GetParent();
  if (!parentOfParent) {
    return nullptr;
  }
  HTMLInputElement* input = HTMLInputElement::FromContent(parentOfParent);
  if (input) {
    
    
    
    
    
    return (input->GetType() == NS_FORM_INPUT_NUMBER) ? input : nullptr;
  }

  return nullptr;
}

void
nsTextEditorState::DestroyEditor()
{
  
  if (mEditorInitialized) {
    if (mTextListener)
      mEditor->RemoveEditorObserver(mTextListener);

    mEditor->PreDestroy(true);
    mEditorInitialized = false;
  }
  ClearValueCache();
}

void
nsTextEditorState::UnbindFromFrame(nsTextControlFrame* aFrame)
{
  NS_ENSURE_TRUE_VOID(mBoundFrame);

  
  NS_ASSERTION(!aFrame || aFrame == mBoundFrame, "Unbinding from the wrong frame");
  NS_ENSURE_TRUE_VOID(!aFrame || aFrame == mBoundFrame);

  
  
  
  bool isInEditAction = false;
  if (mTextListener && mEditor && mEditorInitialized &&
      NS_SUCCEEDED(mEditor->GetIsInEditAction(&isInEditAction)) &&
      isInEditAction) {
    mTextListener->EditAction();
  }

  
  
  nsAutoString value;
  GetValue(value, true);

  if (mRestoringSelection) {
    mRestoringSelection->Revoke();
    mRestoringSelection = nullptr;
  }

  
  
  
  
  
  
  if (mEditorInitialized) {
    HTMLInputElement* number = GetParentNumberControl(aFrame);
    if (number) {
      
      
      
      SelectionProperties props;
      mBoundFrame->GetSelectionRange(&props.mStart, &props.mEnd,
                                     &props.mDirection);
      number->SetSelectionProperties(props);
    } else {
      mBoundFrame->GetSelectionRange(&mSelectionProperties.mStart,
                                     &mSelectionProperties.mEnd,
                                     &mSelectionProperties.mDirection);
      mSelectionCached = true;
    }
  }

  
  DestroyEditor();

  
  if (!SuppressEventHandlers(mBoundFrame->PresContext()))
  {
    nsCOMPtr<nsIControllers> controllers;
    nsCOMPtr<nsIDOMHTMLInputElement> inputElement =
      do_QueryInterface(mTextCtrlElement);
    if (inputElement)
      inputElement->GetControllers(getter_AddRefs(controllers));
    else
    {
      nsCOMPtr<nsIDOMHTMLTextAreaElement> textAreaElement =
        do_QueryInterface(mTextCtrlElement);
      if (textAreaElement) {
        textAreaElement->GetControllers(getter_AddRefs(controllers));
      }
    }

    if (controllers)
    {
      uint32_t numControllers;
      nsresult rv = controllers->GetControllerCount(&numControllers);
      NS_ASSERTION((NS_SUCCEEDED(rv)), "bad result in gfx text control destructor");
      for (uint32_t i = 0; i < numControllers; i ++)
      {
        nsCOMPtr<nsIController> controller;
        rv = controllers->GetControllerAt(i, getter_AddRefs(controller));
        if (NS_SUCCEEDED(rv) && controller)
        {
          nsCOMPtr<nsIControllerContext> editController = do_QueryInterface(controller);
          if (editController)
          {
            editController->SetCommandContext(nullptr);
          }
        }
      }
    }
  }

  if (mSelCon) {
    if (mTextListener) {
      nsRefPtr<nsISelection> domSelection;
      if (NS_SUCCEEDED(mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                                             getter_AddRefs(domSelection))) &&
          domSelection) {
        nsCOMPtr<nsISelectionPrivate> selPriv(do_QueryInterface(domSelection));

        selPriv->RemoveSelectionListener(static_cast<nsISelectionListener*>
                                         (mTextListener));
      }
    }

    mSelCon->SetScrollableFrame(nullptr);
    mSelCon = nullptr;
  }

  if (mTextListener)
  {
    mTextListener->SetFrame(nullptr);

    nsCOMPtr<EventTarget> target = do_QueryInterface(mTextCtrlElement);
    EventListenerManager* manager = target->GetExistingListenerManager();
    if (manager) {
      manager->RemoveEventListenerByType(mTextListener,
        NS_LITERAL_STRING("keydown"),
        TrustedEventsAtSystemGroupBubble());
      manager->RemoveEventListenerByType(mTextListener,
        NS_LITERAL_STRING("keypress"),
        TrustedEventsAtSystemGroupBubble());
      manager->RemoveEventListenerByType(mTextListener,
        NS_LITERAL_STRING("keyup"),
        TrustedEventsAtSystemGroupBubble());
    }

    mTextListener = nullptr;
  }

  mBoundFrame = nullptr;

  
  
  if (!mValueTransferInProgress) {
    bool success = SetValue(value, false, false);
    
    NS_ENSURE_TRUE_VOID(success);
  }

  if (mRootNode && mMutationObserver) {
    mRootNode->RemoveMutationObserver(mMutationObserver);
    mMutationObserver = nullptr;
  }

  
  
  
  nsContentUtils::DestroyAnonymousContent(&mRootNode);
  nsContentUtils::DestroyAnonymousContent(&mPlaceholderDiv);
}

nsresult
nsTextEditorState::CreateRootNode()
{
  NS_ENSURE_TRUE(!mRootNode, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG_POINTER(mBoundFrame);

  nsIPresShell *shell = mBoundFrame->PresContext()->GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  nsIDocument *doc = shell->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::div, nullptr,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);

  nsresult rv = NS_NewHTMLElement(getter_AddRefs(mRootNode), nodeInfo.forget(),
                                  NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!IsSingleLineTextControl()) {
    mMutationObserver = new nsAnonDivObserver(this);
    mRootNode->AddMutationObserver(mMutationObserver);
  }

  return rv;
}

nsresult
nsTextEditorState::InitializeRootNode()
{
  
  mRootNode->SetFlags(NODE_IS_EDITABLE);

  
  
  
  nsAutoString classValue;
  classValue.AppendLiteral("anonymous-div");
  int32_t wrapCols = GetWrapCols();
  if (wrapCols > 0) {
    classValue.AppendLiteral(" wrap");
  }
  if (!IsSingleLineTextControl()) {
    
    
    
    
    const nsStyleDisplay* disp = mBoundFrame->StyleDisplay();
    if (disp->mOverflowX != NS_STYLE_OVERFLOW_VISIBLE &&
        disp->mOverflowX != NS_STYLE_OVERFLOW_CLIP) {
      classValue.AppendLiteral(" inherit-overflow");
    }
  }
  nsresult rv = mRootNode->SetAttr(kNameSpaceID_None, nsGkAtoms::_class,
                                   classValue, false);
  NS_ENSURE_SUCCESS(rv, rv);

  return mBoundFrame->UpdateValueDisplay(false);
}

nsresult
nsTextEditorState::CreatePlaceholderNode()
{
#ifdef DEBUG
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
    if (content) {
      nsAutoString placeholderTxt;
      content->GetAttr(kNameSpaceID_None, nsGkAtoms::placeholder,
                       placeholderTxt);
      nsContentUtils::RemoveNewlines(placeholderTxt);
      NS_ASSERTION(!placeholderTxt.IsEmpty(), "CreatePlaceholderNode() shouldn't \
be called if @placeholder is the empty string when trimmed from line breaks");
    }
  }
#endif 

  NS_ENSURE_TRUE(!mPlaceholderDiv, NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG_POINTER(mBoundFrame);

  nsIPresShell *shell = mBoundFrame->PresContext()->GetPresShell();
  NS_ENSURE_TRUE(shell, NS_ERROR_FAILURE);

  nsIDocument *doc = shell->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsNodeInfoManager* pNodeInfoManager = doc->NodeInfoManager();
  NS_ENSURE_TRUE(pNodeInfoManager, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  
  
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = pNodeInfoManager->GetNodeInfo(nsGkAtoms::div, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

  rv = NS_NewHTMLElement(getter_AddRefs(mPlaceholderDiv), nodeInfo.forget(),
                         NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsRefPtr<nsTextNode> placeholderText = new nsTextNode(pNodeInfoManager);

  rv = mPlaceholderDiv->AppendChildTo(placeholderText, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  UpdatePlaceholderText(false);

  return NS_OK;
}

bool
nsTextEditorState::GetMaxLength(int32_t* aMaxLength)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
  nsGenericHTMLElement* element =
    nsGenericHTMLElement::FromContentOrNull(content);
  NS_ENSURE_TRUE(element, false);

  const nsAttrValue* attr = element->GetParsedAttr(nsGkAtoms::maxlength);
  if (attr && attr->Type() == nsAttrValue::eInteger) {
    *aMaxLength = attr->GetIntegerValue();

    return true;
  }

  return false;
}

void
nsTextEditorState::GetValue(nsAString& aValue, bool aIgnoreWrap) const
{
  if (mEditor && mBoundFrame && (mEditorInitialized || !IsSingleLineTextControl())) {
    bool canCache = aIgnoreWrap && !IsSingleLineTextControl();
    if (canCache && !mCachedValue.IsEmpty()) {
      aValue = mCachedValue;
      return;
    }

    aValue.Truncate(); 

    uint32_t flags = (nsIDocumentEncoder::OutputLFLineBreak |
                      nsIDocumentEncoder::OutputPreformatted |
                      nsIDocumentEncoder::OutputPersistNBSP);

    if (IsPlainTextControl())
    {
      flags |= nsIDocumentEncoder::OutputBodyOnly;
    }

    if (!aIgnoreWrap) {
      nsITextControlElement::nsHTMLTextWrap wrapProp;
      nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
      if (content &&
          nsITextControlElement::GetWrapPropertyEnum(content, wrapProp) &&
          wrapProp == nsITextControlElement::eHTMLTextWrap_Hard) {
        flags |= nsIDocumentEncoder::OutputWrap;
      }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    { 
      AutoNoJSAPI nojsapi;

      mEditor->OutputToString(NS_LITERAL_STRING("text/plain"), flags,
                              aValue);
    }
    if (canCache) {
      mCachedValue = aValue;
    } else {
      mCachedValue.Truncate();
    }
  } else {
    if (!mTextCtrlElement->ValueChanged() || !mValue) {
      mTextCtrlElement->GetDefaultValueFromContent(aValue);
    } else {
      aValue = NS_ConvertUTF8toUTF16(*mValue);
    }
  }
}

bool
nsTextEditorState::SetValue(const nsAString& aValue, bool aUserInput,
                            bool aSetValueChanged)
{
  if (mEditor && mBoundFrame) {
    
    
    
    
    
    nsAutoScriptBlocker scriptBlocker;

#ifdef DEBUG
    if (IsSingleLineTextControl()) {
      NS_ASSERTION(mEditorInitialized || mInitializing,
                   "We should never try to use the editor if we're not initialized unless we're being initialized");
    }
#endif

    nsAutoString currentValue;
    mBoundFrame->GetText(currentValue);

    nsWeakFrame weakFrame(mBoundFrame);

    
    if (!currentValue.Equals(aValue))
    {
      ValueSetter valueSetter(mEditor);

      
      
      
      
      nsString newValue;
      if (!newValue.Assign(aValue, fallible)) {
        return false;
      }
      if (aValue.FindChar(char16_t('\r')) != -1) {
        if (!nsContentUtils::PlatformToDOMLineBreaks(newValue, fallible)) {
          return false;
        }
      }

      nsCOMPtr<nsIDOMDocument> domDoc;
      mEditor->GetDocument(getter_AddRefs(domDoc));
      if (!domDoc) {
        NS_WARNING("Why don't we have a document?");
        return true;
      }

      
      
      
      {
        AutoNoJSAPI nojsapi;

        nsCOMPtr<nsISelection> domSel;
        nsCOMPtr<nsISelectionPrivate> selPriv;
        mSelCon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                              getter_AddRefs(domSel));
        if (domSel)
        {
          selPriv = do_QueryInterface(domSel);
          if (selPriv)
            selPriv->StartBatchChanges();
        }

        nsCOMPtr<nsISelectionController> kungFuDeathGrip = mSelCon.get();
        uint32_t currentLength = currentValue.Length();
        uint32_t newlength = newValue.Length();
        if (!currentLength ||
            !StringBeginsWith(newValue, currentValue)) {
          
          currentLength = 0;
          mSelCon->SelectAll();
        } else {
          
          mBoundFrame->SelectAllOrCollapseToEndOfText(false);
        }
        const nsAString& insertValue =
          StringTail(newValue, newlength - currentLength);
        nsCOMPtr<nsIPlaintextEditor> plaintextEditor = do_QueryInterface(mEditor);
        if (!plaintextEditor || !weakFrame.IsAlive()) {
          NS_WARNING("Somehow not a plaintext editor?");
          return true;
        }

        valueSetter.Init();

        
        
        uint32_t flags, savedFlags;
        mEditor->GetFlags(&savedFlags);
        flags = savedFlags;
        flags &= ~(nsIPlaintextEditor::eEditorDisabledMask);
        flags &= ~(nsIPlaintextEditor::eEditorReadonlyMask);
        flags |= nsIPlaintextEditor::eEditorDontEchoPassword;
        mEditor->SetFlags(flags);

        mTextListener->SettingValue(true);
        mTextListener->SetValueChanged(aSetValueChanged);

        
        int32_t savedMaxLength;
        plaintextEditor->GetMaxTextLength(&savedMaxLength);
        plaintextEditor->SetMaxTextLength(-1);

        if (insertValue.IsEmpty()) {
          mEditor->DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
        } else {
          plaintextEditor->InsertText(insertValue);
        }

        mTextListener->SetValueChanged(true);
        mTextListener->SettingValue(false);

        if (!weakFrame.IsAlive()) {
          
          
          
          
          
          if (!mBoundFrame) {
            return SetValue(newValue, false, aSetValueChanged);
          }
          return true;
        }

        if (!IsSingleLineTextControl()) {
          if (!mCachedValue.Assign(newValue, fallible)) {
            return false;
          }
        }

        plaintextEditor->SetMaxTextLength(savedMaxLength);
        mEditor->SetFlags(savedFlags);
        if (selPriv)
          selPriv->EndBatchChanges();
      }
    }
  } else {
    if (!mValue) {
      mValue = new nsCString;
    }
    nsString value;
    if (!value.Assign(aValue, fallible)) {
      return false;
    }
    if (!nsContentUtils::PlatformToDOMLineBreaks(value, fallible)) {
      return false;
    }
    if (!CopyUTF16toUTF8(value, *mValue, fallible)) {
      return false;
    }

    
    if (mBoundFrame) {
      mBoundFrame->UpdateValueDisplay(true);
    }
  }

  
  
  ValueWasChanged(!!mRootNode);

  mTextCtrlElement->OnValueChanged(!!mRootNode);

  return true;
}

void
nsTextEditorState::InitializeKeyboardEventListeners()
{
  
  nsCOMPtr<EventTarget> target = do_QueryInterface(mTextCtrlElement);
  EventListenerManager* manager = target->GetOrCreateListenerManager();
  if (manager) {
    manager->AddEventListenerByType(mTextListener,
                                    NS_LITERAL_STRING("keydown"),
                                    TrustedEventsAtSystemGroupBubble());
    manager->AddEventListenerByType(mTextListener,
                                    NS_LITERAL_STRING("keypress"),
                                    TrustedEventsAtSystemGroupBubble());
    manager->AddEventListenerByType(mTextListener,
                                    NS_LITERAL_STRING("keyup"),
                                    TrustedEventsAtSystemGroupBubble());
  }

  mSelCon->SetScrollableFrame(do_QueryFrame(mBoundFrame->GetFirstPrincipalChild()));
}

void
nsTextEditorState::ValueWasChanged(bool aNotify)
{
  UpdatePlaceholderVisibility(aNotify);
}

void
nsTextEditorState::UpdatePlaceholderText(bool aNotify)
{
  NS_ASSERTION(mPlaceholderDiv, "This function should not be called if "
                                "mPlaceholderDiv isn't set");

  
  if (!mPlaceholderDiv)
    return;

  nsAutoString placeholderValue;

  nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
  content->GetAttr(kNameSpaceID_None, nsGkAtoms::placeholder, placeholderValue);
  nsContentUtils::RemoveNewlines(placeholderValue);
  NS_ASSERTION(mPlaceholderDiv->GetFirstChild(), "placeholder div has no child");
  mPlaceholderDiv->GetFirstChild()->SetText(placeholderValue, aNotify);
}

void
nsTextEditorState::UpdatePlaceholderVisibility(bool aNotify)
{
  nsAutoString value;
  GetValue(value, true);

  mPlaceholderVisibility = value.IsEmpty();

  if (mPlaceholderVisibility &&
      !Preferences::GetBool("dom.placeholder.show_on_focus", true)) {
    nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
    mPlaceholderVisibility = !nsContentUtils::IsFocusedContent(content);
  }

  if (mBoundFrame && aNotify) {
    mBoundFrame->InvalidateFrame();
  }
}

void
nsTextEditorState::HideSelectionIfBlurred()
{
  MOZ_ASSERT(mSelCon, "Should have a selection controller if we have a frame!");
  nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
  if (!nsContentUtils::IsFocusedContent(content)) {
    mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);
  }
}

NS_IMPL_ISUPPORTS(nsAnonDivObserver, nsIMutationObserver)

void
nsAnonDivObserver::CharacterDataChanged(nsIDocument*             aDocument,
                                        nsIContent*              aContent,
                                        CharacterDataChangeInfo* aInfo)
{
  mTextEditorState->ClearValueCache();
}

void
nsAnonDivObserver::ContentAppended(nsIDocument* aDocument,
                                   nsIContent*  aContainer,
                                   nsIContent*  aFirstNewContent,
                                   int32_t      )
{
  mTextEditorState->ClearValueCache();
}

void
nsAnonDivObserver::ContentInserted(nsIDocument* aDocument,
                                   nsIContent*  aContainer,
                                   nsIContent*  aChild,
                                   int32_t      )
{
  mTextEditorState->ClearValueCache();
}

void
nsAnonDivObserver::ContentRemoved(nsIDocument* aDocument,
                                  nsIContent*  aContainer,
                                  nsIContent*  aChild,
                                  int32_t      aIndexInContainer,
                                  nsIContent*  aPreviousSibling)
{
  mTextEditorState->ClearValueCache();
}
