





#include "nsTextEditorState.h"

#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsIView.h"
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
#include "nsGenericHTMLElement.h"
#include "nsIDOMEventListener.h"
#include "nsIEditorObserver.h"
#include "nsINativeKeyBindings.h"
#include "nsIDocumentEncoder.h"
#include "nsISelectionPrivate.h"
#include "nsPIDOMWindow.h"
#include "nsServiceManagerUtils.h"
#include "nsIEditor.h"
#include "nsTextEditRules.h"
#include "mozilla/Selection.h"
#include "nsEventListenerManager.h"
#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

static NS_DEFINE_CID(kTextEditorCID, NS_TEXTEDITOR_CID);

static nsINativeKeyBindings *sNativeInputBindings = nsnull;
static nsINativeKeyBindings *sNativeTextAreaBindings = nsnull;

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
    mFrame = nsnull;
    mTextEditorState = nsnull;
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
  if (aContent->IsHTML()) {
    static nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::HARD, &nsGkAtoms::OFF, nsnull};

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
    return nsnull;
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

class nsAnonDivObserver MOZ_FINAL : public nsStubMutationObserver
{
public:
  nsAnonDivObserver(nsTextEditorState* aTextEditorState)
  : mTextEditorState(aTextEditorState) {}
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

private:
  nsTextEditorState* mTextEditorState;
};

class nsTextInputSelectionImpl MOZ_FINAL : public nsSupportsWeakReference
                                         , public nsISelectionController
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTextInputSelectionImpl, nsISelectionController)

  nsTextInputSelectionImpl(nsFrameSelection *aSel, nsIPresShell *aShell, nsIContent *aLimiter);
  ~nsTextInputSelectionImpl(){}

  void SetScrollableFrame(nsIScrollableFrame *aScrollableFrame);
  nsFrameSelection* GetConstFrameSelection()
    { return mFrameSelection; }

  
  NS_IMETHOD SetDisplaySelection(PRInt16 toggle);
  NS_IMETHOD GetDisplaySelection(PRInt16 *_retval);
  NS_IMETHOD SetSelectionFlags(PRInt16 aInEnable);
  NS_IMETHOD GetSelectionFlags(PRInt16 *aOutEnable);
  NS_IMETHOD GetSelection(PRInt16 type, nsISelection **_retval);
  NS_IMETHOD ScrollSelectionIntoView(PRInt16 aType, PRInt16 aRegion, PRInt16 aFlags);
  NS_IMETHOD RepaintSelection(PRInt16 type);
  NS_IMETHOD RepaintSelection(nsPresContext* aPresContext, SelectionType aSelectionType);
  NS_IMETHOD SetCaretEnabled(bool enabled);
  NS_IMETHOD SetCaretReadOnly(bool aReadOnly);
  NS_IMETHOD GetCaretEnabled(bool *_retval);
  NS_IMETHOD GetCaretVisible(bool *_retval);
  NS_IMETHOD SetCaretVisibilityDuringSelection(bool aVisibility);
  NS_IMETHOD CharacterMove(bool aForward, bool aExtend);
  NS_IMETHOD CharacterExtendForDelete();
  NS_IMETHOD CharacterExtendForBackspace();
  NS_IMETHOD WordMove(bool aForward, bool aExtend);
  NS_IMETHOD WordExtendForDelete(bool aForward);
  NS_IMETHOD LineMove(bool aForward, bool aExtend);
  NS_IMETHOD IntraLineMove(bool aForward, bool aExtend);
  NS_IMETHOD PageMove(bool aForward, bool aExtend);
  NS_IMETHOD CompleteScroll(bool aForward);
  NS_IMETHOD CompleteMove(bool aForward, bool aExtend);
  NS_IMETHOD ScrollPage(bool aForward);
  NS_IMETHOD ScrollLine(bool aForward);
  NS_IMETHOD ScrollCharacter(bool aRight);
  NS_IMETHOD SelectAll(void);
  NS_IMETHOD CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, bool *_retval);
  virtual nsresult CheckVisibilityContent(nsIContent* aNode, PRInt16 aStartOffset, PRInt16 aEndOffset, bool* aRetval);

private:
  nsRefPtr<nsFrameSelection> mFrameSelection;
  nsCOMPtr<nsIContent>       mLimiter;
  nsIScrollableFrame        *mScrollFrame;
  nsWeakPtr mPresShellWeak;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTextInputSelectionImpl)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTextInputSelectionImpl)
