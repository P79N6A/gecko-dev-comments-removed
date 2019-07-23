








































#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsCSSAnonBoxes.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsINameSpaceManager.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"

#include "nsIDOMText.h"
#include "nsIDOMMutationEvent.h"
#include "nsFrameManager.h"
#include "nsStyleChangeList.h"

#include "nsGkAtoms.h"
#include "nsMathMLParts.h"
#include "nsMathMLContainerFrame.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"
#include "nsCSSFrameConstructor.h"
#include "nsIReflowCallback.h"





NS_QUERYFRAME_HEAD(nsMathMLContainerFrame)
  NS_QUERYFRAME_ENTRY(nsMathMLFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)





nsresult
nsMathMLContainerFrame::ReflowError(nsIRenderingContext& aRenderingContext,
                                    nsHTMLReflowMetrics& aDesiredSize)
{
  nsresult rv;

  
  mEmbellishData.flags = 0;
  mPresentationData.flags = NS_MATHML_ERROR;

  
  
  nsLayoutUtils::SetFontFromStyle(&aRenderingContext, GetStyleContext());

  
  nsAutoString errorMsg; errorMsg.AssignLiteral("invalid-markup");
  rv = aRenderingContext.GetBoundingMetrics(errorMsg.get(),
                                            PRUint32(errorMsg.Length()),
                                            mBoundingMetrics);
  if (NS_FAILED(rv)) {
    NS_WARNING("GetBoundingMetrics failed");
    aDesiredSize.width = aDesiredSize.height = 0;
    aDesiredSize.ascent = 0;
    return NS_OK;
  }

  
  nsCOMPtr<nsIFontMetrics> fm;
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));
  fm->GetMaxAscent(aDesiredSize.ascent);
  nscoord descent;
  fm->GetMaxDescent(descent);
  aDesiredSize.height = aDesiredSize.ascent + descent;
  aDesiredSize.width = mBoundingMetrics.width;

  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  return NS_OK;
}

class nsDisplayMathMLError : public nsDisplayItem {
public:
  nsDisplayMathMLError(nsIFrame* aFrame)
    : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayMathMLError);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMathMLError() {
    MOZ_COUNT_DTOR(nsDisplayMathMLError);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("MathMLError")
};

void nsDisplayMathMLError::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  
  nsLayoutUtils::SetFontFromStyle(aCtx, mFrame->GetStyleContext());

  nsPoint pt = aBuilder->ToReferenceFrame(mFrame);
  aCtx->SetColor(NS_RGB(255,0,0));
  aCtx->FillRect(nsRect(pt, mFrame->GetSize()));
  aCtx->SetColor(NS_RGB(255,255,255));

  nscoord ascent;
  nsCOMPtr<nsIFontMetrics> fm;
  aCtx->GetFontMetrics(*getter_AddRefs(fm));
  fm->GetMaxAscent(ascent);

  nsAutoString errorMsg; errorMsg.AssignLiteral("invalid-markup");
  aCtx->DrawString(errorMsg.get(), PRUint32(errorMsg.Length()), pt.x, pt.y+ascent);
}






static PRBool
IsForeignChild(const nsIFrame* aFrame)
{
  
  
  return !(aFrame->IsFrameOfType(nsIFrame::eMathML)) ||
    aFrame->GetType() == nsGkAtoms::blockFrame;
}

static void
DeleteHTMLReflowMetrics(void *aObject, nsIAtom *aPropertyName,
                        void *aPropertyValue, void *aData)
{
  delete static_cast<nsHTMLReflowMetrics*>(aPropertyValue);
}

 void
nsMathMLContainerFrame::SaveReflowAndBoundingMetricsFor(nsIFrame*                  aFrame,
                                                        const nsHTMLReflowMetrics& aReflowMetrics,
                                                        const nsBoundingMetrics&   aBoundingMetrics)
{
  nsHTMLReflowMetrics *metrics = new nsHTMLReflowMetrics(aReflowMetrics);
  metrics->mBoundingMetrics = aBoundingMetrics;
  aFrame->SetProperty(nsGkAtoms::HTMLReflowMetricsProperty, metrics,
                      DeleteHTMLReflowMetrics);
}


 void
