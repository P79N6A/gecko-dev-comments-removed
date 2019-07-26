



#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsComboboxControlFrame.h"
#include "nsIDOMEventTarget.h"
#include "nsFrameManager.h"
#include "nsFormControlFrame.h"
#include "nsGfxButtonControlFrame.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsHTMLParts.h"
#include "nsIFormControl.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMElement.h"
#include "nsIListControlFrame.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsPIDOMWindow.h"
#include "nsIPresShell.h"
#include "nsContentList.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsEventDispatcher.h"
#include "nsEventListenerManager.h"
#include "nsIDOMNode.h"
#include "nsISelectControlFrame.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"
#include "nsContentUtils.h"
#include "nsTextFragment.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDocument.h"
#include "nsINodeInfo.h"
#include "nsIScrollableFrame.h"
#include "nsListControlFrame.h"
#include "nsContentCID.h"
#include "nsIServiceManager.h"
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsITheme.h"
#include "nsThemeConstants.h"
#include "nsAsyncDOMEvent.h"
#include "nsRenderingContext.h"
#include "mozilla/Preferences.h"
#include "nsContentList.h"
#include "mozilla/Likely.h"

using namespace mozilla;

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
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD HandleEvent(nsIDOMEvent*)
  {
    mComboBox->ShowDropDown(!mComboBox->IsDroppedDown());
    return NS_OK;
  }

  nsComboButtonListener(nsComboboxControlFrame* aCombobox)
  {
    mComboBox = aCombobox;
  }

  virtual ~nsComboButtonListener() {}

  nsComboboxControlFrame* mComboBox;
};

NS_IMPL_ISUPPORTS1(nsComboButtonListener,
                   nsIDOMEventListener)


nsComboboxControlFrame* nsComboboxControlFrame::sFocused = nullptr;

nsIFrame*
NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, uint32_t aStateFlags)
{
  nsComboboxControlFrame* it = new (aPresShell) nsComboboxControlFrame(aContext);

  if (it) {
    
    it->AddStateBits(aStateFlags);
  }

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsComboboxControlFrame)

namespace {

class DestroyWidgetRunnable : public nsRunnable {
public:
  NS_DECL_NSIRUNNABLE

  explicit DestroyWidgetRunnable(nsIContent* aCombobox) :
    mCombobox(aCombobox),
    mWidget(GetWidget())
  {
  }

private:
  nsIWidget* GetWidget(nsIView** aOutView = nullptr) const;

private:
  nsCOMPtr<nsIContent> mCombobox;
  nsIWidget* mWidget;
};

NS_IMETHODIMP DestroyWidgetRunnable::Run()
{
  nsIView* view = nullptr;
  nsIWidget* currentWidget = GetWidget(&view);
  
  
  if (view && mWidget && mWidget == currentWidget) {
    view->DestroyWidget();
  }
  return NS_OK;
}

nsIWidget* DestroyWidgetRunnable::GetWidget(nsIView** aOutView) const
{
  nsIFrame* primaryFrame = mCombobox->GetPrimaryFrame();
  nsIComboboxControlFrame* comboboxFrame = do_QueryFrame(primaryFrame);
  if (comboboxFrame) {
    nsIFrame* dropdown = comboboxFrame->GetDropDown();
    if (dropdown) {
      nsIView* view = dropdown->GetView();
      NS_ASSERTION(view, "nsComboboxControlFrame view is null");
      if (aOutView) {
        *aOutView = view;
      }
      if (view) {
        return view->GetWidget();
      }
    }
  }
  return nullptr;
}

}





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
  return a11y::eHTMLComboboxAccessible;
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
  nsIView* view = mDropdownFrame->GetView();
  nsIViewManager* viewManager = view->GetViewManager();

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
  nsMouseEvent event(true, aShowPopup ?
                     NS_XUL_POPUP_SHOWING : NS_XUL_POPUP_HIDING, nullptr,
                     nsMouseEvent::eReal);

  nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();
  if (shell)
    shell->HandleDOMEventWithTarget(mContent, &event, &status);
}