NS_INTERFACE_TABLE_HEAD(nsTextInputSelectionImpl)
  NS_INTERFACE_TABLE3(nsTextInputSelectionImpl,
                      nsISelectionController,
                      nsISelectionDisplay,
                      nsISupportsWeakReference)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(nsTextInputSelectionImpl)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_2(nsTextInputSelectionImpl, mFrameSelection, mLimiter)




nsTextInputSelectionImpl::nsTextInputSelectionImpl(nsFrameSelection *aSel,
                                                   nsIPresShell *aShell,
                                                   nsIContent *aLimiter)
  : mScrollFrame(nsnull)
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
    mFrameSelection = nsnull;
  }
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetDisplaySelection(PRInt16 aToggle)
{
  if (!mFrameSelection)
    return NS_ERROR_NULL_POINTER;
  
  mFrameSelection->SetDisplaySelection(aToggle);
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetDisplaySelection(PRInt16 *aToggle)
{
  if (!mFrameSelection)
    return NS_ERROR_NULL_POINTER;

  *aToggle = mFrameSelection->GetDisplaySelection();
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::SetSelectionFlags(PRInt16 aToggle)
{
  return NS_OK;
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetSelectionFlags(PRInt16 *aOutEnable)
{
  *aOutEnable = nsISelectionDisplay::DISPLAY_TEXT;
  return NS_OK; 
}

NS_IMETHODIMP
nsTextInputSelectionImpl::GetSelection(PRInt16 type, nsISelection **_retval)
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
nsTextInputSelectionImpl::ScrollSelectionIntoView(PRInt16 aType, PRInt16 aRegion, PRInt16 aFlags)
{
  if (!mFrameSelection) 
    return NS_ERROR_FAILURE; 

  return mFrameSelection->ScrollSelectionIntoView(aType, aRegion, aFlags);
}

NS_IMETHODIMP
nsTextInputSelectionImpl::RepaintSelection(PRInt16 type)
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
      nsISelection* domSel = mFrameSelection->
        GetSelection(nsISelectionController::SELECTION_NORMAL);
      if (domSel)
        return caret->GetCaretVisible(_retval);
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

  
  PRInt32 offset = 0;
  nsFrameSelection::HINT hint = nsFrameSelection::HINTLEFT;
  if (aForward)
  {
    offset = parentDIV->GetChildCount();

    
    

    if (offset > 0)
    {
      nsIContent *child = parentDIV->GetLastChild();

      if (child->Tag() == nsGkAtoms::br)
      {
        --offset;
        hint = nsFrameSelection::HINTRIGHT; 
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
nsTextInputSelectionImpl::CheckVisibility(nsIDOMNode *node, PRInt16 startOffset, PRInt16 EndOffset, bool *_retval)
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
                                                 PRInt16 aStartOffset,
                                                 PRInt16 aEndOffset,
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
  

  virtual ~nsTextInputListener();

  


  void SetFrame(nsTextControlFrame *aFrame){mFrame = aFrame;}

  void SettingValue(bool aValue) { mSettingValue = aValue; }
  void SetValueChanged(bool aSetValueChanged) { mSetValueChanged = aSetValueChanged; }

  NS_DECL_ISUPPORTS

  NS_DECL_NSISELECTIONLISTENER

  NS_DECL_NSIDOMEVENTLISTENER

  NS_DECL_NSIEDITOROBSERVER

protected:

  nsresult  UpdateTextInputCommands(const nsAString& commandsToUpdate);

  NS_HIDDEN_(nsINativeKeyBindings*) GetKeyBindings();

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
: mFrame(nsnull)
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

NS_IMPL_ISUPPORTS4(nsTextInputListener,
                   nsISelectionListener,
                   nsIEditorObserver,
                   nsISupportsWeakReference,
                   nsIDOMEventListener)



NS_IMETHODIMP
nsTextInputListener::NotifySelectionChanged(nsIDOMDocument* aDoc, nsISelection* aSel, PRInt16 aReason)
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
      nsCOMPtr<nsIDocument> doc = content->GetDocument();
      if (doc) 
      {
        nsCOMPtr<nsIPresShell> presShell = doc->GetShell();
        if (presShell) 
        {
          nsEventStatus status = nsEventStatus_eIgnore;
          nsEvent event(true, NS_FORM_SELECTED);

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

  return UpdateTextInputCommands(NS_LITERAL_STRING("select"));
}



static void
DoCommandCallback(const char *aCommand, void *aData)
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

  nsCOMPtr<nsIController> controller;
  controllers->GetControllerForCommand(aCommand, getter_AddRefs(controller));
  if (controller) {
    controller->DoCommand(aCommand);
  }
}

NS_IMETHODIMP
nsTextInputListener::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMKeyEvent> keyEvent(do_QueryInterface(aEvent));
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_INVALID_ARG);

  nsAutoString eventType;
  aEvent->GetType(eventType);

  nsNativeKeyEvent nativeEvent;
  nsINativeKeyBindings *bindings = GetKeyBindings();
  if (bindings &&
      nsContentUtils::DOMEventToNativeKeyEvent(keyEvent, &nativeEvent, false)) {

    bool handled = false;
    if (eventType.EqualsLiteral("keydown")) {
      handled = bindings->KeyDown(nativeEvent, DoCommandCallback, mFrame);
    }
    else if (eventType.EqualsLiteral("keyup")) {
      handled = bindings->KeyUp(nativeEvent, DoCommandCallback, mFrame);
    }
    else if (eventType.EqualsLiteral("keypress")) {
      handled = bindings->KeyPress(nativeEvent, DoCommandCallback, mFrame);
    }
    else {
      NS_ABORT();
    }
    if (handled) {
      aEvent->PreventDefault();
    }
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

  
  PRInt32 numUndoItems = 0;
  PRInt32 numRedoItems = 0;
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




nsresult
nsTextInputListener::UpdateTextInputCommands(const nsAString& commandsToUpdate)
{
  nsIContent* content = mFrame->GetContent();
  NS_ENSURE_TRUE(content, NS_ERROR_FAILURE);
  
  nsCOMPtr<nsIDocument> doc = content->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsPIDOMWindow *domWindow = doc->GetWindow();
  NS_ENSURE_TRUE(domWindow, NS_ERROR_FAILURE);

  return domWindow->UpdateCommands(commandsToUpdate);
}

nsINativeKeyBindings*
nsTextInputListener::GetKeyBindings()
{
  if (mTxtCtrlElement->IsTextArea()) {
    static bool sNoTextAreaBindings = false;

    if (!sNativeTextAreaBindings && !sNoTextAreaBindings) {
      CallGetService(NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX "textarea",
                     &sNativeTextAreaBindings);

      if (!sNativeTextAreaBindings) {
        sNoTextAreaBindings = true;
      }
    }

    return sNativeTextAreaBindings;
  }

  static bool sNoInputBindings = false;
  if (!sNativeInputBindings && !sNoInputBindings) {
    CallGetService(NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX "input",
                   &sNativeInputBindings);

    if (!sNativeInputBindings) {
      sNoInputBindings = true;
    }
  }

  return sNativeInputBindings;
}





nsTextEditorState::nsTextEditorState(nsITextControlElement* aOwningElement)
  : mTextCtrlElement(aOwningElement),
    mRestoringSelection(nsnull),
    mBoundFrame(nsnull),
    mTextListener(nsnull),
    mEverInited(false),
    mEditorInitialized(false),
    mInitializing(false),
    mValueTransferInProgress(false),
    mSelectionCached(true),
    mSelectionRestoreEagerInit(false)
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
    mEditor = nsnull;
  } else {
    
    
    DestroyEditor();
  }
  NS_IF_RELEASE(mTextListener);
}

void nsTextEditorState::Unlink()
{
  nsTextEditorState* tmp = this;
  tmp->Clear();
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mSelCon)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mEditor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRootNode)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPlaceholderDiv)
}

void nsTextEditorState::Traverse(nsCycleCollectionTraversalCallback& cb)
{
  nsTextEditorState* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mSelCon, nsISelectionController)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mEditor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRootNode)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPlaceholderDiv)
}

