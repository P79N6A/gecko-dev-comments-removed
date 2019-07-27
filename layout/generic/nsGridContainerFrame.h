







#ifndef nsGridContainerFrame_h___
#define nsGridContainerFrame_h___

#include "nsContainerFrame.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"





nsContainerFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);

class nsGridContainerFrame final : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsGridContainerFrame)
  NS_DECL_QUERYFRAME

  
  void Reflow(nsPresContext*           aPresContext,
              nsHTMLReflowMetrics&     aDesiredSize,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aStatus) override;
  virtual nsIAtom* GetType() const override;

  void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                        const nsRect&           aDirtyRect,
                        const nsDisplayListSet& aLists) override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  




  static const nsRect& GridItemCB(nsIFrame* aChild);

  struct TrackSize {
    nscoord mBase;
    nscoord mLimit;
  };

  
  static const nscoord VERY_LIKELY_A_GRID_CONTAINER = -123456789;

  NS_DECLARE_FRAME_PROPERTY(GridItemContainingBlockRect, DeleteValue<nsRect>)

protected:
  static const int32_t kAutoLine;
  typedef mozilla::LogicalPoint LogicalPoint;
  typedef mozilla::LogicalRect LogicalRect;
  typedef mozilla::WritingMode WritingMode;
  typedef mozilla::css::GridNamedArea GridNamedArea;
  class GridItemCSSOrderIterator;
  friend nsContainerFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                                    nsStyleContext* aContext);
  explicit nsGridContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  












  struct LineRange {
   LineRange(int32_t aStart, int32_t aEnd)
     : mStart(aStart), mEnd(aEnd)
    {
#ifdef DEBUG
      if (!IsAutoAuto()) {
        if (IsAuto()) {
          MOZ_ASSERT(mEnd >= nsStyleGridLine::kMinLine &&
                     mEnd <= nsStyleGridLine::kMaxLine, "invalid span");
        } else {
          MOZ_ASSERT(mStart >= nsStyleGridLine::kMinLine &&
                     mStart <= nsStyleGridLine::kMaxLine, "invalid start line");
          MOZ_ASSERT(mEnd == kAutoLine ||
                     (mEnd >= nsStyleGridLine::kMinLine &&
                      mEnd <= nsStyleGridLine::kMaxLine), "invalid end line");
        }
      }
#endif
    }
    bool IsAutoAuto() const { return mStart == kAutoLine && mEnd == kAutoLine; }
    bool IsAuto() const { return mStart == kAutoLine; }
    bool IsDefinite() const { return mStart != kAutoLine; }
    uint32_t Extent() const
    {
      MOZ_ASSERT(mEnd != kAutoLine, "Extent is undefined for abs.pos. 'auto'");
      if (IsAuto()) {
        MOZ_ASSERT(mEnd >= 1 && mEnd < nsStyleGridLine::kMaxLine,
                   "invalid span");
        return mEnd;
      }
      return mEnd - mStart;
    }
    



    void ResolveAutoPosition(int32_t aStart)
    {
      MOZ_ASSERT(IsAuto(), "Why call me?");
      MOZ_ASSERT(aStart > 0, "expected a 1-based line number");
      mStart = aStart;
      mEnd += aStart;
    }
    



    uint32_t HypotheticalEnd() const { return IsAuto() ? mEnd + 1 : mEnd; }
    



    void ToPositionAndLength(const nsTArray<TrackSize>& aTrackSizes,
                             nscoord* aPos, nscoord* aLength) const;
    





    void ToPositionAndLengthForAbsPos(const nsTArray<TrackSize>& aTrackSizes,
                                      nscoord aGridOrigin,
                                      nscoord* aPos, nscoord* aLength) const;

    int32_t mStart;  
    int32_t mEnd;    
  };

  





  struct GridArea {
    GridArea(const LineRange& aCols, const LineRange& aRows)
      : mCols(aCols), mRows(aRows) {}
    bool IsDefinite() const { return mCols.IsDefinite() && mRows.IsDefinite(); }
    LineRange mCols;
    LineRange mRows;
  };

  







  struct CellMap {
    struct Cell {
      Cell() : mIsOccupied(false) {}
      bool mIsOccupied : 1;
    };
    void Fill(const GridArea& aGridArea);
    void ClearOccupied();
#if DEBUG
    void Dump() const;
#endif
    nsTArray<nsTArray<Cell>> mCells;
  };

  enum LineRangeSide {
    eLineRangeSideStart, eLineRangeSideEnd
  };
  

















  int32_t ResolveLine(const nsStyleGridLine& aLine,
                      int32_t aNth,
                      uint32_t aFromIndex,
                      const nsTArray<nsTArray<nsString>>& aLineNameList,
                      uint32_t GridNamedArea::* aAreaStart,
                      uint32_t GridNamedArea::* aAreaEnd,
                      uint32_t aExplicitGridEnd,
                      LineRangeSide aEdge,
                      const nsStylePosition* aStyle);
  














  LineRange ResolveLineRange(const nsStyleGridLine& aStart,
                             const nsStyleGridLine& aEnd,
                             const nsTArray<nsTArray<nsString>>& aLineNameList,
                             uint32_t GridNamedArea::* aAreaStart,
                             uint32_t GridNamedArea::* aAreaEnd,
                             uint32_t aExplicitGridEnd,
                             const nsStylePosition* aStyle);

  




  LineRange
  ResolveAbsPosLineRange(const nsStyleGridLine& aStart,
                         const nsStyleGridLine& aEnd,
                         const nsTArray<nsTArray<nsString>>& aLineNameList,
                         uint32_t GridNamedArea::* aAreaStart,
                         uint32_t GridNamedArea::* aAreaEnd,
                         uint32_t aExplicitGridEnd,
                         uint32_t aGridEnd,
                         const nsStylePosition* aStyle);

  






  GridArea PlaceDefinite(nsIFrame* aChild, const nsStylePosition* aStyle);

  






  void PlaceAutoCol(uint32_t aStartCol, GridArea* aArea) const;

  




  uint32_t FindAutoCol(uint32_t aStartCol, uint32_t aLockedRow,
                       const GridArea* aArea) const;

  






  void PlaceAutoRow(uint32_t aStartRow, GridArea* aArea) const;

  




  uint32_t FindAutoRow(uint32_t aLockedCol, uint32_t aStartRow,
                       const GridArea* aArea) const;

  






  void PlaceAutoAutoInRowOrder(uint32_t aStartCol, uint32_t aStartRow,
                               GridArea* aArea) const;

  






  void PlaceAutoAutoInColOrder(uint32_t aStartCol, uint32_t aStartRow,
                               GridArea* aArea) const;

  






  GridArea PlaceAbsPos(nsIFrame* aChild, const nsStylePosition* aStyle);

  






  void PlaceGridItems(GridItemCSSOrderIterator& aIter,
                      const nsStylePosition* aStyle);

  






  void InitializeGridBounds(const nsStylePosition* aStyle);

  



  void InflateGridFor(const GridArea& aArea)
  {
    mGridColEnd = std::max(mGridColEnd, aArea.mCols.HypotheticalEnd());
    mGridRowEnd = std::max(mGridRowEnd, aArea.mRows.HypotheticalEnd());
    MOZ_ASSERT(mGridColEnd <= uint32_t(nsStyleGridLine::kMaxLine -
                                       nsStyleGridLine::kMinLine) &&
               mGridRowEnd <= uint32_t(nsStyleGridLine::kMaxLine -
                                       nsStyleGridLine::kMinLine));
  }

  


  void CalculateTrackSizes(const mozilla::LogicalSize& aPercentageBasis,
                           const nsStylePosition*      aStyle,
                           nsTArray<TrackSize>&        aColSizes,
                           nsTArray<TrackSize>&        aRowSizes);

  




  typedef std::pair<int32_t, int32_t> LinePair;
  LinePair ResolveLineRangeHelper(const nsStyleGridLine& aStart,
                                  const nsStyleGridLine& aEnd,
                                  const nsTArray<nsTArray<nsString>>& aLineNameList,
                                  uint32_t GridNamedArea::* aAreaStart,
                                  uint32_t GridNamedArea::* aAreaEnd,
                                  uint32_t aExplicitGridEnd,
                                  const nsStylePosition* aStyle);

  





  NS_DECLARE_FRAME_PROPERTY(ImplicitNamedAreasProperty,
                            DeleteValue<ImplicitNamedAreas>)
  void InitImplicitNamedAreas(const nsStylePosition* aStyle);
  void AddImplicitNamedAreas(const nsTArray<nsTArray<nsString>>& aLineNameLists);
  typedef nsTHashtable<nsStringHashKey> ImplicitNamedAreas;
  ImplicitNamedAreas* GetImplicitNamedAreas() const {
    return static_cast<ImplicitNamedAreas*>(Properties().Get(ImplicitNamedAreasProperty()));
  }
  bool HasImplicitNamedArea(const nsString& aName) const {
    ImplicitNamedAreas* areas = GetImplicitNamedAreas();
    return areas && areas->Contains(aName);
  }

  NS_DECLARE_FRAME_PROPERTY(GridAreaProperty, DeleteValue<GridArea>)

  


  static GridArea* GetGridAreaForChild(nsIFrame* aChild) {
    return static_cast<GridArea*>(aChild->Properties().Get(GridAreaProperty()));
  }

  




  LogicalRect ContainingBlockFor(const WritingMode& aWM,
                                 const GridArea& aArea,
                                 const nsTArray<TrackSize>& aColSizes,
                                 const nsTArray<TrackSize>& aRowSizes) const;

  








  LogicalRect ContainingBlockForAbsPos(const WritingMode& aWM,
                                       const GridArea& aArea,
                                       const nsTArray<TrackSize>& aColSizes,
                                       const nsTArray<TrackSize>& aRowSizes,
                                       const LogicalPoint& aGridOrigin,
                                       const LogicalRect& aGridCB) const;

  


  void ReflowChildren(GridItemCSSOrderIterator&   aIter,
                      const LogicalRect&          aContentArea,
                      const nsTArray<TrackSize>&  aColSizes,
                      const nsTArray<TrackSize>&  aRowSizes,
                      nsHTMLReflowMetrics&        aDesiredSize,
                      const nsHTMLReflowState&    aReflowState,
                      nsReflowStatus&             aStatus);

#ifdef DEBUG
  void SanityCheckAnonymousGridItems() const;
#endif 

private:
  


  CellMap mCellMap;

  



  uint32_t mExplicitGridColEnd;
  



  uint32_t mExplicitGridRowEnd;
  
  uint32_t mGridColEnd; 
  uint32_t mGridRowEnd; 

  





  uint32_t mExplicitGridOffsetCol;
  uint32_t mExplicitGridOffsetRow;

  



  bool mIsNormalFlowInCSSOrder : 1;
};

#endif
