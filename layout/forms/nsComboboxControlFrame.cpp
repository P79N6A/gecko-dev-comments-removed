





































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
#include "nsBoxLayoutState.h"
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

NS_IMPL_ISUPPORTS1(nsComboButtonListener, nsIDOMMouseListener)


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
  : nsAreaFrame(aContext),
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



NS_IMETHODIMP
nsComboboxControlFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(0 != aInstancePtr, "null ptr");
  if (NULL == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }

  if (aIID.Equals(NS_GET_IID(nsIComboboxControlFrame))) {
    *aInstancePtr = (void*)(nsIComboboxControlFrame*)this;
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIFormControlFrame))) {
    *aInstancePtr = (void*)(nsIFormControlFrame*)this;
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIAnonymousContentCreator))) {                                         
    *aInstancePtr = (void*)(nsIAnonymousContentCreator*)this;
    return NS_OK;   
  } else if (aIID.Equals(NS_GET_IID(nsISelectControlFrame))) {
    *aInstancePtr = (void *)(nsISelectControlFrame*)this;
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIStatefulFrame))) {
    *aInstancePtr = (void*)(nsIStatefulFrame*)this;
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIRollupListener))) {
    *aInstancePtr = (void*)(nsIRollupListener*)this;
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIScrollableViewProvider))) {
    *aInstancePtr = (void*)(nsIScrollableViewProvider*)this;
    return NS_OK;
  } 
  
  return nsAreaFrame::QueryInterface(aIID, aInstancePtr);
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

  
  
  
  Invalidate(nsRect(0,0,mRect.width,mRect.height), PR_TRUE);

  
  
  
  
  nsIViewManager* vm = PresContext()->GetViewManager();
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
nsComboboxControlFrame::ShowList(nsPresContext* aPresContext, PRBool aShowList)
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

  
  shell->GetDocument()->FlushPendingNotifications(Flush_OnlyReflow);
  if (!weakFrame.IsAlive()) {
    NS_ERROR("Flush_OnlyReflow destroyed the frame");
    return PR_FALSE;
  }

  nsIFrame* listFrame;
  CallQueryInterface(mListControlFrame, &listFrame);
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
  
  
  
  PRInt32 flags = NS_FRAME_NO_MOVE_VIEW | NS_FRAME_NO_VISIBILITY | NS_FRAME_NO_SIZE_VIEW;
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
  nsPresContext* presContext = PresContext();

  nsSize dropdownSize = mDropdownFrame->GetSize();

  nscoord screenHeightInPixels = 0;
  if (NS_SUCCEEDED(nsFormControlFrame::GetScreenHeight(presContext, screenHeightInPixels))) {
    
    nscoord absoluteDropDownHeight = presContext->AppUnitsToDevPixels(dropdownSize.height);
    
    if (GetScreenRect().YMost() + absoluteDropDownHeight > screenHeightInPixels) {
      
      dropdownYOffset = - (dropdownSize.height);
    }
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
nsComboboxControlFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetPrefWidth(aRenderingContext);

  return result;
}

