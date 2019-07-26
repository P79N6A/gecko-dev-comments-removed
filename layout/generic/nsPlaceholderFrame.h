
































#ifndef nsPlaceholderFrame_h___
#define nsPlaceholderFrame_h___

#include "mozilla/Attributes.h"
#include "nsFrame.h"
#include "nsGkAtoms.h"

nsIFrame* NS_NewPlaceholderFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext,
                                 nsFrameState aTypeBit);



#define PLACEHOLDER_FOR_FLOAT    NS_FRAME_STATE_BIT(20)
#define PLACEHOLDER_FOR_ABSPOS   NS_FRAME_STATE_BIT(21)
#define PLACEHOLDER_FOR_FIXEDPOS NS_FRAME_STATE_BIT(22)
#define PLACEHOLDER_FOR_POPUP    NS_FRAME_STATE_BIT(23)
#define PLACEHOLDER_TYPE_MASK    (PLACEHOLDER_FOR_FLOAT | \
                                  PLACEHOLDER_FOR_ABSPOS | \
                                  PLACEHOLDER_FOR_FIXEDPOS | \
                                  PLACEHOLDER_FOR_POPUP)





class nsPlaceholderFrame MOZ_FINAL : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  



  friend nsIFrame* NS_NewPlaceholderFrame(nsIPresShell* aPresShell,
                                          nsStyleContext* aContext,
                                          nsFrameState aTypeBit);
  nsPlaceholderFrame(nsStyleContext* aContext, nsFrameState aTypeBit)
    : nsFrame(aContext)
  {
    NS_PRECONDITION(aTypeBit == PLACEHOLDER_FOR_FLOAT ||
                    aTypeBit == PLACEHOLDER_FOR_ABSPOS ||
                    aTypeBit == PLACEHOLDER_FOR_FIXEDPOS ||
                    aTypeBit == PLACEHOLDER_FOR_POPUP,
                    "Unexpected type bit");
    AddStateBits(aTypeBit);
  }

  
  nsIFrame*  GetOutOfFlowFrame() const { return mOutOfFlowFrame; }
  void       SetOutOfFlowFrame(nsIFrame* aFrame) {
               NS_ASSERTION(!aFrame || !aFrame->GetPrevContinuation(),
                            "OOF must be first continuation");
               mOutOfFlowFrame = aFrame;
             }

  
  
  
  virtual void AddInlineMinWidth(nsRenderingContext* aRenderingContext,
                                 InlineMinWidthData* aData) MOZ_OVERRIDE;
  virtual void AddInlinePrefWidth(nsRenderingContext* aRenderingContext,
                                  InlinePrefWidthData* aData) MOZ_OVERRIDE;
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

#if defined(DEBUG) || (defined(MOZ_REFLOW_PERF_DSP) && defined(MOZ_REFLOW_PERF))
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
#endif 
  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, int32_t aIndent, uint32_t aFlags = 0) const MOZ_OVERRIDE;
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif 

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsEmpty() MOZ_OVERRIDE { return true; }
  virtual bool IsSelfEmpty() MOZ_OVERRIDE { return true; }

  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE
  {
    nsIFrame* realFrame = GetRealFrameForPlaceholder(this);
    return realFrame ? realFrame->AccessibleType() :
                       nsFrame::AccessibleType();
  }
#endif

  virtual nsIFrame* GetParentStyleContextFrame() const MOZ_OVERRIDE;

  



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
