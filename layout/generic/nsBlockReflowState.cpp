











































#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsBlockFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsIFrame.h"
#include "nsFrameManager.h"

#include "nsINameSpaceManager.h"


#ifdef DEBUG
#include "nsBlockDebugFlags.h"
#endif

nsBlockReflowState::nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                                       nsPresContext* aPresContext,
                                       nsBlockFrame* aFrame,
                                       const nsHTMLReflowMetrics& aMetrics,
                                       PRBool aTopMarginRoot,
                                       PRBool aBottomMarginRoot,
                                       PRBool aBlockNeedsSpaceManager)
  : mBlock(aFrame),
    mPresContext(aPresContext),
    mReflowState(aReflowState),
    mPrevBottomMargin(),
    mLineNumber(0),
    mFlags(0),
    mFloatBreakType(NS_STYLE_CLEAR_NONE)
{
  SetFlag(BRS_ISFIRSTINFLOW, aFrame->GetPrevInFlow() == nsnull);

  const nsMargin& borderPadding = BorderPadding();

  if (aTopMarginRoot || 0 != aReflowState.mComputedBorderPadding.top) {
    SetFlag(BRS_ISTOPMARGINROOT, PR_TRUE);
  }
  if (aBottomMarginRoot || 0 != aReflowState.mComputedBorderPadding.bottom) {
    SetFlag(BRS_ISBOTTOMMARGINROOT, PR_TRUE);
  }
  if (GetFlag(BRS_ISTOPMARGINROOT)) {
    SetFlag(BRS_APPLYTOPMARGIN, PR_TRUE);
  }
  if (aBlockNeedsSpaceManager) {
    SetFlag(BRS_SPACE_MGR, PR_TRUE);
  }
  
  mSpaceManager = aReflowState.mSpaceManager;

  NS_ASSERTION(mSpaceManager,
               "SpaceManager should be set in nsBlockReflowState" );
  if (mSpaceManager) {
    
    
    mSpaceManager->Translate(borderPadding.left, borderPadding.top);
    mSpaceManager->GetTranslation(mSpaceManagerX, mSpaceManagerY);
  }

  mReflowStatus = NS_FRAME_COMPLETE;

  mPresContext = aPresContext;
  mNextInFlow = NS_STATIC_CAST(nsBlockFrame*, mBlock->GetNextInFlow());

  NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aReflowState.ComputedWidth(),
               "no unconstrained widths should be present anymore");
  mContentArea.width = aReflowState.ComputedWidth();

  
  
  
  
  
  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight) {
    
    
    
    mBottomEdge = aReflowState.availableHeight - borderPadding.bottom;
    mContentArea.height = PR_MAX(0, mBottomEdge - borderPadding.top);
  }
  else {
    
    
    SetFlag(BRS_UNCONSTRAINEDHEIGHT, PR_TRUE);
    mContentArea.height = mBottomEdge = NS_UNCONSTRAINEDSIZE;
  }

  mY = borderPadding.top;
  mBand.Init(mSpaceManager, mContentArea);

  mPrevChild = nsnull;
  mCurrentLine = aFrame->end_lines();

  mMinLineHeight = nsHTMLReflowState::CalcLineHeight(aReflowState.rendContext,
                                                     aReflowState.frame);
}

void
nsBlockReflowState::SetupOverflowPlaceholdersProperty()
{
  if (mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE ||
      !mOverflowPlaceholders.IsEmpty()) {
    mBlock->SetProperty(nsGkAtoms::overflowPlaceholdersProperty,
                        &mOverflowPlaceholders, nsnull);
    mBlock->AddStateBits(NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS);
  }
}

nsBlockReflowState::~nsBlockReflowState()
{
  NS_ASSERTION(mOverflowPlaceholders.IsEmpty(),
               "Leaking overflow placeholder frames");

  
  
  if (mSpaceManager) {
    const nsMargin& borderPadding = BorderPadding();
    mSpaceManager->Translate(-borderPadding.left, -borderPadding.top);
  }

  if (mBlock->GetStateBits() & NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS) {
    mBlock->UnsetProperty(nsGkAtoms::overflowPlaceholdersProperty);
    mBlock->RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS);
  }
}

