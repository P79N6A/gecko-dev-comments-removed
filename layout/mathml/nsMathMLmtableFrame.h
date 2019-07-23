





































#ifndef nsMathMLmtableFrame_h___
#define nsMathMLmtableFrame_h___

#include "nsCOMPtr.h"
#include "nsMathMLContainerFrame.h"





class nsMathMLmtableOuterFrame : public nsTableOuterFrame,
                                 public nsMathMLFrame
{
public:
  friend nsIFrame* NS_NewMathMLmtableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  

  NS_IMETHOD
  InheritAutomaticData(nsIFrame* aParent);

  NS_IMETHOD
  UpdatePresentationData(PRUint32 aFlagsValues,
                         PRUint32 aWhichFlags);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(PRInt32         aFirstIndex,
                                    PRInt32         aLastIndex,
                                    PRUint32        aFlagsValues,
                                    PRUint32        aWhichFlags);

  

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  NS_IMETHOD
  AttributeChanged(PRInt32  aNameSpaceID,
                   nsIAtom* aAttribute,
                   PRInt32  aModType);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsTableOuterFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

protected:
  nsMathMLmtableOuterFrame(nsStyleContext* aContext) : nsTableOuterFrame(aContext) {}
  virtual ~nsMathMLmtableOuterFrame();

  
  
  
  nsIFrame*
  GetRowFrameAt(nsPresContext* aPresContext,
                PRInt32         aRowIndex);
}; 



class nsMathMLmtableFrame : public nsTableFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmtableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  

  NS_IMETHOD
  SetInitialChildList(nsIAtom*  aListName,
                      nsFrameList& aChildList);

  NS_IMETHOD
  AppendFrames(nsIAtom*  aListName,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableFrame::AppendFrames(aListName, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*  aListName,
               nsIFrame* aPrevFrame,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIAtom*  aListName,
              nsIFrame* aOldFrame)
  {
    nsresult rv = nsTableFrame::RemoveFrame(aListName, aOldFrame);
    RestyleTable();
    return rv;
  }

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsTableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

  
  
  
  void RestyleTable();

protected:
  nsMathMLmtableFrame(nsStyleContext* aContext) : nsTableFrame(aContext) {}
  virtual ~nsMathMLmtableFrame();
}; 



class nsMathMLmtrFrame : public nsTableRowFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmtrFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  

  NS_IMETHOD
  AttributeChanged(PRInt32  aNameSpaceID,
                   nsIAtom* aAttribute,
                   PRInt32  aModType);

  NS_IMETHOD
  AppendFrames(nsIAtom*  aListName,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableRowFrame::AppendFrames(aListName, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  InsertFrames(nsIAtom*  aListName,
               nsIFrame* aPrevFrame,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableRowFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(nsIAtom*  aListName,
              nsIFrame* aOldFrame)
  {
    nsresult rv = nsTableRowFrame::RemoveFrame(aListName, aOldFrame);
    RestyleTable();
    return rv;
  }

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsTableRowFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

  
  void RestyleTable()
  {
    nsTableFrame* tableFrame = nsTableFrame::GetTableFrame(this);
    if (tableFrame && tableFrame->IsFrameOfType(nsIFrame::eMathML)) {
      
      ((nsMathMLmtableFrame*)tableFrame)->RestyleTable();
    }
  }

protected:
  nsMathMLmtrFrame(nsStyleContext* aContext) : nsTableRowFrame(aContext) {}
  virtual ~nsMathMLmtrFrame();
}; 



class nsMathMLmtdFrame : public nsTableCellFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmtdFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  

  NS_IMETHOD
  AttributeChanged(PRInt32  aNameSpaceID,
                   nsIAtom* aAttribute,
                   PRInt32  aModType);

  virtual PRInt32 GetRowSpan();
  virtual PRInt32 GetColSpan();
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsTableCellFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

protected:
  nsMathMLmtdFrame(nsStyleContext* aContext) : nsTableCellFrame(aContext) {}
  virtual ~nsMathMLmtdFrame();
}; 



class nsMathMLmtdInnerFrame : public nsBlockFrame,
                              public nsMathMLFrame {
public:
  friend nsIFrame* NS_NewMathMLmtdInnerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

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

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

protected:
  nsMathMLmtdInnerFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {}
  virtual ~nsMathMLmtdInnerFrame();

  virtual PRIntn GetSkipSides() const { return 0; }
};  

#endif 
