






































#include "nsCOMPtr.h"
#include "nsTextControlFrame.h"
#include "nsIDocument.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIFormControl.h"
#include "nsIServiceManager.h"
#include "nsFrameSelection.h"
#include "nsIPlaintextEditor.h"
#include "nsEditorCID.h"
#include "nsLayoutCID.h"
#include "nsIDocumentEncoder.h"
#include "nsCaret.h"
#include "nsISelectionListener.h"
#include "nsISelectionPrivate.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIControllerContext.h"
#include "nsGenericHTMLElement.h"
#include "nsIEditorIMESupport.h"
#include "nsIPhonetic.h"
#include "nsIEditorObserver.h"
#include "nsEditProperty.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsINameSpaceManager.h"
#include "nsINodeInfo.h"
#include "nsFormControlFrame.h" 
#include "nsIDeviceContext.h" 

#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsIComponentManager.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocument.h"
#include "nsIPresShell.h"
#include "nsIComponentManager.h"

#include "nsBoxLayoutState.h"

#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDocument.h" 
#include "nsIStyleSheet.h"
#include "nsIStyleRule.h"
#include "nsIDOMEventListener.h"
#include "nsGUIEvent.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIEventStateManager.h"

#include "nsIDOMFocusListener.h" 
#include "nsIDOMCharacterData.h" 
#include "nsIDOMNodeList.h" 
#include "nsIDOMRange.h" 
#include "nsPIDOMWindow.h" 
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"

#include "nsIEditorObserver.h"
#include "nsITransactionManager.h"
#include "nsIDOMText.h" 
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsIDOMKeyListener.h"
#include "nsIDOMEventGroup.h"
#include "nsIDOM3EventTarget.h"
#include "nsINativeKeyBindings.h"
#include "nsIJSContextStack.h"
#include "nsFocusManager.h"
#include "nsTextEditRules.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsPresState.h"

#include "mozilla/FunctionTimer.h"

#define DEFAULT_COLUMN_WIDTH 20

#include "nsContentCID.h"
static NS_DEFINE_IID(kRangeCID,     NS_RANGE_CID);

nsIFrame*
NS_NewTextControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTextControlFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTextControlFrame)

NS_QUERYFRAME_HEAD(nsTextControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsITextControlFrame)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)

#ifdef ACCESSIBILITY
already_AddRefed<nsAccessible>
nsTextControlFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    return accService->CreateHTMLTextFieldAccessible(mContent,
                                                     PresContext()->PresShell());
  }

  return nsnull;
}
#endif

#ifdef DEBUG
class EditorInitializerEntryTracker {
public:
  explicit EditorInitializerEntryTracker(nsTextControlFrame &frame)
    : mFrame(frame)
    , mFirstEntry(PR_FALSE)
  {
    if (!mFrame.mInEditorInitialization) {
      mFrame.mInEditorInitialization = PR_TRUE;
      mFirstEntry = PR_TRUE;
    }
  }
  ~EditorInitializerEntryTracker()
  {
    if (mFirstEntry) {
      mFrame.mInEditorInitialization = PR_FALSE;
    }
  }
  PRBool EnteredMoreThanOnce() const { return !mFirstEntry; }
private:
  nsTextControlFrame &mFrame;
  PRBool mFirstEntry;
};
#endif

nsTextControlFrame::nsTextControlFrame(nsIPresShell* aShell, nsStyleContext* aContext)
  : nsStackFrame(aShell, aContext)
  , mUseEditor(PR_FALSE)
  , mIsProcessing(PR_FALSE)
  , mNotifyOnInput(PR_TRUE)
  , mFireChangeEventState(PR_FALSE)
#ifdef DEBUG
  , mInEditorInitialization(PR_FALSE)
#endif
{
}

nsTextControlFrame::~nsTextControlFrame()
{
}

void
nsTextControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mScrollEvent.Revoke();

  
  
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  txtCtrl->UnbindFromFrame(this);

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_FALSE);

  nsBoxFrame::DestroyFrom(aDestructRoot);
}

nsIAtom*
nsTextControlFrame::GetType() const 
{ 
  return nsGkAtoms::textInputFrame;
}

