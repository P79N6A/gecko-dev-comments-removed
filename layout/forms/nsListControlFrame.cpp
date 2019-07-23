






































#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsListControlFrame.h"
#include "nsFormControlFrame.h" 
#include "nsGkAtoms.h"
#include "nsIFormControl.h"
#include "nsIDeviceContext.h" 
#include "nsIDocument.h"
#include "nsIDOMHTMLCollection.h" 
#include "nsIDOMHTMLOptionsCollection.h" 
#include "nsIDOMNSHTMLOptionCollectn.h"
#include "nsIDOMHTMLSelectElement.h" 
#include "nsIDOMNSHTMLSelectElement.h" 
#include "nsIDOMHTMLOptionElement.h" 
#include "nsComboboxControlFrame.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsWidgetsCID.h"
#include "nsIPresShell.h"
#include "nsHTMLParts.h"
#include "nsIDOMEventTarget.h"
#include "nsEventDispatcher.h"
#include "nsIEventStateManager.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsILookAndFeel.h"
#include "nsIFontMetrics.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsGUIEvent.h"
#include "nsIServiceManager.h"
#include "nsINodeInfo.h"
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsISelectElement.h"
#include "nsIPrivateDOMEvent.h"
#include "nsCSSRendering.h"
#include "nsITheme.h"
#include "nsIDOMMouseListener.h"
#include "nsIDOMMouseMotionListener.h"
#include "nsIDOMKeyListener.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"


const nscoord kMaxDropDownRows          = 20; 
const PRInt32 kNothingSelected          = -1;


nsListControlFrame * nsListControlFrame::mFocused = nsnull;
nsString * nsListControlFrame::sIncrementalString = nsnull;


#define INCREMENTAL_SEARCH_KEYPRESS_TIME 1000





DOMTimeStamp nsListControlFrame::gLastKeyTime = 0;







class nsListEventListener : public nsIDOMKeyListener,
                            public nsIDOMMouseListener,
                            public nsIDOMMouseMotionListener
{
public:
  nsListEventListener(nsListControlFrame *aFrame)
    : mFrame(aFrame) { }

  void SetFrame(nsListControlFrame *aFrame) { mFrame = aFrame; }

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  
  NS_IMETHOD KeyDown(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyUp(nsIDOMEvent* aKeyEvent);
  NS_IMETHOD KeyPress(nsIDOMEvent* aKeyEvent);

  
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent);

  
  NS_IMETHOD MouseMove(nsIDOMEvent* aMouseEvent);
  NS_IMETHOD DragMove(nsIDOMEvent* aMouseEvent);

private:
  nsListControlFrame  *mFrame;
};


nsIFrame*
NS_NewListControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsListControlFrame* it =
    new (aPresShell) nsListControlFrame(aPresShell, aPresShell->GetDocument(), aContext);

  if (it) {
    it->AddStateBits(NS_FRAME_INDEPENDENT_SELECTION);
  }

  return it;
}


nsListControlFrame::nsListControlFrame(
  nsIPresShell* aShell, nsIDocument* aDocument, nsStyleContext* aContext)
  : nsHTMLScrollFrame(aShell, aContext, PR_FALSE),
    mMightNeedSecondPass(PR_FALSE),
    mLastDropdownComputedHeight(NS_UNCONSTRAINEDSIZE)
{
  mComboboxFrame      = nsnull;
  mChangesSinceDragStart = PR_FALSE;
  mButtonDown         = PR_FALSE;

  mIsAllContentHere   = PR_FALSE;
  mIsAllFramesHere    = PR_FALSE;
  mHasBeenInitialized = PR_FALSE;
  mNeedToReset        = PR_TRUE;
  mPostChildrenLoadedReset = PR_FALSE;

  mControlSelectMode           = PR_FALSE;
}


nsListControlFrame::~nsListControlFrame()
{
  mComboboxFrame = nsnull;
}


void
nsListControlFrame::Destroy()
{
  
  ENSURE_TRUE(mContent);

  
  

  mEventListener->SetFrame(nsnull);

  mContent->RemoveEventListenerByIID(static_cast<nsIDOMMouseListener*>
                                                (mEventListener),
                                     NS_GET_IID(nsIDOMMouseListener));

  mContent->RemoveEventListenerByIID(static_cast<nsIDOMMouseMotionListener*>
                                                (mEventListener),
                                     NS_GET_IID(nsIDOMMouseMotionListener));

  mContent->RemoveEventListenerByIID(static_cast<nsIDOMKeyListener*>
                                                (mEventListener),
                                     NS_GET_IID(nsIDOMKeyListener));

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_FALSE);
  nsHTMLScrollFrame::Destroy();
}

NS_IMETHODIMP
nsListControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  
  
  
  if (aBuilder->IsBackgroundOnly())
    return NS_OK;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsListControlFrame");

  
  
  
  
  
  return nsHTMLScrollFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}








void nsListControlFrame::PaintFocus(nsIRenderingContext& aRC, nsPoint aPt)
{
  if (mFocused != this) return;

  
  
  PRInt32 focusedIndex;
  if (mEndSelectionIndex == kNothingSelected) {
    focusedIndex = GetSelectedIndex();
  } else {
    focusedIndex = mEndSelectionIndex;
  }

  nsPresContext* presContext = PresContext();
  if (!GetScrollableView()) return;

  nsIPresShell *presShell = presContext->GetPresShell();
  if (!presShell) return;

  nsIFrame* containerFrame = GetOptionsContainer();
  if (!containerFrame) return;

  nsIFrame * childframe = nsnull;
  nsresult result = NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> focusedContent;

  nsCOMPtr<nsIDOMNSHTMLSelectElement> selectNSElement(do_QueryInterface(mContent));
  NS_ASSERTION(selectNSElement, "Can't be null");

  nsCOMPtr<nsISelectElement> selectElement(do_QueryInterface(mContent));
  NS_ASSERTION(selectElement, "Can't be null");

  
  if (focusedIndex != kNothingSelected) {
    focusedContent = GetOptionContent(focusedIndex);
    
    if (focusedContent) {
      childframe = presShell->GetPrimaryFrameFor(focusedContent);
    }
  } else {
    nsCOMPtr<nsIDOMHTMLSelectElement> selectHTMLElement(do_QueryInterface(mContent));
    NS_ASSERTION(selectElement, "Can't be null");

    
    
    nsCOMPtr<nsIDOMNode> node;

    PRUint32 length;
    selectHTMLElement->GetLength(&length);
    if (length) {
      
      PRBool isDisabled = PR_TRUE;
      for (PRInt32 i=0;i<PRInt32(length) && isDisabled;i++) {
        if (NS_FAILED(selectNSElement->Item(i, getter_AddRefs(node))) || !node) {
          break;
        }
        if (NS_FAILED(selectElement->IsOptionDisabled(i, &isDisabled))) {
          break;
        }
        if (isDisabled) {
          node = nsnull;
        } else {
          break;
        }
      }
      if (!node) {
        return;
      }
    }

    
    if (node) {
      focusedContent = do_QueryInterface(node);
      childframe = presShell->GetPrimaryFrameFor(focusedContent);
    }
    if (!childframe) {
      
      
      childframe = containerFrame->GetFirstChild(nsnull);
      if (childframe &&
          !childframe->GetContent()->IsNodeOfType(nsINode::eELEMENT)) {
        childframe = nsnull;
      }
      result = NS_OK;
    }
  }

  nsRect fRect;
  if (childframe) {
    
    fRect = childframe->GetRect();
    
    fRect.MoveBy(childframe->GetParent()->GetOffsetTo(this));
  } else {
    fRect.x = fRect.y = 0;
    fRect.width = GetScrollPortSize().width;
    fRect.height = CalcFallbackRowHeight();
    fRect.MoveBy(containerFrame->GetOffsetTo(this));
  }
  fRect += aPt;
  
  PRBool lastItemIsSelected = PR_FALSE;
  if (focusedContent) {
    nsCOMPtr<nsIDOMHTMLOptionElement> domOpt =
      do_QueryInterface(focusedContent);
    if (domOpt) {
      domOpt->GetSelected(&lastItemIsSelected);
    }
  }

  
  nscolor color;
  presContext->LookAndFeel()->
    GetColor(lastItemIsSelected ?
             nsILookAndFeel::eColor_WidgetSelectForeground :
             nsILookAndFeel::eColor_WidgetSelectBackground, color);

  nsCSSRendering::PaintFocus(presContext, aRC, fRect, color);
}