bool
nsComboboxControlFrame::ShowList(bool aShowList)
{
  nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();

  nsWeakFrame weakFrame(this);

  if (aShowList) {
    nsIView* view = mDropdownFrame->GetView();
    NS_ASSERTION(!view->HasWidget(),
                 "We shouldn't have a widget before we need to display the popup");

    
    view->GetViewManager()->SetViewFloating(view, true);

    nsWidgetInitData widgetData;
    widgetData.mWindowType  = eWindowType_popup;
    widgetData.mBorderStyle = eBorderStyle_default;
    view->CreateWidgetForPopup(&widgetData);
  }

  ShowPopup(aShowList);  
  if (!weakFrame.IsAlive()) {
    return false;
  }

  mDroppedDown = aShowList;
  if (mDroppedDown) {
    
    
    mListControlFrame->AboutToDropDown();
    mListControlFrame->CaptureMouseEvents(true);
  }

  
  shell->GetDocument()->FlushPendingNotifications(Flush_Layout);
  if (!weakFrame.IsAlive()) {
    return false;
  }

  nsIFrame* listFrame = do_QueryFrame(mListControlFrame);
  if (listFrame) {
    nsIView* view = listFrame->GetView();
    NS_ASSERTION(view, "nsComboboxControlFrame view is null");
    if (view) {
      nsIWidget* widget = view->GetWidget();
      if (widget) {
        widget->CaptureRollupEvents(this, mDroppedDown);

        if (!aShowList) {
          nsCOMPtr<nsIRunnable> widgetDestroyer =
            new DestroyWidgetRunnable(GetContent());
          NS_DispatchToMainThread(widgetDestroyer);
        }
      }
    }
  }

  return weakFrame.IsAlive();
}

class nsResizeDropdownAtFinalPosition
  : public nsIReflowCallback, public nsRunnable
{
public:
  nsResizeDropdownAtFinalPosition(nsComboboxControlFrame* aFrame)
    : mFrame(aFrame)
  {
    MOZ_COUNT_CTOR(nsResizeDropdownAtFinalPosition);
  }
  ~nsResizeDropdownAtFinalPosition()
  {
    MOZ_COUNT_DTOR(nsResizeDropdownAtFinalPosition);
  }

  virtual bool ReflowFinished()
  {
    Run();
    NS_RELEASE_THIS();
    return false;
  }

  virtual void ReflowCallbackCanceled()
  {
    NS_RELEASE_THIS();
  }

  NS_IMETHODIMP Run()
  {
    if (mFrame.IsAlive()) {
      static_cast<nsComboboxControlFrame*>(mFrame.GetFrame())->
        AbsolutelyPositionDropDown();
    }
    return NS_OK;
  }

  nsWeakFrame mFrame;
};

nsresult
nsComboboxControlFrame::ReflowDropdown(nsPresContext*  aPresContext,
                                       const nsHTMLReflowState& aReflowState)
{
  
  
  
  if (!aReflowState.ShouldReflowAllKids() &&
      !NS_SUBTREE_DIRTY(mDropdownFrame)) {
    return NS_OK;
  }

  
  
  
  nsSize availSize(aReflowState.availableWidth, NS_UNCONSTRAINEDSIZE);
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, mDropdownFrame,
                                   availSize);

  
  
  
  nscoord forcedWidth = aReflowState.ComputedWidth() +
    aReflowState.mComputedBorderPadding.LeftRight() -
    kidReflowState.mComputedBorderPadding.LeftRight();
  kidReflowState.SetComputedWidth(NS_MAX(kidReflowState.ComputedWidth(),
                                         forcedWidth));

  
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    nsIView* view = mDropdownFrame->GetView();
    nsIViewManager* viewManager = view->GetViewManager();
    viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
    nsRect emptyRect(0, 0, 0, 0);
    viewManager->ResizeView(view, emptyRect);
  }

  
  
  int32_t flags = NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_VISIBILITY | NS_FRAME_NO_SIZE_VIEW;
  if (mDroppedDown) {
    flags = 0;
  }
  nsRect rect = mDropdownFrame->GetRect();
  nsHTMLReflowMetrics desiredSize;
  nsReflowStatus ignoredStatus;
  nsresult rv = ReflowChild(mDropdownFrame, aPresContext, desiredSize,
                            kidReflowState, rect.x, rect.y, flags,
                            ignoredStatus);

   
  FinishReflowChild(mDropdownFrame, aPresContext, &kidReflowState,
                    desiredSize, rect.x, rect.y, flags);
  return rv;
}

