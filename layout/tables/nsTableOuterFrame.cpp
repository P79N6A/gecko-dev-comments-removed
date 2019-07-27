




#include "nsTableOuterFrame.h"

#include "nsFrameManager.h"
#include "nsTableFrame.h"
#include "nsTableCellFrame.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "prinrval.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsIServiceManager.h"
#include "nsIDOMNode.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"
#include "nsIFrameInlines.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::layout;

#define NO_SIDE 100

 nscoord
nsTableOuterFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  nsIFrame* kid = mFrames.FirstChild();
  if (!kid) {
    NS_NOTREACHED("no inner table");
    return nsContainerFrame::GetLogicalBaseline(aWritingMode);
  }

  return kid->GetLogicalBaseline(aWritingMode) +
         kid->BStart(aWritingMode, mRect.width);
}

nsTableOuterFrame::nsTableOuterFrame(nsStyleContext* aContext):
  nsContainerFrame(aContext)
{
}

nsTableOuterFrame::~nsTableOuterFrame()
{
}

NS_QUERYFRAME_HEAD(nsTableOuterFrame)
  NS_QUERYFRAME_ENTRY(nsTableOuterFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef ACCESSIBILITY
a11y::AccType
nsTableOuterFrame::AccessibleType()
{
  return a11y::eHTMLTableType;
}
#endif

void
nsTableOuterFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  DestroyAbsoluteFrames(aDestructRoot);
  mCaptionFrames.DestroyFramesFrom(aDestructRoot);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

const nsFrameList&
nsTableOuterFrame::GetChildList(ChildListID aListID) const
{
  if (aListID == kCaptionList) {
    return mCaptionFrames;
  }

  return nsContainerFrame::GetChildList(aListID);
}

void
nsTableOuterFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  nsContainerFrame::GetChildLists(aLists);
  mCaptionFrames.AppendIfNonempty(aLists, kCaptionList);
}

void 
nsTableOuterFrame::SetInitialChildList(ChildListID     aListID,
                                       nsFrameList&    aChildList)
{
  MOZ_ASSERT(kCaptionList == aListID || aListID == kPrincipalList,
             "unexpected child list");
  MOZ_ASSERT(GetChildList(aListID).IsEmpty(),
             "already have child frames in SetInitialChildList");

  if (kCaptionList == aListID) {
    
    mCaptionFrames.SetFrames(aChildList);
  } else {
    MOZ_ASSERT(aChildList.FirstChild() &&
               aChildList.FirstChild() == aChildList.LastChild() &&
               nsGkAtoms::tableFrame == aChildList.FirstChild()->GetType(),
               "expected a single table frame");
    mFrames.SetFrames(aChildList);
  }
}

void
nsTableOuterFrame::AppendFrames(ChildListID     aListID,
                                nsFrameList&    aFrameList)
{
  
  
  MOZ_ASSERT(kCaptionList == aListID, "unexpected child list");
  MOZ_ASSERT(aFrameList.IsEmpty() ||
             aFrameList.FirstChild()->IsTableCaption(),
             "appending non-caption frame to captionList");
  mCaptionFrames.AppendFrames(this, aFrameList);

  
  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsTableOuterFrame::InsertFrames(ChildListID     aListID,
                                nsIFrame*       aPrevFrame,
                                nsFrameList&    aFrameList)
{
  MOZ_ASSERT(kCaptionList == aListID, "unexpected child list");
  MOZ_ASSERT(aFrameList.IsEmpty() ||
             aFrameList.FirstChild()->IsTableCaption(),
             "inserting non-caption frame into captionList");
  MOZ_ASSERT(!aPrevFrame || aPrevFrame->GetParent() == this,
             "inserting after sibling frame with different parent");
  mCaptionFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

  
  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsTableOuterFrame::RemoveFrame(ChildListID     aListID,
                               nsIFrame*       aOldFrame)
{
  
  
  NS_PRECONDITION(kCaptionList == aListID, "can't remove inner frame");

  if (HasSideCaption(GetWritingMode())) {
    
    
    InnerTableFrame()->AddStateBits(NS_FRAME_IS_DIRTY);
  }

  
  mCaptionFrames.DestroyFrame(aOldFrame);
  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN); 
}

void
nsTableOuterFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                    const nsRect&           aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  
  

  
  
  if (mCaptionFrames.IsEmpty()) {
    BuildDisplayListForInnerTable(aBuilder, aDirtyRect, aLists);
    return;
  }

  nsDisplayListCollection set;
  BuildDisplayListForInnerTable(aBuilder, aDirtyRect, set);
  
  nsDisplayListSet captionSet(set, set.BlockBorderBackgrounds());
  BuildDisplayListForChild(aBuilder, mCaptionFrames.FirstChild(),
                           aDirtyRect, captionSet);
  
  
  
  set.SortAllByContentOrder(aBuilder, GetContent());
  set.MoveTo(aLists);
}

