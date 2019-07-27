



#ifndef nsTableFrame_h__
#define nsTableFrame_h__

#include "mozilla/Attributes.h"
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
  return (f->StyleVisibility()->IsVisible() &&
          (!f->StyleBackground()->IsTransparent() ||
           f->StyleDisplay()->mAppearance ||
           f->StyleBorder()->HasBorder()));
}

class nsDisplayTableItem : public nsDisplayItem
{
public:
  nsDisplayTableItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame) : 
      nsDisplayItem(aBuilder, aFrame),
      mPartHasFixedBackground(false) {}

  
  
  
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;

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
  NS_DECL_FRAMEARENA_HELPERS

  static void DestroyPositionedTablePartArray(void* aPropertyValue);
  NS_DECLARE_FRAME_PROPERTY(PositionedTablePartArray, DestroyPositionedTablePartArray)

  
  friend class nsTableOuterFrame;

  




  friend nsTableFrame* NS_NewTableFrame(nsIPresShell* aPresShell,
                                        nsStyleContext* aContext);

  


  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  static float GetTwipsToPixels(nsPresContext* aPresContext);

  
  
  static bool AncestorsHaveStyleHeight(const nsHTMLReflowState& aParentReflowState);

  
  
  static void CheckRequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState);

  
  
  static void RequestSpecialHeightReflow(const nsHTMLReflowState& aReflowState);

  static void RePositionViews(nsIFrame* aFrame);

  static bool PageBreakAfter(nsIFrame* aSourceFrame,
                               nsIFrame* aNextFrame);
  
  
  
  
  static void RegisterPositionedTablePart(nsIFrame* aFrame);

  
  static void UnregisterPositionedTablePart(nsIFrame* aFrame,
                                            nsIFrame* aDestructRoot);

  nsPoint GetFirstSectionOrigin(const nsHTMLReflowState& aReflowState) const;
  


  void AttributeChangedFor(nsIFrame*       aFrame,
                           nsIContent*     aContent, 
                           nsIAtom*        aAttribute); 

  
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual nsMargin GetUsedBorder() const MOZ_OVERRIDE;
  virtual nsMargin GetUsedPadding() const MOZ_OVERRIDE;
  virtual nsMargin GetUsedMargin() const MOZ_OVERRIDE;

  
  nsMargin GetChildAreaOffset(const nsHTMLReflowState* aReflowState) const;

  
  static nsTableFrame* GetTableFrame(nsIFrame* aSourceFrame);
                                 
  


  static nsTableFrame* GetTableFramePassingThrough(nsIFrame* aMustPassThrough,
                                                   nsIFrame* aSourceFrame);

  typedef void (* DisplayGenericTablePartTraversal)
      (nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
       const nsRect& aDirtyRect, const nsDisplayListSet& aLists);
  static void GenericTraversal(nsDisplayListBuilder* aBuilder, nsFrame* aFrame,
                               const nsRect& aDirtyRect, const nsDisplayListSet& aLists);

  










  static void DisplayGenericTablePart(nsDisplayListBuilder* aBuilder,
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

  virtual const nsFrameList& GetChildList(ChildListID aListID) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  




  void PaintTableBorderBackground(nsRenderingContext& aRenderingContext,
                                  const nsRect& aDirtyRect,
                                  nsPoint aPt, uint32_t aBGPaintFlags);

  



  static bool AnyTablePartHasUndecodedBackgroundImage(nsIFrame* aStart,
                                                      nsIFrame* aEnd);

  



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

  virtual void MarkIntrinsicISizesDirty() MOZ_OVERRIDE;
  
  
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual IntrinsicISizeOffsetData
    IntrinsicISizeOffsets(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              uint32_t aFlags) MOZ_OVERRIDE;

  virtual mozilla::LogicalSize
  ComputeAutoSize(nsRenderingContext *aRenderingContext,
                  mozilla::WritingMode aWritingMode,
                  const mozilla::LogicalSize& aCBSize,
                  nscoord aAvailableISize,
                  const mozilla::LogicalSize& aMargin,
                  const mozilla::LogicalSize& aBorder,
                  const mozilla::LogicalSize& aPadding,
                  bool aShrinkWrap) MOZ_OVERRIDE;

  



  nscoord TableShrinkWidthToFit(nsRenderingContext *aRenderingContext,
                                nscoord aWidthInCB);

  
  













  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  void ReflowTable(nsHTMLReflowMetrics&     aDesiredSize,
                   const nsHTMLReflowState& aReflowState,
                   nscoord                  aAvailHeight,
                   nsIFrame*&               aLastChildReflowed,
                   nsReflowStatus&          aStatus);

  nsFrameList& GetColGroups();

  virtual nsIFrame* GetParentStyleContextFrame() const MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    if (aFlags & eSupportsCSSTransforms) {
      return false;
    }
    return nsContainerFrame::IsFrameOfType(aFlags);
  }

#ifdef DEBUG_FRAME_DUMP
  
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  int32_t GetColumnWidth(int32_t aColIndex);

  








  virtual nscoord GetCellSpacingX(int32_t aColIndex);

  












  virtual nscoord GetCellSpacingX(int32_t aStartColIndex,
                                  int32_t aEndColIndex);

  








  virtual nscoord GetCellSpacingY(int32_t aRowIndex);

  












  virtual nscoord GetCellSpacingY(int32_t aStartRowIndex,
                                  int32_t aEndRowIndex);

private:
  


  nscoord GetCellSpacingX();
  nscoord GetCellSpacingY();

public:
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const MOZ_OVERRIDE;
  










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

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

protected:

  


  explicit nsTableFrame(nsStyleContext* aContext);

  
  virtual ~nsTableFrame();

  void InitChildReflowState(nsHTMLReflowState& aReflowState);

  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const MOZ_OVERRIDE;

public:
  bool IsRowInserted() const;
  void   SetRowInserted(bool aValue);

protected:
    
  
  
  
  nsresult SetupHeaderFooterChild(const nsTableReflowState& aReflowState,
                                  nsTableRowGroupFrame* aFrame,
                                  nscoord* aDesiredHeight);

  void ReflowChildren(nsTableReflowState&  aReflowState,
                      nsReflowStatus&      aStatus,
                      nsIFrame*&           aLastChildReflowed,
                      nsOverflowAreas&     aOverflowAreas);

  
  
  
  void ReflowColGroups(nsRenderingContext* aRenderingContext);

  



  nscoord GetCollapsedWidth(nsMargin aBorderPadding);

  
  




  void AdjustForCollapsingRowsCols(nsHTMLReflowMetrics& aDesiredSize,
                                   nsMargin             aBorderPadding);

  




  void FixupPositionedTableParts(nsPresContext*           aPresContext,
                                 nsHTMLReflowMetrics&     aDesiredSize,
                                 const nsHTMLReflowState& aReflowState);

  
  void ClearAllPositionedTableParts();

  nsITableLayoutStrategy* LayoutStrategy() const {
    return static_cast<nsTableFrame*>(FirstInFlow())->
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
  static_cast<nsTableFrame*>(FirstInFlow())->mBits.mNeedToCollapse = (unsigned)aValue;
}

inline bool nsTableFrame::NeedToCollapse() const
{
  return (bool) static_cast<nsTableFrame*>(FirstInFlow())->mBits.mNeedToCollapse;
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
  return static_cast<nsTableFrame*>(FirstInFlow())->mColGroups;
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
  explicit nsTableIterator(nsIFrame& aSource);
  explicit nsTableIterator(nsFrameList& aSource);
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