void
nsListControlFrame::InvalidateFocus()
{
  if (mFocused != this)
    return;

  nsIFrame* containerFrame = GetOptionsContainer();
  if (containerFrame) {
    
    
    
    nsRect invalidateArea = containerFrame->GetOverflowRect();
    nsRect emptyFallbackArea(0, 0, GetScrollPortSize().width, CalcFallbackRowHeight());
    invalidateArea.UnionRect(invalidateArea, emptyFallbackArea);
    containerFrame->Invalidate(invalidateArea);
  }
}

NS_QUERYFRAME_HEAD(nsListControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
  NS_QUERYFRAME_ENTRY(nsIListControlFrame)
  NS_QUERYFRAME_ENTRY(nsISelectControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLScrollFrame)

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsListControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(mContent);
    nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(PresContext()->PresShell()));
    return accService->CreateHTMLListboxAccessible(node, weakShell, aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

static nscoord
GetMaxOptionHeight(nsIFrame* aContainer)
{
  nscoord result = 0;
  for (nsIFrame* option = aContainer->GetFirstChild(nsnull);
       option; option = option->GetNextSibling()) {
    nscoord optionHeight;
    if (nsCOMPtr<nsIDOMHTMLOptGroupElement>
        (do_QueryInterface(option->GetContent()))) {
      
      optionHeight = GetMaxOptionHeight(option);
    } else {
      
      optionHeight = option->GetSize().height;
    }
    if (result < optionHeight)
      result = optionHeight;
  }
  return result;
}

static inline PRBool
IsOptGroup(nsIContent *aContent)
{
  return (aContent->NodeInfo()->Equals(nsGkAtoms::optgroup) &&
          aContent->IsNodeOfType(nsINode::eHTML));
}

static inline PRBool
IsOption(nsIContent *aContent)
{
  return (aContent->NodeInfo()->Equals(nsGkAtoms::option) &&
          aContent->IsNodeOfType(nsINode::eHTML));
}

static PRUint32
GetNumberOfOptionsRecursive(nsIContent* aContent)
{
  PRUint32 optionCount = 0;
  const PRUint32 childCount = aContent ? aContent->GetChildCount() : 0;
  for (PRUint32 index = 0; index < childCount; ++index) {
    nsIContent* child = aContent->GetChildAt(index);
    if (::IsOption(child)) {
      ++optionCount;
    }
    else if (::IsOptGroup(child)) {
      optionCount += ::GetNumberOfOptionsRecursive(child);
    }
  }
  return optionCount;
}

static nscoord
GetOptGroupLabelsHeight(nsPresContext* aPresContext,
                        nsIContent*    aContent,
                        nscoord        aRowHeight)
{
  nscoord height = 0;
  const PRUint32 childCount = aContent ? aContent->GetChildCount() : 0;
  for (PRUint32 index = 0; index < childCount; ++index) {
    nsIContent* child = aContent->GetChildAt(index);
    if (::IsOptGroup(child)) {
      PRUint32 numOptions = ::GetNumberOfOptionsRecursive(child);
      nscoord optionsHeight = aRowHeight * numOptions;
      nsIFrame* frame = aPresContext->GetPresShell()->GetPrimaryFrameFor(child);
      nscoord totalHeight = frame ? frame->GetSize().height : 0;
      height += PR_MAX(0, totalHeight - optionsHeight);
    }
  }
  return height;
}





nscoord
nsListControlFrame::CalcHeightOfARow()
{
  
  
  
  
  PRInt32 heightOfARow = GetMaxOptionHeight(GetOptionsContainer());

  
  
  if (heightOfARow == 0 && GetNumberOfOptions() == 0) {
    heightOfARow = CalcFallbackRowHeight();
  }

  return heightOfARow;
}

nscoord
nsListControlFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  
  
  
  result = GetScrolledFrame()->GetPrefWidth(aRenderingContext);
  result = NSCoordSaturatingAdd(result,
          GetDesiredScrollbarSizes(PresContext(), aRenderingContext).LeftRight());

  return result;
}

nscoord
nsListControlFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  
  
  
  result = GetScrolledFrame()->GetMinWidth(aRenderingContext);
  result += GetDesiredScrollbarSizes(PresContext(), aRenderingContext).LeftRight();

  return result;
}

NS_IMETHODIMP 
nsListControlFrame::Reflow(nsPresContext*           aPresContext, 
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState, 
                           nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aReflowState.ComputedWidth() != NS_UNCONSTRAINEDSIZE,
                  "Must have a computed width");

  
  
  if (mIsAllContentHere && !mHasBeenInitialized) {
    if (PR_FALSE == mIsAllFramesHere) {
      CheckIfAllFramesHere();
    }
    if (mIsAllFramesHere && !mHasBeenInitialized) {
      mHasBeenInitialized = PR_TRUE;
    }
  }

  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, PR_TRUE);
  }

  if (IsInDropDownMode()) {
    return ReflowAsDropdown(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  


















  PRBool autoHeight = (aReflowState.ComputedHeight() == NS_UNCONSTRAINEDSIZE);

  mMightNeedSecondPass = autoHeight &&
    (NS_SUBTREE_DIRTY(this) || aReflowState.ShouldReflowAllKids());
  
  nsHTMLReflowState state(aReflowState);
  PRInt32 length = GetNumberOfOptions();  

  nscoord oldHeightOfARow = HeightOfARow();
  
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW) && autoHeight) {
    
    
    nscoord computedHeight = CalcIntrinsicHeight(oldHeightOfARow, length);
    state.ApplyMinMaxConstraints(nsnull, &computedHeight);
    state.SetComputedHeight(computedHeight);
  }

  nsresult rv = nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize,
                                          state, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mMightNeedSecondPass) {
    NS_ASSERTION(!autoHeight || HeightOfARow() == oldHeightOfARow,
                 "How did our height of a row change if nothing was dirty?");
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if we don't need a second pass!");
    return rv;
  }

  mMightNeedSecondPass = PR_FALSE;

  
  
  if (!IsScrollbarUpdateSuppressed()) {
    
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if the height of a row has not "
                 "changed!");
    return rv;
  }

  SetSuppressScrollbarUpdate(PR_FALSE);

  
  
  
  
  
  nsHTMLScrollFrame::DidReflow(aPresContext, &state, aStatus);

  
  nscoord computedHeight = CalcIntrinsicHeight(HeightOfARow(), length); 
  state.ApplyMinMaxConstraints(nsnull, &computedHeight);
  state.SetComputedHeight(computedHeight);

  nsHTMLScrollFrame::WillReflow(aPresContext);

  
  
  
  return nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
}

nsresult
nsListControlFrame::ReflowAsDropdown(nsPresContext*           aPresContext, 
                                     nsHTMLReflowMetrics&     aDesiredSize,
                                     const nsHTMLReflowState& aReflowState, 
                                     nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aReflowState.ComputedHeight() == NS_UNCONSTRAINEDSIZE,
                  "We should not have a computed height here!");
  
  mMightNeedSecondPass = NS_SUBTREE_DIRTY(this) ||
    aReflowState.ShouldReflowAllKids();

#ifdef DEBUG
  nscoord oldHeightOfARow = HeightOfARow();
#endif

  nsHTMLReflowState state(aReflowState);

  nscoord oldVisibleHeight;
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    
    
    
    state.SetComputedHeight(mLastDropdownComputedHeight);
    oldVisibleHeight = GetScrolledFrame()->GetSize().height;
  } else {
    
    
    oldVisibleHeight = NS_UNCONSTRAINEDSIZE;
  }

  nsresult rv = nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize,
                                          state, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mMightNeedSecondPass) {
    NS_ASSERTION(oldVisibleHeight == GetScrolledFrame()->GetSize().height,
                 "How did our kid's height change if nothing was dirty?");
    NS_ASSERTION(HeightOfARow() == oldHeightOfARow,
                 "How did our height of a row change if nothing was dirty?");
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if we don't need a second pass!");
    NS_ASSERTION(!(GetStateBits() & NS_FRAME_FIRST_REFLOW),
                 "How can we avoid a second pass during first reflow?");
    return rv;
  }

  mMightNeedSecondPass = PR_FALSE;

  
  
  if (!IsScrollbarUpdateSuppressed()) {
    
    NS_ASSERTION(!(GetStateBits() & NS_FRAME_FIRST_REFLOW),
                 "How can we avoid a second pass during first reflow?");
    return rv;
  }

  SetSuppressScrollbarUpdate(PR_FALSE);

  nscoord visibleHeight = GetScrolledFrame()->GetSize().height;
  nscoord heightOfARow = HeightOfARow();

  
  
  
  
  
  nsHTMLScrollFrame::DidReflow(aPresContext, &state, aStatus);

  
  mNumDisplayRows = kMaxDropDownRows;
  if (visibleHeight > mNumDisplayRows * heightOfARow) {
    visibleHeight = mNumDisplayRows * heightOfARow;
    
    
    
    
    
    
    
    
    
    
    nsRect screen = nsFormControlFrame::GetUsableScreenRect(aPresContext);
    nscoord screenHeight = screen.height;

    nscoord availDropHgt = (screenHeight / 2) - (heightOfARow*2); 
    availDropHgt -= aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;

    nscoord hgt = visibleHeight + aReflowState.mComputedBorderPadding.top + aReflowState.mComputedBorderPadding.bottom;
    if (heightOfARow > 0) {
      if (hgt > availDropHgt) {
        visibleHeight = (availDropHgt / heightOfARow) * heightOfARow;
      }
      mNumDisplayRows = visibleHeight / heightOfARow;
    } else {
      
      visibleHeight   = 1;
      mNumDisplayRows = 1;
    }

    state.SetComputedHeight(mNumDisplayRows * heightOfARow);
    
    
    
  } else if (visibleHeight == 0) {
    
    state.SetComputedHeight(heightOfARow);
  } else {
    
    state.SetComputedHeight(NS_UNCONSTRAINEDSIZE);
  }

  
  
  
  mLastDropdownComputedHeight = state.ComputedHeight();

  nsHTMLScrollFrame::WillReflow(aPresContext);
  return nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
}

