









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
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  
  virtual nsSplittableType GetSplittableType() const MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  






  
  
  virtual nsIFrame* GetPrevContinuation() const MOZ_OVERRIDE;
  virtual nsIFrame* GetNextContinuation() const MOZ_OVERRIDE;

  
  virtual void SetPrevContinuation(nsIFrame*) MOZ_OVERRIDE;
  virtual void SetNextContinuation(nsIFrame*) MOZ_OVERRIDE;

  
  virtual nsIFrame* FirstContinuation() const MOZ_OVERRIDE;
  virtual nsIFrame* LastContinuation() const MOZ_OVERRIDE;

#ifdef DEBUG
  
  static bool IsInPrevContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
  static bool IsInNextContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
#endif
  
  
  nsIFrame* GetPrevInFlow() const;
  nsIFrame* GetNextInFlow() const;

  virtual nsIFrame* GetPrevInFlowVirtual() const MOZ_OVERRIDE { return GetPrevInFlow(); }
  virtual nsIFrame* GetNextInFlowVirtual() const MOZ_OVERRIDE { return GetNextInFlow(); }
  
  
  virtual void SetPrevInFlow(nsIFrame*) MOZ_OVERRIDE;
  virtual void SetNextInFlow(nsIFrame*) MOZ_OVERRIDE;

  
  virtual nsIFrame* FirstInFlow() const MOZ_OVERRIDE;
  virtual nsIFrame* LastInFlow() const MOZ_OVERRIDE;

  
  
  static void RemoveFromFlow(nsIFrame* aFrame);

protected:
  explicit nsSplittableFrame(nsStyleContext* aContext) : nsFrame(aContext) {}

  






  nscoord GetConsumedBSize() const;

  



  nscoord GetEffectiveComputedBSize(const nsHTMLReflowState& aReflowState,
                                    nscoord aConsumed = NS_INTRINSICSIZE) const;

  


  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent) MOZ_OVERRIDE;
#endif

  nsIFrame*   mPrevContinuation;
  nsIFrame*   mNextContinuation;
};

#endif 
