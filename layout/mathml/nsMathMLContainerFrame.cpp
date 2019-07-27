




#include "nsMathMLContainerFrame.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsNameSpaceManager.h"
#include "nsRenderingContext.h"
#include "nsIDOMMutationEvent.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsDisplayList.h"
#include "nsIReflowCallback.h"
#include "mozilla/Likely.h"
#include "nsIScriptError.h"
#include "nsContentUtils.h"
#include "nsMathMLElement.h"

using namespace mozilla;





NS_IMPL_FRAMEARENA_HELPERS(nsMathMLContainerFrame)

NS_QUERYFRAME_HEAD(nsMathMLContainerFrame)
  NS_QUERYFRAME_ENTRY(nsIMathMLFrame)
  NS_QUERYFRAME_ENTRY(nsMathMLContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)





nsresult
nsMathMLContainerFrame::ReflowError(nsRenderingContext& aRenderingContext,
                                    nsHTMLReflowMetrics& aDesiredSize)
{
  
  mEmbellishData.flags = 0;
  mPresentationData.flags = NS_MATHML_ERROR;

  
  
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  aRenderingContext.SetFont(fm);

  
  nsAutoString errorMsg; errorMsg.AssignLiteral("invalid-markup");
  mBoundingMetrics =
    aRenderingContext.GetBoundingMetrics(errorMsg.get(), errorMsg.Length());

  
  WritingMode wm = aDesiredSize.GetWritingMode();
  aDesiredSize.SetBlockStartAscent(fm->MaxAscent());
  nscoord descent = fm->MaxDescent();
  aDesiredSize.BSize(wm) = aDesiredSize.BlockStartAscent() + descent;
  aDesiredSize.ISize(wm) = mBoundingMetrics.width;

  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  return NS_OK;
}

class nsDisplayMathMLError : public nsDisplayItem {
public:
  nsDisplayMathMLError(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayMathMLError);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLError() {
    MOZ_COUNT_DTOR(nsDisplayMathMLError);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("MathMLError", TYPE_MATHML_ERROR)
};

void nsDisplayMathMLError::Paint(nsDisplayListBuilder* aBuilder,
                                 nsRenderingContext* aCtx)
{
  
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(mFrame, getter_AddRefs(fm));
  aCtx->SetFont(fm);

  nsPoint pt = ToReferenceFrame();
  aCtx->SetColor(NS_RGB(255,0,0));
  aCtx->FillRect(nsRect(pt, mFrame->GetSize()));
  aCtx->SetColor(NS_RGB(255,255,255));

  nscoord ascent = aCtx->FontMetrics()->MaxAscent();

  NS_NAMED_LITERAL_STRING(errorMsg, "invalid-markup");
  aCtx->DrawString(errorMsg.get(), uint32_t(errorMsg.Length()),
                   pt.x, pt.y+ascent);
}






static bool
IsForeignChild(const nsIFrame* aFrame)
{
  
  
  return !(aFrame->IsFrameOfType(nsIFrame::eMathML)) ||
    aFrame->GetType() == nsGkAtoms::blockFrame;
}

static void
DestroyHTMLReflowMetrics(void *aPropertyValue)
{
  delete static_cast<nsHTMLReflowMetrics*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(HTMLReflowMetricsProperty, DestroyHTMLReflowMetrics)

 void
nsMathMLContainerFrame::SaveReflowAndBoundingMetricsFor(nsIFrame*                  aFrame,
                                                        const nsHTMLReflowMetrics& aReflowMetrics,
                                                        const nsBoundingMetrics&   aBoundingMetrics)
{
  nsHTMLReflowMetrics *metrics = new nsHTMLReflowMetrics(aReflowMetrics);
  metrics->mBoundingMetrics = aBoundingMetrics;
  aFrame->Properties().Set(HTMLReflowMetricsProperty(), metrics);
}


 void
nsMathMLContainerFrame::GetReflowAndBoundingMetricsFor(nsIFrame*            aFrame,
                                                       nsHTMLReflowMetrics& aReflowMetrics,
                                                       nsBoundingMetrics&   aBoundingMetrics,
                                                       eMathMLFrameType*    aMathMLFrameType)
{
  NS_PRECONDITION(aFrame, "null arg");

  nsHTMLReflowMetrics *metrics = static_cast<nsHTMLReflowMetrics*>
    (aFrame->Properties().Get(HTMLReflowMetricsProperty()));

  
  
  
  NS_ASSERTION(metrics, "Didn't SaveReflowAndBoundingMetricsFor frame!");
  if (metrics) {
    aReflowMetrics = *metrics;
    aBoundingMetrics = metrics->mBoundingMetrics;
  }

  if (aMathMLFrameType) {
    if (!IsForeignChild(aFrame)) {
      nsIMathMLFrame* mathMLFrame = do_QueryFrame(aFrame);
      if (mathMLFrame) {
        *aMathMLFrameType = mathMLFrame->GetMathMLFrameType();
        return;
      }
    }
    *aMathMLFrameType = eMathMLFrameType_UNKNOWN;
  }

}

void
nsMathMLContainerFrame::ClearSavedChildMetrics()
{
  nsIFrame* childFrame = mFrames.FirstChild();
  FramePropertyTable* props = PresContext()->PropertyTable();
  while (childFrame) {
    props->Delete(childFrame, HTMLReflowMetricsProperty());
    childFrame = childFrame->GetNextSibling();
  }
}



void
nsMathMLContainerFrame::GetPreferredStretchSize(nsRenderingContext& aRenderingContext,
                                                uint32_t             aOptions,
                                                nsStretchDirection   aStretchDirection,
                                                nsBoundingMetrics&   aPreferredStretchSize)
{
  if (aOptions & STRETCH_CONSIDER_ACTUAL_SIZE) {
    
    aPreferredStretchSize = mBoundingMetrics;
  }
  else if (aOptions & STRETCH_CONSIDER_EMBELLISHMENTS) {
    
    nsHTMLReflowMetrics metrics(GetWritingMode()); 
    Place(aRenderingContext, false, metrics);
    aPreferredStretchSize = metrics.mBoundingMetrics;
  }
  else {
    
    bool stretchAll =
      NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags) ||
      NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags);
    NS_ASSERTION(NS_MATHML_IS_EMBELLISH_OPERATOR(mEmbellishData.flags) ||
                 stretchAll,
                 "invalid call to GetPreferredStretchSize");
    bool firstTime = true;
    nsBoundingMetrics bm, bmChild;
    nsIFrame* childFrame =
      stretchAll ? GetFirstPrincipalChild() : mPresentationData.baseFrame;
    while (childFrame) {
      
      nsIMathMLFrame* mathMLFrame = do_QueryFrame(childFrame);
      if (mathMLFrame) {
        nsEmbellishData embellishData;
        nsPresentationData presentationData;
        mathMLFrame->GetEmbellishData(embellishData);
        mathMLFrame->GetPresentationData(presentationData);
        if (NS_MATHML_IS_EMBELLISH_OPERATOR(embellishData.flags) &&
            embellishData.direction == aStretchDirection &&
            presentationData.baseFrame) {
          
          
          
          nsIMathMLFrame* mathMLchildFrame = do_QueryFrame(presentationData.baseFrame);
          if (mathMLchildFrame) {
            mathMLFrame = mathMLchildFrame;
          }
        }
        mathMLFrame->GetBoundingMetrics(bmChild);
      }
      else {
        nsHTMLReflowMetrics unused(GetWritingMode());
        GetReflowAndBoundingMetricsFor(childFrame, unused, bmChild);
      }

      if (firstTime) {
        firstTime = false;
        bm = bmChild;
        if (!stretchAll) {
          
          
          break;
        }
      }
      else {
        if (NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags)) {
          
          
          
          bm.descent += bmChild.ascent + bmChild.descent;
          
          
          
          
          if (bmChild.width == 0) {
            bmChild.rightBearing -= bmChild.leftBearing;
            bmChild.leftBearing = 0;
          }
          if (bm.leftBearing > bmChild.leftBearing)
            bm.leftBearing = bmChild.leftBearing;
          if (bm.rightBearing < bmChild.rightBearing)
            bm.rightBearing = bmChild.rightBearing;
        }
        else if (NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags)) {
          
          bm += bmChild;
        }
        else {
          NS_ERROR("unexpected case in GetPreferredStretchSize");
          break;
        }
      }
      childFrame = childFrame->GetNextSibling();
    }
    aPreferredStretchSize = bm;
  }
}