nsresult
nsTextControlFrame::CalcIntrinsicSize(nsRenderingContext* aRenderingContext,
                                      nsSize&              aIntrinsicSize)
{
  
  nscoord lineHeight  = 0;
  nscoord charWidth   = 0;
  nscoord charMaxAdvance  = 0;

  nsRefPtr<nsFontMetrics> fontMet;
  nsresult rv =
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));
  NS_ENSURE_SUCCESS(rv, rv);
  aRenderingContext->SetFont(fontMet);

  lineHeight =
    nsHTMLReflowState::CalcLineHeight(GetStyleContext(), NS_AUTOHEIGHT);
  charWidth = fontMet->AveCharWidth();
  charMaxAdvance = fontMet->MaxAdvance();

  
  PRInt32 cols = GetCols();
  aIntrinsicSize.width = cols * charWidth;

  
  
  
  
  if (charWidth != charMaxAdvance) {
    nscoord internalPadding = NS_MAX(0, charMaxAdvance -
                                        nsPresContext::CSSPixelsToAppUnits(4));
    nscoord t = nsPresContext::CSSPixelsToAppUnits(1); 
   
    nscoord rest = internalPadding % t; 
    if (rest < t - rest) {
      internalPadding -= rest;
    } else {
      internalPadding += t - rest;
    }
    
    aIntrinsicSize.width += internalPadding;
  } else {
    
    
    if (PresContext()->CompatibilityMode() == eCompatibility_FullStandards) {
      aIntrinsicSize.width += 1;
    }

    
    
    
    nsMargin childPadding;
    nsIFrame* firstChild = GetFirstChild(nsnull);
    if (firstChild && firstChild->GetStylePadding()->GetPadding(childPadding)) {
      aIntrinsicSize.width += childPadding.LeftRight();
    } else {
      NS_ERROR("Percentage padding on value div?");
    }
  }

  
  {
    const nsStyleCoord& lsCoord = GetStyleText()->mLetterSpacing;
    if (eStyleUnit_Coord == lsCoord.GetUnit()) {
      nscoord letterSpacing = lsCoord.GetCoordValue();
      if (letterSpacing != 0) {
        aIntrinsicSize.width += cols * letterSpacing;
      }
    }
  }

  
  
  aIntrinsicSize.height = lineHeight * GetRows();

  
  if (IsTextArea()) {
    nsIFrame* first = GetFirstChild(nsnull);

    nsIScrollableFrame *scrollableFrame = do_QueryFrame(first);
    NS_ASSERTION(scrollableFrame, "Child must be scrollable");

    if (scrollableFrame) {
      nsMargin scrollbarSizes =
      scrollableFrame->GetDesiredScrollbarSizes(PresContext(), aRenderingContext);

      aIntrinsicSize.width  += scrollbarSizes.LeftRight();

      aIntrinsicSize.height += scrollbarSizes.TopBottom();;
    }
  }

  return NS_OK;
}

nsresult
nsTextControlFrame::EnsureEditorInitialized()
{
  

  
  
  
  
  
  
  
  
  

  
  
  if (mUseEditor)
    return NS_OK;

  NS_TIME_FUNCTION;

  nsIDocument* doc = mContent->GetCurrentDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsWeakFrame weakFrame(this);

  
  
  
  doc->FlushPendingNotifications(Flush_ContentAndNotify);
  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_ERROR_FAILURE);

  
  
  nsAutoScriptBlocker scriptBlocker;

  
  
  nsCxPusher pusher;
  pusher.PushNull();

  
  class EnsureSetFocus {
  public:
    explicit EnsureSetFocus(nsTextControlFrame* aFrame)
      : mFrame(aFrame) {}
    ~EnsureSetFocus() {
      if (nsContentUtils::IsFocusedContent(mFrame->GetContent()))
        mFrame->SetFocus(PR_TRUE, PR_FALSE);
    }
  private:
    nsTextControlFrame *mFrame;
  };
  EnsureSetFocus makeSureSetFocusHappens(this);

#ifdef DEBUG
  
  
  const EditorInitializerEntryTracker tracker(*this);
  NS_ASSERTION(!tracker.EnteredMoreThanOnce(),
               "EnsureEditorInitialized has been called while a previous call was in progress");
#endif

  
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsresult rv = txtCtrl->CreateEditor();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mUseEditor = PR_TRUE;

  return NS_OK;
}

nsresult
nsTextControlFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  NS_ASSERTION(mContent, "We should have a content!");

  mState |= NS_FRAME_INDEPENDENT_SELECTION;

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  
  nsresult rv = txtCtrl->BindToFrame(this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIContent* rootNode = txtCtrl->GetRootEditorNode();
  NS_ENSURE_TRUE(rootNode, NS_ERROR_OUT_OF_MEMORY);

  if (!aElements.AppendElement(rootNode))
    return NS_ERROR_OUT_OF_MEMORY;

  
  nsAutoString placeholderTxt;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::placeholder,
                    placeholderTxt);
  nsContentUtils::RemoveNewlines(placeholderTxt);
  mUsePlaceholder = !placeholderTxt.IsEmpty();

  
  if (mUsePlaceholder) {
    nsIContent* placeholderNode = txtCtrl->CreatePlaceholderNode();
    NS_ENSURE_TRUE(placeholderNode, NS_ERROR_OUT_OF_MEMORY);

    if (!aElements.AppendElement(placeholderNode))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = UpdateValueDisplay(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool initEagerly = !IsSingleLineTextControl();
  if (!initEagerly) {
    nsCOMPtr<nsIDOMNSHTMLElement> element = do_QueryInterface(txtCtrl);
    if (element) {
      
      element->GetSpellcheck(&initEagerly);
    }
  }

  if (initEagerly) {
    NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                 "Someone forgot a script blocker?");

    if (!nsContentUtils::AddScriptRunner(new EditorInitializer(this))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
}

void
nsTextControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                             PRUint32 aFilter)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  aElements.MaybeAppendElement(txtCtrl->GetRootEditorNode());
  if (!(aFilter & nsIContent::eSkipPlaceholderContent))
    aElements.MaybeAppendElement(txtCtrl->GetPlaceholderNode());
}

