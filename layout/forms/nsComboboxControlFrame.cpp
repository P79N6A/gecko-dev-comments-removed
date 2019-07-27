




#include "nsComboboxControlFrame.h"

#include "gfxUtils.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"
#include "nsCOMPtr.h"
#include "nsFocusManager.h"
#include "nsFormControlFrame.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsHTMLParts.h"
#include "nsIFormControl.h"
#include "nsNameSpaceManager.h"
#include "nsIListControlFrame.h"
#include "nsPIDOMWindow.h"
#include "nsIPresShell.h"
#include "nsContentList.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMNode.h"
#include "nsISelectControlFrame.h"
#include "nsContentUtils.h"
#include "nsIDocument.h"
#include "nsIScrollableFrame.h"
#include "nsListControlFrame.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsITheme.h"
#include "nsThemeConstants.h"
#include "nsRenderingContext.h"
#include "mozilla/Likely.h"
#include <algorithm>
#include "nsTextNode.h"
#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/EventStates.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/unused.h"

#ifdef XP_WIN
#define COMBOBOX_ROLLUP_CONSUME_EVENT 0
#else
#define COMBOBOX_ROLLUP_CONSUME_EVENT 1
#endif

using namespace mozilla;
using namespace mozilla::gfx;

NS_IMETHODIMP
nsComboboxControlFrame::RedisplayTextEvent::Run()
{
  if (mControlFrame)
    mControlFrame->HandleRedisplayTextEvent();
  return NS_OK;
}

class nsPresState;

#define FIX_FOR_BUG_53259


















class nsComboButtonListener : public nsIDOMEventListener
{
private:
  virtual ~nsComboButtonListener() {}

public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD HandleEvent(nsIDOMEvent*) override
  {
    mComboBox->ShowDropDown(!mComboBox->IsDroppedDown());
    return NS_OK;
  }

  explicit nsComboButtonListener(nsComboboxControlFrame* aCombobox)
  {
    mComboBox = aCombobox;
  }

  nsComboboxControlFrame* mComboBox;
};

NS_IMPL_ISUPPORTS(nsComboButtonListener,
                  nsIDOMEventListener)


nsComboboxControlFrame* nsComboboxControlFrame::sFocused = nullptr;

nsContainerFrame*
NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, nsFrameState aStateFlags)
{
  nsComboboxControlFrame* it = new (aPresShell) nsComboboxControlFrame(aContext);

  if (it) {
    
    it->AddStateBits(aStateFlags);
  }

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsComboboxControlFrame)





#ifdef DO_REFLOW_COUNTER

#define MAX_REFLOW_CNT 1024
static int32_t gTotalReqs    = 0;;
static int32_t gTotalReflows = 0;;
static int32_t gReflowControlCntRQ[MAX_REFLOW_CNT];
static int32_t gReflowControlCnt[MAX_REFLOW_CNT];
static int32_t gReflowInx = -1;

#define REFLOW_COUNTER() \
  if (mReflowId > -1) \
    gReflowControlCnt[mReflowId]++;

#define REFLOW_COUNTER_REQUEST() \
  if (mReflowId > -1) \
    gReflowControlCntRQ[mReflowId]++;

#define REFLOW_COUNTER_DUMP(__desc) \
  if (mReflowId > -1) {\
    gTotalReqs    += gReflowControlCntRQ[mReflowId];\
    gTotalReflows += gReflowControlCnt[mReflowId];\
    printf("** Id:%5d %s RF: %d RQ: %d   %d/%d  %5.2f\n", \
           mReflowId, (__desc), \
           gReflowControlCnt[mReflowId], \
           gReflowControlCntRQ[mReflowId],\
           gTotalReflows, gTotalReqs, float(gTotalReflows)/float(gTotalReqs)*100.0f);\
  }

#define REFLOW_COUNTER_INIT() \
  if (gReflowInx < MAX_REFLOW_CNT) { \
    gReflowInx++; \
    mReflowId = gReflowInx; \
    gReflowControlCnt[mReflowId] = 0; \
    gReflowControlCntRQ[mReflowId] = 0; \
  } else { \
    mReflowId = -1; \
  }


#define REFLOW_DEBUG_MSG(_msg1) printf((_msg1))
#define REFLOW_DEBUG_MSG2(_msg1, _msg2) printf((_msg1), (_msg2))
#define REFLOW_DEBUG_MSG3(_msg1, _msg2, _msg3) printf((_msg1), (_msg2), (_msg3))
#define REFLOW_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4) printf((_msg1), (_msg2), (_msg3), (_msg4))

#else 

#define REFLOW_COUNTER_REQUEST()
#define REFLOW_COUNTER()
#define REFLOW_COUNTER_DUMP(__desc)
#define REFLOW_COUNTER_INIT()

#define REFLOW_DEBUG_MSG(_msg)
#define REFLOW_DEBUG_MSG2(_msg1, _msg2)
#define REFLOW_DEBUG_MSG3(_msg1, _msg2, _msg3)
#define REFLOW_DEBUG_MSG4(_msg1, _msg2, _msg3, _msg4)


#endif




#ifdef DO_VERY_NOISY
#define REFLOW_NOISY_MSG(_msg1) printf((_msg1))
#define REFLOW_NOISY_MSG2(_msg1, _msg2) printf((_msg1), (_msg2))
#define REFLOW_NOISY_MSG3(_msg1, _msg2, _msg3) printf((_msg1), (_msg2), (_msg3))
#define REFLOW_NOISY_MSG4(_msg1, _msg2, _msg3, _msg4) printf((_msg1), (_msg2), (_msg3), (_msg4))
#else
#define REFLOW_NOISY_MSG(_msg)
#define REFLOW_NOISY_MSG2(_msg1, _msg2)
#define REFLOW_NOISY_MSG3(_msg1, _msg2, _msg3)
#define REFLOW_NOISY_MSG4(_msg1, _msg2, _msg3, _msg4)
#endif




