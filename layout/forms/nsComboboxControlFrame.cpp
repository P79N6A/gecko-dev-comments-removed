





































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
#include "nsIDOMNSHTMLOptionCollectn.h" 
#include "nsIPresShell.h"
#include "nsIDeviceContext.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsEventDispatcher.h"
#include "nsIEventStateManager.h"
#include "nsIEventListenerManager.h"
#include "nsIDOMNode.h"
#include "nsIPrivateDOMEvent.h"
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
#ifdef ACCESSIBILITY
#include "nsIAccessibilityService.h"
#endif
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsITheme.h"
#include "nsThemeConstants.h"

NS_IMETHODIMP
nsComboboxControlFrame::RedisplayTextEvent::Run()
{
  if (mControlFrame)
    mControlFrame->HandleRedisplayTextEvent();
  return NS_OK;
}

class nsPresState;

#define FIX_FOR_BUG_53259















const PRInt32 kSizeNotSet = -1;






class nsComboButtonListener: public nsIDOMMouseListener
{
  public:

  NS_DECL_ISUPPORTS
  NS_IMETHOD HandleEvent(nsIDOMEvent* anEvent) { return PR_FALSE; }
  NS_IMETHOD MouseDown(nsIDOMEvent* aMouseEvent) { return PR_FALSE; }
  NS_IMETHOD MouseUp(nsIDOMEvent* aMouseEvent) { return PR_FALSE; }
  NS_IMETHOD MouseDblClick(nsIDOMEvent* aMouseEvent) { return PR_FALSE; }
  NS_IMETHOD MouseOver(nsIDOMEvent* aMouseEvent) { return PR_FALSE; }
  NS_IMETHOD MouseOut(nsIDOMEvent* aMouseEvent) { return PR_FALSE; }