nscoord
nsTextControlFrame::GetMinWidth(nsRenderingContext* aRenderingContext)
{
  
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetPrefWidth(aRenderingContext);

  return result;
}

nsSize
nsTextControlFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                    nsSize aCBSize, nscoord aAvailableWidth,
                                    nsSize aMargin, nsSize aBorder,
                                    nsSize aPadding, PRBool aShrinkWrap)
{
  nsSize autoSize;
  nsresult rv = CalcIntrinsicSize(aRenderingContext, autoSize);
  if (NS_FAILED(rv)) {
    
    autoSize.SizeTo(0, 0);
  }
#ifdef DEBUG
  
  else if (GetStylePosition()->mWidth.GetUnit() == eStyleUnit_Auto) {
    nsSize ancestorAutoSize =
      nsStackFrame::ComputeAutoSize(aRenderingContext,
                                    aCBSize, aAvailableWidth,
                                    aMargin, aBorder,
                                    aPadding, aShrinkWrap);
    NS_ASSERTION(ancestorAutoSize.width == autoSize.width,
                 "Incorrect size computed by ComputeAutoSize?");
  }
#endif
  
  return autoSize;
}




NS_IMETHODIMP
nsTextControlFrame::Reflow(nsPresContext*   aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTextControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  
  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, PR_TRUE);
  }

  return nsStackFrame::Reflow(aPresContext, aDesiredSize, aReflowState,
                              aStatus);
}

nsSize
nsTextControlFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  if (!DoesNeedRecalc(mPrefSize))
     return mPrefSize;

#ifdef DEBUG_LAYOUT
  PropagateDebug(aState);
#endif

  nsSize pref(0,0);

  nsresult rv = CalcIntrinsicSize(aState.GetRenderingContext(), pref);
  NS_ENSURE_SUCCESS(rv, pref);
  AddBorderAndPadding(pref);

  PRBool widthSet, heightSet;
  nsIBox::AddCSSPrefSize(this, pref, widthSet, heightSet);

  nsSize minSize = GetMinSize(aState);
  nsSize maxSize = GetMaxSize(aState);
  mPrefSize = BoundsCheck(minSize, pref, maxSize);

#ifdef DEBUG_rods
  {
    nsMargin borderPadding(0,0,0,0);
    GetBorderAndPadding(borderPadding);
    nsSize size(169, 24);
    nsSize actual(pref.width/15, 
                  pref.height/15);
    printf("nsGfxText(field) %d,%d  %d,%d  %d,%d\n", 
           size.width, size.height, actual.width, actual.height, actual.width-size.width, actual.height-size.height);  
  }
#endif

  return mPrefSize;
}

nsSize
nsTextControlFrame::GetMinSize(nsBoxLayoutState& aState)
{
  
  return nsBox::GetMinSize(aState);
}

nsSize
nsTextControlFrame::GetMaxSize(nsBoxLayoutState& aState)
{
  
  return nsBox::GetMaxSize(aState);
}

nscoord
nsTextControlFrame::GetBoxAscent(nsBoxLayoutState& aState)
{
  
  

  
  nsRect clientRect;
  GetClientRect(clientRect);
  nscoord lineHeight =
    IsSingleLineTextControl() ? clientRect.height :
    nsHTMLReflowState::CalcLineHeight(GetStyleContext(), NS_AUTOHEIGHT);

  nsRefPtr<nsFontMetrics> fontMet;
  nsresult rv =
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));
  NS_ENSURE_SUCCESS(rv, 0);

  nscoord ascent = nsLayoutUtils::GetCenteredFontBaseline(fontMet, lineHeight);

  
  ascent += clientRect.y;

  return ascent;
}

PRBool
nsTextControlFrame::IsCollapsed(nsBoxLayoutState& aBoxLayoutState)
{
  
  return PR_FALSE;
}