void
nsTableOuterFrame::BuildDisplayListForInnerTable(nsDisplayListBuilder*   aBuilder,
                                                 const nsRect&           aDirtyRect,
                                                 const nsDisplayListSet& aLists)
{
  
  
  nsIFrame* kid = mFrames.FirstChild();
  
  while (kid) {
    BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
    kid = kid->GetNextSibling();
  }
}

nsStyleContext*
nsTableOuterFrame::GetParentStyleContext(nsIFrame** aProviderFrame) const
{
  
  
  
  
  
  
  
  
  

  return (*aProviderFrame = InnerTableFrame())->StyleContext();
}



void
nsTableOuterFrame::InitChildReflowState(nsPresContext&    aPresContext,                     
                                        nsHTMLReflowState& aReflowState)
                                    
{
  nsMargin collapseBorder;
  nsMargin collapsePadding(0,0,0,0);
  nsMargin* pCollapseBorder  = nullptr;
  nsMargin* pCollapsePadding = nullptr;
  if (aReflowState.frame == InnerTableFrame() &&
      InnerTableFrame()->IsBorderCollapse()) {
    WritingMode wm = aReflowState.GetWritingMode();
    LogicalMargin border = InnerTableFrame()->GetIncludedOuterBCBorder(wm);
    collapseBorder = border.GetPhysicalMargin(wm);
    pCollapseBorder = &collapseBorder;
    pCollapsePadding = &collapsePadding;
  }
  aReflowState.Init(&aPresContext, nullptr, pCollapseBorder, pCollapsePadding);
}



void
nsTableOuterFrame::GetChildMargin(nsPresContext*           aPresContext,
                                  const nsHTMLReflowState& aOuterRS,
                                  nsIFrame*                aChildFrame,
                                  nscoord                  aAvailISize,
                                  LogicalMargin&           aMargin)
{
  NS_ASSERTION(!aChildFrame->IsTableCaption(),
               "didn't expect caption frame; writing-mode may be wrong!");

  
  

  
  
  WritingMode wm = aOuterRS.GetWritingMode();
  LogicalSize availSize(wm, aAvailISize, aOuterRS.AvailableSize(wm).BSize(wm));
  nsHTMLReflowState childRS(aPresContext, aOuterRS, aChildFrame, availSize,
                            nullptr, nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(*aPresContext, childRS);

  aMargin = childRS.ComputedLogicalMargin();
}

static nsSize
GetContainingBlockSize(const nsHTMLReflowState& aOuterRS)
{
  nsSize size(0,0);
  const nsHTMLReflowState* containRS = aOuterRS.mCBReflowState;

  if (containRS) {
    size.width = containRS->ComputedWidth();
    if (NS_UNCONSTRAINEDSIZE == size.width) {
      size.width = 0;
    }
    size.height = containRS->ComputedHeight();
    if (NS_UNCONSTRAINEDSIZE == size.height) {
      size.height = 0;
    }
  }
  return size;
}

 nscoord
nsTableOuterFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord iSize = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                    InnerTableFrame(), nsLayoutUtils::MIN_ISIZE);
  DISPLAY_MIN_WIDTH(this, iSize);
  if (mCaptionFrames.NotEmpty()) {
    nscoord capISize =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                           mCaptionFrames.FirstChild(),
                                           nsLayoutUtils::MIN_ISIZE);
    if (HasSideCaption(GetWritingMode())) {
      iSize += capISize;
    } else {
      if (capISize > iSize) {
        iSize = capISize;
      }
    }
  }
  return iSize;
}

 nscoord
nsTableOuterFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord maxISize;
  DISPLAY_PREF_WIDTH(this, maxISize);

  maxISize = nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
               InnerTableFrame(), nsLayoutUtils::PREF_ISIZE);
  WritingMode wm = GetWritingMode();
  if (mCaptionFrames.NotEmpty()) {
    uint8_t captionSide = GetLogicalCaptionSide(wm);
    switch (captionSide) {
    case NS_STYLE_CAPTION_SIDE_ISTART:
    case NS_STYLE_CAPTION_SIDE_IEND:
      {
        nscoord capMin =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                               mCaptionFrames.FirstChild(),
                                               nsLayoutUtils::MIN_ISIZE);
        maxISize += capMin;
      }
      break;
    default:
      {
        nsLayoutUtils::IntrinsicISizeType iwt;
        if (captionSide == NS_STYLE_CAPTION_SIDE_BSTART ||
            captionSide == NS_STYLE_CAPTION_SIDE_BEND) {
          
          
          iwt = nsLayoutUtils::MIN_ISIZE;
        } else {
          NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE ||
                       captionSide == NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE,
                       "unexpected caption side");
          iwt = nsLayoutUtils::PREF_ISIZE;
        }
        nscoord capPref =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                               mCaptionFrames.FirstChild(),
                                               iwt);
        maxISize = std::max(maxISize, capPref);
      }
      break;
    }
  }
  return maxISize;
}