nsGfxScrollFrameInner::ScrollbarStyles
nsListControlFrame::GetScrollbarStyles() const
{
  
  
  PRInt32 verticalStyle = IsInDropDownMode() ? NS_STYLE_OVERFLOW_AUTO
    : NS_STYLE_OVERFLOW_SCROLL;
  return nsGfxScrollFrameInner::ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN,
                                                verticalStyle);
}

PRBool
nsListControlFrame::ShouldPropagateComputedHeightToScrolledContent() const
{
  return !IsInDropDownMode();
}


PRBool 
nsListControlFrame::IsOptionElement(nsIContent* aContent)
{
  PRBool result = PR_FALSE;
 
  nsCOMPtr<nsIDOMHTMLOptionElement> optElem;
  if (NS_SUCCEEDED(aContent->QueryInterface(NS_GET_IID(nsIDOMHTMLOptionElement),(void**) getter_AddRefs(optElem)))) {      
    if (optElem != nsnull) {
      result = PR_TRUE;
    }
  }
 
  return result;
}

nsIFrame*
nsListControlFrame::GetContentInsertionFrame() {
  return GetOptionsContainer()->GetContentInsertionFrame();
}





nsIContent *
nsListControlFrame::GetOptionFromContent(nsIContent *aContent) 
{
  for (nsIContent* content = aContent; content; content = content->GetParent()) {
    if (IsOptionElement(content)) {
      return content;
    }
  }

  return nsnull;
}





PRInt32 
nsListControlFrame::GetIndexFromContent(nsIContent *aContent)
{
  nsCOMPtr<nsIDOMHTMLOptionElement> option;
  option = do_QueryInterface(aContent);
  if (option) {
    PRInt32 retval;
    option->GetIndex(&retval);
    if (retval >= 0) {
      return retval;
    }
  }
  return kNothingSelected;
}


PRBool
nsListControlFrame::ExtendedSelection(PRInt32 aStartIndex,
                                      PRInt32 aEndIndex,
                                      PRBool aClearAll)
{
  return SetOptionsSelectedFromFrame(aStartIndex, aEndIndex,
                                     PR_TRUE, aClearAll);
}


PRBool
nsListControlFrame::SingleSelection(PRInt32 aClickedIndex, PRBool aDoToggle)
{
  if (mComboboxFrame) {
    mComboboxFrame->UpdateRecentIndex(GetSelectedIndex());
  }

  PRBool wasChanged = PR_FALSE;
  
  if (aDoToggle) {
    wasChanged = ToggleOptionSelectedFromFrame(aClickedIndex);
  } else {
    wasChanged = SetOptionsSelectedFromFrame(aClickedIndex, aClickedIndex,
                                PR_TRUE, PR_TRUE);
  }
  ScrollToIndex(aClickedIndex);
  mStartSelectionIndex = aClickedIndex;
  mEndSelectionIndex = aClickedIndex;
  InvalidateFocus();
  return wasChanged;
}

void
nsListControlFrame::InitSelectionRange(PRInt32 aClickedIndex)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRInt32 selectedIndex = GetSelectedIndex();
  if (selectedIndex >= 0) {
    
    nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
    NS_ASSERTION(options, "Collection of options is null!");
    PRUint32 numOptions;
    options->GetLength(&numOptions);
    PRUint32 i;
    
    for (i=selectedIndex+1; i < numOptions; i++) {
      PRBool selected;
      nsCOMPtr<nsIDOMHTMLOptionElement> option = GetOption(options, i);
      option->GetSelected(&selected);
      if (!selected) {
        break;
      }
    }

    if (aClickedIndex < selectedIndex) {
      
      
      mStartSelectionIndex = i-1;
      mEndSelectionIndex = selectedIndex;
    } else {
      
      
      mStartSelectionIndex = selectedIndex;
      mEndSelectionIndex = i-1;
    }
  }
}


PRBool
nsListControlFrame::PerformSelection(PRInt32 aClickedIndex,
                                     PRBool aIsShift,
                                     PRBool aIsControl)
{
  PRBool wasChanged = PR_FALSE;

  if (aClickedIndex == kNothingSelected) {
  }
  else if (GetMultiple()) {
    if (aIsShift) {
      
      
      if (mStartSelectionIndex == kNothingSelected) {
        InitSelectionRange(aClickedIndex);
      }

      
      
      PRInt32 startIndex;
      PRInt32 endIndex;
      if (mStartSelectionIndex == kNothingSelected) {
        startIndex = aClickedIndex;
        endIndex   = aClickedIndex;
      } else if (mStartSelectionIndex <= aClickedIndex) {
        startIndex = mStartSelectionIndex;
        endIndex   = aClickedIndex;
      } else {
        startIndex = aClickedIndex;
        endIndex   = mStartSelectionIndex;
      }

      
      wasChanged = ExtendedSelection(startIndex, endIndex, !aIsControl);
      ScrollToIndex(aClickedIndex);

      if (mStartSelectionIndex == kNothingSelected) {
        mStartSelectionIndex = aClickedIndex;
        mEndSelectionIndex = aClickedIndex;
      } else {
        mEndSelectionIndex = aClickedIndex;
      }
      InvalidateFocus();
    } else if (aIsControl) {
      wasChanged = SingleSelection(aClickedIndex, PR_TRUE);
    } else {
      wasChanged = SingleSelection(aClickedIndex, PR_FALSE);
    }
  } else {
    wasChanged = SingleSelection(aClickedIndex, PR_FALSE);
  }

  return wasChanged;
}


PRBool
nsListControlFrame::HandleListSelection(nsIDOMEvent* aEvent,
                                        PRInt32 aClickedIndex)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
  PRBool isShift;
  PRBool isControl;
#ifdef XP_MACOSX
  mouseEvent->GetMetaKey(&isControl);
#else
  mouseEvent->GetCtrlKey(&isControl);
#endif
  mouseEvent->GetShiftKey(&isShift);
  return PerformSelection(aClickedIndex, isShift, isControl);
}


void
nsListControlFrame::CaptureMouseEvents(PRBool aGrabMouseEvents)
{
  
  
  
  
  
  if (aGrabMouseEvents && IsInDropDownMode() && nsComboboxControlFrame::ToolkitHasNativePopup())
    return;
  
  nsIView* view = GetScrolledFrame()->GetView();

  NS_ASSERTION(view, "no view???");
  if (NS_UNLIKELY(!view))
    return;

  nsIViewManager* viewMan = view->GetViewManager();
  if (viewMan) {
    PRBool result;
    
    if (aGrabMouseEvents) {
      viewMan->GrabMouseEvents(view, result);
    } else {
      nsIView* curGrabber;
      viewMan->GetMouseEventGrabber(curGrabber);
      PRBool dropDownIsHidden = PR_FALSE;
      if (IsInDropDownMode()) {
        dropDownIsHidden = !mComboboxFrame->IsDroppedDown();
      }
      if (curGrabber == view || dropDownIsHidden) {
        
        
        
        
        
        
        viewMan->GrabMouseEvents(nsnull, result);
      }
    }
  }
}


