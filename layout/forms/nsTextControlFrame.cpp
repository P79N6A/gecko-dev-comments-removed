




#include "mozilla/DebugOnly.h"

#include "nsCOMPtr.h"
#include "nsTextControlFrame.h"
#include "nsIPlaintextEditor.h"
#include "nsCaret.h"
#include "nsGenericHTMLElement.h"
#include "nsIEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsIPhonetic.h"
#include "nsTextFragment.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsNameSpaceManager.h"
#include "nsFormControlFrame.h" 

#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsIPresShell.h"

#include <algorithm>
#include "nsIDOMNodeList.h" 
#include "nsIDOMRange.h" 
#include "nsPIDOMWindow.h" 
#include "nsIDOMNode.h"

#include "nsIDOMText.h" 
#include "nsFocusManager.h"
#include "nsTextEditRules.h"
#include "nsPresState.h"
#include "nsContentList.h"
#include "nsAttrValueInlines.h"
#include "mozilla/dom/Selection.h"
#include "nsContentUtils.h"
#include "nsTextNode.h"
#include "nsStyleSet.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/MathAlgorithms.h"

#define DEFAULT_COLUMN_WIDTH 20

using namespace mozilla;

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
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef ACCESSIBILITY
a11y::AccType
nsTextControlFrame::AccessibleType()
{
  return a11y::eHTMLTextFieldType;
}
#endif

#ifdef DEBUG
class EditorInitializerEntryTracker {
public:
  explicit EditorInitializerEntryTracker(nsTextControlFrame &frame)
    : mFrame(frame)
    , mFirstEntry(false)
  {
    if (!mFrame.mInEditorInitialization) {
      mFrame.mInEditorInitialization = true;
      mFirstEntry = true;
    }
  }
  ~EditorInitializerEntryTracker()
  {
    if (mFirstEntry) {
      mFrame.mInEditorInitialization = false;
    }
  }
  bool EnteredMoreThanOnce() const { return !mFirstEntry; }
private:
  nsTextControlFrame &mFrame;
  bool mFirstEntry;
};
#endif

nsTextControlFrame::nsTextControlFrame(nsIPresShell* aShell, nsStyleContext* aContext)
  : nsContainerFrame(aContext)
  , mEditorHasBeenInitialized(false)
  , mIsProcessing(false)
#ifdef DEBUG
  , mInEditorInitialization(false)
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

  EditorInitializer* initializer = (EditorInitializer*) Properties().Get(TextControlInitializer());
  if (initializer) {
    initializer->Revoke();
    Properties().Delete(TextControlInitializer());
  }

  
  
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  txtCtrl->UnbindFromFrame(this);

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);

  nsContainerFrame::DestroyFrom(aDestructRoot);
}

nsIAtom*
nsTextControlFrame::GetType() const 
{ 
  return nsGkAtoms::textInputFrame;
}

