




#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsUnicharUtils.h"
#include "nsListControlFrame.h"
#include "nsFormControlFrame.h" 
#include "nsGkAtoms.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsComboboxControlFrame.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIPresShell.h"
#include "nsIDOMMouseEvent.h"
#include "nsIXULRuntime.h"
#include "nsFontMetrics.h"
#include "nsIScrollableFrame.h"
#include "nsCSSRendering.h"
#include "nsIDOMEventListener.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsContentUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/HTMLOptionsCollection.h"
#include "mozilla/dom/HTMLSelectElement.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"
#include <algorithm>

using namespace mozilla;


const uint32_t kMaxDropDownRows         = 20; 
const int32_t kNothingSelected          = -1;


nsListControlFrame * nsListControlFrame::mFocused = nullptr;
nsString * nsListControlFrame::sIncrementalString = nullptr;


#define INCREMENTAL_SEARCH_KEYPRESS_TIME 1000





DOMTimeStamp nsListControlFrame::gLastKeyTime = 0;







class nsListEventListener final : public nsIDOMEventListener
{
public:
  explicit nsListEventListener(nsListControlFrame *aFrame)
    : mFrame(aFrame) { }

  void SetFrame(nsListControlFrame *aFrame) { mFrame = aFrame; }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

private:
  ~nsListEventListener() {}

  nsListControlFrame  *mFrame;
};


nsContainerFrame*
NS_NewListControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsListControlFrame* it =
    new (aPresShell) nsListControlFrame(aContext);

  it->AddStateBits(NS_FRAME_INDEPENDENT_SELECTION);

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsListControlFrame)


nsListControlFrame::nsListControlFrame(nsStyleContext* aContext)
  : nsHTMLScrollFrame(aContext, false),
    mMightNeedSecondPass(false),
    mHasPendingInterruptAtStartOfReflow(false),
    mDropdownCanGrow(false),
    mForceSelection(false),
    mLastDropdownComputedBSize(NS_UNCONSTRAINEDSIZE)
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

  mContent->RemoveSystemEventListener(NS_LITERAL_STRING("keydown"),
                                      mEventListener, false);
  mContent->RemoveSystemEventListener(NS_LITERAL_STRING("keypress"),
                                      mEventListener, false);
  mContent->RemoveSystemEventListener(NS_LITERAL_STRING("mousedown"),
                                      mEventListener, false);
  mContent->RemoveSystemEventListener(NS_LITERAL_STRING("mouseup"),
                                      mEventListener, false);
  mContent->RemoveSystemEventListener(NS_LITERAL_STRING("mousemove"),
                                      mEventListener, false);

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsHTMLScrollFrame::DestroyFrom(aDestructRoot);
}

void
nsListControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  
  
  
  if (aBuilder->IsBackgroundOnly())
    return;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsListControlFrame");

  if (IsInDropDownMode()) {
    NS_ASSERTION(NS_GET_A(mLastDropdownBackstopColor) == 255,
                 "need an opaque backstop color");
    
    
    
    aLists.BorderBackground()->AppendNewToBottom(
      new (aBuilder) nsDisplaySolidColor(aBuilder,
        this, nsRect(aBuilder->ToReferenceFrame(this), GetSize()),
        mLastDropdownBackstopColor));
  }

  nsHTMLScrollFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
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
    if (GetWritingMode().IsVertical()) {
      fRect.width = GetScrollPortRect().width;
      fRect.height = CalcFallbackRowBSize(inflation);
    } else {
      fRect.width = CalcFallbackRowBSize(inflation);
      fRect.height = GetScrollPortRect().height;
    }
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
GetMaxOptionBSize(nsIFrame* aContainer, WritingMode aWM)
{
  nscoord result = 0;
  for (nsIFrame* option = aContainer->GetFirstPrincipalChild();
       option; option = option->GetNextSibling()) {
    nscoord optionBSize;
    if (nsCOMPtr<nsIDOMHTMLOptGroupElement>
        (do_QueryInterface(option->GetContent()))) {
      
      optionBSize = GetMaxOptionBSize(option, aWM);
    } else {
      
      optionBSize = option->BSize(aWM);
    }
    if (result < optionBSize)
      result = optionBSize;
  }
  return result;
}





nscoord
nsListControlFrame::CalcBSizeOfARow()
{
  
  
  
  
  
  int32_t blockSizeOfARow = GetMaxOptionBSize(GetOptionsContainer(),
                                              GetWritingMode());

  
  
  if (blockSizeOfARow == 0 && GetNumberOfOptions() == 0) {
    float inflation = nsLayoutUtils::FontSizeInflationFor(this);
    blockSizeOfARow = CalcFallbackRowBSize(inflation);
  }

  return blockSizeOfARow;
}