PRBool
nsTextControlFrame::IsLeaf() const
{
  return PR_TRUE;
}

NS_IMETHODIMP
nsTextControlFrame::ScrollOnFocusEvent::Run()
{
  if (mFrame) {
    nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(mFrame->GetContent());
    NS_ASSERTION(txtCtrl, "Content not a text control element");
    nsISelectionController* selCon = txtCtrl->GetSelectionController();
    if (selCon) {
      mFrame->mScrollEvent.Forget();
      selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
                                      nsISelectionController::SELECTION_FOCUS_REGION,
                                      nsISelectionController::SCROLL_SYNCHRONOUS);
    }
  }
  return NS_OK;
}


void nsTextControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  
  mScrollEvent.Revoke();

  if (!aOn) {
    if (mUsePlaceholder) {
      PRInt32 textLength;
      GetTextLength(&textLength);

      if (!textLength) {
        nsWeakFrame weakFrame(this);

        txtCtrl->SetPlaceholderClass(PR_TRUE, PR_TRUE);

        if (!weakFrame.IsAlive()) {
          return;
        }
      }
    }

    return;
  }

  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  if (!selCon)
    return;

  if (mUsePlaceholder) {
    nsWeakFrame weakFrame(this);

    txtCtrl->SetPlaceholderClass(PR_FALSE, PR_TRUE);

    if (!weakFrame.IsAlive()) {
      return;
    }
  }

  InitFocusedValue();

  nsCOMPtr<nsISelection> ourSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, 
    getter_AddRefs(ourSel));
  if (!ourSel) return;

  nsIPresShell* presShell = PresContext()->GetPresShell();
  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) return;

  
  nsISelection *caretSelection = caret->GetCaretDOMSelection();
  const PRBool isFocusedRightNow = ourSel == caretSelection;
  if (!isFocusedRightNow) {
    
    PRUint32 lastFocusMethod = 0;
    nsIDocument* doc = GetContent()->GetCurrentDoc();
    if (doc) {
      nsIFocusManager* fm = nsFocusManager::GetFocusManager();
      if (fm) {
        fm->GetLastFocusMethod(doc->GetWindow(), &lastFocusMethod);
      }
    }
    if (!(lastFocusMethod & nsIFocusManager::FLAG_BYMOUSE)) {
      nsRefPtr<ScrollOnFocusEvent> event = new ScrollOnFocusEvent(this);
      nsresult rv = NS_DispatchToCurrentThread(event);
      if (NS_SUCCEEDED(rv)) {
        mScrollEvent = event;
      }
    }
  }

  
  caret->SetCaretDOMSelection(ourSel);

  
  
  

  nsCOMPtr<nsISelectionController> selcon = do_QueryInterface(presShell);
  nsCOMPtr<nsISelection> docSel;
  selcon->GetSelection(nsISelectionController::SELECTION_NORMAL,
    getter_AddRefs(docSel));
  if (!docSel) return;

  PRBool isCollapsed = PR_FALSE;
  docSel->GetIsCollapsed(&isCollapsed);
  if (!isCollapsed)
    docSel->RemoveAllRanges();
}

nsresult nsTextControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  if (!mIsProcessing)
  {
    mIsProcessing = PR_TRUE;
    if (nsGkAtoms::select == aName)
    {
      
      
      
      
      
      
      

      nsWeakFrame weakThis = this;
      SelectAllOrCollapseToEndOfText(PR_TRUE);  
      if (!weakThis.IsAlive()) {
        return NS_OK;
      }
    }
    mIsProcessing = PR_FALSE;
  }
  return NS_OK;
}

nsresult
nsTextControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  NS_ASSERTION(nsGkAtoms::value != aName,
               "Should get the value from the content node instead");
  return NS_OK;
}



NS_IMETHODIMP
nsTextControlFrame::GetEditor(nsIEditor **aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  *aEditor = txtCtrl->GetTextEditor();
  NS_IF_ADDREF(*aEditor);
  return NS_OK;
}

NS_IMETHODIMP
nsTextControlFrame::GetTextLength(PRInt32* aTextLength)
{
  NS_ENSURE_ARG_POINTER(aTextLength);

  nsAutoString   textContents;
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  txtCtrl->GetTextEditorValue(textContents, PR_FALSE);   
  *aTextLength = textContents.Length();
  return NS_OK;
}

nsresult
nsTextControlFrame::SetSelectionInternal(nsIDOMNode *aStartNode,
                                         PRInt32 aStartOffset,
                                         nsIDOMNode *aEndNode,
                                         PRInt32 aEndOffset)
{
  
  
  

  nsCOMPtr<nsIDOMRange> range = do_CreateInstance(kRangeCID);
  NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);

  nsresult rv = range->SetStart(aStartNode, aStartOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = range->SetEnd(aEndNode, aEndOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);

  nsCOMPtr<nsISelection> selection;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));  
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  rv = selection->RemoveAllRanges();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selection->AddRange(range);  
  return rv;
}