#ifdef DO_PIXELS
#define PX(__v) __v / 15
#else
#define PX(__v) __v
#endif





nsComboboxControlFrame::nsComboboxControlFrame(nsStyleContext* aContext)
  : nsBlockFrame(aContext)
  , mDisplayFrame(nullptr)
  , mButtonFrame(nullptr)
  , mDropdownFrame(nullptr)
  , mListControlFrame(nullptr)
  , mDisplayWidth(0)
  , mRecentSelectedIndex(NS_SKIP_NOTIFY_INDEX)
  , mDisplayedIndex(-1)
  , mLastDropDownAboveScreenY(nscoord_MIN)
  , mLastDropDownBelowScreenY(nscoord_MIN)
  , mDroppedDown(false)
  , mInRedisplayText(false)
  , mDelayedShowDropDown(false)
{
  REFLOW_COUNTER_INIT()
}


nsComboboxControlFrame::~nsComboboxControlFrame()
{
  REFLOW_COUNTER_DUMP("nsCCF");
}



NS_QUERYFRAME_HEAD(nsComboboxControlFrame)
  NS_QUERYFRAME_ENTRY(nsIComboboxControlFrame)
  NS_QUERYFRAME_ENTRY(nsIFormControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
  NS_QUERYFRAME_ENTRY(nsISelectControlFrame)
  NS_QUERYFRAME_ENTRY(nsIStatefulFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

#ifdef ACCESSIBILITY
a11y::AccType
nsComboboxControlFrame::AccessibleType()
{
  return a11y::eHTMLComboboxType;
}
#endif

void
nsComboboxControlFrame::SetFocus(bool aOn, bool aRepaint)
{
  nsWeakFrame weakFrame(this);
  if (aOn) {
    nsListControlFrame::ComboboxFocusSet();
    sFocused = this;
    if (mDelayedShowDropDown) {
      ShowDropDown(true); 
      if (!weakFrame.IsAlive()) {
        return;
      }
    }
  } else {
    sFocused = nullptr;
    mDelayedShowDropDown = false;
    if (mDroppedDown) {
      mListControlFrame->ComboboxFinish(mDisplayedIndex); 
      if (!weakFrame.IsAlive()) {
        return;
      }
    }
    
    mListControlFrame->FireOnChange();
  }

  if (!weakFrame.IsAlive()) {
    return;
  }

  
  
  
  InvalidateFrame();
}

void
nsComboboxControlFrame::ShowPopup(bool aShowPopup)
{
  nsView* view = mDropdownFrame->GetView();
  nsViewManager* viewManager = view->GetViewManager();

  if (aShowPopup) {
    nsRect rect = mDropdownFrame->GetRect();
    rect.x = rect.y = 0;
    viewManager->ResizeView(view, rect);
    viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
  } else {
    viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
    nsRect emptyRect(0, 0, 0, 0);
    viewManager->ResizeView(view, emptyRect);
  }

  
  nsEventStatus status = nsEventStatus_eIgnore;
  WidgetMouseEvent event(true, aShowPopup ?
                         NS_XUL_POPUP_SHOWING : NS_XUL_POPUP_HIDING, nullptr,
                         WidgetMouseEvent::eReal);

  nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();
  if (shell)
    shell->HandleDOMEventWithTarget(mContent, &event, &status);
}

bool
nsComboboxControlFrame::ShowList(bool aShowList)
{
  nsView* view = mDropdownFrame->GetView();
  if (aShowList) {
    NS_ASSERTION(!view->HasWidget(),
                 "We shouldn't have a widget before we need to display the popup");

    
    view->GetViewManager()->SetViewFloating(view, true);

    nsWidgetInitData widgetData;
    widgetData.mWindowType  = eWindowType_popup;
    widgetData.mBorderStyle = eBorderStyle_default;
    view->CreateWidgetForPopup(&widgetData);
  } else {
    nsIWidget* widget = view->GetWidget();
    if (widget) {
      
      widget->CaptureRollupEvents(this, false);
    }
  }

  nsWeakFrame weakFrame(this);
  ShowPopup(aShowList);  
  if (!weakFrame.IsAlive()) {
    return false;
  }

  mDroppedDown = aShowList;
  nsIWidget* widget = view->GetWidget();
  if (mDroppedDown) {
    
    
    mListControlFrame->AboutToDropDown();
    mListControlFrame->CaptureMouseEvents(true);
    if (widget) {
      widget->CaptureRollupEvents(this, true);
    }
  } else {
    if (widget) {
      view->DestroyWidget();
    }
  }

  return weakFrame.IsAlive();
}

class nsResizeDropdownAtFinalPosition final
  : public nsIReflowCallback, public nsRunnable
{
public:
  explicit nsResizeDropdownAtFinalPosition(nsComboboxControlFrame* aFrame)
    : mFrame(aFrame)
  {
    MOZ_COUNT_CTOR(nsResizeDropdownAtFinalPosition);
  }

protected:
  ~nsResizeDropdownAtFinalPosition()
  {
    MOZ_COUNT_DTOR(nsResizeDropdownAtFinalPosition);
  }

public:
  virtual bool ReflowFinished() override
  {
    Run();
    NS_RELEASE_THIS();
    return false;
  }

  virtual void ReflowCallbackCanceled() override
  {
    NS_RELEASE_THIS();
  }

  NS_IMETHODIMP Run() override
  {
    if (mFrame.IsAlive()) {
      static_cast<nsComboboxControlFrame*>(mFrame.GetFrame())->
        AbsolutelyPositionDropDown();
    }
    return NS_OK;
  }

  nsWeakFrame mFrame;
};

void
nsComboboxControlFrame::ReflowDropdown(nsPresContext*  aPresContext,
                                       const nsHTMLReflowState& aReflowState)
{
  
  
  
  if (!aReflowState.ShouldReflowAllKids() &&
      !NS_SUBTREE_DIRTY(mDropdownFrame)) {
    return;
  }

  
  
  
  WritingMode wm = mDropdownFrame->GetWritingMode();
  LogicalSize availSize = aReflowState.AvailableSize(wm);
  availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, mDropdownFrame,
                                   availSize);

  
  
  
  nscoord forcedWidth = aReflowState.ComputedWidth() +
    aReflowState.ComputedPhysicalBorderPadding().LeftRight() -
    kidReflowState.ComputedPhysicalBorderPadding().LeftRight();
  kidReflowState.SetComputedWidth(std::max(kidReflowState.ComputedWidth(),
                                         forcedWidth));

  
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    nsView* view = mDropdownFrame->GetView();
    nsViewManager* viewManager = view->GetViewManager();
    viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
    nsRect emptyRect(0, 0, 0, 0);
    viewManager->ResizeView(view, emptyRect);
  }

  
  
  int32_t flags = NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_VISIBILITY | NS_FRAME_NO_SIZE_VIEW;
  if (mDroppedDown) {
    flags = 0;
  }
  nsRect rect = mDropdownFrame->GetRect();
  nsHTMLReflowMetrics desiredSize(aReflowState);
  nsReflowStatus ignoredStatus;
  ReflowChild(mDropdownFrame, aPresContext, desiredSize,
              kidReflowState, rect.x, rect.y, flags, ignoredStatus);

   
  FinishReflowChild(mDropdownFrame, aPresContext, desiredSize,
                    &kidReflowState, rect.x, rect.y, flags);
}

