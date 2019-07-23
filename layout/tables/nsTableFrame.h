



































#ifndef nsTableFrame_h__
#define nsTableFrame_h__

#include "nscore.h"
#include "nsTPtrArray.h"
#include "nsHTMLContainerFrame.h"
#include "nsStyleCoord.h"
#include "nsStyleConsts.h"
#include "nsITableLayout.h"
#include "nsTableColFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsCellMap.h"
#include "nsGkAtoms.h"
#include "nsDisplayList.h"

class nsTableCellFrame;
class nsTableColFrame;
class nsTableRowGroupFrame;
class nsTableRowFrame;
class nsTableColGroupFrame;
class nsITableLayoutStrategy;
class nsStyleContext;

struct nsTableReflowState;
struct nsStylePosition;





#define NS_TABLE_FRAME_COLGROUP_LIST_INDEX 0
#define NS_TABLE_FRAME_OVERFLOW_LIST_INDEX 1
#define NS_TABLE_FRAME_LAST_LIST_INDEX    NS_TABLE_FRAME_OVERFLOW_LIST_INDEX

static inline PRBool IS_TABLE_CELL(nsIAtom* frameType) {
  return nsGkAtoms::tableCellFrame == frameType ||
    nsGkAtoms::bcTableCellFrame == frameType;
}

class nsDisplayTableItem : public nsDisplayItem
{
public:
  nsDisplayTableItem(nsIFrame* aFrame) : nsDisplayItem(aFrame),
      mPartHasFixedBackground(PR_FALSE) {}

  virtual PRBool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder);
  
  
  
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);

  void UpdateForFrameBackground(nsIFrame* aFrame);

private:
  PRPackedBool mPartHasFixedBackground;
};

class nsAutoPushCurrentTableItem
{
public:
  nsAutoPushCurrentTableItem() : mBuilder(nsnull) {}
  
  void Push(nsDisplayListBuilder* aBuilder, nsDisplayTableItem* aPushItem)
  {
    mBuilder = aBuilder;
    mOldCurrentItem = aBuilder->GetCurrentTableItem();
    aBuilder->SetCurrentTableItem(aPushItem);
#ifdef DEBUG
    mPushedItem = aPushItem;
#endif
  }
  ~nsAutoPushCurrentTableItem() {
    if (!mBuilder)
      return;
#ifdef DEBUG
    NS_ASSERTION(mBuilder->GetCurrentTableItem() == mPushedItem,
                 "Someone messed with the current table item behind our back!");
#endif
    mBuilder->SetCurrentTableItem(mOldCurrentItem);
  }

private:
  nsDisplayListBuilder* mBuilder;
  nsDisplayTableItem*   mOldCurrentItem;
#ifdef DEBUG
  nsDisplayTableItem*   mPushedItem;
#endif
};













class nsTableFrame : public nsHTMLContainerFrame, public nsITableLayout
{
public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  friend class nsTableOuterFrame;

  




