






































#ifndef nsCanvasFrame_h___
#define nsCanvasFrame_h___


#include "nsHTMLContainerFrame.h"
#include "nsStyleContext.h"
#include "nsRenderingContext.h"
#include "nsGUIEvent.h"
#include "nsGkAtoms.h"
#include "nsIScrollPositionListener.h"
#include "nsDisplayList.h"
#include "nsAbsoluteContainingBlock.h"

class nsPresContext;








class nsCanvasFrame : public nsHTMLContainerFrame, 
                      public nsIScrollPositionListener
{
public:
  nsCanvasFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext),
    mDoPaintFocus(PR_FALSE),
    mAddedScrollPositionListener(PR_FALSE),
    mAbsoluteContainer(nsGkAtoms::absoluteList) {}

  NS_DECL_QUERYFRAME_TARGET(nsCanvasFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS


  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;
  virtual nsFrameList GetChildList(nsIAtom* aListName) const;

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  virtual PRBool IsContainingBlock() const { return PR_TRUE; }
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsHTMLContainerFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers));
  }

  


  NS_IMETHOD SetHasFocus(PRBool aHasFocus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintFocus(nsRenderingContext& aRenderingContext, nsPoint aPt);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY);
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY) {}

  




  virtual nsIAtom* GetType() const;

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              PRBool         aForceNormal)
  {
    NS_ASSERTION(!aForceNormal, "No-one should be passing this in here");

    
    
    nsresult rv = nsContainerFrame::StealFrame(aPresContext, aChild, PR_TRUE);
    if (NS_FAILED(rv)) {
      rv = nsContainerFrame::StealFrame(aPresContext, aChild);
    }
    return rv;
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  NS_IMETHOD GetContentForEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent** aContent);

  nsRect CanvasArea() const;

protected:
  virtual PRIntn GetSkipSides() const;

  
  PRPackedBool              mDoPaintFocus;
  PRPackedBool              mAddedScrollPositionListener;
  nsAbsoluteContainingBlock mAbsoluteContainer;
};







class nsDisplayCanvasBackground : public nsDisplayBackground {
public:
  nsDisplayCanvasBackground(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame)
    : nsDisplayBackground(aBuilder, aFrame)
  {
    mExtraBackgroundColor = NS_RGBA(0,0,0,0);
  }

  virtual PRBool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion,
                                   PRBool& aContainsRootContentDocBG)
  {
    PRBool retval = NS_GET_A(mExtraBackgroundColor) > 0 ||
           nsDisplayBackground::ComputeVisibility(aBuilder, aVisibleRegion,
                                                  aAllowVisibleRegionExpansion,
                                                  aContainsRootContentDocBG);
    if (retval && mFrame->PresContext()->IsRootContentDocument()) {
      aContainsRootContentDocBG = PR_TRUE;
    }
    return retval;
  }
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   PRBool* aForceTransparentSurface = nsnull)
  {
    if (aForceTransparentSurface) {
      *aForceTransparentSurface = PR_FALSE;
    }
    if (NS_GET_A(mExtraBackgroundColor) == 255)
      return nsRegion(GetBounds(aBuilder));
    return nsDisplayBackground::GetOpaqueRegion(aBuilder);
  }
  virtual PRBool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor)
  {
    nscolor background;
    if (!nsDisplayBackground::IsUniform(aBuilder, &background))
      return PR_FALSE;
    NS_ASSERTION(background == NS_RGBA(0,0,0,0),
                 "The nsDisplayBackground for a canvas frame doesn't paint "
                 "its background color normally");
    *aColor = mExtraBackgroundColor;
    return PR_TRUE;
  }
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder)
  {
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    nsRect r = frame->CanvasArea() + ToReferenceFrame();
    if (mSnappingEnabled) {
      nscoord appUnitsPerDevPixel = frame->PresContext()->AppUnitsPerDevPixel();
      r = r.ToNearestPixels(appUnitsPerDevPixel).ToAppUnits(appUnitsPerDevPixel);
    }
    return r;
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
  {
    
    aOutFrames->AppendElement(mFrame);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);

  void SetExtraBackgroundColor(nscolor aColor)
  {
    mExtraBackgroundColor = aColor;
  }

  NS_DISPLAY_DECL_NAME("CanvasBackground", TYPE_CANVAS_BACKGROUND)

private:
  nscolor mExtraBackgroundColor;
};


#endif 
