











#include "nsLeafBoxFrame.h"
#include "nsBoxFrame.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsNameSpaceManager.h"
#include "nsBoxLayoutState.h"
#include "nsWidgetsCID.h"
#include "nsViewManager.h"
#include "nsContainerFrame.h"
#include "nsDisplayList.h"
#include <algorithm>

using namespace mozilla;






nsIFrame*
NS_NewLeafBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsLeafBoxFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsLeafBoxFrame)

nsLeafBoxFrame::nsLeafBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext)
    : nsLeafFrame(aContext)
{
}

#ifdef DEBUG_LAYOUT
void
nsLeafBoxFrame::GetBoxName(nsAutoString& aName)
{
   GetFrameName(aName);
}
#endif





void
nsLeafBoxFrame::Init(nsIContent*       aContent,
                     nsContainerFrame* aParent,
                     nsIFrame*         aPrevInFlow)
{
  nsLeafFrame::Init(aContent, aParent, aPrevInFlow);

  if (GetStateBits() & NS_FRAME_FONT_INFLATION_CONTAINER) {
    AddStateBits(NS_FRAME_FONT_INFLATION_FLOW_ROOT);
  }

  UpdateMouseThrough();
}

nsresult
nsLeafBoxFrame::AttributeChanged(int32_t aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 int32_t aModType)
{
  nsresult rv = nsLeafFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                              aModType);

  if (aAttribute == nsGkAtoms::mousethrough) 
    UpdateMouseThrough();

  return rv;
}

void nsLeafBoxFrame::UpdateMouseThrough()
{
  if (mContent) {
    static nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::never, &nsGkAtoms::always, nullptr};
    switch (mContent->FindAttrValueIn(kNameSpaceID_None,
                                      nsGkAtoms::mousethrough,
                                      strings, eCaseMatters)) {
      case 0: AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); break;
      case 1: AddStateBits(NS_FRAME_MOUSE_THROUGH_ALWAYS); break;
      case 2: {
          RemoveStateBits(NS_FRAME_MOUSE_THROUGH_ALWAYS);
          RemoveStateBits(NS_FRAME_MOUSE_THROUGH_NEVER);
          break;
      }
    }
  }
}

void
nsLeafBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists)
{
  
  
  
  
  
  DisplayBorderBackgroundOutline(aBuilder, aLists);

  if (!aBuilder->IsForEventDelivery() || !IsVisibleForPainting(aBuilder))
    return;

  aLists.Content()->AppendNewToTop(new (aBuilder)
    nsDisplayEventReceiver(aBuilder, this));
}

 nscoord
nsLeafBoxFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  nsBoxLayoutState state(PresContext(), aRenderingContext);
  nsSize minSize = GetMinSize(state);

  
  
  
  
  nsMargin bp;
  GetBorderAndPadding(bp);

  result = minSize.width - bp.LeftRight();

  return result;
}

 nscoord
nsLeafBoxFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  nsBoxLayoutState state(PresContext(), aRenderingContext);
  nsSize prefSize = GetPrefSize(state);

  
  
  
  
  nsMargin bp;
  GetBorderAndPadding(bp);

  result = prefSize.width - bp.LeftRight();

  return result;
}

nscoord
nsLeafBoxFrame::GetIntrinsicISize()
{
  
  return 0;
}

LogicalSize
nsLeafBoxFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                WritingMode aWM,
                                const LogicalSize& aCBSize,
                                nscoord aAvailableISize,
                                const LogicalSize& aMargin,
                                const LogicalSize& aBorder,
                                const LogicalSize& aPadding,
                                bool aShrinkWrap)
{
  
  return nsFrame::ComputeAutoSize(aRenderingContext, aWM,
                                  aCBSize, aAvailableISize,
                                  aMargin, aBorder, aPadding, aShrinkWrap);
}

