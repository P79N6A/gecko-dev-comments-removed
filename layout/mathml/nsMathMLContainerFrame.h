




#ifndef nsMathMLContainerFrame_h___
#define nsMathMLContainerFrame_h___

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#include "nsMathMLChar.h"
#include "nsMathMLFrame.h"
#include "nsMathMLParts.h"
#include "mozilla/Likely.h"











#define STRETCH_CONSIDER_ACTUAL_SIZE    0x00000001 // just use our current size
#define STRETCH_CONSIDER_EMBELLISHMENTS 0x00000002 // size calculations include embellishments

class nsMathMLContainerFrame : public nsContainerFrame,
                               public nsMathMLFrame {
  friend class nsMathMLmfencedFrame;
public:
  nsMathMLContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  

  NS_IMETHOD
  Stretch(nsRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aFlagsToUpdate)
  {
    PropagatePresentationDataFromChildAt(this, aFirstIndex, aLastIndex,
      aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }
  
  
  
  
  
  
  
  
  
  
  void
  SetIncrementScriptLevel(int32_t aChildIndex, bool aIncrement);

  
  

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return !(aFlags & nsIFrame::eLineParticipant) &&
      nsContainerFrame::IsFrameOfType(aFlags &
              ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

  NS_IMETHOD
  AppendFrames(ChildListID     aListID,
               nsFrameList&    aFrameList);

  NS_IMETHOD
  InsertFrames(ChildListID     aListID,
               nsIFrame*       aPrevFrame,
               nsFrameList&    aFrameList) MOZ_OVERRIDE;

  NS_IMETHOD
  RemoveFrame(ChildListID     aListID,
              nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  



  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  


  virtual nscoord GetIntrinsicWidth(nsRenderingContext *aRenderingContext);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  NS_IMETHOD
  WillReflow(nsPresContext* aPresContext)
  {
    mPresentationData.flags &= ~NS_MATHML_ERROR;
    return nsContainerFrame::WillReflow(aPresContext);
  }

  NS_IMETHOD
  DidReflow(nsPresContext*           aPresContext,
            const nsHTMLReflowState*  aReflowState,
            nsDidReflowStatus         aStatus)

  {
    mPresentationData.flags &= ~NS_MATHML_STRETCH_DONE;
    return nsContainerFrame::DidReflow(aPresContext, aReflowState, aStatus);
  }

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual bool UpdateOverflow();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_IMETHOD
  AttributeChanged(int32_t         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   int32_t         aModType);

  
  nscoord
  MirrorIfRTL(nscoord aParentWidth, nscoord aChildWidth, nscoord aChildLeading)
  {
    return (NS_MATHML_IS_RTL(mPresentationData.flags) ?
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
  ReportParseError(const PRUnichar*           aAttribute,
                   const PRUnichar*           aValue);

  



  nsresult
  ReportChildCountError();

  



  nsresult
  ReportErrorToConsole(const char*       aErrorMsgId,
                       const PRUnichar** aParams = nullptr,
                       uint32_t          aParamCount = 0);

  
  
  
  nsresult 
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
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell,
          nsStyleContext* aContext, uint32_t aFlags);

  
  
  NS_IMETHOD
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList)
  {
    NS_ASSERTION(aListID == kPrincipalList, "unexpected frame list");
    nsresult rv = nsBlockFrame::SetInitialChildList(aListID, aChildList);
    
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
    return rv;
  }

  NS_IMETHOD
  AppendFrames(ChildListID     aListID,
               nsFrameList&    aFrameList)
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsresult rv = nsBlockFrame::AppendFrames(aListID, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(ChildListID     aListID,
               nsIFrame*       aPrevFrame,
               nsFrameList&    aFrameList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsresult rv = nsBlockFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(ChildListID     aListID,
              nsIFrame*       aOldFrame) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsresult rv = nsBlockFrame::RemoveFrame(aListID, aOldFrame);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE {
    return nsBlockFrame::IsFrameOfType(aFlags &
              ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

protected:
  nsMathMLmathBlockFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {
    
    
    AddStateBits(NS_BLOCK_FLOAT_MGR);
  }
  virtual ~nsMathMLmathBlockFrame() {}
};



class nsMathMLmathInlineFrame : public nsInlineFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  SetInitialChildList(ChildListID     aListID,
                      nsFrameList&    aChildList)
  {
    NS_ASSERTION(aListID == kPrincipalList, "unexpected frame list");
    nsresult rv = nsInlineFrame::SetInitialChildList(aListID, aChildList);
    
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
    return rv;
  }

  NS_IMETHOD
  AppendFrames(ChildListID     aListID,
               nsFrameList&    aFrameList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsresult rv = nsInlineFrame::AppendFrames(aListID, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(ChildListID     aListID,
               nsIFrame*       aPrevFrame,
               nsFrameList&    aFrameList) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsresult rv = nsInlineFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(ChildListID     aListID,
              nsIFrame*       aOldFrame) MOZ_OVERRIDE
  {
    NS_ASSERTION(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
                 "unexpected frame list");
    nsresult rv = nsInlineFrame::RemoveFrame(aListID, aOldFrame);
    if (MOZ_LIKELY(aListID == kPrincipalList))
      nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const {
      return nsInlineFrame::IsFrameOfType(aFlags &
                ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

protected:
  nsMathMLmathInlineFrame(nsStyleContext* aContext) : nsInlineFrame(aContext) {}
  virtual ~nsMathMLmathInlineFrame() {}
};

#endif 