static nscoord
ChildShrinkWrapISize(nsRenderingContext *aRenderingContext,
                     nsIFrame *aChildFrame, WritingMode aWM,
                     LogicalSize aCBSize, nscoord aAvailableISize,
                     nscoord *aMarginResult = nullptr)
{
  AutoMaybeDisableFontInflation an(aChildFrame);

  
  WritingMode childWM = aChildFrame->GetWritingMode();

  nsCSSOffsetState offsets(aChildFrame, aRenderingContext, aWM,
                           aCBSize.ISize(aWM));
  LogicalSize marginSize =
    offsets.ComputedLogicalMargin().Size(childWM).ConvertTo(aWM, childWM);
  LogicalSize paddingSize =
    offsets.ComputedLogicalPadding().Size(childWM).ConvertTo(aWM, childWM);
  LogicalSize bpSize =
    offsets.ComputedLogicalBorderPadding().Size(childWM).ConvertTo(aWM,
                                                                   childWM);
  LogicalSize size =
    aChildFrame->ComputeSize(aRenderingContext, aWM, aCBSize, aAvailableISize,
                             marginSize, bpSize - paddingSize, paddingSize,
                             nsIFrame::ComputeSizeFlags::eShrinkWrap);
  if (aMarginResult) {
    *aMarginResult = offsets.ComputedLogicalMargin().IStartEnd(aWM);
  }
  return size.ISize(aWM) + marginSize.ISize(aWM) + bpSize.ISize(aWM);
}


LogicalSize
nsTableOuterFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                   WritingMode aWM,
                                   const LogicalSize& aCBSize,
                                   nscoord aAvailableISize,
                                   const LogicalSize& aMargin,
                                   const LogicalSize& aBorder,
                                   const LogicalSize& aPadding,
                                   bool aShrinkWrap)
{
  nscoord kidAvailableISize = aAvailableISize - aMargin.ISize(aWM);
  NS_ASSERTION(aBorder.IsAllZero() && aPadding.IsAllZero(),
               "Table outer frames cannot have borders or paddings");

  
  
  
  

  
  uint8_t captionSide = GetLogicalCaptionSide(aWM);
  nscoord inlineSize;
  if (captionSide == NO_SIDE) {
    inlineSize = ChildShrinkWrapISize(aRenderingContext, InnerTableFrame(), aWM,
                                      aCBSize, kidAvailableISize);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_ISTART ||
             captionSide == NS_STYLE_CAPTION_SIDE_IEND) {
    nscoord capISize = ChildShrinkWrapISize(aRenderingContext,
                                            mCaptionFrames.FirstChild(), aWM,
                                            aCBSize, kidAvailableISize);
    inlineSize = capISize + ChildShrinkWrapISize(aRenderingContext,
                                                 InnerTableFrame(), aWM,
                                                 aCBSize,
                                                 kidAvailableISize - capISize);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_BSTART ||
             captionSide == NS_STYLE_CAPTION_SIDE_BEND) {
    nscoord margin;
    inlineSize = ChildShrinkWrapISize(aRenderingContext, InnerTableFrame(), aWM,
                                      aCBSize, kidAvailableISize, &margin);
    nscoord capISize = ChildShrinkWrapISize(aRenderingContext,
                                            mCaptionFrames.FirstChild(), aWM,
                                            aCBSize, inlineSize - margin);
    if (capISize > inlineSize) {
      inlineSize = capISize;
    }
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE,
                 "unexpected caption-side");
    inlineSize = ChildShrinkWrapISize(aRenderingContext, InnerTableFrame(), aWM,
                                      aCBSize, kidAvailableISize);
    nscoord capISize = ChildShrinkWrapISize(aRenderingContext,
                                            mCaptionFrames.FirstChild(), aWM,
                                            aCBSize, kidAvailableISize);
    if (capISize > inlineSize) {
      inlineSize = capISize;
    }
  }

  return LogicalSize(aWM, inlineSize, NS_UNCONSTRAINEDSIZE);
}