nsPoint
nsComboboxControlFrame::GetCSSTransformTranslation()
{
  nsIFrame* frame = this;
  bool is3DTransform = false;
  Matrix transform;
  while (frame) {
    nsIFrame* parent;
    Matrix4x4 ctm = frame->GetTransformMatrix(nullptr, &parent);
    Matrix matrix;
    if (ctm.Is2D(&matrix)) {
      transform = transform * matrix;
    } else {
      is3DTransform = true;
      break;
    }
    frame = parent;
  }
  nsPoint translation;
  if (!is3DTransform && !transform.HasNonTranslation()) {
    nsPresContext* pc = PresContext();
    
    
    nsRootPresContext* rootPC = pc->GetRootPresContext();
    if (rootPC) {
      int32_t apd = pc->AppUnitsPerDevPixel();
      translation.x = NSFloatPixelsToAppUnits(transform._31, apd);
      translation.y = NSFloatPixelsToAppUnits(transform._32, apd);
      translation -= GetOffsetToCrossDoc(rootPC->PresShell()->GetRootFrame());
    }
  }
  return translation;
}

class nsAsyncRollup : public nsRunnable
{
public:
  explicit nsAsyncRollup(nsComboboxControlFrame* aFrame) : mFrame(aFrame) {}
  NS_IMETHODIMP Run()
  {
    if (mFrame.IsAlive()) {
      static_cast<nsComboboxControlFrame*>(mFrame.GetFrame())
        ->RollupFromList();
    }
    return NS_OK;
  }
  nsWeakFrame mFrame;
};

class nsAsyncResize : public nsRunnable
{
public:
  explicit nsAsyncResize(nsComboboxControlFrame* aFrame) : mFrame(aFrame) {}
  NS_IMETHODIMP Run()
  {
    if (mFrame.IsAlive()) {
      nsComboboxControlFrame* combo =
        static_cast<nsComboboxControlFrame*>(mFrame.GetFrame());
      static_cast<nsListControlFrame*>(combo->mDropdownFrame)->
        SetSuppressScrollbarUpdate(true);
      nsCOMPtr<nsIPresShell> shell = mFrame->PresContext()->PresShell();
      shell->FrameNeedsReflow(combo->mDropdownFrame, nsIPresShell::eResize,
                              NS_FRAME_IS_DIRTY);
      shell->FlushPendingNotifications(Flush_Layout);
      if (mFrame.IsAlive()) {
        combo = static_cast<nsComboboxControlFrame*>(mFrame.GetFrame());
        static_cast<nsListControlFrame*>(combo->mDropdownFrame)->
          SetSuppressScrollbarUpdate(false);
        if (combo->mDelayedShowDropDown) {
          combo->ShowDropDown(true);
        }
      }
    }
    return NS_OK;
  }
  nsWeakFrame mFrame;
};

void
nsComboboxControlFrame::GetAvailableDropdownSpace(nscoord* aAbove,
                                                  nscoord* aBelow,
                                                  nsPoint* aTranslation)
{
  
  
  
  
  
  
  
  

  
  
  
  *aTranslation = GetCSSTransformTranslation();
  *aAbove = 0;
  *aBelow = 0;

  nsRect screen = nsFormControlFrame::GetUsableScreenRect(PresContext());
  if (mLastDropDownBelowScreenY == nscoord_MIN) {
    nsRect thisScreenRect = GetScreenRectInAppUnits();
    mLastDropDownBelowScreenY = thisScreenRect.YMost() + aTranslation->y;
    mLastDropDownAboveScreenY = thisScreenRect.y + aTranslation->y;
  }

  nscoord minY;
  nsPresContext* pc = PresContext()->GetToplevelContentDocumentPresContext();
  nsIFrame* root = pc ? pc->PresShell()->GetRootFrame() : nullptr;
  if (root) {
    minY = root->GetScreenRectInAppUnits().y;
    if (mLastDropDownBelowScreenY < minY) {
      
      return;
    }
  } else {
    minY = screen.y;
  }

  nscoord below = screen.YMost() - mLastDropDownBelowScreenY;
  nscoord above = mLastDropDownAboveScreenY - minY;

  
  
  if (above >= below) {
    nsListControlFrame* lcf = static_cast<nsListControlFrame*>(mDropdownFrame);
    nscoord rowHeight = lcf->GetHeightOfARow();
    if (above < below + rowHeight) {
      above -= rowHeight;
    }
  }

  *aBelow = below;
  *aAbove = above;
}