nsresult
nsTextControlFrame::ScrollSelectionIntoView()
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  if (selCon) {
    
    return selCon->ScrollSelectionIntoView(nsISelectionController::SELECTION_NORMAL,
                                           nsISelectionController::SELECTION_FOCUS_REGION,
                                           nsISelectionController::SCROLL_FIRST_ANCESTOR_ONLY);
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsTextControlFrame::GetRootNodeAndInitializeEditor(nsIDOMElement **aRootElement)
{
  NS_ENSURE_ARG_POINTER(aRootElement);

  nsCOMPtr<nsIEditor> editor;
  GetEditor(getter_AddRefs(editor));
  if (!editor)
    return NS_OK;

  return editor->GetRootElement(aRootElement);
}

nsresult
nsTextControlFrame::SelectAllOrCollapseToEndOfText(PRBool aSelect)
{
  nsCOMPtr<nsIDOMElement> rootElement;
  nsresult rv = GetRootNodeAndInitializeEditor(getter_AddRefs(rootElement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> rootContent = do_QueryInterface(rootElement);
  nsCOMPtr<nsIDOMNode> rootNode(do_QueryInterface(rootElement));

  NS_ENSURE_TRUE(rootNode && rootContent, NS_ERROR_FAILURE);

  PRInt32 numChildren = rootContent->GetChildCount();

  if (numChildren > 0) {
    
    
    nsIContent *child = rootContent->GetChildAt(numChildren - 1);
    if (child) {
      if (child->Tag() == nsGkAtoms::br)
        --numChildren;
    }
    if (!aSelect && numChildren) {
      child = rootContent->GetChildAt(numChildren - 1);
      if (child && child->IsNodeOfType(nsINode::eTEXT)) {
        rootNode = do_QueryInterface(child);
        const nsTextFragment* fragment = child->GetText();
        numChildren = fragment ? fragment->GetLength() : 0;
      }
    }
  }

  rv = SetSelectionInternal(rootNode, aSelect ? 0 : numChildren,
                            rootNode, numChildren);
  NS_ENSURE_SUCCESS(rv, rv);

  return ScrollSelectionIntoView();
}

nsresult
nsTextControlFrame::SetSelectionEndPoints(PRInt32 aSelStart, PRInt32 aSelEnd)
{
  NS_ASSERTION(aSelStart <= aSelEnd, "Invalid selection offsets!");

  if (aSelStart > aSelEnd)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startOffset, endOffset;

  

  nsresult rv = OffsetToDOMPoint(aSelStart, getter_AddRefs(startNode), &startOffset);

  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelStart == aSelEnd) {
    
    endNode   = startNode;
    endOffset = startOffset;
  }
  else {
    
    

    rv = OffsetToDOMPoint(aSelEnd, getter_AddRefs(endNode), &endOffset);

    NS_ENSURE_SUCCESS(rv, rv);
  }

  return SetSelectionInternal(startNode, startOffset, endNode, endOffset);
}

NS_IMETHODIMP
nsTextControlFrame::SetSelectionRange(PRInt32 aSelStart, PRInt32 aSelEnd)
{
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelStart > aSelEnd) {
    
    

    aSelStart   = aSelEnd;
  }

  return SetSelectionEndPoints(aSelStart, aSelEnd);
}


NS_IMETHODIMP
nsTextControlFrame::SetSelectionStart(PRInt32 aSelectionStart)
{
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 selStart = 0, selEnd = 0; 

  rv = GetSelectionRange(&selStart, &selEnd);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelectionStart > selEnd) {
    
    selEnd = aSelectionStart; 
  }

  selStart = aSelectionStart;
  
  return SetSelectionEndPoints(selStart, selEnd);
}

NS_IMETHODIMP
nsTextControlFrame::SetSelectionEnd(PRInt32 aSelectionEnd)
{
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 selStart = 0, selEnd = 0; 

  rv = GetSelectionRange(&selStart, &selEnd);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelectionEnd < selStart) {
    
    selStart = aSelectionEnd; 
  }

  selEnd = aSelectionEnd;
  
  return SetSelectionEndPoints(selStart, selEnd);
}