uint8_t
nsTableOuterFrame::GetLogicalCaptionSide(WritingMode aWM)
{
  if (mCaptionFrames.NotEmpty()) {
    return mCaptionFrames.FirstChild()->StyleTableBorder()->
      LogicalCaptionSide(aWM);
  }
  else {
    return NO_SIDE; 
  }
}

uint8_t
nsTableOuterFrame::GetCaptionVerticalAlign()
{
  const nsStyleCoord& va =
    mCaptionFrames.FirstChild()->StyleTextReset()->mVerticalAlign;
  return (va.GetUnit() == eStyleUnit_Enumerated)
           ? va.GetIntValue()
           : NS_STYLE_VERTICAL_ALIGN_TOP;
}

void
nsTableOuterFrame::SetDesiredSize(uint8_t              aCaptionSide,
                                  const LogicalSize&   aInnerSize,
                                  const LogicalSize&   aCaptionSize,
                                  const LogicalMargin& aInnerMargin,
                                  const LogicalMargin& aCaptionMargin,
                                  nscoord&             aISize,
                                  nscoord&             aBSize,
                                  WritingMode          aWM)
{
  aISize = aBSize = 0;

  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_ISTART:
      aISize =
        std::max(aInnerMargin.IStart(aWM),
                 aCaptionMargin.IStartEnd(aWM) + aCaptionSize.ISize(aWM)) +
        aInnerSize.ISize(aWM) + aInnerMargin.IEnd(aWM);
      break;
    case NS_STYLE_CAPTION_SIDE_IEND:
      aISize =
        std::max(aInnerMargin.IEnd(aWM),
                 aCaptionMargin.IStartEnd(aWM) + aCaptionSize.ISize(aWM)) +
        aInnerSize.ISize(aWM) + aInnerMargin.IStart(aWM);
      break;
    default:
      aISize =
        std::max(aInnerMargin.IStartEnd(aWM) + aInnerSize.ISize(aWM),
                 aCaptionMargin.IStartEnd(aWM) + aCaptionSize.ISize(aWM));
      break;
  }

  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_BSTART:
    case NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE:
      aBSize = aInnerSize.BSize(aWM) + aInnerMargin.BEnd(aWM);
      aBSize +=
        std::max(aInnerMargin.BStart(aWM),
                 aCaptionSize.BSize(aWM) + aCaptionMargin.BStartEnd(aWM));
      break;
    case NS_STYLE_CAPTION_SIDE_BEND:
    case NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE:
      aBSize = aInnerSize.BSize(aWM) + aInnerMargin.BStart(aWM);
      aBSize +=
        std::max(aInnerMargin.BEnd(aWM),
                 aCaptionSize.BSize(aWM) + aCaptionMargin.BStartEnd(aWM));
      break;
    case NS_STYLE_CAPTION_SIDE_ISTART:
    case NS_STYLE_CAPTION_SIDE_IEND:
      aBSize = aInnerMargin.BStart(aWM);
      aBSize +=
        std::max(aInnerSize.BSize(aWM) + aInnerMargin.BEnd(aWM),
                 aCaptionSize.BSize(aWM) + aCaptionMargin.BEnd(aWM));
      break;
    default:
      NS_ASSERTION(aCaptionSide == NO_SIDE, "unexpected caption side");
      aBSize = aInnerSize.BSize(aWM) + aInnerMargin.BStartEnd(aWM);
      break;
  }

  
  aISize = std::max(aISize, 0);
  aBSize = std::max(aBSize, 0);
}