nscoord
nsListControlFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  
  
  
  
  WritingMode wm = GetWritingMode();
  result = GetScrolledFrame()->GetPrefISize(aRenderingContext);
  LogicalMargin scrollbarSize(wm, GetDesiredScrollbarSizes(PresContext(),
                                                           aRenderingContext));
  result = NSCoordSaturatingAdd(result, scrollbarSize.IStartEnd(wm));
  return result;
}

nscoord
nsListControlFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  
  
  
  
  WritingMode wm = GetWritingMode();
  result = GetScrolledFrame()->GetMinISize(aRenderingContext);
  LogicalMargin scrollbarSize(wm, GetDesiredScrollbarSizes(PresContext(),
                                                           aRenderingContext));
  result += scrollbarSize.IStartEnd(wm);

  return result;
}

void
nsListControlFrame::Reflow(nsPresContext*           aPresContext, 
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState, 
                           nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aReflowState.ComputedISize() != NS_UNCONSTRAINEDSIZE,
                  "Must have a computed inline size");

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
    ReflowAsDropdown(aPresContext, aDesiredSize, aReflowState, aStatus);
    return;
  }

  MarkInReflow();
  





















  bool autoBSize = (aReflowState.ComputedBSize() == NS_UNCONSTRAINEDSIZE);

  mMightNeedSecondPass = autoBSize &&
    (NS_SUBTREE_DIRTY(this) || aReflowState.ShouldReflowAllKids());
  
  nsHTMLReflowState state(aReflowState);
  int32_t length = GetNumberOfRows();

  nscoord oldBSizeOfARow = BSizeOfARow();

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW) && autoBSize) {
    
    
    
    nscoord computedBSize = CalcIntrinsicBSize(oldBSizeOfARow, length);
    computedBSize = state.ApplyMinMaxBSize(computedBSize);
    state.SetComputedBSize(computedBSize);
  }

  nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);

  if (!mMightNeedSecondPass) {
    NS_ASSERTION(!autoBSize || BSizeOfARow() == oldBSizeOfARow,
                 "How did our BSize of a row change if nothing was dirty?");
    NS_ASSERTION(!autoBSize ||
                 !(GetStateBits() & NS_FRAME_FIRST_REFLOW),
                 "How do we not need a second pass during initial reflow at "
                 "auto BSize?");
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if we don't need a second pass!");
    if (!autoBSize) {
      
      
      
      
      
      nscoord rowBSize = CalcBSizeOfARow();
      if (rowBSize == 0) {
        
        mNumDisplayRows = 1;
      } else {
        mNumDisplayRows = std::max(1, state.ComputedBSize() / rowBSize);
      }
    }

    return;
  }

  mMightNeedSecondPass = false;

  
  
  if (!IsScrollbarUpdateSuppressed()) {
    
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if the block size of a row has not "
                 "changed!");
    return;
  }

  SetSuppressScrollbarUpdate(false);

  
  
  
  
  
  
  nsHTMLScrollFrame::DidReflow(aPresContext, &state,
                               nsDidReflowStatus::FINISHED);

  
  nscoord computedBSize = CalcIntrinsicBSize(BSizeOfARow(), length);
  computedBSize = state.ApplyMinMaxBSize(computedBSize);
  state.SetComputedBSize(computedBSize);

  
  
  
  nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
}

void
nsListControlFrame::ReflowAsDropdown(nsPresContext*           aPresContext, 
                                     nsHTMLReflowMetrics&     aDesiredSize,
                                     const nsHTMLReflowState& aReflowState, 
                                     nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aReflowState.ComputedBSize() == NS_UNCONSTRAINEDSIZE,
                  "We should not have a computed block size here!");
  
  mMightNeedSecondPass = NS_SUBTREE_DIRTY(this) ||
    aReflowState.ShouldReflowAllKids();

  WritingMode wm = aReflowState.GetWritingMode();
#ifdef DEBUG
  nscoord oldBSizeOfARow = BSizeOfARow();
  nscoord oldVisibleBSize = (GetStateBits() & NS_FRAME_FIRST_REFLOW) ?
    NS_UNCONSTRAINEDSIZE : GetScrolledFrame()->BSize(wm);
