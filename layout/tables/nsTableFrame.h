



































#ifndef nsTableFrame_h__
#define nsTableFrame_h__

#include "nscore.h"
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

static inline bool IS_TABLE_CELL(nsIAtom* frameType) {
  return nsGkAtoms::tableCellFrame == frameType ||
    nsGkAtoms::bcTableCellFrame == frameType;
}

class nsDisplayTableItem : public nsDisplayItem
{
public:
  nsDisplayTableItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame) : 
      nsDisplayItem(aBuilder, aFrame),
      mPartHasFixedBackground(PR_FALSE) {}

  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame);
  
  
  
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);

  void UpdateForFrameBackground(nsIFrame* aFrame);

private:
  bool mPartHasFixedBackground;
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

  static float GetTwipsToPixels(nsPresContext* aPresContext);

  
  
  static bool AncestorsHaveStyleHeight(const nsHTMLReflowState& aParentReflowState);

  
  
  static void CheckRequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState);

  
  
  static void RequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState);

  static void RePositionViews(nsIFrame* aFrame);

  static bool PageBreakAfter(nsIFrame* aSourceFrame,
                               nsIFrame* aNextFrame);

  nsPoint GetFirstSectionOrigin(const nsHTMLReflowState& aReflowState) const;
  


  void AttributeChangedFor(nsIFrame*       aFrame,
                           nsIContent*     aContent, 
                           nsIAtom*        aAttribute); 

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(ChildListID     aListID,
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
  bool IsAutoHeight();
  
  


  bool IsRowGroup(PRInt32 aDisplayType) const;

  


  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);

  virtual nsFrameList GetChildList(ChildListID aListID) const;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  




  void PaintTableBorderBackground(nsRenderingContext& aRenderingContext,
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
  bool BCRecalcNeeded(nsStyleContext* aOldStyleContext,
                        nsStyleContext* aNewStyleContext);
  void PaintBCBorders(nsRenderingContext& aRenderingContext,
                      const nsRect&        aDirtyRect);

  virtual void MarkIntrinsicWidthsDirty();
  
  
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual IntrinsicWidthOffsetData
    IntrinsicWidthOffsets(nsRenderingContext* aRenderingContext);

  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             bool aShrinkWrap);
  virtual nsSize ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap);
  



  nscoord TableShrinkWidthToFit(nsRenderingContext *aRenderingContext,
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

  virtual nsIFrame* GetParentStyleContextFrame();

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  
  virtual PRInt32 GetColumnWidth(PRInt32 aColIndex);

  
  virtual nscoord GetCellSpacingX();

  
  virtual nscoord GetCellSpacingY();
 
  virtual nscoord GetBaseline() const;
  










  virtual PRInt32  GetEffectiveRowSpan(PRInt32                 aStartRowIndex,
                                       const nsTableCellFrame& aCell) const;
  virtual PRInt32  GetEffectiveRowSpan(const nsTableCellFrame& aCell,
                                       nsCellMap*              aCellMap = nsnull);

  







  virtual PRInt32  GetEffectiveColSpan(const nsTableCellFrame& aCell,
                                       nsCellMap*              aCellMap = nsnull) const;

  


  bool HasMoreThanOneCell(PRInt32 aRowIndex) const;

  



  nsTableColFrame* GetColFrame(PRInt32 aColIndex) const;

  




  void InsertCol(nsTableColFrame& aColFrame,
                 PRInt32          aColIndex);

  nsTableColGroupFrame* CreateAnonymousColGroupFrame(nsTableColGroupType aType);

  PRInt32 DestroyAnonymousColFrames(PRInt32 aNumFrames);

  
  
  
  void AppendAnonymousColFrames(PRInt32 aNumColsToAdd);

  
  
  
  void AppendAnonymousColFrames(nsTableColGroupFrame* aColGroupFrame,
                                PRInt32               aNumColsToAdd,
                                nsTableColType        aColType,
                                bool                  aAddToTable);

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

  void AppendRows(nsTableRowGroupFrame*       aRowGroupFrame,
                  PRInt32                     aRowIndex,
                  nsTArray<nsTableRowFrame*>& aRowFrames);

  PRInt32 InsertRows(nsTableRowGroupFrame*       aRowGroupFrame,
                     nsTArray<nsTableRowFrame*>& aFrames,
                     PRInt32                     aRowIndex,
                     bool                        aConsiderSpans);

  virtual void RemoveRows(nsTableRowFrame& aFirstRowFrame,
                          PRInt32          aNumRowsToRemove,
                          bool             aConsiderSpans);

  


  void InsertRowGroups(const nsFrameList::Slice& aRowGroups);

  void InsertColGroups(PRInt32                   aStartColIndex,
                       const nsFrameList::Slice& aColgroups);

  virtual void RemoveCol(nsTableColGroupFrame* aColGroupFrame,
                         PRInt32               aColIndex,
                         bool                  aRemoveFromCache,
                         bool                  aRemoveFromCellMap);

  NS_IMETHOD GetIndexByRowAndColumn(PRInt32 aRow, PRInt32 aColumn, PRInt32 *aIndex);
  NS_IMETHOD GetRowAndColumnByIndex(PRInt32 aIndex, PRInt32 *aRow, PRInt32 *aColumn);

  bool ColumnHasCellSpacingBefore(PRInt32 aColIndex) const;

  bool HasPctCol() const;
  void SetHasPctCol(bool aValue);

  bool HasCellSpanningPctCol() const;
  void SetHasCellSpanningPctCol(bool aValue);

  











  static void InvalidateFrame(nsIFrame* aFrame,
                              const nsRect& aOrigRect,
                              const nsRect& aOrigVisualOverflow,
                              bool aIsFirstReflow);

protected:

  


  nsTableFrame(nsStyleContext* aContext);

  
  virtual ~nsTableFrame();

  void InitChildReflowState(nsHTMLReflowState& aReflowState);

  
  virtual PRIntn GetSkipSides() const;

public:
  bool IsRowInserted() const;
  void   SetRowInserted(bool aValue);

protected:
    
  
  
  
  nsresult SetupHeaderFooterChild(const nsTableReflowState& aReflowState,
                                  nsTableRowGroupFrame* aFrame,
                                  nscoord* aDesiredHeight);

  nsresult ReflowChildren(nsTableReflowState&  aReflowState,
                          nsReflowStatus&      aStatus,
                          nsIFrame*&           aLastChildReflowed,
                          nsOverflowAreas&     aOverflowAreas);

  
  
  
  void ReflowColGroups(nsRenderingContext* aRenderingContext);

  



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
                  const nsRect&        aOriginalKidVisualOverflow);
   void PlaceRepeatedFooter(nsTableReflowState& aReflowState,
                            nsTableRowGroupFrame *aTfoot,
                            nscoord aFooterHeight);

  nsIFrame* GetFirstBodyRowGroupFrame();