NS_IMETHODIMP
nsMathMLContainerFrame::Stretch(nsRenderingContext& aRenderingContext,
                                nsStretchDirection   aStretchDirection,
                                nsBoundingMetrics&   aContainerSize,
                                nsHTMLReflowMetrics& aDesiredStretchSize)
{
  if (NS_MATHML_IS_EMBELLISH_OPERATOR(mEmbellishData.flags)) {

    if (NS_MATHML_STRETCH_WAS_DONE(mPresentationData.flags)) {
      NS_WARNING("it is wrong to fire stretch more than once on a frame");
      return NS_OK;
    }
    mPresentationData.flags |= NS_MATHML_STRETCH_DONE;

    if (NS_MATHML_HAS_ERROR(mPresentationData.flags)) {
      NS_WARNING("it is wrong to fire stretch on a erroneous frame");
      return NS_OK;
    }

    

    nsIFrame* baseFrame = mPresentationData.baseFrame;
    if (baseFrame) {
      nsIMathMLFrame* mathMLFrame = do_QueryFrame(baseFrame);
      NS_ASSERTION(mathMLFrame, "Something is wrong somewhere");
      if (mathMLFrame) {
        bool stretchAll =
          NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags) ||
          NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags);

        
        
        nsHTMLReflowMetrics childSize(aDesiredStretchSize);
        GetReflowAndBoundingMetricsFor(baseFrame, childSize, childSize.mBoundingMetrics);

        
        
        
        
        
        
        
        
        
        
        
        nsBoundingMetrics containerSize = aContainerSize;
        if (aStretchDirection != NS_STRETCH_DIRECTION_DEFAULT &&
            aStretchDirection != mEmbellishData.direction) {
          if (mEmbellishData.direction == NS_STRETCH_DIRECTION_UNSUPPORTED) {
            containerSize = childSize.mBoundingMetrics;
          }
          else {
            GetPreferredStretchSize(aRenderingContext, 
                                    stretchAll ? STRETCH_CONSIDER_EMBELLISHMENTS : 0,
                                    mEmbellishData.direction, containerSize);
          }
        }

        
        mathMLFrame->Stretch(aRenderingContext,
                             mEmbellishData.direction, containerSize, childSize);
        
        SaveReflowAndBoundingMetricsFor(baseFrame, childSize,
                                        childSize.mBoundingMetrics);
        
        
        
        

        if (stretchAll) {

          nsStretchDirection stretchDir =
            NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags) ?
              NS_STRETCH_DIRECTION_VERTICAL : NS_STRETCH_DIRECTION_HORIZONTAL;

          GetPreferredStretchSize(aRenderingContext, STRETCH_CONSIDER_EMBELLISHMENTS,
                                  stretchDir, containerSize);

          nsIFrame* childFrame = mFrames.FirstChild();
          while (childFrame) {
            if (childFrame != mPresentationData.baseFrame) {
              mathMLFrame = do_QueryFrame(childFrame);
              if (mathMLFrame) {
                
                GetReflowAndBoundingMetricsFor(childFrame, 
                  childSize, childSize.mBoundingMetrics);
                
                mathMLFrame->Stretch(aRenderingContext, stretchDir,
                                     containerSize, childSize);
                
                SaveReflowAndBoundingMetricsFor(childFrame, childSize,
                                                childSize.mBoundingMetrics);
              }
            }
            childFrame = childFrame->GetNextSibling();
          }
        }

        
        nsresult rv = Place(aRenderingContext, true, aDesiredStretchSize);
        if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
          
          DidReflowChildren(mFrames.FirstChild());
        }

        
        
        

        nsEmbellishData parentData;
        GetEmbellishDataFrom(GetParent(), parentData);
        
        
        if (parentData.coreFrame != mEmbellishData.coreFrame) {
          
          
          
          nsEmbellishData coreData;
          GetEmbellishDataFrom(mEmbellishData.coreFrame, coreData);

          mBoundingMetrics.width +=
            coreData.leadingSpace + coreData.trailingSpace;
          aDesiredStretchSize.Width() = mBoundingMetrics.width;
          aDesiredStretchSize.mBoundingMetrics.width = mBoundingMetrics.width;

          nscoord dx = (StyleVisibility()->mDirection ?
                        coreData.trailingSpace : coreData.leadingSpace);
          if (dx != 0) {
            mBoundingMetrics.leftBearing += dx;
            mBoundingMetrics.rightBearing += dx;
            aDesiredStretchSize.mBoundingMetrics.leftBearing += dx;
            aDesiredStretchSize.mBoundingMetrics.rightBearing += dx;

            nsIFrame* childFrame = mFrames.FirstChild();
            while (childFrame) {
              childFrame->SetPosition(childFrame->GetPosition()
                                      + nsPoint(dx, 0));
              childFrame = childFrame->GetNextSibling();
            }
          }
        }

        
        ClearSavedChildMetrics();
        
        GatherAndStoreOverflow(&aDesiredStretchSize);
      }
    }
  }
  return NS_OK;
}