nsComboboxControlFrame::DropDownPositionState
nsComboboxControlFrame::AbsolutelyPositionDropDown()
{
  nsPoint translation;
  nscoord above, below;
  mLastDropDownBelowScreenY = nscoord_MIN;
  GetAvailableDropdownSpace(&above, &below, &translation);
  if (above <= 0 && below <= 0) {
    if (IsDroppedDown()) {
      
      nsView* view = mDropdownFrame->GetView();
      view->GetViewManager()->SetViewVisibility(view, nsViewVisibility_kHide);
      NS_DispatchToCurrentThread(new nsAsyncRollup(this));
    }
    return eDropDownPositionSuppressed;
  }

  nsSize dropdownSize = mDropdownFrame->GetSize();
  nscoord height = std::max(above, below);
  nsListControlFrame* lcf = static_cast<nsListControlFrame*>(mDropdownFrame);
  if (height < dropdownSize.height) {
    if (lcf->GetNumDisplayRows() > 1) {
      
      
      NS_DispatchToCurrentThread(new nsAsyncResize(this));
      return eDropDownPositionPendingResize;
    }
  } else if (height > (dropdownSize.height + lcf->GetHeightOfARow() * 1.5) &&
             lcf->GetDropdownCanGrow()) {
    
    
    
    
    NS_DispatchToCurrentThread(new nsAsyncResize(this));
    return eDropDownPositionPendingResize;
  }

  
  
  
  bool b = dropdownSize.height <= below || dropdownSize.height > above;
  nsPoint dropdownPosition(0, b ? GetRect().height : -dropdownSize.height);
  if (StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    
    dropdownPosition.x = GetRect().width - dropdownSize.width;
  }

  
  
  const nsPoint currentPos = mDropdownFrame->GetPosition();
  const nsPoint newPos = dropdownPosition + translation;
  if (currentPos != newPos) {
    mDropdownFrame->SetPosition(newPos);
    nsContainerFrame::PositionFrameView(mDropdownFrame);
  }
  return eDropDownPositionFinal;
}

void
nsComboboxControlFrame::NotifyGeometryChange()
{
  
  
  
  if (IsDroppedDown() &&
      !(GetStateBits() & NS_FRAME_IS_DIRTY) &&
      !mDelayedShowDropDown) {
    
    
    nsRefPtr<nsResizeDropdownAtFinalPosition> resize =
      new nsResizeDropdownAtFinalPosition(this);
    NS_DispatchToCurrentThread(resize);
  }
}




#ifdef DO_REFLOW_DEBUG
static int myCounter = 0;

static void printSize(char * aDesc, nscoord aSize)
{
  printf(" %s: ", aDesc);
  if (aSize == NS_UNCONSTRAINEDSIZE) {
    printf("UC");
  } else {
    printf("%d", PX(aSize));
  }
}
#endif





nscoord
nsComboboxControlFrame::GetIntrinsicISize(nsRenderingContext* aRenderingContext,
                                          nsLayoutUtils::IntrinsicISizeType aType)
{
  
  nscoord scrollbarWidth = 0;
  nsPresContext* presContext = PresContext();
  if (mListControlFrame) {
    nsIScrollableFrame* scrollable = do_QueryFrame(mListControlFrame);
    NS_ASSERTION(scrollable, "List must be a scrollable frame");
    scrollbarWidth = scrollable->GetNondisappearingScrollbarWidth(
      presContext, aRenderingContext);
  }

  nscoord displayWidth = 0;
  if (MOZ_LIKELY(mDisplayFrame)) {
    displayWidth = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                        mDisplayFrame,
                                                        aType);
  }

  if (mDropdownFrame) {
    nscoord dropdownContentWidth;
    bool isUsingOverlayScrollbars =
      LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars) != 0;
    if (aType == nsLayoutUtils::MIN_ISIZE) {
      dropdownContentWidth = mDropdownFrame->GetMinISize(aRenderingContext);
      if (isUsingOverlayScrollbars) {
        dropdownContentWidth += scrollbarWidth;
      }
    } else {
      NS_ASSERTION(aType == nsLayoutUtils::PREF_ISIZE, "Unexpected type");
      dropdownContentWidth = mDropdownFrame->GetPrefISize(aRenderingContext);
      if (isUsingOverlayScrollbars) {
        dropdownContentWidth += scrollbarWidth;
      }
    }
    dropdownContentWidth = NSCoordSaturatingSubtract(dropdownContentWidth,
                                                     scrollbarWidth,
                                                     nscoord_MAX);

    displayWidth = std::max(dropdownContentWidth, displayWidth);
  }

  
  if ((!IsThemed() ||
       presContext->GetTheme()->ThemeNeedsComboboxDropmarker()) &&
      StyleDisplay()->mAppearance != NS_THEME_NONE) {
    displayWidth += scrollbarWidth;
  }

  return displayWidth;

}

nscoord
nsComboboxControlFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord minWidth;
  DISPLAY_MIN_WIDTH(this, minWidth);
  minWidth = GetIntrinsicISize(aRenderingContext, nsLayoutUtils::MIN_ISIZE);
  return minWidth;
}

