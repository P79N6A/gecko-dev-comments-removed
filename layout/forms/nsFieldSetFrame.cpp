




#include "nsFieldSetFrame.h"

#include "mozilla/gfx/2D.h"
#include "nsCSSAnonBoxes.h"
#include "nsLayoutUtils.h"
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
using namespace mozilla::gfx;
using namespace mozilla::image;
using namespace mozilla::layout;

nsContainerFrame*
NS_NewFieldSetFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFieldSetFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFieldSetFrame)

nsFieldSetFrame::nsFieldSetFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
  , mLegendRect(GetWritingMode())
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
  WritingMode wm = GetWritingMode();
  css::Side legendSide = wm.PhysicalSide(eLogicalSideBStart);
  nscoord legendBorder = StyleBorder()->GetComputedBorderWidth(legendSide);
  LogicalRect r(wm, LogicalPoint(wm, 0, 0), GetLogicalSize(wm));
  nscoord containerWidth = r.Width(wm);
  if (legendBorder < mLegendRect.BSize(wm)) {
    nscoord off = (mLegendRect.BSize(wm) - legendBorder) / 2;
    r.BStart(wm) += off;
    r.BSize(wm) -= off;
  }
  return r.GetPhysicalRect(wm, containerWidth);
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
  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;
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
  DrawResult result = static_cast<nsFieldSetFrame*>(mFrame)->
    PaintBorderBackground(*aCtx, ToReferenceFrame(),
                          mVisibleRect, aBuilder->GetBackgroundPaintFlags());

  nsDisplayItemGenericImageGeometry::UpdateDrawResult(this, result);
}

nsDisplayItemGeometry*
nsDisplayFieldSetBorderBackground::AllocateGeometry(nsDisplayListBuilder* aBuilder)
{
  return new nsDisplayItemGenericImageGeometry(this, aBuilder);
}

