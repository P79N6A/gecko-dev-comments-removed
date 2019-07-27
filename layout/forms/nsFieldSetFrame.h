




#ifndef nsFieldSetFrame_h___
#define nsFieldSetFrame_h___

#include "mozilla/Attributes.h"
#include "imgIContainer.h"
#include "nsContainerFrame.h"

class nsFieldSetFrame final : public nsContainerFrame
{
  typedef mozilla::image::DrawResult DrawResult;

public:
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsFieldSetFrame(nsStyleContext* aContext);

  nscoord
    GetIntrinsicISize(nsRenderingContext* aRenderingContext,
                      nsLayoutUtils::IntrinsicISizeType);
  virtual nscoord GetMinISize(nsRenderingContext* aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext* aRenderingContext) override;
  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              ComputeSizeFlags aFlags) override;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const override;

  



  virtual nsRect VisualBorderRectRelativeToSelf() const override;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;
                               
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  DrawResult PaintBorderBackground(nsRenderingContext& aRenderingContext,
                                   nsPoint aPt, const nsRect& aDirtyRect,
                                   uint32_t aBGFlags);

#ifdef DEBUG
  virtual void SetInitialChildList(ChildListID    aListID,
                                   nsFrameList&   aChildList) override;
  virtual void AppendFrames(ChildListID    aListID,
                            nsFrameList&   aFrameList) override;
  virtual void InsertFrames(ChildListID    aListID,
                            nsIFrame*      aPrevFrame,
                            nsFrameList&   aFrameList) override;
  virtual void RemoveFrame(ChildListID    aListID,
                           nsIFrame*      aOldFrame) override;
#endif

  virtual nsIAtom* GetType() const override;
  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
             ~nsIFrame::eCanContainOverflowContainers);
  }
  virtual nsIScrollableFrame* GetScrollTargetFrame() override
  {
    return do_QueryFrame(GetInner());
  }

#ifdef ACCESSIBILITY  
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override {
    return MakeFrameName(NS_LITERAL_STRING("FieldSet"), aResult);
  }
#endif

  





  nsIFrame* GetInner() const;

  




  nsIFrame* GetLegend() const;

protected:
  mozilla::LogicalRect mLegendRect;
  nscoord   mLegendSpace;
};

#endif 