#endif

  nsHTMLReflowState state(aReflowState);

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    
    
    
    
    
    state.SetComputedBSize(mLastDropdownComputedBSize);
  }

  nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);

  if (!mMightNeedSecondPass) {
    NS_ASSERTION(oldVisibleBSize == GetScrolledFrame()->BSize(wm),
                 "How did our kid's BSize change if nothing was dirty?");
    NS_ASSERTION(BSizeOfARow() == oldBSizeOfARow,
                 "How did our BSize of a row change if nothing was dirty?");
    NS_ASSERTION(!IsScrollbarUpdateSuppressed(),
                 "Shouldn't be suppressing if we don't need a second pass!");
    NS_ASSERTION(!(GetStateBits() & NS_FRAME_FIRST_REFLOW),
                 "How can we avoid a second pass during first reflow?");
    return;
  }

  mMightNeedSecondPass = false;

  
  
  if (!IsScrollbarUpdateSuppressed()) {
    
    NS_ASSERTION(!(GetStateBits() & NS_FRAME_FIRST_REFLOW),
                 "How can we avoid a second pass during first reflow?");
    return;
  }

  SetSuppressScrollbarUpdate(false);

  nscoord visibleBSize = GetScrolledFrame()->BSize(wm);
  nscoord blockSizeOfARow = BSizeOfARow();

  
  
  
  
  
  
  nsHTMLScrollFrame::DidReflow(aPresContext, &state,
                               nsDidReflowStatus::FINISHED);

  
  
  

  mDropdownCanGrow = false;
  if (visibleBSize <= 0 || blockSizeOfARow <= 0) {
    
    
    state.SetComputedBSize(blockSizeOfARow);
    mNumDisplayRows = 1;
  } else {
    nsComboboxControlFrame* combobox =
      static_cast<nsComboboxControlFrame*>(mComboboxFrame);
    LogicalPoint translation(wm);
    nscoord before, after;
    combobox->GetAvailableDropdownSpace(wm, &before, &after, &translation);
    if (before <= 0 && after <= 0) {
      state.SetComputedBSize(blockSizeOfARow);
      mNumDisplayRows = 1;
      mDropdownCanGrow = GetNumberOfRows() > 1;
    } else {
      nscoord bp = aReflowState.ComputedLogicalBorderPadding().BStartEnd(wm);
      nscoord availableBSize = std::max(before, after) - bp;
      nscoord newBSize;
      uint32_t rows;
      if (visibleBSize <= availableBSize) {
        
        rows = GetNumberOfRows();
        mNumDisplayRows = clamped<uint32_t>(rows, 1, kMaxDropDownRows);
        if (mNumDisplayRows == rows) {
          newBSize = visibleBSize;  
        } else {
          newBSize = mNumDisplayRows * blockSizeOfARow; 
        }
      } else {
        rows = availableBSize / blockSizeOfARow;
        mNumDisplayRows = clamped<uint32_t>(rows, 1, kMaxDropDownRows);
        newBSize = mNumDisplayRows * blockSizeOfARow; 
      }
      state.SetComputedBSize(newBSize);
      mDropdownCanGrow = visibleBSize - newBSize >= blockSizeOfARow &&
                         mNumDisplayRows != kMaxDropDownRows;
    }
  }

  mLastDropdownComputedBSize = state.ComputedBSize();

  nsHTMLScrollFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
}

ScrollbarStyles
nsListControlFrame::GetScrollbarStyles() const
{
  
  
  int32_t style = IsInDropDownMode() ? NS_STYLE_OVERFLOW_AUTO
                                     : NS_STYLE_OVERFLOW_SCROLL;
  if (GetWritingMode().IsVertical()) {
    return ScrollbarStyles(style, NS_STYLE_OVERFLOW_HIDDEN);
  } else {
    return ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN, style);
  }
}

bool
nsListControlFrame::ShouldPropagateComputedBSizeToScrolledContent() const
{
  return !IsInDropDownMode();
}