nsFrameSelection*
nsTextEditorState::GetConstFrameSelection() {
  if (mSelCon)
    return mSelCon->GetConstFrameSelection();
  return nsnull;
}

nsIEditor*
nsTextEditorState::GetEditor()
{
  if (!mEditor) {
    nsresult rv = PrepareEditor();
    NS_ENSURE_SUCCESS(rv, nsnull);
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
    : mState(aState)
    , mOwnerContent(aOwnerContent)
    , mCurrentValue(aCurrentValue)
  {
    mState.mValueTransferInProgress = true;
  }

  NS_IMETHOD Run() {
    
    const nsAString *value = nsnull;
    if (!mCurrentValue.IsEmpty()) {
      value = &mCurrentValue;
    }

    mState.PrepareEditor(value);

    mState.mValueTransferInProgress = false;

    return NS_OK;
  }

private:
  nsTextEditorState &mState;
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
  NS_ENSURE_TRUE(mSelCon, NS_ERROR_OUT_OF_MEMORY);
  mTextListener = new nsTextInputListener(mTextCtrlElement);
  NS_ENSURE_TRUE(mTextListener, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(mTextListener);

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

    
    PRUint32 flags;
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

  
  PRUint32 editorFlags = 0;
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
  if (!mEditor) {
    shouldInitializeEditor = true;

    
    newEditor = do_CreateInstance(kTextEditorCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mBoundFrame->UpdateValueDisplay(false, true);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    if (aValue || !mEditorInitialized) {
      
      rv = mBoundFrame->UpdateValueDisplay(true, !mEditorInitialized, aValue);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    newEditor = mEditor; 
  }

  if (!mEditorInitialized) {
    
    
    
    

    
    nsCOMPtr<nsIDOMDocument> domdoc = do_QueryInterface(shell->GetDocument());
    if (!domdoc)
      return NS_ERROR_FAILURE;

    
    
    
    
    
    
    
    nsCxPusher pusher;
    pusher.PushNull();

    rv = newEditor->Init(domdoc, GetRootNode(), mSelCon, editorFlags);
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
      PRUint32 numControllers;
      bool found = false;
      rv = controllers->GetControllerCount(&numControllers);
      for (PRUint32 i = 0; i < numControllers; i ++) {
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

      
      PRInt32 maxLength;
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

  
  
  
  nsAutoString defaultValue;
  if (aValue) {
    defaultValue = *aValue;
  } else {
    GetValue(defaultValue, true);
  }

  if (shouldInitializeEditor) {
    
    mEditor = newEditor;
  }

  
  
  
  

  if (!defaultValue.IsEmpty()) {
    rv = newEditor->SetFlags(editorFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    

    rv = newEditor->EnableUndo(false);
    NS_ENSURE_SUCCESS(rv, rv);

    SetValue(defaultValue, false, false);

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

  
  if (mSelectionCached) {
    if (mRestoringSelection) 
      mRestoringSelection->Revoke();
    mRestoringSelection = new RestoreSelectionState(this, mBoundFrame);
    if (mRestoringSelection) {
      nsContentUtils::AddScriptRunner(mRestoringSelection);
    }
  }

  
  mSelectionCached = false;

  return rv;
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
}

void
nsTextEditorState::UnbindFromFrame(nsTextControlFrame* aFrame)
{
  NS_ENSURE_TRUE(mBoundFrame, );

  
  NS_ASSERTION(!aFrame || aFrame == mBoundFrame, "Unbinding from the wrong frame");
  NS_ENSURE_TRUE(!aFrame || aFrame == mBoundFrame, );

  
  
  nsAutoString value;
  GetValue(value, true);

  if (mRestoringSelection) {
    mRestoringSelection->Revoke();
    mRestoringSelection = nsnull;
  }

  
  
  
  
  
  
  if (mEditorInitialized) {
    mBoundFrame->GetSelectionRange(&mSelectionProperties.mStart,
                                   &mSelectionProperties.mEnd,
                                   &mSelectionProperties.mDirection);
    mSelectionCached = true;
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
      PRUint32 numControllers;
      nsresult rv = controllers->GetControllerCount(&numControllers);
      NS_ASSERTION((NS_SUCCEEDED(rv)), "bad result in gfx text control destructor");
      for (PRUint32 i = 0; i < numControllers; i ++)
      {
        nsCOMPtr<nsIController> controller;
        rv = controllers->GetControllerAt(i, getter_AddRefs(controller));
        if (NS_SUCCEEDED(rv) && controller)
        {
          nsCOMPtr<nsIControllerContext> editController = do_QueryInterface(controller);
          if (editController)
          {
            editController->SetCommandContext(nsnull);
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

    mSelCon->SetScrollableFrame(nsnull);
    mSelCon = nsnull;
  }

  if (mTextListener)
  {
    mTextListener->SetFrame(nsnull);

    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mTextCtrlElement);
    nsEventListenerManager* manager =
      target->GetListenerManager(false);
    if (manager) {
      manager->RemoveEventListenerByType(mTextListener,
                                         NS_LITERAL_STRING("keydown"),
                                         NS_EVENT_FLAG_BUBBLE |
                                         NS_EVENT_FLAG_SYSTEM_EVENT);
      manager->RemoveEventListenerByType(mTextListener,
                                         NS_LITERAL_STRING("keypress"),
                                         NS_EVENT_FLAG_BUBBLE |
                                         NS_EVENT_FLAG_SYSTEM_EVENT);
      manager->RemoveEventListenerByType(mTextListener,
                                         NS_LITERAL_STRING("keyup"),
                                         NS_EVENT_FLAG_BUBBLE |
                                         NS_EVENT_FLAG_SYSTEM_EVENT);
    }

    NS_RELEASE(mTextListener);
    mTextListener = nsnull;
  }

  mBoundFrame = nsnull;

  
  
  if (!mValueTransferInProgress) {
    SetValue(value, false, false);
  }

  if (mRootNode && mMutationObserver) {
    mRootNode->RemoveMutationObserver(mMutationObserver);
    mMutationObserver = nsnull;
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

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::div, nsnull,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

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
  PRInt32 wrapCols = GetWrapCols();
  if (wrapCols >= 0) {
    classValue.AppendLiteral(" wrap");
  }
  if (!IsSingleLineTextControl()) {
    
    
    
    
    const nsStyleDisplay* disp = mBoundFrame->GetStyleDisplay();
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
  nsCOMPtr<nsIContent> placeholderText;

  
  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = pNodeInfoManager->GetNodeInfo(nsGkAtoms::div, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  rv = NS_NewHTMLElement(getter_AddRefs(mPlaceholderDiv), nodeInfo.forget(),
                         NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = NS_NewTextNode(getter_AddRefs(placeholderText), pNodeInfoManager);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPlaceholderDiv->AppendChildTo(placeholderText, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  UpdatePlaceholderText(false);

  return NS_OK;
}

bool
nsTextEditorState::GetMaxLength(PRInt32* aMaxLength)
{
  nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
  NS_ENSURE_TRUE(content, false);
  nsGenericHTMLElement* element = nsGenericHTMLElement::FromContent(content);
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

    PRUint32 flags = (nsIDocumentEncoder::OutputLFLineBreak |
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
      nsCxPusher pusher;
      pusher.PushNull();

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

void
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
    if (!mEditorInitialized && IsSingleLineTextControl()) {
      
      
      NS_ASSERTION(mRootNode, "We should have a root node here");
      nsIContent *textContent = mRootNode->GetFirstChild();
      nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(textContent);
      if (textNode) {
        textNode->GetData(currentValue);
      }
    } else {
      mBoundFrame->GetText(currentValue);
    }

    nsWeakFrame weakFrame(mBoundFrame);

    
    if (!currentValue.Equals(aValue))
    {
      nsTextControlFrame::ValueSetter valueSetter(mEditor);

      
      
      
      
      nsString newValue(aValue);
      if (aValue.FindChar(PRUnichar('\r')) != -1) {
        nsContentUtils::PlatformToDOMLineBreaks(newValue);
      }

      nsCOMPtr<nsIDOMDocument> domDoc;
      mEditor->GetDocument(getter_AddRefs(domDoc));
      if (!domDoc) {
        NS_WARNING("Why don't we have a document?");
        return;
      }

      
      
      
      { 
        nsCxPusher pusher;
        pusher.PushNull();

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
        PRUint32 currentLength = currentValue.Length();
        PRUint32 newlength = newValue.Length();
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
          return;
        }

        valueSetter.Init();

        
        
        PRUint32 flags, savedFlags;
        mEditor->GetFlags(&savedFlags);
        flags = savedFlags;
        flags &= ~(nsIPlaintextEditor::eEditorDisabledMask);
        flags &= ~(nsIPlaintextEditor::eEditorReadonlyMask);
        flags |= nsIPlaintextEditor::eEditorDontEchoPassword;
        mEditor->SetFlags(flags);

        mTextListener->SettingValue(true);
        mTextListener->SetValueChanged(aSetValueChanged);

        
        PRInt32 savedMaxLength;
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
            SetValue(newValue, false, aSetValueChanged);
          }
          valueSetter.Cancel();
          return;
        }

        if (!IsSingleLineTextControl()) {
          mCachedValue = newValue;
        }

        plaintextEditor->SetMaxTextLength(savedMaxLength);
        mEditor->SetFlags(savedFlags);
        if (selPriv)
          selPriv->EndBatchChanges();
      }
    }

    
    if (!weakFrame.IsAlive()) {
      return;
    }
    nsIScrollableFrame* scrollableFrame = do_QueryFrame(mBoundFrame->GetFirstPrincipalChild());
    if (scrollableFrame)
    {
      
      
      scrollableFrame->ScrollTo(nsPoint(0, 0), nsIScrollableFrame::INSTANT);
    }

  } else {
    if (!mValue) {
      mValue = new nsCString;
    }
    nsString value(aValue);
    nsContentUtils::PlatformToDOMLineBreaks(value);
    CopyUTF16toUTF8(value, *mValue);

    
    if (mBoundFrame) {
      mBoundFrame->UpdateValueDisplay(true);
    }
  }

  
  
  ValueWasChanged(!!mRootNode);

  mTextCtrlElement->OnValueChanged(!!mRootNode);
}

void
nsTextEditorState::InitializeKeyboardEventListeners()
{
  
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mTextCtrlElement);
  nsEventListenerManager* manager = target->GetListenerManager(true);
  if (manager) {
    manager->AddEventListenerByType(mTextListener,
                                    NS_LITERAL_STRING("keydown"),
                                    NS_EVENT_FLAG_BUBBLE |
                                    NS_EVENT_FLAG_SYSTEM_EVENT);
    manager->AddEventListenerByType(mTextListener,
                                    NS_LITERAL_STRING("keypress"),
                                    NS_EVENT_FLAG_BUBBLE |
                                    NS_EVENT_FLAG_SYSTEM_EVENT);
    manager->AddEventListenerByType(mTextListener,
                                    NS_LITERAL_STRING("keyup"),
                                    NS_EVENT_FLAG_BUBBLE |
                                    NS_EVENT_FLAG_SYSTEM_EVENT);
  }

  mSelCon->SetScrollableFrame(do_QueryFrame(mBoundFrame->GetFirstPrincipalChild()));
}

 void
nsTextEditorState::ShutDown()
{
  NS_IF_RELEASE(sNativeTextAreaBindings);
  NS_IF_RELEASE(sNativeInputBindings);
}

void
nsTextEditorState::ValueWasChanged(bool aNotify)
{
  
  if (!mPlaceholderDiv) {
    return;
  }

  nsAutoString valueString;
  GetValue(valueString, true);
  SetPlaceholderClass(valueString.IsEmpty(), aNotify);
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
  ValueWasChanged(aNotify);
}

void
nsTextEditorState::SetPlaceholderClass(bool aVisible,
                                       bool aNotify)
{
  NS_ASSERTION(mPlaceholderDiv, "This function should not be called if "
                                "mPlaceholderDiv isn't set");

  
  if (!mBoundFrame)
    return;

  nsAutoString classValue;

  classValue.Assign(NS_LITERAL_STRING("anonymous-div placeholder"));

  if (!aVisible)
    classValue.AppendLiteral(" hidden");

  nsIContent* placeholderDiv = GetPlaceholderNode();
  NS_ENSURE_TRUE(placeholderDiv, );

  placeholderDiv->SetAttr(kNameSpaceID_None, nsGkAtoms::_class,
                          classValue, aNotify);
}

void
nsTextEditorState::HideSelectionIfBlurred()
{
  NS_ABORT_IF_FALSE(mSelCon, "Should have a selection controller if we have a frame!");
  nsCOMPtr<nsIContent> content = do_QueryInterface(mTextCtrlElement);
  if (!nsContentUtils::IsFocusedContent(content)) {
    mSelCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);
  }
}

NS_IMPL_ISUPPORTS1(nsAnonDivObserver, nsIMutationObserver)

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
                                   PRInt32      )
{
  mTextEditorState->ClearValueCache();
}

void
nsAnonDivObserver::ContentInserted(nsIDocument* aDocument,
                                   nsIContent*  aContainer,
                                   nsIContent*  aChild,
                                   PRInt32      )
{
  mTextEditorState->ClearValueCache();
}

void
nsAnonDivObserver::ContentRemoved(nsIDocument* aDocument,
                                  nsIContent*  aContainer,
                                  nsIContent*  aChild,
                                  PRInt32      aIndexInContainer,
                                  nsIContent*  aPreviousSibling)
{
  mTextEditorState->ClearValueCache();
}
