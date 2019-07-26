




#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsListControlFrame.h"
#include "nsFormControlFrame.h" 
#include "nsGkAtoms.h"
#include "nsIFormControl.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsComboboxControlFrame.h"
#include "nsViewManager.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsWidgetsCID.h"
#include "nsIPresShell.h"
#include "nsHTMLParts.h"
#include "nsIDOMEventTarget.h"
#include "nsEventDispatcher.h"
#include "nsEventStateManager.h"
#include "nsEventListenerManager.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsFontMetrics.h"
#include "nsIScrollableFrame.h"
#include "nsGUIEvent.h"
#include "nsIServiceManager.h"
#include "nsINodeInfo.h"
#include "nsHTMLSelectElement.h"
#include "nsCSSRendering.h"
#include "nsITheme.h"
#include "nsIDOMEventListener.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsContentUtils.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/Attributes.h"
#include <algorithm>

using namespace mozilla;


const uint32_t kMaxDropDownRows         = 20; 
const int32_t kNothingSelected          = -1;


nsListControlFrame * nsListControlFrame::mFocused = nullptr;
nsString * nsListControlFrame::sIncrementalString = nullptr;


#define INCREMENTAL_SEARCH_KEYPRESS_TIME 1000





DOMTimeStamp nsListControlFrame::gLastKeyTime = 0;







class nsListEventListener MOZ_FINAL : public nsIDOMEventListener
{
public:
  nsListEventListener(nsListControlFrame *aFrame)
    : mFrame(aFrame) { }

  void SetFrame(nsListControlFrame *aFrame) { mFrame = aFrame; }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

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

NS_IMPL_FRAMEARENA_HELPERS(nsListControlFrame)


nsListControlFrame::nsListControlFrame(
  nsIPresShell* aShell, nsIDocument* aDocument, nsStyleContext* aContext)
  : nsHTMLScrollFrame(aShell, aContext, false),
    mMightNeedSecondPass(false),
    mHasPendingInterruptAtStartOfReflow(false),
    mDropdownCanGrow(false),
    mLastDropdownComputedHeight(NS_UNCONSTRAINEDSIZE)
{
  mComboboxFrame      = nullptr;
  mChangesSinceDragStart = false;
  mButtonDown         = false;

  mIsAllContentHere   = false;
  mIsAllFramesHere    = false;
  mHasBeenInitialized = false;
  mNeedToReset        = true;
  mPostChildrenLoadedReset = false;

  mControlSelectMode           = false;
}


nsListControlFrame::~nsListControlFrame()
{
  mComboboxFrame = nullptr;
}


void
nsListControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  ENSURE_TRUE(mContent);

  
  

  mEventListener->SetFrame(nullptr);

  mContent->RemoveEventListener(NS_LITERAL_STRING("keypress"), mEventListener,
                                false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mousedown"), mEventListener,
                                false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mouseup"), mEventListener,
                                false);
  mContent->RemoveEventListener(NS_LITERAL_STRING("mousemove"), mEventListener,
                                false);

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsHTMLScrollFrame::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
nsListControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  
  
  
  if (aBuilder->IsBackgroundOnly())
    return NS_OK;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsListControlFrame");

  if (IsInDropDownMode()) {
    NS_ASSERTION(NS_GET_A(mLastDropdownBackstopColor) == 255,
                 "need an opaque backstop color");
    
    
    
    aLists.BorderBackground()->AppendNewToBottom(
      new (aBuilder) nsDisplaySolidColor(aBuilder,
        this, nsRect(aBuilder->ToReferenceFrame(this), GetSize()),
        mLastDropdownBackstopColor));
  }

  
  
  
  
  
  return nsHTMLScrollFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}








void nsListControlFrame::PaintFocus(nsRenderingContext& aRC, nsPoint aPt)
{
  if (mFocused != this) return;

  nsPresContext* presContext = PresContext();

  nsIFrame* containerFrame = GetOptionsContainer();
  if (!containerFrame) return;

  nsIFrame* childframe = nullptr;
  nsCOMPtr<nsIContent> focusedContent = GetCurrentOption();
  if (focusedContent) {
    childframe = focusedContent->GetPrimaryFrame();
  }

  nsRect fRect;
  if (childframe) {
    
    fRect = childframe->GetRect();
    
    fRect.MoveBy(childframe->GetParent()->GetOffsetTo(this));
  } else {
    float inflation = nsLayoutUtils::FontSizeInflationFor(this);
    fRect.x = fRect.y = 0;
    fRect.width = GetScrollPortRect().width;
    fRect.height = CalcFallbackRowHeight(inflation);
    fRect.MoveBy(containerFrame->GetOffsetTo(this));
  }
  fRect += aPt;
  
  bool lastItemIsSelected = false;
  if (focusedContent) {
    nsCOMPtr<nsIDOMHTMLOptionElement> domOpt =
      do_QueryInterface(focusedContent);
    if (domOpt) {
      domOpt->GetSelected(&lastItemIsSelected);
    }
  }

  
  nscolor color =
    LookAndFeel::GetColor(lastItemIsSelected ?
                            LookAndFeel::eColorID_WidgetSelectForeground :
                            LookAndFeel::eColorID_WidgetSelectBackground);

  nsCSSRendering::PaintFocus(presContext, aRC, fRect, color);
}

void
nsListControlFrame::InvalidateFocus()
{
  if (mFocused != this)
    return;

  nsIFrame* containerFrame = GetOptionsContainer();
  if (containerFrame) {
    containerFrame->InvalidateFrame();
  }
}