public:
  typedef nsAutoTArray<nsTableRowGroupFrame*, 8> RowGroupArray;
  




protected:
  void PushChildren(const RowGroupArray& aRowGroups, PRInt32 aPushFrom);

public:
  
  
  
  
  

  void OrderRowGroups(RowGroupArray& aChildren,
                      nsTableRowGroupFrame** aHead = nsnull,
                      nsTableRowGroupFrame** aFoot = nsnull) const;

  
  nsTableRowGroupFrame* GetTHead() const;

  
  nsTableRowGroupFrame* GetTFoot() const;

  
  
  
  bool RowIsSpannedInto(PRInt32 aRowIndex, PRInt32 aNumEffCols);

  
  
  
  bool RowHasSpanningCells(PRInt32 aRowIndex, PRInt32 aNumEffCols);

protected:

  bool HaveReflowedColGroups() const;
  void   SetHaveReflowedColGroups(bool aValue);

public:
  bool IsBorderCollapse() const;

  bool NeedToCalcBCBorders() const;
  void SetNeedToCalcBCBorders(bool aValue);

  bool NeedToCollapse() const;
  void SetNeedToCollapse(bool aValue);

  bool HasZeroColSpans() const;
  void SetHasZeroColSpans(bool aValue);

  bool NeedColSpanExpansion() const;
  void SetNeedColSpanExpansion(bool aValue);

  





  void SetGeometryDirty() { mBits.mGeometryDirty = PR_TRUE; }
  void ClearGeometryDirty() { mBits.mGeometryDirty = PR_FALSE; }
  bool IsGeometryDirty() const { return mBits.mGeometryDirty; }

  


  virtual nsTableCellMap* GetCellMap() const;

  




  void AdjustRowIndices(PRInt32 aRowIndex,
                        PRInt32 aAdjustment);

  





  void ResetRowIndices(const nsFrameList::Slice& aRowGroupsToExclude);

  nsTArray<nsTableColFrame*>& GetColCache();

protected:

  void SetBorderCollapse(bool aValue);

  void CalcBCBorders();

  void ExpandBCDamageArea(nsRect& aRect) const;

  void SetColumnDimensions(nscoord         aHeight,
                           const nsMargin& aReflowState);

  PRInt32 CollectRows(nsIFrame*                   aFrame,
                      nsTArray<nsTableRowFrame*>& aCollection);