nsresult 
nsTableOuterFrame::GetCaptionOrigin(uint32_t             aCaptionSide,
                                    const LogicalSize&   aContainBlockSize,
                                    const LogicalSize&   aInnerSize, 
                                    const LogicalMargin& aInnerMargin,
                                    const LogicalSize&   aCaptionSize,
                                    LogicalMargin&       aCaptionMargin,
                                    LogicalPoint&        aOrigin,
                                    WritingMode          aWM)
{
  aOrigin.I(aWM) = aOrigin.B(aWM) = 0;
  if ((NS_UNCONSTRAINEDSIZE == aInnerSize.ISize(aWM)) ||
      (NS_UNCONSTRAINEDSIZE == aInnerSize.BSize(aWM)) ||
      (NS_UNCONSTRAINEDSIZE == aCaptionSize.ISize(aWM)) ||
      (NS_UNCONSTRAINEDSIZE == aCaptionSize.BSize(aWM))) {
    return NS_OK;
  }
  if (mCaptionFrames.IsEmpty()) {
    return NS_OK;
  }
  
  NS_ASSERTION(NS_AUTOMARGIN != aCaptionMargin.IStart(aWM) &&
               NS_AUTOMARGIN != aCaptionMargin.BStart(aWM) &&
               NS_AUTOMARGIN != aCaptionMargin.BEnd(aWM),
               "The computed caption margin is auto?");

  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_BEND:
    case NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE:
      aOrigin.I(aWM) = aCaptionMargin.IStart(aWM);
      if (aCaptionSide == NS_STYLE_CAPTION_SIDE_BEND) {
        
        
        aOrigin.I(aWM) += aInnerMargin.IStart(aWM);
      }
      break;
    case NS_STYLE_CAPTION_SIDE_ISTART:
      aOrigin.I(aWM) = aCaptionMargin.IStart(aWM);
      break;
    case NS_STYLE_CAPTION_SIDE_IEND:
      aOrigin.I(aWM) = aInnerMargin.IStart(aWM) + aInnerSize.ISize(aWM) +
                       aCaptionMargin.IStart(aWM);
      break;
    default: 
      NS_ASSERTION(aCaptionSide == NS_STYLE_CAPTION_SIDE_BSTART ||
                   aCaptionSide == NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE,
                   "unexpected caption side");
      aOrigin.I(aWM) = aCaptionMargin.IStart(aWM);
      if (aCaptionSide == NS_STYLE_CAPTION_SIDE_BSTART) {
        
        
        aOrigin.I(aWM) += aInnerMargin.IStart(aWM);
      }
      break;
  }
  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_IEND:
    case NS_STYLE_CAPTION_SIDE_ISTART:
      aOrigin.B(aWM) = aInnerMargin.BStart(aWM);
      switch (GetCaptionVerticalAlign()) {
        case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
          aOrigin.B(aWM) = std::max(0, aInnerMargin.BStart(aWM) +
                                       ((aInnerSize.BSize(aWM) -
                                         aCaptionSize.BSize(aWM)) / 2));
          break;
        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
          aOrigin.B(aWM) = std::max(0, aInnerMargin.BStart(aWM) +
                                       aInnerSize.BSize(aWM) -
                                       aCaptionSize.BSize(aWM));
          break;
        default:
          break;
      }
      break;
    case NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE:
    case NS_STYLE_CAPTION_SIDE_BEND:
      aOrigin.B(aWM) = aInnerMargin.BStart(aWM) + aInnerSize.BSize(aWM) +
                       aCaptionMargin.BStart(aWM);
      break;
    case NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE:
    case NS_STYLE_CAPTION_SIDE_BSTART:
      aOrigin.B(aWM) = aInnerMargin.BStart(aWM) + aCaptionMargin.BStart(aWM);
      break;
    default:
      NS_NOTREACHED("Unknown caption alignment type");
      break;
  }
  return NS_OK;
}

