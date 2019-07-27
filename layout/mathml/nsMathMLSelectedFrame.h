




#ifndef nsMathMLSelectedFrame_h___
#define nsMathMLSelectedFrame_h___

#include "nsMathMLContainerFrame.h"

class nsMathMLSelectedFrame : public nsMathMLContainerFrame {
public:
  virtual void
  Init(nsIContent*       aContent,
       nsContainerFrame* aParent,
       nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  NS_IMETHOD
  TransmitAutomaticData() MOZ_OVERRIDE;

  virtual void
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList) MOZ_OVERRIDE;

  virtual nsresult
  ChildListChanged(int32_t aModType) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize) MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              ComputeSizeFlags aFlags) MOZ_OVERRIDE;

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nsQueryFrame::FrameIID GetFrameId() MOZ_OVERRIDE = 0;

protected:
  explicit nsMathMLSelectedFrame(nsStyleContext* aContext) :
    nsMathMLContainerFrame(aContext) {}
  virtual ~nsMathMLSelectedFrame();
  
  virtual nsIFrame* GetSelectedFrame() = 0;
  nsIFrame*       mSelectedFrame;

  bool            mInvalidMarkup;
  
private:
  void* operator new(size_t, nsIPresShell*) MOZ_MUST_OVERRIDE = delete;
};

#endif 