nscoord
nsComboboxControlFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord prefWidth;
  DISPLAY_PREF_WIDTH(this, prefWidth);
  prefWidth = GetIntrinsicISize(aRenderingContext, nsLayoutUtils::PREF_ISIZE);
  return prefWidth;
}

void
nsComboboxControlFrame::Reflow(nsPresContext*          aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  MarkInReflow();
  

  
  
  
  
  
  
  
  

  if (!mDisplayFrame || !mButtonFrame || !mDropdownFrame) {
    NS_ERROR("Why did the frame constructor allow this to happen?  Fix it!!");
    return;
  }

  
  int32_t selectedIndex;
  nsAutoString selectedOptionText;
  if (!mDroppedDown) {
    selectedIndex = mListControlFrame->GetSelectedIndex();
  }
  else {
    
    
    selectedIndex = mDisplayedIndex;
  }
  if (selectedIndex != -1) {
    mListControlFrame->GetOptionText(selectedIndex, selectedOptionText);
  }
  if (mDisplayedOptionText != selectedOptionText) {
    RedisplayText(selectedIndex);
  }

  
  ReflowDropdown(aPresContext, aReflowState);
  nsRefPtr<nsResizeDropdownAtFinalPosition> resize =
    new nsResizeDropdownAtFinalPosition(this);
  if (NS_SUCCEEDED(aPresContext->PresShell()->PostReflowCallback(resize))) {
    
    
    unused << resize.forget();
  }

  
  
  nscoord buttonWidth;
  const nsStyleDisplay *disp = StyleDisplay();
  if ((IsThemed(disp) && !aPresContext->GetTheme()->ThemeNeedsComboboxDropmarker()) ||
      StyleDisplay()->mAppearance == NS_THEME_NONE) {
    buttonWidth = 0;
  }
  else {
    nsIScrollableFrame* scrollable = do_QueryFrame(mListControlFrame);
    NS_ASSERTION(scrollable, "List must be a scrollable frame");
    buttonWidth = scrollable->GetNondisappearingScrollbarWidth(
      PresContext(), aReflowState.rendContext);
    if (buttonWidth > aReflowState.ComputedWidth()) {
      buttonWidth = 0;
    }
  }

  mDisplayWidth = aReflowState.ComputedWidth() - buttonWidth;

  nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  nsRect buttonRect = mButtonFrame->GetRect();

  if (StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    buttonRect.x = aReflowState.ComputedPhysicalBorderPadding().left -
                   aReflowState.ComputedPhysicalPadding().left;
  }
  else {
    buttonRect.x = aReflowState.ComputedPhysicalBorderPadding().LeftRight() +
                   mDisplayWidth -
                   (aReflowState.ComputedPhysicalBorderPadding().right -
                    aReflowState.ComputedPhysicalPadding().right);
  }
  buttonRect.width = buttonWidth;

  buttonRect.y = this->GetUsedBorder().top;
  buttonRect.height = mDisplayFrame->GetRect().height +
                      this->GetUsedPadding().TopBottom();

  mButtonFrame->SetRect(buttonRect);

  if (!NS_INLINE_IS_BREAK_BEFORE(aStatus) &&
      !NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
    
    
    aStatus = NS_FRAME_COMPLETE;
  }
}



nsIAtom*
nsComboboxControlFrame::GetType() const
{
  return nsGkAtoms::comboboxControlFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsComboboxControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ComboboxControl"), aResult);
}
#endif





void
nsComboboxControlFrame::ShowDropDown(bool aDoDropDown)
{
  mDelayedShowDropDown = false;
  EventStates eventStates = mContent->AsElement()->State();
  if (aDoDropDown && eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    return;
  }

  if (!mDroppedDown && aDoDropDown) {
    nsFocusManager* fm = nsFocusManager::GetFocusManager();
    if (!fm || fm->GetFocusedContent() == GetContent()) {
      DropDownPositionState state = AbsolutelyPositionDropDown();
      if (state == eDropDownPositionFinal) {
        ShowList(aDoDropDown); 
      } else if (state == eDropDownPositionPendingResize) {
        
        mDelayedShowDropDown = true;
      }
    } else {
      
      mDelayedShowDropDown = true;
    }
  } else if (mDroppedDown && !aDoDropDown) {
    ShowList(aDoDropDown); 
  }
}

void
nsComboboxControlFrame::SetDropDown(nsIFrame* aDropDownFrame)
{
  mDropdownFrame = aDropDownFrame;
  mListControlFrame = do_QueryFrame(mDropdownFrame);
}

nsIFrame*
nsComboboxControlFrame::GetDropDown()
{
  return mDropdownFrame;
}



NS_IMETHODIMP
nsComboboxControlFrame::RedisplaySelectedText()
{
  nsAutoScriptBlocker scriptBlocker;
  return RedisplayText(mListControlFrame->GetSelectedIndex());
}

nsresult
nsComboboxControlFrame::RedisplayText(int32_t aIndex)
{
  
  if (aIndex != -1) {
    mListControlFrame->GetOptionText(aIndex, mDisplayedOptionText);
  } else {
    mDisplayedOptionText.Truncate();
  }
  mDisplayedIndex = aIndex;

  REFLOW_DEBUG_MSG2("RedisplayText \"%s\"\n",
                    NS_LossyConvertUTF16toASCII(mDisplayedOptionText).get());

  
  nsresult rv = NS_OK;
  if (mDisplayContent) {
    
    
    

    
    
    mRedisplayTextEvent.Revoke();

    NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                 "If we happen to run our redisplay event now, we might kill "
                 "ourselves!");

    nsRefPtr<RedisplayTextEvent> event = new RedisplayTextEvent(this);
    mRedisplayTextEvent = event;
    if (!nsContentUtils::AddScriptRunner(event))
      mRedisplayTextEvent.Forget();
  }
  return rv;
}