nsresult 
nsTableOuterFrame::GetInnerOrigin(uint32_t             aCaptionSide,
                                  const LogicalSize&   aContainBlockSize,
                                  const LogicalSize&   aCaptionSize, 
                                  const LogicalMargin& aCaptionMargin,
                                  const LogicalSize&   aInnerSize,
                                  LogicalMargin&       aInnerMargin,
                                  LogicalPoint&        aOrigin,
                                  WritingMode          aWM)
{
  NS_ASSERTION(NS_AUTOMARGIN != aCaptionMargin.IStart(aWM) &&
               NS_AUTOMARGIN != aCaptionMargin.IEnd(aWM),
               "The computed caption margin is auto?");
  NS_ASSERTION(NS_AUTOMARGIN != aInnerMargin.IStart(aWM) &&
               NS_AUTOMARGIN != aInnerMargin.IEnd(aWM) &&
               NS_AUTOMARGIN != aInnerMargin.BStart(aWM) &&
               NS_AUTOMARGIN != aInnerMargin.BEnd(aWM),
               "The computed inner margin is auto?");
  
  aOrigin.I(aWM) = aOrigin.B(aWM) = 0;
  if ((NS_UNCONSTRAINEDSIZE == aInnerSize.ISize(aWM)) ||
      (NS_UNCONSTRAINEDSIZE == aInnerSize.BSize(aWM)) ||
      (NS_UNCONSTRAINEDSIZE == aCaptionSize.ISize(aWM)) ||
      (NS_UNCONSTRAINEDSIZE == aCaptionSize.BSize(aWM))) {
    return NS_OK;
  }

  nscoord minCapISize =
    aCaptionSize.ISize(aWM) + aCaptionMargin.IStartEnd(aWM);

  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_ISTART:
      if (aInnerMargin.IStart(aWM) < minCapISize) {
        
        aInnerMargin.IEnd(aWM) += aInnerMargin.IStart(aWM) - minCapISize;
        aInnerMargin.IEnd(aWM)  = std::max(0, aInnerMargin.IEnd(aWM));
        aInnerMargin.IStart(aWM) = minCapISize;
      }
      aOrigin.I(aWM) = aInnerMargin.IStart(aWM);
      break;
    default:
      NS_ASSERTION(aCaptionSide == NS_STYLE_CAPTION_SIDE_BSTART ||
                   aCaptionSide == NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE ||
                   aCaptionSide == NS_STYLE_CAPTION_SIDE_BEND ||
                   aCaptionSide == NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE ||
                   aCaptionSide == NS_STYLE_CAPTION_SIDE_IEND ||
                   aCaptionSide == NO_SIDE,
                   "unexpected caption side");
      aOrigin.I(aWM) = aInnerMargin.IStart(aWM);
      break;
  }
  
  
  switch (aCaptionSide) {
    case NS_STYLE_CAPTION_SIDE_BEND:
    case NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE:
      aOrigin.B(aWM) = aInnerMargin.BStart(aWM);
      break;
    case NS_STYLE_CAPTION_SIDE_ISTART:
    case NS_STYLE_CAPTION_SIDE_IEND:
      aOrigin.B(aWM) = aInnerMargin.BStart(aWM);
      switch (GetCaptionVerticalAlign()) {
        case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
          aOrigin.B(aWM) =
            std::max(aInnerMargin.BStart(aWM),
                     (aCaptionSize.BSize(aWM) - aInnerSize.BSize(aWM)) / 2);
          break;
        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
          aOrigin.B(aWM) =
            std::max(aInnerMargin.BStart(aWM),
                     aCaptionSize.BSize(aWM) - aInnerSize.BSize(aWM));
          break;
        default:
          break;
      }
      break;
    case NO_SIDE:
    case NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE:
    case NS_STYLE_CAPTION_SIDE_BSTART:
      aOrigin.B(aWM) = aInnerMargin.BStart(aWM) + aCaptionSize.BSize(aWM) +
                       aCaptionMargin.BStartEnd(aWM);
      break;
    default:
      NS_NOTREACHED("Unknown caption alignment type");
      break;
  }
  return NS_OK;
}

void
nsTableOuterFrame::OuterBeginReflowChild(nsPresContext*            aPresContext,
                                         nsIFrame*                 aChildFrame,
                                         const nsHTMLReflowState&  aOuterRS,
                                         Maybe<nsHTMLReflowState>& aChildRS,
                                         nscoord                   aAvailISize)
{
  
  WritingMode wm = aChildFrame->GetWritingMode();
  LogicalSize outerSize = aOuterRS.AvailableSize(wm);
  nscoord availBSize = outerSize.BSize(wm);
  if (NS_UNCONSTRAINEDSIZE != availBSize) {
    if (mCaptionFrames.FirstChild() == aChildFrame) {
      availBSize = NS_UNCONSTRAINEDSIZE;
    } else {
      LogicalMargin margin(wm);
      GetChildMargin(aPresContext, aOuterRS, aChildFrame,
                     outerSize.ISize(wm), margin);

      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.BStart(wm),
                   "No unconstrainedsize arithmetic, please");
      availBSize -= margin.BStart(wm);

      NS_ASSERTION(NS_UNCONSTRAINEDSIZE != margin.BEnd(wm),
                   "No unconstrainedsize arithmetic, please");
      availBSize -= margin.BEnd(wm);
    }
  }
  LogicalSize availSize(wm, aAvailISize, availBSize);
  
  
  aChildRS.emplace(aPresContext, aOuterRS, aChildFrame, availSize,
                  nullptr, nsHTMLReflowState::CALLER_WILL_INIT);
  InitChildReflowState(*aPresContext, *aChildRS);

  
  if (aChildRS->mFlags.mIsTopOfPage &&
      mCaptionFrames.FirstChild() == aChildFrame) {
    uint8_t captionSide = GetLogicalCaptionSide(wm);
    if (captionSide == NS_STYLE_CAPTION_SIDE_BEND ||
        captionSide == NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE) {
      aChildRS->mFlags.mIsTopOfPage = false;
    }
  }
}