nsMathMLContainerFrame::GetReflowAndBoundingMetricsFor(nsIFrame*            aFrame,
                                                       nsHTMLReflowMetrics& aReflowMetrics,
                                                       nsBoundingMetrics&   aBoundingMetrics,
                                                       eMathMLFrameType*    aMathMLFrameType)
{
  NS_PRECONDITION(aFrame, "null arg");

  nsHTMLReflowMetrics *metrics = static_cast<nsHTMLReflowMetrics*>
    (aFrame->GetProperty(nsGkAtoms::HTMLReflowMetricsProperty));

  
  
  
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
  while (childFrame) {
    childFrame->DeleteProperty(nsGkAtoms::HTMLReflowMetricsProperty);
    childFrame = childFrame->GetNextSibling();
  }
}



void
nsMathMLContainerFrame::GetPreferredStretchSize(nsIRenderingContext& aRenderingContext,
                                                PRUint32             aOptions,
                                                nsStretchDirection   aStretchDirection,
                                                nsBoundingMetrics&   aPreferredStretchSize)
{
  if (aOptions & STRETCH_CONSIDER_ACTUAL_SIZE) {
    
    aPreferredStretchSize = mBoundingMetrics;
  }
  else if (aOptions & STRETCH_CONSIDER_EMBELLISHMENTS) {
    
    nsHTMLReflowMetrics metrics;
    Place(aRenderingContext, PR_FALSE, metrics);
    aPreferredStretchSize = metrics.mBoundingMetrics;
  }
  else {
    
    NS_ASSERTION(NS_MATHML_IS_EMBELLISH_OPERATOR(mEmbellishData.flags) ||
                 NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags) ||
                 NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags),
                 "invalid call to GetPreferredStretchSize");
    PRBool firstTime = PR_TRUE;
    nsBoundingMetrics bm, bmChild;
    
    nsIFrame* childFrame = GetFirstChild(nsnull);
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
        nsHTMLReflowMetrics unused;
        GetReflowAndBoundingMetricsFor(childFrame, unused, bmChild);
      }

      if (firstTime) {
        firstTime = PR_FALSE;
        bm = bmChild;
        if (!NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags) &&
            !NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags)) {
          
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
nsMathMLContainerFrame::Stretch(nsIRenderingContext& aRenderingContext,
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
        PRBool stretchAll =
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

        
        nsresult rv = Place(aRenderingContext, PR_TRUE, aDesiredStretchSize);
        if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
          
          DidReflowChildren(mFrames.FirstChild());
        }

        
        
        

        nsEmbellishData parentData;
        GetEmbellishDataFrom(mParent, parentData);
        
        
        if (parentData.coreFrame != mEmbellishData.coreFrame) {
          
          
          
          nsEmbellishData coreData;
          GetEmbellishDataFrom(mEmbellishData.coreFrame, coreData);

          mBoundingMetrics.width += coreData.leftSpace + coreData.rightSpace;
          aDesiredStretchSize.width = mBoundingMetrics.width;
          aDesiredStretchSize.mBoundingMetrics.width = mBoundingMetrics.width;

          nscoord dx = coreData.leftSpace;
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
nsMathMLContainerFrame::FinalizeReflow(nsIRenderingContext& aRenderingContext,
                                       nsHTMLReflowMetrics& aDesiredSize)
{
  
  
  
  
  
  
  
  
  


  
  
  
  

  
  
  
  
  
  PRBool placeOrigin = !NS_MATHML_IS_EMBELLISH_OPERATOR(mEmbellishData.flags) ||
                       (mEmbellishData.coreFrame != this && !mPresentationData.baseFrame &&
                        mEmbellishData.direction == NS_STRETCH_DIRECTION_UNSUPPORTED);
  nsresult rv = Place(aRenderingContext, placeOrigin, aDesiredSize);

  
  
  
  
  
  
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
    DidReflowChildren(GetFirstChild(nsnull));
    return rv;
  }

  PRBool parentWillFireStretch = PR_FALSE;
  if (!placeOrigin) {
    
    
    
    nsIMathMLFrame* mathMLFrame = do_QueryFrame(mParent);
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
        parentWillFireStretch = PR_TRUE;
      }
    }
    if (!parentWillFireStretch) {
      

      PRBool stretchAll =
        
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
#ifdef NS_DEBUG
      {
        
        
        nsIFrame* childFrame = GetFirstChild(nsnull);
        for ( ; childFrame; childFrame = childFrame->GetNextSibling()) {
          NS_ASSERTION(!(childFrame->GetStateBits() & NS_FRAME_IN_REFLOW),
                       "DidReflow() was never called");
        }
      }
#endif
    }
  }

  
  FixInterFrameSpacing(aDesiredSize);

  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  if (!parentWillFireStretch) {
    
    
    ClearSavedChildMetrics();
    
    GatherAndStoreOverflow(&aDesiredSize);
  }

  return NS_OK;
}











 void