public: 

  PRInt32 GetStartRowIndex(nsTableRowGroupFrame* aRowGroupFrame);

  

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

  
  virtual bool IsAutoLayout();

  
  
  
  NS_IMETHOD GetCellDataAt(PRInt32 aRowIndex, PRInt32 aColIndex, 
                           nsIDOMElement* &aCell,   
                           PRInt32& aStartRowIndex, PRInt32& aStartColIndex, 
                           PRInt32& aRowSpan, PRInt32& aColSpan,
                           PRInt32& aActualRowSpan, PRInt32& aActualColSpan,
                           bool& aIsSelected);

  



  NS_IMETHOD GetTableSize(PRInt32& aRowCount, PRInt32& aColCount);

  

public:
 
#ifdef DEBUG
  void Dump(bool            aDumpRows,
            bool            aDumpCols, 
            bool            aDumpCellMap);
#endif

protected:
#ifdef DEBUG
  void DumpRowGroup(nsIFrame* aChildFrame);
#endif
  
  nsAutoTArray<nsTableColFrame*, 8> mColFrames;

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


inline bool nsTableFrame::IsRowGroup(PRInt32 aDisplayType) const
{
  return bool((NS_STYLE_DISPLAY_TABLE_HEADER_GROUP == aDisplayType) ||
                (NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP == aDisplayType) ||
                (NS_STYLE_DISPLAY_TABLE_ROW_GROUP    == aDisplayType));
}

inline void nsTableFrame::SetHaveReflowedColGroups(bool aValue)
{
  mBits.mHaveReflowedColGroups = aValue;
}

inline bool nsTableFrame::HaveReflowedColGroups() const
{
  return (bool)mBits.mHaveReflowedColGroups;
}

inline bool nsTableFrame::HasPctCol() const
{
  return (bool)mBits.mHasPctCol;
}

inline void nsTableFrame::SetHasPctCol(bool aValue)
{
  mBits.mHasPctCol = (unsigned)aValue;
}

inline bool nsTableFrame::HasCellSpanningPctCol() const
{
  return (bool)mBits.mCellSpansPctCol;
}

inline void nsTableFrame::SetHasCellSpanningPctCol(bool aValue)
{
  mBits.mCellSpansPctCol = (unsigned)aValue;
}

inline bool nsTableFrame::IsRowInserted() const
{
  return (bool)mBits.mRowInserted;
}

inline void nsTableFrame::SetRowInserted(bool aValue)
{
  mBits.mRowInserted = (unsigned)aValue;
}

inline void nsTableFrame::SetNeedToCollapse(bool aValue)
{
  static_cast<nsTableFrame*>(GetFirstInFlow())->mBits.mNeedToCollapse = (unsigned)aValue;
}

inline bool nsTableFrame::NeedToCollapse() const
{
  return (bool) static_cast<nsTableFrame*>(GetFirstInFlow())->mBits.mNeedToCollapse;
}

inline void nsTableFrame::SetHasZeroColSpans(bool aValue)
{
  mBits.mHasZeroColSpans = (unsigned)aValue;
}

inline bool nsTableFrame::HasZeroColSpans() const
{
  return (bool)mBits.mHasZeroColSpans;
}

inline void nsTableFrame::SetNeedColSpanExpansion(bool aValue)
{
  mBits.mNeedColSpanExpansion = (unsigned)aValue;
}

inline bool nsTableFrame::NeedColSpanExpansion() const
{
  return (bool)mBits.mNeedColSpanExpansion;
}


inline nsFrameList& nsTableFrame::GetColGroups()
{
  return static_cast<nsTableFrame*>(GetFirstInFlow())->mColGroups;
}

inline nsTArray<nsTableColFrame*>& nsTableFrame::GetColCache()
{
  return mColFrames;
}

inline bool nsTableFrame::IsBorderCollapse() const
{
  return (bool)mBits.mIsBorderCollapse;
}

inline void nsTableFrame::SetBorderCollapse(bool aValue) 
{
  mBits.mIsBorderCollapse = aValue;
}

inline bool nsTableFrame::NeedToCalcBCBorders() const
{
  return (bool)mBits.mNeedToCalcBCBorders;
}

inline void nsTableFrame::SetNeedToCalcBCBorders(bool aValue)
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
  bool      IsLeftToRight();
  PRInt32   Count();

protected:
  void Init(nsIFrame* aFirstChild);
  bool      mLeftToRight;
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
