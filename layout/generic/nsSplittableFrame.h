









#ifndef nsSplittableFrame_h___
#define nsSplittableFrame_h___

#include "mozilla/Attributes.h"
#include "nsFrame.h"


class nsSplittableFrame : public nsFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;
  
  virtual nsSplittableType GetSplittableType() const MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  






  
  
  virtual nsIFrame* GetPrevContinuation() const MOZ_OVERRIDE;
  virtual nsIFrame* GetNextContinuation() const MOZ_OVERRIDE;

  
  NS_IMETHOD SetPrevContinuation(nsIFrame*) MOZ_OVERRIDE;
  NS_IMETHOD SetNextContinuation(nsIFrame*) MOZ_OVERRIDE;

  
  virtual nsIFrame* GetFirstContinuation() const MOZ_OVERRIDE;
  virtual nsIFrame* GetLastContinuation() const MOZ_OVERRIDE;

#ifdef DEBUG
  
  static bool IsInPrevContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
  static bool IsInNextContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
#endif
  
  
  nsIFrame* GetPrevInFlow() const;
  nsIFrame* GetNextInFlow() const;

  virtual nsIFrame* GetPrevInFlowVirtual() const MOZ_OVERRIDE { return GetPrevInFlow(); }
  virtual nsIFrame* GetNextInFlowVirtual() const MOZ_OVERRIDE { return GetNextInFlow(); }
  
  
  NS_IMETHOD  SetPrevInFlow(nsIFrame*) MOZ_OVERRIDE;
  NS_IMETHOD  SetNextInFlow(nsIFrame*) MOZ_OVERRIDE;

  
  virtual nsIFrame* GetFirstInFlow() const MOZ_OVERRIDE;
  virtual nsIFrame* GetLastInFlow() const MOZ_OVERRIDE;

  
  
  static void RemoveFromFlow(nsIFrame* aFrame);

protected:
  nsSplittableFrame(nsStyleContext* aContext) : nsFrame(aContext) {}

  






  nscoord GetConsumedHeight() const;

  



  nscoord GetEffectiveComputedHeight(const nsHTMLReflowState& aReflowState,
                                     nscoord aConsumed = NS_INTRINSICSIZE) const;

  


















  void ComputeFinalHeight(const nsHTMLReflowState& aReflowState,
                          nsReflowStatus*          aStatus,
                          nscoord                  aContentHeight,
                          const nsMargin&          aBorderPadding,
                          nsHTMLReflowMetrics&     aMetrics,
                          nscoord                  aConsumed);

  



  virtual int GetSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const;

#ifdef DEBUG
  virtual void DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent) MOZ_OVERRIDE;
#endif

  nsIFrame*   mPrevContinuation;
  nsIFrame*   mNextContinuation;
};

#endif 
