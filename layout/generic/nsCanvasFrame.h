






#ifndef nsCanvasFrame_h___
#define nsCanvasFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIScrollPositionListener.h"
#include "nsDisplayList.h"
#include "nsGkAtoms.h"

class nsPresContext;
class nsRenderingContext;
class nsEvent;








class nsCanvasFrame : public nsContainerFrame,
                      public nsIScrollPositionListener
{
public:
  nsCanvasFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext),
    mDoPaintFocus(false),
    mAddedScrollPositionListener(false) {}

  NS_DECL_QUERYFRAME_TARGET(nsCanvasFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS


  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;
  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;
  NS_IMETHOD RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers));
  }

  


  NS_IMETHOD SetHasFocus(bool aHasFocus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  void PaintFocus(nsRenderingContext& aRenderingContext, nsPoint aPt);

  
  virtual void ScrollPositionWillChange(nscoord aX, nscoord aY);
  virtual void ScrollPositionDidChange(nscoord aX, nscoord aY) {}

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              bool           aForceNormal) MOZ_OVERRIDE
  {
    NS_ASSERTION(!aForceNormal, "No-one should be passing this in here");

    
    
    nsresult rv = nsContainerFrame::StealFrame(aPresContext, aChild, true);
    if (NS_FAILED(rv)) {
      rv = nsContainerFrame::StealFrame(aPresContext, aChild);
    }
    return rv;
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  NS_IMETHOD GetContentForEvent(nsEvent* aEvent,
                                nsIContent** aContent) MOZ_OVERRIDE;

  nsRect CanvasArea() const;

protected:
  virtual int GetSkipSides() const;

  
  bool                      mDoPaintFocus;
  bool                      mAddedScrollPositionListener;
};







class nsDisplayCanvasBackground : public nsDisplayBackground {
public:
  nsDisplayCanvasBackground(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                            uint32_t aLayer, bool aIsThemed,
                            const nsStyleBackground* aBackgroundStyle)
    : nsDisplayBackground(aBuilder, aFrame, aLayer, aIsThemed, aBackgroundStyle),
      mExtraBackgroundColor(NS_RGBA(0,0,0,0))
  {
  }

  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion,
                                 const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE
  {
    return NS_GET_A(mExtraBackgroundColor) > 0 ||
      nsDisplayBackground::ComputeVisibility(aBuilder, aVisibleRegion,
                                             aAllowVisibleRegionExpansion);
  }
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE
  {
    if (NS_GET_A(mExtraBackgroundColor) == 255) {
      return nsRegion(GetBounds(aBuilder, aSnap));
    }
    return nsDisplayBackground::GetOpaqueRegion(aBuilder, aSnap);
  }
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE
  {
    nscolor background;
    if (!nsDisplayBackground::IsUniform(aBuilder, &background))
      return false;
    NS_ASSERTION(background == NS_RGBA(0,0,0,0),
                 "The nsDisplayBackground for a canvas frame doesn't paint "
                 "its background color normally");
    *aColor = mExtraBackgroundColor;
    return true;
  }
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    *aSnap = true;
    return frame->CanvasArea() + ToReferenceFrame();
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE
  {
    
    aOutFrames->AppendElement(mFrame);
  }
  virtual bool ShouldFixToViewport(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    
    
    
    
    return mBackgroundStyle &&
      mBackgroundStyle->mLayers[mLayer].mAttachment == NS_STYLE_BG_ATTACHMENT_FIXED &&
      !mBackgroundStyle->mLayers[mLayer].mImage.IsEmpty();
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;

  void SetExtraBackgroundColor(nscolor aColor)
  {
    mExtraBackgroundColor = aColor;
  }

  NS_DISPLAY_DECL_NAME("CanvasBackground", TYPE_CANVAS_BACKGROUND)

private:
  nscolor mExtraBackgroundColor;
};

#endif 