nsresult
nsMathMLContainerFrame::FinalizeReflow(nsRenderingContext& aRenderingContext,
                                       nsHTMLReflowMetrics& aDesiredSize)
{
  
  
  
  
  
  
  
  
  


  
  
  
  

  
  
  
  
  
  bool placeOrigin = !NS_MATHML_IS_EMBELLISH_OPERATOR(mEmbellishData.flags) ||
                       (mEmbellishData.coreFrame != this && !mPresentationData.baseFrame &&
                        mEmbellishData.direction == NS_STRETCH_DIRECTION_UNSUPPORTED);
  nsresult rv = Place(aRenderingContext, placeOrigin, aDesiredSize);

  
  
  
  
  
  
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
    GatherAndStoreOverflow(&aDesiredSize);
    DidReflowChildren(GetFirstPrincipalChild());
    return rv;
  }

  bool parentWillFireStretch = false;
  if (!placeOrigin) {
    
    
    
    nsIMathMLFrame* mathMLFrame = do_QueryFrame(GetParent());
    if (mathMLFrame) {
      nsEmbellishData embellishData;
      nsPresentationData presentationData;
      mathMLFrame->GetEmbellishData(embellishData);
      mathMLFrame->GetPresentationData(presentationData);
      if (NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(presentationData.flags) ||
          NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(presentationData.flags) ||
          (NS_MATHML_IS_EMBELLISH_OPERATOR(embellishData.flags)
            && presentationData.baseFrame == this))
      {
        parentWillFireStretch = true;
      }
    }
    if (!parentWillFireStretch) {
      

      bool stretchAll =
        
        NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags);

      nsBoundingMetrics defaultSize;
      if (mEmbellishData.coreFrame == this 
          || stretchAll) { 
        
        defaultSize = aDesiredSize.mBoundingMetrics;
      }
      else { 
        
        GetPreferredStretchSize(aRenderingContext, 0, mEmbellishData.direction,
                                defaultSize);
      }
      Stretch(aRenderingContext, NS_STRETCH_DIRECTION_DEFAULT, defaultSize,
              aDesiredSize);
#ifdef DEBUG
      {
        
        
        nsIFrame* childFrame = GetFirstPrincipalChild();
        for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
          NS_ASSERTION(!(childFrame->GetStateBits() & NS_FRAME_IN_REFLOW),
                       "DidReflow() was never called");
        }
      }
#endif
    }
  }

  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  
  FixInterFrameSpacing(aDesiredSize);

  if (!parentWillFireStretch) {
    
    
    ClearSavedChildMetrics();
    
    GatherAndStoreOverflow(&aDesiredSize);
  }

  return NS_OK;
}











 void
nsMathMLContainerFrame::PropagatePresentationDataFor(nsIFrame*       aFrame,
                                                     uint32_t        aFlagsValues,
                                                     uint32_t        aFlagsToUpdate)
{
  if (!aFrame || !aFlagsToUpdate)
    return;
  nsIMathMLFrame* mathMLFrame = do_QueryFrame(aFrame);
  if (mathMLFrame) {
    
    mathMLFrame->UpdatePresentationData(aFlagsValues,
                                        aFlagsToUpdate);
    
    
    mathMLFrame->UpdatePresentationDataFromChildAt(0, -1,
      aFlagsValues, aFlagsToUpdate);
  }
  else {
    
    nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();
    while (childFrame) {
      PropagatePresentationDataFor(childFrame,
        aFlagsValues, aFlagsToUpdate);
      childFrame = childFrame->GetNextSibling();
    }
  }
}

 void
