









































#ifndef nsSplittableFrame_h___
#define nsSplittableFrame_h___

#include "nsFrame.h"


class nsSplittableFrame : public nsFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  
  virtual nsSplittableType GetSplittableType() const;

  virtual void Destroy();

  






  
  
  virtual nsIFrame* GetPrevContinuation() const;
  virtual nsIFrame* GetNextContinuation() const;

  
  NS_IMETHOD SetPrevContinuation(nsIFrame*);
  NS_IMETHOD SetNextContinuation(nsIFrame*);

  
  virtual nsIFrame* GetFirstContinuation() const;
  virtual nsIFrame* GetLastContinuation() const;

#ifdef DEBUG
  
  static PRBool IsInPrevContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
  static PRBool IsInNextContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2);
#endif
  
  
  nsIFrame* GetPrevInFlow() const;
  nsIFrame* GetNextInFlow() const;

  virtual nsIFrame* GetPrevInFlowVirtual() const { return GetPrevInFlow(); }
  virtual nsIFrame* GetNextInFlowVirtual() const { return GetNextInFlow(); }
  
  
  NS_IMETHOD  SetPrevInFlow(nsIFrame*);
  NS_IMETHOD  SetNextInFlow(nsIFrame*);

  
  virtual nsIFrame* GetFirstInFlow() const;
  virtual nsIFrame* GetLastInFlow() const;

  
  
  static void RemoveFromFlow(nsIFrame* aFrame);

protected:
  nsSplittableFrame(nsStyleContext* aContext) : nsFrame(aContext) {}

#ifdef DEBUG
  virtual void DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent);
#endif

  nsIFrame*   mPrevContinuation;
  nsIFrame*   mNextContinuation;
};

#endif 