NS_IMETHODIMP 
nsListControlFrame::HandleEvent(nsPresContext* aPresContext, 
                                nsGUIEvent*    aEvent,
                                nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  























  if (nsEventStatus_eConsumeNoDefault == *aEventStatus)
    return NS_OK;

  
  
  const nsStyleUserInterface* uiStyle = GetStyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled))
    return NS_OK;

  return nsHTMLScrollFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}



NS_IMETHODIMP
nsListControlFrame::SetInitialChildList(nsIAtom*       aListName,
                                        nsFrameList&   aChildList)
{
  
  mIsAllContentHere = mContent->IsDoneAddingChildren();
  if (!mIsAllContentHere) {
    mIsAllFramesHere    = PR_FALSE;
    mHasBeenInitialized = PR_FALSE;
  }
  nsresult rv = nsHTMLScrollFrame::SetInitialChildList(aListName, aChildList);

  
  
  








  return rv;
}


nsresult
nsListControlFrame::GetSizeAttribute(PRInt32 *aSize) {
  nsresult rv = NS_OK;
  nsIDOMHTMLSelectElement* selectElement;
  rv = mContent->QueryInterface(NS_GET_IID(nsIDOMHTMLSelectElement),(void**) &selectElement);
  if (mContent && NS_SUCCEEDED(rv)) {
    rv = selectElement->GetSize(aSize);
    NS_RELEASE(selectElement);
  }
  return rv;
}



NS_IMETHODIMP  
nsListControlFrame::Init(nsIContent*     aContent,
                         nsIFrame*       aParent,
                         nsIFrame*       aPrevInFlow)
{
  nsresult result = nsHTMLScrollFrame::Init(aContent, aParent, aPrevInFlow);

  
  NS_ENSURE_STATE(mContent);

  
  
  
  
  mEventListener = new nsListEventListener(this);
  if (!mEventListener) 
    return NS_ERROR_OUT_OF_MEMORY;

  mContent->AddEventListenerByIID(static_cast<nsIDOMMouseListener*>
                                             (mEventListener),
                                  NS_GET_IID(nsIDOMMouseListener));

  mContent->AddEventListenerByIID(static_cast<nsIDOMMouseMotionListener*>
                                             (mEventListener),
                                  NS_GET_IID(nsIDOMMouseMotionListener));

  mContent->AddEventListenerByIID(static_cast<nsIDOMKeyListener*>
                                             (mEventListener),
                                  NS_GET_IID(nsIDOMKeyListener));

  mStartSelectionIndex = kNothingSelected;
  mEndSelectionIndex = kNothingSelected;

  return result;
}

PRBool
nsListControlFrame::GetMultiple(nsIDOMHTMLSelectElement* aSelect) const
{
  PRBool multiple = PR_FALSE;
  nsresult rv = NS_OK;
  if (aSelect) {
    rv = aSelect->GetMultiple(&multiple);
  } else {
    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement = 
       do_QueryInterface(mContent);
  
    if (selectElement) {
      rv = selectElement->GetMultiple(&multiple);
    }
  }
  if (NS_SUCCEEDED(rv)) {
    return multiple;
  }
  return PR_FALSE;
}

already_AddRefed<nsIContent> 
nsListControlFrame::GetOptionAsContent(nsIDOMHTMLOptionsCollection* aCollection, PRInt32 aIndex) 
{
  nsIContent * content = nsnull;
  nsCOMPtr<nsIDOMHTMLOptionElement> optionElement = GetOption(aCollection,
                                                              aIndex);

  NS_ASSERTION(optionElement != nsnull, "could not get option element by index!");

  if (optionElement) {
    CallQueryInterface(optionElement, &content);
  }
 
  return content;
}

already_AddRefed<nsIContent> 
nsListControlFrame::GetOptionContent(PRInt32 aIndex) const
  
{
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
  NS_ASSERTION(options.get() != nsnull, "Collection of options is null!");

  if (options) {
    return GetOptionAsContent(options, aIndex);
  } 
  return nsnull;
}

already_AddRefed<nsIDOMHTMLOptionsCollection>
nsListControlFrame::GetOptions(nsIContent * aContent)
{
  nsIDOMHTMLOptionsCollection* options = nsnull;
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement = do_QueryInterface(aContent);
  if (selectElement) {
    selectElement->GetOptions(&options);  
  }

  return options;
}

already_AddRefed<nsIDOMHTMLOptionElement>
nsListControlFrame::GetOption(nsIDOMHTMLOptionsCollection* aCollection,
                              PRInt32 aIndex)
{
  nsCOMPtr<nsIDOMNode> node;
  if (NS_SUCCEEDED(aCollection->Item(aIndex, getter_AddRefs(node)))) {
    NS_ASSERTION(node,
                 "Item was successful, but node from collection was null!");
    if (node) {
      nsIDOMHTMLOptionElement* option = nsnull;
      CallQueryInterface(node, &option);

      return option;
    }
  } else {
    NS_ERROR("Couldn't get option by index from collection!");
  }
  return nsnull;
}

PRBool 
nsListControlFrame::IsContentSelected(nsIContent* aContent) const
{
  PRBool isSelected = PR_FALSE;

  nsCOMPtr<nsIDOMHTMLOptionElement> optEl = do_QueryInterface(aContent);
  if (optEl)
    optEl->GetSelected(&isSelected);

  return isSelected;
}

PRBool 
nsListControlFrame::IsContentSelectedByIndex(PRInt32 aIndex) const 
{
  nsCOMPtr<nsIContent> content = GetOptionContent(aIndex);
  NS_ASSERTION(content, "Failed to retrieve option content");

  return IsContentSelected(content);
}

NS_IMETHODIMP
nsListControlFrame::OnOptionSelected(PRInt32 aIndex, PRBool aSelected)
{
  if (aSelected) {
    ScrollToIndex(aIndex);
  }
  return NS_OK;
}

PRIntn
nsListControlFrame::GetSkipSides() const
{    
    
  return 0;
}

void
nsListControlFrame::OnContentReset()
{
  ResetList(PR_TRUE);
}

void 
nsListControlFrame::ResetList(PRBool aAllowScrolling)
{
  
  
  if (!mIsAllFramesHere) {
    return;
  }

  if (aAllowScrolling) {
    mPostChildrenLoadedReset = PR_TRUE;

    
    PRInt32 indexToSelect = kNothingSelected;

    nsCOMPtr<nsIDOMHTMLSelectElement> selectElement(do_QueryInterface(mContent));
    NS_ASSERTION(selectElement, "No select element!");
    if (selectElement) {
      selectElement->GetSelectedIndex(&indexToSelect);
      ScrollToIndex(indexToSelect);
    }
  }

  mStartSelectionIndex = kNothingSelected;
  mEndSelectionIndex = kNothingSelected;
  InvalidateFocus();
  
} 
 
void 
nsListControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  InvalidateFocus();

  if (aOn) {
    ComboboxFocusSet();
    mFocused = this;
  } else {
    mFocused = nsnull;
  }

  InvalidateFocus();
}

void nsListControlFrame::ComboboxFocusSet()
{
  gLastKeyTime = 0;
}

void
nsListControlFrame::SetComboboxFrame(nsIFrame* aComboboxFrame)
{
  if (nsnull != aComboboxFrame) {
    mComboboxFrame = do_QueryFrame(aComboboxFrame);
  }
}

void
nsListControlFrame::GetOptionText(PRInt32 aIndex, nsAString & aStr)
{
  aStr.SetLength(0);
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);

  if (options) {
    PRUint32 numOptions;
    options->GetLength(&numOptions);

    if (numOptions != 0) {
      nsCOMPtr<nsIDOMHTMLOptionElement> optionElement =
        GetOption(options, aIndex);
      if (optionElement) {
#if 0 
        nsAutoString text;
        optionElement->GetLabel(text);
        
        
        if (!text.IsEmpty()) { 
          nsAutoString compressText = text;
          compressText.CompressWhitespace(PR_TRUE, PR_TRUE);
          if (!compressText.IsEmpty()) {
            text = compressText;
          }
        }

        if (text.IsEmpty()) {
          
          
          optionElement->GetText(text);
        }          
        aStr = text;
#else
        optionElement->GetText(aStr);
#endif
      }
    }
  }
}

PRInt32
nsListControlFrame::GetSelectedIndex()
{
  PRInt32 aIndex;
  
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement(do_QueryInterface(mContent));
  selectElement->GetSelectedIndex(&aIndex);
  
  return aIndex;
}

PRBool 
nsListControlFrame::IsInDropDownMode() const
{
  return (mComboboxFrame != nsnull);
}