nsMathMLContainerFrame::PropagatePresentationDataFromChildAt(nsIFrame*       aParentFrame,
                                                             int32_t         aFirstChildIndex,
                                                             int32_t         aLastChildIndex,
                                                             uint32_t        aFlagsValues,
                                                             uint32_t        aFlagsToUpdate)
{
  if (!aParentFrame || !aFlagsToUpdate)
    return;
  int32_t index = 0;
  nsIFrame* childFrame = aParentFrame->GetFirstPrincipalChild();
  while (childFrame) {
    if ((index >= aFirstChildIndex) &&
        ((aLastChildIndex <= 0) || ((aLastChildIndex > 0) &&
         (index <= aLastChildIndex)))) {
      PropagatePresentationDataFor(childFrame,
        aFlagsValues, aFlagsToUpdate);
    }
    index++;
    childFrame = childFrame->GetNextSibling();
  }
}







void
nsMathMLContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags)) {
    if (!IsVisibleForPainting(aBuilder))
      return;

    aLists.Content()->AppendNewToTop(
      new (aBuilder) nsDisplayMathMLError(aBuilder, this));
    return;
  }

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists,
                                      DISPLAY_CHILD_INLINE);

#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
  
  
  
  
  
  
  DisplayBoundingMetrics(aBuilder, this, mReference, mBoundingMetrics, aLists);
#endif
}




 void
nsMathMLContainerFrame::RebuildAutomaticDataForChildren(nsIFrame* aParentFrame)
{
  
  
  
  
  nsIFrame* childFrame = aParentFrame->GetFirstPrincipalChild();
  while (childFrame) {
    nsIMathMLFrame* childMathMLFrame = do_QueryFrame(childFrame);
    if (childMathMLFrame) {
      childMathMLFrame->InheritAutomaticData(aParentFrame);
    }
    RebuildAutomaticDataForChildren(childFrame);
    childFrame = childFrame->GetNextSibling();
  }
  nsIMathMLFrame* mathMLFrame = do_QueryFrame(aParentFrame);
  if (mathMLFrame) {
    mathMLFrame->TransmitAutomaticData();
  }
}

 nsresult
nsMathMLContainerFrame::ReLayoutChildren(nsIFrame* aParentFrame)
{
  if (!aParentFrame)
    return NS_OK;

  
  nsIFrame* frame = aParentFrame;
  while (1) {
     nsIFrame* parent = frame->GetParent();
     if (!parent || !parent->GetContent())
       break;

    
    nsIMathMLFrame* mathMLFrame = do_QueryFrame(frame);
    if (mathMLFrame)
      break;

    
    nsIContent* content = frame->GetContent();
    NS_ASSERTION(content, "dangling frame without a content node");
    if (!content)
      break;
    if (content->GetNameSpaceID() == kNameSpaceID_MathML &&
        content->Tag() == nsGkAtoms::math)
      break;

    frame = parent;
  }

  
  RebuildAutomaticDataForChildren(frame);

  
  nsIFrame* parent = frame->GetParent();
  NS_ASSERTION(parent, "No parent to pass the reflow request up to");
  if (!parent)
    return NS_OK;

  frame->PresContext()->PresShell()->
    FrameNeedsReflow(frame, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);

  return NS_OK;
}





nsresult
nsMathMLContainerFrame::ChildListChanged(int32_t aModType)
{
  
  
  
  nsIFrame* frame = this;
  if (mEmbellishData.coreFrame) {
    nsIFrame* parent = GetParent();
    nsEmbellishData embellishData;
    for ( ; parent; frame = parent, parent = parent->GetParent()) {
      GetEmbellishDataFrom(parent, embellishData);
      if (embellishData.coreFrame != mEmbellishData.coreFrame)
        break;
    }
  }
  return ReLayoutChildren(frame);
}

void
nsMathMLContainerFrame::AppendFrames(ChildListID     aListID,
                                     nsFrameList&    aFrameList)
{
  MOZ_ASSERT(aListID == kPrincipalList);
  mFrames.AppendFrames(this, aFrameList);
  ChildListChanged(nsIDOMMutationEvent::ADDITION);
}

void
nsMathMLContainerFrame::InsertFrames(ChildListID     aListID,
                                     nsIFrame*       aPrevFrame,
                                     nsFrameList&    aFrameList)
{
  MOZ_ASSERT(aListID == kPrincipalList);
  mFrames.InsertFrames(this, aPrevFrame, aFrameList);
  ChildListChanged(nsIDOMMutationEvent::ADDITION);
}

void
nsMathMLContainerFrame::RemoveFrame(ChildListID     aListID,
                                    nsIFrame*       aOldFrame)
{
  MOZ_ASSERT(aListID == kPrincipalList);
  mFrames.DestroyFrame(aOldFrame);
  ChildListChanged(nsIDOMMutationEvent::REMOVAL);
}

nsresult
nsMathMLContainerFrame::AttributeChanged(int32_t         aNameSpaceID,
                                         nsIAtom*        aAttribute,
                                         int32_t         aModType)
{
  
  
  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);

  return NS_OK;
}

void
nsMathMLContainerFrame::GatherAndStoreOverflow(nsHTMLReflowMetrics* aMetrics)
{
  
  
  aMetrics->SetOverflowAreasToDesiredBounds();

  
  
  nsRect boundingBox(mBoundingMetrics.leftBearing,
                     aMetrics->BlockStartAscent() - mBoundingMetrics.ascent,
                     mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing,
                     mBoundingMetrics.ascent + mBoundingMetrics.descent);

  
  
  aMetrics->mOverflowAreas.UnionAllWith(boundingBox);

  
  
  
  
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    ConsiderChildOverflow(aMetrics->mOverflowAreas, childFrame);
    childFrame = childFrame->GetNextSibling();
  }

  FinishAndStoreOverflow(aMetrics);
}