void
nsComboboxControlFrame::HandleRedisplayTextEvent()
{
  
  
  
  
  
  
  nsWeakFrame weakThis(this);
  PresContext()->Document()->
    FlushPendingNotifications(Flush_ContentAndNotify);
  if (!weakThis.IsAlive())
    return;

  
  
  
  NS_PRECONDITION(!mInRedisplayText, "Nested RedisplayText");
  mInRedisplayText = true;
  mRedisplayTextEvent.Forget();

  ActuallyDisplayText(true);
  
  PresContext()->PresShell()->FrameNeedsReflow(mDisplayFrame,
                                               nsIPresShell::eStyleChange,
                                               NS_FRAME_IS_DIRTY);

  mInRedisplayText = false;
}

void
nsComboboxControlFrame::ActuallyDisplayText(bool aNotify)
{
  if (mDisplayedOptionText.IsEmpty()) {
    
    
    static const char16_t space = 0xA0;
    mDisplayContent->SetText(&space, 1, aNotify);
  } else {
    mDisplayContent->SetText(mDisplayedOptionText, aNotify);
  }
}

int32_t
nsComboboxControlFrame::GetIndexOfDisplayArea()
{
  return mDisplayedIndex;
}




NS_IMETHODIMP
nsComboboxControlFrame::DoneAddingChildren(bool aIsDone)
{
  nsISelectControlFrame* listFrame = do_QueryFrame(mDropdownFrame);
  if (!listFrame)
    return NS_ERROR_FAILURE;

  return listFrame->DoneAddingChildren(aIsDone);
}

NS_IMETHODIMP
nsComboboxControlFrame::AddOption(int32_t aIndex)
{
  if (aIndex <= mDisplayedIndex) {
    ++mDisplayedIndex;
  }

  nsListControlFrame* lcf = static_cast<nsListControlFrame*>(mDropdownFrame);
  return lcf->AddOption(aIndex);
}


NS_IMETHODIMP
nsComboboxControlFrame::RemoveOption(int32_t aIndex)
{
  nsWeakFrame weakThis(this);
  if (mListControlFrame->GetNumberOfOptions() > 0) {
    if (aIndex < mDisplayedIndex) {
      --mDisplayedIndex;
    } else if (aIndex == mDisplayedIndex) {
      mDisplayedIndex = 0; 
      RedisplayText(mDisplayedIndex);
    }
  }
  else {
    
    RedisplayText(-1);
  }

  if (!weakThis.IsAlive())
    return NS_OK;

  nsListControlFrame* lcf = static_cast<nsListControlFrame*>(mDropdownFrame);
  return lcf->RemoveOption(aIndex);
}

NS_IMETHODIMP
nsComboboxControlFrame::OnSetSelectedIndex(int32_t aOldIndex, int32_t aNewIndex)
{
  nsAutoScriptBlocker scriptBlocker;
  RedisplayText(aNewIndex);
  NS_ASSERTION(mDropdownFrame, "No dropdown frame!");

  nsISelectControlFrame* listFrame = do_QueryFrame(mDropdownFrame);
  NS_ASSERTION(listFrame, "No list frame!");

  return listFrame->OnSetSelectedIndex(aOldIndex, aNewIndex);
}




nsresult
nsComboboxControlFrame::HandleEvent(nsPresContext* aPresContext,
                                    WidgetGUIEvent* aEvent,
                                    nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  EventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    return NS_OK;
  }

#if COMBOBOX_ROLLUP_CONSUME_EVENT == 0
  if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
    nsIWidget* widget = GetNearestWidget();
    if (widget && GetContent() == widget->GetLastRollup()) {
      
      
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
      return NS_OK;
    }
  }
#endif

  
  
  const nsStyleUserInterface* uiStyle = StyleUserInterface();
  if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE || uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
    return nsBlockFrame::HandleEvent(aPresContext, aEvent, aEventStatus);

  return NS_OK;
}


nsresult
nsComboboxControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  nsIFormControlFrame* fcFrame = do_QueryFrame(mDropdownFrame);
  if (!fcFrame) {
    return NS_NOINTERFACE;
  }

  return fcFrame->SetFormProperty(aName, aValue);
}

nsContainerFrame*
nsComboboxControlFrame::GetContentInsertionFrame() {
  return mInRedisplayText ? mDisplayFrame : mDropdownFrame->GetContentInsertionFrame();
}

nsresult
nsComboboxControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  
  
  
  
  
  
  
  

  
  
  

    
  

  

  nsNodeInfoManager *nimgr = mContent->NodeInfo()->NodeInfoManager();

  mDisplayContent = new nsTextNode(nimgr);

  
  mDisplayedIndex = mListControlFrame->GetSelectedIndex();
  if (mDisplayedIndex != -1) {
    mListControlFrame->GetOptionText(mDisplayedIndex, mDisplayedOptionText);
  }
  ActuallyDisplayText(false);

  if (!aElements.AppendElement(mDisplayContent))
    return NS_ERROR_OUT_OF_MEMORY;

  mButtonContent = mContent->OwnerDoc()->CreateHTMLElement(nsGkAtoms::button);
  if (!mButtonContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  mButtonListener = new nsComboButtonListener(this);
  mButtonContent->AddEventListener(NS_LITERAL_STRING("click"), mButtonListener,
                                   false, false);

  mButtonContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                          NS_LITERAL_STRING("button"), false);
  
  mButtonContent->SetAttr(kNameSpaceID_None, nsGkAtoms::tabindex,
                          NS_LITERAL_STRING("-1"), false);

  if (!aElements.AppendElement(mButtonContent))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