PRInt32
nsListControlFrame::GetNumberOfOptions()
{
  if (mContent != nsnull) {
    nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);

    if (!options) {
      return 0;
    } else {
      PRUint32 length = 0;
      options->GetLength(&length);
      return (PRInt32)length;
    }
  }
  return 0;
}




PRBool nsListControlFrame::CheckIfAllFramesHere()
{
  
  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  if (node) {
    
    
    mIsAllFramesHere = PR_TRUE;
  }
  

  return mIsAllFramesHere;
}

NS_IMETHODIMP
nsListControlFrame::DoneAddingChildren(PRBool aIsDone)
{
  mIsAllContentHere = aIsDone;
  if (mIsAllContentHere) {
    
    
    
    if (mIsAllFramesHere == PR_FALSE) {
      
      if (CheckIfAllFramesHere()) {
        mHasBeenInitialized = PR_TRUE;
        ResetList(PR_TRUE);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::AddOption(PRInt32 aIndex)
{
#ifdef DO_REFLOW_DEBUG
  printf("---- Id: %d nsLCF %p Added Option %d\n", mReflowId, this, aIndex);
#endif

  if (!mIsAllContentHere) {
    mIsAllContentHere = mContent->IsDoneAddingChildren();
    if (!mIsAllContentHere) {
      mIsAllFramesHere    = PR_FALSE;
      mHasBeenInitialized = PR_FALSE;
    } else {
      mIsAllFramesHere = (aIndex == GetNumberOfOptions()-1);
    }
  }
  
  
  mNeedToReset = PR_TRUE;

  if (!mHasBeenInitialized) {
    return NS_OK;
  }

  mPostChildrenLoadedReset = mIsAllContentHere;
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::RemoveOption(PRInt32 aIndex)
{
  
  if (IsInDropDownMode()) {
    mNeedToReset = PR_TRUE;
    mPostChildrenLoadedReset = mIsAllContentHere;
  }

  if (mStartSelectionIndex >= aIndex) {
    --mStartSelectionIndex;
    if (mStartSelectionIndex < 0) {
      mStartSelectionIndex = kNothingSelected;
    }    
  }

  if (mEndSelectionIndex >= aIndex) {
    --mEndSelectionIndex;
    if (mEndSelectionIndex < 0) {
      mEndSelectionIndex = kNothingSelected;
    }    
  }

  InvalidateFocus();
  return NS_OK;
}





PRBool
nsListControlFrame::SetOptionsSelectedFromFrame(PRInt32 aStartIndex,
                                                PRInt32 aEndIndex,
                                                PRBool aValue,
                                                PRBool aClearAll)
{
  nsCOMPtr<nsISelectElement> selectElement(do_QueryInterface(mContent));
  PRBool wasChanged = PR_FALSE;
#ifdef DEBUG
  nsresult rv = 
#endif
    selectElement->SetOptionsSelectedByIndex(aStartIndex,
                                             aEndIndex,
                                             aValue,
                                             aClearAll,
                                             PR_FALSE,
                                             PR_TRUE,
                                             &wasChanged);
  NS_ASSERTION(NS_SUCCEEDED(rv), "SetSelected failed");
  return wasChanged;
}

PRBool
nsListControlFrame::ToggleOptionSelectedFromFrame(PRInt32 aIndex)
{
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
  NS_ASSERTION(options, "No options");
  if (!options) {
    return PR_FALSE;
  }
  nsCOMPtr<nsIDOMHTMLOptionElement> option = GetOption(options, aIndex);
  NS_ASSERTION(option, "No option");
  if (!option) {
    return PR_FALSE;
  }

  PRBool value = PR_FALSE;
  nsresult rv = option->GetSelected(&value);

  NS_ASSERTION(NS_SUCCEEDED(rv), "GetSelected failed");
  nsCOMPtr<nsISelectElement> selectElement(do_QueryInterface(mContent));
  PRBool wasChanged = PR_FALSE;
  rv = selectElement->SetOptionsSelectedByIndex(aIndex,
                                                aIndex,
                                                !value,
                                                PR_FALSE,
                                                PR_FALSE,
                                                PR_TRUE,
                                                &wasChanged);

  NS_ASSERTION(NS_SUCCEEDED(rv), "SetSelected failed");

  return wasChanged;
}



PRBool
nsListControlFrame::UpdateSelection()
{
  if (mIsAllFramesHere) {
    
    nsWeakFrame weakFrame(this);
    if (mComboboxFrame) {
      mComboboxFrame->RedisplaySelectedText();
    }
    
    else if (mIsAllContentHere) {
      FireOnChange();
    }
    return weakFrame.IsAlive();
  }
  return PR_TRUE;
}

void
nsListControlFrame::ComboboxFinish(PRInt32 aIndex)
{
  gLastKeyTime = 0;

  if (mComboboxFrame) {
    PerformSelection(aIndex, PR_FALSE, PR_FALSE);

    PRInt32 displayIndex = mComboboxFrame->GetIndexOfDisplayArea();

    nsWeakFrame weakFrame(this);

    if (displayIndex != aIndex) {
      mComboboxFrame->RedisplaySelectedText(); 
    }

    if (weakFrame.IsAlive() && mComboboxFrame) {
      mComboboxFrame->RollupFromList(); 
    }
  }
}


void
nsListControlFrame::FireOnChange()
{
  if (mComboboxFrame) {
    
    PRInt32 index = mComboboxFrame->UpdateRecentIndex(NS_SKIP_NOTIFY_INDEX);
    if (index == NS_SKIP_NOTIFY_INDEX)
      return;

    
    if (index == GetSelectedIndex())
      return;
  }

  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event(PR_TRUE, NS_FORM_CHANGE);

  nsCOMPtr<nsIPresShell> presShell = PresContext()->GetPresShell();
  if (presShell) {
    presShell->HandleEventWithTarget(&event, this, nsnull, &status);
  }
}


NS_IMETHODIMP
nsListControlFrame::GetOptionSelected(PRInt32 aIndex, PRBool* aValue)
{
  *aValue = IsContentSelectedByIndex(aIndex);
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  if (mComboboxFrame) {
    
    
    mComboboxFrame->UpdateRecentIndex(NS_SKIP_NOTIFY_INDEX);
  }

  ScrollToIndex(aNewIndex);
  mStartSelectionIndex = aNewIndex;
  mEndSelectionIndex = aNewIndex;
  InvalidateFocus();

#ifdef ACCESSIBILITY
  FireMenuItemActiveEvent();
#endif

  return NS_OK;
}





nsresult
nsListControlFrame::SetFormProperty(nsIAtom* aName,
                                const nsAString& aValue)
{
  if (nsGkAtoms::selected == aName) {
    return NS_ERROR_INVALID_ARG; 
  } else if (nsGkAtoms::selectedindex == aName) {
    
    return NS_ERROR_INVALID_ARG;
  }

  
  

  return NS_OK;
}

nsresult 
nsListControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  
  if (nsGkAtoms::selected == aName) {
    nsAutoString val(aValue);
    PRInt32 error = 0;
    PRBool selected = PR_FALSE;
    PRInt32 indx = val.ToInteger(&error, 10); 
    if (error == 0)
       selected = IsContentSelectedByIndex(indx); 
  
    aValue.Assign(selected ? NS_LITERAL_STRING("1") : NS_LITERAL_STRING("0"));
    
  
  } else if (nsGkAtoms::selectedindex == aName) {
    
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

void
nsListControlFrame::SyncViewWithFrame()
{
    
    
    
    
  mComboboxFrame->AbsolutelyPositionDropDown();

  nsContainerFrame::PositionFrameView(this);
}

void
nsListControlFrame::AboutToDropDown()
{
  if (mIsAllContentHere && mIsAllFramesHere && mHasBeenInitialized) {
    ScrollToIndex(GetSelectedIndex());
#ifdef ACCESSIBILITY
    FireMenuItemActiveEvent(); 
#endif
  }
  mItemSelectionStarted = PR_FALSE;
}


void
nsListControlFrame::AboutToRollup()
{
  
  
  
  
  
  
  
  

  if (IsInDropDownMode()) {
    ComboboxFinish(mComboboxFrame->GetIndexOfDisplayArea()); 
  }
}

NS_IMETHODIMP
nsListControlFrame::DidReflow(nsPresContext*           aPresContext,
                              const nsHTMLReflowState* aReflowState,
                              nsDidReflowStatus        aStatus)
{
  nsresult rv;
  
  if (IsInDropDownMode()) 
  {
    
    rv = nsHTMLScrollFrame::DidReflow(aPresContext, aReflowState, aStatus);
    SyncViewWithFrame();
  } else {
    rv = nsHTMLScrollFrame::DidReflow(aPresContext, aReflowState, aStatus);
  }

  if (mNeedToReset) {
    mNeedToReset = PR_FALSE;
    
    
    
    
    
    
    
    
    
    ResetList(!DidHistoryRestore() || mPostChildrenLoadedReset);
  }

  return rv;
}

nsIAtom*
nsListControlFrame::GetType() const
{
  return nsGkAtoms::listControlFrame; 
}

PRBool
nsListControlFrame::IsContainingBlock() const
{
  
  
  
  return PR_TRUE;
}

void
nsListControlFrame::InvalidateInternal(const nsRect& aDamageRect,
                                       nscoord aX, nscoord aY, nsIFrame* aForChild,
                                       PRUint32 aFlags)
{
  if (!IsInDropDownMode()) {
    nsHTMLScrollFrame::InvalidateInternal(aDamageRect, aX, aY, this, aFlags);
    return;
  }
  InvalidateRoot(aDamageRect + nsPoint(aX, aY), aFlags);
}

#ifdef DEBUG
NS_IMETHODIMP
nsListControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ListControl"), aResult);
}
#endif

nscoord
nsListControlFrame::GetHeightOfARow()
{
  return HeightOfARow();
}

nsresult
nsListControlFrame::IsOptionDisabled(PRInt32 anIndex, PRBool &aIsDisabled)
{
  nsCOMPtr<nsISelectElement> sel(do_QueryInterface(mContent));
  if (sel) {
    sel->IsOptionDisabled(anIndex, &aIsDisabled);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}




PRBool
nsListControlFrame::IsLeftButton(nsIDOMEvent* aMouseEvent)
{
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  if (mouseEvent) {
    PRUint16 whichButton;
    if (NS_SUCCEEDED(mouseEvent->GetButton(&whichButton))) {
      return whichButton != 0?PR_FALSE:PR_TRUE;
    }
  }
  return PR_FALSE;
}

nscoord
nsListControlFrame::CalcFallbackRowHeight()
{
  nscoord rowHeight = 0;
  
  nsCOMPtr<nsIFontMetrics> fontMet;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));
  if (fontMet) {
    fontMet->GetHeight(rowHeight);
  }

  return rowHeight;
}

nscoord
nsListControlFrame::CalcIntrinsicHeight(nscoord aHeightOfARow,
                                        PRInt32 aNumberOfOptions)
{
  NS_PRECONDITION(!IsInDropDownMode(),
                  "Shouldn't be in dropdown mode when we call this");
  
  mNumDisplayRows = 1;
  GetSizeAttribute(&mNumDisplayRows);

  
  nscoord extraHeight = 0;
  
  if (mNumDisplayRows < 1) {
    
    
    
    nscoord labelHeight =
      ::GetOptGroupLabelsHeight(PresContext(), mContent, aHeightOfARow);

    if (GetMultiple()) {
      if (aNumberOfOptions < 2) {
        
        mNumDisplayRows = 1;
        extraHeight = PR_MAX(aHeightOfARow, labelHeight);
      }
      else if (aNumberOfOptions * aHeightOfARow + labelHeight >
               kMaxDropDownRows * aHeightOfARow) {
        mNumDisplayRows = kMaxDropDownRows;
      } else {
        mNumDisplayRows = aNumberOfOptions;
        extraHeight = labelHeight;
      }
    }
    else {
      NS_NOTREACHED("Shouldn't hit this case -- we should a be a combobox if "
                    "we have no size set and no multiple set!");
    }
  }

  return mNumDisplayRows * aHeightOfARow + extraHeight;
}




nsresult
nsListControlFrame::MouseUp(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent != nsnull, "aMouseEvent is null.");

  UpdateInListState(aMouseEvent);

  mButtonDown = PR_FALSE;

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return NS_OK;
  }

  
  
  
  if (!IsLeftButton(aMouseEvent)) {
    if (IsInDropDownMode()) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        aMouseEvent->PreventDefault();
        aMouseEvent->StopPropagation();
      } else {
        CaptureMouseEvents(PR_FALSE);
        return NS_OK;
      }
      CaptureMouseEvents(PR_FALSE);
      return NS_ERROR_FAILURE; 
    } else {
      CaptureMouseEvents(PR_FALSE);
      return NS_OK;
    }
  }

  const nsStyleVisibility* vis = GetStyleVisibility();
      
  if (!vis->IsVisible()) {
    return NS_OK;
  }

  if (IsInDropDownMode()) {
    
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aMouseEvent));
    nsMouseEvent * mouseEvent;
    mouseEvent = (nsMouseEvent *) privateEvent->GetInternalNSEvent();

    PRInt32 selectedIndex;
    if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
      
      PRBool isDisabled = PR_FALSE;
      IsOptionDisabled(selectedIndex, isDisabled);
      if (isDisabled) {
        aMouseEvent->PreventDefault();
        aMouseEvent->StopPropagation();
        CaptureMouseEvents(PR_FALSE);
        return NS_ERROR_FAILURE;
      }

      if (kNothingSelected != selectedIndex) {
        nsWeakFrame weakFrame(this);
        ComboboxFinish(selectedIndex);
        if (!weakFrame.IsAlive())
          return NS_OK;
        FireOnChange();
      }

      mouseEvent->clickCount = 1;
    } else {
      
      mouseEvent->clickCount = IgnoreMouseEventForSelection(aMouseEvent) ? 1 : 0;
    }
  } else {
    CaptureMouseEvents(PR_FALSE);
    
    if (mChangesSinceDragStart) {
      
      
      mChangesSinceDragStart = PR_FALSE;
      FireOnChange();
    }
  }

  return NS_OK;
}