nsresult
nsTextControlFrame::DOMPointToOffset(nsIDOMNode* aNode,
                                     PRInt32 aNodeOffset,
                                     PRInt32* aResult)
{
  NS_ENSURE_ARG_POINTER(aNode && aResult);

  *aResult = 0;

  nsCOMPtr<nsIDOMElement> rootElement;
  nsresult rv = GetRootNodeAndInitializeEditor(getter_AddRefs(rootElement));
  nsCOMPtr<nsIDOMNode> rootNode(do_QueryInterface(rootElement));

  NS_ENSURE_TRUE(rootNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNodeList> nodeList;

  rv = rootNode->GetChildNodes(getter_AddRefs(nodeList));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

  PRUint32 length = 0;
  rv = nodeList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!length || aNodeOffset < 0)
    return NS_OK;

  NS_ASSERTION(length <= 2, "We should have one text node and one mozBR at most");

  nsCOMPtr<nsIDOMNode> firstNode;
  rv = nodeList->Item(0, getter_AddRefs(firstNode));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(firstNode);

  nsCOMPtr<nsIDOMText> nodeAsText = do_QueryInterface(aNode);
  if (nodeAsText || (aNode == rootNode && aNodeOffset == 0)) {
    
    *aResult = aNodeOffset;
  } else {
    
    
    if (textNode) {
      rv = textNode->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);
      *aResult = PRInt32(length);
    }
  }

  return NS_OK;
}

nsresult
nsTextControlFrame::OffsetToDOMPoint(PRInt32 aOffset,
                                     nsIDOMNode** aResult,
                                     PRInt32* aPosition)
{
  NS_ENSURE_ARG_POINTER(aResult && aPosition);

  *aResult = nsnull;
  *aPosition = 0;

  nsCOMPtr<nsIDOMElement> rootElement;
  nsresult rv = GetRootNodeAndInitializeEditor(getter_AddRefs(rootElement));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMNode> rootNode(do_QueryInterface(rootElement));

  NS_ENSURE_TRUE(rootNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNodeList> nodeList;

  rv = rootNode->GetChildNodes(getter_AddRefs(nodeList));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(nodeList, NS_ERROR_FAILURE);

  PRUint32 length = 0;

  rv = nodeList->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(length <= 2, "We should have one text node and one mozBR at most");

  nsCOMPtr<nsIDOMNode> firstNode;
  rv = nodeList->Item(0, getter_AddRefs(firstNode));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(firstNode);

  if (length == 0 || aOffset < 0) {
    NS_IF_ADDREF(*aResult = rootNode);
    *aPosition = 0;
  } else if (textNode) {
    PRUint32 textLength = 0;
    textNode->GetLength(&textLength);
    if (length == 2 && PRUint32(aOffset) == textLength) {
      
      
      NS_IF_ADDREF(*aResult = rootNode);
      *aPosition = 1;
    } else {
      
      NS_IF_ADDREF(*aResult = firstNode);
      *aPosition = NS_MIN(aOffset, PRInt32(textLength));
    }
  } else {
    NS_IF_ADDREF(*aResult = rootNode);
    *aPosition = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTextControlFrame::GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd)
{
  
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  *aSelectionStart = 0;
  *aSelectionEnd = 0;

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
  nsCOMPtr<nsISelection> selection;
  rv = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));  
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  PRInt32 numRanges = 0;
  selection->GetRangeCount(&numRanges);

  if (numRanges < 1)
    return NS_OK;

  

  nsCOMPtr<nsIDOMRange> firstRange;
  rv = selection->GetRangeAt(0, getter_AddRefs(firstRange));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(firstRange, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  PRInt32 startOffset = 0, endOffset = 0;

  

  rv = firstRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);

  rv = firstRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  

  rv = firstRange->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(endNode, NS_ERROR_FAILURE);

  rv = firstRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  

  rv = DOMPointToOffset(startNode, startOffset, aSelectionStart);
  NS_ENSURE_SUCCESS(rv, rv);

  

  return DOMPointToOffset(endNode, endOffset, aSelectionEnd);
}