void
nsComboboxControlFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                                 uint32_t aFilter)
{
  if (mDisplayContent) {
    aElements.AppendElement(mDisplayContent);
  }

  if (mButtonContent) {
    aElements.AppendElement(mButtonContent);
  }
}



class nsComboboxDisplayFrame : public nsBlockFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsComboboxDisplayFrame (nsStyleContext* aContext,
                          nsComboboxControlFrame* aComboBox)
    : nsBlockFrame(aContext),
      mComboBox(aComboBox)
  {}

  
  
  virtual nsIAtom* GetType() const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplacedContainsBlock));
  }

  virtual void Reflow(nsPresContext*           aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

protected:
  nsComboboxControlFrame* mComboBox;
};

NS_IMPL_FRAMEARENA_HELPERS(nsComboboxDisplayFrame)

nsIAtom*
nsComboboxDisplayFrame::GetType() const
{
  return nsGkAtoms::comboboxDisplayFrame;
}

void
nsComboboxDisplayFrame::Reflow(nsPresContext*           aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  nsHTMLReflowState state(aReflowState);
  if (state.ComputedHeight() == NS_INTRINSICSIZE) {
    
    
    
    state.SetComputedHeight(mComboBox->mListControlFrame->GetHeightOfARow());
  }
  nscoord computedWidth = mComboBox->mDisplayWidth -
    state.ComputedPhysicalBorderPadding().LeftRight();
  if (computedWidth < 0) {
    computedWidth = 0;
  }
  state.SetComputedWidth(computedWidth);

  return nsBlockFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
}

void
nsComboboxDisplayFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  nsDisplayListCollection set;
  nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, set);

  
  if (mComboBox->IsThemed()) {
    set.BorderBackground()->DeleteAll();
  }

  set.MoveTo(aLists);
}

nsIFrame*
nsComboboxControlFrame::CreateFrameFor(nsIContent*      aContent)
{
  NS_PRECONDITION(nullptr != aContent, "null ptr");

  NS_ASSERTION(mDisplayContent, "mDisplayContent can't be null!");

  if (mDisplayContent != aContent) {
    
    return nullptr;
  }

  
  nsIPresShell *shell = PresContext()->PresShell();
  nsStyleSet *styleSet = shell->StyleSet();

  
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = styleSet->
    ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozDisplayComboboxControlFrame,
                             mStyleContext);

  nsRefPtr<nsStyleContext> textStyleContext;
  textStyleContext = styleSet->ResolveStyleForNonElement(mStyleContext);

  
  mDisplayFrame = new (shell) nsComboboxDisplayFrame(styleContext, this);
  mDisplayFrame->Init(mContent, this, nullptr);

  
  nsIFrame* textFrame = NS_NewTextFrame(shell, textStyleContext);

  
  textFrame->Init(aContent, mDisplayFrame, nullptr);
  mDisplayContent->SetPrimaryFrame(textFrame);

  nsFrameList textList(textFrame, textFrame);
  mDisplayFrame->SetInitialChildList(kPrincipalList, textList);
  return mDisplayFrame;
}

void
nsComboboxControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  mRedisplayTextEvent.Revoke();

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);

  if (mDroppedDown) {
    MOZ_ASSERT(mDropdownFrame, "mDroppedDown without frame");
    nsView* view = mDropdownFrame->GetView();
    MOZ_ASSERT(view);
    nsIWidget* widget = view->GetWidget();
    if (widget) {
      widget->CaptureRollupEvents(this, false);
    }
  }

  
  mPopupFrames.DestroyFramesFrom(aDestructRoot);
  nsContentUtils::DestroyAnonymousContent(&mDisplayContent);
  nsContentUtils::DestroyAnonymousContent(&mButtonContent);
  nsBlockFrame::DestroyFrom(aDestructRoot);
}

const nsFrameList&
nsComboboxControlFrame::GetChildList(ChildListID aListID) const
{
  if (kSelectPopupList == aListID) {
    return mPopupFrames;
  }
  return nsBlockFrame::GetChildList(aListID);
}

void
nsComboboxControlFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  nsBlockFrame::GetChildLists(aLists);
  mPopupFrames.AppendIfNonempty(aLists, kSelectPopupList);
}

void
nsComboboxControlFrame::SetInitialChildList(ChildListID     aListID,
                                            nsFrameList&    aChildList)
{
  if (kSelectPopupList == aListID) {
    mPopupFrames.SetFrames(aChildList);
  } else {
    for (nsFrameList::Enumerator e(aChildList); !e.AtEnd(); e.Next()) {
      nsCOMPtr<nsIFormControl> formControl =
        do_QueryInterface(e.get()->GetContent());
      if (formControl && formControl->GetType() == NS_FORM_BUTTON_BUTTON) {
        mButtonFrame = e.get();
        break;
      }
    }
    NS_ASSERTION(mButtonFrame, "missing button frame in initial child list");
    nsBlockFrame::SetInitialChildList(aListID, aChildList);
  }
}


  

bool
nsComboboxControlFrame::Rollup(uint32_t aCount, bool aFlush,
                               const nsIntPoint* pos, nsIContent** aLastRolledUp)
{
  if (!mDroppedDown) {
    return false;
  }

  bool consume = !!COMBOBOX_ROLLUP_CONSUME_EVENT;
  nsWeakFrame weakFrame(this);
  mListControlFrame->AboutToRollup(); 
  if (!weakFrame.IsAlive()) {
    return consume;
  }
  ShowDropDown(false); 
  if (weakFrame.IsAlive()) {
    mListControlFrame->CaptureMouseEvents(false);
  }

  if (aFlush && weakFrame.IsAlive()) {
    
    
    nsViewManager* viewManager = mDropdownFrame->GetView()->GetViewManager();
    viewManager->UpdateWidgetGeometry();
  }

  if (aLastRolledUp) {
    *aLastRolledUp = GetContent();
  }
  return consume;
}