void
nsDisplayFieldSetBorderBackground::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                                             const nsDisplayItemGeometry* aGeometry,
                                                             nsRegion *aInvalidRegion)
{
  auto geometry =
    static_cast<const nsDisplayItemGenericImageGeometry*>(aGeometry);

  if (aBuilder->ShouldSyncDecodeImages() &&
      geometry->ShouldInvalidateToSyncDecodeImages()) {
    bool snap;
    aInvalidRegion->Or(*aInvalidRegion, GetBounds(aBuilder, &snap));
  }

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

DrawResult
nsFieldSetFrame::PaintBorderBackground(nsRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect, uint32_t aBGFlags)
{
  
  
  
  
  WritingMode wm = GetWritingMode();
  nsRect rect = VisualBorderRectRelativeToSelf();
  nscoord off = wm.IsVertical() ? rect.x : rect.y;
  rect += aPt;
  nsPresContext* presContext = PresContext();

  DrawResult result =
    nsCSSRendering::PaintBackground(presContext, aRenderingContext, this,
                                    aDirtyRect, rect, aBGFlags);

  nsCSSRendering::PaintBoxShadowInner(presContext, aRenderingContext,
                                      this, rect, aDirtyRect);

  if (nsIFrame* legend = GetLegend()) {
    css::Side legendSide = wm.PhysicalSide(eLogicalSideBStart);
    nscoord legendBorderWidth =
      StyleBorder()->GetComputedBorderWidth(legendSide);

    
    
    LogicalRect legendRect(wm, legend->GetRect() + aPt, rect.width);

    
    
    LogicalRect clipRect = LogicalRect(wm, rect, rect.width);
    DrawTarget* drawTarget = aRenderingContext.GetDrawTarget();
    gfxContext* gfx = aRenderingContext.ThebesContext();
    int32_t appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();

    
    clipRect.ISize(wm) = legendRect.IStart(wm) - clipRect.IStart(wm);
    clipRect.BSize(wm) = legendBorderWidth;

    gfx->Save();
    gfx->Clip(NSRectToSnappedRect(clipRect.GetPhysicalRect(wm, rect.width),
                                  appUnitsPerDevPixel, *drawTarget));
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext);
    gfx->Restore();

    
    clipRect = LogicalRect(wm, rect, rect.width);
    clipRect.ISize(wm) = clipRect.IEnd(wm) - legendRect.IEnd(wm);
    clipRect.IStart(wm) = legendRect.IEnd(wm);
    clipRect.BSize(wm) = legendBorderWidth;

    gfx->Save();
    gfx->Clip(NSRectToSnappedRect(clipRect.GetPhysicalRect(wm, rect.width),
                                  appUnitsPerDevPixel, *drawTarget));
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext);
    gfx->Restore();

    
    clipRect = LogicalRect(wm, rect, rect.width);
    clipRect.BStart(wm) += legendBorderWidth;
    clipRect.BSize(wm) =
      GetLogicalRect(rect.width).BSize(wm) - (off + legendBorderWidth);

    gfx->Save();
    gfx->Clip(NSRectToSnappedRect(clipRect.GetPhysicalRect(wm, rect.width),
                                  appUnitsPerDevPixel, *drawTarget));
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect, rect, mStyleContext);
    gfx->Restore();
  } else {
    nsCSSRendering::PaintBorder(presContext, aRenderingContext, this,
                                aDirtyRect,
                                nsRect(aPt, mRect.Size()),
                                mStyleContext);
  }

  return result;
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
                             ComputeSizeFlags aFlags)
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

  NS_PRECONDITION(aReflowState.ComputedISize() != NS_INTRINSICSIZE,
                  "Should have a precomputed inline-size!");

  
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

  
  
  
  
  WritingMode wm = GetWritingMode();
  WritingMode innerWM = inner ? inner->GetWritingMode() : wm;
  WritingMode legendWM = legend ? legend->GetWritingMode() : wm;
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

  
  LogicalMargin border = aReflowState.ComputedLogicalBorderPadding() -
                         aReflowState.ComputedLogicalPadding();

  
  
  LogicalMargin legendMargin(wm);
  
  Maybe<nsHTMLReflowState> legendReflowState;
  if (legend) {
    legendReflowState.emplace(aPresContext, aReflowState, legend,
                                legendAvailSize);
  }
  if (reflowLegend) {
    nsHTMLReflowMetrics legendDesiredSize(aReflowState);

    ReflowChild(legend, aPresContext, legendDesiredSize, *legendReflowState,
                wm, LogicalPoint(wm), 0, NS_FRAME_NO_MOVE_FRAME, aStatus);
#ifdef NOISY_REFLOW
    printf("  returned (%d, %d)\n",
           legendDesiredSize.Width(), legendDesiredSize.Height());
#endif
    
    legendMargin = legend->GetLogicalUsedMargin(wm);
    mLegendRect =
      LogicalRect(wm, 0, 0,
                  legendDesiredSize.ISize(wm) + legendMargin.IStartEnd(wm),
                  legendDesiredSize.BSize(wm) + legendMargin.BStartEnd(wm));
    nscoord oldSpace = mLegendSpace;
    mLegendSpace = 0;
    if (mLegendRect.BSize(wm) > border.BStart(wm)) {
      
      mLegendSpace = mLegendRect.BSize(wm) - border.BStart(wm);
    } else {
      mLegendRect.BStart(wm) =
        (border.BStart(wm) - mLegendRect.BSize(wm)) / 2;
    }

    
    
    if (mLegendSpace != oldSpace && inner) {
      reflowInner = true;
    }

    
    FinishReflowChild(legend, aPresContext, legendDesiredSize,
                      legendReflowState.ptr(), wm, LogicalPoint(wm), 0,
                      NS_FRAME_NO_MOVE_FRAME);
  } else if (!legend) {
    mLegendRect.SetEmpty();
    mLegendSpace = 0;
  } else {
    
    
    legendMargin = legend->GetLogicalUsedMargin(wm);
  }

  nscoord containerWidth = (wm.IsVertical() ? mLegendSpace : 0) +
                            border.LeftRight(wm);
  
  if (reflowInner) {
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, inner,
                                     innerAvailSize, -1, -1,
                                     nsHTMLReflowState::CALLER_WILL_INIT);
    
    kidReflowState.Init(aPresContext, -1, -1, nullptr,
                        &aReflowState.ComputedPhysicalPadding());
    
    
    
    if (aReflowState.ComputedBSize() != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.SetComputedBSize(
         std::max(0, aReflowState.ComputedBSize() - mLegendSpace));
    }

    if (aReflowState.ComputedMinBSize() > 0) {
      kidReflowState.ComputedMinBSize() =
        std::max(0, aReflowState.ComputedMinBSize() - mLegendSpace);
    }

    if (aReflowState.ComputedMaxBSize() != NS_UNCONSTRAINEDSIZE) {
      kidReflowState.ComputedMaxBSize() =
        std::max(0, aReflowState.ComputedMaxBSize() - mLegendSpace);
    }

    nsHTMLReflowMetrics kidDesiredSize(kidReflowState,
                                       aDesiredSize.mFlags);
    
    NS_ASSERTION(kidReflowState.ComputedPhysicalMargin() == nsMargin(0,0,0,0),
                 "Margins on anonymous fieldset child not supported!");
    LogicalPoint pt(wm, border.IStart(wm), border.BStart(wm) + mLegendSpace);

    ReflowChild(inner, aPresContext, kidDesiredSize, kidReflowState,
                wm, pt, containerWidth, 0, aStatus);

    
    FinishReflowChild(inner, aPresContext, kidDesiredSize,
                      &kidReflowState, wm, pt,
                      containerWidth + kidDesiredSize.Width(), 0);
    NS_FRAME_TRACE_REFLOW_OUT("FieldSet::Reflow", aStatus);
  }

  if (inner) {
    containerWidth += inner->GetSize().width;
  }

  LogicalRect contentRect(wm);
  if (inner) {
    
    
    
    contentRect = inner->GetLogicalRect(wm, containerWidth);
  }

  
  LogicalSize availSize = aReflowState.ComputedSizeWithPadding(wm);
  if (availSize.ISize(wm) > contentRect.ISize(wm)) {
    contentRect.ISize(wm) = innerAvailSize.ISize(wm);
  }

  if (legend) {
    
    
    LogicalRect innerContentRect = contentRect;
    innerContentRect.Deflate(wm, aReflowState.ComputedLogicalPadding());
    
    
    if (innerContentRect.ISize(wm) > mLegendRect.ISize(wm)) {
      int32_t align = static_cast<nsLegendFrame*>
        (legend->GetContentInsertionFrame())->GetAlign();
      if (!wm.IsBidiLTR()) {
        if (align == NS_STYLE_TEXT_ALIGN_LEFT ||
            align == NS_STYLE_TEXT_ALIGN_MOZ_LEFT) {
          align = NS_STYLE_TEXT_ALIGN_END;
        } else if (align == NS_STYLE_TEXT_ALIGN_RIGHT ||
                   align == NS_STYLE_TEXT_ALIGN_MOZ_RIGHT) {
          align = NS_STYLE_TEXT_ALIGN_DEFAULT;
        }
      }
      switch (align) {
        case NS_STYLE_TEXT_ALIGN_END:
          mLegendRect.IStart(wm) =
            innerContentRect.IEnd(wm) - mLegendRect.ISize(wm);
          break;
        case NS_STYLE_TEXT_ALIGN_CENTER:
        case NS_STYLE_TEXT_ALIGN_MOZ_CENTER:
          
          mLegendRect.IStart(wm) = innerContentRect.IStart(wm) +
            (innerContentRect.ISize(wm) - mLegendRect.ISize(wm)) / 2;
          break;
        default:
          mLegendRect.IStart(wm) = innerContentRect.IStart(wm);
          break;
      }
    } else {
      
      mLegendRect.IStart(wm) = innerContentRect.IStart(wm);
      innerContentRect.ISize(wm) = mLegendRect.ISize(wm);
      contentRect.ISize(wm) = mLegendRect.ISize(wm) +
        aReflowState.ComputedLogicalPadding().IStartEnd(wm);
    }

    
    LogicalRect actualLegendRect = mLegendRect;
    actualLegendRect.Deflate(wm, legendMargin);
    LogicalPoint actualLegendPos(actualLegendRect.Origin(wm));
    legendReflowState->ApplyRelativePositioning(&actualLegendPos, containerWidth);
    legend->SetPosition(wm, actualLegendPos, containerWidth);
    nsContainerFrame::PositionFrameView(legend);
    nsContainerFrame::PositionChildViews(legend);
  }

  
  LogicalSize finalSize(wm, contentRect.ISize(wm) + border.IStartEnd(wm),
                        mLegendSpace + border.BStartEnd(wm) +
                        (inner ? inner->BSize(wm) : 0));
  aDesiredSize.SetSize(wm, finalSize);
  aDesiredSize.SetOverflowAreasToDesiredBounds();

  if (legend) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, legend);
  }
  if (inner) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, inner);
  }

  
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