  friend nsIFrame* NS_NewTableFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  


  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);


  static void* GetProperty(nsIFrame*            aFrame,
                           nsIAtom*             aPropertyName,
                           PRBool               aCreateIfNecessary = PR_FALSE);

  static float GetTwipsToPixels(nsPresContext* aPresContext);

  
  
  static PRBool AncestorsHaveStyleHeight(const nsHTMLReflowState& aParentReflowState);

  
  
  static void CheckRequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState);

  
  
  static void RequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState);

  virtual PRBool IsContainingBlock() const;

  static void RePositionViews(nsIFrame* aFrame);

  static PRBool PageBreakAfter(nsIFrame& aSourceFrame,
                               nsIFrame* aNextFrame);

  nsPoint GetFirstSectionOrigin(const nsHTMLReflowState& aReflowState) const;
  


  void AttributeChangedFor(nsIFrame*       aFrame,
                           nsIContent*     aContent, 
                           nsIAtom*        aAttribute); 

  
  virtual void Destroy();
  
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsMargin GetUsedBorder() const;
  virtual nsMargin GetUsedPadding() const;

  
  nsMargin GetChildAreaOffset(const nsHTMLReflowState* aReflowState) const;

  
  static nsTableFrame* GetTableFrame(nsIFrame* aSourceFrame);
                                 
  typedef nsresult (* DisplayGenericTablePartTraversal)
      (nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
       const nsRect& aDirtyRect, const nsDisplayListSet& aLists);
  static nsresult GenericTraversal(nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
                                   const nsRect& aDirtyRect, const nsDisplayListSet& aLists);

  










  static nsresult DisplayGenericTablePart(nsDisplayListBuilder* aBuilder,
                                          nsFrame* aFrame,
                                          const nsRect& aDirtyRect,
                                          const nsDisplayListSet& aLists,
                                          nsDisplayTableItem* aDisplayItem,
                                          DisplayGenericTablePartTraversal aTraversal = GenericTraversal);

  
  
  static nsIFrame* GetFrameAtOrBefore(nsIFrame*       aParentFrame,
                                      nsIFrame*       aPriorChildFrame,
                                      nsIAtom*        aChildType);
  PRBool IsAutoWidth(PRBool* aIsPctWidth = nsnull);
  PRBool IsAutoHeight();
  static PRBool IsPctHeight(nsStyleContext* aStyleContext);
  
  


  PRBool IsRowGroup(PRInt32 aDisplayType) const;

  


  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);

  virtual nsFrameList GetChildList(nsIAtom* aListName) const;

  
  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  




  void PaintTableBorderBackground(nsIRenderingContext& aRenderingContext,
                                  const nsRect& aDirtyRect,
                                  nsPoint aPt, PRUint32 aBGPaintFlags);

  



  nsMargin GetOuterBCBorder() const;

  


  nsMargin GetIncludedOuterBCBorder() const;

  



  nsMargin GetExcludedOuterBCBorder() const;

  



  nsMargin GetDeflationForBackground(nsPresContext* aPresContext) const;

  



  nscoord GetContinuousLeftBCBorderWidth() const;
  void SetContinuousLeftBCBorderWidth(nscoord aValue);

  friend class nsDelayedCalcBCBorders;
  
  void SetBCDamageArea(const nsRect& aValue);
  PRBool BCRecalcNeeded(nsStyleContext* aOldStyleContext,
                        nsStyleContext* aNewStyleContext);
  void PaintBCBorders(nsIRenderingContext& aRenderingContext,
                      const nsRect&        aDirtyRect);

  virtual void MarkIntrinsicWidthsDirty();
  
  
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual IntrinsicWidthOffsetData
    IntrinsicWidthOffsets(nsIRenderingContext* aRenderingContext);

  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  virtual nsSize ComputeAutoSize(nsIRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, PRBool aShrinkWrap);
  



  nscoord TableShrinkWidthToFit(nsIRenderingContext *aRenderingContext,
                                nscoord aWidthInCB);

  
  













  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  nsresult ReflowTable(nsHTMLReflowMetrics&     aDesiredSize,
                       const nsHTMLReflowState& aReflowState,
                       nscoord                  aAvailHeight,
                       nsIFrame*&               aLastChildReflowed,
                       nsReflowStatus&          aStatus);

  nsFrameList& GetColGroups();

  NS_IMETHOD GetParentStyleContextFrame(nsPresContext* aPresContext,
                                        nsIFrame**      aProviderFrame,
                                        PRBool*         aIsChild);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  
  virtual PRInt32 GetColumnWidth(PRInt32 aColIndex);

  
  virtual void SetColumnWidth(PRInt32 aColIndex, nscoord aWidth);

  
  virtual nscoord GetCellSpacingX();

  
  virtual nscoord GetCellSpacingY();
 
  virtual nscoord GetBaseline() const;
  










  virtual PRInt32  GetEffectiveRowSpan(PRInt32                 aStartRowIndex,
                                       const nsTableCellFrame& aCell) const;
  virtual PRInt32  GetEffectiveRowSpan(const nsTableCellFrame& aCell,
                                       nsCellMap*              aCellMap = nsnull);

  







  virtual PRInt32  GetEffectiveColSpan(const nsTableCellFrame& aCell,
                                       nsCellMap*              aCellMap = nsnull) const;

  


  PRBool HasMoreThanOneCell(PRInt32 aRowIndex) const;

  


  PRInt32 GetEffectiveCOLSAttribute();

  



  nsTableColFrame* GetColFrame(PRInt32 aColIndex) const;

  




  void InsertCol(nsTableColFrame& aColFrame,
                 PRInt32          aColIndex);

  nsTableColGroupFrame* CreateAnonymousColGroupFrame(nsTableColGroupType aType);

  PRInt32 DestroyAnonymousColFrames(PRInt32 aNumFrames);

  
  
  
  void AppendAnonymousColFrames(PRInt32 aNumColsToAdd);

  
  
  
  void AppendAnonymousColFrames(nsTableColGroupFrame* aColGroupFrame,
                                PRInt32               aNumColsToAdd,
                                nsTableColType        aColType,
                                PRBool                aAddToTable);

  void MatchCellMapToColCache(nsTableCellMap* aCellMap);
  
  void ClearColCache();

  void DidResizeColumns();

  virtual void AppendCell(nsTableCellFrame& aCellFrame,
                          PRInt32           aRowIndex);

  virtual void InsertCells(nsTArray<nsTableCellFrame*>& aCellFrames,
                           PRInt32                      aRowIndex,
                           PRInt32                      aColIndexBefore);

  virtual void RemoveCell(nsTableCellFrame* aCellFrame,
                          PRInt32           aRowIndex);

  void AppendRows(nsTableRowGroupFrame&       aRowGroupFrame,
                  PRInt32                     aRowIndex,
                  nsTArray<nsTableRowFrame*>& aRowFrames);

  PRInt32 InsertRow(nsTableRowGroupFrame& aRowGroupFrame,
                    nsIFrame&             aFrame,
                    PRInt32               aRowIndex,
                    PRBool                aConsiderSpans);

  PRInt32 InsertRows(nsTableRowGroupFrame&       aRowGroupFrame,
                     nsTArray<nsTableRowFrame*>& aFrames,
                     PRInt32                     aRowIndex,
                     PRBool                      aConsiderSpans);

  virtual void RemoveRows(nsTableRowFrame& aFirstRowFrame,
                          PRInt32          aNumRowsToRemove,
                          PRBool           aConsiderSpans);

  


  void InsertRowGroups(const nsFrameList::Slice& aRowGroups);

  void InsertColGroups(PRInt32                   aStartColIndex,
                       const nsFrameList::Slice& aColgroups);

  virtual void RemoveCol(nsTableColGroupFrame* aColGroupFrame,
                         PRInt32               aColIndex,
                         PRBool                aRemoveFromCache,
                         PRBool                aRemoveFromCellMap);

  NS_IMETHOD GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn, PRInt32 *aIndex);
  NS_IMETHOD GetRowAndColumnByIndex(PRInt32 aIndex, PRInt32 *aRow, PRInt32 *aColumn);

  PRBool ColumnHasCellSpacingBefore(PRInt32 aColIndex) const;

  PRBool HasPctCol() const;
  void SetHasPctCol(PRBool aValue);

  PRBool HasCellSpanningPctCol() const;
  void SetHasCellSpanningPctCol(PRBool aValue);

  











  static void InvalidateFrame(nsIFrame* aFrame,
                              const nsRect& aOrigRect,
                              const nsRect& aOrigOverflowRect,
                              PRBool aIsFirstReflow);