nsMathMLContainerFrame::PropagatePresentationDataFor(nsIFrame*       aFrame,
                                                     PRUint32        aFlagsValues,
                                                     PRUint32        aFlagsToUpdate)
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
    
    nsIFrame* childFrame = aFrame->GetFirstChild(nsnull);
    while (childFrame) {
      PropagatePresentationDataFor(childFrame,
        aFlagsValues, aFlagsToUpdate);
      childFrame = childFrame->GetNextSibling();
    }
  }
}

 void
nsMathMLContainerFrame::PropagatePresentationDataFromChildAt(nsIFrame*       aParentFrame,
                                                             PRInt32         aFirstChildIndex,
                                                             PRInt32         aLastChildIndex,
                                                             PRUint32        aFlagsValues,
                                                             PRUint32        aFlagsToUpdate)
{
  if (!aParentFrame || !aFlagsToUpdate)
    return;
  PRInt32 index = 0;
  nsIFrame* childFrame = aParentFrame->GetFirstChild(nsnull);
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







NS_IMETHODIMP
nsMathMLContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags)) {
    if (!IsVisibleForPainting(aBuilder))
      return NS_OK;

    return aLists.Content()->AppendNewToTop(new (aBuilder) nsDisplayMathMLError(this));
  }

  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DisplayTextDecorationsAndChildren(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
  
  
  
  
  
  
  rv = DisplayBoundingMetrics(aBuilder, this, mReference, mBoundingMetrics, aLists);
#endif
  return rv;
}




 void
nsMathMLContainerFrame::RebuildAutomaticDataForChildren(nsIFrame* aParentFrame)
{
  
  
  
  
  nsIFrame* childFrame = aParentFrame->GetFirstChild(nsnull);
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
    
    if (content->Tag() == nsGkAtoms::math)
      break;

    
    
    
    
    
    frame->AddStateBits(NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN);

    frame = parent;
  }

  
  RebuildAutomaticDataForChildren(frame);

  
  nsIFrame* parent = frame->GetParent();
  NS_ASSERTION(parent, "No parent to pass the reflow request up to");
  if (!parent)
    return NS_OK;

  return frame->PresContext()->PresShell()->
    FrameNeedsReflow(frame, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
}