nsContainerFrame*
nsListControlFrame::GetContentInsertionFrame() {
  return GetOptionsContainer()->GetContentInsertionFrame();
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
  nsWeakFrame weakFrame(this);
  ScrollToIndex(aClickedIndex);
  if (!weakFrame.IsAlive()) {
    return wasChanged;
  }

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
    
    nsRefPtr<dom::HTMLOptionsCollection> options = GetOptions();
    NS_ASSERTION(options, "Collection of options is null!");
    uint32_t numOptions = options->Length();
    
    uint32_t i;
    for (i = selectedIndex + 1; i < numOptions; i++) {
      if (!options->ItemAsOption(i)->Selected()) {
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
      if (content->IsHTMLElement(nsGkAtoms::option)) {
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

  if (aClickedIndex == kNothingSelected && !mForceSelection) {
    
  } else if (GetMultiple()) {
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
      nsWeakFrame weakFrame(this);
      ScrollToIndex(aClickedIndex);
      if (!weakFrame.IsAlive()) {
        return wasChanged;
      }

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


nsresult 
nsListControlFrame::HandleEvent(nsPresContext* aPresContext,
                                WidgetGUIEvent* aEvent,
                                nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  























  if (nsEventStatus_eConsumeNoDefault == *aEventStatus)
    return NS_OK;

  
  
  const nsStyleUserInterface* uiStyle = StyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  EventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED))
    return NS_OK;

  return nsHTMLScrollFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}



void
nsListControlFrame::SetInitialChildList(ChildListID    aListID,
                                        nsFrameList&   aChildList)
{
  
  mIsAllContentHere = mContent->IsDoneAddingChildren();
  if (!mIsAllContentHere) {
    mIsAllFramesHere    = false;
    mHasBeenInitialized = false;
  }
  nsHTMLScrollFrame::SetInitialChildList(aListID, aChildList);

  
  
  







}


void
nsListControlFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent,
                         nsIFrame*         aPrevInFlow)
{
  nsHTMLScrollFrame::Init(aContent, aParent, aPrevInFlow);

  
  
  
  
  mEventListener = new nsListEventListener(this);

  mContent->AddSystemEventListener(NS_LITERAL_STRING("keydown"),
                                   mEventListener, false, false);
  mContent->AddSystemEventListener(NS_LITERAL_STRING("keypress"),
                                   mEventListener, false, false);
  mContent->AddSystemEventListener(NS_LITERAL_STRING("mousedown"),
                                   mEventListener, false, false);
  mContent->AddSystemEventListener(NS_LITERAL_STRING("mouseup"),
                                   mEventListener, false, false);
  mContent->AddSystemEventListener(NS_LITERAL_STRING("mousemove"),
                                   mEventListener, false, false);

  mStartSelectionIndex = kNothingSelected;
  mEndSelectionIndex = kNothingSelected;

  mLastDropdownBackstopColor = PresContext()->DefaultBackgroundColor();

  if (IsInDropDownMode()) {
    AddStateBits(NS_FRAME_IN_POPUP);
  }
}

dom::HTMLOptionsCollection*
nsListControlFrame::GetOptions() const
{
  dom::HTMLSelectElement* select =
    dom::HTMLSelectElement::FromContentOrNull(mContent);
  NS_ENSURE_TRUE(select, nullptr);

  return select->Options();
}

dom::HTMLOptionElement*
nsListControlFrame::GetOption(uint32_t aIndex) const
{
  dom::HTMLSelectElement* select =
    dom::HTMLSelectElement::FromContentOrNull(mContent);
  NS_ENSURE_TRUE(select, nullptr);

  return select->Item(aIndex);
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
      nsWeakFrame weakFrame(this);
      ScrollToIndex(indexToSelect);
      if (!weakFrame.IsAlive()) {
        return;
      }
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
nsListControlFrame::GetOptionText(uint32_t aIndex, nsAString& aStr)
{
  aStr.Truncate();
  if (dom::HTMLOptionElement* optionElement = GetOption(aIndex)) {
    optionElement->GetText(aStr);
  }
}

int32_t
nsListControlFrame::GetSelectedIndex()
{
  dom::HTMLSelectElement* select =
    dom::HTMLSelectElement::FromContentOrNull(mContent);
  return select->SelectedIndex();
}

dom::HTMLOptionElement*
nsListControlFrame::GetCurrentOption()
{
  
  
  int32_t focusedIndex = (mEndSelectionIndex == kNothingSelected) ?
    GetSelectedIndex() : mEndSelectionIndex;

  if (focusedIndex != kNothingSelected) {
    return GetOption(AssertedCast<uint32_t>(focusedIndex));
  }

  
  nsRefPtr<dom::HTMLSelectElement> selectElement =
    dom::HTMLSelectElement::FromContent(mContent);

  for (uint32_t i = 0, length = selectElement->Length(); i < length; ++i) {
    dom::HTMLOptionElement* node = selectElement->Item(i);
    if (!node) {
      return nullptr;
    }

    if (!selectElement->IsOptionDisabled(node)) {
      return node;
    }
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
  dom::HTMLOptionsCollection* options = GetOptions();
  if (!options) {
    return 0;
  }

  return options->Length();
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
  nsRefPtr<dom::HTMLSelectElement> selectElement =
    dom::HTMLSelectElement::FromContent(mContent);

  uint32_t mask = dom::HTMLSelectElement::NOTIFY;
  if (mForceSelection) {
    mask |= dom::HTMLSelectElement::SET_DISABLED;
  }
  if (aValue) {
    mask |= dom::HTMLSelectElement::IS_SELECTED;
  }
  if (aClearAll) {
    mask |= dom::HTMLSelectElement::CLEAR_ALL;
  }

  return selectElement->SetOptionsSelectedByIndex(aStartIndex, aEndIndex, mask);
}

bool
nsListControlFrame::ToggleOptionSelectedFromFrame(int32_t aIndex)
{
  nsRefPtr<dom::HTMLOptionElement> option =
    GetOption(static_cast<uint32_t>(aIndex));
  NS_ENSURE_TRUE(option, false);

  nsRefPtr<dom::HTMLSelectElement> selectElement =
    dom::HTMLSelectElement::FromContent(mContent);

  uint32_t mask = dom::HTMLSelectElement::NOTIFY;
  if (!option->Selected()) {
    mask |= dom::HTMLSelectElement::IS_SELECTED;
  }

  return selectElement->SetOptionsSelectedByIndex(aIndex, aIndex, mask);
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
    int32_t displayIndex = mComboboxFrame->GetIndexOfDisplayArea();
    
    mForceSelection = displayIndex == aIndex;

    nsWeakFrame weakFrame(this);
    PerformSelection(aIndex, false, false);  
    if (!weakFrame.IsAlive() || !mComboboxFrame) {
      return;
    }

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

  nsWeakFrame weakFrame(this);
  ScrollToIndex(aNewIndex);
  if (!weakFrame.IsAlive()) {
    return NS_OK;
  }
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

void
nsListControlFrame::AboutToDropDown()
{
  NS_ASSERTION(IsInDropDownMode(),
    "AboutToDropDown called without being in dropdown mode");

  
  
  
  
  
  
  
  
  
  nsIFrame* comboboxFrame = do_QueryFrame(mComboboxFrame);
  nsStyleContext* context = comboboxFrame->StyleContext()->GetParent();
  mLastDropdownBackstopColor = NS_RGBA(0,0,0,0);
  while (NS_GET_A(mLastDropdownBackstopColor) < 255 && context) {
    mLastDropdownBackstopColor =
      NS_ComposeColors(context->StyleBackground()->mBackgroundColor,
                       mLastDropdownBackstopColor);
    context = context->GetParent();
  }
  mLastDropdownBackstopColor =
    NS_ComposeColors(PresContext()->DefaultBackgroundColor(),
                     mLastDropdownBackstopColor);

  if (mIsAllContentHere && mIsAllFramesHere && mHasBeenInitialized) {
    nsWeakFrame weakFrame(this);
    ScrollToIndex(GetSelectedIndex());
    if (!weakFrame.IsAlive()) {
      return;
    }
#ifdef ACCESSIBILITY
    FireMenuItemActiveEvent(); 
#endif
  }
  mItemSelectionStarted = false;
  mForceSelection = false;
}


void
nsListControlFrame::AboutToRollup()
{
  
  
  
  
  
  
  
  

  if (IsInDropDownMode()) {
    ComboboxFinish(mComboboxFrame->GetIndexOfDisplayArea()); 
  }
}

void
nsListControlFrame::DidReflow(nsPresContext*           aPresContext,
                              const nsHTMLReflowState* aReflowState,
                              nsDidReflowStatus        aStatus)
{
  bool wasInterrupted = !mHasPendingInterruptAtStartOfReflow &&
                          aPresContext->HasPendingInterrupt();

  nsHTMLScrollFrame::DidReflow(aPresContext, aReflowState, aStatus);

  if (mNeedToReset && !wasInterrupted) {
    mNeedToReset = false;
    
    
    
    
    
    
    
    
    
    ResetList(!DidHistoryRestore() || mPostChildrenLoadedReset);
  }

  mHasPendingInterruptAtStartOfReflow = false;
}

nsIAtom*
nsListControlFrame::GetType() const
{
  return nsGkAtoms::listControlFrame; 
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsListControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ListControl"), aResult);
}
#endif

nscoord
nsListControlFrame::GetBSizeOfARow()
{
  return BSizeOfARow();
}

nsresult
nsListControlFrame::IsOptionDisabled(int32_t anIndex, bool &aIsDisabled)
{
  nsRefPtr<dom::HTMLSelectElement> sel =
    dom::HTMLSelectElement::FromContent(mContent);
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
    int16_t whichButton;
    if (NS_SUCCEEDED(mouseEvent->GetButton(&whichButton))) {
      return whichButton != 0?false:true;
    }
  }
  return false;
}

nscoord
nsListControlFrame::CalcFallbackRowBSize(float aFontSizeInflation)
{
  nscoord rowBSize = 0;

  nsRefPtr<nsFontMetrics> fontMet;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet),
                                        aFontSizeInflation);
  if (fontMet) {
    rowBSize = fontMet->MaxHeight();
  }

  return rowBSize;
}

nscoord
nsListControlFrame::CalcIntrinsicBSize(nscoord aBSizeOfARow,
                                       int32_t aNumberOfOptions)
{
  NS_PRECONDITION(!IsInDropDownMode(),
                  "Shouldn't be in dropdown mode when we call this");

  dom::HTMLSelectElement* select =
    dom::HTMLSelectElement::FromContentOrNull(mContent);
  if (select) {
    mNumDisplayRows = select->Size();
  } else {
    mNumDisplayRows = 1;
  }

  if (mNumDisplayRows < 1) {
    mNumDisplayRows = 4;
  }

  return mNumDisplayRows * aBSizeOfARow;
}




nsresult
nsListControlFrame::MouseUp(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent != nullptr, "aMouseEvent is null.");

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_ERROR_FAILURE);

  UpdateInListState(aMouseEvent);

  mButtonDown = false;

  EventStates eventStates = mContent->AsElement()->State();
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

  const nsStyleVisibility* vis = StyleVisibility();
      
  if (!vis->IsVisible()) {
    return NS_OK;
  }

  if (IsInDropDownMode()) {
    
    
    
    
    
    
    
    
    
    
    WidgetMouseEvent* mouseEvent =
      aMouseEvent->GetInternalNSEvent()->AsMouseEvent();

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

  nsRefPtr<dom::HTMLOptionElement> option;
  for (nsCOMPtr<nsIContent> content =
         PresContext()->EventStateManager()->GetEventTargetContent(nullptr);
       content && !option;
       content = content->GetParent()) {
    option = dom::HTMLOptionElement::FromContent(content);
  }

  if (option) {
    aCurIndex = option->Index();
    MOZ_ASSERT(aCurIndex >= 0);
    return NS_OK;
  }

  int32_t numOptions = GetNumberOfOptions();
  if (numOptions < 1)
    return NS_ERROR_FAILURE;

  nsPoint pt = nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(aMouseEvent, this);

  
  
  nsRefPtr<dom::HTMLOptionElement> firstOption = GetOption(0);
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

  nsRefPtr<dom::HTMLOptionElement> lastOption = GetOption(numOptions - 1);
  
  
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

static bool
FireShowDropDownEvent(nsIContent* aContent)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content &&
      Preferences::GetBool("browser.tabs.remote.desktopbehavior", false)) {
    nsContentUtils::DispatchChromeEvent(aContent->OwnerDoc(), aContent,
                                        NS_LITERAL_STRING("mozshowdropdown"), true,
                                        false);
    return true;
  }

  return false;
}

