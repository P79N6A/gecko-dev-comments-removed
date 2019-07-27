



#include "nsSelectsAreaFrame.h"
#include "nsIContent.h"
#include "nsListControlFrame.h"
#include "nsDisplayList.h"

nsContainerFrame*
NS_NewSelectsAreaFrame(nsIPresShell* aShell, nsStyleContext* aContext, nsFrameState aFlags)
{
  nsSelectsAreaFrame* it = new (aShell) nsSelectsAreaFrame(aContext);

  
  
  it->SetFlags(aFlags | NS_BLOCK_FLOAT_MGR);

  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsSelectsAreaFrame)







class nsDisplayOptionEventGrabber : public nsDisplayWrapList {
public:
  nsDisplayOptionEventGrabber(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame, nsDisplayItem* aItem)
    : nsDisplayWrapList(aBuilder, aFrame, aItem) {}
  nsDisplayOptionEventGrabber(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aBuilder, aFrame, aList) {}
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE {
    return false;
  }
  NS_DISPLAY_DECL_NAME("OptionEventGrabber", TYPE_OPTION_EVENT_GRABBER)
};

void nsDisplayOptionEventGrabber::HitTest(nsDisplayListBuilder* aBuilder,
    const nsRect& aRect, HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsTArray<nsIFrame*> outFrames;
  mList.HitTest(aBuilder, aRect, aState, &outFrames);

  for (uint32_t i = 0; i < outFrames.Length(); i++) {
    nsIFrame* selectedFrame = outFrames.ElementAt(i);
    while (selectedFrame &&
           !(selectedFrame->GetContent() &&
             selectedFrame->GetContent()->IsHTMLElement(nsGkAtoms::option))) {
      selectedFrame = selectedFrame->GetParent();
    }
    if (selectedFrame) {
      aOutFrames->AppendElement(selectedFrame);
    } else {
      
      aOutFrames->AppendElement(outFrames.ElementAt(i));
    }
  }
}

class nsOptionEventGrabberWrapper : public nsDisplayWrapper
{
public:
  nsOptionEventGrabberWrapper() {}
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) {
    return new (aBuilder) nsDisplayOptionEventGrabber(aBuilder, aFrame, aList);
  }
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) {
    return new (aBuilder) nsDisplayOptionEventGrabber(aBuilder, aItem->Frame(), aItem);
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
  return nullptr;
}

class nsDisplayListFocus : public nsDisplayItem {
public:
  nsDisplayListFocus(nsDisplayListBuilder* aBuilder,
                     nsSelectsAreaFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayListFocus);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayListFocus() {
    MOZ_COUNT_DTOR(nsDisplayListFocus);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE {
    *aSnap = false;
    
    
    nsListControlFrame* listFrame = GetEnclosingListFrame(Frame());
    return listFrame->GetVisualOverflowRectRelativeToSelf() +
           listFrame->GetOffsetToCrossDoc(ReferenceFrame());
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE {
    nsListControlFrame* listFrame = GetEnclosingListFrame(Frame());
    
    listFrame->PaintFocus(*aCtx, aBuilder->ToReferenceFrame(listFrame));
  }
  NS_DISPLAY_DECL_NAME("ListFocus", TYPE_LIST_FOCUS)
};

void
nsSelectsAreaFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  if (!aBuilder->IsForEventDelivery()) {
    BuildDisplayListInternal(aBuilder, aDirtyRect, aLists);
    return;
  }

  nsDisplayListCollection set;
  BuildDisplayListInternal(aBuilder, aDirtyRect, set);
  
  nsOptionEventGrabberWrapper wrapper;
  wrapper.WrapLists(aBuilder, this, set, aLists);
}

void
nsSelectsAreaFrame::BuildDisplayListInternal(nsDisplayListBuilder*   aBuilder,
                                             const nsRect&           aDirtyRect,
                                             const nsDisplayListSet& aLists)
{
  nsBlockFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);

  nsListControlFrame* listFrame = GetEnclosingListFrame(this);
  if (listFrame && listFrame->IsFocused()) {
    
    
    
    aLists.Outlines()->AppendNewToTop(new (aBuilder)
      nsDisplayListFocus(aBuilder, this));
  }
}

void
nsSelectsAreaFrame::Reflow(nsPresContext*           aPresContext, 
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState, 
                           nsReflowStatus&          aStatus)
{
  nsListControlFrame* list = GetEnclosingListFrame(this);
  NS_ASSERTION(list,
               "Must have an nsListControlFrame!  Frame constructor is "
               "broken");
  
  bool isInDropdownMode = list->IsInDropDownMode();
  
  
  
  nscoord oldHeight;
  if (isInDropdownMode) {
    
    
    if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
      oldHeight = GetSize().height;
    } else {
      oldHeight = NS_UNCONSTRAINEDSIZE;
    }
  }
  
  nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
  if (list->MightNeedSecondPass()) {
    nscoord newHeightOfARow = list->CalcHeightOfARow();
    
    
    
    if (newHeightOfARow != mHeightOfARow ||
        (isInDropdownMode && (oldHeight != aDesiredSize.Height() ||
                              oldHeight != GetSize().height))) {
      mHeightOfARow = newHeightOfARow;
      list->SetSuppressScrollbarUpdate(true);
    }
  }
}