NS_IMETHODIMP
nsTextControlFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  const PRBool needEditor = nsGkAtoms::maxlength == aAttribute ||
                            nsGkAtoms::readonly == aAttribute ||
                            nsGkAtoms::disabled == aAttribute ||
                            nsGkAtoms::spellcheck == aAttribute;
  nsCOMPtr<nsIEditor> editor;
  if (needEditor) {
    GetEditor(getter_AddRefs(editor));
  }
  if ((needEditor && !editor) || !selCon)
    return nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);;

  nsresult rv = NS_OK;

  if (nsGkAtoms::maxlength == aAttribute) 
  {
    PRInt32 maxLength;
    PRBool maxDefined = GetMaxLength(&maxLength);
    
    nsCOMPtr<nsIPlaintextEditor> textEditor = do_QueryInterface(editor);
    if (textEditor)
    {
      if (maxDefined) 
      {  
          textEditor->SetMaxTextLength(maxLength);
        
      }
      else { 
          textEditor->SetMaxTextLength(-1);
      }
    }
    rv = NS_OK; 
  } 
  else if (nsGkAtoms::readonly == aAttribute) 
  {
    PRUint32 flags;
    editor->GetFlags(&flags);
    if (AttributeExists(nsGkAtoms::readonly))
    { 
      flags |= nsIPlaintextEditor::eEditorReadonlyMask;
      if (nsContentUtils::IsFocusedContent(mContent))
        selCon->SetCaretEnabled(PR_FALSE);
    }
    else 
    { 
      flags &= ~(nsIPlaintextEditor::eEditorReadonlyMask);
      if (!(flags & nsIPlaintextEditor::eEditorDisabledMask) &&
          nsContentUtils::IsFocusedContent(mContent))
        selCon->SetCaretEnabled(PR_TRUE);
    }
    editor->SetFlags(flags);
  }
  else if (nsGkAtoms::disabled == aAttribute) 
  {
    PRUint32 flags;
    editor->GetFlags(&flags);
    if (AttributeExists(nsGkAtoms::disabled))
    { 
      flags |= nsIPlaintextEditor::eEditorDisabledMask;
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_OFF);
      if (nsContentUtils::IsFocusedContent(mContent))
        selCon->SetCaretEnabled(PR_FALSE);
    }
    else 
    { 
      flags &= ~(nsIPlaintextEditor::eEditorDisabledMask);
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_HIDDEN);
      if (nsContentUtils::IsFocusedContent(mContent)) {
        selCon->SetCaretEnabled(PR_TRUE);
      }
    }
    editor->SetFlags(flags);
  }
  else if (!mUseEditor && nsGkAtoms::value == aAttribute) {
    UpdateValueDisplay(PR_TRUE);
  }
  
  
  else {
    rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
  }

  return rv;
}


nsresult
nsTextControlFrame::GetText(nsString& aText)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  if (IsSingleLineTextControl()) {
    
    txtCtrl->GetTextEditorValue(aText, PR_TRUE);
  } else {
    nsCOMPtr<nsIDOMHTMLTextAreaElement> textArea = do_QueryInterface(mContent);
    if (textArea) {
      rv = textArea->GetValue(aText);
    }
  }
  return rv;
}


nsresult
nsTextControlFrame::GetPhonetic(nsAString& aPhonetic)
{
  aPhonetic.Truncate(0); 

  nsCOMPtr<nsIEditor> editor;
  nsresult rv = GetEditor(getter_AddRefs(editor));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIEditorIMESupport> imeSupport = do_QueryInterface(editor);
  if (imeSupport) {
    nsCOMPtr<nsIPhonetic> phonetic = do_QueryInterface(imeSupport);
    if (phonetic)
      phonetic->GetPhonetic(aPhonetic);
  }
  return NS_OK;
}




PRBool
nsTextControlFrame::GetMaxLength(PRInt32* aSize)
{
  *aSize = -1;

  nsGenericHTMLElement *content = nsGenericHTMLElement::FromContent(mContent);
  if (content) {
    const nsAttrValue* attr = content->GetParsedAttr(nsGkAtoms::maxlength);
    if (attr && attr->Type() == nsAttrValue::eInteger) {
      *aSize = attr->GetIntegerValue();

      return PR_TRUE;
    }
  }
  return PR_FALSE;
}


void
nsTextControlFrame::FireOnInput(PRBool aTrusted)
{
  if (!mNotifyOnInput)
    return; 
  
  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsUIEvent event(aTrusted, NS_FORM_INPUT, 0);

  
  
  nsCOMPtr<nsIPresShell> shell = PresContext()->PresShell();
  shell->HandleEventWithTarget(&event, nsnull, mContent, &status);
}

nsresult
nsTextControlFrame::InitFocusedValue()
{
  return GetText(mFocusedValue);
}

NS_IMETHODIMP
nsTextControlFrame::CheckFireOnChange()
{
  nsString value;
  GetText(value);
  if (!mFocusedValue.Equals(value))
  {
    mFocusedValue = value;
    
    nsContentUtils::DispatchTrustedEvent(mContent->GetOwnerDoc(), mContent,
                                         NS_LITERAL_STRING("change"), PR_TRUE,
                                         PR_FALSE);
  }
  return NS_OK;
}