nsPoint
nsComboboxControlFrame::GetCSSTransformTranslation()
{
  nsIFrame* frame = this;
  bool is3DTransform = false;
  gfxMatrix transform;
  while (frame) {
    nsIFrame* parent;
    gfx3DMatrix ctm = frame->GetTransformMatrix(nullptr, &parent);
    gfxMatrix matrix;
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
    gfxPoint pixelTranslation = transform.GetTranslation();
    int32_t apd = pc->AppUnitsPerDevPixel();
    translation.x = NSFloatPixelsToAppUnits(float(pixelTranslation.x), apd);
    translation.y = NSFloatPixelsToAppUnits(float(pixelTranslation.y), apd);
    
    
    nsRootPresContext* rootPC = pc->GetRootPresContext();
    if (rootPC) {
      translation -= GetOffsetToCrossDoc(rootPC->PresShell()->GetRootFrame());
    } else {
      translation.x = translation.y = 0;
    }
  }
  return translation;
}

class nsAsyncRollup : public nsRunnable
{
public:
  nsAsyncRollup(nsComboboxControlFrame* aFrame) : mFrame(aFrame) {}
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
  nsAsyncResize(nsComboboxControlFrame* aFrame) : mFrame(aFrame) {}
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
    
    nsIView* view = mDropdownFrame->GetView();
    view->GetViewManager()->SetViewVisibility(view, nsViewVisibility_kHide);
    NS_DispatchToCurrentThread(new nsAsyncRollup(this));
    return eDropDownPositionSuppressed;
  }

  nsSize dropdownSize = mDropdownFrame->GetSize();
  nscoord height = NS_MAX(above, below);
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

  
  
  bool b = dropdownSize.height <= below || below >= above;
  nsPoint dropdownPosition(0, b ? GetRect().height : -dropdownSize.height);
  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    
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
nsComboboxControlFrame::GetIntrinsicWidth(nsRenderingContext* aRenderingContext,
                                          nsLayoutUtils::IntrinsicWidthType aType)
{
  
  nscoord scrollbarWidth = 0;
  nsPresContext* presContext = PresContext();
  if (mListControlFrame) {
    nsIScrollableFrame* scrollable = do_QueryFrame(mListControlFrame);
    NS_ASSERTION(scrollable, "List must be a scrollable frame");
    scrollbarWidth =
      scrollable->GetDesiredScrollbarSizes(presContext, aRenderingContext).LeftRight();
  }

  nscoord displayWidth = 0;
  if (MOZ_LIKELY(mDisplayFrame)) {
    displayWidth = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                                        mDisplayFrame,
                                                        aType);
  }

  if (mDropdownFrame) {
    nscoord dropdownContentWidth;
    if (aType == nsLayoutUtils::MIN_WIDTH) {
      dropdownContentWidth = mDropdownFrame->GetMinWidth(aRenderingContext);
    } else {
      NS_ASSERTION(aType == nsLayoutUtils::PREF_WIDTH, "Unexpected type");
      dropdownContentWidth = mDropdownFrame->GetPrefWidth(aRenderingContext);
    }
    dropdownContentWidth = NSCoordSaturatingSubtract(dropdownContentWidth,
                                                     scrollbarWidth,
                                                     nscoord_MAX);

    displayWidth = NS_MAX(dropdownContentWidth, displayWidth);
  }

  
  if (!IsThemed() || presContext->GetTheme()->ThemeNeedsComboboxDropmarker())
    displayWidth += scrollbarWidth;

  return displayWidth;

}

