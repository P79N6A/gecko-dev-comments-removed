







#ifndef nsRubyBaseContainerFrame_h___
#define nsRubyBaseContainerFrame_h___

#include "nsContainerFrame.h"
#include "nsRubyTextContainerFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyTextFrame.h"





nsContainerFrame* NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);

class nsRubyBaseContainerFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyBaseContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE;
  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  void AppendTextContainer(nsIFrame* aFrame);
  void ClearTextContainers();

protected:
  friend nsContainerFrame*
    NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  explicit nsRubyBaseContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}
  





  nsTArray<nsRubyTextContainerFrame*> mTextContainers;
};

#endif 
