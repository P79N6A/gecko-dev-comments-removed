










































#ifndef nsMathMLForeignFrameWrapper_h___
#define nsMathMLForeignFrameWrapper_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"

class nsMathMLForeignFrameWrapper : public nsBlockFrame,
                                    public nsMathMLFrame {
public:
  friend nsIFrame* NS_NewMathMLForeignFrameWrapper(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_ISUPPORTS_INHERITED

  

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRInt32         aScriptLevelIncrement,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
  {
    nsMathMLContainerFrame::PropagatePresentationDataFromChildAt(this,
      aFirstIndex, aLastIndex, aScriptLevelIncrement, aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }

  NS_IMETHOD
  ReResolveScriptStyle(PRInt32 aParentScriptLevel)
  {
    nsMathMLContainerFrame::PropagateScriptStyleFor(this, aParentScriptLevel);
    return NS_OK;
  }

  

#ifdef NS_DEBUG
  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsIFrame*       aChildList)
  {
    nsresult rv = nsBlockFrame::SetInitialChildList(aListName, aChildList);
    
    nsFrameList frameList(aChildList);
    NS_ASSERTION(frameList.FirstChild() && frameList.GetLength() == 1,
                 "there must be one and only one child frame");
    return rv;
  }
#endif

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  
  NS_IMETHOD
  AppendFrames(nsIAtom*        aListName,
               nsIFrame*       aFrameList)
  {
    NS_NOTREACHED("unsupported operation");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsIFrame*       aFrameList)
  {
    NS_NOTREACHED("unsupported operation");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  NS_IMETHOD
  RemoveFrame(nsIAtom*        aListName,
              nsIFrame*       aOldFrame)
  {
    return mParent->RemoveFrame(aListName, this);
  }

protected:
  nsMathMLForeignFrameWrapper(nsStyleContext* aContext) : nsBlockFrame(aContext) {}
  virtual ~nsMathMLForeignFrameWrapper() {}
};

#endif 