  NS_IMETHOD MouseClick(nsIDOMEvent* aMouseEvent) 
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

NS_IMPL_ISUPPORTS2(nsComboButtonListener,
                   nsIDOMMouseListener,
                   nsIDOMEventListener)


nsComboboxControlFrame * nsComboboxControlFrame::mFocused = nsnull;

nsIFrame*
NS_NewComboboxControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aStateFlags)
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
static PRInt32 gTotalReqs    = 0;;
static PRInt32 gTotalReflows = 0;;
static PRInt32 gReflowControlCntRQ[MAX_REFLOW_CNT];
static PRInt32 gReflowControlCnt[MAX_REFLOW_CNT];
static PRInt32 gReflowInx = -1;

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
  : nsBlockFrame(aContext),
    mDisplayWidth(0)
{
  mListControlFrame            = nsnull;
  mDroppedDown                 = PR_FALSE;
  mDisplayFrame                = nsnull;
  mButtonFrame                 = nsnull;
  mDropdownFrame               = nsnull;

  mInRedisplayText = PR_FALSE;

  mRecentSelectedIndex = NS_SKIP_NOTIFY_INDEX;

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
  NS_QUERYFRAME_ENTRY(nsIScrollableViewProvider)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

NS_IMPL_QUERY_INTERFACE1(nsComboboxControlFrame, nsIRollupListener)

NS_IMETHODIMP_(nsrefcnt)
nsComboboxControlFrame::AddRef()
{
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
nsComboboxControlFrame::Release()
{
  return 1;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsComboboxControlFrame::GetAccessible(nsIAccessible** aAccessible)
{
  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(mContent);
    nsCOMPtr<nsIWeakReference> weakShell(do_GetWeakReference(PresContext()->PresShell()));
    return accService->CreateHTMLComboboxAccessible(node, weakShell, aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif

void 
nsComboboxControlFrame::SetFocus(PRBool aOn, PRBool aRepaint)
{
  nsWeakFrame weakFrame(this);
  if (aOn) {
    nsListControlFrame::ComboboxFocusSet();
    mFocused = this;
  } else {
    mFocused = nsnull;
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

  
  
  
  Invalidate(nsRect(0,0,mRect.width,mRect.height));

  
  
  
  
  nsIViewManager* vm = PresContext()->GetPresShell()->GetViewManager();
  if (vm) {
    vm->UpdateAllViews(NS_VMREFRESH_NO_SYNC);
  }
}

void
nsComboboxControlFrame::ShowPopup(PRBool aShowPopup)
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
  nsMouseEvent event(PR_TRUE, aShowPopup ?
                     NS_XUL_POPUP_SHOWING : NS_XUL_POPUP_HIDING, nsnull,
                     nsMouseEvent::eReal);

  nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();
  if (shell) 
    shell->HandleDOMEventWithTarget(mContent, &event, &status);
}

PRBool
nsComboboxControlFrame::ShowList(PRBool aShowList)
{
  nsCOMPtr<nsIPresShell> shell = PresContext()->GetPresShell();

  nsWeakFrame weakFrame(this);
  ShowPopup(aShowList);  
  if (!weakFrame.IsAlive()) {
    return PR_FALSE;
  }

  mDroppedDown = aShowList;
  if (mDroppedDown) {
    
    
    mListControlFrame->AboutToDropDown();
    mListControlFrame->CaptureMouseEvents(PR_TRUE);
  }

  
  shell->GetDocument()->FlushPendingNotifications(Flush_Layout);
  if (!weakFrame.IsAlive()) {
    return PR_FALSE;
  }

  nsIFrame* listFrame = do_QueryFrame(mListControlFrame);
  if (listFrame) {
    nsIView* view = listFrame->GetView();
    NS_ASSERTION(view, "nsComboboxControlFrame view is null");
    if (view) {
      nsIWidget* widget = view->GetWidget();
      if (widget)
        widget->CaptureRollupEvents(this, mDroppedDown, mDroppedDown);
    }
  }

  return weakFrame.IsAlive();
}

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
  kidReflowState.SetComputedWidth(PR_MAX(kidReflowState.ComputedWidth(),
                                         forcedWidth));

  
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    nsIView* view = mDropdownFrame->GetView();
    nsIViewManager* viewManager = view->GetViewManager();
    viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
    nsRect emptyRect(0, 0, 0, 0);
    viewManager->ResizeView(view, emptyRect);
  }
  
  
  
  PRInt32 flags = NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_VISIBILITY | NS_FRAME_NO_SIZE_VIEW;
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

void
nsComboboxControlFrame::AbsolutelyPositionDropDown()
{
   
   
   

   
   
   
   
   
   

   
   
  nscoord dropdownYOffset = GetRect().height;
  nsSize dropdownSize = mDropdownFrame->GetSize();

  nsRect screen = nsFormControlFrame::GetUsableScreenRect(PresContext());

  
  if (GetScreenRectInAppUnits().YMost() + dropdownSize.height > screen.YMost()) {
    
    dropdownYOffset = - (dropdownSize.height);
  }

  nsPoint dropdownPosition;
  const nsStyleVisibility* vis = GetStyleVisibility();
  if (vis->mDirection == NS_STYLE_DIRECTION_RTL) {
    
    dropdownPosition.x = GetRect().width - dropdownSize.width;
  } else {
    dropdownPosition.x = 0;
  }
  dropdownPosition.y = dropdownYOffset; 

  mDropdownFrame->SetPosition(dropdownPosition);
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
nsComboboxControlFrame::GetIntrinsicWidth(nsIRenderingContext* aRenderingContext,
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
  if (NS_LIKELY(mDisplayFrame)) {
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
  
    displayWidth = PR_MAX(dropdownContentWidth, displayWidth);
  }

  
  if (!IsThemed() || presContext->GetTheme()->ThemeNeedsComboboxDropmarker())
    displayWidth += scrollbarWidth;

  return displayWidth;

}

nscoord
nsComboboxControlFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord minWidth;
  DISPLAY_MIN_WIDTH(this, minWidth);
  minWidth = GetIntrinsicWidth(aRenderingContext, nsLayoutUtils::MIN_WIDTH);
  return minWidth;
}

nscoord
nsComboboxControlFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
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

  
  PRInt32 selectedIndex;
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

#ifdef NS_DEBUG
NS_IMETHODIMP
nsComboboxControlFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ComboboxControl"), aResult);
}
#endif





void
nsComboboxControlFrame::ShowDropDown(PRBool aDoDropDown) 
{
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return;
  }

  if (!mDroppedDown && aDoDropDown) {
    if (mListControlFrame) {
      mListControlFrame->SyncViewWithFrame();
    }
    ShowList(aDoDropDown); 
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
nsComboboxControlFrame::RedisplayText(PRInt32 aIndex)
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
  
  
  
  
  
  
  PresContext()->Document()->
    FlushPendingNotifications(Flush_ContentAndNotify);
  
  
  
  
  NS_PRECONDITION(!mInRedisplayText, "Nested RedisplayText");
  mInRedisplayText = PR_TRUE;
  mRedisplayTextEvent.Forget();

  ActuallyDisplayText(PR_TRUE);
  
  PresContext()->PresShell()->FrameNeedsReflow(mDisplayFrame,
                                               nsIPresShell::eStyleChange,
                                               NS_FRAME_IS_DIRTY);

  mInRedisplayText = PR_FALSE;
}

void
nsComboboxControlFrame::ActuallyDisplayText(PRBool aNotify)
{
  if (mDisplayedOptionText.IsEmpty()) {
    
    
    static const PRUnichar space = 0xA0;
    mDisplayContent->SetText(&space, 1, aNotify);
  } else {
    mDisplayContent->SetText(mDisplayedOptionText, aNotify);
  }
}

PRInt32
nsComboboxControlFrame::GetIndexOfDisplayArea()
{
  return mDisplayedIndex;
}




NS_IMETHODIMP
nsComboboxControlFrame::DoneAddingChildren(PRBool aIsDone)
{
  nsISelectControlFrame* listFrame = do_QueryFrame(mDropdownFrame);
  if (!listFrame)
    return NS_ERROR_FAILURE;

  return listFrame->DoneAddingChildren(aIsDone);
}

NS_IMETHODIMP
nsComboboxControlFrame::AddOption(PRInt32 aIndex)
{
  if (aIndex <= mDisplayedIndex) {
    ++mDisplayedIndex;
  }

  nsListControlFrame* lcf = static_cast<nsListControlFrame*>(mDropdownFrame);
  return lcf->AddOption(aIndex);
}
  

NS_IMETHODIMP
nsComboboxControlFrame::RemoveOption(PRInt32 aIndex)
{
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

  nsListControlFrame* lcf = static_cast<nsListControlFrame*>(mDropdownFrame);
  return lcf->RemoveOption(aIndex);
}

NS_IMETHODIMP
nsComboboxControlFrame::GetOptionSelected(PRInt32 aIndex, PRBool* aValue)
{
  NS_ASSERTION(mDropdownFrame, "No dropdown frame!");

  nsISelectControlFrame* listFrame = do_QueryFrame(mDropdownFrame);
  NS_ASSERTION(listFrame, "No list frame!");

  return listFrame->GetOptionSelected(aIndex, aValue);
}

NS_IMETHODIMP
nsComboboxControlFrame::OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex)
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
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
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
nsComboboxControlFrame::CreateAnonymousContent(nsTArray<nsIContent*>& aElements)
{
  
  
  
  
  
  
  
  

  
  
  

    
  

  

  nsNodeInfoManager *nimgr = mContent->NodeInfo()->NodeInfoManager();

  NS_NewTextNode(getter_AddRefs(mDisplayContent), nimgr);
  if (!mDisplayContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mDisplayedIndex = mListControlFrame->GetSelectedIndex();
  if (mDisplayedIndex != -1) {
    mListControlFrame->GetOptionText(mDisplayedIndex, mDisplayedOptionText);
  }
  ActuallyDisplayText(PR_FALSE);

  if (!aElements.AppendElement(mDisplayContent))
    return NS_ERROR_OUT_OF_MEMORY;

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = nimgr->GetNodeInfo(nsGkAtoms::input, nsnull, kNameSpaceID_XHTML);

  
  NS_NewHTMLElement(getter_AddRefs(mButtonContent), nodeInfo, PR_FALSE);
  if (!mButtonContent)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  mButtonListener = new nsComboButtonListener(this);
  if (!mButtonListener)
    return NS_ERROR_OUT_OF_MEMORY;
  mButtonContent->AddEventListenerByIID(mButtonListener,
                                        NS_GET_IID(nsIDOMMouseListener));

  mButtonContent->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                          NS_LITERAL_STRING("button"), PR_FALSE);
  
  mButtonContent->SetAttr(kNameSpaceID_None, nsGkAtoms::tabindex,
                          NS_LITERAL_STRING("-1"), PR_FALSE);

  if (!aElements.AppendElement(mButtonContent))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
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

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
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
  NS_PRECONDITION(nsnull != aContent, "null ptr");

  NS_ASSERTION(mDisplayContent, "mDisplayContent can't be null!");

  if (mDisplayContent != aContent) {
    
    return nsnull;
  }
  
  
  nsIPresShell *shell = PresContext()->PresShell();
  nsStyleSet *styleSet = shell->StyleSet();

  
  nsRefPtr<nsStyleContext> styleContext;
  styleContext = styleSet->
    ResolvePseudoStyleFor(mContent, 
                          nsCSSAnonBoxes::mozDisplayComboboxControlFrame,
                          mStyleContext);
  if (NS_UNLIKELY(!styleContext)) {
    return nsnull;
  }

  nsRefPtr<nsStyleContext> textStyleContext;
  textStyleContext = styleSet->ResolveStyleForNonElement(mStyleContext);
  if (NS_UNLIKELY(!textStyleContext)) {
    return nsnull;
  }

  
  mDisplayFrame = new (shell) nsComboboxDisplayFrame(styleContext, this);
  if (NS_UNLIKELY(!mDisplayFrame)) {
    return nsnull;
  }

  nsresult rv = mDisplayFrame->Init(mContent, this, nsnull);
  if (NS_FAILED(rv)) {
    mDisplayFrame->Destroy();
    mDisplayFrame = nsnull;
    return nsnull;
  }

  
  mTextFrame = NS_NewTextFrame(shell, textStyleContext);
  if (NS_UNLIKELY(!mTextFrame)) {
    return nsnull;
  }

  
  rv = mTextFrame->Init(aContent, mDisplayFrame, nsnull);
  if (NS_FAILED(rv)) {
    mDisplayFrame->Destroy();
    mDisplayFrame = nsnull;
    mTextFrame->Destroy();
    mTextFrame = nsnull;
    return nsnull;
  }

  nsFrameList textList(mTextFrame);
  mDisplayFrame->SetInitialChildList(nsnull, textList);
  return mDisplayFrame;
}

void
nsComboboxControlFrame::Destroy()
{
  
  mRedisplayTextEvent.Revoke();

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_FALSE);

  if (mDroppedDown) {
    
    nsIFrame * listFrame = do_QueryFrame(mListControlFrame);
    if (listFrame) {
      nsIView* view = listFrame->GetView();
      NS_ASSERTION(view, "nsComboboxControlFrame view is null");
      if (view) {
        nsIWidget* widget = view->GetWidget();
        if (widget)
          widget->CaptureRollupEvents(this, PR_FALSE, PR_TRUE);
      }
    }
  }

  
  mPopupFrames.DestroyFrames();
  nsContentUtils::DestroyAnonymousContent(&mDisplayContent);
  nsContentUtils::DestroyAnonymousContent(&mButtonContent);
  nsBlockFrame::Destroy();
}


nsFrameList
nsComboboxControlFrame::GetChildList(nsIAtom* aListName) const
{
  if (nsGkAtoms::selectPopupList == aListName) {
    return mPopupFrames;
  }
  return nsBlockFrame::GetChildList(aListName);
}

NS_IMETHODIMP
nsComboboxControlFrame::SetInitialChildList(nsIAtom*        aListName,
                                            nsFrameList&    aChildList)
{
  nsresult rv = NS_OK;
  if (nsGkAtoms::selectPopupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {
    for (nsFrameList::Enumerator e(aChildList); !e.AtEnd(); e.Next()) {
      nsCOMPtr<nsIFormControl> formControl =
        do_QueryInterface(e.get()->GetContent());
      if (formControl && formControl->GetType() == NS_FORM_INPUT_BUTTON) {
        mButtonFrame = e.get();
        break;
      }
    }
    NS_ASSERTION(mButtonFrame, "missing button frame in initial child list");
    rv = nsBlockFrame::SetInitialChildList(aListName, aChildList);
  }
  return rv;
}

#define NS_COMBO_FRAME_POPUP_LIST_INDEX   (NS_BLOCK_LIST_COUNT)

nsIAtom*
nsComboboxControlFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
   
   
   
   
  if (aIndex < NS_BLOCK_LIST_COUNT) {
    return nsBlockFrame::GetAdditionalChildListName(aIndex);
  }
  
  if (NS_COMBO_FRAME_POPUP_LIST_INDEX == aIndex) {
    return nsGkAtoms::selectPopupList;
  }
  return nsnull;
}


  

NS_IMETHODIMP 
nsComboboxControlFrame::Rollup(PRUint32 aCount,
                               nsIContent** aLastRolledUp)
{
  if (aLastRolledUp)
    *aLastRolledUp = nsnull;

  if (mDroppedDown) {
    nsWeakFrame weakFrame(this);
    mListControlFrame->AboutToRollup(); 
    if (!weakFrame.IsAlive())
      return NS_OK;
    ShowDropDown(PR_FALSE); 
    if (!weakFrame.IsAlive())
      return NS_OK;
    mListControlFrame->CaptureMouseEvents(PR_FALSE);
  }
  return NS_OK;
}

void
nsComboboxControlFrame::RollupFromList()
{
  if (ShowList(PR_FALSE))
    mListControlFrame->CaptureMouseEvents(PR_FALSE);
}

PRInt32
nsComboboxControlFrame::UpdateRecentIndex(PRInt32 aIndex)
{
  PRInt32 index = mRecentSelectedIndex;
  if (mRecentSelectedIndex == NS_SKIP_NOTIFY_INDEX || aIndex == NS_SKIP_NOTIFY_INDEX)
    mRecentSelectedIndex = aIndex;
  return index;
}

class nsDisplayComboboxFocus : public nsDisplayItem {
public:
  nsDisplayComboboxFocus(nsComboboxControlFrame* aFrame)
    : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayComboboxFocus);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayComboboxFocus() {
    MOZ_COUNT_DTOR(nsDisplayComboboxFocus);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("ComboboxFocus")
};

void nsDisplayComboboxFocus::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  static_cast<nsComboboxControlFrame*>(mFrame)
    ->PaintFocus(*aCtx, aBuilder->ToReferenceFrame(mFrame));
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

  nsPresContext *presContext = PresContext();
  const nsStyleDisplay *disp = GetStyleDisplay();
  if ((!IsThemed(disp) ||
       !presContext->GetTheme()->ThemeDrawsFocusForWidget(presContext, this, disp->mAppearance)) &&
      mDisplayFrame && IsVisibleForPainting(aBuilder)) {
    nsresult rv = aLists.Content()->AppendNewToTop(new (aBuilder)
                                                   nsDisplayComboboxFocus(this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return DisplaySelectionOverlay(aBuilder, aLists);
}

void nsComboboxControlFrame::PaintFocus(nsIRenderingContext& aRenderingContext,
                                        nsPoint aPt)
{
  
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled) ||
      mFocused != this)
    return;

  aRenderingContext.PushState();
  nsRect clipRect = mDisplayFrame->GetRect() + aPt;
  aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);

  
  
  

  
  

  aRenderingContext.SetLineStyle(nsLineStyle_kDotted);
  aRenderingContext.SetColor(GetStyleColor()->mColor);

  

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  clipRect.width -= onePixel;
  clipRect.height -= onePixel;
  aRenderingContext.DrawLine(clipRect.x, clipRect.y, 
                             clipRect.x+clipRect.width, clipRect.y);
  aRenderingContext.DrawLine(clipRect.x+clipRect.width, clipRect.y, 
                             clipRect.x+clipRect.width, clipRect.y+clipRect.height);
  aRenderingContext.DrawLine(clipRect.x+clipRect.width, clipRect.y+clipRect.height, 
                             clipRect.x, clipRect.y+clipRect.height);
  aRenderingContext.DrawLine(clipRect.x, clipRect.y+clipRect.height, 
                             clipRect.x, clipRect.y);
  aRenderingContext.DrawLine(clipRect.x, clipRect.y+clipRect.height, 
                             clipRect.x, clipRect.y);

  aRenderingContext.PopState();
}


  

nsIScrollableView* nsComboboxControlFrame::GetScrollableView()
{
  if (!mDropdownFrame)
    return nsnull;

  nsIScrollableFrame* scrollable = do_QueryFrame(mDropdownFrame);
  if (!scrollable)
    return nsnull;

  return scrollable->GetScrollableView();
}





NS_IMETHODIMP
nsComboboxControlFrame::OnOptionSelected(PRInt32 aIndex, PRBool aSelected)
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
  
  nsCOMPtr<nsIDOMEvent> event;
  nsPresContext* presContext = PresContext();
  if (NS_SUCCEEDED(nsEventDispatcher::CreateEvent(presContext, nsnull,
                                                  NS_LITERAL_STRING("Events"),
                                                  getter_AddRefs(event)))) {
    event->InitEvent(NS_LITERAL_STRING("ValueChange"), PR_TRUE, PR_TRUE);

    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
    privateEvent->SetTrusted(PR_TRUE);
    nsEventDispatcher::DispatchDOMEvent(mContent, nsnull, event, nsnull,
                                        nsnull);
  }
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











PRBool
nsComboboxControlFrame::ToolkitHasNativePopup()
{
  return nsContentUtils::GetBoolPref("ui.use_native_popup_windows");
}