protected:

  


  nsTableFrame(nsStyleContext* aContext);

  
  virtual ~nsTableFrame();

  void InitChildReflowState(nsHTMLReflowState& aReflowState);

  
  virtual PRIntn GetSkipSides() const;

public:
  PRBool IsRowInserted() const;
  void   SetRowInserted(PRBool aValue);

protected:
    
  
  
  
  nsresult SetupHeaderFooterChild(const nsTableReflowState& aReflowState,
                                  nsTableRowGroupFrame* aFrame,
                                  nscoord* aDesiredHeight);

  NS_METHOD ReflowChildren(nsTableReflowState&  aReflowState,
                           nsReflowStatus&      aStatus,
                           nsIFrame*&           aLastChildReflowed,
                           nsRect&              aOverflowArea);

  
  
  
  void ReflowColGroups(nsIRenderingContext* aRenderingContext);

  



  nscoord GetCollapsedWidth(nsMargin aBorderPadding);

  
  




  void AdjustForCollapsingRowsCols(nsHTMLReflowMetrics& aDesiredSize,
                                   nsMargin             aBorderPadding);

  nsITableLayoutStrategy* LayoutStrategy() const {
    return static_cast<nsTableFrame*>(GetFirstInFlow())->
      mTableLayoutStrategy;
  }