nsresult
nsMathMLContainerFrame::ChildListChanged(PRInt32 aModType)
{
  
  
  
  nsIFrame* frame = this;
  if (mEmbellishData.coreFrame) {
    nsIFrame* parent = mParent;
    nsEmbellishData embellishData;
    for ( ; parent; frame = parent, parent = parent->GetParent()) {
      GetEmbellishDataFrom(parent, embellishData);
      if (embellishData.coreFrame != mEmbellishData.coreFrame)
        break;

      
      
      
      
      
      frame->AddStateBits(NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
  return ReLayoutChildren(frame);
}

NS_IMETHODIMP
nsMathMLContainerFrame::AppendFrames(nsIAtom*        aListName,
                                     nsFrameList&    aFrameList)
{
  if (aListName) {
    return NS_ERROR_INVALID_ARG;
  }
  mFrames.AppendFrames(this, aFrameList);
  return ChildListChanged(nsIDOMMutationEvent::ADDITION);
}

NS_IMETHODIMP
nsMathMLContainerFrame::InsertFrames(nsIAtom*        aListName,
                                     nsIFrame*       aPrevFrame,
                                     nsFrameList&    aFrameList)
{
  if (aListName) {
    return NS_ERROR_INVALID_ARG;
  }
  
  mFrames.InsertFrames(this, aPrevFrame, aFrameList);
  return ChildListChanged(nsIDOMMutationEvent::ADDITION);
}

NS_IMETHODIMP
nsMathMLContainerFrame::RemoveFrame(nsIAtom*        aListName,
                                    nsIFrame*       aOldFrame)
{
  if (aListName) {
    return NS_ERROR_INVALID_ARG;
  }
  
  mFrames.DestroyFrame(aOldFrame);
  return ChildListChanged(nsIDOMMutationEvent::REMOVAL);
}

NS_IMETHODIMP
nsMathMLContainerFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                         nsIAtom*        aAttribute,
                                         PRInt32         aModType)
{
  
  
  
  return PresContext()->PresShell()->
           FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                            NS_FRAME_IS_DIRTY);
}

void
nsMathMLContainerFrame::GatherAndStoreOverflow(nsHTMLReflowMetrics* aMetrics)
{
  
  
  nsRect frameRect(0, 0, aMetrics->width, aMetrics->height);

  
  if (PresContext()->CompatibilityMode() != eCompatibility_NavQuirks) {
    nsRect shadowRect = nsLayoutUtils::GetTextShadowRectsUnion(frameRect, this);
    frameRect.UnionRect(frameRect, shadowRect);
  }

  
  
  nsRect boundingBox(mBoundingMetrics.leftBearing,
                     aMetrics->ascent - mBoundingMetrics.ascent,
                     mBoundingMetrics.rightBearing - mBoundingMetrics.leftBearing,
                     mBoundingMetrics.ascent + mBoundingMetrics.descent);

  aMetrics->mOverflowArea.UnionRect(frameRect, boundingBox);

  
  
  
  
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    ConsiderChildOverflow(aMetrics->mOverflowArea, childFrame);
    childFrame = childFrame->GetNextSibling();
  }

  FinishAndStoreOverflow(aMetrics);
}

nsresult 
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
  
  nsresult rv = nsHTMLContainerFrame::
         ReflowChild(aChildFrame, aPresContext, aDesiredSize, aReflowState,
                     0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);

  if (NS_FAILED(rv))
    return rv;

  if (aDesiredSize.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    
    if(!nsLayoutUtils::GetLastLineBaseline(aChildFrame,
                                           &aDesiredSize.ascent)) {
      
      
      
      aDesiredSize.ascent = aDesiredSize.height;
    }
  }
  if (IsForeignChild(aChildFrame)) {
    
    nsRect r = aChildFrame->ComputeTightBounds(aReflowState.rendContext->ThebesContext());
    aDesiredSize.mBoundingMetrics.leftBearing = r.x;
    aDesiredSize.mBoundingMetrics.rightBearing = r.XMost();
    aDesiredSize.mBoundingMetrics.ascent = aDesiredSize.ascent - r.y;
    aDesiredSize.mBoundingMetrics.descent = r.YMost() - aDesiredSize.ascent;
    aDesiredSize.mBoundingMetrics.width = aDesiredSize.width;
  }
  return rv;
}

NS_IMETHODIMP
nsMathMLContainerFrame::Reflow(nsPresContext*           aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.mBoundingMetrics.Clear();

  
  
  

  nsReflowStatus childStatus;
  nsSize availSize(aReflowState.ComputedWidth(), NS_UNCONSTRAINEDSIZE);
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    nsHTMLReflowMetrics childDesiredSize(aDesiredSize.mFlags);
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    nsresult rv = ReflowChild(childFrame, aPresContext, childDesiredSize,
                              childReflowState, childStatus);
    
    if (NS_FAILED(rv)) {
      
      DidReflowChildren(mFrames.FirstChild(), childFrame);
      return rv;
    }

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
        
        nsHTMLReflowMetrics childDesiredSize;
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
  return NS_OK;
}

 nscoord
nsMathMLContainerFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  result = GetIntrinsicWidth(aRenderingContext);
  return result;
}

 nscoord
nsMathMLContainerFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  result = GetIntrinsicWidth(aRenderingContext);
  return result;
}

 nscoord
nsMathMLContainerFrame::GetIntrinsicWidth(nsIRenderingContext* aRenderingContext)
{
  
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    
    
    
    nscoord width =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, childFrame,
                                           nsLayoutUtils::PREF_WIDTH);

    nsHTMLReflowMetrics childDesiredSize;
    childDesiredSize.width = width;
    childDesiredSize.mBoundingMetrics.width = width;
    
    childDesiredSize.mBoundingMetrics.leftBearing = 0;
    childDesiredSize.mBoundingMetrics.rightBearing = width;

    SaveReflowAndBoundingMetricsFor(childFrame, childDesiredSize,
                                    childDesiredSize.mBoundingMetrics);

    childFrame = childFrame->GetNextSibling();
  }

  
  nsHTMLReflowMetrics desiredSize;
  nsresult rv = MeasureForWidth(*aRenderingContext, desiredSize);
  if (NS_FAILED(rv)) {
    ReflowError(*aRenderingContext, desiredSize);
  }

  ClearSavedChildMetrics();

  return desiredSize.width;
}

 nsresult
nsMathMLContainerFrame::MeasureForWidth(nsIRenderingContext& aRenderingContext,
                                        nsHTMLReflowMetrics& aDesiredSize)
{
  return Place(aRenderingContext, PR_FALSE, aDesiredSize);
}