bool
nsMathMLContainerFrame::UpdateOverflow()
{
  
  PresContext()->PresShell()->FrameNeedsReflow(
    this, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);

  
  return false;
}

void
nsMathMLContainerFrame::ReflowChild(nsIFrame*                aChildFrame,
                                    nsPresContext*           aPresContext,
                                    nsHTMLReflowMetrics&     aDesiredSize,
                                    const nsHTMLReflowState& aReflowState,
                                    nsReflowStatus&          aStatus)
{
  
  
  
  
  
  
  
  
  

#ifdef DEBUG
  nsInlineFrame* inlineFrame = do_QueryFrame(aChildFrame);
  NS_ASSERTION(!inlineFrame, "Inline frames should be wrapped in blocks");
#endif
  
  nsContainerFrame::
         ReflowChild(aChildFrame, aPresContext, aDesiredSize, aReflowState,
                     0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);

  if (aDesiredSize.BlockStartAscent() == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    
    nscoord ascent;
    WritingMode wm = aDesiredSize.GetWritingMode();
    if (!nsLayoutUtils::GetLastLineBaseline(wm, aChildFrame, &ascent)) {
      
      
      
      aDesiredSize.SetBlockStartAscent(aDesiredSize.BSize(wm));
    } else {
      aDesiredSize.SetBlockStartAscent(ascent);
    }
  }
  if (IsForeignChild(aChildFrame)) {
    
    nsRect r = aChildFrame->ComputeTightBounds(aReflowState.rendContext->ThebesContext());
    aDesiredSize.mBoundingMetrics.leftBearing = r.x;
    aDesiredSize.mBoundingMetrics.rightBearing = r.XMost();
    aDesiredSize.mBoundingMetrics.ascent = aDesiredSize.BlockStartAscent() - r.y;
    aDesiredSize.mBoundingMetrics.descent = r.YMost() - aDesiredSize.BlockStartAscent();
    aDesiredSize.mBoundingMetrics.width = aDesiredSize.Width();
  }
}

void
nsMathMLContainerFrame::Reflow(nsPresContext*           aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  aDesiredSize.Width() = aDesiredSize.Height() = 0;
  aDesiredSize.SetBlockStartAscent(0);
  aDesiredSize.mBoundingMetrics = nsBoundingMetrics();

  
  
  

  nsReflowStatus childStatus;
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    nsHTMLReflowMetrics childDesiredSize(aReflowState, 
                                         aDesiredSize.mFlags);
    WritingMode wm = childFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    ReflowChild(childFrame, aPresContext, childDesiredSize,
                childReflowState, childStatus);
    
    SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                    childDesiredSize.mBoundingMetrics);
    childFrame = childFrame->GetNextSibling();
  }

  
  
  

  
  

  if (!NS_MATHML_IS_EMBELLISH_OPERATOR(mEmbellishData.flags) &&
      (NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags) ||
       NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags))) {

    
    nsStretchDirection stretchDir =
      NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags) 
      ? NS_STRETCH_DIRECTION_VERTICAL 
      : NS_STRETCH_DIRECTION_HORIZONTAL;

    
    
    
    
    nsBoundingMetrics containerSize;
    GetPreferredStretchSize(*aReflowState.rendContext, 0, stretchDir,
                            containerSize);

    
    childFrame = mFrames.FirstChild();
    while (childFrame) {
      nsIMathMLFrame* mathMLFrame = do_QueryFrame(childFrame);
      if (mathMLFrame) {
        
        nsHTMLReflowMetrics childDesiredSize(aReflowState);
        GetReflowAndBoundingMetricsFor(childFrame,
          childDesiredSize, childDesiredSize.mBoundingMetrics);

        mathMLFrame->Stretch(*aReflowState.rendContext, stretchDir,
                             containerSize, childDesiredSize);
        
        SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                        childDesiredSize.mBoundingMetrics);
      }
      childFrame = childFrame->GetNextSibling();
    }
  }

  
  
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

static nscoord AddInterFrameSpacingToSize(nsHTMLReflowMetrics&    aDesiredSize,
                                          nsMathMLContainerFrame* aFrame);

 nscoord
nsMathMLContainerFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  nsHTMLReflowMetrics desiredSize(GetWritingMode());
  GetIntrinsicISizeMetrics(aRenderingContext, desiredSize);

  
  
  AddInterFrameSpacingToSize(desiredSize, this);
  result = desiredSize.ISize(GetWritingMode());
  return result;
}

 nscoord
nsMathMLContainerFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  nsHTMLReflowMetrics desiredSize(GetWritingMode());
  GetIntrinsicISizeMetrics(aRenderingContext, desiredSize);

  
  
  AddInterFrameSpacingToSize(desiredSize, this);
  result = desiredSize.ISize(GetWritingMode());
  return result;
}

 void
nsMathMLContainerFrame::GetIntrinsicISizeMetrics(nsRenderingContext* aRenderingContext, nsHTMLReflowMetrics& aDesiredSize)
{
  
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    nsHTMLReflowMetrics childDesiredSize(GetWritingMode()); 

    nsMathMLContainerFrame* containerFrame = do_QueryFrame(childFrame);
    if (containerFrame) {
      containerFrame->GetIntrinsicISizeMetrics(aRenderingContext,
                                               childDesiredSize);
    } else {
      
      
      
      nscoord width =
        nsLayoutUtils::IntrinsicForContainer(aRenderingContext, childFrame,
                                             nsLayoutUtils::PREF_ISIZE);

      childDesiredSize.Width() = width;
      childDesiredSize.mBoundingMetrics.width = width;
      childDesiredSize.mBoundingMetrics.leftBearing = 0;
      childDesiredSize.mBoundingMetrics.rightBearing = width;

      nscoord x, xMost;
      if (NS_SUCCEEDED(childFrame->GetPrefWidthTightBounds(aRenderingContext,
                                                           &x, &xMost))) {
        childDesiredSize.mBoundingMetrics.leftBearing = x;
        childDesiredSize.mBoundingMetrics.rightBearing = xMost;
      }
    }

    SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                    childDesiredSize.mBoundingMetrics);

    childFrame = childFrame->GetNextSibling();
  }

  
  nsresult rv = MeasureForWidth(*aRenderingContext, aDesiredSize);
  if (NS_FAILED(rv)) {
    ReflowError(*aRenderingContext, aDesiredSize);
  }

  ClearSavedChildMetrics();
}

 nsresult