nsIWidget*
nsComboboxControlFrame::GetRollupWidget()
{
  nsView* view = mDropdownFrame->GetView();
  MOZ_ASSERT(view);
  return view->GetWidget();
}

void
nsComboboxControlFrame::RollupFromList()
{
  if (ShowList(false))
    mListControlFrame->CaptureMouseEvents(false);
}

int32_t
nsComboboxControlFrame::UpdateRecentIndex(int32_t aIndex)
{
  int32_t index = mRecentSelectedIndex;
  if (mRecentSelectedIndex == NS_SKIP_NOTIFY_INDEX || aIndex == NS_SKIP_NOTIFY_INDEX)
    mRecentSelectedIndex = aIndex;
  return index;
}

class nsDisplayComboboxFocus : public nsDisplayItem {
public:
  nsDisplayComboboxFocus(nsDisplayListBuilder* aBuilder,
                         nsComboboxControlFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayComboboxFocus);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayComboboxFocus() {
    MOZ_COUNT_DTOR(nsDisplayComboboxFocus);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("ComboboxFocus", TYPE_COMBOBOX_FOCUS)
};

void nsDisplayComboboxFocus::Paint(nsDisplayListBuilder* aBuilder,
                                   nsRenderingContext* aCtx)
{
  static_cast<nsComboboxControlFrame*>(mFrame)
    ->PaintFocus(*aCtx->GetDrawTarget(), ToReferenceFrame());
}

void
nsComboboxControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
#ifdef NOISY
  printf("%p paint at (%d, %d, %d, %d)\n", this,
    aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
#endif

  if (aBuilder->IsForEventDelivery()) {
    
    
    DisplayBorderBackgroundOutline(aBuilder, aLists);
  } else {
    
    
    nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  
  nsIDocument* doc = mContent->GetComposedDoc();
  if (doc) {
    nsPIDOMWindow* window = doc->GetWindow();
    if (window && window->ShouldShowFocusRing()) {
      nsPresContext *presContext = PresContext();
      const nsStyleDisplay *disp = StyleDisplay();
      if ((!IsThemed(disp) ||
           !presContext->GetTheme()->ThemeDrawsFocusForWidget(disp->mAppearance)) &&
          mDisplayFrame && IsVisibleForPainting(aBuilder)) {
        aLists.Content()->AppendNewToTop(
          new (aBuilder) nsDisplayComboboxFocus(aBuilder, this));
      }
    }
  }

  DisplaySelectionOverlay(aBuilder, aLists.Content());
}

void nsComboboxControlFrame::PaintFocus(DrawTarget& aDrawTarget,
                                        nsPoint aPt)
{
  
  EventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED) || sFocused != this)
    return;

  int32_t appUnitsPerDevPixel = PresContext()->AppUnitsPerDevPixel();

  nsRect clipRect = mDisplayFrame->GetRect() + aPt;
  aDrawTarget.PushClipRect(NSRectToSnappedRect(clipRect,
                                               appUnitsPerDevPixel,
                                               aDrawTarget));

  
  
  

  
  

  StrokeOptions strokeOptions;
  nsLayoutUtils::InitDashPattern(strokeOptions, NS_STYLE_BORDER_STYLE_DOTTED);
  ColorPattern color(ToDeviceColor(StyleColor()->mColor));
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  clipRect.width -= onePixel;
  clipRect.height -= onePixel;
  Rect r = ToRect(nsLayoutUtils::RectToGfxRect(clipRect, appUnitsPerDevPixel));
  StrokeSnappedEdgesOfRect(r, aDrawTarget, color, strokeOptions);

  aDrawTarget.PopClip();
}





NS_IMETHODIMP
nsComboboxControlFrame::OnOptionSelected(int32_t aIndex, bool aSelected)
{
  if (mDroppedDown) {
    nsISelectControlFrame *selectFrame = do_QueryFrame(mListControlFrame);
    if (selectFrame) {
      selectFrame->OnOptionSelected(aIndex, aSelected);
    }
  } else {
    if (aSelected) {
      nsAutoScriptBlocker blocker;
      RedisplayText(aIndex);
    } else {
      nsWeakFrame weakFrame(this);
      RedisplaySelectedText();
      if (weakFrame.IsAlive()) {
        FireValueChangeEvent(); 
      }
    }
  }

  return NS_OK;
}

void nsComboboxControlFrame::FireValueChangeEvent()
{
  
  nsContentUtils::AddScriptRunner(
    new AsyncEventDispatcher(mContent, NS_LITERAL_STRING("ValueChange"), true,
                             false));
}

void
nsComboboxControlFrame::OnContentReset()
{
  if (mListControlFrame) {
    mListControlFrame->OnContentReset();
  }
}





NS_IMETHODIMP
nsComboboxControlFrame::SaveState(nsPresState** aState)
{
  if (!mListControlFrame)
    return NS_ERROR_FAILURE;

  nsIStatefulFrame* stateful = do_QueryFrame(mListControlFrame);
  return stateful->SaveState(aState);
}

NS_IMETHODIMP
nsComboboxControlFrame::RestoreState(nsPresState* aState)
{
  if (!mListControlFrame)
    return NS_ERROR_FAILURE;

  nsIStatefulFrame* stateful = do_QueryFrame(mListControlFrame);
  NS_ASSERTION(stateful, "Must implement nsIStatefulFrame");
  return stateful->RestoreState(aState);
}






bool
nsComboboxControlFrame::ToolkitHasNativePopup()
{
#ifdef MOZ_USE_NATIVE_POPUP_WINDOWS
  return true;
#else
  return false;
#endif 
}

