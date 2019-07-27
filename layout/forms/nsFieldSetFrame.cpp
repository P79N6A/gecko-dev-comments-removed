




#include "nsFieldSetFrame.h"

#include "nsCSSAnonBoxes.h"
#include "nsLegendFrame.h"
#include "nsCSSRendering.h"
#include <algorithm>
#include "nsIFrame.h"
#include "nsPresContext.h"
#include "RestyleManager.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsDisplayList.h"
#include "nsRenderingContext.h"
#include "nsIScrollableFrame.h"
#include "mozilla/Likely.h"
#include "mozilla/Maybe.h"

using namespace mozilla;
using namespace mozilla::layout;

nsContainerFrame*
NS_NewFieldSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFieldSetFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFieldSetFrame)

nsFieldSetFrame::nsFieldSetFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
{
  mLegendSpace  = 0;
}

nsIAtom*
nsFieldSetFrame::GetType() const
{
  return nsGkAtoms::fieldSetFrame;
}

nsRect
nsFieldSetFrame::VisualBorderRectRelativeToSelf() const
{
  nscoord topBorder = StyleBorder()->GetComputedBorderWidth(NS_SIDE_TOP);
  nsRect r(nsPoint(0,0), GetSize());
  if (topBorder < mLegendRect.height) {
    nscoord yoff = (mLegendRect.height - topBorder) / 2;
    r.y += yoff;
    r.height -= yoff;
  }
  return r;
}

nsIFrame*
nsFieldSetFrame::GetInner() const
{
  nsIFrame* last = mFrames.LastChild();
  if (last &&
      last->StyleContext()->GetPseudo() == nsCSSAnonBoxes::fieldsetContent) {
    return last;
  }
  MOZ_ASSERT(mFrames.LastChild() == mFrames.FirstChild());
  return nullptr;
}

nsIFrame*
nsFieldSetFrame::GetLegend() const
{
  if (mFrames.FirstChild() == GetInner()) {
    MOZ_ASSERT(mFrames.LastChild() == mFrames.FirstChild());
    return nullptr;
  }
  MOZ_ASSERT(mFrames.FirstChild() &&
             mFrames.FirstChild()->GetContentInsertionFrame()->GetType() ==
               nsGkAtoms::legendFrame);
  return mFrames.FirstChild();
}

class nsDisplayFieldSetBorderBackground : public nsDisplayItem {
public:
  nsDisplayFieldSetBorderBackground(nsDisplayListBuilder* aBuilder,
                                    nsFieldSetFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayFieldSetBorderBackground);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayFieldSetBorderBackground() {
    MOZ_COUNT_DTOR(nsDisplayFieldSetBorderBackground);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion *aInvalidRegion) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("FieldSetBorderBackground", TYPE_FIELDSET_BORDER_BACKGROUND)
};

void nsDisplayFieldSetBorderBackground::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                                                HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  
  
  
  aOutFrames->AppendElement(mFrame);
}

void
nsDisplayFieldSetBorderBackground::Paint(nsDisplayListBuilder* aBuilder,
                                         nsRenderingContext* aCtx)
{
  static_cast<nsFieldSetFrame*>(mFrame)->
    PaintBorderBackground(*aCtx, ToReferenceFrame(),
                          mVisibleRect, aBuilder->GetBackgroundPaintFlags());
}

void
nsDisplayFieldSetBorderBackground::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                                             const nsDisplayItemGeometry* aGeometry,
                                                             nsRegion *aInvalidRegion)
{
  AddInvalidRegionForSyncDecodeBackgroundImages(aBuilder, aGeometry, aInvalidRegion);

  nsDisplayItem::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
}

void
nsFieldSetFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists) {
  
  
  
  
  if (!(GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) &&
      IsVisibleForPainting(aBuilder)) {
    if (StyleBorder()->mBoxShadow) {
      aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBoxShadowOuter(aBuilder, this));
    }

    
    
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayFieldSetBorderBackground(aBuilder, this));
  
    DisplayOutlineUnconditional(aBuilder, aLists);

    DO_GLOBAL_REFLOW_COUNT_DSP("nsFieldSetFrame");
  }

  if (GetPrevInFlow()) {
    DisplayOverflowContainers(aBuilder, aDirtyRect, aLists);
  }

  nsDisplayListCollection contentDisplayItems;
  if (nsIFrame* inner = GetInner()) {
    
    
    
    
    
    
    BuildDisplayListForChild(aBuilder, inner, aDirtyRect, contentDisplayItems);
  }
  if (nsIFrame* legend = GetLegend()) {
    
    
    nsDisplayListSet set(aLists, aLists.BlockBorderBackgrounds());
    BuildDisplayListForChild(aBuilder, legend, aDirtyRect, set);
  }
  
  
  
  
  contentDisplayItems.MoveTo(aLists);
}