NS_QUERYFRAME_HEAD(nsListControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
  NS_QUERYFRAME_ENTRY(nsIListControlFrame)
  NS_QUERYFRAME_ENTRY(nsISelectControlFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLScrollFrame)

#ifdef ACCESSIBILITY
a11y::AccType
nsListControlFrame::AccessibleType()
{
  return a11y::eHTMLSelectListType;
}
#endif

static nscoord
GetMaxOptionHeight(nsIFrame* aContainer)
{
  nscoord result = 0;
  for (nsIFrame* option = aContainer->GetFirstPrincipalChild();
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





nscoord
nsListControlFrame::CalcHeightOfARow()
{
  
  
  
  
  int32_t heightOfARow = GetMaxOptionHeight(GetOptionsContainer());

  
  
  if (heightOfARow == 0 && GetNumberOfOptions() == 0) {
    float inflation = nsLayoutUtils::FontSizeInflationFor(this);
    heightOfARow = CalcFallbackRowHeight(inflation);
  }

  return heightOfARow;
}

nscoord
nsListControlFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  
  
  
  result = GetScrolledFrame()->GetPrefWidth(aRenderingContext);
  result = NSCoordSaturatingAdd(result,
          GetDesiredScrollbarSizes(PresContext(), aRenderingContext).LeftRight());

  return result;
}

nscoord
nsListControlFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
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

  SchedulePaint();

  mHasPendingInterruptAtStartOfReflow = aPresContext->HasPendingInterrupt();

  
  
  if (mIsAllContentHere && !mHasBeenInitialized) {
    if (false == mIsAllFramesHere) {
      CheckIfAllFramesHere();
    }
    if (mIsAllFramesHere && !mHasBeenInitialized) {
      mHasBeenInitialized = true;
    }
  }

  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, true);
  }

  if (IsInDropDownMode()) {
    return ReflowAsDropdown(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  


















  bool autoHeight = (aReflowState.ComputedHeight() == NS_UNCONSTRAINEDSIZE);

  mMightNeedSecondPass = autoHeight &&
    (NS_SUBTREE_DIRTY(this) || aReflowState.ShouldReflowAllKids());
  
  nsHTMLReflowState state(aReflowState);
  int32_t length = GetNumberOfRows();

  nscoord oldHeightOfARow = HeightOfARow();

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW) && autoHeight) {
    
    
    nscoord computedHeight = CalcIntrinsicHeight(oldHeightOfARow, length);
    computedHeight = state.ApplyMinMaxHeight(computedHeight);
    state.SetComputedHeight(computedHeight);
  }

  nsresult rv = nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize,
                                          state, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mMightNeedSecondPass) {
    NS_ASSERTION(!autoHeight || HeightOfARow() == oldHeightOfARow,
                 "How did our height of a row change if nothing was dirty?");
    NS_ASSERTION(!autoHeight ||
                 !(GetStateBits() & NS_FRAME_FIRST_REFLOW),
                 "How do we not need a second pass during initial reflow at "
                 "auto height?");
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if we don't need a second pass!");
    if (!autoHeight) {
      
      
      
      
      
      nscoord rowHeight = CalcHeightOfARow();
      if (rowHeight == 0) {
        
        mNumDisplayRows = 1;
      } else {
        mNumDisplayRows = std::max(1, state.ComputedHeight() / rowHeight);
      }
    }

    return rv;
  }

  mMightNeedSecondPass = false;

  
  
  if (!IsScrollbarUpdateSuppressed()) {
    
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if the height of a row has not "
                 "changed!");
    return rv;
  }

  SetSuppressScrollbarUpdate(false);

  
  
  
  
  
  nsHTMLScrollFrame::DidReflow(aPresContext, &state,
                               nsDidReflowStatus::FINISHED);

  
  nscoord computedHeight = CalcIntrinsicHeight(HeightOfARow(), length); 
  computedHeight = state.ApplyMinMaxHeight(computedHeight);
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
  nscoord oldVisibleHeight = (GetStateBits() & NS_FRAME_FIRST_REFLOW) ?
    NS_UNCONSTRAINEDSIZE : GetScrolledFrame()->GetSize().height;
#endif

  nsHTMLReflowState state(aReflowState);

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    
    
    
    state.SetComputedHeight(mLastDropdownComputedHeight);
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

  mMightNeedSecondPass = false;

  
  
  if (!IsScrollbarUpdateSuppressed()) {
    
    NS_ASSERTION(!(GetStateBits() & NS_FRAME_FIRST_REFLOW),
                 "How can we avoid a second pass during first reflow?");
    return rv;
  }

  SetSuppressScrollbarUpdate(false);

  nscoord visibleHeight = GetScrolledFrame()->GetSize().height;
  nscoord heightOfARow = HeightOfARow();

  
  
  
  
  
  nsHTMLScrollFrame::DidReflow(aPresContext, &state,
                               nsDidReflowStatus::FINISHED);

  
  
  

  mDropdownCanGrow = false;
  if (visibleHeight <= 0 || heightOfARow <= 0) {
    
    state.SetComputedHeight(heightOfARow);
    mNumDisplayRows = 1;
  } else {
    nsComboboxControlFrame* combobox = static_cast<nsComboboxControlFrame*>(mComboboxFrame);
    nsPoint translation;
    nscoord above, below;
    combobox->GetAvailableDropdownSpace(&above, &below, &translation);
    if (above <= 0 && below <= 0) {
      state.SetComputedHeight(heightOfARow);
      mNumDisplayRows = 1;
      mDropdownCanGrow = GetNumberOfRows() > 1;
    } else {
      nscoord bp = aReflowState.mComputedBorderPadding.TopBottom();
      nscoord availableHeight = std::max(above, below) - bp;
      nscoord newHeight;
      uint32_t rows;
      if (visibleHeight <= availableHeight) {
        
        rows = GetNumberOfRows();
        mNumDisplayRows = clamped<uint32_t>(rows, 1, kMaxDropDownRows);
        if (mNumDisplayRows == rows) {
          newHeight = visibleHeight;  
        } else {
          newHeight = mNumDisplayRows * heightOfARow; 
        }
      } else {
        rows = availableHeight / heightOfARow;
        mNumDisplayRows = clamped<uint32_t>(rows, 1, kMaxDropDownRows);
        newHeight = mNumDisplayRows * heightOfARow; 
      }
      state.SetComputedHeight(newHeight);
      mDropdownCanGrow = visibleHeight - newHeight >= heightOfARow &&
                         mNumDisplayRows != kMaxDropDownRows;
    }
  }

  mLastDropdownComputedHeight = state.ComputedHeight();

  nsHTMLScrollFrame::WillReflow(aPresContext);
  return nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
}

