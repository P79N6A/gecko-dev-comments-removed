




#ifndef nsFirstLetterFrame_h__
#define nsFirstLetterFrame_h__



#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"

class nsFirstLetterFrame MOZ_FINAL : public nsContainerFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsFirstLetterFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsFirstLetterFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  bool IsFloating() const { return GetStateBits() & NS_FRAME_OUT_OF_FLOW; }

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    if (!IsFloating())
      aFlags = aFlags & ~(nsIFrame::eLineParticipant);
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer));
  }

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) MOZ_OVERRIDE;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              uint32_t aFlags) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const MOZ_OVERRIDE;
  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const MOZ_OVERRIDE;


  virtual nsresult GetChildFrameContainingOffset(int32_t inContentOffset,
                                                 bool inHint,
                                                 int32_t* outFrameContentOffset,
                                                 nsIFrame** outChildFrame) MOZ_OVERRIDE;

  nscoord GetFirstLetterBaseline() const { return mBaseline; }

  
  
  
  
  nsresult CreateContinuationForFloatingParent(nsPresContext* aPresContext,
                                               nsIFrame* aChild,
                                               nsIFrame** aContinuation,
                                               bool aIsFluid);

protected:
  nscoord mBaseline;

  void DrainOverflowFrames(nsPresContext* aPresContext);
};

#endif 