void
nsFieldSetFrame::PaintBorderBackground(nsRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect, uint32_t aBGFlags)
{
  
  
  
  
  nsRect rect = VisualBorderRectRelativeToSelf();
  nscoord yoff = rect.y;
  rect += aPt;
  nsPresContext* presContext = PresContext();

  nsCSSRendering::PaintBackground(presContext, aRenderingContext, this,
                                  aDirtyRect, rect, aBGFlags);

  nsCSSRendering::PaintBoxShadowInner(presContext, aRenderingContext,
                                      this, rect, aDirtyRect);

   if (nsIFrame* legend = GetLegend()) {
     nscoord topBorder = StyleBorder()->GetComputedBorderWidth(NS_SIDE_TOP);

    
    
    nsRect legendRect = legend->GetRect() + aPt;
    
    
    

    
    nsRect clipRect(rect);
    clipRect.width = legendRect.x - rect.x;
    clipRect.height = topBorder;

    aRenderingContext.ThebesContext()->Save();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext);

    aRenderingContext.ThebesContext()->Restore();


    
    clipRect = rect;
    clipRect.x = legendRect.XMost();
    clipRect.width = rect.XMost() - legendRect.XMost();
    clipRect.height = topBorder;

    aRenderingContext.ThebesContext()->Save();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext);

    aRenderingContext.ThebesContext()->Restore();

    
    
    clipRect = rect;
    clipRect.y += topBorder;
    clipRect.height = mRect.height - (yoff + topBorder);
    
    aRenderingContext.ThebesContext()->Save();
    aRenderingContext.IntersectClip(clipRect);
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext);

    aRenderingContext.ThebesContext()->Restore();
  } else {

    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect,
                                nsRect(aPt, mRect.Size()),
                                mStyleContext);
  }
}

nscoord
nsFieldSetFrame::GetIntrinsicISize(nsRenderingContext* aRenderingContext,
                                   nsLayoutUtils::IntrinsicISizeType aType)
{
  nscoord legendWidth = 0;
  nscoord contentWidth = 0;
  if (nsIFrame* legend = GetLegend()) {
    legendWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, legend, aType);
  }

  if (nsIFrame* inner = GetInner()) {
    
    
    
    contentWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, inner, aType,
                                           nsLayoutUtils::IGNORE_PADDING);
  }

  return std::max(legendWidth, contentWidth);
}


nscoord
nsFieldSetFrame::GetMinISize(nsRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);

  result = GetIntrinsicISize(aRenderingContext, nsLayoutUtils::MIN_ISIZE);
  return result;
}

nscoord
nsFieldSetFrame::GetPrefISize(nsRenderingContext* aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);

  result = GetIntrinsicISize(aRenderingContext, nsLayoutUtils::PREF_ISIZE);
  return result;
}


LogicalSize
nsFieldSetFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                             WritingMode aWM,
                             const LogicalSize& aCBSize,
                             nscoord aAvailableISize,
                             const LogicalSize& aMargin,
                             const LogicalSize& aBorder,
                             const LogicalSize& aPadding,
                             uint32_t aFlags)
{
  LogicalSize result =
    nsContainerFrame::ComputeSize(aRenderingContext, aWM,
                                  aCBSize, aAvailableISize,
                                  aMargin, aBorder, aPadding, aFlags);

  
  
  
  if (aWM.IsVertical() != GetWritingMode().IsVertical()) {
    return result;
  }

  

  
  
  AutoMaybeDisableFontInflation an(this);

  nscoord minISize = GetMinISize(aRenderingContext);
  if (minISize > result.ISize(aWM)) {
    result.ISize(aWM) = minISize;
  }

  return result;
}