void
nsListControlFrame::UpdateInListState(nsIDOMEvent* aEvent)
{
  if (!mComboboxFrame || !mComboboxFrame->IsDroppedDown())
    return;

  nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aEvent, this);
  nsRect borderInnerEdge = GetScrollableView()->View()->GetBounds();
  if (pt.y >= borderInnerEdge.y && pt.y < borderInnerEdge.YMost()) {
    mItemSelectionStarted = PR_TRUE;
  }
}

PRBool nsListControlFrame::IgnoreMouseEventForSelection(nsIDOMEvent* aEvent)
{
  if (!mComboboxFrame)
    return PR_FALSE;

  
  
  if (!mComboboxFrame->IsDroppedDown())
    return PR_TRUE;

  return !mItemSelectionStarted;
}

#ifdef ACCESSIBILITY
void
nsListControlFrame::FireMenuItemActiveEvent()
{
  if (mFocused != this && !IsInDropDownMode()) {
    return;
  }

  
  
  PRInt32 focusedIndex;
  if (mEndSelectionIndex == kNothingSelected) {
    focusedIndex = GetSelectedIndex();
  } else {
    focusedIndex = mEndSelectionIndex;
  }
  if (focusedIndex == kNothingSelected) {
    return;
  }

  nsCOMPtr<nsIContent> optionContent = GetOptionContent(focusedIndex);
  if (!optionContent) {
    return;
  }

  FireDOMEvent(NS_LITERAL_STRING("DOMMenuItemActive"), optionContent);
}
#endif