nscoord
nsComboboxControlFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord minWidth;
  DISPLAY_MIN_WIDTH(this, minWidth);
  minWidth = GetIntrinsicWidth(aRenderingContext, nsLayoutUtils::MIN_WIDTH);
  return minWidth;
}

nscoord
nsComboboxControlFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord prefWidth;
  DISPLAY_PREF_WIDTH(this, prefWidth);
  prefWidth = GetIntrinsicWidth(aRenderingContext, nsLayoutUtils::PREF_WIDTH);
  return prefWidth;
}

NS_IMETHODIMP
nsComboboxControlFrame::Reflow(nsPresContext*          aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  

  
  
  
  
  
  
  
  

  if (!mDisplayFrame || !mButtonFrame || !mDropdownFrame) {
    NS_ERROR("Why did the frame constructor allow this to happen?  Fix it!!");
    return NS_ERROR_UNEXPECTED;
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
    
    
    resize.forget();
  }

  
  
  nscoord buttonWidth;
  const nsStyleDisplay *disp = GetStyleDisplay();
  if (IsThemed(disp) && !aPresContext->GetTheme()->ThemeNeedsComboboxDropmarker()) {
    buttonWidth = 0;
  }
  else {
    nsIScrollableFrame* scrollable = do_QueryFrame(mListControlFrame);
    NS_ASSERTION(scrollable, "List must be a scrollable frame");
    buttonWidth =
      scrollable->GetDesiredScrollbarSizes(PresContext(),
                                           aReflowState.rendContext).LeftRight();
    if (buttonWidth > aReflowState.ComputedWidth()) {
      buttonWidth = 0;
    }
  }

  mDisplayWidth = aReflowState.ComputedWidth() - buttonWidth;

  nsresult rv = nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState,
                                    aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsRect buttonRect = mButtonFrame->GetRect();
  
  
  if (aReflowState.ComputedHeight() == NS_INTRINSICSIZE) {
    
    
    nsRect displayRect = mDisplayFrame->GetRect();
    buttonRect.height = displayRect.height;
    buttonRect.y = displayRect.y;
  }
#ifdef DEBUG
  else {
    nscoord buttonHeight = mButtonFrame->GetSize().height;
    nscoord displayHeight = mDisplayFrame->GetSize().height;

    
    
    NS_ASSERTION(buttonHeight == displayHeight ||
                 (aReflowState.ComputedHeight() < buttonHeight &&
                  buttonHeight ==
                    mButtonFrame->GetUsedBorderAndPadding().TopBottom()) ||
                 (aReflowState.ComputedHeight() < displayHeight &&
                  displayHeight ==
                    mDisplayFrame->GetUsedBorderAndPadding().TopBottom()),
                 "Different heights?");
  }
#endif

  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    
    buttonRect.x -= buttonWidth - buttonRect.width;
  }
  buttonRect.width = buttonWidth;
  mButtonFrame->SetRect(buttonRect);

  return rv;
}



nsIAtom*
nsComboboxControlFrame::GetType() const
{
  return nsGkAtoms::comboboxControlFrame;
}

#ifdef DEBUG
NS_IMETHODIMP
nsComboboxControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ComboboxControl"), aResult);
}
#endif