nsresult
nsListControlFrame::MouseDown(nsIDOMEvent* aMouseEvent)
{
  NS_ASSERTION(aMouseEvent != nullptr, "aMouseEvent is null.");

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(aMouseEvent);
  NS_ENSURE_TRUE(mouseEvent, NS_ERROR_FAILURE);

  UpdateInListState(aMouseEvent);

  EventStates eventStates = mContent->AsElement()->State();
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
    nsWeakFrame weakFrame(this);
    bool change =
      HandleListSelection(aMouseEvent, selectedIndex); 
    if (!weakFrame.IsAlive()) {
      return NS_OK;
    }
    mChangesSinceDragStart = change;
  } else {
    
    if (mComboboxFrame) {
      if (FireShowDropDownEvent(mContent)) {
        return NS_OK;
      }

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
      nsWeakFrame weakFrame(this);
      
      bool wasChanged = PerformSelection(selectedIndex,
                                           !isControl, isControl);
      if (!weakFrame.IsAlive()) {
        return NS_OK;
      }
      mChangesSinceDragStart = mChangesSinceDragStart || wasChanged;
    }
  }
  return NS_OK;
}




void
nsListControlFrame::ScrollToIndex(int32_t aIndex)
{
  if (aIndex < 0) {
    
    
    ScrollTo(nsPoint(0, 0), nsIScrollableFrame::INSTANT);
  } else {
    nsRefPtr<dom::HTMLOptionElement> option =
      GetOption(AssertedCast<uint32_t>(aIndex));
    if (option) {
      ScrollToFrame(*option);
    }
  }
}

