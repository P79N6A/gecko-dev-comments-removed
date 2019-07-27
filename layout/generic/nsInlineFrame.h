






#ifndef nsInlineFrame_h___
#define nsInlineFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"

class nsLineLayout;

typedef nsContainerFrame nsInlineFrameBase;







class nsInlineFrame : public nsInlineFrameBase
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsInlineFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsInlineFrame* NS_NewInlineFrame(nsIPresShell* aPresShell,
                                          nsStyleContext* aContext);

  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif
  virtual nsIAtom* GetType() const override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    if (aFlags & eSupportsCSSTransforms) {
      return false;
    }
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer | nsIFrame::eLineParticipant));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) override;

  virtual bool IsEmpty() override;
  virtual bool IsSelfEmpty() override;

  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) override;
  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;
  virtual nsresult StealFrame(nsIFrame* aChild, bool aForceNormal) override;

  
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) override;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) override;
  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              ComputeSizeFlags aFlags) override;
  virtual nsRect ComputeTightBounds(gfxContext* aContext) const override;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) override;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) override;

  virtual bool CanContinueTextRun() const override;

  virtual void PullOverflowsFromPrevInFlow() override;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const override;
  virtual bool DrainSelfOverflowList() override;

  


  bool IsFirst() const {
    
    
    return (GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET)
             ? !!(GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_IS_FIRST)
             : (!GetPrevInFlow());
  }

  


  bool IsLast() const {
    
    
    return (GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET)
             ? !!(GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_IS_LAST)
             : (!GetNextInFlow());
  }

protected:
  
  struct InlineReflowState {
    nsIFrame* mPrevFrame;
    nsInlineFrame* mNextInFlow;
    nsIFrame*      mLineContainer;
    nsLineLayout*  mLineLayout;
    bool mSetParentPointer;  
                                     

    InlineReflowState()  {
      mPrevFrame = nullptr;
      mNextInFlow = nullptr;
      mLineContainer = nullptr;
      mLineLayout = nullptr;
      mSetParentPointer = false;
    }
  };

  explicit nsInlineFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const override;

  void ReflowFrames(nsPresContext* aPresContext,
                    const nsHTMLReflowState& aReflowState,
                    InlineReflowState& rs,
                    nsHTMLReflowMetrics& aMetrics,
                    nsReflowStatus& aStatus);

  void ReflowInlineFrame(nsPresContext* aPresContext,
                         const nsHTMLReflowState& aReflowState,
                         InlineReflowState& rs,
                         nsIFrame* aFrame,
                         nsReflowStatus& aStatus);

  





  void ReparentFloatsForInlineChild(nsIFrame* aOurBlock, nsIFrame* aFrame,
                                    bool aReparentSiblings);

  virtual nsIFrame* PullOneFrame(nsPresContext* aPresContext,
                                 InlineReflowState& rs,
                                 bool* aIsComplete);

  virtual void PushFrames(nsPresContext* aPresContext,
                          nsIFrame* aFromChild,
                          nsIFrame* aPrevSibling,
                          InlineReflowState& aState);

private:
  
  
  enum DrainFlags {
    eDontReparentFrames = 1, 
    eInFirstLine = 2, 
    eForDestroy = 4, 
                     
                     
  };
  





  bool DrainSelfOverflowListInternal(DrainFlags aFlags,
                                     nsIFrame* aLineContainer);
protected:
  nscoord mBaseline;
};







class nsFirstLineFrame final : public nsInlineFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsFirstLineFrame* NS_NewFirstLineFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif
  virtual nsIAtom* GetType() const override;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) override;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;
  virtual void PullOverflowsFromPrevInFlow() override;
  virtual bool DrainSelfOverflowList() override;

protected:
  explicit nsFirstLineFrame(nsStyleContext* aContext) : nsInlineFrame(aContext) {}

  virtual nsIFrame* PullOneFrame(nsPresContext* aPresContext,
                                 InlineReflowState& rs,
                                 bool* aIsComplete) override;
};

#endif
