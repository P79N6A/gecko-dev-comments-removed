



































#ifndef nsTableOuterFrame_h__
#define nsTableOuterFrame_h__

#include "nscore.h"
#include "nsHTMLContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsITableLayout.h"

struct nsStyleTable;
class nsTableFrame;

class nsTableCaptionFrame : public nsBlockFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsIAtom* GetType() const;
  friend nsIFrame* NS_NewTableCaptionFrame(nsIPresShell* aPresShell, nsStyleContext*  aContext);

  virtual nsSize ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, PRBool aShrinkWrap);

  NS_IMETHOD GetParentStyleContextFrame(nsPresContext* aPresContext,
                                        nsIFrame**      aProviderFrame,
                                        PRBool*         aIsChild);
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

protected:
  nsTableCaptionFrame(nsStyleContext*  aContext);
  virtual ~nsTableCaptionFrame();
};











class nsTableOuterFrame : public nsHTMLContainerFrame, public nsITableLayout
{
public:
  using nsBox::GetMargin;

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  




  friend nsIFrame* NS_NewTableOuterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  
  

  virtual void Destroy();
  
  virtual PRBool IsContainingBlock() const;

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);
 
  virtual nsFrameList GetChildList(nsIAtom* aListName) const;

  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;

  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);

  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);

  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstChild(nsnull)->GetContentInsertionFrame();
  }

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  nsresult BuildDisplayListForInnerTable(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

  virtual nscoord GetBaseline() const;

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual nsSize ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, PRBool aShrinkWrap);

  


  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  

  void SetSelected(PRBool aSelected,
                   SelectionType aType);

  NS_IMETHOD GetParentStyleContextFrame(nsPresContext* aPresContext,
                                        nsIFrame**      aProviderFrame,
                                        PRBool*         aIsChild);

  

  
  NS_IMETHOD GetCellDataAt(PRInt32 aRowIndex, PRInt32 aColIndex, 
                           nsIDOMElement* &aCell,   
                           PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                           PRInt32& aRowSpan, PRInt32& aColSpan,
                           PRInt32& aActualRowSpan, PRInt32& aActualColSpan,
                           PRBool& aIsSelected);

  
  NS_IMETHOD GetTableSize(PRInt32& aRowCount, PRInt32& aColCount);

  NS_IMETHOD GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn, PRInt32 *aIndex);
  NS_IMETHOD GetRowAndColumnByIndex(PRInt32 aIndex, PRInt32 *aRow, PRInt32 *aColumn);

protected:


  nsTableOuterFrame(nsStyleContext* aContext);
  virtual ~nsTableOuterFrame();

  void InitChildReflowState(nsPresContext&    aPresContext,                     
                            nsHTMLReflowState& aReflowState);

  


  virtual PRIntn GetSkipSides() const;

  PRUint8 GetCaptionSide(); 

  PRBool HasSideCaption() {
    PRUint8 captionSide = GetCaptionSide();
    return captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
           captionSide == NS_STYLE_CAPTION_SIDE_RIGHT;
  }
  
  PRUint8 GetCaptionVerticalAlign();

  void SetDesiredSize(PRUint8         aCaptionSide,
                      const nsMargin& aInnerMargin,
                      const nsMargin& aCaptionMargin,
                      nscoord&        aWidth,
                      nscoord&        aHeight);

  void BalanceLeftRightCaption(PRUint8         aCaptionSide,
                               const nsMargin& aInnerMargin, 
                               const nsMargin& aCaptionMargin,
                               nscoord&        aInnerWidth,
                               nscoord&        aCaptionWidth);

  nsresult   GetCaptionOrigin(PRUint32         aCaptionSide,
                              const nsSize&    aContainBlockSize,
                              const nsSize&    aInnerSize, 
                              const nsMargin&  aInnerMargin,
                              const nsSize&    aCaptionSize,
                              nsMargin&        aCaptionMargin,
                              nsPoint&         aOrigin);

  nsresult   GetInnerOrigin(PRUint32         aCaptionSide,
                            const nsSize&    aContainBlockSize,
                            const nsSize&    aCaptionSize, 
                            const nsMargin&  aCaptionMargin,
                            const nsSize&    aInnerSize,
                            nsMargin&        aInnerMargin,
                            nsPoint&         aOrigin);
  
  
  void OuterBeginReflowChild(nsPresContext*           aPresContext,
                             nsIFrame*                aChildFrame,
                             const nsHTMLReflowState& aOuterRS,
                             void*                    aChildRSSpace,
                             nscoord                  aAvailWidth);

  nsresult OuterDoReflowChild(nsPresContext*           aPresContext,
                              nsIFrame*                aChildFrame,
                              const nsHTMLReflowState& aChildRS,
                              nsHTMLReflowMetrics&     aMetrics,
                              nsReflowStatus&          aStatus);

  
  void UpdateReflowMetrics(PRUint8              aCaptionSide,
                           nsHTMLReflowMetrics& aMet,
                           const nsMargin&      aInnerMargin,
                           const nsMargin&      aCaptionMargin);

  
  
  void GetMargin(nsPresContext*           aPresContext,
                 const nsHTMLReflowState& aOuterRS,
                 nsIFrame*                aChildFrame,
                 nscoord                  aAvailableWidth,
                 nsMargin&                aMargin);

private:
  
  nsTableFrame* mInnerTableFrame; 
  nsFrameList   mCaptionFrames;
  nsIFrame*     mCaptionFrame;
};

inline PRIntn nsTableOuterFrame::GetSkipSides() const
{ return 0; }

#endif