nsMathMLContainerFrame::MeasureForWidth(nsRenderingContext& aRenderingContext,
                                        nsHTMLReflowMetrics& aDesiredSize)
{
  return Place(aRenderingContext, false, aDesiredSize);
}





static int32_t kInterFrameSpacingTable[eMathMLFrameType_COUNT][eMathMLFrameType_COUNT] =
{
  
  
  

  
     {0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01},
     {0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01},
   {0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00}
};

#define GET_INTERSPACE(scriptlevel_, frametype1_, frametype2_, space_)  \
   /* no space if there is a frame that we know nothing about */        \
   if (frametype1_ == eMathMLFrameType_UNKNOWN ||                       \
       frametype2_ == eMathMLFrameType_UNKNOWN)                         \
    space_ = 0;                                                         \
  else {                                                                \
    space_ = kInterFrameSpacingTable[frametype1_][frametype2_];         \
    space_ = (scriptlevel_ > 0 && (space_ & 0xF0))                      \
      ? 0 /* spacing is disabled */                                     \
      : space_ & 0x0F;                                                  \
  }                                                                     \











static nscoord
GetInterFrameSpacing(int32_t           aScriptLevel,
                     eMathMLFrameType  aFirstFrameType,
                     eMathMLFrameType  aSecondFrameType,
                     eMathMLFrameType* aFromFrameType, 
                     int32_t*          aCarrySpace)    
{
  eMathMLFrameType firstType = aFirstFrameType;
  eMathMLFrameType secondType = aSecondFrameType;

  int32_t space;
  GET_INTERSPACE(aScriptLevel, firstType, secondType, space);

  
  if (secondType == eMathMLFrameType_OperatorInvisible) {
    
    
    if (*aFromFrameType == eMathMLFrameType_UNKNOWN) {
      *aFromFrameType = firstType;
      *aCarrySpace = space;
    }
    
    space = 0;
  }
  else if (*aFromFrameType != eMathMLFrameType_UNKNOWN) {
    
    

    firstType = *aFromFrameType;

    
    
    
    
    
    
    
    
    
    if (firstType == eMathMLFrameType_UprightIdentifier) {
      firstType = eMathMLFrameType_OperatorUserDefined;
    }
    else if (secondType == eMathMLFrameType_UprightIdentifier) {
      secondType = eMathMLFrameType_OperatorUserDefined;
    }

    GET_INTERSPACE(aScriptLevel, firstType, secondType, space);

    
    
    
    
    
    if (secondType != eMathMLFrameType_OperatorOrdinary &&
        space < *aCarrySpace)
      space = *aCarrySpace;

    
    *aFromFrameType = eMathMLFrameType_UNKNOWN;
    *aCarrySpace = 0;
  }

  return space;
}

static nscoord GetThinSpace(const nsStyleFont* aStyleFont)
{
  return NSToCoordRound(float(aStyleFont->mFont.size)*float(3) / float(18));
}

class nsMathMLContainerFrame::RowChildFrameIterator {
public:
  explicit RowChildFrameIterator(nsMathMLContainerFrame* aParentFrame) :
    mParentFrame(aParentFrame),
    mSize(aParentFrame->GetWritingMode()), 
    mX(0),
    mCarrySpace(0),
    mFromFrameType(eMathMLFrameType_UNKNOWN),
    mRTL(aParentFrame->StyleVisibility()->mDirection)
  {
    if (!mRTL) {
      mChildFrame = aParentFrame->mFrames.FirstChild();
    } else {
      mChildFrame = aParentFrame->mFrames.LastChild();
    }

    if (!mChildFrame)
      return;

    InitMetricsForChild();
  }

  RowChildFrameIterator& operator++()
  {
    
    mX += mSize.mBoundingMetrics.width + mItalicCorrection;

    if (!mRTL) {
      mChildFrame = mChildFrame->GetNextSibling();
    } else {
      mChildFrame = mChildFrame->GetPrevSibling();
    }

    if (!mChildFrame)
      return *this;

    eMathMLFrameType prevFrameType = mChildFrameType;
    InitMetricsForChild();

    
    const nsStyleFont* font = mParentFrame->StyleFont();
    nscoord space =
      GetInterFrameSpacing(font->mScriptLevel,
                           prevFrameType, mChildFrameType,
                           &mFromFrameType, &mCarrySpace);
    mX += space * GetThinSpace(font);
    return *this;
  }

  nsIFrame* Frame() const { return mChildFrame; }
  nscoord X() const { return mX; }
  const nsHTMLReflowMetrics& ReflowMetrics() const { return mSize; }
  nscoord Ascent() const { return mSize.BlockStartAscent(); }
  nscoord Descent() const { return mSize.Height() - mSize.BlockStartAscent(); }
  const nsBoundingMetrics& BoundingMetrics() const {
    return mSize.mBoundingMetrics;
  }

private:
  const nsMathMLContainerFrame* mParentFrame;
  nsIFrame* mChildFrame;
  nsHTMLReflowMetrics mSize;
  nscoord mX;