static PRInt32 kInterFrameSpacingTable[eMathMLFrameType_COUNT][eMathMLFrameType_COUNT] =
{
  
  
  

  
     {0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
     {0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01},
     {0x01, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01},
    {0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01},
   {0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01}
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
GetInterFrameSpacing(PRInt32           aScriptLevel,
                     eMathMLFrameType  aFirstFrameType,
                     eMathMLFrameType  aSecondFrameType,
                     eMathMLFrameType* aFromFrameType, 
                     PRInt32*          aCarrySpace)    
{
  eMathMLFrameType firstType = aFirstFrameType;
  eMathMLFrameType secondType = aSecondFrameType;

  PRInt32 space;
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
    mChildFrame(aParentFrame->mFrames.FirstChild()),
    mX(0),
    mCarrySpace(0),
    mFromFrameType(eMathMLFrameType_UNKNOWN)
  {
    if (!mChildFrame)
      return;

    InitMetricsForChild();
    
    
    if (mParentFrame->GetContent()->Tag() == nsGkAtoms::msqrt_) {
      mX = 0;
    }
  }

  RowChildFrameIterator& operator++()
  {
    
    mX += mSize.mBoundingMetrics.width + mItalicCorrection;

    mChildFrame = mChildFrame->GetNextSibling();
    if (!mChildFrame)
      return *this;

    eMathMLFrameType prevFrameType = mChildFrameType;
    InitMetricsForChild();

    
    const nsStyleFont* font = mParentFrame->GetStyleFont();
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
  nscoord Ascent() const { return mSize.ascent; }
  nscoord Descent() const { return mSize.height - mSize.ascent; }
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
  PRInt32 mCarrySpace;
  eMathMLFrameType mFromFrameType;

  void InitMetricsForChild()
  {
    GetReflowAndBoundingMetricsFor(mChildFrame, mSize, mSize.mBoundingMetrics,
                                   &mChildFrameType);
    nscoord leftCorrection;
    GetItalicCorrection(mSize.mBoundingMetrics, leftCorrection,
                        mItalicCorrection);
    
    
    mX += leftCorrection;
  }
};

 nsresult
nsMathMLContainerFrame::Place(nsIRenderingContext& aRenderingContext,
                              PRBool               aPlaceOrigin,
                              nsHTMLReflowMetrics& aDesiredSize)
{
  
  mBoundingMetrics.Clear();

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

  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.height = ascent + descent;
  aDesiredSize.ascent = ascent;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  
  

  if (aPlaceOrigin) {
    PositionRowChildFrames(0, aDesiredSize.ascent);
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
    FinishReflowChild(child.Frame(), PresContext(), nsnull,
                      child.ReflowMetrics(), dx, dy, 0);
    ++child;
  }
}

class ForceReflow : public nsIReflowCallback {
public:
  virtual PRBool ReflowFinished() {
    return PR_TRUE;
  }
  virtual void ReflowCallbackCanceled() {}
};



static ForceReflow gForceReflow;

void
nsMathMLContainerFrame::SetIncrementScriptLevel(PRInt32 aChildIndex, PRBool aIncrement)
{
  nsIFrame* child = nsFrameList(GetFirstChild(nsnull)).FrameAt(aChildIndex);
  if (!child)
    return;
  nsIContent* content = child->GetContent();
  if (!content->IsNodeOfType(nsINode::eMATHML))
    return;
  nsMathMLElement* element = static_cast<nsMathMLElement*>(content);

  if (element->GetIncrementScriptLevel() == aIncrement)
    return;

  
  
  element->SetIncrementScriptLevel(aIncrement, PR_TRUE);
  PresContext()->PresShell()->PostReflowCallback(&gForceReflow);
}




static nscoord
GetInterFrameSpacingFor(PRInt32         aScriptLevel,
                        nsIFrame*       aParentFrame,
                        nsIFrame*       aChildFrame)
{
  nsIFrame* childFrame = aParentFrame->GetFirstChild(nsnull);
  if (!childFrame || aChildFrame == childFrame)
    return 0;

  PRInt32 carrySpace = 0;
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
      
      nsStyleContext* parentContext = aParentFrame->GetStyleContext();
      nscoord thinSpace = GetThinSpace(parentContext->GetStyleFont());
      
      return space * thinSpace;
    }
    childFrame = childFrame->GetNextSibling();
  }

  NS_NOTREACHED("child not in the childlist of its parent");
  return 0;
}

nscoord
nsMathMLContainerFrame::FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize)
{
  nscoord gap = 0;
  nsIContent* parentContent = mParent->GetContent();
  if (NS_UNLIKELY(!parentContent)) {
    return 0;
  }
  
  nsIAtom *parentTag = parentContent->Tag();
  if (parentTag == nsGkAtoms::math ||
      parentTag == nsGkAtoms::mtd_) {
    gap = GetInterFrameSpacingFor(GetStyleFont()->mScriptLevel, mParent, this);
    
    nscoord leftCorrection = 0, italicCorrection = 0;
    GetItalicCorrection(mBoundingMetrics, leftCorrection, italicCorrection);
    gap += leftCorrection;
    
    if (gap) {
      nsIFrame* childFrame = mFrames.FirstChild();
      while (childFrame) {
        childFrame->SetPosition(childFrame->GetPosition() + nsPoint(gap, 0));
        childFrame = childFrame->GetNextSibling();
      }
      mBoundingMetrics.leftBearing += gap;
      mBoundingMetrics.rightBearing += gap;
      mBoundingMetrics.width += gap;
      aDesiredSize.width += gap;
    }
    mBoundingMetrics.width += italicCorrection;
    aDesiredSize.width += italicCorrection;
  }
  return gap;
}

 void
nsMathMLContainerFrame::DidReflowChildren(nsIFrame* aFirst, nsIFrame* aStop)

{
  if (NS_UNLIKELY(!aFirst))
    return;

  for (nsIFrame* frame = aFirst;
       frame != aStop;
       frame = frame->GetNextSibling()) {
    NS_ASSERTION(frame, "aStop isn't a sibling");
    if (frame->GetStateBits() & NS_FRAME_IN_REFLOW) {
      
      nsIFrame* grandchild = frame->GetFirstChild(nsnull);
      if (grandchild)
        DidReflowChildren(grandchild, nsnull);

      frame->DidReflow(frame->PresContext(), nsnull,
                       NS_FRAME_REFLOW_FINISHED);
    }
  }
}



nsIFrame*
NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                           PRUint32 aFlags)
{
  nsMathMLmathBlockFrame* it = new (aPresShell) nsMathMLmathBlockFrame(aContext);
  if (it) {
    it->SetFlags(aFlags);
  }
  return it;
}

nsIFrame*
NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmathInlineFrame(aContext);
}
