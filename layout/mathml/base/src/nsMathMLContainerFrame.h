








































#ifndef nsMathMLContainerFrame_h___
#define nsMathMLContainerFrame_h___

#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#include "nsMathMLChar.h"
#include "nsMathMLFrame.h"
#include "nsMathMLParts.h"











#define STRETCH_CONSIDER_ACTUAL_SIZE    0x00000001 // just use our current size
#define STRETCH_CONSIDER_EMBELLISHMENTS 0x00000002 // size calculations include embellishments

class nsMathMLContainerFrame : public nsHTMLContainerFrame,
                               public nsMathMLFrame {
  friend class nsMathMLmfencedFrame;
public:
  nsMathMLContainerFrame(nsStyleContext* aContext) : nsHTMLContainerFrame(aContext) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  

  NS_IMETHOD
  Stretch(nsIRenderingContext& aRenderingContext,
          nsStretchDirection   aStretchDirection,
          nsBoundingMetrics&   aContainerSize,
          nsHTMLReflowMetrics& aDesiredStretchSize);

  NS_IMETHOD
  Place(nsIRenderingContext& aRenderingContext,
        PRBool               aPlaceOrigin,
        nsHTMLReflowMetrics& aDesiredSize);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
  {
    PropagatePresentationDataFromChildAt(this, aFirstIndex, aLastIndex,
      aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }
  
  
  
  
  
  
  
  
  
  
  void
  SetIncrementScriptLevel(PRInt32 aChildIndex, PRBool aIncrement);

  
  

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return !(aFlags & nsIFrame::eLineParticipant) &&
      nsHTMLContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

  NS_IMETHOD
  AppendFrames(nsIAtom*        aListName,
               nsIFrame*       aFrameList);

  NS_IMETHOD
  InsertFrames(nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList);

  NS_IMETHOD
  RemoveFrame(nsIAtom*        aListName,
              nsIFrame*       aOldFrame);

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  NS_IMETHOD
  WillReflow(nsPresContext* aPresContext)
  {
    mPresentationData.flags &= ~NS_MATHML_ERROR;
    return nsHTMLContainerFrame::WillReflow(aPresContext);
  }

  NS_IMETHOD
  DidReflow(nsPresContext*           aPresContext,
            const nsHTMLReflowState*  aReflowState,
            nsDidReflowStatus         aStatus)

  {
    mPresentationData.flags &= ~NS_MATHML_STRETCH_DONE;
    return nsHTMLContainerFrame::DidReflow(aPresContext, aReflowState, aStatus);
  }

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_IMETHOD
  AttributeChanged(PRInt32         aNameSpaceID,
                   nsIAtom*        aAttribute,
                   PRInt32         aModType);

  
  

  
  
  virtual nsresult
  ChildListChanged(PRInt32 aModType);

  
  
  virtual void
  GetPreferredStretchSize(nsIRenderingContext& aRenderingContext,
                          PRUint32             aOptions,
                          nsStretchDirection   aStretchDirection,
                          nsBoundingMetrics&   aPreferredStretchSize);

  
  
  virtual nsresult
  ReflowError(nsIRenderingContext& aRenderingContext,
              nsHTMLReflowMetrics& aDesiredSize);

  
  
  
  nsresult 
  ReflowChild(nsIFrame*                aKidFrame,
              nsPresContext*          aPresContext,
              nsHTMLReflowMetrics&     aDesiredSize,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aStatus);

  
  
  
  
  
  
  
  virtual nscoord
  FixInterFrameSpacing(nsHTMLReflowMetrics& aDesiredSize);

  
  
  virtual nsresult
  FinalizeReflow(nsIRenderingContext& aRenderingContext,
                 nsHTMLReflowMetrics& aDesiredSize);

  
  
  
  
  
  
  static void
  GetReflowAndBoundingMetricsFor(nsIFrame*            aFrame,
                                 nsHTMLReflowMetrics& aReflowMetrics,
                                 nsBoundingMetrics&   aBoundingMetrics,
                                 eMathMLFrameType*    aMathMLFrameType = nsnull);

  
  
  static void
  PropagatePresentationDataFor(nsIFrame*       aFrame,
                               PRUint32        aFlagsValues,
                               PRUint32        aFlagsToUpdate);

  static void
  PropagatePresentationDataFromChildAt(nsIFrame*       aParentFrame,
                                       PRInt32         aFirstChildIndex,
                                       PRInt32         aLastChildIndex,
                                       PRUint32        aFlagsValues,
                                       PRUint32        aFlagsToUpdate);

  
  
  
  
  
  
  
  
  
  
  
  
  static void
  RebuildAutomaticDataForChildren(nsIFrame* aParentFrame);

  
  
  
  
  
  
  
  
  
  
  static nsresult
  ReLayoutChildren(nsIFrame* aParentFrame, nsFrameState aBits);

protected:
  virtual PRIntn GetSkipSides() const { return 0; }

  
  
  
  void
  PositionRowChildFrames(nscoord aOffsetX, nscoord aBaseline);

  
  
  
  void GatherAndStoreOverflow(nsHTMLReflowMetrics* aMetrics);

  





  void DidReflowChildren(nsIFrame* aFirst, nsIFrame* aStop = nsnull);

private:
  class RowChildFrameIterator;
  friend class RowChildFrameIterator;
};










class nsMathMLmathBlockFrame : public nsBlockFrame {
public:
  friend nsIFrame* NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell,
          nsStyleContext* aContext, PRUint32 aFlags);

  
  
  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    NS_ASSERTION(!aListName, "unexpected frame list");
    nsresult rv = nsBlockFrame::SetInitialChildList(aListName, aChildList);
    
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
    return rv;
  }

  NS_IMETHOD
  AppendFrames(nsIAtom*        aListName,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName || nsGkAtoms::nextBidi == aListName,
                 "unexpected frame list");
    nsresult rv = nsBlockFrame::AppendFrames(aListName, aFrameList);
    if (NS_LIKELY(!aListName))
      nsMathMLContainerFrame::ReLayoutChildren(this,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName || nsGkAtoms::nextBidi == aListName,
                 "unexpected frame list");
    nsresult rv = nsBlockFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
    if (NS_LIKELY(!aListName))
      nsMathMLContainerFrame::ReLayoutChildren(this,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIAtom*        aListName,
              nsIFrame*       aOldFrame)
  {
    NS_ASSERTION(!aListName || nsGkAtoms::nextBidi == aListName,
                 "unexpected frame list");
    nsresult rv = nsBlockFrame::RemoveFrame(aListName, aOldFrame);
    if (NS_LIKELY(!aListName))
      nsMathMLContainerFrame::ReLayoutChildren(this,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
    return rv;
  }

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const {
    return nsBlockFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

protected:
  nsMathMLmathBlockFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {
    
    
    AddStateBits(NS_BLOCK_SPACE_MGR);
  }
  virtual ~nsMathMLmathBlockFrame() {}
};



class nsMathMLmathInlineFrame : public nsInlineFrame {
public:
  friend nsIFrame* NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    NS_ASSERTION(!aListName, "unexpected frame list");
    nsresult rv = nsInlineFrame::SetInitialChildList(aListName, aChildList);
    
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
    return rv;
  }

  NS_IMETHOD
  AppendFrames(nsIAtom*        aListName,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName || nsGkAtoms::nextBidi == aListName,
                 "unexpected frame list");
    nsresult rv = nsInlineFrame::AppendFrames(aListName, aFrameList);
    if (NS_LIKELY(!aListName))
      nsMathMLContainerFrame::ReLayoutChildren(this,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName || nsGkAtoms::nextBidi == aListName,
                 "unexpected frame list");
    nsresult rv = nsInlineFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
    if (NS_LIKELY(!aListName))
      nsMathMLContainerFrame::ReLayoutChildren(this,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIAtom*        aListName,
              nsIFrame*       aOldFrame)
  {
    NS_ASSERTION(!aListName || nsGkAtoms::nextBidi == aListName,
                 "unexpected frame list");
    nsresult rv = nsInlineFrame::RemoveFrame(aListName, aOldFrame);
    if (NS_LIKELY(!aListName))
      nsMathMLContainerFrame::ReLayoutChildren(this,
                                               NS_FRAME_HAS_DIRTY_CHILDREN);
    return rv;
  }

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const {
    return nsInlineFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

protected:
  nsMathMLmathInlineFrame(nsStyleContext* aContext) : nsInlineFrame(aContext) {}
  virtual ~nsMathMLmathInlineFrame() {}
};

#endif 
