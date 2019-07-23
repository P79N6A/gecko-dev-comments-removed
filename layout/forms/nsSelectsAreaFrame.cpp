




































#include "nsSelectsAreaFrame.h"
#include "nsCOMPtr.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIContent.h"
#include "nsListControlFrame.h"
#include "nsDisplayList.h"

nsIFrame*
NS_NewSelectsAreaFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRUint32 aFlags)
{
  nsSelectsAreaFrame* it = new (aShell) nsSelectsAreaFrame(aContext);

  if (it) {
    
    
    it->SetFlags(aFlags | NS_BLOCK_FLOAT_MGR);
  }

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsSelectsAreaFrame)


PRBool 
nsSelectsAreaFrame::IsOptionElement(nsIContent* aContent)
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


PRBool 
nsSelectsAreaFrame::IsOptionElementFrame(nsIFrame *aFrame)
{
  nsIContent *content = aFrame->GetContent();
  if (content) {
    return IsOptionElement(content);
  }
  return PR_FALSE;
}






class nsDisplayOptionEventGrabber : public nsDisplayWrapList {
public:
  nsDisplayOptionEventGrabber(nsIFrame* aFrame, nsDisplayItem* aItem)
    : nsDisplayWrapList(aFrame, aItem) {}
  nsDisplayOptionEventGrabber(nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aFrame, aList) {}
  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState);
  NS_DISPLAY_DECL_NAME("OptionEventGrabber")

  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem);
};

nsIFrame* nsDisplayOptionEventGrabber::HitTest(nsDisplayListBuilder* aBuilder,
    nsPoint aPt, HitTestState* aState)
{
  nsIFrame* frame = mList.HitTest(aBuilder, aPt, aState);

  if (frame) {
    nsIFrame* selectedFrame = frame;
    while (selectedFrame &&
           !nsSelectsAreaFrame::IsOptionElementFrame(selectedFrame)) {
      selectedFrame = selectedFrame->GetParent();
    }
    if (selectedFrame) {
      return selectedFrame;
    }
    
  }

  return frame;
}

nsDisplayWrapList* nsDisplayOptionEventGrabber::WrapWithClone(
    nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
  return new (aBuilder) nsDisplayOptionEventGrabber(aItem->GetUnderlyingFrame(), aItem);
}

class nsOptionEventGrabberWrapper : public nsDisplayWrapper
{
public:
  nsOptionEventGrabberWrapper() {}
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) {
    
    
    return new (aBuilder) nsDisplayOptionEventGrabber(nsnull, aList);
  }
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) {
    return new (aBuilder) nsDisplayOptionEventGrabber(aItem->GetUnderlyingFrame(), aItem);
  }
};

static nsListControlFrame* GetEnclosingListFrame(nsIFrame* aSelectsAreaFrame)
{
  nsIFrame* frame = aSelectsAreaFrame->GetParent();
  while (frame) {
    if (frame->GetType() == nsGkAtoms::listControlFrame)
      return static_cast<nsListControlFrame*>(frame);
    frame = frame->GetParent();
  }
  return nsnull;
}

class nsDisplayListFocus : public nsDisplayItem {
public:
  nsDisplayListFocus(nsSelectsAreaFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayListFocus);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayListFocus() {
    MOZ_COUNT_DTOR(nsDisplayListFocus);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    
    
    nsListControlFrame* listFrame = GetEnclosingListFrame(GetUnderlyingFrame());
    return listFrame->GetOverflowRect() + aBuilder->ToReferenceFrame(listFrame);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx) {
    nsListControlFrame* listFrame = GetEnclosingListFrame(GetUnderlyingFrame());
    
    listFrame->PaintFocus(*aCtx, aBuilder->ToReferenceFrame(listFrame));
  }
  NS_DISPLAY_DECL_NAME("ListFocus")
};

NS_IMETHODIMP
nsSelectsAreaFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  if (!aBuilder->IsForEventDelivery())
    return BuildDisplayListInternal(aBuilder, aDirtyRect, aLists);
    
  nsDisplayListCollection set;
  nsresult rv = BuildDisplayListInternal(aBuilder, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsOptionEventGrabberWrapper wrapper;
  return wrapper.WrapLists(aBuilder, this, set, aLists);
}

nsresult
nsSelectsAreaFrame::BuildDisplayListInternal(nsDisplayListBuilder*   aBuilder,
                                             const nsRect&           aDirtyRect,
                                             const nsDisplayListSet& aLists)
{
  nsresult rv = nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  nsListControlFrame* listFrame = GetEnclosingListFrame(this);
  if (listFrame && listFrame->IsFocused()) {
    
    
    
    return aLists.Outlines()->AppendNewToTop(new (aBuilder)
      nsDisplayListFocus(this));
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
nsSelectsAreaFrame::Reflow(nsPresContext*           aPresContext, 
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState, 
                           nsReflowStatus&          aStatus)
{
  nsListControlFrame* list = GetEnclosingListFrame(this);
  NS_ASSERTION(list,
               "Must have an nsListControlFrame!  Frame constructor is "
               "broken");
  
  PRBool isInDropdownMode = list->IsInDropDownMode();
  
  
  
  nscoord oldHeight;
  if (isInDropdownMode) {
    
    
    if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
      oldHeight = GetSize().height;
    } else {
      oldHeight = NS_UNCONSTRAINEDSIZE;
    }
  }
  
  nsresult rv = nsBlockFrame::Reflow(aPresContext, aDesiredSize,
                                    aReflowState, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (list->MightNeedSecondPass()) {
    nscoord newHeightOfARow = list->CalcHeightOfARow();
    
    
    
    if (newHeightOfARow != mHeightOfARow ||
        (isInDropdownMode && (oldHeight != aDesiredSize.height ||
                              oldHeight != GetSize().height))) {
      mHeightOfARow = newHeightOfARow;
      list->SetSuppressScrollbarUpdate(PR_TRUE);
    }
  }

  return rv;
}