void
nsTableOuterFrame::OuterDoReflowChild(nsPresContext*             aPresContext,
                                      nsIFrame*                  aChildFrame,
                                      const nsHTMLReflowState&   aChildRS,
                                      nsHTMLReflowMetrics&       aMetrics,
                                      nsReflowStatus&            aStatus)
{
  
  
  
  
  const nscoord zeroCWidth = 0;
  WritingMode wm = aChildRS.GetWritingMode();

  
  LogicalPoint childPt = aChildFrame->GetLogicalPosition(wm, zeroCWidth);
  uint32_t flags = NS_FRAME_NO_MOVE_FRAME;

  
  
  
  
  
  if (aChildFrame == InnerTableFrame()) {
    flags |= NS_FRAME_NO_DELETE_NEXT_IN_FLOW_CHILD;
  }

  ReflowChild(aChildFrame, aPresContext, aMetrics, aChildRS,
              wm, childPt, zeroCWidth, flags, aStatus);
}

void 
nsTableOuterFrame::UpdateOverflowAreas(nsHTMLReflowMetrics& aMet)
{
  aMet.SetOverflowAreasToDesiredBounds();
  ConsiderChildOverflow(aMet.mOverflowAreas, InnerTableFrame());
  if (mCaptionFrames.NotEmpty()) {
    ConsiderChildOverflow(aMet.mOverflowAreas, mCaptionFrames.FirstChild());
  }
}