void
nsComboboxControlFrame::ShowDropDown(bool aDoDropDown)
{
  mDelayedShowDropDown = false;
  nsEventStates eventStates = mContent->AsElement()->State();
  if (aDoDropDown && eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    return;
  }

  if (!mDroppedDown && aDoDropDown) {
    if (sFocused == this) {
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
    
    
    static const PRUnichar space = 0xA0;
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




NS_IMETHODIMP
nsComboboxControlFrame::HandleEvent(nsPresContext* aPresContext,
                                       nsGUIEvent*     aEvent,
                                       nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED)) {
    return NS_OK;
  }

  
  
  const nsStyleUserInterface* uiStyle = GetStyleUserInterface();
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

nsresult
nsComboboxControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  nsIFormControlFrame* fcFrame = do_QueryFrame(mDropdownFrame);
  if (!fcFrame) {
    return NS_ERROR_FAILURE;
  }

  return fcFrame->GetFormProperty(aName, aValue);
}

nsIFrame*
nsComboboxControlFrame::GetContentInsertionFrame() {
  return mInRedisplayText ? mDisplayFrame : mDropdownFrame->GetContentInsertionFrame();
}

nsresult
nsComboboxControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  
  
  
  
  
  
  
  

  
  
  

    
  

  

  nsNodeInfoManager *nimgr = mContent->NodeInfo()->NodeInfoManager();

  NS_NewTextNode(getter_AddRefs(mDisplayContent), nimgr);
  if (!mDisplayContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mDisplayedIndex = mListControlFrame->GetSelectedIndex();
  if (mDisplayedIndex != -1) {
    mListControlFrame->GetOptionText(mDisplayedIndex, mDisplayedOptionText);
  }
  ActuallyDisplayText(false);

  if (!aElements.AppendElement(mDisplayContent))
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = nimgr->GetNodeInfo(nsGkAtoms::button, nullptr, kNameSpaceID_XHTML,
                                nsIDOMNode::ELEMENT_NODE);

  
  NS_NewHTMLElement(getter_AddRefs(mButtonContent), nodeInfo.forget(),
                    dom::NOT_FROM_PARSER);
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
nsComboboxControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                                 uint32_t aFilter)
{
  aElements.MaybeAppendElement(mDisplayContent);
  aElements.MaybeAppendElement(mButtonContent);
}



class nsComboboxDisplayFrame : public nsBlockFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsComboboxDisplayFrame (nsStyleContext* aContext,
                          nsComboboxControlFrame* aComboBox)
    : nsBlockFrame(aContext),
      mComboBox(aComboBox)
  {}

  
  
  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eReplacedContainsBlock));
  }

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

protected:
  nsComboboxControlFrame* mComboBox;
};

NS_IMPL_FRAMEARENA_HELPERS(nsComboboxDisplayFrame)

nsIAtom*
nsComboboxDisplayFrame::GetType() const
{
  return nsGkAtoms::comboboxDisplayFrame;
}

NS_IMETHODIMP
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
    state.mComputedBorderPadding.LeftRight();
  if (computedWidth < 0) {
    computedWidth = 0;
  }
  state.SetComputedWidth(computedWidth);

  return nsBlockFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
}

NS_IMETHODIMP
nsComboboxDisplayFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  nsDisplayListCollection set;
  nsresult rv = nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, set);
  if (NS_FAILED(rv))
    return rv;

  
  if (mComboBox->IsThemed()) {
    set.BorderBackground()->DeleteAll();
  }

  set.MoveTo(aLists);

  return NS_OK;
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
  if (MOZ_UNLIKELY(!styleContext)) {
    return nullptr;
  }

  nsRefPtr<nsStyleContext> textStyleContext;
  textStyleContext = styleSet->ResolveStyleForNonElement(mStyleContext);
  if (MOZ_UNLIKELY(!textStyleContext)) {
    return nullptr;
  }

  
  mDisplayFrame = new (shell) nsComboboxDisplayFrame(styleContext, this);
  if (MOZ_UNLIKELY(!mDisplayFrame)) {
    return nullptr;
  }

  nsresult rv = mDisplayFrame->Init(mContent, this, nullptr);
  if (NS_FAILED(rv)) {
    mDisplayFrame->Destroy();
    mDisplayFrame = nullptr;
    return nullptr;
  }

  
  nsIFrame* textFrame = NS_NewTextFrame(shell, textStyleContext);
  if (MOZ_UNLIKELY(!textFrame)) {
    return nullptr;
  }

  
  rv = textFrame->Init(aContent, mDisplayFrame, nullptr);
  if (NS_FAILED(rv)) {
    mDisplayFrame->Destroy();
    mDisplayFrame = nullptr;
    textFrame->Destroy();
    textFrame = nullptr;
    return nullptr;
  }
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
    
    nsIFrame * listFrame = do_QueryFrame(mListControlFrame);
    if (listFrame) {
      nsIView* view = listFrame->GetView();
      NS_ASSERTION(view, "nsComboboxControlFrame view is null");
      if (view) {
        nsIWidget* widget = view->GetWidget();
        if (widget)
          widget->CaptureRollupEvents(this, false);
      }
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

NS_IMETHODIMP
nsComboboxControlFrame::SetInitialChildList(ChildListID     aListID,
                                            nsFrameList&    aChildList)
{
  nsresult rv = NS_OK;
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
    rv = nsBlockFrame::SetInitialChildList(aListID, aChildList);
  }
  return rv;
}


  