private:
  

  void ProcessRowInserted(nscoord aNewHeight);

  

public:

  
  
  nscoord CalcBorderBoxHeight(const nsHTMLReflowState& aReflowState);

protected:

  
  
  
  void CalcDesiredHeight(const nsHTMLReflowState& aReflowState, nsHTMLReflowMetrics& aDesiredSize);

  
 
  void DistributeHeightToRows(const nsHTMLReflowState& aReflowState,
                              nscoord                  aAmount);

  void PlaceChild(nsTableReflowState&  aReflowState,
                  nsIFrame*            aKidFrame,
                  nsHTMLReflowMetrics& aKidDesiredSize,
                  const nsRect&        aOriginalKidRect,
                  const nsRect&        aOriginalKidOverflowRect);

  nsIFrame* GetFirstBodyRowGroupFrame();
  




  typedef nsAutoTPtrArray<nsIFrame, 8> FrameArray;
  void PushChildren(const FrameArray& aFrames, PRInt32 aPushFrom);

public:
  
  
  
  
  
  typedef nsAutoTPtrArray<nsTableRowGroupFrame, 8> RowGroupArray;
  void OrderRowGroups(RowGroupArray& aChildren) const;

  
  nsTableRowGroupFrame* GetTHead() const;

  
  nsTableRowGroupFrame* GetTFoot() const;

protected:
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRUint32 OrderRowGroups(FrameArray& aChildren,
                          nsTableRowGroupFrame** aHead,
                          nsTableRowGroupFrame** aFoot) const;

public:
  
  
  
  PRBool RowIsSpannedInto(PRInt32 aRowIndex, PRInt32 aNumEffCols);

  
  
  
  PRBool RowHasSpanningCells(PRInt32 aRowIndex, PRInt32 aNumEffCols);

  
  
  PRBool ColIsSpannedInto(PRInt32 aColIndex);

  
  
  PRBool ColHasSpanningCells(PRInt32 aColIndex);

protected:

  PRBool HaveReflowedColGroups() const;
  void   SetHaveReflowedColGroups(PRBool aValue);