void
nsListControlFrame::ScrollToFrame(dom::HTMLOptionElement& aOptElement)
{
  
  nsIFrame* childFrame = aOptElement.GetPrimaryFrame();
  if (childFrame) {
    PresContext()->PresShell()->
      ScrollFrameRectIntoView(childFrame,
                              nsRect(nsPoint(0, 0), childFrame->GetSize()),
                              nsIPresShell::ScrollAxis(), nsIPresShell::ScrollAxis(),
                              nsIPresShell::SCROLL_OVERFLOW_HIDDEN |
                              nsIPresShell::SCROLL_FIRST_ANCESTOR_ONLY);
  }
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
      if (!FireShowDropDownEvent(mContent)) {
        mComboboxFrame->ShowDropDown(true);
      }
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
nsListControlFrame::KeyDown(nsIDOMEvent* aKeyEvent)
{
  MOZ_ASSERT(aKeyEvent, "aKeyEvent is null.");

  EventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    return NS_OK;
  }

  AutoIncrementalSearchResetter incrementalSearchResetter;

  
  
  
  
  

  const WidgetKeyboardEvent* keyEvent =
    aKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
  MOZ_ASSERT(keyEvent,
    "DOM event must have WidgetKeyboardEvent for its internal event");

  if (keyEvent->IsAlt()) {
    if (keyEvent->keyCode == NS_VK_UP || keyEvent->keyCode == NS_VK_DOWN) {
      DropDownToggleKey(aKeyEvent);
    }
    return NS_OK;
  }

  
  nsRefPtr<dom::HTMLOptionsCollection> options = GetOptions();
  NS_ENSURE_TRUE(options, NS_ERROR_FAILURE);

  uint32_t numOptions = options->Length();

  
  int32_t newIndex = kNothingSelected;

  bool isControlOrMeta = (keyEvent->IsControl() || keyEvent->IsMeta());
  
  if (isControlOrMeta && !GetMultiple() &&
      (keyEvent->keyCode == NS_VK_PAGE_UP ||
       keyEvent->keyCode == NS_VK_PAGE_DOWN)) {
    return NS_OK;
  }
  if (isControlOrMeta && (keyEvent->keyCode == NS_VK_UP ||
                          keyEvent->keyCode == NS_VK_LEFT ||
                          keyEvent->keyCode == NS_VK_DOWN ||
                          keyEvent->keyCode == NS_VK_RIGHT ||
                          keyEvent->keyCode == NS_VK_HOME ||
                          keyEvent->keyCode == NS_VK_END)) {
    
    isControlOrMeta = mControlSelectMode = GetMultiple();
  } else if (keyEvent->keyCode != NS_VK_SPACE) {
    mControlSelectMode = false;
  }

  switch (keyEvent->keyCode) {
    case NS_VK_UP:
    case NS_VK_LEFT:
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                static_cast<int32_t>(numOptions),
                                -1, -1);
      break;
    case NS_VK_DOWN:
    case NS_VK_RIGHT:
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                static_cast<int32_t>(numOptions),
                                1, 1);
      break;
    case NS_VK_RETURN:
      if (IsInDropDownMode()) {
        if (mComboboxFrame->IsDroppedDown()) {
          
          
          aKeyEvent->PreventDefault();

          nsWeakFrame weakFrame(this);
          ComboboxFinish(mEndSelectionIndex);
          if (!weakFrame.IsAlive()) {
            return NS_OK;
          }
        }
        
        
        
        FireOnChange();
        return NS_OK;
      }

      
      if (!GetMultiple()) {
        return NS_OK;
      }

      newIndex = mEndSelectionIndex;
      break;
    case NS_VK_ESCAPE: {
      
      if (!IsInDropDownMode()) {
        return NS_OK;
      }

      AboutToRollup();
      
      
      
      aKeyEvent->PreventDefault();
      return NS_OK;
    }
    case NS_VK_PAGE_UP: {
      int32_t itemsPerPage =
        std::max(1, static_cast<int32_t>(mNumDisplayRows - 1));
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                static_cast<int32_t>(numOptions),
                                -itemsPerPage, -1);
      break;
    }
    case NS_VK_PAGE_DOWN: {
      int32_t itemsPerPage =
        std::max(1, static_cast<int32_t>(mNumDisplayRows - 1));
      AdjustIndexForDisabledOpt(mEndSelectionIndex, newIndex,
                                static_cast<int32_t>(numOptions),
                                itemsPerPage, 1);
      break;
    }
    case NS_VK_HOME:
      AdjustIndexForDisabledOpt(0, newIndex,
                                static_cast<int32_t>(numOptions),
                                0, 1);
      break;
    case NS_VK_END:
      AdjustIndexForDisabledOpt(static_cast<int32_t>(numOptions) - 1, newIndex,
                                static_cast<int32_t>(numOptions),
                                0, -1);
      break;

