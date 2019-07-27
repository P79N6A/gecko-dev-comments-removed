






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
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    if (aFlags & eSupportsCSSTransforms) {
      return false;
    }
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer | nsIFrame::eLineParticipant));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;

  virtual bool IsEmpty() MOZ_OVERRIDE;
  virtual bool IsSelfEmpty() MOZ_OVERRIDE;

  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) MOZ_OVERRIDE;
  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  
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
  virtual nsRect ComputeTightBounds(gfxContext* aContext) const MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;

  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;

  virtual void PullOverflowsFromPrevInFlow() MOZ_OVERRIDE;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const MOZ_OVERRIDE;
  virtual bool DrainSelfOverflowList() MOZ_OVERRIDE;

  


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

  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const MOZ_OVERRIDE;

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







class nsFirstLineFrame MOZ_FINAL : public nsInlineFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsFirstLineFrame* NS_NewFirstLineFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext);

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void PullOverflowsFromPrevInFlow() MOZ_OVERRIDE;
  virtual bool DrainSelfOverflowList() MOZ_OVERRIDE;

protected:
  explicit nsFirstLineFrame(nsStyleContext* aContext) : nsInlineFrame(aContext) {}

  virtual nsIFrame* PullOneFrame(nsPresContext* aPresContext,
                                 InlineReflowState& rs,
                                 bool* aIsComplete) MOZ_OVERRIDE;
};

#endif