public:
  PRBool IsBorderCollapse() const;

  PRBool NeedToCalcBCBorders() const;
  void SetNeedToCalcBCBorders(PRBool aValue);

  PRBool NeedToCollapse() const;
  void SetNeedToCollapse(PRBool aValue);

  PRBool HasZeroColSpans() const;
  void SetHasZeroColSpans(PRBool aValue);

  PRBool NeedColSpanExpansion() const;
  void SetNeedColSpanExpansion(PRBool aValue);

  





  void SetGeometryDirty() { mBits.mGeometryDirty = PR_TRUE; }
  void ClearGeometryDirty() { mBits.mGeometryDirty = PR_FALSE; }
  PRBool IsGeometryDirty() const { return mBits.mGeometryDirty; }

  


  virtual nsTableCellMap* GetCellMap() const;

  




  void AdjustRowIndices(PRInt32 aRowIndex,
                        PRInt32 aAdjustment);

  





  void ResetRowIndices(const nsFrameList::Slice& aRowGroupsToExclude);

  nsTArray<nsTableColFrame*>& GetColCache();

  

  static nsTableRowGroupFrame* GetRowGroupFrame(nsIFrame* aFrame,
                                                nsIAtom*  aFrameTypeIn = nsnull);

protected:

  void SetBorderCollapse(PRBool aValue);

  void CalcBCBorders();

  void ExpandBCDamageArea(nsRect& aRect) const;

  void SetColumnDimensions(nscoord         aHeight,
                           const nsMargin& aReflowState);

  PRInt32 CollectRows(nsIFrame*                   aFrame,
                      nsTArray<nsTableRowFrame*>& aCollection);

public: 

  PRInt32 GetStartRowIndex(nsTableRowGroupFrame& aRowGroupFrame);

  

  PRInt32 GetRowCount () const
  {
    return GetCellMap()->GetRowCount();
  }

  

  PRInt32 GetEffectiveColCount() const;

  
  PRInt32 GetColCount () const
  {
    return GetCellMap()->GetColCount();
  }

  
  PRInt32 GetIndexOfLastRealCol();

  
  virtual PRBool IsAutoLayout();

  
  
  
  NS_IMETHOD GetCellDataAt(PRInt32 aRowIndex, PRInt32 aColIndex, 
                           nsIDOMElement* &aCell,   
                           PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                           PRInt32& aRowSpan, PRInt32& aColSpan,
                           PRInt32& aActualRowSpan, PRInt32& aActualColSpan,
                           PRBool& aIsSelected);

  



  NS_IMETHOD GetTableSize(PRInt32& aRowCount, PRInt32& aColCount);

  

public:
 
#ifdef DEBUG
  void Dump(PRBool          aDumpRows,
            PRBool          aDumpCols, 
            PRBool          aDumpCellMap);
#endif

protected:
#ifdef DEBUG
  void DumpRowGroup(nsIFrame* aChildFrame);
#endif
  
  nsAutoTPtrArray<nsTableColFrame, 8> mColFrames;

  struct TableBits {
    PRUint32 mHaveReflowedColGroups:1; 
    PRUint32 mHasPctCol:1;             
    PRUint32 mCellSpansPctCol:1;       
    PRUint32 mIsBorderCollapse:1;      
    PRUint32 mRowInserted:1;
    PRUint32 mNeedToCalcBCBorders:1;
    PRUint32 mGeometryDirty:1;
    PRUint32 mLeftContBCBorder:8;
    PRUint32 mNeedToCollapse:1;    
    PRUint32 mHasZeroColSpans:1;
    PRUint32 mNeedColSpanExpansion:1;
    PRUint32 mResizedColumns:1;        
  } mBits;

  nsTableCellMap*         mCellMap;            
  nsITableLayoutStrategy* mTableLayoutStrategy;
  nsFrameList             mColGroups;          
};


inline PRBool nsTableFrame::IsRowGroup(PRInt32 aDisplayType) const
{
  return PRBool((NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == aDisplayType) ||
                (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == aDisplayType) ||
                (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == aDisplayType));
}

inline void nsTableFrame::SetHaveReflowedColGroups(PRBool aValue)
{
  mBits.mHaveReflowedColGroups = aValue;
}

inline PRBool nsTableFrame::HaveReflowedColGroups() const
{
  return (PRBool)mBits.mHaveReflowedColGroups;
}

inline PRBool nsTableFrame::HasPctCol() const
{
  return (PRBool)mBits.mHasPctCol;
}