nsresult
nsListControlFrame::GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, 
                                         PRInt32&     aCurIndex)
{
  if (IgnoreMouseEventForSelection(aMouseEvent))
    return NS_ERROR_FAILURE;

  nsIView* view = GetScrolledFrame()->GetView();
  nsIViewManager* viewMan = view->GetViewManager();
  nsIView* curGrabber;
  viewMan->GetMouseEventGrabber(curGrabber);
  if (curGrabber != view) {
    
    nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aMouseEvent, this);
    nsRect borderInnerEdge = GetScrollableView()->View()->GetBounds();
    if (!borderInnerEdge.Contains(pt)) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCOMPtr<nsIContent> content;
  PresContext()->EventStateManager()->
    GetEventTargetContent(nsnull, getter_AddRefs(content));

  nsCOMPtr<nsIContent> optionContent = GetOptionFromContent(content);
  if (optionContent) {
    aCurIndex = GetIndexFromContent(optionContent);
    return NS_OK;
  }

  nsIPresShell *presShell = PresContext()->PresShell();
  PRInt32 numOptions = GetNumberOfOptions();
  if (numOptions < 1)
    return NS_ERROR_FAILURE;

  nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aMouseEvent, this);

  
  
  nsCOMPtr<nsIContent> firstOption = GetOptionContent(0);
  NS_ASSERTION(firstOption, "Can't find first option that's supposed to be there");
  nsIFrame* optionFrame = presShell->GetPrimaryFrameFor(firstOption);
  if (optionFrame) {
    nsPoint ptInOptionFrame = pt - optionFrame->GetOffsetTo(this);
    if (ptInOptionFrame.y < 0 && ptInOptionFrame.x >= 0 &&
        ptInOptionFrame.x < optionFrame->GetSize().width) {
      aCurIndex = 0;
      return NS_OK;
    }
  }

  nsCOMPtr<nsIContent> lastOption = GetOptionContent(numOptions - 1);
  
  
  NS_ASSERTION(lastOption, "Can't find last option that's supposed to be there");
  optionFrame = presShell->GetPrimaryFrameFor(lastOption);
  if (optionFrame) {
    nsPoint ptInOptionFrame = pt - optionFrame->GetOffsetTo(this);
    if (ptInOptionFrame.y >= optionFrame->GetSize().height && ptInOptionFrame.x >= 0 &&
        ptInOptionFrame.x < optionFrame->GetSize().width) {
      aCurIndex = numOptions - 1;
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsListControlFrame::MouseDown(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent != nsnull, "aMouseEvent is null.");

  UpdateInListState(aMouseEvent);

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return NS_OK;
  }

  
  
  
  if (!IsLeftButton(aMouseEvent)) {
    if (IsInDropDownMode()) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        aMouseEvent->PreventDefault();
        aMouseEvent->StopPropagation();
      } else {
        return NS_OK;
      }
      return NS_ERROR_FAILURE; 
    } else {
      return NS_OK;
    }
  }

  PRInt32 selectedIndex;
  if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
    
    mButtonDown = PR_TRUE;
    CaptureMouseEvents(PR_TRUE);
    mChangesSinceDragStart = HandleListSelection(aMouseEvent, selectedIndex);
#ifdef ACCESSIBILITY
    if (mChangesSinceDragStart) {
      FireMenuItemActiveEvent();
    }
#endif
  } else {
    
    if (mComboboxFrame) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        return NS_OK;
      }

      if (!nsComboboxControlFrame::ToolkitHasNativePopup())
      {
        PRBool isDroppedDown = mComboboxFrame->IsDroppedDown();
        nsIFrame* comboFrame = do_QueryFrame(mComboboxFrame);
        nsWeakFrame weakFrame(comboFrame);
        mComboboxFrame->ShowDropDown(!isDroppedDown);
        if (!weakFrame.IsAlive())
          return NS_OK;
        if (isDroppedDown) {
          CaptureMouseEvents(PR_FALSE);
        }
      }
    }
  }

  return NS_OK;
}




nsresult
nsListControlFrame::MouseMove(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent, "aMouseEvent is null.");

  UpdateInListState(aMouseEvent);

  if (IsInDropDownMode()) { 
    if (mComboboxFrame->IsDroppedDown()) {
      PRInt32 selectedIndex;
      if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
        PerformSelection(selectedIndex, PR_FALSE, PR_FALSE);
      }
    }
  } else {
    if (mButtonDown) {
      return DragMove(aMouseEvent);
    }
  }
  return NS_OK;
}

nsresult
nsListControlFrame::DragMove(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent, "aMouseEvent is null.");

  UpdateInListState(aMouseEvent);

  if (!IsInDropDownMode()) { 
    PRInt32 selectedIndex;
    if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
      
      if (selectedIndex == mEndSelectionIndex) {
        return NS_OK;
      }
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
      NS_ASSERTION(mouseEvent, "aMouseEvent is not an nsIDOMMouseEvent!");
      PRBool isControl;
#ifdef XP_MACOSX
      mouseEvent->GetMetaKey(&isControl);
#else
      mouseEvent->GetCtrlKey(&isControl);
#endif
      
      PRBool wasChanged = PerformSelection(selectedIndex,
                                           !isControl, isControl);
      mChangesSinceDragStart = mChangesSinceDragStart || wasChanged;
    }
  }
  return NS_OK;
}




