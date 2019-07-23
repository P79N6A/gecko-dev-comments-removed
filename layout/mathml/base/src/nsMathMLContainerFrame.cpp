







































#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsCSSAnonBoxes.h"
#include "nsUnitConversion.h"
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
#include "nsMathMLChar.h"
#include "nsMathMLContainerFrame.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsDisplayList.h"
#include "nsCSSFrameConstructor.h"

NS_DEFINE_CID(kInlineFrameCID, NS_INLINE_FRAME_CID);








NS_IMPL_ADDREF_INHERITED(nsMathMLContainerFrame, nsMathMLFrame)
NS_IMPL_RELEASE_INHERITED(nsMathMLContainerFrame, nsMathMLFrame)
NS_IMPL_QUERY_INTERFACE_INHERITED1(nsMathMLContainerFrame, nsHTMLContainerFrame, nsMathMLFrame)





nsresult
nsMathMLContainerFrame::ReflowError(nsIRenderingContext& aRenderingContext,
                                    nsHTMLReflowMetrics& aDesiredSize)
{
  nsresult rv;

  
  mEmbellishData.flags = 0;
  mPresentationData.flags = NS_MATHML_ERROR;

  
  
  aRenderingContext.SetFont(GetStyleFont()->mFont, nsnull);

  
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
  
  aCtx->SetFont(mFrame->GetStyleFont()->mFont, nsnull);

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







void
nsMathMLContainerFrame::GetReflowAndBoundingMetricsFor(nsIFrame*            aFrame,
                                                       nsHTMLReflowMetrics& aReflowMetrics,
                                                       nsBoundingMetrics&   aBoundingMetrics,
                                                       eMathMLFrameType*    aMathMLFrameType)
{
  NS_PRECONDITION(aFrame, "null arg");

  
  
  

  nsRect rect = aFrame->GetRect();
  aReflowMetrics.ascent  = rect.y;
  aReflowMetrics.width   = rect.width;
  aReflowMetrics.height  = rect.height;
  nscoord descent = aReflowMetrics.height - aReflowMetrics.ascent;

  if (aFrame->IsFrameOfType(nsIFrame::eMathML)) {
    nsIMathMLFrame* mathMLFrame;
    CallQueryInterface(aFrame, &mathMLFrame);
    if (mathMLFrame) {
      mathMLFrame->GetBoundingMetrics(aBoundingMetrics);
      if (aMathMLFrameType)
        *aMathMLFrameType = mathMLFrame->GetMathMLFrameType();

      return;
    }
  }

 
 aBoundingMetrics.descent = descent;
 aBoundingMetrics.ascent  = aReflowMetrics.ascent;
 aBoundingMetrics.width   = aReflowMetrics.width;
 aBoundingMetrics.rightBearing = aReflowMetrics.width;
 if (aMathMLFrameType)
   *aMathMLFrameType = eMathMLFrameType_UNKNOWN;
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
      
      nsRect rect = childFrame->GetRect();
      bmChild.ascent = rect.y;
      bmChild.descent = rect.x;
      bmChild.width = rect.width;
      bmChild.rightBearing = rect.width;
      bmChild.leftBearing = 0;

      nsIMathMLFrame* mathMLFrame;
      childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
      if (mathMLFrame) {
        nsEmbellishData embellishData;
        nsPresentationData presentationData;
        mathMLFrame->GetEmbellishData(embellishData);
        mathMLFrame->GetPresentationData(presentationData);
        if (NS_MATHML_IS_EMBELLISH_OPERATOR(embellishData.flags) &&
            embellishData.direction == aStretchDirection &&
            presentationData.baseFrame) {
          
          nsIMathMLFrame* mathMLchildFrame;
          presentationData.baseFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLchildFrame);
          if (mathMLchildFrame) {
            mathMLFrame = mathMLchildFrame;
          }
        }
        mathMLFrame->GetBoundingMetrics(bmChild);
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

    

    nsIFrame* childFrame = mPresentationData.baseFrame;
    if (childFrame) {
      nsIMathMLFrame* mathMLFrame;
      childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
      NS_ASSERTION(mathMLFrame, "Something is wrong somewhere");
      if (mathMLFrame) {
        PRBool stretchAll =
          NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags) ||
          NS_MATHML_WILL_STRETCH_ALL_CHILDREN_HORIZONTALLY(mPresentationData.flags);

        
        
        nsHTMLReflowMetrics childSize(aDesiredStretchSize);
        GetReflowAndBoundingMetricsFor(childFrame, childSize, childSize.mBoundingMetrics);

        
        
        
        
        
        
        
        
        
        
        
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

        
        childFrame->SetRect(nsRect(0, childSize.ascent,
                                   childSize.width, childSize.height));

        
        
        

        if (stretchAll) {

          nsStretchDirection stretchDir =
            NS_MATHML_WILL_STRETCH_ALL_CHILDREN_VERTICALLY(mPresentationData.flags) ?
              NS_STRETCH_DIRECTION_VERTICAL : NS_STRETCH_DIRECTION_HORIZONTAL;

          GetPreferredStretchSize(aRenderingContext, STRETCH_CONSIDER_EMBELLISHMENTS,
                                  stretchDir, containerSize);

          childFrame = mFrames.FirstChild();
          while (childFrame) {
            if (childFrame != mPresentationData.baseFrame) {
              childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
              if (mathMLFrame) {
                
                GetReflowAndBoundingMetricsFor(childFrame, 
                  childSize, childSize.mBoundingMetrics);
                
                mathMLFrame->Stretch(aRenderingContext, stretchDir,
                                     containerSize, childSize);
                
                childFrame->SetRect(nsRect(0, childSize.ascent,
                                           childSize.width, childSize.height));
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
          if (!dx) return NS_OK;

          mBoundingMetrics.leftBearing += dx;
          mBoundingMetrics.rightBearing += dx;
          aDesiredStretchSize.mBoundingMetrics.leftBearing += dx;
          aDesiredStretchSize.mBoundingMetrics.rightBearing += dx;

          childFrame = mFrames.FirstChild();
          while (childFrame) {
            childFrame->SetPosition(childFrame->GetPosition()
				    + nsPoint(dx, 0));
            childFrame = childFrame->GetNextSibling();
          }
        }
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

  if (!placeOrigin) {
    
    
    
    PRBool parentWillFireStretch = PR_FALSE;
    nsIMathMLFrame* mathMLFrame;
    mParent->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
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
  
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  
  FixInterFrameSpacing(aDesiredSize);

  return NS_OK;
}











 void
nsMathMLContainerFrame::PropagatePresentationDataFor(nsIFrame*       aFrame,
                                                     PRInt32         aScriptLevelIncrement,
                                                     PRUint32        aFlagsValues,
                                                     PRUint32        aFlagsToUpdate)
{
  if (!aFrame || (!aFlagsToUpdate && !aScriptLevelIncrement))
    return;
  nsIMathMLFrame* mathMLFrame;
  aFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
  if (mathMLFrame) {
    
    mathMLFrame->UpdatePresentationData(aScriptLevelIncrement, aFlagsValues,
                                        aFlagsToUpdate);
    
    
    mathMLFrame->UpdatePresentationDataFromChildAt(0, -1,
      aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
  }
  else {
    
    nsIFrame* childFrame = aFrame->GetFirstChild(nsnull);
    while (childFrame) {
      PropagatePresentationDataFor(childFrame,
        aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
      childFrame = childFrame->GetNextSibling();
    }
  }
}

 void
nsMathMLContainerFrame::PropagatePresentationDataFromChildAt(nsIFrame*       aParentFrame,
                                                             PRInt32         aFirstChildIndex,
                                                             PRInt32         aLastChildIndex,
                                                             PRInt32         aScriptLevelIncrement,
                                                             PRUint32        aFlagsValues,
                                                             PRUint32        aFlagsToUpdate)
{
  if (!aParentFrame || (!aFlagsToUpdate && !aScriptLevelIncrement))
    return;
  PRInt32 index = 0;
  nsIFrame* childFrame = aParentFrame->GetFirstChild(nsnull);
  while (childFrame) {
    if ((index >= aFirstChildIndex) &&
        ((aLastChildIndex <= 0) || ((aLastChildIndex > 0) &&
         (index <= aLastChildIndex)))) {
      PropagatePresentationDataFor(childFrame,
        aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
    }
    index++;
    childFrame = childFrame->GetNextSibling();
  }
}






 void
nsMathMLContainerFrame::PropagateScriptStyleFor(nsIFrame*       aFrame,
                                                PRInt32         aParentScriptLevel)
{
  if (!aFrame)
    return;
  nsIMathMLFrame* mathMLFrame;
  aFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
  if (mathMLFrame) {
    
    nsPresentationData presentationData;
    mathMLFrame->GetPresentationData(presentationData);
    PRInt32 gap = presentationData.scriptLevel - aParentScriptLevel;

    
    
    aParentScriptLevel = presentationData.scriptLevel;

    nsStyleContext* oldStyleContext = aFrame->GetStyleContext();
    nsStyleContext* parentContext = oldStyleContext->GetParent();

    nsIContent* content = aFrame->GetContent();
    if (!gap) {
      
      
      if (!aFrame->GetParent() || aFrame->GetParent()->GetContent() != content)
        content->UnsetAttr(kNameSpaceID_None, nsGkAtoms::MOZfontsize, PR_FALSE);
    }
    else {
      
      nscoord scriptminsize = aFrame->GetPresContext()->PointsToAppUnits(NS_MATHML_SCRIPTMINSIZE);
      float scriptsizemultiplier = NS_MATHML_SCRIPTSIZEMULTIPLIER;
#if 0
       
       
       
       

       
       GetAttribute(nsnull, presentationData.mstyle,
                        nsGkAtoms::scriptminsize_, fontsize);
       if (!fontsize.IsEmpty()) {
         nsCSSValue cssValue;
         if (ParseNumericValue(fontsize, cssValue)) {
           nsCSSUnit unit = cssValue.GetUnit();
           if (eCSSUnit_Number == unit)
             scriptminsize = nscoord(float(scriptminsize) * cssValue.GetFloatValue());
           else if (eCSSUnit_Percent == unit)
             scriptminsize = nscoord(float(scriptminsize) * cssValue.GetPercentValue());
           else if (eCSSUnit_Null != unit)
             scriptminsize = CalcLength(mStyleContext, cssValue);
         }
       }
#endif

      
      nsAutoString fontsize;
      if (0 > gap) { 
        if (gap < NS_MATHML_CSS_NEGATIVE_SCRIPTLEVEL_LIMIT)
          gap = NS_MATHML_CSS_NEGATIVE_SCRIPTLEVEL_LIMIT;
        gap = -gap;
        scriptsizemultiplier = 1.0f / scriptsizemultiplier;
        fontsize.AssignLiteral("-");
      }
      else { 
        if (gap > NS_MATHML_CSS_POSITIVE_SCRIPTLEVEL_LIMIT)
          gap = NS_MATHML_CSS_POSITIVE_SCRIPTLEVEL_LIMIT;
        fontsize.AssignLiteral("+");
      }
      fontsize.AppendInt(gap, 10);
      
      const nsStyleFont* font = parentContext->GetStyleFont();
      nscoord newFontSize = font->mFont.size;
      while (0 < gap--) {
        newFontSize = (nscoord)((float)(newFontSize) * scriptsizemultiplier);
      }
      if (newFontSize <= scriptminsize) {
        fontsize.AssignLiteral("scriptminsize");
      }

      
      content->SetAttr(kNameSpaceID_None, nsGkAtoms::MOZfontsize,
                       fontsize, PR_FALSE);
    }

    
    nsFrameManager *fm = aFrame->GetPresContext()->FrameManager();
    nsStyleChangeList changeList;
    fm->ComputeStyleChangeFor(aFrame, &changeList, NS_STYLE_HINT_NONE);
#ifdef DEBUG
    
    nsIFrame* parentFrame = aFrame->GetParent();
    fm->DebugVerifyStyleTree(parentFrame ? parentFrame : aFrame);
#endif
  }

  
  nsIFrame* childFrame = aFrame->GetFirstChild(nsnull);
  while (childFrame) {
    childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
    if (mathMLFrame) {
      
      
      mathMLFrame->ReResolveScriptStyle(aParentScriptLevel);
    }
    else {
      PropagateScriptStyleFor(childFrame, aParentScriptLevel);
    }
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



NS_IMETHODIMP
nsMathMLContainerFrame::Init(nsIContent*      aContent,
                             nsIFrame*        aParent,
                             nsIFrame*        aPrevInFlow)
{
  MapCommonAttributesIntoCSS(GetPresContext(), aContent);

  
  return nsHTMLContainerFrame::Init(aContent, aParent, aPrevInFlow);

  
  
}




 void
nsMathMLContainerFrame::RebuildAutomaticDataForChildren(nsIFrame* aParentFrame)
{
  
  
  
  
  nsIFrame* childFrame = aParentFrame->GetFirstChild(nsnull);
  while (childFrame) {
    nsIMathMLFrame* childMathMLFrame;
    childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&childMathMLFrame);
    if (childMathMLFrame) {
      childMathMLFrame->InheritAutomaticData(aParentFrame);
    }
    RebuildAutomaticDataForChildren(childFrame);
    childFrame = childFrame->GetNextSibling();
  }
  nsIMathMLFrame* mathMLFrame;
  aParentFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
  if (mathMLFrame) {
    mathMLFrame->TransmitAutomaticData();
  }
}

 nsresult
nsMathMLContainerFrame::ReLayoutChildren(nsIFrame* aParentFrame)
{
  if (!aParentFrame)
    return NS_OK;

  
  PRInt32 parentScriptLevel = 0;
  nsIFrame* frame = aParentFrame;
  while (1) {
     nsIFrame* parent = frame->GetParent();
     if (!parent || !parent->GetContent())
       break;

    
    nsIMathMLFrame* mathMLFrame;
    frame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
    if (mathMLFrame) {
      nsPresentationData parentData;
      mathMLFrame->GetPresentationData(parentData);
      parentScriptLevel = parentData.scriptLevel;
      break;
    }

    
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

  
  nsIFrame* childFrame = aParentFrame->GetFirstChild(nsnull);
  while (childFrame) {
    nsIMathMLFrame* mathMLFrame;
    childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
    if (mathMLFrame) {
      
      
      mathMLFrame->ReResolveScriptStyle(parentScriptLevel);
    }
    else {
      PropagateScriptStyleFor(childFrame, parentScriptLevel);
    }
    childFrame = childFrame->GetNextSibling();
  }

  
  nsIFrame* parent = frame->GetParent();
  NS_ASSERTION(parent, "No parent to pass the reflow request up to");
  if (!parent)
    return NS_OK;

  return frame->GetPresContext()->PresShell()->
           FrameNeedsReflow(frame, nsIPresShell::eStyleChange);
}





nsresult
nsMathMLContainerFrame::ChildListChanged(PRInt32 aModType)
{
  
  
  
  nsIFrame* frame = this;
  if (mEmbellishData.coreFrame) {
    nsIFrame* parent = mParent;
    nsEmbellishData embellishData;
    for ( ; parent; frame = parent, parent = parent->GetParent()) {
      frame->AddStateBits(NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN);
      GetEmbellishDataFrom(parent, embellishData);
      if (embellishData.coreFrame != mEmbellishData.coreFrame)
        break;
    }
  }
  return ReLayoutChildren(frame);
}

NS_IMETHODIMP
nsMathMLContainerFrame::AppendFrames(nsIAtom*        aListName,
                                     nsIFrame*       aFrameList)
{
  if (aListName) {
    return NS_ERROR_INVALID_ARG;
  }
  if (aFrameList) {
    mFrames.AppendFrames(this, aFrameList);
    return ChildListChanged(nsIDOMMutationEvent::ADDITION);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLContainerFrame::InsertFrames(nsIAtom*        aListName,
                                     nsIFrame*       aPrevFrame,
                                     nsIFrame*       aFrameList)
{
  if (aListName) {
    return NS_ERROR_INVALID_ARG;
  }
  if (aFrameList) {
    
    mFrames.InsertFrames(this, aPrevFrame, aFrameList);
    return ChildListChanged(nsIDOMMutationEvent::ADDITION);
  }
  return NS_OK;
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
  
  if (CommonAttributeChangedFor(GetPresContext(), mContent, aAttribute))
    return NS_OK;

  
  
  
  return GetPresContext()->PresShell()->
           FrameNeedsReflow(this, nsIPresShell::eStyleChange);
}

nsresult 
nsMathMLContainerFrame::ReflowChild(nsIFrame*                aChildFrame,
                                    nsPresContext*           aPresContext,
                                    nsHTMLReflowMetrics&     aDesiredSize,
                                    const nsHTMLReflowState& aReflowState,
                                    nsReflowStatus&          aStatus)
{
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.mBoundingMetrics.Clear();
  aDesiredSize.mFlags |= NS_REFLOW_CALC_BOUNDING_METRICS;

  
  
  
  
  
  
  
  
  

  nsInlineFrame* inlineFrame;
  aChildFrame->QueryInterface(kInlineFrameCID, (void**)&inlineFrame);
  if (!inlineFrame)
    return nsHTMLContainerFrame::
           ReflowChild(aChildFrame, aPresContext, aDesiredSize, aReflowState,
                       0, 0, NS_FRAME_NO_MOVE_FRAME, aStatus);

  
  return ReflowForeignChild(aChildFrame, aPresContext, aDesiredSize, aReflowState, aStatus);                       
}

nsresult 
nsMathMLContainerFrame::ReflowForeignChild(nsIFrame*                aChildFrame,
                                           nsPresContext*           aPresContext,
                                           nsHTMLReflowMetrics&     aDesiredSize,
                                           const nsHTMLReflowState& aReflowState,
                                           nsReflowStatus&          aStatus)
{
  nsAutoSpaceManager autoSpaceManager(NS_CONST_CAST(nsHTMLReflowState &, aReflowState));
  nsresult rv = autoSpaceManager.CreateSpaceManagerFor(aPresContext, this);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsSize availSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  nsLineLayout ll(aPresContext, aReflowState.mSpaceManager,
                  aReflowState.parentReflowState, nsnull);
  ll.BeginLineReflow(0, 0, availSize.width, availSize.height, PR_FALSE, PR_FALSE);
  PRBool pushedFrame;
  ll.ReflowFrame(aChildFrame, aStatus, &aDesiredSize, pushedFrame);
  NS_ASSERTION(!pushedFrame, "unexpected");
  ll.EndLineReflow();

  
  aDesiredSize.mBoundingMetrics.ascent = aDesiredSize.ascent;
  aDesiredSize.mBoundingMetrics.descent = aDesiredSize.height - aDesiredSize.ascent;
  aDesiredSize.mBoundingMetrics.width = aDesiredSize.width;
  aDesiredSize.mBoundingMetrics.rightBearing = aDesiredSize.width;

  
  

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLContainerFrame::Reflow(nsPresContext*          aPresContext,
                               nsHTMLReflowMetrics&     aDesiredSize,
                               const nsHTMLReflowState& aReflowState,
                               nsReflowStatus&          aStatus)
{
  nsresult rv;
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  aDesiredSize.mBoundingMetrics.Clear();

  
  
  

  nsReflowStatus childStatus;
  nsSize availSize(aReflowState.ComputedWidth(), aReflowState.mComputedHeight);
  nsHTMLReflowMetrics childDesiredSize(
                      aDesiredSize.mFlags | NS_REFLOW_CALC_BOUNDING_METRICS);
  nsIFrame* childFrame = mFrames.FirstChild();
  while (childFrame) {
    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       childFrame, availSize);
    rv = ReflowChild(childFrame, aPresContext, childDesiredSize,
                     childReflowState, childStatus);
    
    if (NS_FAILED(rv)) {
      
      DidReflowChildren(mFrames.FirstChild(), childFrame);
      return rv;
    }

    
    
    
    childFrame->SetRect(nsRect(0, childDesiredSize.ascent,
                               childDesiredSize.width, childDesiredSize.height));
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
      nsIMathMLFrame* mathMLFrame;
      childFrame->QueryInterface(NS_GET_IID(nsIMathMLFrame), (void**)&mathMLFrame);
      if (mathMLFrame) {
        
        GetReflowAndBoundingMetricsFor(childFrame,
          childDesiredSize, childDesiredSize.mBoundingMetrics);

        mathMLFrame->Stretch(*aReflowState.rendContext, stretchDir,
                             containerSize, childDesiredSize);
        
        childFrame->SetRect(nsRect(0, childDesiredSize.ascent,
                                   childDesiredSize.width, childDesiredSize.height));
      }
      childFrame = childFrame->GetNextSibling();
    }
  }

  
  
  FinalizeReflow(*aReflowState.rendContext, aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
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

NS_IMETHODIMP
nsMathMLContainerFrame::Place(nsIRenderingContext& aRenderingContext,
                              PRBool               aPlaceOrigin,
                              nsHTMLReflowMetrics& aDesiredSize)
{
  
  aDesiredSize.width = aDesiredSize.height = 0;
  aDesiredSize.ascent = 0;
  mBoundingMetrics.Clear();

  
  const nsStyleFont* font = GetStyleFont();
  nscoord thinSpace = NSToCoordRound(float(font->mFont.size)*float(3) / float(18));

  PRInt32 count = 0;
  PRInt32 carrySpace = 0;
  nsHTMLReflowMetrics childSize;
  nsBoundingMetrics bmChild;
  nscoord leftCorrection = 0, italicCorrection = 0;
  eMathMLFrameType fromFrameType = eMathMLFrameType_UNKNOWN;
  eMathMLFrameType prevFrameType = eMathMLFrameType_UNKNOWN;
  eMathMLFrameType childFrameType;

  nsIFrame* childFrame = mFrames.FirstChild();
  nscoord ascent = 0, descent = 0;
  while (childFrame) {
    GetReflowAndBoundingMetricsFor(childFrame, childSize, bmChild, &childFrameType);
    GetItalicCorrection(bmChild, leftCorrection, italicCorrection);
    if (0 == count) {
      ascent = childSize.ascent;
      descent = childSize.height - ascent;
      mBoundingMetrics = bmChild;
      
      

      if (mContent->Tag() == nsGkAtoms::msqrt_)
        leftCorrection = 0;
      else
        mBoundingMetrics.leftBearing += leftCorrection;
    }
    else {
      nscoord childDescent = childSize.height - childSize.ascent;
      if (descent < childDescent)
        descent = childDescent;
      if (ascent < childSize.ascent)
        ascent = childSize.ascent;
      
      nscoord space = GetInterFrameSpacing(mPresentationData.scriptLevel,
        prevFrameType, childFrameType, &fromFrameType, &carrySpace);
      mBoundingMetrics.width += space * thinSpace;
      
      mBoundingMetrics += bmChild;
    }
    count++;
    prevFrameType = childFrameType;
    
    
    mBoundingMetrics.width += leftCorrection;
    mBoundingMetrics.rightBearing += leftCorrection;
    
    
    
    
    mBoundingMetrics.width += italicCorrection;

    childFrame = childFrame->GetNextSibling();
  }
  aDesiredSize.width = mBoundingMetrics.width;
  aDesiredSize.height = ascent + descent;
  aDesiredSize.ascent = ascent;
  aDesiredSize.mBoundingMetrics = mBoundingMetrics;

  mReference.x = 0;
  mReference.y = aDesiredSize.ascent;

  
  

  if (aPlaceOrigin) {
    count = 0;
    nscoord dx = 0, dy = 0;
    italicCorrection = 0;
    carrySpace = 0;
    fromFrameType = eMathMLFrameType_UNKNOWN;
    childFrame = mFrames.FirstChild();
    while (childFrame) {
      GetReflowAndBoundingMetricsFor(childFrame, childSize, bmChild, &childFrameType);
      GetItalicCorrection(bmChild, leftCorrection, italicCorrection);
      dy = aDesiredSize.ascent - childSize.ascent;
      if (0 == count) {
        

        if (mContent->Tag() == nsGkAtoms::msqrt_)
          leftCorrection = 0;
      }
      else {
        
        nscoord space = GetInterFrameSpacing(mPresentationData.scriptLevel,
          prevFrameType, childFrameType, &fromFrameType, &carrySpace);
        dx += space * thinSpace;
      }
      count++;
      prevFrameType = childFrameType;
      
      dx += leftCorrection;
      FinishReflowChild(childFrame, GetPresContext(), nsnull, childSize,
                        dx, dy, 0);
      
      dx += bmChild.width + italicCorrection;
      childFrame = childFrame->GetNextSibling();
    }
  }

  return NS_OK;
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
      const nsStyleFont* font = parentContext->GetStyleFont();
      nscoord thinSpace = NSToCoordRound(float(font->mFont.size)*float(3) / float(18));
      
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
    gap = GetInterFrameSpacingFor(mPresentationData.scriptLevel, mParent, this);
    
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
      frame->DidReflow(frame->GetPresContext(), nsnull,
                       NS_FRAME_REFLOW_FINISHED);
    }
  }
}



nsIFrame*
NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmathBlockFrame(aContext);
}



nsIFrame*
NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMathMLmathInlineFrame(aContext);
}