nsresult
nsTextControlFrame::CalcIntrinsicSize(nsRenderingContext* aRenderingContext,
                                      nsSize&             aIntrinsicSize,
                                      float               aFontSizeInflation)
{
  
  nscoord lineHeight  = 0;
  nscoord charWidth   = 0;
  nscoord charMaxAdvance  = 0;

  nsRefPtr<nsFontMetrics> fontMet;
  nsresult rv =
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet),
                                          aFontSizeInflation);
  NS_ENSURE_SUCCESS(rv, rv);
  aRenderingContext->SetFont(fontMet);

  lineHeight =
    nsHTMLReflowState::CalcLineHeight(GetContent(), StyleContext(),
                                      NS_AUTOHEIGHT, aFontSizeInflation);
  charWidth = fontMet->AveCharWidth();
  charMaxAdvance = fontMet->MaxAdvance();

  
  int32_t cols = GetCols();
  aIntrinsicSize.width = cols * charWidth;

  
  
  
  if (mozilla::Abs(charWidth - charMaxAdvance) > (unsigned)nsPresContext::CSSPixelsToAppUnits(1)) {
    nscoord internalPadding =
      std::max(0, charMaxAdvance - nsPresContext::CSSPixelsToAppUnits(4));
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
  }

  
  {
    const nsStyleCoord& lsCoord = StyleText()->mLetterSpacing;
    if (eStyleUnit_Coord == lsCoord.GetUnit()) {
      nscoord letterSpacing = lsCoord.GetCoordValue();
      if (letterSpacing != 0) {
        aIntrinsicSize.width += cols * letterSpacing;
      }
    }
  }

  
  
  aIntrinsicSize.height = lineHeight * GetRows();

  
  if (IsTextArea()) {
    nsIFrame* first = GetFirstPrincipalChild();

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
  

  
  
  
  
  
  
  
  
  

  if (mEditorHasBeenInitialized)
    return NS_OK;

  nsIDocument* doc = mContent->GetComposedDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

  nsWeakFrame weakFrame(this);

  
  
  
  doc->FlushPendingNotifications(Flush_ContentAndNotify);
  NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_ERROR_FAILURE);

  
  
  {
    nsAutoScriptBlocker scriptBlocker;

    
    
    mozilla::dom::AutoNoJSAPI nojsapi;

    
    class EnsureSetFocus {
    public:
      explicit EnsureSetFocus(nsTextControlFrame* aFrame)
        : mFrame(aFrame) {}
      ~EnsureSetFocus() {
        if (nsContentUtils::IsFocusedContent(mFrame->GetContent()))
          mFrame->SetFocus(true, false);
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
    NS_ENSURE_STATE(weakFrame.IsAlive());

    
    
    mEditorHasBeenInitialized = true;

    
    if (weakFrame.IsAlive()) {
      SetSelectionEndPoints(0, 0);
    }
  }
  NS_ENSURE_STATE(weakFrame.IsAlive());
  return NS_OK;
}

nsresult
nsTextControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
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

    
    nsCSSPseudoElements::Type pseudoType =
      nsCSSPseudoElements::ePseudo_mozPlaceholder;

    nsRefPtr<nsStyleContext> placeholderStyleContext =
      PresContext()->StyleSet()->ResolvePseudoElementStyle(
          mContent->AsElement(), pseudoType, StyleContext(),
          placeholderNode->AsElement());

    if (!aElements.AppendElement(ContentInfo(placeholderNode,
                                 placeholderStyleContext))) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  rv = UpdateValueDisplay(false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool initEagerly = !IsSingleLineTextControl();
  if (!initEagerly) {
    
    
    nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
    NS_ASSERTION(txtCtrl, "Content not a text control element");
    initEagerly = txtCtrl->HasCachedSelection();
  }
  if (!initEagerly) {
    nsCOMPtr<nsIDOMHTMLElement> element = do_QueryInterface(txtCtrl);
    if (element) {
      
      element->GetSpellcheck(&initEagerly);
    }
  }

  if (initEagerly) {
    NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                 "Someone forgot a script blocker?");
    EditorInitializer* initializer = (EditorInitializer*) Properties().Get(TextControlInitializer());
    if (initializer) {
      initializer->Revoke();
    }
    initializer = new EditorInitializer(this);
    Properties().Set(TextControlInitializer(),initializer);
    if (!nsContentUtils::AddScriptRunner(initializer)) {
      initializer->Revoke(); 
      Properties().Delete(TextControlInitializer());
      delete initializer;
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
}

void
nsTextControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                             uint32_t aFilter)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  aElements.MaybeAppendElement(txtCtrl->GetRootEditorNode());
  if (!(aFilter & nsIContent::eSkipPlaceholderContent))
    aElements.MaybeAppendElement(txtCtrl->GetPlaceholderNode());
  
}

nscoord
nsTextControlFrame::GetPrefISize(nsRenderingContext* aRenderingContext)
{
    DebugOnly<nscoord> result = 0;
    DISPLAY_PREF_WIDTH(this, result);

    float inflation = nsLayoutUtils::FontSizeInflationFor(this);
    nsSize autoSize;
    CalcIntrinsicSize(aRenderingContext, autoSize, inflation);

    return autoSize.width; 
}