nscoord
nsComboboxControlFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  if (NS_LIKELY(mDropdownFrame != nsnull)) {
    result = mDropdownFrame->GetPrefWidth(aRenderingContext);
  } else {
    result = 0;
  }

  
  
  
  if (NS_LIKELY(mListControlFrame && mDisplayFrame)) {
    nsIScrollableFrame* scrollable;
    CallQueryInterface(mListControlFrame, &scrollable);
    NS_ASSERTION(scrollable, "List must be a scrollable frame");
    nsBoxLayoutState bls(PresContext(), aRenderingContext);
    nscoord displayResult =
      scrollable->GetDesiredScrollbarSizes(&bls).LeftRight() +
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, mDisplayFrame,
                                           nsLayoutUtils::PREF_WIDTH);

    result = PR_MAX(result, displayResult);
  }

  return result;
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
    nsIScrollableFrame* scrollable;
    CallQueryInterface(mListControlFrame, &scrollable);
    NS_ASSERTION(scrollable, "List must be a scrollable frame");
    nsBoxLayoutState bls(PresContext(), aReflowState.rendContext);
    buttonWidth = scrollable->GetDesiredScrollbarSizes(&bls).LeftRight();
    if (buttonWidth > aReflowState.ComputedWidth()) {
      buttonWidth = 0;
    }
  }

  mDisplayWidth = aReflowState.ComputedWidth() - buttonWidth;

  nsresult rv = nsAreaFrame::Reflow(aPresContext, aDesiredSize, aReflowState,
                                    aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsRect buttonRect = mButtonFrame->GetRect();
  
  
  if (aReflowState.mComputedHeight == NS_INTRINSICSIZE) {
    
    
    nsRect displayRect = mDisplayFrame->GetRect();
    buttonRect.height = displayRect.height;
    buttonRect.y = displayRect.y;
  }
#ifdef DEBUG
  else {
    nscoord buttonHeight = mButtonFrame->GetSize().height;
    nscoord displayHeight = mDisplayFrame->GetSize().height;

    
    
    NS_ASSERTION(buttonHeight == displayHeight ||
                 (aReflowState.mComputedHeight < buttonHeight &&
                  buttonHeight ==
                    mButtonFrame->GetUsedBorderAndPadding().TopBottom()) ||
                 (aReflowState.mComputedHeight < displayHeight &&
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
    ShowList(PresContext(), aDoDropDown); 
  } else if (mDroppedDown && !aDoDropDown) {
    ShowList(PresContext(), aDoDropDown); 
  }
}

void
nsComboboxControlFrame::SetDropDown(nsIFrame* aDropDownFrame)
{
  mDropdownFrame = aDropDownFrame;
 
  CallQueryInterface(mDropdownFrame, &mListControlFrame);
}

nsIFrame*
nsComboboxControlFrame::GetDropDown() 
{
  return mDropdownFrame;
}



NS_IMETHODIMP
nsComboboxControlFrame::RedisplaySelectedText()
{
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

    nsRefPtr<RedisplayTextEvent> event = new RedisplayTextEvent(this);
    rv = NS_DispatchToCurrentThread(event);
    if (NS_SUCCEEDED(rv))
      mRedisplayTextEvent = event;
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
  nsISelectControlFrame* listFrame = nsnull;
  nsresult rv = NS_ERROR_FAILURE;
  if (mDropdownFrame != nsnull) {
    rv = CallQueryInterface(mDropdownFrame, &listFrame);
    if (listFrame) {
      rv = listFrame->DoneAddingChildren(aIsDone);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsComboboxControlFrame::AddOption(nsPresContext* aPresContext, PRInt32 aIndex)
{
  if (aIndex <= mDisplayedIndex) {
    ++mDisplayedIndex;
  }

  nsListControlFrame* lcf = NS_STATIC_CAST(nsListControlFrame*, mDropdownFrame);
  return lcf->AddOption(aPresContext, aIndex);
}
  

NS_IMETHODIMP
nsComboboxControlFrame::RemoveOption(nsPresContext* aPresContext, PRInt32 aIndex)
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

  nsListControlFrame* lcf = NS_STATIC_CAST(nsListControlFrame*, mDropdownFrame);
  return lcf->RemoveOption(aPresContext, aIndex);
}

NS_IMETHODIMP
nsComboboxControlFrame::GetOptionSelected(PRInt32 aIndex, PRBool* aValue)
{
  nsISelectControlFrame* listFrame = nsnull;
  NS_ASSERTION(mDropdownFrame, "No dropdown frame!");

  CallQueryInterface(mDropdownFrame, &listFrame);
  NS_ASSERTION(listFrame, "No list frame!");

  return listFrame->GetOptionSelected(aIndex, aValue);
}

NS_IMETHODIMP
nsComboboxControlFrame::OnSetSelectedIndex(PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  nsISelectControlFrame* listFrame = nsnull;
  NS_ASSERTION(mDropdownFrame, "No dropdown frame!");

  CallQueryInterface(mDropdownFrame, &listFrame);
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
    return nsAreaFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
    
  return NS_OK;
}


nsresult
nsComboboxControlFrame::SetFormProperty(nsIAtom* aName, const nsAString& aValue)
{
  nsIFormControlFrame* fcFrame = nsnull;
  nsresult result = CallQueryInterface(mDropdownFrame, &fcFrame);
  if (NS_FAILED(result)) {
    return result;
  }
  if (fcFrame) {
    return fcFrame->SetFormProperty(aName, aValue);
  }
  return NS_OK;
}

nsresult 
nsComboboxControlFrame::GetFormProperty(nsIAtom* aName, nsAString& aValue) const
{
  nsIFormControlFrame* fcFrame = nsnull;
  nsresult result = CallQueryInterface(mDropdownFrame, &fcFrame);
  if(NS_FAILED(result)) {
    return result;
  }
  if (fcFrame) {
    return fcFrame->GetFormProperty(aName, aValue);
  }
  return NS_OK;
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
  nimgr->GetNodeInfo(nsGkAtoms::input, nsnull, kNameSpaceID_None,
                     getter_AddRefs(nodeInfo));

  
  NS_NewHTMLElement(getter_AddRefs(mButtonContent), nodeInfo);
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

protected:
  nsComboboxControlFrame* mComboBox;
};

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
  if (state.mComputedHeight == NS_INTRINSICSIZE) {
    
    
    
    state.mComputedHeight = mComboBox->mListControlFrame->GetHeightOfARow();
  }
  nscoord computedWidth = mComboBox->mDisplayWidth -
    state.mComputedBorderPadding.LeftRight(); 
  if (computedWidth < 0) {
    computedWidth = 0;
  }
  state.SetComputedWidth(computedWidth);

  return nsBlockFrame::Reflow(aPresContext, aDesiredSize, state, aStatus);
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
  textStyleContext = styleSet->ResolveStyleForNonElement(styleContext);
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

  mDisplayFrame->SetInitialChildList(nsnull, mTextFrame);
  return mDisplayFrame;
}

void
nsComboboxControlFrame::Destroy()
{
  
  mRedisplayTextEvent.Revoke();

  nsFormControlFrame::RegUnRegAccessKey(NS_STATIC_CAST(nsIFrame*, this), PR_FALSE);

  if (mDroppedDown) {
    
    nsIFrame * listFrame;
    if (NS_OK == mListControlFrame->QueryInterface(NS_GET_IID(nsIFrame), (void **)&listFrame)) {
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
  nsAreaFrame::Destroy();
}


nsIFrame*
nsComboboxControlFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (nsGkAtoms::popupList == aListName) {
    return mPopupFrames.FirstChild();
  }
  return nsAreaFrame::GetFirstChild(aListName);
}

NS_IMETHODIMP
nsComboboxControlFrame::SetInitialChildList(nsIAtom*        aListName,
                                            nsIFrame*       aChildList)
{
  nsresult rv = NS_OK;
  if (nsGkAtoms::popupList == aListName) {
    mPopupFrames.SetFrames(aChildList);
  } else {
    rv = nsAreaFrame::SetInitialChildList(aListName, aChildList);

    for (nsIFrame * child = aChildList; child;
         child = child->GetNextSibling()) {
      nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(child->GetContent());
      if (formControl && formControl->GetType() == NS_FORM_INPUT_BUTTON) {
        mButtonFrame = child;
        break;
      }
    }
    NS_ASSERTION(mButtonFrame, "missing button frame in initial child list");
  }
  return rv;
}

nsIAtom*
nsComboboxControlFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
   
   
   
   
  if (aIndex <= NS_BLOCK_FRAME_ABSOLUTE_LIST_INDEX) {
    return nsAreaFrame::GetAdditionalChildListName(aIndex);
  }
  
  if (NS_COMBO_FRAME_POPUP_LIST_INDEX == aIndex) {
    return nsGkAtoms::popupList;
  }
  return nsnull;
}


  

NS_IMETHODIMP 
nsComboboxControlFrame::Rollup()
{
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
  if (ShowList(PresContext(), PR_FALSE))
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
  NS_STATIC_CAST(nsComboboxControlFrame*, mFrame)
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
    
    
    nsresult rv = nsAreaFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
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
                                        nsPoint aPt) {
  aRenderingContext.PushState();
  nsRect clipRect = mDisplayFrame->GetRect() + aPt;
  aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);

  
  
  

  
  
  
  if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled) &&
      mFocused == this) {
    aRenderingContext.SetLineStyle(nsLineStyle_kDotted);
    aRenderingContext.SetColor(GetStyleColor()->mColor);
  } else {
    aRenderingContext.SetColor(GetStyleBackground()->mBackgroundColor);
    aRenderingContext.SetLineStyle(nsLineStyle_kSolid);
  }
  
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

  nsIScrollableFrame* scrollable = nsnull;
  nsresult rv = CallQueryInterface(mDropdownFrame, &scrollable);
  if (NS_FAILED(rv))
    return nsnull;

  return scrollable->GetScrollableView();
}