nsresult
nsListControlFrame::ScrollToIndex(PRInt32 aIndex)
{
  if (aIndex < 0) {
    
    
    return ScrollToFrame(nsnull);
  } else {
    nsCOMPtr<nsIContent> content = GetOptionContent(aIndex);
    if (content) {
      return ScrollToFrame(content);
    }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsListControlFrame::ScrollToFrame(nsIContent* aOptElement)
{
  nsIScrollableView* scrollableView = GetScrollableView();

  if (scrollableView) {
    
    if (nsnull == aOptElement) {
      scrollableView->ScrollTo(0, 0, 0);
      return NS_OK;
    }
  
    
    nsIPresShell *presShell = PresContext()->PresShell();
    nsIFrame * childframe;
    if (aOptElement) {
      childframe = presShell->GetPrimaryFrameFor(aOptElement);
    } else {
      return NS_ERROR_FAILURE;
    }

    if (childframe) {
      if (scrollableView) {
        nscoord x;
        nscoord y;
        scrollableView->GetScrollPosition(x,y);
        
        nsRect rect = scrollableView->View()->GetBounds();
        
        rect.x = x;
        rect.y = y;

        
        nsRect fRect = childframe->GetRect();
        nsPoint pnt;
        nsIView * view;
        childframe->GetOffsetFromView(pnt, &view);

        

        
        
        
        
        
        nsCOMPtr<nsIContent> parentContent = aOptElement->GetParent();
        nsCOMPtr<nsIDOMHTMLOptGroupElement> optGroup(do_QueryInterface(parentContent));
        nsRect optRect(0,0,0,0);
        if (optGroup) {
          nsIFrame * optFrame = presShell->GetPrimaryFrameFor(parentContent);
          if (optFrame) {
            optRect = optFrame->GetRect();
          }
        }
        fRect.y += optRect.y;

        
        
        
        if (!(rect.y <= fRect.y && fRect.YMost() <= rect.YMost())) {
          
          if (fRect.YMost() > rect.YMost()) {
            y = fRect.y-(rect.height-fRect.height);
          } else {
            y = fRect.y;
          }
          scrollableView->ScrollTo(pnt.x, y, 0);
        }

      }
    }
  }
  return NS_OK;
}

















void
nsListControlFrame::AdjustIndexForDisabledOpt(PRInt32 aStartIndex,
                                              PRInt32 &aNewIndex,
                                              PRInt32 aNumOptions,
                                              PRInt32 aDoAdjustInc,
                                              PRInt32 aDoAdjustIncNext)
{
  
  if (aNumOptions == 0) {
    aNewIndex = kNothingSelected;
    return;
  }

  
  PRBool doingReverse = PR_FALSE;
  
  PRInt32 bottom      = 0;
  
  PRInt32 top         = aNumOptions;

  
  
  
  
  
  
  PRInt32 startIndex = aStartIndex;
  if (startIndex < bottom) {
    startIndex = GetSelectedIndex();
  }
  PRInt32 newIndex    = startIndex + aDoAdjustInc;

  
  if (newIndex < bottom) {
    newIndex = 0;
  } else if (newIndex >= top) {
    newIndex = aNumOptions-1;
  }

  while (1) {
    
    PRBool isDisabled = PR_TRUE;
    if (NS_SUCCEEDED(IsOptionDisabled(newIndex, isDisabled)) && !isDisabled) {
      break;
    }

    
    newIndex += aDoAdjustIncNext;

    
    if (newIndex < bottom) {
      if (doingReverse) {
        return; 
      } else {
        
        
        
        newIndex         = bottom;
        aDoAdjustIncNext = 1;
        doingReverse     = PR_TRUE;
        top              = startIndex;
      }
    } else  if (newIndex >= top) {
      if (doingReverse) {
        return;        
      } else {
        
        
        
        newIndex = top - 1;
        aDoAdjustIncNext = -1;
        doingReverse     = PR_TRUE;
        bottom           = startIndex;
      }
    }
  }

  
  aNewIndex     = newIndex;
}

nsAString& 
nsListControlFrame::GetIncrementalString()
{ 
  if (sIncrementalString == nsnull)
    sIncrementalString = new nsString();

  return *sIncrementalString;
}

void
nsListControlFrame::Shutdown()
{
  delete sIncrementalString;
  sIncrementalString = nsnull;
}

void
nsListControlFrame::DropDownToggleKey(nsIDOMEvent* aKeyEvent)
{
  
  
  if (IsInDropDownMode() && !nsComboboxControlFrame::ToolkitHasNativePopup()) {
    aKeyEvent->PreventDefault();
    if (!mComboboxFrame->IsDroppedDown()) {
      mComboboxFrame->ShowDropDown(PR_TRUE);
    } else {
      nsWeakFrame weakFrame(this);
      
      ComboboxFinish(mEndSelectionIndex);
      if (weakFrame.IsAlive()) {
        FireOnChange();
      }
    }
  }
}

nsresult
nsListControlFrame::KeyPress(nsIDOMEvent* aKeyEvent)
{
  NS_ASSERTION(aKeyEvent, "keyEvent is null.");

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled))
    return NS_OK;

  
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_FAILURE);

  PRUint32 keycode = 0;
  PRUint32 charcode = 0;
  keyEvent->GetKeyCode(&keycode);
  keyEvent->GetCharCode(&charcode);

  PRBool isAlt = PR_FALSE;

  keyEvent->GetAltKey(&isAlt);
  if (isAlt) {
    if (keycode == nsIDOMKeyEvent::DOM_VK_UP || keycode == nsIDOMKeyEvent::DOM_VK_DOWN) {
      DropDownToggleKey(aKeyEvent);
    }
    return NS_OK;
  }

  
  PRBool isControl = PR_FALSE;
  PRBool isShift   = PR_FALSE;
  keyEvent->GetCtrlKey(&isControl);
  if (!isControl) {
    keyEvent->GetMetaKey(&isControl);
  }
  keyEvent->GetShiftKey(&isShift);

  
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
  NS_ENSURE_TRUE(options, NS_ERROR_FAILURE);

  PRUint32 numOptions = 0;
  options->GetLength(&numOptions);

  
  PRBool didIncrementalSearch = PR_FALSE;
  
  
  
  PRInt32 newIndex = kNothingSelected;

  
  
  
  
  
  
  if (isControl && (keycode == nsIDOMKeyEvent::DOM_VK_UP ||
                    keycode == nsIDOMKeyEvent::DOM_VK_LEFT ||
                    keycode == nsIDOMKeyEvent::DOM_VK_DOWN ||
                    keycode == nsIDOMKeyEvent::DOM_VK_RIGHT)) {
    
    isControl = mControlSelectMode = GetMultiple();
  } else if (charcode != ' ') {
    mControlSelectMode = PR_FALSE;
  }
  switch (keycode) {

    case nsIDOMKeyEvent::DOM_VK_UP:
    case nsIDOMKeyEvent::DOM_VK_LEFT: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                -1, -1);
      } break;
    
    case nsIDOMKeyEvent::DOM_VK_DOWN:
    case nsIDOMKeyEvent::DOM_VK_RIGHT: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                1, 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_RETURN: {
      if (mComboboxFrame != nsnull) {
        if (mComboboxFrame->IsDroppedDown()) {
          nsWeakFrame weakFrame(this);
          ComboboxFinish(mEndSelectionIndex);
          if (!weakFrame.IsAlive())
            return NS_OK;
        }
        FireOnChange();
        return NS_OK;
      } else {
        newIndex = mEndSelectionIndex;
      }
      } break;

    case nsIDOMKeyEvent::DOM_VK_ESCAPE: {
      nsWeakFrame weakFrame(this);
      AboutToRollup();
      if (!weakFrame.IsAlive()) {
        aKeyEvent->PreventDefault(); 
        return NS_OK;
      }
    } break;

    case nsIDOMKeyEvent::DOM_VK_PAGE_UP: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                -(mNumDisplayRows-1), -1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_PAGE_DOWN: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (PRInt32)numOptions,
                                (mNumDisplayRows-1), 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_HOME: {
      AdjustIndexForDisabledOpt(0, newIndex,
                                (PRInt32)numOptions,
                                0, 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_END: {
      AdjustIndexForDisabledOpt(numOptions-1, newIndex,
                                (PRInt32)numOptions,
                                0, -1);
      } break;

#if defined(XP_WIN) || defined(XP_OS2)
    case nsIDOMKeyEvent::DOM_VK_F4: {
      DropDownToggleKey(aKeyEvent);
      return NS_OK;
    } break;
#endif

    case nsIDOMKeyEvent::DOM_VK_TAB: {
      return NS_OK;
    }

    default: { 
               
      
      if (isControl && charcode != ' ') {
        return NS_OK;
      }

      didIncrementalSearch = PR_TRUE;
      if (charcode == 0) {
        
        if (keycode == NS_VK_BACK && !GetIncrementalString().IsEmpty()) {
          GetIncrementalString().Truncate(GetIncrementalString().Length() - 1);
          aKeyEvent->PreventDefault();
        }
        return NS_OK;
      }
      
      DOMTimeStamp keyTime;
      aKeyEvent->GetTimeStamp(&keyTime);

      
      
      
      
      
      if (keyTime - gLastKeyTime > INCREMENTAL_SEARCH_KEYPRESS_TIME) {
        
        
        if (charcode == ' ') {
          newIndex = mEndSelectionIndex;
          break;
        }
        GetIncrementalString().Truncate();
      }
      gLastKeyTime = keyTime;

      
      PRUnichar uniChar = ToLowerCase(static_cast<PRUnichar>(charcode));
      GetIncrementalString().Append(uniChar);

      
      nsAutoString incrementalString(GetIncrementalString());
      PRUint32 charIndex = 1, stringLength = incrementalString.Length();
      while (charIndex < stringLength && incrementalString[charIndex] == incrementalString[charIndex - 1]) {
        charIndex++;
      }
      if (charIndex == stringLength) {
        incrementalString.Truncate(1);
        stringLength = 1;
      }

      
      
      
      
      
      
      PRInt32 startIndex = GetSelectedIndex();
      if (startIndex == kNothingSelected) {
        startIndex = 0;
      } else if (stringLength == 1) {
        startIndex++;
      }

      PRUint32 i;
      for (i = 0; i < numOptions; i++) {
        PRUint32 index = (i + startIndex) % numOptions;
        nsCOMPtr<nsIDOMHTMLOptionElement> optionElement =
          GetOption(options, index);
        if (optionElement) {
          nsAutoString text;
          if (NS_OK == optionElement->GetText(text)) {
            if (StringBeginsWith(text, incrementalString,
                                 nsCaseInsensitiveStringComparator())) {
              PRBool wasChanged = PerformSelection(index, isShift, isControl);
              if (wasChanged) {
                
                if (!UpdateSelection()) {
                  return NS_OK;
                }
#ifdef ACCESSIBILITY
                FireMenuItemActiveEvent(); 
#endif
              }
              break;
            }
          }
        }
      } 

    } break;
  } 

  
  aKeyEvent->PreventDefault();

  
  if (!didIncrementalSearch) {
    GetIncrementalString().Truncate();
  }

  
  
  if (newIndex != kNothingSelected) {
    
    PRBool wasChanged = PR_FALSE;
    if (isControl && charcode != ' ') {
      mStartSelectionIndex = newIndex;
      mEndSelectionIndex = newIndex;
      InvalidateFocus();
      ScrollToIndex(newIndex);
    } else if (mControlSelectMode && charcode == ' ') {
      wasChanged = SingleSelection(newIndex, PR_TRUE);
    } else {
      wasChanged = PerformSelection(newIndex, isShift, isControl);
    }
    if (wasChanged) {
       
      if (!UpdateSelection()) {
        return NS_OK;
      }
    }
#ifdef ACCESSIBILITY
    if (charcode != ' ') {
      FireMenuItemActiveEvent();
    }
#endif
  }

  return NS_OK;
}






NS_IMPL_ADDREF(nsListEventListener)
NS_IMPL_RELEASE(nsListEventListener)
NS_INTERFACE_MAP_BEGIN(nsListEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseMotionListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener, nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMouseListener)
NS_INTERFACE_MAP_END

#define FORWARD_EVENT(_event) \
NS_IMETHODIMP \
nsListEventListener::_event(nsIDOMEvent* aEvent) \
{ \
  if (mFrame) \
    return mFrame->nsListControlFrame::_event(aEvent); \
  return NS_OK; \
}

#define IGNORE_EVENT(_event) \
NS_IMETHODIMP \
nsListEventListener::_event(nsIDOMEvent* aEvent) \
{ return NS_OK; }

IGNORE_EVENT(HandleEvent)



IGNORE_EVENT(KeyDown)
IGNORE_EVENT(KeyUp)
FORWARD_EVENT(KeyPress)



FORWARD_EVENT(MouseDown)
FORWARD_EVENT(MouseUp)
IGNORE_EVENT(MouseClick)
IGNORE_EVENT(MouseDblClick)
IGNORE_EVENT(MouseOver)
IGNORE_EVENT(MouseOut)



FORWARD_EVENT(MouseMove)

IGNORE_EVENT(DragMove)

#undef FORWARD_EVENT
#undef IGNORE_EVENT

