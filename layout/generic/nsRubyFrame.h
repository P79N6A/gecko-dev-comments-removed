







#ifndef nsRubyFrame_h___
#define nsRubyFrame_h___

#include "nsInlineFrame.h"

class nsRubyBaseContainerFrame;

typedef nsInlineFrame nsRubyFrameSuper;





nsContainerFrame* NS_NewRubyFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

class nsRubyFrame final : public nsRubyFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const override;
  virtual bool IsFrameOfType(uint32_t aFlags) const override;
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) override;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) override;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  void GetBlockLeadings(nscoord& aStartLeading, nscoord& aEndLeading)
  {
    aStartLeading = mBStartLeading;
    aEndLeading = mBEndLeading;
  }

protected:
  friend nsContainerFrame* NS_NewRubyFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);
  explicit nsRubyFrame(nsStyleContext* aContext)
    : nsRubyFrameSuper(aContext) {}

  void ReflowSegment(nsPresContext* aPresContext,
                     const nsHTMLReflowState& aReflowState,
                     nsRubyBaseContainerFrame* aBaseContainer,
                     nsReflowStatus& aStatus);

  nsRubyBaseContainerFrame* PullOneSegment(ContinuationTraversingState& aState);

  
  
  nscoord mBStartLeading;
  nscoord mBEndLeading;
};

#endif 
