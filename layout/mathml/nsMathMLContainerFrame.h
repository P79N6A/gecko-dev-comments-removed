




#ifndef nsMathMLContainerFrame_h___
#define nsMathMLContainerFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsMathMLOperators.h"
#include "nsMathMLFrame.h"
#include "mozilla/Likely.h"











#define STRETCH_CONSIDER_ACTUAL_SIZE    0x00000001 // just use our current size
#define STRETCH_CONSIDER_EMBELLISHMENTS 0x00000002 // size calculations include embellishments

class nsMathMLContainerFrame : public nsContainerFrame,
                               public nsMathMLFrame {
  friend class nsMathMLmfencedFrame;
public:
  explicit nsMathMLContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  NS_DECL_QUERYFRAME_TARGET(nsMathMLContainerFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  

  NS_IMETHOD
  Stretch(nsRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize) MOZ_OVERRIDE;

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aFlagsToUpdate) MOZ_OVERRIDE
  {
    PropagatePresentationDataFromChildAt(this, aFirstIndex, aLastIndex,
      aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }
  
  
  
  
  
  
  
  
  
  
  void
  SetIncrementScriptLevel(int32_t aChildIndex, bool aIncrement);

  
  

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return !(aFlags & nsIFrame::eLineParticipant) &&
      nsContainerFrame::IsFrameOfType(aFlags &
              ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

  virtual void
  AppendFrames(ChildListID     aListID,
               nsFrameList&    aFrameList) MOZ_OVERRIDE;

  virtual void
  InsertFrames(ChildListID     aListID,
               nsIFrame*       aPrevFrame,
               nsFrameList&    aFrameList) MOZ_OVERRIDE;

  virtual void
  RemoveFrame(ChildListID     aListID,
              nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  



  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  


  virtual void
  GetIntrinsicISizeMetrics(nsRenderingContext* aRenderingContext,
                           nsHTMLReflowMetrics& aDesiredSize);

  virtual void
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual void
  WillReflow(nsPresContext* aPresContext) MOZ_OVERRIDE
  {
    mPresentationData.flags &= ~NS_MATHML_ERROR;
    nsContainerFrame::WillReflow(aPresContext);
  }

  virtual void DidReflow(nsPresContext*           aPresContext,
            const nsHTMLReflowState*  aReflowState,
            nsDidReflowStatus         aStatus) MOZ_OVERRIDE

  {
    mPresentationData.flags &= ~NS_MATHML_STRETCH_DONE;
    return nsContainerFrame::DidReflow(aPresContext, aReflowState, aStatus);
  }

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult
  AttributeChanged(int32_t         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   int32_t         aModType) MOZ_OVERRIDE;

  
  nscoord
  MirrorIfRTL(nscoord aParentWidth, nscoord aChildWidth, nscoord aChildLeading)
  {
    return (StyleVisibility()->mDirection ?
            aParentWidth - aChildWidth - aChildLeading : aChildLeading);
  }

  
  

protected:
  




























  virtual nsresult
  Place(nsRenderingContext& aRenderingContext,
        bool                 aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  
  
  
  
  
  
  
  
  virtual nsresult
  MeasureForWidth(nsRenderingContext& aRenderingContext,
                  nsHTMLReflowMetrics& aDesiredSize);


  
  
  virtual nsresult
  ChildListChanged(int32_t aModType);

  
  
  void
  GetPreferredStretchSize(nsRenderingContext& aRenderingContext,
                          uint32_t             aOptions,
                          nsStretchDirection   aStretchDirection,
                          nsBoundingMetrics&   aPreferredStretchSize);

  
  
  nsresult
  TransmitAutomaticDataForMrowLikeElement();

public:
  
  
  nsresult
  ReflowError(nsRenderingContext& aRenderingContext,
              nsHTMLReflowMetrics& aDesiredSize);
  





  nsresult
  ReportParseError(const char16_t*           aAttribute,
                   const char16_t*           aValue);

  



  nsresult
  ReportChildCountError();

  




  nsresult
  ReportInvalidChildError(nsIAtom* aChildTag);

  



  nsresult
  ReportErrorToConsole(const char*       aErrorMsgId,
                       const char16_t** aParams = nullptr,
                       uint32_t          aParamCount = 0);

  
  
  
  void
  ReflowChild(nsIFrame*                aKidFrame,
              nsPresContext*          aPresContext,
              nsHTMLReflowMetrics&     aDesiredSize,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aStatus);

protected:
  
  
  
  
  
  
  
  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize);

  
  
  virtual nsresult
  FinalizeReflow(nsRenderingContext& aRenderingContext,
                 nsHTMLReflowMetrics& aDesiredSize);

  
  static void
  SaveReflowAndBoundingMetricsFor(nsIFrame*                  aFrame,
                                  const nsHTMLReflowMetrics& aReflowMetrics,
                                  const nsBoundingMetrics&   aBoundingMetrics);

  
  
  
  
  
  
  
  static void
  GetReflowAndBoundingMetricsFor(nsIFrame*            aFrame,
                                 nsHTMLReflowMetrics& aReflowMetrics,
                                 nsBoundingMetrics&   aBoundingMetrics,
                                 eMathMLFrameType*    aMathMLFrameType = nullptr);

  
  
  void ClearSavedChildMetrics();

  
  
  static void
  PropagatePresentationDataFor(nsIFrame*       aFrame,
                               uint32_t        aFlagsValues,
                               uint32_t        aFlagsToUpdate);

public:
  static void
  PropagatePresentationDataFromChildAt(nsIFrame*       aParentFrame,
                                       int32_t         aFirstChildIndex,
                                       int32_t         aLastChildIndex,
                                       uint32_t        aFlagsValues,
                                       uint32_t        aFlagsToUpdate);

  
  static void
  PropagateFrameFlagFor(nsIFrame* aFrame,
                        nsFrameState aFlags);

  
  
  
  
  
  
  
  
  
  
  
  
  static void
  RebuildAutomaticDataForChildren(nsIFrame* aParentFrame);

  
  
  
  
  
  
  
  
  
  
  static nsresult
  ReLayoutChildren(nsIFrame* aParentFrame);

protected:
  
  
  
  void
  PositionRowChildFrames(nscoord aOffsetX, nscoord aBaseline);

  
  
  
  void GatherAndStoreOverflow(nsHTMLReflowMetrics* aMetrics);

  





  static void DidReflowChildren(nsIFrame* aFirst, nsIFrame* aStop = nullptr);

private:
  class RowChildFrameIterator;
  friend class RowChildFrameIterator;
};










class nsMathMLmathBlockFrame : public nsBlockFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsMathMLmathBlockFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsContainerFrame* NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell,
          nsStyleContext* aContext, nsFrameState aFlags);

  
  
  virtual void
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList, "unexpected frame list");
    nsBlockFrame::SetInitialChildList(aListID, aChildList);
    
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
  }

  virtual void
  AppendFrames(ChildListID     aListID,
               nsFrameList&    aFrameList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsBlockFrame::AppendFrames(aListID, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
  }

  virtual void
  InsertFrames(ChildListID     aListID,
               nsIFrame*       aPrevFrame,
               nsFrameList&    aFrameList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsBlockFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
  }

  virtual void
  RemoveFrame(ChildListID     aListID,
              nsIFrame*       aOldFrame) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsBlockFrame::RemoveFrame(aListID, aOldFrame);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE {
    return nsBlockFrame::IsFrameOfType(aFlags &
              ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

  
  bool IsMrowLike() {
    return mFrames.FirstChild() != mFrames.LastChild() ||
           !mFrames.FirstChild();
  }

protected:
  explicit nsMathMLmathBlockFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {
    
    
    AddStateBits(NS_BLOCK_FLOAT_MGR);
  }
  virtual ~nsMathMLmathBlockFrame() {}
};



class nsMathMLmathInlineFrame : public nsInlineFrame,
                                public nsMathMLFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsMathMLmathInlineFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsContainerFrame* NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell,
                                                       nsStyleContext* aContext);

  virtual void
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList, "unexpected frame list");
    nsInlineFrame::SetInitialChildList(aListID, aChildList);
    
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
  }

  virtual void
  AppendFrames(ChildListID     aListID,
               nsFrameList&    aFrameList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsInlineFrame::AppendFrames(aListID, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
  }

  virtual void
  InsertFrames(ChildListID     aListID,
               nsIFrame*       aPrevFrame,
               nsFrameList&    aFrameList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsInlineFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
  }

  virtual void
  RemoveFrame(ChildListID     aListID,
              nsIFrame*       aOldFrame) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsInlineFrame::RemoveFrame(aListID, aOldFrame);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE {
      return nsInlineFrame::IsFrameOfType(aFlags &
                ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

  bool
  IsMrowLike() MOZ_OVERRIDE {
    return mFrames.FirstChild() != mFrames.LastChild() ||
           !mFrames.FirstChild();
  }

protected:
  explicit nsMathMLmathInlineFrame(nsStyleContext* aContext) : nsInlineFrame(aContext) {}
  virtual ~nsMathMLmathInlineFrame() {}
};

#endif 
