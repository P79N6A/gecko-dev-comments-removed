




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
  UpdatePresentationData(uint32_t aFlagsValues,
                         uint32_t aWhichFlags);

  NS_IMETHOD
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aWhichFlags);

  

  NS_IMETHOD
  Reflow(nsPresContext*          aPresContext,
         nsHTMLReflowMetrics&     aDesiredSize,
         const nsHTMLReflowState& aReflowState,
         nsReflowStatus&          aStatus);

  NS_IMETHOD
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType);

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsTableOuterFrame::IsFrameOfType(aFlags & ~(nsIFrame::eMathML));
  }

protected:
  nsMathMLmtableOuterFrame(nsStyleContext* aContext) : nsTableOuterFrame(aContext) {}
  virtual ~nsMathMLmtableOuterFrame();

  
  
  
  nsIFrame*
  GetRowFrameAt(nsPresContext* aPresContext,
                int32_t         aRowIndex);
}; 



class nsMathMLmtableFrame : public nsTableFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewMathMLmtableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  

  NS_IMETHOD
  SetInitialChildList(ChildListID  aListID,
                      nsFrameList& aChildList);

  NS_IMETHOD
  AppendFrames(ChildListID  aListID,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableFrame::AppendFrames(aListID, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  InsertFrames(ChildListID aListID,
               nsIFrame* aPrevFrame,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(ChildListID aListID,
              nsIFrame* aOldFrame)
  {
    nsresult rv = nsTableFrame::RemoveFrame(aListID, aOldFrame);
    RestyleTable();
    return rv;
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const
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
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType);

  NS_IMETHOD
  AppendFrames(ChildListID  aListID,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableRowFrame::AppendFrames(aListID, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  InsertFrames(ChildListID aListID,
               nsIFrame* aPrevFrame,
               nsFrameList& aFrameList)
  {
    nsresult rv = nsTableRowFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
    RestyleTable();
    return rv;
  }

  NS_IMETHOD
  RemoveFrame(ChildListID aListID,
              nsIFrame* aOldFrame)
  {
    nsresult rv = nsTableRowFrame::RemoveFrame(aListID, aOldFrame);
    RestyleTable();
    return rv;
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const
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
  AttributeChanged(int32_t  aNameSpaceID,
                   nsIAtom* aAttribute,
                   int32_t  aModType);

  virtual int32_t GetRowSpan();
  virtual int32_t GetColSpan();
  virtual bool IsFrameOfType(uint32_t aFlags) const
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
  UpdatePresentationDataFromChildAt(int32_t         aFirstIndex,
                                    int32_t         aLastIndex,
                                    uint32_t        aFlagsValues,
                                    uint32_t        aFlagsToUpdate)
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

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsBlockFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eMathML | nsIFrame::eExcludesIgnorableWhitespace));
  }

protected:
  nsMathMLmtdInnerFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {}
  virtual ~nsMathMLmtdInnerFrame();

  virtual int GetSkipSides() const { return 0; }
};  

#endif 