nsGfxScrollFrameInner::ScrollbarStyles
nsListControlFrame::GetScrollbarStyles() const
{
  
  
  int32_t verticalStyle = IsInDropDownMode() ? NS_STYLE_OVERFLOW_AUTO
    : NS_STYLE_OVERFLOW_SCROLL;
  return nsGfxScrollFrameInner::ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN,
                                                verticalStyle);
}

bool
nsListControlFrame::ShouldPropagateComputedHeightToScrolledContent() const
{
  return !IsInDropDownMode();
}


nsIFrame*
nsListControlFrame::GetContentInsertionFrame() {
  return GetOptionsContainer()->GetContentInsertionFrame();
}





nsIContent *
nsListControlFrame::GetOptionFromContent(nsIContent *aContent) 
{
  for (nsIContent* content = aContent; content; content = content->GetParent()) {
    if (content->IsHTML(nsGkAtoms::option)) {
      return content;
    }
  }

  return nullptr;
}





int32_t 
nsListControlFrame::GetIndexFromContent(nsIContent *aContent)
{
  nsCOMPtr<nsIDOMHTMLOptionElement> option;
  option = do_QueryInterface(aContent);
  if (option) {
    int32_t retval;
    option->GetIndex(&retval);
    if (retval >= 0) {
      return retval;
    }
  }
  return kNothingSelected;
}


bool
nsListControlFrame::ExtendedSelection(int32_t aStartIndex,
                                      int32_t aEndIndex,
                                      bool aClearAll)
{
  return SetOptionsSelectedFromFrame(aStartIndex, aEndIndex,
                                     true, aClearAll);
}


bool
nsListControlFrame::SingleSelection(int32_t aClickedIndex, bool aDoToggle)
{
  if (mComboboxFrame) {
    mComboboxFrame->UpdateRecentIndex(GetSelectedIndex());
  }

  bool wasChanged = false;
  
  if (aDoToggle) {
    wasChanged = ToggleOptionSelectedFromFrame(aClickedIndex);
  } else {
    wasChanged = SetOptionsSelectedFromFrame(aClickedIndex, aClickedIndex,
                                true, true);
  }
  ScrollToIndex(aClickedIndex);

#ifdef ACCESSIBILITY
  bool isCurrentOptionChanged = mEndSelectionIndex != aClickedIndex;
#endif
  mStartSelectionIndex = aClickedIndex;
  mEndSelectionIndex = aClickedIndex;
  InvalidateFocus();

#ifdef ACCESSIBILITY
  if (isCurrentOptionChanged) {
    FireMenuItemActiveEvent();
  }
#endif

  return wasChanged;
}

void
nsListControlFrame::InitSelectionRange(int32_t aClickedIndex)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int32_t selectedIndex = GetSelectedIndex();
  if (selectedIndex >= 0) {
    
    nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
    NS_ASSERTION(options, "Collection of options is null!");
    uint32_t numOptions;
    options->GetLength(&numOptions);
    uint32_t i;
    
    for (i=selectedIndex+1; i < numOptions; i++) {
      bool selected;
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

static uint32_t
CountOptionsAndOptgroups(nsIFrame* aFrame)
{
  uint32_t count = 0;
  nsFrameList::Enumerator e(aFrame->PrincipalChildList());
  for (; !e.AtEnd(); e.Next()) {
    nsIFrame* child = e.get();
    nsIContent* content = child->GetContent();
    if (content) {
      if (content->IsHTML(nsGkAtoms::option)) {
        ++count;
      } else {
        nsCOMPtr<nsIDOMHTMLOptGroupElement> optgroup = do_QueryInterface(content);
        if (optgroup) {
          nsAutoString label;
          optgroup->GetLabel(label);
          if (label.Length() > 0) {
            ++count;
          }
          count += CountOptionsAndOptgroups(child);
        }
      }
    }
  }
  return count;
}

uint32_t
nsListControlFrame::GetNumberOfRows()
{
  return ::CountOptionsAndOptgroups(GetContentInsertionFrame());
}


bool
nsListControlFrame::PerformSelection(int32_t aClickedIndex,
                                     bool aIsShift,
                                     bool aIsControl)
{
  bool wasChanged = false;

  if (aClickedIndex == kNothingSelected) {
  }
  else if (GetMultiple()) {
    if (aIsShift) {
      
      
      if (mStartSelectionIndex == kNothingSelected) {
        InitSelectionRange(aClickedIndex);
      }

      
      
      int32_t startIndex;
      int32_t endIndex;
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
      }
#ifdef ACCESSIBILITY
      bool isCurrentOptionChanged = mEndSelectionIndex != aClickedIndex;
#endif
      mEndSelectionIndex = aClickedIndex;
      InvalidateFocus();

#ifdef ACCESSIBILITY
      if (isCurrentOptionChanged) {
        FireMenuItemActiveEvent();
      }
#endif
    } else if (aIsControl) {
      wasChanged = SingleSelection(aClickedIndex, true);
    } else {
      wasChanged = SingleSelection(aClickedIndex, false);
    }
  } else {
    wasChanged = SingleSelection(aClickedIndex, false);
  }

  return wasChanged;
}


bool
nsListControlFrame::HandleListSelection(nsIDOMEvent* aEvent,
                                        int32_t aClickedIndex)
{
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aEvent);
  bool isShift;
  bool isControl;
#ifdef XP_MACOSX
  mouseEvent->GetMetaKey(&isControl);
#else
  mouseEvent->GetCtrlKey(&isControl);
#endif
  mouseEvent->GetShiftKey(&isShift);
  return PerformSelection(aClickedIndex, isShift, isControl);
}


void
nsListControlFrame::CaptureMouseEvents(bool aGrabMouseEvents)
{
  
  
  
  
  
  if (aGrabMouseEvents && IsInDropDownMode() && nsComboboxControlFrame::ToolkitHasNativePopup())
    return;

  if (aGrabMouseEvents) {
    nsIPresShell::SetCapturingContent(mContent, CAPTURE_IGNOREALLOWED);
  } else {
    nsIContent* capturingContent = nsIPresShell::GetCapturingContent();

    bool dropDownIsHidden = false;
    if (IsInDropDownMode()) {
      dropDownIsHidden = !mComboboxFrame->IsDroppedDown();
    }
    if (capturingContent == mContent || dropDownIsHidden) {
      
      
      
      
      
      
      nsIPresShell::SetCapturingContent(nullptr, 0);
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

  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED))
    return NS_OK;

  return nsHTMLScrollFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}



