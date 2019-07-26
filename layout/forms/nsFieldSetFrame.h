




#ifndef nsFieldSetFrame_h___
#define nsFieldSetFrame_h___

#include "nsContainerFrame.h"

class nsFieldSetFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsFieldSetFrame(nsStyleContext* aContext);

  NS_HIDDEN_(nscoord)
    GetIntrinsicWidth(nsRenderingContext* aRenderingContext,
                      nsLayoutUtils::IntrinsicWidthType);
  virtual nscoord GetMinWidth(nsRenderingContext* aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext* aRenderingContext);
  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;
  virtual nscoord GetBaseline() const;

  



  virtual nsRect VisualBorderRectRelativeToSelf() const MOZ_OVERRIDE;

  virtual nsresult Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
                               
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  void PaintBorderBackground(nsRenderingContext& aRenderingContext,
    nsPoint aPt, const nsRect& aDirtyRect, uint32_t aBGFlags);

  virtual nsresult AppendFrames(ChildListID    aListID,
                          nsFrameList&   aFrameList);
  virtual nsresult InsertFrames(ChildListID    aListID,
                          nsIFrame*      aPrevFrame,
                          nsFrameList&   aFrameList);
  virtual nsresult RemoveFrame(ChildListID    aListID,
                         nsIFrame*      aOldFrame);

  virtual nsIAtom* GetType() const;
  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
             ~nsIFrame::eCanContainOverflowContainers);
  }
  virtual nsIScrollableFrame* GetScrollTargetFrame() MOZ_OVERRIDE
  {
    return do_QueryFrame(GetInner());
  }

#ifdef ACCESSIBILITY  
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

#ifdef DEBUG
  virtual nsresult SetInitialChildList(ChildListID    aListID,
                                 nsFrameList&   aChildList);
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("FieldSet"), aResult);
  }
#endif

  





  nsIFrame* GetInner() const;

  




  nsIFrame* GetLegend() const;

protected:
  nsRect    mLegendRect;
  nscoord   mLegendSpace;
};

#endif 
