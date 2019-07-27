









#ifndef nsViewportFrame_h___
#define nsViewportFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"

class nsPresContext;






class ViewportFrame : public nsContainerFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(ViewportFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  typedef nsContainerFrame Super;

  explicit ViewportFrame(nsStyleContext* aContext)
    : nsContainerFrame(aContext)
  {}
  virtual ~ViewportFrame() { } 

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual mozilla::WritingMode GetWritingMode() const override
  {
    nsIFrame* firstChild = mFrames.FirstChild();
    if (firstChild) {
      return firstChild->GetWritingMode();
    }
    return nsIFrame::GetWritingMode();
  }

#ifdef DEBUG
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) override;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;
  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  




  virtual nsIAtom* GetType() const override;

  virtual bool UpdateOverflow() override;

  





  nsRect AdjustReflowStateAsContainingBlock(nsHTMLReflowState* aReflowState) const;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

private:
  virtual mozilla::layout::FrameChildListID GetAbsoluteListID() const override { return kFixedList; }

protected:
  






  nsPoint AdjustReflowStateForScrollbars(nsHTMLReflowState* aReflowState) const;
};


#endif 