nsLineBox*
nsBlockReflowState::NewLineBox(nsIFrame* aFrame,
                               PRInt32 aCount,
                               PRBool aIsBlock)
{
  return NS_NewLineBox(mPresContext->PresShell(), aFrame, aCount, aIsBlock);
}

void
nsBlockReflowState::FreeLineBox(nsLineBox* aLine)
{
  if (aLine) {
    aLine->Destroy(mPresContext->PresShell());
  }
}




void
nsBlockReflowState::ComputeBlockAvailSpace(nsIFrame* aFrame,
                                           const nsStyleDisplay* aDisplay,
                                           nsRect& aResult)
{
#ifdef REALLY_NOISY_REFLOW
  printf("CBAS frame=%p has float count %d\n", aFrame, mBand.GetFloatCount());
  mBand.List();
#endif
  aResult.y = mY;
  aResult.height = GetFlag(BRS_UNCONSTRAINEDHEIGHT)
    ? NS_UNCONSTRAINEDSIZE
    : mBottomEdge - mY;

  const nsMargin& borderPadding = BorderPadding();

  
  

  nsSplittableType splitType = aFrame->GetSplittableType();
  if ((NS_FRAME_SPLITTABLE_NON_RECTANGULAR == splitType ||     
       NS_FRAME_NOT_SPLITTABLE == splitType) &&                
      !(aFrame->IsFrameOfType(nsIFrame::eReplaced)) &&         
      aFrame->GetType() != nsGkAtoms::scrollFrame)         
  {
    if (mBand.GetFloatCount()) {
      
      
      const nsStyleBorder* borderStyle = aFrame->GetStyleBorder();
      switch (borderStyle->mFloatEdge) {
        default:
        case NS_STYLE_FLOAT_EDGE_CONTENT:  
          
          
          aResult.x = borderPadding.left;
          aResult.width = mContentArea.width;
          break;
        case NS_STYLE_FLOAT_EDGE_BORDER: 
        case NS_STYLE_FLOAT_EDGE_PADDING:
          {
            
            
            nsMargin m(0, 0, 0, 0);
            const nsStyleMargin* styleMargin = aFrame->GetStyleMargin();
            styleMargin->GetMargin(m); 
            if (NS_STYLE_FLOAT_EDGE_PADDING == borderStyle->mFloatEdge) {
              
              m += borderStyle->GetBorder();
            }

            
            if (mBand.GetLeftFloatCount()) {
              aResult.x = mAvailSpaceRect.x + borderPadding.left - m.left;
            }
            else {
              aResult.x = borderPadding.left;
            }

            
            if (mBand.GetRightFloatCount()) {
              if (mBand.GetLeftFloatCount()) {
                aResult.width = mAvailSpaceRect.width + m.left + m.right;
              }
              else {
                aResult.width = mAvailSpaceRect.width + m.right;
              }
            }
            else {
              aResult.width = mAvailSpaceRect.width + m.left;
            }
          }
          break;

        case NS_STYLE_FLOAT_EDGE_MARGIN:
          {
            
            
            aResult.x = mAvailSpaceRect.x + borderPadding.left;
            aResult.width = mAvailSpaceRect.width;
          }
          break;
      }
    }
    else {
      
      
      
      aResult.x = borderPadding.left;
      aResult.width = mContentArea.width;
    }
  }
  else {
    
    
    
    aResult.x = mAvailSpaceRect.x + borderPadding.left;
    aResult.width = mAvailSpaceRect.width;
  }

#ifdef REALLY_NOISY_REFLOW
  printf("  CBAS: result %d %d %d %d\n", aResult.x, aResult.y, aResult.width, aResult.height);
#endif
}

