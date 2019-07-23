










































#ifndef nsMathMLForeignFrameWrapper_h___
#define nsMathMLForeignFrameWrapper_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"

class nsMathMLForeignFrameWrapper : public nsBlockFrame,
                                    public nsMathMLFrame {
public:
  friend nsIFrame* NS_NewMathMLForeignFrameWrapper(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aFlagsToUpdate)
  {
    nsMathMLContainerFrame::PropagatePresentationDataFromChildAt(this,
      aFirstIndex, aLastIndex, aFlagsValues, aFlagsToUpdate);
    return NS_OK;
  }

  

#ifdef NS_DEBUG
  NS_IMETHOD
  SetInitialChildList(nsIAtom*        aListName,
                      nsFrameList&    aChildList)
  {
    NS_ASSERTION(aChildList.NotEmpty() && aChildList.GetLength() == 1,
                 "there must be one and only one child frame");
    return nsBlockFrame::SetInitialChildList(aListName, aChildList);
  }
#endif

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  
  NS_IMETHOD
  AppendFrames(nsIAtom*        aListName,
               nsFrameList&    aFrameList)
  {
    NS_NOTREACHED("unsupported operation");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*        aListName,
               nsIFrame*       aPrevFrame,
               nsFrameList&    aFrameList)
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
