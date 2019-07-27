







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

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  struct TrackSize {
    nscoord mBase;
    nscoord mLimit;
  };

protected:
  typedef mozilla::LogicalRect LogicalRect;
  typedef mozilla::WritingMode WritingMode;
  typedef mozilla::css::GridNamedArea GridNamedArea;
  friend nsContainerFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                                    nsStyleContext* aContext);
  explicit nsGridContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  







  struct LineRange {
   LineRange(uint32_t aStart, uint32_t aEnd)
      : mStart(aStart), mEnd(aEnd) {}
    bool IsAuto() const { return mStart == 0; }
    bool IsDefinite() const { return mStart != 0; }
    uint32_t Extent() const { return mEnd - mStart; }
    



    void ResolveAutoPosition(uint32_t aStart)
    {
      MOZ_ASSERT(IsAuto(), "Why call me?");
      MOZ_ASSERT(aStart > 0, "expected a 1-based line number");
      MOZ_ASSERT(Extent() == mEnd, "'auto' representation changed?");
      mStart = aStart;
      mEnd += aStart;
    }
    



    uint32_t HypotheticalEnd() const { return IsAuto() ? mEnd + 1 : mEnd; }
    



    void ToPositionAndLength(const nsTArray<TrackSize>& aTrackSizes,
                             nscoord* aPos, nscoord* aLength) const;

    uint32_t mStart;  
    uint32_t mEnd;    
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
  

















  uint32_t ResolveLine(const nsStyleGridLine& aLine,
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

  





  void PlaceGridItems(const nsStylePosition* aStyle);

  






  void InitializeGridBounds(const nsStylePosition* aStyle);

  



  void InflateGridFor(const GridArea& aArea)
  {
    mGridColEnd = std::max(mGridColEnd, aArea.mCols.HypotheticalEnd());
    mGridRowEnd = std::max(mGridRowEnd, aArea.mRows.HypotheticalEnd());
  }

  


  void CalculateTrackSizes(const mozilla::LogicalSize& aPercentageBasis,
                           const nsStylePosition*      aStyle,
                           nsTArray<TrackSize>&        aColSizes,
                           nsTArray<TrackSize>&        aRowSizes);

  




  typedef std::pair<uint32_t, uint32_t> LinePair;
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

  


  void ReflowChildren(const LogicalRect&          aContentArea,
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
};

#endif 