void
nsBlockReflowState::GetAvailableSpace(nscoord aY, PRBool aRelaxHeightConstraint)
{
#ifdef DEBUG
  
  nscoord wx, wy;
  mSpaceManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mSpaceManagerX) && (wy == mSpaceManagerY),
               "bad coord system");
#endif

  mBand.GetAvailableSpace(aY - BorderPadding().top, aRelaxHeightConstraint,
                          mAvailSpaceRect);

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("GetAvailableSpace: band=%d,%d,%d,%d count=%d\n",
           mAvailSpaceRect.x, mAvailSpaceRect.y,
           mAvailSpaceRect.width, mAvailSpaceRect.height,
           mBand.GetTrapezoidCount());
  }
#endif
}














void
nsBlockReflowState::ReconstructMarginAbove(nsLineList::iterator aLine)
{
  mPrevBottomMargin.Zero();
  nsBlockFrame *block = mBlock;

  nsLineList::iterator firstLine = block->begin_lines();
  for (;;) {
    --aLine;
    if (aLine->IsBlock()) {
      mPrevBottomMargin = aLine->GetCarriedOutBottomMargin();
      break;
    }
    if (!aLine->IsEmpty()) {
      break;
    }
    if (aLine == firstLine) {
      
      
      if (!GetFlag(BRS_ISTOPMARGINROOT)) {
        mPrevBottomMargin.Zero();
      }
      break;
    }
  }
}









void
nsBlockReflowState::RecoverFloats(nsLineList::iterator aLine,
                                  nscoord aDeltaY)
{
  if (aLine->HasFloats()) {
    
    
    nsFloatCache* fc = aLine->GetFirstFloat();
    while (fc) {
      nsIFrame* floatFrame = fc->mPlaceholder->GetOutOfFlowFrame();
      if (aDeltaY != 0) {
        fc->mRegion.y += aDeltaY;
        nsPoint p = floatFrame->GetPosition();
        floatFrame->SetPosition(nsPoint(p.x, p.y + aDeltaY));
      }
#ifdef DEBUG
      if (nsBlockFrame::gNoisyReflow || nsBlockFrame::gNoisySpaceManager) {
        nscoord tx, ty;
        mSpaceManager->GetTranslation(tx, ty);
        nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
        printf("RecoverFloats: txy=%d,%d (%d,%d) ",
               tx, ty, mSpaceManagerX, mSpaceManagerY);
        nsFrame::ListTag(stdout, floatFrame);
        printf(" aDeltaY=%d region={%d,%d,%d,%d}\n",
               aDeltaY, fc->mRegion.x, fc->mRegion.y,
               fc->mRegion.width, fc->mRegion.height);
      }
#endif
      mSpaceManager->AddRectRegion(floatFrame, fc->mRegion);
      fc = fc->Next();
    }
  } else if (aLine->IsBlock()) {
    nsBlockFrame *kid = nsnull;
    aLine->mFirstChild->QueryInterface(kBlockFrameCID, (void**)&kid);
    
    
    
    if (kid && !nsBlockFrame::BlockNeedsSpaceManager(kid)) {
      nscoord tx = kid->mRect.x, ty = kid->mRect.y;

      
      
      
      if (NS_STYLE_POSITION_RELATIVE == kid->GetStyleDisplay()->mPosition) {
        nsPoint *offsets = NS_STATIC_CAST(nsPoint*,
          mPresContext->PropertyTable()->GetProperty(kid,
                                       nsGkAtoms::computedOffsetProperty));

        if (offsets) {
          tx -= offsets->x;
          ty -= offsets->y;
        }
      }
 
      mSpaceManager->Translate(tx, ty);
      for (nsBlockFrame::line_iterator line = kid->begin_lines(),
                                   line_end = kid->end_lines();
           line != line_end;
           ++line)
        
        
        
        RecoverFloats(line, 0);
      mSpaceManager->Translate(-tx, -ty);
    }
  }
}













