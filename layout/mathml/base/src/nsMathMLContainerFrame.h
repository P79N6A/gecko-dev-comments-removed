






































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















#define NS_MATHML_CSS_POSITIVE_SCRIPTLEVEL_LIMIT  +5
#define NS_MATHML_CSS_NEGATIVE_SCRIPTLEVEL_LIMIT  -5
#define NS_MATHML_SCRIPTSIZEMULTIPLIER             0.71f
#define NS_MATHML_SCRIPTMINSIZE                    8


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
                                    PRInt32         aScriptLevelIncrement,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
  {
    PropagatePresentationDataFromChildAt(this, aFirstIndex, aLastIndex,
      aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }

  NS_IMETHOD
  ReResolveScriptStyle(PRInt32 aParentScriptLevel)
  {
    PropagateScriptStyleFor(this, aParentScriptLevel);
    return NS_OK;
  }

  
  

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsHTMLContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

  NS_IMETHOD
  Init(nsIContent*      aContent,
       nsIFrame*        aParent,
       nsIFrame*        aPrevInFlow);

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

  nsresult 
  ReflowForeignChild(nsIFrame*                aKidFrame,
                     nsPresContext*           aPresContext,
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
  PropagateScriptStyleFor(nsIFrame*       aFrame,
                          PRInt32         aParentScriptLevel);

  
  
  static void
  PropagatePresentationDataFor(nsIFrame*       aFrame,
                               PRInt32         aScriptLevelIncrement,
                               PRUint32        aFlagsValues,
                               PRUint32        aFlagsToUpdate);

  static void
  PropagatePresentationDataFromChildAt(nsIFrame*       aParentFrame,
                                       PRInt32         aFirstChildIndex,
                                       PRInt32         aLastChildIndex,
                                       PRInt32         aScriptLevelIncrement,
                                       PRUint32        aFlagsValues,
                                       PRUint32        aFlagsToUpdate);

  
  
  
  
  
  
  
  
  
  
  
  
  static void
  RebuildAutomaticDataForChildren(nsIFrame* aParentFrame);

  
  
  
  
  
  
  
  
  static nsresult
  ReLayoutChildren(nsIFrame* aParentFrame);

protected:
  virtual PRIntn GetSkipSides() const { return 0; }

  





  void DidReflowChildren(nsIFrame* aFirst, nsIFrame* aStop = nsnull);
};










class nsMathMLmathBlockFrame : public nsBlockFrame {
public:
  friend nsIFrame* NS_NewMathMLmathBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  
  
  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv = nsBlockFrame::SetInitialChildList(aListName, aChildList);
    
    nsMathMLContainerFrame::MapCommonAttributesIntoCSS(PresContext(), this);
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
    return rv;
  }

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus)
  {
    if (mScriptStyleChanged) {
      mScriptStyleChanged = PR_FALSE;
      nsMathMLContainerFrame::PropagateScriptStyleFor(this, 0);
    }
    return nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  NS_IMETHOD
  AppendFrames(nsIAtom*        aListName,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName, "internal error");
    nsresult rv = nsBlockFrame::AppendFrames(aListName, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName, "internal error");
    nsresult rv = nsBlockFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIAtom*        aListName,
              nsIFrame*       aOldFrame)
  {
    NS_ASSERTION(!aListName, "internal error");
    nsresult rv = nsBlockFrame::RemoveFrame(aListName, aOldFrame);
    nsMathMLContainerFrame::ReLayoutChildren(this);
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

  NS_IMETHOD
  DidSetStyleContext()
  {
    mScriptStyleChanged = PR_TRUE;
    return nsBlockFrame::DidSetStyleContext();
  }

  PRBool mScriptStyleChanged;
};



class nsMathMLmathInlineFrame : public nsInlineFrame {
public:
  friend nsIFrame* NS_NewMathMLmathInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv = nsInlineFrame::SetInitialChildList(aListName, aChildList);
    
    nsMathMLContainerFrame::MapCommonAttributesIntoCSS(PresContext(), this);
    nsMathMLContainerFrame::RebuildAutomaticDataForChildren(this);
    return rv;
  }

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus)
  {
    if (mScriptStyleChanged) {
      mScriptStyleChanged = PR_FALSE;
      nsMathMLContainerFrame::PropagateScriptStyleFor(this, 0);
    }
    return nsInlineFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  }

  NS_IMETHOD
  AppendFrames(nsIAtom*        aListName,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName, "internal error");
    nsresult rv = nsInlineFrame::AppendFrames(aListName, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList)
  {
    NS_ASSERTION(!aListName, "internal error");
    nsresult rv = nsInlineFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
    nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIAtom*        aListName,
              nsIFrame*       aOldFrame)
  {
    NS_ASSERTION(!aListName, "internal error");
    nsresult rv = nsInlineFrame::RemoveFrame(aListName, aOldFrame);
    nsMathMLContainerFrame::ReLayoutChildren(this);
    return rv;
  }

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const {
    
    if (aFlags & (nsIFrame::eBidiInlineContainer))
      return PR_FALSE;
    return nsInlineFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

protected:
  nsMathMLmathInlineFrame(nsStyleContext* aContext) : nsInlineFrame(aContext) {}
  virtual ~nsMathMLmathInlineFrame() {}

  NS_IMETHOD
  DidSetStyleContext()
  {
    mScriptStyleChanged = PR_TRUE;
    return nsInlineFrame::DidSetStyleContext();
  }

  PRBool mScriptStyleChanged;
};

#endif 