#if defined(XP_WIN)
    case NS_VK_F4:
      if (!isControlOrMeta) {
        DropDownToggleKey(aKeyEvent);
      }
      return NS_OK;
#endif

    default: 
      incrementalSearchResetter.Cancel();
      return NS_OK;
  }

  aKeyEvent->PreventDefault();

  
  
  PostHandleKeyEvent(newIndex, 0, keyEvent->IsShift(), isControlOrMeta);
  return NS_OK;
}

nsresult
nsListControlFrame::KeyPress(nsIDOMEvent* aKeyEvent)
{
  MOZ_ASSERT(aKeyEvent, "aKeyEvent is null.");

  EventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    return NS_OK;
  }

  AutoIncrementalSearchResetter incrementalSearchResetter;

  const WidgetKeyboardEvent* keyEvent =
    aKeyEvent->GetInternalNSEvent()->AsKeyboardEvent();
  MOZ_ASSERT(keyEvent,
    "DOM event must have WidgetKeyboardEvent for its internal event");

  
  

  
  if (keyEvent->mFlags.mDefaultPrevented) {
    return NS_OK;
  }

  if (keyEvent->IsAlt()) {
    return NS_OK;
  }

  
  
  
  if (keyEvent->charCode != ' ') {
    mControlSelectMode = false;
  }

  bool isControlOrMeta = (keyEvent->IsControl() || keyEvent->IsMeta());
  if (isControlOrMeta && keyEvent->charCode != ' ') {
    return NS_OK;
  }

  
  
  if (!keyEvent->charCode) {
    
    
    if (keyEvent->keyCode == NS_VK_BACK) {
      incrementalSearchResetter.Cancel();
      if (!GetIncrementalString().IsEmpty()) {
        GetIncrementalString().Truncate(GetIncrementalString().Length() - 1);
      }
      aKeyEvent->PreventDefault();
    } else {
      
      
      
      
    }
    return NS_OK;
  }

  incrementalSearchResetter.Cancel();

  
  aKeyEvent->PreventDefault();

  

  
  
  
  
  
  if (keyEvent->time - gLastKeyTime > INCREMENTAL_SEARCH_KEYPRESS_TIME) {
    
    
    if (keyEvent->charCode == ' ') {
      
      
      PostHandleKeyEvent(mEndSelectionIndex, keyEvent->charCode,
                         keyEvent->IsShift(), isControlOrMeta);

      return NS_OK;
    }

    GetIncrementalString().Truncate();
  }

  gLastKeyTime = keyEvent->time;

  
  char16_t uniChar = ToLowerCase(static_cast<char16_t>(keyEvent->charCode));
  GetIncrementalString().Append(uniChar);

  
  
  nsAutoString incrementalString(GetIncrementalString());
  uint32_t charIndex = 1, stringLength = incrementalString.Length();
  while (charIndex < stringLength &&
         incrementalString[charIndex] == incrementalString[charIndex - 1]) {
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

  
  nsRefPtr<dom::HTMLOptionsCollection> options = GetOptions();
  NS_ENSURE_TRUE(options, NS_ERROR_FAILURE);

  uint32_t numOptions = options->Length();

  nsWeakFrame weakFrame(this);
  for (uint32_t i = 0; i < numOptions; ++i) {
    uint32_t index = (i + startIndex) % numOptions;
    nsRefPtr<dom::HTMLOptionElement> optionElement =
      options->ItemAsOption(index);
    if (!optionElement || !optionElement->GetPrimaryFrame()) {
      continue;
    }

    nsAutoString text;
    if (NS_FAILED(optionElement->GetText(text)) ||
        !StringBeginsWith(
           nsContentUtils::TrimWhitespace<
             nsContentUtils::IsHTMLWhitespaceOrNBSP>(text, false),
           incrementalString, nsCaseInsensitiveStringComparator())) {
      continue;
    }

    bool wasChanged = PerformSelection(index, keyEvent->IsShift(), isControlOrMeta);
    if (!weakFrame.IsAlive()) {
      return NS_OK;
    }
    if (!wasChanged) {
      break;
    }

    
    
    if (!UpdateSelection()) {
      return NS_OK;
    }
    break;
  }

  return NS_OK;
}