void
nsBlockReflowState::RecoverStateFrom(nsLineList::iterator aLine,
                                     nscoord aDeltaY)
{
  
  mCurrentLine = aLine;

  
  if (aLine->HasFloats() || aLine->IsBlock()) {
    
    
    
    const nsMargin& bp = BorderPadding();
    mSpaceManager->Translate(-bp.left, -bp.top);

    RecoverFloats(aLine, aDeltaY);

#ifdef DEBUG
    if (nsBlockFrame::gNoisyReflow || nsBlockFrame::gNoisySpaceManager) {
      mSpaceManager->List(stdout);
    }
#endif
    
    mSpaceManager->Translate(bp.left, bp.top);
  }
}

PRBool
nsBlockReflowState::IsImpactedByFloat() const
{
#ifdef REALLY_NOISY_REFLOW
  printf("nsBlockReflowState::IsImpactedByFloat %p returned %d\n", 
         this, mBand.GetFloatCount());
#endif
  return mBand.GetFloatCount() > 0;
}


PRBool
nsBlockReflowState::InitFloat(nsLineLayout&       aLineLayout,
                              nsPlaceholderFrame* aPlaceholder,
                              nsReflowStatus&     aReflowStatus)
{
  
  nsIFrame* floatFrame = aPlaceholder->GetOutOfFlowFrame();
  floatFrame->SetParent(mBlock);

  
  
  return AddFloat(aLineLayout, aPlaceholder, PR_TRUE, aReflowStatus);
}











