



#ifndef nsTableFrame_h__
#define nsTableFrame_h__

#include "celldata.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsStyleCoord.h"
#include "nsStyleConsts.h"
#include "nsTableColFrame.h"
#include "nsTableColGroupFrame.h"
#include "nsCellMap.h"
#include "nsGkAtoms.h"
#include "nsDisplayList.h"

class nsTableCellFrame;
class nsTableCellMap;
class nsTableColFrame;
class nsColGroupFrame;
class nsTableRowGroupFrame;
class nsTableRowFrame;
class nsTableColGroupFrame;
class nsITableLayoutStrategy;
class nsStyleContext;

struct nsTableReflowState;
struct nsStylePosition;
struct BCPropertyData;

static inline bool IS_TABLE_CELL(nsIAtom* frameType) {
  return nsGkAtoms::tableCellFrame == frameType ||
    nsGkAtoms::bcTableCellFrame == frameType;
}

static inline bool FrameHasBorderOrBackground(nsIFrame* f) {
  return (f->GetStyleVisibility()->IsVisible() &&
          (!f->GetStyleBackground()->IsTransparent() ||
           f->GetStyleDisplay()->mAppearance ||
           f->GetStyleBorder()->HasBorder()));
}

class nsDisplayTableItem : public nsDisplayItem
{
public:
  nsDisplayTableItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame) : 
      nsDisplayItem(aBuilder, aFrame),
      mPartHasFixedBackground(false) {}

  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame);
  
  
  
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);

  void UpdateForFrameBackground(nsIFrame* aFrame);

private:
  bool mPartHasFixedBackground;
};

class nsAutoPushCurrentTableItem
{
public:
  nsAutoPushCurrentTableItem() : mBuilder(nullptr) {}
  
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











class nsTableFrame : public nsContainerFrame
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
  virtual nsMargin GetUsedMargin() const;

  
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
  
  


  bool IsRowGroup(int32_t aDisplayType) const;

  


  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);

  virtual const nsFrameList& GetChildList(ChildListID aListID) const;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  




  void PaintTableBorderBackground(nsRenderingContext& aRenderingContext,
                                  const nsRect& aDirtyRect,
                                  nsPoint aPt, uint32_t aBGPaintFlags);

  



  nsMargin GetOuterBCBorder() const;

  


  nsMargin GetIncludedOuterBCBorder() const;

  



  nsMargin GetExcludedOuterBCBorder() const;

  



  nsMargin GetDeflationForBackground(nsPresContext* aPresContext) const;

  



  nscoord GetContinuousLeftBCBorderWidth() const;
  void SetContinuousLeftBCBorderWidth(nscoord aValue);

  friend class nsDelayedCalcBCBorders;
  
  void AddBCDamageArea(const nsIntRect& aValue);
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
                             uint32_t aFlags) MOZ_OVERRIDE;
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

  virtual nsIFrame* GetParentStyleContextFrame() const;

  




  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    if (aFlags & eSupportsCSSTransforms) {
      return false;
    }
    return nsContainerFrame::IsFrameOfType(aFlags);
  }

#ifdef DEBUG
  
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  
  int32_t GetColumnWidth(int32_t aColIndex);

  
  nscoord GetCellSpacingX();

  
  nscoord GetCellSpacingY();
 
  virtual nscoord GetBaseline() const MOZ_OVERRIDE;
  










  int32_t  GetEffectiveRowSpan(int32_t                 aStartRowIndex,
                               const nsTableCellFrame& aCell) const;
  int32_t  GetEffectiveRowSpan(const nsTableCellFrame& aCell,
                               nsCellMap*              aCellMap = nullptr);

  







  int32_t  GetEffectiveColSpan(const nsTableCellFrame& aCell,
                               nsCellMap*              aCellMap = nullptr) const;

  


  bool HasMoreThanOneCell(int32_t aRowIndex) const;

  



  nsTableColFrame* GetColFrame(int32_t aColIndex) const;

  




  void InsertCol(nsTableColFrame& aColFrame,
                 int32_t          aColIndex);

  nsTableColGroupFrame* CreateAnonymousColGroupFrame(nsTableColGroupType aType);

  int32_t DestroyAnonymousColFrames(int32_t aNumFrames);

  
  
  
  void AppendAnonymousColFrames(int32_t aNumColsToAdd);

  
  
  
  void AppendAnonymousColFrames(nsTableColGroupFrame* aColGroupFrame,
                                int32_t               aNumColsToAdd,
                                nsTableColType        aColType,
                                bool                  aAddToTable);

  void MatchCellMapToColCache(nsTableCellMap* aCellMap);
  
  void ClearColCache();

  void DidResizeColumns();

  void AppendCell(nsTableCellFrame& aCellFrame,
                  int32_t           aRowIndex);

  void InsertCells(nsTArray<nsTableCellFrame*>& aCellFrames,
                   int32_t                      aRowIndex,
                   int32_t                      aColIndexBefore);

  void RemoveCell(nsTableCellFrame* aCellFrame,
                  int32_t           aRowIndex);

  void AppendRows(nsTableRowGroupFrame*       aRowGroupFrame,
                  int32_t                     aRowIndex,
                  nsTArray<nsTableRowFrame*>& aRowFrames);

  int32_t InsertRows(nsTableRowGroupFrame*       aRowGroupFrame,
                     nsTArray<nsTableRowFrame*>& aFrames,
                     int32_t                     aRowIndex,
                     bool                        aConsiderSpans);

  void RemoveRows(nsTableRowFrame& aFirstRowFrame,
                  int32_t          aNumRowsToRemove,
                  bool             aConsiderSpans);

  


  void InsertRowGroups(const nsFrameList::Slice& aRowGroups);

  void InsertColGroups(int32_t                   aStartColIndex,
                       const nsFrameList::Slice& aColgroups);

  void RemoveCol(nsTableColGroupFrame* aColGroupFrame,
                 int32_t               aColIndex,
                 bool                  aRemoveFromCache,
                 bool                  aRemoveFromCellMap);

  bool ColumnHasCellSpacingBefore(int32_t aColIndex) const;

  bool HasPctCol() const;
  void SetHasPctCol(bool aValue);

  bool HasCellSpanningPctCol() const;
  void SetHasCellSpanningPctCol(bool aValue);

  











  static void InvalidateTableFrame(nsIFrame* aFrame,
                                   const nsRect& aOrigRect,
                                   const nsRect& aOrigVisualOverflow,
                                   bool aIsFirstReflow);

  virtual bool UpdateOverflow();

