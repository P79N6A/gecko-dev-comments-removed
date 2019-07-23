








































































#ifndef nsPlaceholderFrame_h___
#define nsPlaceholderFrame_h___

#include "nsSplittableFrame.h"
#include "nsGkAtoms.h"

nsIFrame* NS_NewPlaceholderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);





class nsPlaceholderFrame : public nsSplittableFrame {
public:
  


  friend nsIFrame* NS_NewPlaceholderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  nsPlaceholderFrame(nsStyleContext* aContext) : nsSplittableFrame(aContext) {}
  virtual ~nsPlaceholderFrame();

  
  nsIFrame*  GetOutOfFlowFrame() const {return mOutOfFlowFrame;}
  void       SetOutOfFlowFrame(nsIFrame* aFrame) {mOutOfFlowFrame = aFrame;}

  
  
  
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual void AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  virtual void Destroy();
  virtual nsSplittableType GetSplittableType() const;

  
#if defined(DEBUG) || (defined(MOZ_REFLOW_PERF_DSP) && defined(MOZ_REFLOW_PERF))
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
#endif 
  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
#endif 

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual PRBool IsEmpty() { return PR_TRUE; }
  virtual PRBool IsSelfEmpty() { return PR_TRUE; }

  virtual PRBool CanContinueTextRun() const;

#ifdef ACCESSIBILITY
  NS_IMETHOD  GetAccessible(nsIAccessible** aAccessible)
  {
    nsIFrame *realFrame = GetRealFrameForPlaceholder(this);
    return realFrame ? realFrame->GetAccessible(aAccessible) :
                       nsSplittableFrame::GetAccessible(aAccessible);
  }
#endif

  



  static nsIFrame* GetRealFrameFor(nsIFrame* aFrame) {
    NS_PRECONDITION(aFrame, "Must have a frame to work with");
    if (aFrame->GetType() == nsGkAtoms::placeholderFrame) {
      return GetRealFrameForPlaceholder(aFrame);
    }
    return aFrame;
  }

  


  static nsIFrame* GetRealFrameForPlaceholder(nsIFrame* aFrame) {
    NS_PRECONDITION(aFrame->GetType() == nsGkAtoms::placeholderFrame,
                    "Must have placeholder frame as input");
    nsIFrame* outOfFlow =
      static_cast<nsPlaceholderFrame*>(aFrame)->GetOutOfFlowFrame();
    NS_ASSERTION(outOfFlow, "Null out-of-flow for placeholder?");
    return outOfFlow;
  }

protected:
  nsIFrame* mOutOfFlowFrame;
};

#endif 