  nscoord mItalicCorrection;
  eMathMLFrameType mChildFrameType;
  int32_t mCarrySpace;
  eMathMLFrameType mFromFrameType;

  bool mRTL;

  void InitMetricsForChild()
  {
    GetReflowAndBoundingMetricsFor(mChildFrame, mSize, mSize.mBoundingMetrics,
                                   &mChildFrameType);
    nscoord leftCorrection, rightCorrection;
    GetItalicCorrection(mSize.mBoundingMetrics,
                        leftCorrection, rightCorrection);
    if (!mChildFrame->GetPrevSibling() &&
        mParentFrame->GetContent()->Tag() == nsGkAtoms::msqrt_) {
      
      
      if (!mRTL) {
        leftCorrection = 0;
      } else {
        rightCorrection = 0;
      }
    }
    
    
    mX += leftCorrection;
    mItalicCorrection = rightCorrection;
  }
};

 nsresult
nsMathMLContainerFrame::Place(nsRenderingContext& aRenderingContext,
                              bool                 aPlaceOrigin,
                              nsHTMLReflowMetrics& aDesiredSize)
{
  
  mBoundingMetrics = nsBoundingMetrics();

  RowChildFrameIterator child(this);
  nscoord ascent = 0, descent = 0;
  while (child.Frame()) {
    if (descent < child.Descent())
      descent = child.Descent();
    if (ascent < child.Ascent())
      ascent = child.Ascent();
    
    mBoundingMetrics.width = child.X();
    mBoundingMetrics += child.BoundingMetrics();
    ++child;
  }
  
  
  
  
  mBoundingMetrics.width = child.X();

  aDesiredSize.Width() = std::max(0, mBoundingMetrics.width);
  aDesiredSize.Height() = ascent + descent;
  aDesiredSize.SetBlockStartAscent(ascent);
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.BlockStartAscent();

  
  

  if (aPlaceOrigin) {
    PositionRowChildFrames(0, aDesiredSize.BlockStartAscent());
  }

  return NS_OK;
}

void
nsMathMLContainerFrame::PositionRowChildFrames(nscoord aOffsetX,
                                               nscoord aBaseline)
{
  RowChildFrameIterator child(this);
  while (child.Frame()) {
    nscoord dx = aOffsetX + child.X();
    nscoord dy = aBaseline - child.Ascent();
    FinishReflowChild(child.Frame(), PresContext(), child.ReflowMetrics(),
                      nullptr, dx, dy, 0);
    ++child;
  }
}

class ForceReflow : public nsIReflowCallback {
public:
  virtual bool ReflowFinished() MOZ_OVERRIDE {
    return true;
  }
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE {}
};



static ForceReflow gForceReflow;

void
nsMathMLContainerFrame::SetIncrementScriptLevel(int32_t aChildIndex, bool aIncrement)
{
  nsIFrame* child = PrincipalChildList().FrameAt(aChildIndex);
  if (!child)
    return;
  nsIContent* content = child->GetContent();
  if (!content->IsMathML())
    return;
  nsMathMLElement* element = static_cast<nsMathMLElement*>(content);

  if (element->GetIncrementScriptLevel() == aIncrement)
    return;

  
  
  element->SetIncrementScriptLevel(aIncrement, true);
  PresContext()->PresShell()->PostReflowCallback(&gForceReflow);
}




static nscoord
GetInterFrameSpacingFor(int32_t         aScriptLevel,
                        nsIFrame*       aParentFrame,
                        nsIFrame*       aChildFrame)
{
  nsIFrame* childFrame = aParentFrame->GetFirstPrincipalChild();
  if (!childFrame || aChildFrame == childFrame)
    return 0;

  int32_t carrySpace = 0;
  eMathMLFrameType fromFrameType = eMathMLFrameType_UNKNOWN;
  eMathMLFrameType prevFrameType = eMathMLFrameType_UNKNOWN;
  eMathMLFrameType childFrameType = nsMathMLFrame::GetMathMLFrameTypeFor(childFrame);
  childFrame = childFrame->GetNextSibling();
  while (childFrame) {
    prevFrameType = childFrameType;
    childFrameType = nsMathMLFrame::GetMathMLFrameTypeFor(childFrame);
    nscoord space = GetInterFrameSpacing(aScriptLevel,
      prevFrameType, childFrameType, &fromFrameType, &carrySpace);
    if (aChildFrame == childFrame) {
      
      nsStyleContext* parentContext = aParentFrame->StyleContext();
      nscoord thinSpace = GetThinSpace(parentContext->StyleFont());
      
      return space * thinSpace;
    }
    childFrame = childFrame->GetNextSibling();
  }

  NS_NOTREACHED("child not in the childlist of its parent");
  return 0;
}

static nscoord
AddInterFrameSpacingToSize(nsHTMLReflowMetrics&    aDesiredSize,
                           nsMathMLContainerFrame* aFrame)
{
  nscoord gap = 0;
  nsIFrame* parent = aFrame->GetParent();
  nsIContent* parentContent = parent->GetContent();
  if (MOZ_UNLIKELY(!parentContent)) {
    return 0;
  }
  nsIAtom *parentTag = parentContent->Tag();
  if (parentContent->GetNameSpaceID() == kNameSpaceID_MathML && 
      (parentTag == nsGkAtoms::math || parentTag == nsGkAtoms::mtd_)) {
    gap = GetInterFrameSpacingFor(aFrame->StyleFont()->mScriptLevel,
                                  parent, aFrame);
    
    nscoord leftCorrection = 0, italicCorrection = 0;
    aFrame->GetItalicCorrection(aDesiredSize.mBoundingMetrics,
                                leftCorrection, italicCorrection);
    gap += leftCorrection;
    if (gap) {
      aDesiredSize.mBoundingMetrics.leftBearing += gap;
      aDesiredSize.mBoundingMetrics.rightBearing += gap;
      aDesiredSize.mBoundingMetrics.width += gap;
      aDesiredSize.Width() += gap;
    }
    aDesiredSize.mBoundingMetrics.width += italicCorrection;
    aDesiredSize.Width() += italicCorrection;
  }
  return gap;
}