nscoord
nsTextControlFrame::GetMinISize(nsRenderingContext* aRenderingContext)
{
  
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetPrefISize(aRenderingContext);

  return result;
}

nsSize
nsTextControlFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                    nsSize aCBSize, nscoord aAvailableWidth,
                                    nsSize aMargin, nsSize aBorder,
                                    nsSize aPadding, bool aShrinkWrap)
{
  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  nsSize autoSize;
  nsresult rv = CalcIntrinsicSize(aRenderingContext, autoSize, inflation);
  if (NS_FAILED(rv)) {
    
    autoSize.SizeTo(0, 0);
  }
#ifdef DEBUG
  
  else if (StylePosition()->mWidth.GetUnit() == eStyleUnit_Auto) {
    nsSize ancestorAutoSize =
      nsContainerFrame::ComputeAutoSize(aRenderingContext,
                                        aCBSize, aAvailableWidth,
                                        aMargin, aBorder,
                                        aPadding, aShrinkWrap);
    
    NS_ASSERTION(inflation != 1.0f || ancestorAutoSize.width == autoSize.width,
                 "Incorrect size computed by ComputeAutoSize?");
  }
#endif

  return autoSize;
}

void
nsTextControlFrame::Reflow(nsPresContext*   aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTextControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  
  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, true);
  }

  
  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize
    finalSize(wm,
              aReflowState.ComputedISize() +
              aReflowState.ComputedLogicalBorderPadding().IStartEnd(wm),
              aReflowState.ComputedBSize() +
              aReflowState.ComputedLogicalBorderPadding().BStartEnd(wm));
  aDesiredSize.SetSize(wm, finalSize);

  
  nscoord lineHeight = aReflowState.ComputedBSize();
  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  if (!IsSingleLineTextControl()) {
    lineHeight = nsHTMLReflowState::CalcLineHeight(GetContent(), StyleContext(),
                                                   NS_AUTOHEIGHT, inflation);
  }
  nsRefPtr<nsFontMetrics> fontMet;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet),
                                        inflation);
  
  aDesiredSize.SetBlockStartAscent(
    nsLayoutUtils::GetCenteredFontBaseline(fontMet, lineHeight) +
    aReflowState.ComputedLogicalBorderPadding().BStart(wm));

  
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    ReflowTextControlChild(kid, aPresContext, aReflowState, aStatus, aDesiredSize);
    kid = kid->GetNextSibling();
  }

  
  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

void
nsTextControlFrame::ReflowTextControlChild(nsIFrame*                aKid,
                                           nsPresContext*           aPresContext,
                                           const nsHTMLReflowState& aReflowState,
                                           nsReflowStatus&          aStatus,
                                           nsHTMLReflowMetrics& aParentDesiredSize)
{
  
  WritingMode wm = aKid->GetWritingMode();
  LogicalSize availSize = aReflowState.ComputedSizeWithPadding(wm);
  availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;

  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, 
                                   aKid, availSize, -1, -1,
                                   nsHTMLReflowState::CALLER_WILL_INIT);
  
  kidReflowState.Init(aPresContext, -1, -1, nullptr, &aReflowState.ComputedPhysicalPadding());

  
  kidReflowState.SetComputedWidth(aReflowState.ComputedWidth());
  kidReflowState.SetComputedHeight(aReflowState.ComputedHeight());

  
  nscoord xOffset = aReflowState.ComputedPhysicalBorderPadding().left -
                    aReflowState.ComputedPhysicalPadding().left;
  nscoord yOffset = aReflowState.ComputedPhysicalBorderPadding().top -
                    aReflowState.ComputedPhysicalPadding().top;

  
  nsHTMLReflowMetrics desiredSize(aReflowState);
  ReflowChild(aKid, aPresContext, desiredSize, kidReflowState, 
              xOffset, yOffset, 0, aStatus);

  
  FinishReflowChild(aKid, aPresContext, desiredSize,
                    &kidReflowState, xOffset, yOffset, 0);

  
  aParentDesiredSize.mOverflowAreas.UnionWith(desiredSize.mOverflowAreas);
}