void
nsFieldSetFrame::Reflow(nsPresContext*           aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFieldSetFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_PRECONDITION(aReflowState.ComputedWidth() != NS_INTRINSICSIZE,
                  "Should have a precomputed width!");      
  
  
  aStatus = NS_FRAME_COMPLETE;

  nsOverflowAreas ocBounds;
  nsReflowStatus ocStatus = NS_FRAME_COMPLETE;
  if (GetPrevInFlow()) {
    ReflowOverflowContainerChildren(aPresContext, aReflowState, ocBounds, 0,
                                    ocStatus);
  }

  
  bool reflowInner;
  bool reflowLegend;
  nsIFrame* legend = GetLegend();
  nsIFrame* inner = GetInner();
  if (aReflowState.ShouldReflowAllKids()) {
    reflowInner = inner != nullptr;
    reflowLegend = legend != nullptr;
  } else {
    reflowInner = inner && NS_SUBTREE_DIRTY(inner);
    reflowLegend = legend && NS_SUBTREE_DIRTY(legend);
  }

  
  
  
  
  WritingMode innerWM = inner ? inner->GetWritingMode() : GetWritingMode();
  WritingMode legendWM = legend ? legend->GetWritingMode() : GetWritingMode();
  LogicalSize innerAvailSize = aReflowState.ComputedSizeWithPadding(innerWM);
  LogicalSize legendAvailSize = aReflowState.ComputedSizeWithPadding(legendWM);
  innerAvailSize.BSize(innerWM) = legendAvailSize.BSize(legendWM) =
    NS_UNCONSTRAINEDSIZE;
  NS_ASSERTION(!inner ||
      nsLayoutUtils::IntrinsicForContainer(aReflowState.rendContext,
                                           inner,
                                           nsLayoutUtils::MIN_ISIZE) <=
               innerAvailSize.ISize(innerWM),
               "Bogus availSize.ISize; should be bigger");
  NS_ASSERTION(!legend ||
      nsLayoutUtils::IntrinsicForContainer(aReflowState.rendContext,
                                           legend,
                                           nsLayoutUtils::MIN_ISIZE) <=
               legendAvailSize.ISize(legendWM),
               "Bogus availSize.ISize; should be bigger");

  
  nsMargin border = aReflowState.ComputedPhysicalBorderPadding() - aReflowState.ComputedPhysicalPadding();

  
  
  nsMargin legendMargin(0,0,0,0);
  
  Maybe<nsHTMLReflowState> legendReflowState;
  if (legend) {
    legendReflowState.emplace(aPresContext, aReflowState, legend,
                                legendAvailSize);
  }
  if (reflowLegend) {
    nsHTMLReflowMetrics legendDesiredSize(aReflowState);

    ReflowChild(legend, aPresContext, legendDesiredSize, *legendReflowState,
                0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
#ifdef NOISY_REFLOW
    printf("  returned (%d, %d)\n",
           legendDesiredSize.Width(), legendDesiredSize.Height());
#endif
    
    legendMargin = legend->GetUsedMargin();
    mLegendRect.width  = legendDesiredSize.Width() + legendMargin.left + legendMargin.right;
    mLegendRect.height = legendDesiredSize.Height() + legendMargin.top + legendMargin.bottom;
    mLegendRect.x = 0;
    mLegendRect.y = 0;

    nscoord oldSpace = mLegendSpace;
    mLegendSpace = 0;
    if (mLegendRect.height > border.top) {
      
      mLegendSpace = mLegendRect.height - border.top;
    } else {
      mLegendRect.y = (border.top - mLegendRect.height)/2;
    }

    
    
    if (mLegendSpace != oldSpace && inner) {
      reflowInner = true;
    }

    FinishReflowChild(legend, aPresContext, legendDesiredSize,
                      legendReflowState.ptr(), 0, 0, NS_FRAME_NO_MOVE_FRAME);    
  } else if (!legend) {
    mLegendRect.SetEmpty();
    mLegendSpace = 0;
  } else {
    
    
    legendMargin = legend->GetUsedMargin();
  }

  
  if (reflowInner) {
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, inner,
                                     innerAvailSize, -1, -1,
                                     nsHTMLReflowState::CALLER_WILL_INIT);
    
    kidReflowState.Init(aPresContext, -1, -1, nullptr,
                        &aReflowState.ComputedPhysicalPadding());
    
    
    
    if (aReflowState.ComputedHeight() != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.SetComputedHeight(
         std::max(0, aReflowState.ComputedHeight() - mLegendSpace));
    }

    if (aReflowState.ComputedMinHeight() > 0) {
      kidReflowState.ComputedMinHeight() =
        std::max(0, aReflowState.ComputedMinHeight() - mLegendSpace);
    }

    if (aReflowState.ComputedMaxHeight() != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.ComputedMaxHeight() =
        std::max(0, aReflowState.ComputedMaxHeight() - mLegendSpace);
    }

    nsHTMLReflowMetrics kidDesiredSize(kidReflowState,
                                       aDesiredSize.mFlags);
    
    NS_ASSERTION(kidReflowState.ComputedPhysicalMargin() == nsMargin(0,0,0,0),
                 "Margins on anonymous fieldset child not supported!");
    nsPoint pt(border.left, border.top + mLegendSpace);
    ReflowChild(inner, aPresContext, kidDesiredSize, kidReflowState,
                pt.x, pt.y, 0, aStatus);

    FinishReflowChild(inner, aPresContext, kidDesiredSize,
                      &kidReflowState, pt.x, pt.y, 0);
    NS_FRAME_TRACE_REFLOW_OUT("FieldSet::Reflow", aStatus);
  }

  LogicalRect contentRect(innerWM);
  if (inner) {
    
    
    contentRect = inner->GetLogicalRect(aReflowState.ComputedWidth());
  }

  
  if (innerAvailSize.ISize(innerWM) > contentRect.ISize(innerWM)) {
    contentRect.ISize(innerWM) = innerAvailSize.ISize(innerWM);
  }

  
  nsRect physicalContentRect =
    contentRect.GetPhysicalRect(innerWM, aReflowState.ComputedWidth());
  if (legend) {
    
    
    nsRect innerContentRect = physicalContentRect;
    innerContentRect.Deflate(aReflowState.ComputedPhysicalPadding());
    
    if (innerContentRect.width > mLegendRect.width) {
      int32_t align = static_cast<nsLegendFrame*>
        (legend->GetContentInsertionFrame())->GetAlign();

      switch (align) {
        case NS_STYLE_TEXT_ALIGN_RIGHT:
          mLegendRect.x = innerContentRect.XMost() - mLegendRect.width;
          break;
        case NS_STYLE_TEXT_ALIGN_CENTER:
          
          mLegendRect.x = innerContentRect.width / 2 - mLegendRect.width / 2 + innerContentRect.x;
          break;
        default:
          mLegendRect.x = innerContentRect.x;
          break;
      }
    } else {
      
      mLegendRect.x = innerContentRect.x;
      innerContentRect.width = mLegendRect.width;
      physicalContentRect.width = mLegendRect.width +
        aReflowState.ComputedPhysicalPadding().LeftRight();
    }

    
    nsRect actualLegendRect(mLegendRect);
    actualLegendRect.Deflate(legendMargin);
    nsPoint actualLegendPos(actualLegendRect.TopLeft());
    legendReflowState->ApplyRelativePositioning(&actualLegendPos);
    legend->SetPosition(actualLegendPos);
    nsContainerFrame::PositionFrameView(legend);
    nsContainerFrame::PositionChildViews(legend);
  }

  
  WritingMode wm = aReflowState.GetWritingMode();
  nsSize finalSize(physicalContentRect.width + border.LeftRight(),
                   mLegendSpace + border.TopBottom() +
                   (inner ? inner->GetRect().height : 0));
  aDesiredSize.SetSize(wm, LogicalSize(wm, finalSize));
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  if (legend)
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, legend);
  if (inner)
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, inner);

  
  aDesiredSize.mOverflowAreas.UnionWith(ocBounds);
  NS_MergeReflowStatusInto(&aStatus, ocStatus);

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus);

  InvalidateFrame();

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