bool
nsComboboxControlFrame::Rollup(uint32_t aCount, nsIContent** aLastRolledUp)
{
  if (!mDroppedDown)
    return false;

  nsWeakFrame weakFrame(this);
  mListControlFrame->AboutToRollup(); 
  if (!weakFrame.IsAlive())
    return true;
  ShowDropDown(false); 
  if (weakFrame.IsAlive()) {
    mListControlFrame->CaptureMouseEvents(false);
  }

  return true;
}

nsIWidget*
nsComboboxControlFrame::GetRollupWidget()
{
  nsIFrame* listFrame = do_QueryFrame(mListControlFrame);
  if (!listFrame)
    return nullptr;

  nsIView* view = listFrame->GetView();
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
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("ComboboxFocus", TYPE_COMBOBOX_FOCUS)
};

void nsDisplayComboboxFocus::Paint(nsDisplayListBuilder* aBuilder,
                                   nsRenderingContext* aCtx)
{
  static_cast<nsComboboxControlFrame*>(mFrame)
    ->PaintFocus(*aCtx, ToReferenceFrame());
}

NS_IMETHODIMP
nsComboboxControlFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
#ifdef NOISY
  printf("%p paint at (%d, %d, %d, %d)\n", this,
    aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
#endif

  if (aBuilder->IsForEventDelivery()) {
    
    
    nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    
    nsresult rv = nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsIDocument* doc = mContent->GetCurrentDoc();
  if (doc) {
    nsPIDOMWindow* window = doc->GetWindow();
    if (window && window->ShouldShowFocusRing()) {
      nsPresContext *presContext = PresContext();
      const nsStyleDisplay *disp = GetStyleDisplay();
      if ((!IsThemed(disp) ||
           !presContext->GetTheme()->ThemeDrawsFocusForWidget(presContext, this, disp->mAppearance)) &&
          mDisplayFrame && IsVisibleForPainting(aBuilder)) {
        nsresult rv = aLists.Content()->AppendNewToTop(
            new (aBuilder) nsDisplayComboboxFocus(aBuilder, this));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return DisplaySelectionOverlay(aBuilder, aLists.Content());
}

void nsComboboxControlFrame::PaintFocus(nsRenderingContext& aRenderingContext,
                                        nsPoint aPt)
{
  
  nsEventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED) || sFocused != this)
    return;

  aRenderingContext.PushState();
  nsRect clipRect = mDisplayFrame->GetRect() + aPt;
  aRenderingContext.IntersectClip(clipRect);

  
  
  

  
  

  aRenderingContext.SetLineStyle(nsLineStyle_kDotted);
  aRenderingContext.SetColor(GetStyleColor()->mColor);

  

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  clipRect.width -= onePixel;
  clipRect.height -= onePixel;
  aRenderingContext.DrawLine(clipRect.TopLeft(), clipRect.TopRight());
  aRenderingContext.DrawLine(clipRect.TopRight(), clipRect.BottomRight());
  aRenderingContext.DrawLine(clipRect.BottomRight(), clipRect.BottomLeft());
  aRenderingContext.DrawLine(clipRect.BottomLeft(), clipRect.TopLeft());

  aRenderingContext.PopState();
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
    new nsAsyncDOMEvent(mContent, NS_LITERAL_STRING("ValueChange"), true,
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
nsComboboxControlFrame::SaveState(SpecialStateID aStateID,
                                  nsPresState** aState)
{
  if (!mListControlFrame)
    return NS_ERROR_FAILURE;

  nsIStatefulFrame* stateful = do_QueryFrame(mListControlFrame);
  return stateful->SaveState(aStateID, aState);
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