NS_IMETHODIMP
nsListControlFrame::SetInitialChildList(ChildListID    aListID,
                                        nsFrameList&   aChildList)
{
  
  mIsAllContentHere = mContent->IsDoneAddingChildren();
  if (!mIsAllContentHere) {
    mIsAllFramesHere    = false;
    mHasBeenInitialized = false;
  }
  nsresult rv = nsHTMLScrollFrame::SetInitialChildList(aListID, aChildList);

  
  
  








  return rv;
}


nsresult
nsListControlFrame::GetSizeAttribute(uint32_t *aSize) {
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

  mContent->AddEventListener(NS_LITERAL_STRING("keypress"), mEventListener,
                             false, false);
  mContent->AddEventListener(NS_LITERAL_STRING("mousedown"), mEventListener,
                             false, false);
  mContent->AddEventListener(NS_LITERAL_STRING("mouseup"), mEventListener,
                             false, false);
  mContent->AddEventListener(NS_LITERAL_STRING("mousemove"), mEventListener,
                             false, false);

  mStartSelectionIndex = kNothingSelected;
  mEndSelectionIndex = kNothingSelected;

  mLastDropdownBackstopColor = PresContext()->DefaultBackgroundColor();

  AddStateBits(NS_FRAME_IN_POPUP);

  return result;
}

already_AddRefed<nsIContent> 
nsListControlFrame::GetOptionAsContent(nsIDOMHTMLOptionsCollection* aCollection, int32_t aIndex) 
{
  nsIContent * content = nullptr;
  nsCOMPtr<nsIDOMHTMLOptionElement> optionElement = GetOption(aCollection,
                                                              aIndex);

  NS_ASSERTION(optionElement != nullptr, "could not get option element by index!");

  if (optionElement) {
    CallQueryInterface(optionElement, &content);
  }
 
  return content;
}

already_AddRefed<nsIContent> 
nsListControlFrame::GetOptionContent(int32_t aIndex) const
  
{
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
  NS_ASSERTION(options.get() != nullptr, "Collection of options is null!");

  if (options) {
    return GetOptionAsContent(options, aIndex);
  } 
  return nullptr;
}

already_AddRefed<nsIDOMHTMLOptionsCollection>
nsListControlFrame::GetOptions(nsIContent * aContent)
{
  nsIDOMHTMLOptionsCollection* options = nullptr;
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement = do_QueryInterface(aContent);
  if (selectElement) {
    selectElement->GetOptions(&options);  
  }

  return options;
}

already_AddRefed<nsIDOMHTMLOptionElement>
nsListControlFrame::GetOption(nsIDOMHTMLOptionsCollection* aCollection,
                              int32_t aIndex)
{
  nsCOMPtr<nsIDOMNode> node;
  if (NS_SUCCEEDED(aCollection->Item(aIndex, getter_AddRefs(node)))) {
    NS_ASSERTION(node,
                 "Item was successful, but node from collection was null!");
    if (node) {
      nsIDOMHTMLOptionElement* option = nullptr;
      CallQueryInterface(node, &option);

      return option;
    }
  } else {
    NS_ERROR("Couldn't get option by index from collection!");
  }
  return nullptr;
}

bool 
nsListControlFrame::IsContentSelected(nsIContent* aContent) const
{
  bool isSelected = false;

  nsCOMPtr<nsIDOMHTMLOptionElement> optEl = do_QueryInterface(aContent);
  if (optEl)
    optEl->GetSelected(&isSelected);

  return isSelected;
}

bool 
nsListControlFrame::IsContentSelectedByIndex(int32_t aIndex) const 
{
  nsCOMPtr<nsIContent> content = GetOptionContent(aIndex);
  NS_ASSERTION(content, "Failed to retrieve option content");

  return IsContentSelected(content);
}

NS_IMETHODIMP
nsListControlFrame::OnOptionSelected(int32_t aIndex, bool aSelected)
{
  if (aSelected) {
    ScrollToIndex(aIndex);
  }
  return NS_OK;
}

void
nsListControlFrame::OnContentReset()
{
  ResetList(true);
}