#ifdef DEBUG
void
nsFieldSetFrame::SetInitialChildList(ChildListID    aListID,
                                     nsFrameList&   aChildList)
{
  nsContainerFrame::SetInitialChildList(kPrincipalList, aChildList);
  MOZ_ASSERT(GetInner());
}
void
nsFieldSetFrame::AppendFrames(ChildListID    aListID,
                              nsFrameList&   aFrameList)
{
  MOZ_CRASH("nsFieldSetFrame::AppendFrames not supported");
}

void
nsFieldSetFrame::InsertFrames(ChildListID    aListID,
                              nsIFrame*      aPrevFrame,
                              nsFrameList&   aFrameList)
{
  MOZ_CRASH("nsFieldSetFrame::InsertFrames not supported");
}

void
nsFieldSetFrame::RemoveFrame(ChildListID    aListID,
                             nsIFrame*      aOldFrame)
{
  MOZ_CRASH("nsFieldSetFrame::RemoveFrame not supported");
}
#endif

#ifdef ACCESSIBILITY
a11y::AccType
nsFieldSetFrame::AccessibleType()
{
  return a11y::eHTMLGroupboxType;
}
#endif

nscoord
nsFieldSetFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  nsIFrame* inner = GetInner();
  return inner->BStart(aWritingMode, GetParent()->GetSize().width) +
    inner->GetLogicalBaseline(aWritingMode);
}