nscoord
nsMathMLContainerFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = 0;
  gap = AddInterFrameSpacingToSize(aDesiredSize, this);
  if (gap) {
    
    nsIFrame* childFrame = mFrames.FirstChild();
    while (childFrame) {
      childFrame->SetPosition(childFrame->GetPosition() + nsPoint(gap, 0));
      childFrame = childFrame->GetNextSibling();
    }
  }
  return gap;
}

 void
nsMathMLContainerFrame::DidReflowChildren(nsIFrame* aFirst, nsIFrame* aStop)

{
  if (MOZ_UNLIKELY(!aFirst))
    return;

  for (nsIFrame* frame = aFirst;
       frame != aStop;
       frame = frame->GetNextSibling()) {
    NS_ASSERTION(frame, "aStop isn't a sibling");
    if (frame->GetStateBits() & NS_FRAME_IN_REFLOW) {
      
      nsIFrame* grandchild = frame->GetFirstPrincipalChild();
      if (grandchild)
        DidReflowChildren(grandchild, nullptr);

      frame->DidReflow(frame->PresContext(), nullptr,
                       nsDidReflowStatus::FINISHED);
    }
  }
}



nsresult
nsMathMLContainerFrame::TransmitAutomaticDataForMrowLikeElement()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsIFrame *childFrame, *baseFrame;
  bool embellishedOpFound = false;
  nsEmbellishData embellishData;
  
  for (childFrame = GetFirstPrincipalChild();
       childFrame;
       childFrame = childFrame->GetNextSibling()) {
    nsIMathMLFrame* mathMLFrame = do_QueryFrame(childFrame);
    if (!mathMLFrame) break;
    if (!mathMLFrame->IsSpaceLike()) {
      if (embellishedOpFound) break;
      baseFrame = childFrame;
      GetEmbellishDataFrom(baseFrame, embellishData);
      if (!NS_MATHML_IS_EMBELLISH_OPERATOR(embellishData.flags)) break;
      embellishedOpFound = true;
    }
  }

  if (!childFrame) {
    
    
    if (!embellishedOpFound) {
      
      mPresentationData.flags |= NS_MATHML_SPACE_LIKE;
    } else {
      
      
      mPresentationData.baseFrame = baseFrame;
      mEmbellishData = embellishData;
    }
  }

  if (childFrame || !embellishedOpFound) {
    
    mPresentationData.baseFrame = nullptr;
    mEmbellishData.flags = 0;
    mEmbellishData.coreFrame = nullptr;
    mEmbellishData.direction = NS_STRETCH_DIRECTION_UNSUPPORTED;
    mEmbellishData.leadingSpace = 0;
    mEmbellishData.trailingSpace = 0;
  }

  if (childFrame || embellishedOpFound) {
    
    mPresentationData.flags &= ~NS_MATHML_SPACE_LIKE;
  }

  return NS_OK;
}

 void
nsMathMLContainerFrame::PropagateFrameFlagFor(nsIFrame* aFrame,
                                              nsFrameState  aFlags)
{
  if (!aFrame || !aFlags)
    return;

  aFrame->AddStateBits(aFlags);
  nsIFrame* childFrame = aFrame->GetFirstPrincipalChild();
  while (childFrame) {
    PropagateFrameFlagFor(childFrame, aFlags);
    childFrame = childFrame->GetNextSibling();
  }
}

nsresult
nsMathMLContainerFrame::ReportErrorToConsole(const char*       errorMsgId,
                                             const char16_t** aParams,
                                             uint32_t          aParamCount)
{
  return nsContentUtils::ReportToConsole(nsIScriptError::errorFlag,
                                         NS_LITERAL_CSTRING("Layout: MathML"), mContent->OwnerDoc(),
                                         nsContentUtils::eMATHML_PROPERTIES,
                                         errorMsgId, aParams, aParamCount);
}

nsresult
nsMathMLContainerFrame::ReportParseError(const char16_t* aAttribute,
                                         const char16_t* aValue)
{
  const char16_t* argv[] = 
    { aValue, aAttribute, mContent->Tag()->GetUTF16String() };
  return ReportErrorToConsole("AttributeParsingError", argv, 3);
}

nsresult
nsMathMLContainerFrame::ReportChildCountError()
{
  const char16_t* arg = mContent->Tag()->GetUTF16String();
  return ReportErrorToConsole("ChildCountIncorrect", &arg, 1);
}

nsresult
nsMathMLContainerFrame::ReportInvalidChildError(nsIAtom* aChildTag)
{
  const char16_t* argv[] =
    { aChildTag->GetUTF16String(), mContent->Tag()->GetUTF16String() };
  return ReportErrorToConsole("InvalidChild", argv, 2);
}



nsContainerFrame*
NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                           nsFrameState aFlags)
{
  nsMathMLmathBlockFrame* it = new (aPresShell) nsMathMLmathBlockFrame(aContext);
  it->SetFlags(aFlags);
  return it;
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmathBlockFrame)

NS_QUERYFRAME_HEAD(nsMathMLmathBlockFrame)
  NS_QUERYFRAME_ENTRY(nsMathMLmathBlockFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

nsContainerFrame*
NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmathInlineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmathInlineFrame)

NS_QUERYFRAME_HEAD(nsMathMLmathInlineFrame)
  NS_QUERYFRAME_ENTRY(nsIMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsInlineFrame)
