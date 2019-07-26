









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
  
  virtual nsSplittableType GetSplittableType() const;

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  






  
  
  virtual nsIFrame* GetPrevContinuation() const MOZ_OVERRIDE;
  virtual nsIFrame* GetNextContinuation() const MOZ_OVERRIDE;

  
  NS_IMETHOD SetPrevContinuation(nsIFrame*) MOZ_OVERRIDE;
  NS_IMETHOD SetNextContinuation(nsIFrame*) MOZ_OVERRIDE;

  
  virtual nsIFrame* GetFirstContinuation() const;
  virtual nsIFrame* GetLastContinuation() const;

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

  
  virtual nsIFrame* GetFirstInFlow() const;
  virtual nsIFrame* GetLastInFlow() const;

  
  
  static void RemoveFromFlow(nsIFrame* aFrame);

protected:
  nsSplittableFrame(nsStyleContext* aContext) : nsFrame(aContext) {}

#ifdef DEBUG
  virtual void DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent) MOZ_OVERRIDE;
#endif

  nsIFrame*   mPrevContinuation;
  nsIFrame*   mNextContinuation;
};

#endif 