void
nsTableOuterFrame::Reflow(nsPresContext*           aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aOuterRS,
                          nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsTableOuterFrame");
  DISPLAY_REFLOW(aPresContext, this, aOuterRS, aDesiredSize, aStatus);

  
  aDesiredSize.ClearSize();
  aStatus = NS_FRAME_COMPLETE;

  if (!HasAnyStateBits(NS_FRAME_FIRST_REFLOW)) {
    
    
    MoveOverflowToChildList();
  }

  Maybe<nsHTMLReflowState> captionRS;
  Maybe<nsHTMLReflowState> innerRS;

  nsRect origInnerRect = InnerTableFrame()->GetRect();
  nsRect origInnerVisualOverflow = InnerTableFrame()->GetVisualOverflowRect();
  bool innerFirstReflow =
    InnerTableFrame()->HasAnyStateBits(NS_FRAME_FIRST_REFLOW);
  nsRect origCaptionRect;
  nsRect origCaptionVisualOverflow;
  bool captionFirstReflow;
  if (mCaptionFrames.NotEmpty()) {
    origCaptionRect = mCaptionFrames.FirstChild()->GetRect();
    origCaptionVisualOverflow =
      mCaptionFrames.FirstChild()->GetVisualOverflowRect();
    captionFirstReflow =
      mCaptionFrames.FirstChild()->HasAnyStateBits(NS_FRAME_FIRST_REFLOW);
  }
  
  
  WritingMode wm = aOuterRS.GetWritingMode();
  uint8_t captionSide = GetLogicalCaptionSide(wm);
  WritingMode captionWM = wm; 

  if (captionSide == NO_SIDE) {
    
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, aOuterRS.ComputedSize(wm).ISize(wm));
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_ISTART ||
             captionSide == NS_STYLE_CAPTION_SIDE_IEND) {
    
    
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRS, aOuterRS.ComputedSize(wm).ISize(wm));
    captionWM = captionRS->GetWritingMode();
    nscoord innerAvailISize = aOuterRS.ComputedSize(wm).ISize(wm) -
      captionRS->ComputedSizeWithMarginBorderPadding(wm).ISize(wm);
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, innerAvailISize);
  } else if (captionSide == NS_STYLE_CAPTION_SIDE_BSTART ||
             captionSide == NS_STYLE_CAPTION_SIDE_BEND) {
    
    
    
    
    
    
    
    
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, aOuterRS.ComputedSize(wm).ISize(wm));
    
    
    
    
    
    nscoord innerBorderISize =
      innerRS->ComputedSizeWithBorderPadding(wm).ISize(wm);
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(), aOuterRS,
                          captionRS, innerBorderISize);
    captionWM = captionRS->GetWritingMode();
  } else {
    NS_ASSERTION(captionSide == NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE ||
                 captionSide == NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE,
                 "unexpected caption-side");
    
    captionWM = mCaptionFrames.FirstChild()->GetWritingMode();
    OuterBeginReflowChild(aPresContext, mCaptionFrames.FirstChild(),
                          aOuterRS, captionRS,
                          aOuterRS.ComputedSize(captionWM).ISize(captionWM));
    OuterBeginReflowChild(aPresContext, InnerTableFrame(), aOuterRS,
                          innerRS, aOuterRS.ComputedSize(wm).ISize(wm));
  }

  
  Maybe<nsHTMLReflowMetrics> captionMet;
  LogicalSize captionSize(wm);
  LogicalMargin captionMargin(wm);
  if (mCaptionFrames.NotEmpty()) {
    captionMet.emplace(wm);
    nsReflowStatus capStatus; 
    OuterDoReflowChild(aPresContext, mCaptionFrames.FirstChild(),
                       *captionRS, *captionMet, capStatus);
    captionSize.ISize(wm) = captionMet->ISize(wm);
    captionSize.BSize(wm) = captionMet->BSize(wm);
    captionMargin =
      captionRS->ComputedLogicalMargin().ConvertTo(wm, captionWM);
    
    
    
    if (NS_UNCONSTRAINEDSIZE != aOuterRS.AvailableBSize()) {
      nscoord captionBSize = 0;
      switch (captionSide) {
        case NS_STYLE_CAPTION_SIDE_BSTART:
        case NS_STYLE_CAPTION_SIDE_BEND:
        case NS_STYLE_CAPTION_SIDE_BSTART_OUTSIDE:
        case NS_STYLE_CAPTION_SIDE_BEND_OUTSIDE:
          captionBSize = captionSize.BSize(wm) + captionMargin.BStartEnd(wm);
          break;
      }
      innerRS->AvailableBSize() =
        std::max(0, innerRS->AvailableBSize() - captionBSize);
    }
  }

  
  
  nsHTMLReflowMetrics innerMet(innerRS->GetWritingMode());
  OuterDoReflowChild(aPresContext, InnerTableFrame(), *innerRS,
                     innerMet, aStatus);
  LogicalSize innerSize(wm, innerMet.ISize(wm), innerMet.BSize(wm));
  LogicalMargin innerMargin = innerRS->ComputedLogicalMargin();

  LogicalSize containSize(wm, GetContainingBlockSize(aOuterRS));

  
  

  
  

  
  
  LogicalSize desiredSize(wm);
  SetDesiredSize(captionSide, innerSize, captionSize,
                 innerMargin, captionMargin,
                 desiredSize.ISize(wm), desiredSize.BSize(wm), wm);
  aDesiredSize.SetSize(wm, desiredSize);
  nscoord containerWidth = aDesiredSize.Width();
  
  

  if (mCaptionFrames.NotEmpty()) {
    LogicalPoint captionOrigin(wm);
    GetCaptionOrigin(captionSide, containSize, innerSize, innerMargin,
                     captionSize, captionMargin, captionOrigin, wm);
    FinishReflowChild(mCaptionFrames.FirstChild(), aPresContext, *captionMet,
                      captionRS.ptr(), wm, captionOrigin, containerWidth,
                      0);
    captionRS.reset();
  }
  
  

  LogicalPoint innerOrigin(wm);
  GetInnerOrigin(captionSide, containSize, captionSize, captionMargin,
                 innerSize, innerMargin, innerOrigin, wm);
  FinishReflowChild(InnerTableFrame(), aPresContext, innerMet, innerRS.ptr(),
                    wm, innerOrigin, containerWidth, 0);
  innerRS.reset();

  nsTableFrame::InvalidateTableFrame(InnerTableFrame(), origInnerRect,
                                     origInnerVisualOverflow,
                                     innerFirstReflow);
  if (mCaptionFrames.NotEmpty()) {
    nsTableFrame::InvalidateTableFrame(mCaptionFrames.FirstChild(),
                                       origCaptionRect,
                                       origCaptionVisualOverflow,
                                       captionFirstReflow);
  }

  UpdateOverflowAreas(aDesiredSize);

  if (GetPrevInFlow()) {
    ReflowOverflowContainerChildren(aPresContext, aOuterRS,
                                    aDesiredSize.mOverflowAreas, 0,
                                    aStatus);
  }

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aOuterRS, aStatus);

  

  NS_FRAME_SET_TRUNCATION(aStatus, aOuterRS, aDesiredSize);
}

nsIAtom*
nsTableOuterFrame::GetType() const
{
  return nsGkAtoms::tableOuterFrame;
}



nsIContent*
nsTableOuterFrame::GetCellAt(uint32_t aRowIdx, uint32_t aColIdx) const
{
  nsTableCellMap* cellMap = InnerTableFrame()->GetCellMap();
  if (!cellMap) {
    return nullptr;
  }

  nsTableCellFrame* cell = cellMap->GetCellInfoAt(aRowIdx, aColIdx);
  if (!cell) {
    return nullptr;
  }

  return cell->GetContent();
}


nsTableOuterFrame*
NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTableOuterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTableOuterFrame)

#ifdef DEBUG_FRAME_DUMP
nsresult
nsTableOuterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("TableOuter"), aResult);
}
#endif

