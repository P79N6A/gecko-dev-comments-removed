









#ifndef nsSplittableFrame_h___
#define nsSplittableFrame_h___

#include "mozilla/Attributes.h"
#include "nsFrame.h"


class nsSplittableFrame : public nsFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;
  
  virtual nsSplittableType GetSplittableType() const override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  






  
  
  virtual nsIFrame* GetPrevContinuation() const override;
  virtual nsIFrame* GetNextContinuation() const override;

  
  virtual void SetPrevContinuation(nsIFrame*) override;
  virtual void SetNextContinuation(nsIFrame*) override;

  
  virtual nsIFrame* FirstContinuation() const override;
  virtual nsIFrame* LastContinuation() const override;

#ifdef DEBUG
  
  static bool IsInPrevContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
  static bool IsInNextContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
#endif
  
  
  nsIFrame* GetPrevInFlow() const;
  nsIFrame* GetNextInFlow() const;

  virtual nsIFrame* GetPrevInFlowVirtual() const override { return GetPrevInFlow(); }
  virtual nsIFrame* GetNextInFlowVirtual() const override { return GetNextInFlow(); }
  
  
  virtual void SetPrevInFlow(nsIFrame*) override;
  virtual void SetNextInFlow(nsIFrame*) override;

  
  virtual nsIFrame* FirstInFlow() const override;
  virtual nsIFrame* LastInFlow() const override;

  
  
  static void RemoveFromFlow(nsIFrame* aFrame);

protected:
  explicit nsSplittableFrame(nsStyleContext* aContext) : nsFrame(aContext) {}

  






  nscoord GetConsumedBSize() const;

  



  nscoord GetEffectiveComputedBSize(const nsHTMLReflowState& aReflowState,
                                    nscoord aConsumed = NS_INTRINSICSIZE) const;

  


  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const override;

#ifdef DEBUG
  virtual void DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent) override;
#endif

  nsIFrame*   mPrevContinuation;
  nsIFrame*   mNextContinuation;
};

#endif 