PRBool
nsBlockReflowState::AddFloat(nsLineLayout&       aLineLayout,
                             nsPlaceholderFrame* aPlaceholder,
                             PRBool              aInitialReflow,
                             nsReflowStatus&     aReflowStatus)
{
  NS_PRECONDITION(mBlock->end_lines() != mCurrentLine, "null ptr");

  aReflowStatus = NS_FRAME_COMPLETE;
  
  nsFloatCache* fc = mFloatCacheFreeList.Alloc();
  fc->mPlaceholder = aPlaceholder;

  PRBool placed;

  
  
  if (aLineLayout.CanPlaceFloatNow()) {
    
    
    
    
    
    nscoord ox, oy;
    mSpaceManager->GetTranslation(ox, oy);
    nscoord dx = ox - mSpaceManagerX;
    nscoord dy = oy - mSpaceManagerY;
    mSpaceManager->Translate(-dx, -dy);

    
    PRBool isLeftFloat;
    
    
    PRBool forceFit = IsAdjacentWithTop() && !aLineLayout.LineIsBreakable();
    placed = FlowAndPlaceFloat(fc, &isLeftFloat, aReflowStatus, forceFit);
    NS_ASSERTION(placed || !forceFit,
                 "If we asked for force-fit, it should have been placed");
    if (forceFit || (placed && !NS_FRAME_IS_TRUNCATED(aReflowStatus))) {
      
      GetAvailableSpace(mY, forceFit);
      aLineLayout.UpdateBand(mAvailSpaceRect.x + BorderPadding().left, mY,
                             mAvailSpaceRect.width,
                             mAvailSpaceRect.height,
                             isLeftFloat,
                             aPlaceholder->GetOutOfFlowFrame());
      
      
      mCurrentLineFloats.Append(fc);
      
      
      aReflowStatus &= ~NS_FRAME_TRUNCATED;
    }
    else {
      if (IsAdjacentWithTop()) {
        
        
        NS_ASSERTION(aLineLayout.LineIsBreakable(),
                     "We can't get here unless forceFit is false");
        aReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
      } else {
        
        
        aReflowStatus |= NS_FRAME_TRUNCATED;
      }
      delete fc;
    }

    
    mSpaceManager->Translate(dx, dy);
  }
  else {
    
    
    placed = PR_TRUE;
    
    
    mBelowCurrentLineFloats.Append(fc);
    if (mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE ||
        aPlaceholder->GetNextInFlow()) {
      
      
      
      
      
      
      
      if (aPlaceholder->GetSplittableType() != NS_FRAME_NOT_SPLITTABLE) {
        aReflowStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
  }
  return placed;
}

PRBool
nsBlockReflowState::CanPlaceFloat(const nsSize& aFloatSize,
                                  PRUint8 aFloats, PRBool aForceFit)
{
  
  
  PRBool result = PR_TRUE;
  if (0 != mBand.GetFloatCount()) {
    
    if (mAvailSpaceRect.width < aFloatSize.width) {
      
      
      result = PR_FALSE;
    }
    else {
      
      
      
      if (mAvailSpaceRect.height < aFloatSize.height) {
        
        
        
        
        
        
        
        
        nscoord xa;
        if (NS_STYLE_FLOAT_LEFT == aFloats) {
          xa = mAvailSpaceRect.x;
        }
        else {
          xa = mAvailSpaceRect.XMost() - aFloatSize.width;

          
          
          
          if (xa < mAvailSpaceRect.x) {
            xa = mAvailSpaceRect.x;
          }
        }
        nscoord xb = xa + aFloatSize.width;

        
        
        const nsMargin& borderPadding = BorderPadding();
        nscoord ya = mY - borderPadding.top;
        if (ya < 0) {
          
          
          
          
          
          ya = 0;
        }
        nscoord yb = ya + aFloatSize.height;

        nscoord saveY = mY;
        for (;;) {
          
          if (mAvailSpaceRect.height <= 0) {
            
            result = PR_FALSE;
            break;
          }

          mY += mAvailSpaceRect.height;
          GetAvailableSpace(mY, aForceFit);

          if (0 == mBand.GetFloatCount()) {
            
            
            break;
          }

          
          
          
          if ((xa < mAvailSpaceRect.x) || (xb > mAvailSpaceRect.XMost())) {
            
            result = PR_FALSE;
            break;
          }

          
          if (yb < mY + mAvailSpaceRect.height) {
            
            
            break;
          }
        }

        
        
        mY = saveY;
        GetAvailableSpace(mY, aForceFit);
      }
    }
  }
  return result;
}

PRBool
nsBlockReflowState::FlowAndPlaceFloat(nsFloatCache*   aFloatCache,
                                      PRBool*         aIsLeftFloat,
                                      nsReflowStatus& aReflowStatus,
                                      PRBool          aForceFit)
{
  aReflowStatus = NS_FRAME_COMPLETE;
  
  
  
  
  
  nscoord saveY = mY;

  nsPlaceholderFrame* placeholder = aFloatCache->mPlaceholder;
  nsIFrame*           floatFrame = placeholder->GetOutOfFlowFrame();

  
  const nsStyleDisplay* floatDisplay = floatFrame->GetStyleDisplay();

  
  nsRect oldRegion = aFloatCache->mRegion;

  
  
  mY = NS_MAX(mSpaceManager->GetLowestRegionTop() + BorderPadding().top, mY);

  
  
  
  if (NS_STYLE_CLEAR_NONE != floatDisplay->mBreakType) {
    
    mY = ClearFloats(mY, floatDisplay->mBreakType);
  }
    
  GetAvailableSpace(mY, aForceFit);

  NS_ASSERTION(floatFrame->GetParent() == mBlock,
               "Float frame has wrong parent");

  
  nsMargin floatMargin;
  mBlock->ReflowFloat(*this, placeholder, floatMargin, aReflowStatus);

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect region = floatFrame->GetRect();
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("flowed float: ");
    nsFrame::ListTag(stdout, floatFrame);
    printf(" (%d,%d,%d,%d)\n",
	   region.x, region.y, region.width, region.height);
  }
#endif

  nsSize floatSize = floatFrame->GetSize() +
                     nsSize(floatMargin.LeftRight(), floatMargin.TopBottom());

  
  
  
  NS_ASSERTION((NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) ||
	       (NS_STYLE_FLOAT_RIGHT == floatDisplay->mFloats),
	       "invalid float type");

  
  PRBool keepFloatOnSameLine = PR_FALSE;

  while (!CanPlaceFloat(floatSize, floatDisplay->mFloats, aForceFit)) {
    if (mAvailSpaceRect.height <= 0) {
      
      mY = saveY;
      return PR_FALSE;
    }

    
    if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
          eCompatibility_NavQuirks != mPresContext->CompatibilityMode() ) {

      mY += mAvailSpaceRect.height;
      GetAvailableSpace(mY, aForceFit);
    } else {
      
      

      
      nsFloatCache* fc = mCurrentLineFloats.Head();
      nsIFrame* prevFrame = nsnull;
      while (fc) {
        if (fc->mPlaceholder->GetOutOfFlowFrame() == floatFrame) {
          break;
        }
        prevFrame = fc->mPlaceholder->GetOutOfFlowFrame();
        fc = fc->Next();
      }
      
      if(prevFrame) {
        
        if (nsGkAtoms::tableOuterFrame == prevFrame->GetType()) {
          
          
          nsIContent* content = prevFrame->GetContent();
          if (content) {
            
            
            if (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::align,
                                     NS_LITERAL_STRING("left"), eIgnoreCase)) {
              keepFloatOnSameLine = PR_TRUE;
              
              
              
              break;
            }
          }
        }
      }

      
      mY += mAvailSpaceRect.height;
      GetAvailableSpace(mY, aForceFit);
      
      
      
      
      mBlock->ReflowFloat(*this, placeholder, floatMargin, aReflowStatus);
      
      floatSize = floatFrame->GetSize() +
                     nsSize(floatMargin.LeftRight(), floatMargin.TopBottom());
    }
  }
  

  
  

  
  
  
  
  PRBool isLeftFloat;
  nscoord floatX, floatY;
  if (NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) {
    isLeftFloat = PR_TRUE;
    floatX = mAvailSpaceRect.x;
  }
  else {
    isLeftFloat = PR_FALSE;
    if (!keepFloatOnSameLine) {
      floatX = mAvailSpaceRect.XMost() - floatSize.width;
    } 
    else {
      
      
      
      floatX = mAvailSpaceRect.x;
    }
  }
  *aIsLeftFloat = isLeftFloat;
  const nsMargin& borderPadding = BorderPadding();
  floatY = mY - borderPadding.top;
  if (floatY < 0) {
    
    
    
    
    
    floatY = 0;
  }

  
  
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) && 
      (NS_UNCONSTRAINEDSIZE != mContentArea.height)) {
    floatSize.height = PR_MAX(floatSize.height, mContentArea.height - floatY);
  }

  nsRect region(floatX, floatY, floatSize.width, floatSize.height);
  
  
  
  if (region.width < 0) {
    
    
    if (isLeftFloat) {
      region.x = region.XMost();
    }
    region.width = 0;
  }
  if (region.height < 0) {
    region.height = 0;
  }