NS_IMETHODIMP
nsTextControlFrame::SetInitialChildList(nsIAtom*        aListName,
                                        nsFrameList&    aChildList)
{
  nsresult rv = nsBoxFrame::SetInitialChildList(aListName, aChildList);

  nsIFrame* first = GetFirstChild(nsnull);

  
  
  
  if (first) {
    first->AddStateBits(NS_FRAME_REFLOW_ROOT);

    nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
    NS_ASSERTION(txtCtrl, "Content not a text control element");
    txtCtrl->InitializeKeyboardEventListeners();

    nsPoint* contentScrollPos = static_cast<nsPoint*>
      (Properties().Get(ContentScrollPos()));
    if (contentScrollPos) {
      
      
      nsIStatefulFrame* statefulFrame = do_QueryFrame(first);
      NS_ASSERTION(statefulFrame, "unexpected type of frame for the anonymous div");
      nsPresState fakePresState;
      fakePresState.SetScrollState(*contentScrollPos);
      statefulFrame->RestoreState(&fakePresState);
      Properties().Remove(ContentScrollPos());
      delete contentScrollPos;
    }
  }
  return rv;
}

PRBool
nsTextControlFrame::IsScrollable() const
{
  return !IsSingleLineTextControl();
}

void
nsTextControlFrame::SetValueChanged(PRBool aValueChanged)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  if (mUsePlaceholder && !nsContentUtils::IsFocusedContent(mContent)) {
    
    
    PRInt32 textLength;
    GetTextLength(&textLength);

    nsWeakFrame weakFrame(this);
    txtCtrl->SetPlaceholderClass(!textLength, PR_TRUE);
    if (!weakFrame.IsAlive()) {
      return;
    }
  }

  txtCtrl->SetValueChanged(aValueChanged);
}


nsresult
nsTextControlFrame::UpdateValueDisplay(PRBool aNotify,
                                       PRBool aBeforeEditorInit,
                                       const nsAString *aValue)
{
  if (!IsSingleLineTextControl()) 
    return NS_OK;

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsIContent* rootNode = txtCtrl->GetRootEditorNode();

  NS_PRECONDITION(rootNode, "Must have a div content\n");
  NS_PRECONDITION(!mUseEditor,
                  "Do not call this after editor has been initialized");
  NS_ASSERTION(!mUsePlaceholder || txtCtrl->GetPlaceholderNode(),
               "A placeholder div must exist");

  nsIContent *textContent = rootNode->GetChildAt(0);
  if (!textContent) {
    
    nsCOMPtr<nsIContent> textNode;
    nsresult rv = NS_NewTextNode(getter_AddRefs(textNode),
                                 mContent->NodeInfo()->NodeInfoManager());
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(textNode, "Must have textcontent!\n");

    rootNode->AppendChildTo(textNode, aNotify);
    textContent = textNode;
  }

  NS_ENSURE_TRUE(textContent, NS_ERROR_UNEXPECTED);

  
  nsAutoString value;
  if (aValue) {
    value = *aValue;
  } else {
    txtCtrl->GetTextEditorValue(value, PR_TRUE);
  }

  
  
  
  if (mUsePlaceholder && !aBeforeEditorInit)
  {
    nsWeakFrame weakFrame(this);
    txtCtrl->SetPlaceholderClass(value.IsEmpty(), aNotify);
    NS_ENSURE_STATE(weakFrame.IsAlive());
  }

  if (aBeforeEditorInit && value.IsEmpty()) {
    rootNode->RemoveChildAt(0, PR_TRUE, PR_FALSE);
    return NS_OK;
  }

  if (!value.IsEmpty() && IsPasswordTextControl()) {
    nsTextEditRules::FillBufWithPWChars(&value, value.Length());
  }
  return textContent->SetText(value, aNotify);
}

NS_IMETHODIMP
nsTextControlFrame::GetOwnedSelectionController(nsISelectionController** aSelCon)
{
  NS_ENSURE_ARG_POINTER(aSelCon);

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  *aSelCon = txtCtrl->GetSelectionController();
  NS_IF_ADDREF(*aSelCon);

  return NS_OK;
}

nsFrameSelection*
nsTextControlFrame::GetOwnedFrameSelection()
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  return txtCtrl->GetConstFrameSelection();
}

NS_IMETHODIMP
nsTextControlFrame::SaveState(nsIStatefulFrame::SpecialStateID aStateID, nsPresState** aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  *aState = nsnull;

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  nsIContent* rootNode = txtCtrl->GetRootEditorNode();
  if (rootNode) {
    
    nsIStatefulFrame* scrollStateFrame = do_QueryFrame(rootNode->GetPrimaryFrame());
    if (scrollStateFrame) {
      return scrollStateFrame->SaveState(aStateID, aState);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTextControlFrame::RestoreState(nsPresState* aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  nsIContent* rootNode = txtCtrl->GetRootEditorNode();
  if (rootNode) {
    
    nsIStatefulFrame* scrollStateFrame = do_QueryFrame(rootNode->GetPrimaryFrame());
    if (scrollStateFrame) {
      return scrollStateFrame->RestoreState(aState);
    }
  }

  
  
  
  Properties().Set(ContentScrollPos(), new nsPoint(aState->GetScrollState()));
  return NS_OK;
}

