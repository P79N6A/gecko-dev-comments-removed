







#ifndef nsRubyTextContainerFrame_h___
#define nsRubyTextContainerFrame_h___

#include "nsBlockFrame.h"
#include "nsRubyBaseFrame.h"
#include "nsRubyTextFrame.h"
#include "nsLineLayout.h"

typedef nsContainerFrame nsRubyTextContainerFrameSuper;





nsContainerFrame* NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);

class nsRubyTextContainerFrame MOZ_FINAL : public nsRubyTextContainerFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyTextContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  virtual void SetInitialChildList(ChildListID aListID,
                                   nsFrameList& aChildList) MOZ_OVERRIDE;
  virtual void AppendFrames(ChildListID aListID,
                            nsFrameList& aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID aListID, nsIFrame* aPrevFrame,
                            nsFrameList& aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID aListID,
                           nsIFrame* aOldFrame) MOZ_OVERRIDE;

  bool IsSpanContainer() const
  {
    return GetStateBits() & NS_RUBY_TEXT_CONTAINER_IS_SPAN;
  }

protected:
  friend nsContainerFrame*
    NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  explicit nsRubyTextContainerFrame(nsStyleContext* aContext)
    : nsRubyTextContainerFrameSuper(aContext)
    , mISize(0) {}

  void UpdateSpanFlag();

  friend class nsRubyBaseContainerFrame;
  void SetISize(nscoord aISize) { mISize = aISize; }

  
  
  
  nscoord mISize;
};

#endif 