nsSize
nsTextControlFrame::GetMinSize(nsBoxLayoutState& aState)
{
  
  return nsBox::GetMinSize(aState);
}

bool
nsTextControlFrame::IsCollapsed()
{
  
  return false;
}

bool
nsTextControlFrame::IsLeaf() const
{
  return true;
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


void nsTextControlFrame::SetFocus(bool aOn, bool aRepaint)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  
  mScrollEvent.Revoke();

  
  
  if (mUsePlaceholder) {
    txtCtrl->UpdatePlaceholderVisibility(true);
  }

  if (!aOn) {
    return;
  }

  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  if (!selCon)
    return;

  nsCOMPtr<nsISelection> ourSel;
  selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, 
    getter_AddRefs(ourSel));
  if (!ourSel) return;

  nsIPresShell* presShell = PresContext()->GetPresShell();
  nsRefPtr<nsCaret> caret = presShell->GetCaret();
  if (!caret) return;

  
  nsISelection *caretSelection = caret->GetCaretDOMSelection();
  const bool isFocusedRightNow = ourSel == caretSelection;
  if (!isFocusedRightNow) {
    
    uint32_t lastFocusMethod = 0;
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

  bool isCollapsed = false;
  docSel->GetIsCollapsed(&isCollapsed);
  if (!isCollapsed)
    docSel->RemoveAllRanges();
}

nsresult nsTextControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  if (!mIsProcessing)
  {
    mIsProcessing = true;
    if (nsGkAtoms::select == aName)
    {
      
      
      
      
      
      
      

      nsWeakFrame weakThis = this;
      SelectAllOrCollapseToEndOfText(true);  
      if (!weakThis.IsAlive()) {
        return NS_OK;
      }
    }
    mIsProcessing = false;
  }
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

