



































#ifndef nsTableColGroupFrame_h__
#define nsTableColGroupFrame_h__

#include "nscore.h"
#include "nsHTMLContainerFrame.h"
#include "nsTableColFrame.h"
#include "nsTablePainter.h"

class nsTableColFrame;

enum nsTableColGroupType {
  eColGroupContent            = 0, 
  eColGroupAnonymousCol       = 1, 
  eColGroupAnonymousCell      = 2  
};







class nsTableColGroupFrame : public nsHTMLContainerFrame
{
public:

  

  




  friend nsIFrame* NS_NewTableColGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  


  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  


  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  


  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) { return NS_OK; }

  







  nsTableColGroupType GetColType() const;

  


  void SetColType(nsTableColGroupType aType);
  
  








  static PRBool GetLastRealColGroup(nsTableFrame* aTableFrame, 
                                    nsIFrame**    aLastColGroup);

  

  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  






  void RemoveChild(nsTableColFrame& aChild,
                   PRBool           aResetSubsequentColIndices);

  





  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  

  virtual PRBool IsContainingBlock() const;

  




  virtual nsIAtom* GetType() const;

  














  nsresult AddColsToTable(PRInt32          aFirstColIndex,
                          PRBool           aResetSubsequentColIndices,
                          nsIFrame*        aFirstFrame,
                          nsIFrame*        aLastFrame = nsnull);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  void Dump(PRInt32 aIndent);
#endif

  



  virtual PRInt32 GetColCount() const;

  
  nsTableColFrame * GetFirstColumn();
  


  nsTableColFrame * GetNextColumn(nsIFrame *aChildFrame);

  


  PRInt32 GetStartColumnIndex();
  
  


  void SetStartColumnIndex(PRInt32 aIndex);

  
  PRInt32 GetSpan();

  

  nsFrameList& GetChildList();

  







  static void ResetColIndices(nsIFrame*       aFirstColGroup,
                              PRInt32         aFirstColIndex,
                              nsIFrame*       aStartColFrame = nsnull);

  





  void GetContinuousBCBorderWidth(nsMargin& aBorder);
  



  void SetContinuousBCBorderWidth(PRUint8     aForSide,
                                  BCPixelSize aPixelValue);
protected:
  nsTableColGroupFrame(nsStyleContext* aContext);

  void InsertColsReflow(PRInt32         aColIndex,
                        nsIFrame*       aFirstFrame,
                        nsIFrame*       aLastFrame = nsnull);

  
  virtual PRIntn GetSkipSides() const;

  
  PRInt32 mColCount;
  
  PRInt32 mStartColIndex;

  
  BCPixelSize mTopContBorderWidth;
  BCPixelSize mBottomContBorderWidth;
};

inline nsTableColGroupFrame::nsTableColGroupFrame(nsStyleContext *aContext)
: nsHTMLContainerFrame(aContext), mColCount(0), mStartColIndex(0)
{ 
  SetColType(eColGroupContent);
}
  
inline PRInt32 nsTableColGroupFrame::GetStartColumnIndex()
{  
  return mStartColIndex;
}

inline void nsTableColGroupFrame::SetStartColumnIndex (PRInt32 aIndex)
{
  mStartColIndex = aIndex;
}

inline PRInt32 nsTableColGroupFrame::GetColCount() const
{  
  return mColCount;
}

inline nsFrameList& nsTableColGroupFrame::GetChildList()
{  
  return mFrames;
}

#endif