#ifdef DEBUG
  nsresult rv =
#endif
  mSpaceManager->AddRectRegion(floatFrame, region);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "bad float placement");

  
  
  
  
  
  
  
  aFloatCache->mRegion = region +
                         nsPoint(borderPadding.left, borderPadding.top);

  
  
  if (aFloatCache->mRegion != oldRegion) {
    
    
    
    
    nscoord top = NS_MIN(region.y, oldRegion.y);
    nscoord bottom = NS_MAX(region.YMost(), oldRegion.YMost());
    mSpaceManager->IncludeInDamage(top, bottom);
  }

#ifdef NOISY_SPACEMANAGER
  nscoord tx, ty;
  mSpaceManager->GetTranslation(tx, ty);
  nsFrame::ListTag(stdout, mBlock);
  printf(": FlowAndPlaceFloat: AddRectRegion: txy=%d,%d (%d,%d) {%d,%d,%d,%d}\n",
         tx, ty, mSpaceManagerX, mSpaceManagerY,
         aFloatCache->mRegion.x, aFloatCache->mRegion.y,
         aFloatCache->mRegion.width, aFloatCache->mRegion.height);
#endif

  
  
  
  
  nsPoint origin(borderPadding.left + floatMargin.left + floatX,
                 borderPadding.top + floatMargin.top + floatY);

  
  if (NS_STYLE_POSITION_RELATIVE == floatDisplay->mPosition) {
    nsPoint *offsets = NS_STATIC_CAST(nsPoint*,
        floatFrame->GetProperty(nsGkAtoms::computedOffsetProperty));
    if (offsets) {
      origin += *offsets;
    }
  }

  
  
  
  floatFrame->SetPosition(origin);
  nsContainerFrame::PositionFrameView(floatFrame);
  nsContainerFrame::PositionChildViews(floatFrame);

  
  nsRect combinedArea = floatFrame->GetOverflowRect() + origin;

  
  mFloatCombinedArea.UnionRect(combinedArea, mFloatCombinedArea);

  
  mY = saveY;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect r = floatFrame->GetRect();
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("placed float: ");
    nsFrame::ListTag(stdout, floatFrame);
    printf(" %d,%d,%d,%d\n", r.x, r.y, r.width, r.height);
  }