protected:

  


  nsTableFrame(nsStyleContext* aContext);

  
  virtual ~nsTableFrame();

  void InitChildReflowState(nsHTMLReflowState& aReflowState);

  virtual int GetSkipSides() const MOZ_OVERRIDE;

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

  
  void HomogenousInsertFrames(ChildListID     aListID,
                              nsIFrame*       aPrevFrame,
                              nsFrameList&    aFrameList);
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
  void PushChildren(const RowGroupArray& aRowGroups, int32_t aPushFrom);

public:
  
  
  
  
  

  void OrderRowGroups(RowGroupArray& aChildren,
                      nsTableRowGroupFrame** aHead = nullptr,
                      nsTableRowGroupFrame** aFoot = nullptr) const;

  
  nsTableRowGroupFrame* GetTHead() const;

  
  nsTableRowGroupFrame* GetTFoot() const;

  
  
  
  bool RowIsSpannedInto(int32_t aRowIndex, int32_t aNumEffCols);

  
  
  
  bool RowHasSpanningCells(int32_t aRowIndex, int32_t aNumEffCols);

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

  





  void SetGeometryDirty() { mBits.mGeometryDirty = true; }
  void ClearGeometryDirty() { mBits.mGeometryDirty = false; }
  bool IsGeometryDirty() const { return mBits.mGeometryDirty; }

  


  nsTableCellMap* GetCellMap() const;

  




  void AdjustRowIndices(int32_t aRowIndex,
                        int32_t aAdjustment);

  





  void ResetRowIndices(const nsFrameList::Slice& aRowGroupsToExclude);

  nsTArray<nsTableColFrame*>& GetColCache();


protected:

  void SetBorderCollapse(bool aValue);

  BCPropertyData* GetBCProperty(bool aCreateIfNecessary = false) const;
  void SetFullBCDamageArea();
  void CalcBCBorders();

  void ExpandBCDamageArea(nsIntRect& aRect) const;

  void SetColumnDimensions(nscoord         aHeight,
                           const nsMargin& aReflowState);

  int32_t CollectRows(nsIFrame*                   aFrame,
                      nsTArray<nsTableRowFrame*>& aCollection);

public: 

  int32_t GetStartRowIndex(nsTableRowGroupFrame* aRowGroupFrame);

  

  int32_t GetRowCount () const
  {
    return GetCellMap()->GetRowCount();
  }

  

  int32_t GetEffectiveColCount() const;

  
  int32_t GetColCount () const
  {
    return GetCellMap()->GetColCount();
  }

  
  int32_t GetIndexOfLastRealCol();

  
  bool IsAutoLayout();

public:
 
#ifdef DEBUG
  void Dump(bool            aDumpRows,
            bool            aDumpCols, 
            bool            aDumpCellMap);
#endif

protected:
  


  void DoRemoveFrame(ChildListID aListID, nsIFrame* aOldFrame);
#ifdef DEBUG
  void DumpRowGroup(nsIFrame* aChildFrame);
#endif
  
  nsAutoTArray<nsTableColFrame*, 8> mColFrames;

  struct TableBits {
    uint32_t mHaveReflowedColGroups:1; 
    uint32_t mHasPctCol:1;             
    uint32_t mCellSpansPctCol:1;       
    uint32_t mIsBorderCollapse:1;      
    uint32_t mRowInserted:1;
    uint32_t mNeedToCalcBCBorders:1;
    uint32_t mGeometryDirty:1;
    uint32_t mLeftContBCBorder:8;
    uint32_t mNeedToCollapse:1;    
    uint32_t mHasZeroColSpans:1;
    uint32_t mNeedColSpanExpansion:1;
    uint32_t mResizedColumns:1;        
  } mBits;

  nsTableCellMap*         mCellMap;            
  nsITableLayoutStrategy* mTableLayoutStrategy;
  nsFrameList             mColGroups;          
};


inline bool nsTableFrame::IsRowGroup(int32_t aDisplayType) const
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
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
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
  int32_t   Count();

protected:
  void Init(nsIFrame* aFirstChild);
  bool      mLeftToRight;
  nsIFrame* mFirstListChild;
  nsIFrame* mFirstChild;
  nsIFrame* mCurrentChild;
  int32_t   mCount;
};

#define ABORT0() \
{NS_ASSERTION(false, "CellIterator program error"); \
return;}

#define ABORT1(aReturn) \
{NS_ASSERTION(false, "CellIterator program error"); \
return aReturn;} 

#endif