void 
nsListControlFrame::ResetList(bool aAllowScrolling)
{
  
  
  if (!mIsAllFramesHere) {
    return;
  }

  if (aAllowScrolling) {
    mPostChildrenLoadedReset = true;

    
    int32_t indexToSelect = kNothingSelected;

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
nsListControlFrame::SetFocus(bool aOn, bool aRepaint)
{
  InvalidateFocus();

  if (aOn) {
    ComboboxFocusSet();
    mFocused = this;
  } else {
    mFocused = nullptr;
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
  if (nullptr != aComboboxFrame) {
    mComboboxFrame = do_QueryFrame(aComboboxFrame);
  }
}

void
nsListControlFrame::GetOptionText(int32_t aIndex, nsAString & aStr)
{
  aStr.SetLength(0);
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);

  if (options) {
    uint32_t numOptions;
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
          compressText.CompressWhitespace(true, true);
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

int32_t
nsListControlFrame::GetSelectedIndex()
{
  int32_t aIndex;
  
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement(do_QueryInterface(mContent));
  selectElement->GetSelectedIndex(&aIndex);
  
  return aIndex;
}

already_AddRefed<nsIContent>
nsListControlFrame::GetCurrentOption()
{
  
  
  int32_t focusedIndex = (mEndSelectionIndex == kNothingSelected) ?
    GetSelectedIndex() : mEndSelectionIndex;

  if (focusedIndex != kNothingSelected) {
    return GetOptionContent(focusedIndex);
  }

  nsRefPtr<nsHTMLSelectElement> selectElement =
    nsHTMLSelectElement::FromContent(mContent);
  NS_ASSERTION(selectElement, "Can't be null");

  
  
  nsCOMPtr<nsIDOMNode> node;

  uint32_t length;
  selectElement->GetLength(&length);
  if (length) {
    bool isDisabled = true;
    for (uint32_t i = 0; i < length && isDisabled; i++) {
      if (NS_FAILED(selectElement->Item(i, getter_AddRefs(node))) || !node) {
        break;
      }
      if (NS_FAILED(selectElement->IsOptionDisabled(i, &isDisabled))) {
        break;
      }
      if (isDisabled) {
        node = nullptr;
      } else {
        break;
      }
    }
    if (!node) {
      return nullptr;
    }
  }

  if (node) {
    nsCOMPtr<nsIContent> focusedOption = do_QueryInterface(node);
    return focusedOption.forget();
  }
  return nullptr;
}

bool 
nsListControlFrame::IsInDropDownMode() const
{
  return (mComboboxFrame != nullptr);
}

uint32_t
nsListControlFrame::GetNumberOfOptions()
{
  if (!mContent) {
    return 0;
  }

  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
  if (!options) {
    return 0;
  }

  uint32_t length = 0;
  options->GetLength(&length);
  return length;
}




bool nsListControlFrame::CheckIfAllFramesHere()
{
  
  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  if (node) {
    
    
    mIsAllFramesHere = true;
  }
  

  return mIsAllFramesHere;
}

NS_IMETHODIMP
nsListControlFrame::DoneAddingChildren(bool aIsDone)
{
  mIsAllContentHere = aIsDone;
  if (mIsAllContentHere) {
    
    
    
    if (!mIsAllFramesHere) {
      
      if (CheckIfAllFramesHere()) {
        mHasBeenInitialized = true;
        ResetList(true);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsListControlFrame::AddOption(int32_t aIndex)
{
#ifdef DO_REFLOW_DEBUG
  printf("---- Id: %d nsLCF %p Added Option %d\n", mReflowId, this, aIndex);
#endif

  if (!mIsAllContentHere) {
    mIsAllContentHere = mContent->IsDoneAddingChildren();
    if (!mIsAllContentHere) {
      mIsAllFramesHere    = false;
      mHasBeenInitialized = false;
    } else {
      mIsAllFramesHere = (aIndex == static_cast<int32_t>(GetNumberOfOptions()-1));
    }
  }

  
  mNeedToReset = true;

  if (!mHasBeenInitialized) {
    return NS_OK;
  }

  mPostChildrenLoadedReset = mIsAllContentHere;
  return NS_OK;
}

static int32_t
DecrementAndClamp(int32_t aSelectionIndex, int32_t aLength)
{
  return aLength == 0 ? kNothingSelected : std::max(0, aSelectionIndex - 1);
}

NS_IMETHODIMP
nsListControlFrame::RemoveOption(int32_t aIndex)
{
  NS_PRECONDITION(aIndex >= 0, "negative <option> index");

  
  if (IsInDropDownMode()) {
    mNeedToReset = true;
    mPostChildrenLoadedReset = mIsAllContentHere;
  }

  if (mStartSelectionIndex != kNothingSelected) {
    NS_ASSERTION(mEndSelectionIndex != kNothingSelected, "");
    int32_t numOptions = GetNumberOfOptions();
    
    
    NS_ASSERTION(aIndex <= numOptions, "out-of-bounds <option> index");

    int32_t forward = mEndSelectionIndex - mStartSelectionIndex;
    int32_t* low  = forward >= 0 ? &mStartSelectionIndex : &mEndSelectionIndex;
    int32_t* high = forward >= 0 ? &mEndSelectionIndex : &mStartSelectionIndex;
    if (aIndex < *low)
      *low = ::DecrementAndClamp(*low, numOptions);
    if (aIndex <= *high)
      *high = ::DecrementAndClamp(*high, numOptions);
    if (forward == 0)
      *low = *high;
  }
  else
    NS_ASSERTION(mEndSelectionIndex == kNothingSelected, "");

  InvalidateFocus();
  return NS_OK;
}





bool
nsListControlFrame::SetOptionsSelectedFromFrame(int32_t aStartIndex,
                                                int32_t aEndIndex,
                                                bool aValue,
                                                bool aClearAll)
{
  nsRefPtr<nsHTMLSelectElement> selectElement =
    nsHTMLSelectElement::FromContent(mContent);
  bool wasChanged = false;
#ifdef DEBUG
  nsresult rv = 
#endif
    selectElement->SetOptionsSelectedByIndex(aStartIndex,
                                             aEndIndex,
                                             aValue,
                                             aClearAll,
                                             false,
                                             true,
                                             &wasChanged);
  NS_ASSERTION(NS_SUCCEEDED(rv), "SetSelected failed");
  return wasChanged;
}

bool
nsListControlFrame::ToggleOptionSelectedFromFrame(int32_t aIndex)
{
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
  NS_ASSERTION(options, "No options");
  if (!options) {
    return false;
  }
  nsCOMPtr<nsIDOMHTMLOptionElement> option = GetOption(options, aIndex);
  NS_ASSERTION(option, "No option");
  if (!option) {
    return false;
  }

  bool value = false;
#ifdef DEBUG
  nsresult rv =
#endif
    option->GetSelected(&value);

  NS_ASSERTION(NS_SUCCEEDED(rv), "GetSelected failed");
  nsRefPtr<nsHTMLSelectElement> selectElement =
    nsHTMLSelectElement::FromContent(mContent);
  bool wasChanged = false;
#ifdef DEBUG
  rv =
#endif
    selectElement->SetOptionsSelectedByIndex(aIndex,
                                             aIndex,
                                             !value,
                                             false,
                                             false,
                                             true,
                                             &wasChanged);

  NS_ASSERTION(NS_SUCCEEDED(rv), "SetSelected failed");

  return wasChanged;
}



bool
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
  return true;
}

void
nsListControlFrame::ComboboxFinish(int32_t aIndex)
{
  gLastKeyTime = 0;

  if (mComboboxFrame) {
    PerformSelection(aIndex, false, false);

    int32_t displayIndex = mComboboxFrame->GetIndexOfDisplayArea();

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
    
    int32_t index = mComboboxFrame->UpdateRecentIndex(NS_SKIP_NOTIFY_INDEX);
    if (index == NS_SKIP_NOTIFY_INDEX)
      return;

    
    if (index == GetSelectedIndex())
      return;
  }

  
  nsContentUtils::DispatchTrustedEvent(mContent->OwnerDoc(), mContent,
                                       NS_LITERAL_STRING("change"), true,
                                       false);
}

NS_IMETHODIMP
nsListControlFrame::OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex)
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
    nsresult error = NS_OK;
    bool selected = false;
    int32_t indx = val.ToInteger(&error, 10); 
    if (NS_SUCCEEDED(error))
       selected = IsContentSelectedByIndex(indx); 
  
    aValue.Assign(selected ? NS_LITERAL_STRING("1") : NS_LITERAL_STRING("0"));
    
  
  } else if (nsGkAtoms::selectedindex == aName) {
    
    return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

void
nsListControlFrame::AboutToDropDown()
{
  NS_ASSERTION(IsInDropDownMode(),
    "AboutToDropDown called without being in dropdown mode");

  
  
  
  
  
  
  
  
  
  nsIFrame* comboboxFrame = do_QueryFrame(mComboboxFrame);
  nsStyleContext* context = comboboxFrame->GetStyleContext()->GetParent();
  mLastDropdownBackstopColor = NS_RGBA(0,0,0,0);
  while (NS_GET_A(mLastDropdownBackstopColor) < 255 && context) {
    mLastDropdownBackstopColor =
      NS_ComposeColors(context->GetStyleBackground()->mBackgroundColor,
                       mLastDropdownBackstopColor);
    context = context->GetParent();
  }
  mLastDropdownBackstopColor =
    NS_ComposeColors(PresContext()->DefaultBackgroundColor(),
                     mLastDropdownBackstopColor);

  if (mIsAllContentHere && mIsAllFramesHere && mHasBeenInitialized) {
    ScrollToIndex(GetSelectedIndex());
#ifdef ACCESSIBILITY
    FireMenuItemActiveEvent(); 
#endif
  }
  mItemSelectionStarted = false;
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
  bool wasInterrupted = !mHasPendingInterruptAtStartOfReflow &&
                          aPresContext->HasPendingInterrupt();

  rv = nsHTMLScrollFrame::DidReflow(aPresContext, aReflowState, aStatus);

  if (mNeedToReset && !wasInterrupted) {
    mNeedToReset = false;
    
    
    
    
    
    
    
    
    
    ResetList(!DidHistoryRestore() || mPostChildrenLoadedReset);
  }

  mHasPendingInterruptAtStartOfReflow = false;
  return rv;
}

nsIAtom*
nsListControlFrame::GetType() const
{
  return nsGkAtoms::listControlFrame; 
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
nsListControlFrame::IsOptionDisabled(int32_t anIndex, bool &aIsDisabled)
{
  nsRefPtr<nsHTMLSelectElement> sel =
    nsHTMLSelectElement::FromContent(mContent);
  if (sel) {
    sel->IsOptionDisabled(anIndex, &aIsDisabled);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}




bool
nsListControlFrame::IsLeftButton(nsIDOMEvent* aMouseEvent)
{
  
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  if (mouseEvent) {
    uint16_t whichButton;
    if (NS_SUCCEEDED(mouseEvent->GetButton(&whichButton))) {
      return whichButton != 0?false:true;
    }
  }
  return false;
}

nscoord
nsListControlFrame::CalcFallbackRowHeight(float aFontSizeInflation)
{
  nscoord rowHeight = 0;

  nsRefPtr<nsFontMetrics> fontMet;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet),
                                        aFontSizeInflation);
  if (fontMet) {
    rowHeight = fontMet->MaxHeight();
  }

  return rowHeight;
}

nscoord
nsListControlFrame::CalcIntrinsicHeight(nscoord aHeightOfARow,
                                        int32_t aNumberOfOptions)
{
  NS_PRECONDITION(!IsInDropDownMode(),
                  "Shouldn't be in dropdown mode when we call this");

  mNumDisplayRows = 1;
  GetSizeAttribute(&mNumDisplayRows);

  if (mNumDisplayRows < 1) {
    mNumDisplayRows = 4;
  }

  return mNumDisplayRows * aHeightOfARow;
}




nsresult
nsListControlFrame::MouseUp(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent != nullptr, "aMouseEvent is null.");

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_ERROR_FAILURE);

  UpdateInListState(aMouseEvent);

  mButtonDown = false;

  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    return NS_OK;
  }

  
  
  
  if (!IsLeftButton(aMouseEvent)) {
    if (IsInDropDownMode()) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        aMouseEvent->PreventDefault();
        aMouseEvent->StopPropagation();
      } else {
        CaptureMouseEvents(false);
        return NS_OK;
      }
      CaptureMouseEvents(false);
      return NS_ERROR_FAILURE; 
    } else {
      CaptureMouseEvents(false);
      return NS_OK;
    }
  }

  const nsStyleVisibility* vis = GetStyleVisibility();
      
  if (!vis->IsVisible()) {
    return NS_OK;
  }

  if (IsInDropDownMode()) {
    
    
    
    
    
    
    
    
    
    
    nsMouseEvent * mouseEvent;
    mouseEvent = (nsMouseEvent *) aMouseEvent->GetInternalNSEvent();

    int32_t selectedIndex;
    if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
      
      bool isDisabled = false;
      IsOptionDisabled(selectedIndex, isDisabled);
      if (isDisabled) {
        aMouseEvent->PreventDefault();
        aMouseEvent->StopPropagation();
        CaptureMouseEvents(false);
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
    CaptureMouseEvents(false);
    
    if (mChangesSinceDragStart) {
      
      
      mChangesSinceDragStart = false;
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
  nsRect borderInnerEdge = GetScrollPortRect();
  if (pt.y >= borderInnerEdge.y && pt.y < borderInnerEdge.YMost()) {
    mItemSelectionStarted = true;
  }
}

bool nsListControlFrame::IgnoreMouseEventForSelection(nsIDOMEvent* aEvent)
{
  if (!mComboboxFrame)
    return false;

  
  
  if (!mComboboxFrame->IsDroppedDown())
    return true;

  return !mItemSelectionStarted;
}

#ifdef ACCESSIBILITY
void
nsListControlFrame::FireMenuItemActiveEvent()
{
  if (mFocused != this && !IsInDropDownMode()) {
    return;
  }

  nsCOMPtr<nsIContent> optionContent = GetCurrentOption();
  if (!optionContent) {
    return;
  }

  FireDOMEvent(NS_LITERAL_STRING("DOMMenuItemActive"), optionContent);
}
#endif

nsresult
nsListControlFrame::GetIndexFromDOMEvent(nsIDOMEvent* aMouseEvent, 
                                         int32_t&     aCurIndex)
{
  if (IgnoreMouseEventForSelection(aMouseEvent))
    return NS_ERROR_FAILURE;

  if (nsIPresShell::GetCapturingContent() != mContent) {
    
    nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aMouseEvent, this);
    nsRect borderInnerEdge = GetScrollPortRect();
    if (!borderInnerEdge.Contains(pt)) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCOMPtr<nsIContent> content = PresContext()->EventStateManager()->
    GetEventTargetContent(nullptr);

  nsCOMPtr<nsIContent> optionContent = GetOptionFromContent(content);
  if (optionContent) {
    aCurIndex = GetIndexFromContent(optionContent);
    return NS_OK;
  }

  int32_t numOptions = GetNumberOfOptions();
  if (numOptions < 1)
    return NS_ERROR_FAILURE;

  nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aMouseEvent, this);

  
  
  nsCOMPtr<nsIContent> firstOption = GetOptionContent(0);
  NS_ASSERTION(firstOption, "Can't find first option that's supposed to be there");
  nsIFrame* optionFrame = firstOption->GetPrimaryFrame();
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
  optionFrame = lastOption->GetPrimaryFrame();
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
  NS_ASSERTION(aMouseEvent != nullptr, "aMouseEvent is null.");

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_ERROR_FAILURE);

  UpdateInListState(aMouseEvent);

  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
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

  int32_t selectedIndex;
  if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
    
    mButtonDown = true;
    CaptureMouseEvents(true);
    mChangesSinceDragStart = HandleListSelection(aMouseEvent, selectedIndex);
  } else {
    
    if (mComboboxFrame) {
      if (!IgnoreMouseEventForSelection(aMouseEvent)) {
        return NS_OK;
      }

      if (!nsComboboxControlFrame::ToolkitHasNativePopup())
      {
        bool isDroppedDown = mComboboxFrame->IsDroppedDown();
        nsIFrame* comboFrame = do_QueryFrame(mComboboxFrame);
        nsWeakFrame weakFrame(comboFrame);
        mComboboxFrame->ShowDropDown(!isDroppedDown);
        if (!weakFrame.IsAlive())
          return NS_OK;
        if (isDroppedDown) {
          CaptureMouseEvents(false);
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
  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_ERROR_FAILURE);

  UpdateInListState(aMouseEvent);

  if (IsInDropDownMode()) { 
    if (mComboboxFrame->IsDroppedDown()) {
      int32_t selectedIndex;
      if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
        PerformSelection(selectedIndex, false, false);
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
    int32_t selectedIndex;
    if (NS_SUCCEEDED(GetIndexFromDOMEvent(aMouseEvent, selectedIndex))) {
      
      if (selectedIndex == mEndSelectionIndex) {
        return NS_OK;
      }
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
      NS_ASSERTION(mouseEvent, "aMouseEvent is not an nsIDOMMouseEvent!");
      bool isControl;
#ifdef XP_MACOSX
      mouseEvent->GetMetaKey(&isControl);
#else
      mouseEvent->GetCtrlKey(&isControl);
#endif
      
      bool wasChanged = PerformSelection(selectedIndex,
                                           !isControl, isControl);
      mChangesSinceDragStart = mChangesSinceDragStart || wasChanged;
    }
  }
  return NS_OK;
}




nsresult
nsListControlFrame::ScrollToIndex(int32_t aIndex)
{
  if (aIndex < 0) {
    
    
    return ScrollToFrame(nullptr);
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
  
  if (nullptr == aOptElement) {
    ScrollTo(nsPoint(0, 0), nsIScrollableFrame::INSTANT);
    return NS_OK;
  }

  
  nsIFrame *childFrame = aOptElement->GetPrimaryFrame();
  if (childFrame) {
    PresContext()->PresShell()->
      ScrollFrameRectIntoView(childFrame,
                              nsRect(nsPoint(0, 0), childFrame->GetSize()),
                              nsIPresShell::ScrollAxis(), nsIPresShell::ScrollAxis(),
                              nsIPresShell::SCROLL_OVERFLOW_HIDDEN |
                              nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY);
  }
  return NS_OK;
}

















void
nsListControlFrame::AdjustIndexForDisabledOpt(int32_t aStartIndex,
                                              int32_t &aNewIndex,
                                              int32_t aNumOptions,
                                              int32_t aDoAdjustInc,
                                              int32_t aDoAdjustIncNext)
{
  
  if (aNumOptions == 0) {
    aNewIndex = kNothingSelected;
    return;
  }

  
  bool doingReverse = false;
  
  int32_t bottom      = 0;
  
  int32_t top         = aNumOptions;

  
  
  
  
  
  
  int32_t startIndex = aStartIndex;
  if (startIndex < bottom) {
    startIndex = GetSelectedIndex();
  }
  int32_t newIndex    = startIndex + aDoAdjustInc;

  
  if (newIndex < bottom) {
    newIndex = 0;
  } else if (newIndex >= top) {
    newIndex = aNumOptions-1;
  }

  while (1) {
    
    bool isDisabled = true;
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
        doingReverse     = true;
        top              = startIndex;
      }
    } else  if (newIndex >= top) {
      if (doingReverse) {
        return;        
      } else {
        
        
        
        newIndex = top - 1;
        aDoAdjustIncNext = -1;
        doingReverse     = true;
        bottom           = startIndex;
      }
    }
  }

  
  aNewIndex     = newIndex;
}

nsAString& 
nsListControlFrame::GetIncrementalString()
{ 
  if (sIncrementalString == nullptr)
    sIncrementalString = new nsString();

  return *sIncrementalString;
}

void
nsListControlFrame::Shutdown()
{
  delete sIncrementalString;
  sIncrementalString = nullptr;
}

void
nsListControlFrame::DropDownToggleKey(nsIDOMEvent* aKeyEvent)
{
  
  
  if (IsInDropDownMode() && !nsComboboxControlFrame::ToolkitHasNativePopup()) {
    aKeyEvent->PreventDefault();
    if (!mComboboxFrame->IsDroppedDown()) {
      mComboboxFrame->ShowDropDown(true);
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

  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED))
    return NS_OK;

  
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_FAILURE);

  uint32_t keycode = 0;
  uint32_t charcode = 0;
  keyEvent->GetKeyCode(&keycode);
  keyEvent->GetCharCode(&charcode);

  bool isAlt = false;

  keyEvent->GetAltKey(&isAlt);
  if (isAlt) {
    if (keycode == nsIDOMKeyEvent::DOM_VK_UP || keycode == nsIDOMKeyEvent::DOM_VK_DOWN) {
      DropDownToggleKey(aKeyEvent);
    }
    return NS_OK;
  }

  
  bool isControl = false;
  bool isShift   = false;
  keyEvent->GetCtrlKey(&isControl);
  if (!isControl) {
    keyEvent->GetMetaKey(&isControl);
  }
  keyEvent->GetShiftKey(&isShift);

  
  nsCOMPtr<nsIDOMHTMLOptionsCollection> options = GetOptions(mContent);
  NS_ENSURE_TRUE(options, NS_ERROR_FAILURE);

  uint32_t numOptions = 0;
  options->GetLength(&numOptions);

  
  bool didIncrementalSearch = false;
  
  
  
  int32_t newIndex = kNothingSelected;

  
  
  
  
  
  
  if (isControl && (keycode == nsIDOMKeyEvent::DOM_VK_UP ||
                    keycode == nsIDOMKeyEvent::DOM_VK_LEFT ||
                    keycode == nsIDOMKeyEvent::DOM_VK_DOWN ||
                    keycode == nsIDOMKeyEvent::DOM_VK_RIGHT)) {
    
    isControl = mControlSelectMode = GetMultiple();
  } else if (charcode != ' ') {
    mControlSelectMode = false;
  }
  switch (keycode) {

    case nsIDOMKeyEvent::DOM_VK_UP:
    case nsIDOMKeyEvent::DOM_VK_LEFT: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (int32_t)numOptions,
                                -1, -1);
      } break;
    
    case nsIDOMKeyEvent::DOM_VK_DOWN:
    case nsIDOMKeyEvent::DOM_VK_RIGHT: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (int32_t)numOptions,
                                1, 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_RETURN: {
      if (mComboboxFrame != nullptr) {
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
                                (int32_t)numOptions,
                                -std::max(1, int32_t(mNumDisplayRows-1)), -1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_PAGE_DOWN: {
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                (int32_t)numOptions,
                                std::max(1, int32_t(mNumDisplayRows-1)), 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_HOME: {
      AdjustIndexForDisabledOpt(0, newIndex,
                                (int32_t)numOptions,
                                0, 1);
      } break;

    case nsIDOMKeyEvent::DOM_VK_END: {
      AdjustIndexForDisabledOpt(numOptions-1, newIndex,
                                (int32_t)numOptions,
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

      didIncrementalSearch = true;
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
      uint32_t charIndex = 1, stringLength = incrementalString.Length();
      while (charIndex < stringLength && incrementalString[charIndex] == incrementalString[charIndex - 1]) {
        charIndex++;
      }
      if (charIndex == stringLength) {
        incrementalString.Truncate(1);
        stringLength = 1;
      }

      
      
      
      
      
      
      int32_t startIndex = GetSelectedIndex();
      if (startIndex == kNothingSelected) {
        startIndex = 0;
      } else if (stringLength == 1) {
        startIndex++;
      }

      uint32_t i;
      for (i = 0; i < numOptions; i++) {
        uint32_t index = (i + startIndex) % numOptions;
        nsCOMPtr<nsIDOMHTMLOptionElement> optionElement =
          GetOption(options, index);
        if (optionElement) {
          nsAutoString text;
          if (NS_OK == optionElement->GetText(text)) {
            if (StringBeginsWith(text, incrementalString,
                                 nsCaseInsensitiveStringComparator())) {
              bool wasChanged = PerformSelection(index, isShift, isControl);
              if (wasChanged) {
                
                if (!UpdateSelection()) {
                  return NS_OK;
                }
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
    
    
    bool wasChanged = false;
    if (isControl && !isShift && charcode != ' ') {
      mStartSelectionIndex = newIndex;
      mEndSelectionIndex = newIndex;
      InvalidateFocus();
      ScrollToIndex(newIndex);

#ifdef ACCESSIBILITY
      FireMenuItemActiveEvent();
#endif
    } else if (mControlSelectMode && charcode == ' ') {
      wasChanged = SingleSelection(newIndex, true);
    } else {
      wasChanged = PerformSelection(newIndex, isShift, isControl);
    }
    if (wasChanged) {
       
      if (!UpdateSelection()) {
        return NS_OK;
      }
    }
  }

  return NS_OK;
}






NS_IMPL_ISUPPORTS1(nsListEventListener, nsIDOMEventListener)

NS_IMETHODIMP
nsListEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  if (!mFrame)
    return NS_OK;

  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("keypress"))
    return mFrame->nsListControlFrame::KeyPress(aEvent);
  if (eventType.EqualsLiteral("mousedown"))
    return mFrame->nsListControlFrame::MouseDown(aEvent);
  if (eventType.EqualsLiteral("mouseup"))
    return mFrame->nsListControlFrame::MouseUp(aEvent);
  if (eventType.EqualsLiteral("mousemove"))
    return mFrame->nsListControlFrame::MouseMove(aEvent);

  NS_ABORT();
  return NS_OK;
}