#endif

  return PR_TRUE;
}




PRBool
nsBlockReflowState::PlaceBelowCurrentLineFloats(nsFloatCacheFreeList& aList, PRBool aForceFit)
{
  nsFloatCache* fc = aList.Head();
  while (fc) {
    {
#ifdef DEBUG
      if (nsBlockFrame::gNoisyReflow) {
        nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
        printf("placing bcl float: ");
        nsFrame::ListTag(stdout, fc->mPlaceholder->GetOutOfFlowFrame());
        printf("\n");
      }
#endif
      
      PRBool isLeftFloat;
      nsReflowStatus reflowStatus;
      PRBool placed = FlowAndPlaceFloat(fc, &isLeftFloat, reflowStatus, aForceFit);
      NS_ASSERTION(placed || !aForceFit,
                   "If we're in force-fit mode, we should have placed the float");

      
      
      NS_WARN_IF_FALSE(NS_FRAME_IS_TRUNCATED(reflowStatus) && aForceFit,
                       "This situation currently leads to data not printing");

      if (!placed || (NS_FRAME_IS_TRUNCATED(reflowStatus) && !aForceFit)) {
        
        return PR_FALSE;
      }
      else if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus)) {
        
        nsresult rv = mBlock->SplitPlaceholder(*this, fc->mPlaceholder);
        if (NS_FAILED(rv)) 
          return PR_FALSE;
      } else {
        
        nsIFrame* nextPlaceholder = fc->mPlaceholder->GetNextInFlow();
        if (nextPlaceholder) {
          nsHTMLContainerFrame* parent =
            NS_STATIC_CAST(nsHTMLContainerFrame*, nextPlaceholder->GetParent());
          parent->DeleteNextInFlowChild(mPresContext, nextPlaceholder);
        }
      }
    }
    fc = fc->Next();
  }
  return PR_TRUE;
}

nscoord
nsBlockReflowState::ClearFloats(nscoord aY, PRUint8 aBreakType)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: in: aY=%d(%d)\n",
           aY, aY - BorderPadding().top);
  }
#endif

#ifdef NOISY_FLOAT_CLEARING
  printf("nsBlockReflowState::ClearFloats: aY=%d breakType=%d\n",
         aY, aBreakType);
  mSpaceManager->List(stdout);
#endif
  
  const nsMargin& bp = BorderPadding();
  nscoord newY = mSpaceManager->ClearFloats(aY - bp.top, aBreakType);
  newY += bp.top;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: out: y=%d(%d)\n", newY, newY - bp.top);
  }
#endif

  return newY;
}