NS_IMETHODIMP
nsComboboxControlFrame::OnOptionSelected(nsPresContext* aPresContext,
                                         PRInt32 aIndex,
                                         PRBool aSelected)
{
  if (mDroppedDown) {
    nsCOMPtr<nsISelectControlFrame> selectFrame
                                     = do_QueryInterface(mListControlFrame);
    if (selectFrame) {
      selectFrame->OnOptionSelected(aPresContext, aIndex, aSelected);
    }
  } else {
    if (aSelected) {
      RedisplayText(aIndex);
    } else {
      RedisplaySelectedText();
      FireValueChangeEvent(); 
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

  nsIStatefulFrame* stateful;
  CallQueryInterface(mListControlFrame, &stateful);
  return stateful->SaveState(aStateID, aState);
}

NS_IMETHODIMP
nsComboboxControlFrame::RestoreState(nsPresState* aState)
{
  if (!mListControlFrame)
    return NS_ERROR_FAILURE;

  nsIStatefulFrame* stateful;
  nsresult rv = CallQueryInterface(mListControlFrame, &stateful);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Must implement nsIStatefulFrame");
  rv = stateful->RestoreState(aState);
  return rv;
}









PRBool
nsComboboxControlFrame::ToolkitHasNativePopup()
{
#ifdef XP_MACOSX
  return nsContentUtils::GetBoolPref("ui.use_native_popup_windows");
#else
  return PR_FALSE;
#endif
}