void
nsListControlFrame::PostHandleKeyEvent(int32_t aNewIndex,
                                       uint32_t aCharCode,
                                       bool aIsShift,
                                       bool aIsControlOrMeta)
{
  if (aNewIndex == kNothingSelected) {
    return;
  }

  
  
  nsWeakFrame weakFrame(this);
  bool wasChanged = false;
  if (aIsControlOrMeta && !aIsShift && aCharCode != ' ') {
    mStartSelectionIndex = aNewIndex;
    mEndSelectionIndex = aNewIndex;
    InvalidateFocus();
    ScrollToIndex(aNewIndex);
    if (!weakFrame.IsAlive()) {
      return;
    }

#ifdef ACCESSIBILITY
    FireMenuItemActiveEvent();
#endif
  } else if (mControlSelectMode && aCharCode == ' ') {
    wasChanged = SingleSelection(aNewIndex, true);
  } else {
    wasChanged = PerformSelection(aNewIndex, aIsShift, aIsControlOrMeta);
  }
  if (wasChanged && weakFrame.IsAlive()) {
    
    UpdateSelection();
  }
}






NS_IMPL_ISUPPORTS(nsListEventListener, nsIDOMEventListener)

NS_IMETHODIMP
nsListEventListener::HandleEvent(nsIDOMEvent* aEvent)
{
  if (!mFrame)
    return NS_OK;

  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("keydown")) {
    return mFrame->nsListControlFrame::KeyDown(aEvent);
  }
  if (eventType.EqualsLiteral("keypress")) {
    return mFrame->nsListControlFrame::KeyPress(aEvent);
  }
  if (eventType.EqualsLiteral("mousedown")) {
    bool defaultPrevented = false;
    aEvent->GetDefaultPrevented(&defaultPrevented);
    if (defaultPrevented) {
      return NS_OK;
    }
    return mFrame->nsListControlFrame::MouseDown(aEvent);
  }
  if (eventType.EqualsLiteral("mouseup")) {
    bool defaultPrevented = false;
    aEvent->GetDefaultPrevented(&defaultPrevented);
    if (defaultPrevented) {
      return NS_OK;
    }
    return mFrame->nsListControlFrame::MouseUp(aEvent);
  }
  if (eventType.EqualsLiteral("mousemove")) {
    
    
    return mFrame->nsListControlFrame::MouseMove(aEvent);
  }

  NS_ABORT();
  return NS_OK;
}