nsresult
nsTextControlFrame::SetSelectionInternal(nsIDOMNode *aStartNode,
                                         int32_t aStartOffset,
                                         nsIDOMNode *aEndNode,
                                         int32_t aEndOffset,
                                         nsITextControlFrame::SelectionDirection aDirection)
{
  
  
  

  nsRefPtr<nsRange> range = new nsRange(mContent);
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

  nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(selection, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsDirection direction;
  if (aDirection == eNone) {
    
    direction = selPriv->GetSelectionDirection();
  } else {
    direction = (aDirection == eBackward) ? eDirPrevious : eDirNext;
  }

  rv = selection->RemoveAllRanges();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selection->AddRange(range);  
  NS_ENSURE_SUCCESS(rv, rv);

  selPriv->SetSelectionDirection(direction);
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

mozilla::dom::Element*
nsTextControlFrame::GetRootNodeAndInitializeEditor()
{
  nsCOMPtr<nsIDOMElement> root;
  GetRootNodeAndInitializeEditor(getter_AddRefs(root));
  nsCOMPtr<mozilla::dom::Element> rootElem = do_QueryInterface(root);
  return rootElem;
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
nsTextControlFrame::SelectAllOrCollapseToEndOfText(bool aSelect)
{
  nsCOMPtr<nsIDOMElement> rootElement;
  nsresult rv = GetRootNodeAndInitializeEditor(getter_AddRefs(rootElement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIContent> rootContent = do_QueryInterface(rootElement);
  nsCOMPtr<nsIDOMNode> rootNode(do_QueryInterface(rootElement));

  NS_ENSURE_TRUE(rootNode && rootContent, NS_ERROR_FAILURE);

  int32_t numChildren = rootContent->GetChildCount();

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
nsTextControlFrame::SetSelectionEndPoints(int32_t aSelStart, int32_t aSelEnd,
                                          nsITextControlFrame::SelectionDirection aDirection)
{
  NS_ASSERTION(aSelStart <= aSelEnd, "Invalid selection offsets!");

  if (aSelStart > aSelEnd)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMNode> startNode, endNode;
  int32_t startOffset, endOffset;

  

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

  return SetSelectionInternal(startNode, startOffset, endNode, endOffset, aDirection);
}

NS_IMETHODIMP
nsTextControlFrame::SetSelectionRange(int32_t aSelStart, int32_t aSelEnd,
                                      nsITextControlFrame::SelectionDirection aDirection)
{
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelStart > aSelEnd) {
    
    

    aSelStart   = aSelEnd;
  }

  return SetSelectionEndPoints(aSelStart, aSelEnd, aDirection);
}


NS_IMETHODIMP
nsTextControlFrame::SetSelectionStart(int32_t aSelectionStart)
{
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t selStart = 0, selEnd = 0; 

  rv = GetSelectionRange(&selStart, &selEnd);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelectionStart > selEnd) {
    
    selEnd = aSelectionStart; 
  }

  selStart = aSelectionStart;
  
  return SetSelectionEndPoints(selStart, selEnd);
}

NS_IMETHODIMP
nsTextControlFrame::SetSelectionEnd(int32_t aSelectionEnd)
{
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t selStart = 0, selEnd = 0; 

  rv = GetSelectionRange(&selStart, &selEnd);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelectionEnd < selStart) {
    
    selStart = aSelectionEnd; 
  }

  selEnd = aSelectionEnd;
  
  return SetSelectionEndPoints(selStart, selEnd);
}

nsresult
nsTextControlFrame::OffsetToDOMPoint(int32_t aOffset,
                                     nsIDOMNode** aResult,
                                     int32_t* aPosition)
{
  NS_ENSURE_ARG_POINTER(aResult && aPosition);

  *aResult = nullptr;
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

  uint32_t length = 0;

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
    uint32_t textLength = 0;
    textNode->GetLength(&textLength);
    if (length == 2 && uint32_t(aOffset) == textLength) {
      
      
      NS_IF_ADDREF(*aResult = rootNode);
      *aPosition = 1;
    } else {
      
      NS_IF_ADDREF(*aResult = firstNode);
      *aPosition = std::min(aOffset, int32_t(textLength));
    }
  } else {
    NS_IF_ADDREF(*aResult = rootNode);
    *aPosition = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTextControlFrame::GetSelectionRange(int32_t* aSelectionStart,
                                      int32_t* aSelectionEnd,
                                      SelectionDirection* aDirection)
{
  
  nsresult rv = EnsureEditorInitialized();
  NS_ENSURE_SUCCESS(rv, rv);

  if (aSelectionStart) {
    *aSelectionStart = 0;
  }
  if (aSelectionEnd) {
    *aSelectionEnd = 0;
  }
  if (aDirection) {
    *aDirection = eNone;
  }

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  NS_ENSURE_TRUE(selCon, NS_ERROR_FAILURE);
  nsCOMPtr<nsISelection> selection;
  rv = selCon->GetSelection(nsISelectionController::SELECTION_NORMAL, getter_AddRefs(selection));  
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  dom::Selection* sel = static_cast<dom::Selection*>(selection.get());
  if (aDirection) {
    nsDirection direction = sel->GetSelectionDirection();
    if (direction == eDirNext) {
      *aDirection = eForward;
    } else if (direction == eDirPrevious) {
      *aDirection = eBackward;
    } else {
      NS_NOTREACHED("Invalid nsDirection enum value");
    }
  }

  if (!aSelectionStart || !aSelectionEnd) {
    return NS_OK;
  }

  mozilla::dom::Element* root = GetRootNodeAndInitializeEditor();
  NS_ENSURE_STATE(root);
  nsContentUtils::GetSelectionInTextControl(sel, root,
                                            *aSelectionStart, *aSelectionEnd);

  return NS_OK;
}




nsresult
nsTextControlFrame::AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsISelectionController* selCon = txtCtrl->GetSelectionController();
  const bool needEditor = nsGkAtoms::maxlength == aAttribute ||
                            nsGkAtoms::readonly == aAttribute ||
                            nsGkAtoms::disabled == aAttribute ||
                            nsGkAtoms::spellcheck == aAttribute;
  nsCOMPtr<nsIEditor> editor;
  if (needEditor) {
    GetEditor(getter_AddRefs(editor));
  }
  if ((needEditor && !editor) || !selCon) {
    return nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
  }

  if (nsGkAtoms::maxlength == aAttribute) {
    int32_t maxLength;
    bool maxDefined = GetMaxLength(&maxLength);
    nsCOMPtr<nsIPlaintextEditor> textEditor = do_QueryInterface(editor);
    if (textEditor) {
      if (maxDefined) { 
        textEditor->SetMaxTextLength(maxLength);
        
      } else { 
        textEditor->SetMaxTextLength(-1);
      }
    }
    return NS_OK;
  }

  if (nsGkAtoms::readonly == aAttribute) {
    uint32_t flags;
    editor->GetFlags(&flags);
    if (AttributeExists(nsGkAtoms::readonly)) { 
      flags |= nsIPlaintextEditor::eEditorReadonlyMask;
      if (nsContentUtils::IsFocusedContent(mContent)) {
        selCon->SetCaretEnabled(false);
      }
    } else { 
      flags &= ~(nsIPlaintextEditor::eEditorReadonlyMask);
      if (!(flags & nsIPlaintextEditor::eEditorDisabledMask) &&
          nsContentUtils::IsFocusedContent(mContent)) {
        selCon->SetCaretEnabled(true);
      }
    }
    editor->SetFlags(flags);
    return NS_OK;
  }

  if (nsGkAtoms::disabled == aAttribute) {
    uint32_t flags;
    editor->GetFlags(&flags);
    int16_t displaySelection = nsISelectionController::SELECTION_OFF;
    const bool focused = nsContentUtils::IsFocusedContent(mContent);
    const bool hasAttr = AttributeExists(nsGkAtoms::disabled);
    if (hasAttr) { 
      flags |= nsIPlaintextEditor::eEditorDisabledMask;
    } else { 
      flags &= ~(nsIPlaintextEditor::eEditorDisabledMask);
      displaySelection = focused ? nsISelectionController::SELECTION_ON
                                 : nsISelectionController::SELECTION_HIDDEN;
    }
    selCon->SetDisplaySelection(displaySelection);
    if (focused) {
      selCon->SetCaretEnabled(!hasAttr);
    }
    editor->SetFlags(flags);
    return NS_OK;
  }

  if (!mEditorHasBeenInitialized && nsGkAtoms::value == aAttribute) {
    UpdateValueDisplay(true);
    return NS_OK;
  }

  
  
  return nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}


nsresult
nsTextControlFrame::GetText(nsString& aText)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  if (IsSingleLineTextControl()) {
    
    txtCtrl->GetTextEditorValue(aText, true);
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




bool
nsTextControlFrame::GetMaxLength(int32_t* aSize)
{
  *aSize = -1;

  nsGenericHTMLElement *content = nsGenericHTMLElement::FromContent(mContent);
  if (content) {
    const nsAttrValue* attr = content->GetParsedAttr(nsGkAtoms::maxlength);
    if (attr && attr->Type() == nsAttrValue::eInteger) {
      *aSize = attr->GetIntegerValue();

      return true;
    }
  }
  return false;
}



void
nsTextControlFrame::SetInitialChildList(ChildListID     aListID,
                                        nsFrameList&    aChildList)
{
  nsContainerFrame::SetInitialChildList(aListID, aChildList);

  nsIFrame* first = GetFirstPrincipalChild();

  
  
  
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
}

void
nsTextControlFrame::SetValueChanged(bool aValueChanged)
{
  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  if (mUsePlaceholder) {
    nsWeakFrame weakFrame(this);
    txtCtrl->UpdatePlaceholderVisibility(true);
    if (!weakFrame.IsAlive()) {
      return;
    }
  }

  txtCtrl->SetValueChanged(aValueChanged);
}


nsresult
nsTextControlFrame::UpdateValueDisplay(bool aNotify,
                                       bool aBeforeEditorInit,
                                       const nsAString *aValue)
{
  if (!IsSingleLineTextControl()) 
    return NS_OK;

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");
  nsIContent* rootNode = txtCtrl->GetRootEditorNode();

  NS_PRECONDITION(rootNode, "Must have a div content\n");
  NS_PRECONDITION(!mEditorHasBeenInitialized,
                  "Do not call this after editor has been initialized");
  NS_ASSERTION(!mUsePlaceholder || txtCtrl->GetPlaceholderNode(),
               "A placeholder div must exist");

  nsIContent *textContent = rootNode->GetChildAt(0);
  if (!textContent) {
    
    nsRefPtr<nsTextNode> textNode =
      new nsTextNode(mContent->NodeInfo()->NodeInfoManager());

    NS_ASSERTION(textNode, "Must have textcontent!\n");

    rootNode->AppendChildTo(textNode, aNotify);
    textContent = textNode;
  }

  NS_ENSURE_TRUE(textContent, NS_ERROR_UNEXPECTED);

  
  nsAutoString value;
  if (aValue) {
    value = *aValue;
  } else {
    txtCtrl->GetTextEditorValue(value, true);
  }

  
  
  
  if (mUsePlaceholder && !aBeforeEditorInit)
  {
    nsWeakFrame weakFrame(this);
    txtCtrl->UpdatePlaceholderVisibility(aNotify);
    NS_ENSURE_STATE(weakFrame.IsAlive());
  }

  if (aBeforeEditorInit && value.IsEmpty()) {
    rootNode->RemoveChildAt(0, true);
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
nsTextControlFrame::SaveState(nsPresState** aState)
{
  NS_ENSURE_ARG_POINTER(aState);

  *aState = nullptr;

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element");

  nsIContent* rootNode = txtCtrl->GetRootEditorNode();
  if (rootNode) {
    
    nsIStatefulFrame* scrollStateFrame = do_QueryFrame(rootNode->GetPrimaryFrame());
    if (scrollStateFrame) {
      return scrollStateFrame->SaveState(aState);
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

nsresult
nsTextControlFrame::PeekOffset(nsPeekOffsetStruct *aPos)
{
  return NS_ERROR_FAILURE;
}

void
nsTextControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  





  DO_GLOBAL_REFLOW_COUNT_DSP("nsTextControlFrame");

  nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
  NS_ASSERTION(txtCtrl, "Content not a text control element!");

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  nsIFrame* kid = mFrames.FirstChild();
  
  
  
  nsDisplayList* content = aLists.Content();
  nsDisplayListSet set(content, content, content, content, content, content);

  while (kid) {
    
    
    if (kid->GetContent() != txtCtrl->GetPlaceholderNode() ||
        txtCtrl->GetPlaceholderVisibility()) {
      BuildDisplayListForChild(aBuilder, kid, aDirtyRect, set, 0);
    }
    kid = kid->GetNextSibling();
  }
}

mozilla::dom::Element*
nsTextControlFrame::GetPseudoElement(nsCSSPseudoElements::Type aType)
{
  if (aType == nsCSSPseudoElements::ePseudo_mozPlaceholder) {
    nsCOMPtr<nsITextControlElement> txtCtrl = do_QueryInterface(GetContent());
    return txtCtrl->GetPlaceholderNode();
  }

  return nsContainerFrame::GetPseudoElement(aType);
}

NS_IMETHODIMP
nsTextControlFrame::EditorInitializer::Run()
{
  if (!mFrame) {
    return NS_OK;
  }

  
  nsAutoScriptBlocker scriptBlocker;

  nsCOMPtr<nsIPresShell> shell =
    mFrame->PresContext()->GetPresShell();
  bool observes = shell->ObservesNativeAnonMutationsForPrint();
  shell->ObserveNativeAnonMutationsForPrint(true);
  
  mFrame->EnsureEditorInitialized();
  shell->ObserveNativeAnonMutationsForPrint(observes);

  
  
  if (!mFrame) {
    return NS_ERROR_FAILURE;
  }

  mFrame->FinishedInitializer();
  return NS_OK;
}