void
nsLeafBoxFrame::Reflow(nsPresContext*   aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  
  
  
  

  DO_GLOBAL_REFLOW_COUNT("nsLeafBoxFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_ASSERTION(aReflowState.ComputedWidth() >=0 &&
               aReflowState.ComputedHeight() >= 0, "Computed Size < 0");

#ifdef DO_NOISY_REFLOW
  printf("\n-------------Starting LeafBoxFrame Reflow ----------------------------\n");
  printf("%p ** nsLBF::Reflow %d R: ", this, myCounter++);
  switch (aReflowState.reason) {
    case eReflowReason_Initial:
      printf("Ini");break;
    case eReflowReason_Incremental:
      printf("Inc");break;
    case eReflowReason_Resize:
      printf("Rsz");break;
    case eReflowReason_StyleChange:
      printf("Sty");break;
    case eReflowReason_Dirty:
      printf("Drt ");
      break;
    default:printf("<unknown>%d", aReflowState.reason);break;
  }
  
  printSize("AW", aReflowState.AvailableWidth());
  printSize("AH", aReflowState.AvailableHeight());
  printSize("CW", aReflowState.ComputedWidth());
  printSize("CH", aReflowState.ComputedHeight());

  printf(" *\n");

#endif

  aStatus = NS_FRAME_COMPLETE;

  
  nsBoxLayoutState state(aPresContext, aReflowState.rendContext);

  nsSize computedSize(aReflowState.ComputedWidth(),aReflowState.ComputedHeight());

  nsMargin m;
  m = aReflowState.ComputedPhysicalBorderPadding();

  

  
  if (aReflowState.ComputedHeight() == 0) {
    nsSize minSize = GetMinSize(state);
    computedSize.height = minSize.height - m.top - m.bottom;
  }

  nsSize prefSize(0,0);

  
  if (computedSize.width == NS_INTRINSICSIZE || computedSize.height == NS_INTRINSICSIZE) {
     prefSize = GetPrefSize(state);
     nsSize minSize = GetMinSize(state);
     nsSize maxSize = GetMaxSize(state);
     prefSize = BoundsCheck(minSize, prefSize, maxSize);
  }

  
  if (aReflowState.ComputedWidth() == NS_INTRINSICSIZE) {
    computedSize.width = prefSize.width;
  } else {
    computedSize.width += m.left + m.right;
  }

  if (aReflowState.ComputedHeight() == NS_INTRINSICSIZE) {
    computedSize.height = prefSize.height;
  } else {
    computedSize.height += m.top + m.bottom;
  }

  
  
  
  
  if (computedSize.width > aReflowState.ComputedMaxWidth())
    computedSize.width = aReflowState.ComputedMaxWidth();

  if (computedSize.width < aReflowState.ComputedMinWidth())
    computedSize.width = aReflowState.ComputedMinWidth();

  
  
  
  
  computedSize.height = std::max(0, computedSize.height - m.TopBottom());
  computedSize.height = NS_CSS_MINMAX(computedSize.height,
                                      aReflowState.ComputedMinHeight(),
                                      aReflowState.ComputedMaxHeight());
  computedSize.height += m.TopBottom();

  nsRect r(mRect.x, mRect.y, computedSize.width, computedSize.height);

  SetBounds(state, r);
 
  
  Layout(state);
  
  
  aDesiredSize.Width() = mRect.width;
  aDesiredSize.Height() = mRect.height;
  aDesiredSize.SetBlockStartAscent(GetBoxAscent(state));

  
  aDesiredSize.mOverflowAreas = GetOverflowAreas();

#ifdef DO_NOISY_REFLOW
  {
    printf("%p ** nsLBF(done) W:%d H:%d  ", this, aDesiredSize.Width(), aDesiredSize.Height());

    if (maxElementWidth) {
      printf("MW:%d\n", *maxElementWidth); 
    } else {
      printf("MW:?\n"); 
    }

  }
#endif
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsLeafBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("LeafBox"), aResult);
}
#endif

nsIAtom*
nsLeafBoxFrame::GetType() const
{
  return nsGkAtoms::leafBoxFrame;
}

nsresult
nsLeafBoxFrame::CharacterDataChanged(CharacterDataChangeInfo* aInfo)
{
  MarkIntrinsicISizesDirty();
  return nsLeafFrame::CharacterDataChanged(aInfo);
}

 nsSize
nsLeafBoxFrame::GetPrefSize(nsBoxLayoutState& aState)
{
    return nsBox::GetPrefSize(aState);
}

 nsSize
nsLeafBoxFrame::GetMinSize(nsBoxLayoutState& aState)
{
    return nsBox::GetMinSize(aState);
}

 nsSize
nsLeafBoxFrame::GetMaxSize(nsBoxLayoutState& aState)
{
    return nsBox::GetMaxSize(aState);
}

 nscoord
nsLeafBoxFrame::GetFlex(nsBoxLayoutState& aState)
{
    return nsBox::GetFlex(aState);
}

 nscoord
nsLeafBoxFrame::GetBoxAscent(nsBoxLayoutState& aState)
{
    return nsBox::GetBoxAscent(aState);
}

 void
nsLeafBoxFrame::MarkIntrinsicISizesDirty()
{
  
  
}

NS_IMETHODIMP
nsLeafBoxFrame::DoLayout(nsBoxLayoutState& aState)
{
    return nsBox::DoLayout(aState);
}