inline void nsTableFrame::SetHasPctCol(PRBool aValue)
{
  mBits.mHasPctCol = (unsigned)aValue;
}

inline PRBool nsTableFrame::HasCellSpanningPctCol() const
{
  return (PRBool)mBits.mCellSpansPctCol;
}

inline void nsTableFrame::SetHasCellSpanningPctCol(PRBool aValue)
{
  mBits.mCellSpansPctCol = (unsigned)aValue;
}

inline PRBool nsTableFrame::IsRowInserted() const
{
  return (PRBool)mBits.mRowInserted;
}

inline void nsTableFrame::SetRowInserted(PRBool aValue)
{
  mBits.mRowInserted = (unsigned)aValue;
}

inline void nsTableFrame::SetNeedToCollapse(PRBool aValue)
{
  static_cast<nsTableFrame*>(GetFirstInFlow())->mBits.mNeedToCollapse = (unsigned)aValue;
}

inline PRBool nsTableFrame::NeedToCollapse() const
{
  return (PRBool) static_cast<nsTableFrame*>(GetFirstInFlow())->mBits.mNeedToCollapse;
}

inline void nsTableFrame::SetHasZeroColSpans(PRBool aValue)
{
  mBits.mHasZeroColSpans = (unsigned)aValue;
}

inline PRBool nsTableFrame::HasZeroColSpans() const
{
  return (PRBool)mBits.mHasZeroColSpans;
}

inline void nsTableFrame::SetNeedColSpanExpansion(PRBool aValue)
{
  mBits.mNeedColSpanExpansion = (unsigned)aValue;
}

inline PRBool nsTableFrame::NeedColSpanExpansion() const
{
  return (PRBool)mBits.mNeedColSpanExpansion;
}


inline nsFrameList& nsTableFrame::GetColGroups()
{
  return static_cast<nsTableFrame*>(GetFirstInFlow())->mColGroups;
}

inline nsTArray<nsTableColFrame*>& nsTableFrame::GetColCache()
{
  return mColFrames;
}

inline PRBool nsTableFrame::IsBorderCollapse() const
{
  return (PRBool)mBits.mIsBorderCollapse;
}

inline void nsTableFrame::SetBorderCollapse(PRBool aValue) 
{
  mBits.mIsBorderCollapse = aValue;
}

inline PRBool nsTableFrame::NeedToCalcBCBorders() const
{
  return (PRBool)mBits.mNeedToCalcBCBorders;
}

inline void nsTableFrame::SetNeedToCalcBCBorders(PRBool aValue)
{
  mBits.mNeedToCalcBCBorders = (unsigned)aValue;
}

inline nscoord
nsTableFrame::GetContinuousLeftBCBorderWidth() const
{
  PRInt32 aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  return BC_BORDER_RIGHT_HALF_COORD(aPixelsToTwips, mBits.mLeftContBCBorder);
}

inline void nsTableFrame::SetContinuousLeftBCBorderWidth(nscoord aValue)
{
  mBits.mLeftContBCBorder = (unsigned) aValue;
}

class nsTableIterator
{
public:
  nsTableIterator(nsIFrame& aSource);
  nsTableIterator(nsFrameList& aSource);
  nsIFrame* First();
  nsIFrame* Next();
  PRBool    IsLeftToRight();
  PRInt32   Count();

protected:
  void Init(nsIFrame* aFirstChild);
  PRBool    mLeftToRight;
  nsIFrame* mFirstListChild;
  nsIFrame* mFirstChild;
  nsIFrame* mCurrentChild;
  PRInt32   mCount;
};

#define ABORT0() \
{NS_ASSERTION(PR_FALSE, "CellIterator program error"); \
return;}

#define ABORT1(aReturn) \
{NS_ASSERTION(PR_FALSE, "CellIterator program error"); \
return aReturn;} 

#endif
