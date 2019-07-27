







#ifndef nsGridContainerFrame_h___
#define nsGridContainerFrame_h___

#include "nsContainerFrame.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"





nsContainerFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);

class nsGridContainerFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsGridContainerFrame)
  NS_DECL_QUERYFRAME

  
  void Reflow(nsPresContext*           aPresContext,
              nsHTMLReflowMetrics&     aDesiredSize,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aStatus) MOZ_OVERRIDE;
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
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

  




  void PlaceGridItems(const nsStylePosition* aStyle);

  






  void InitializeGridBounds(const nsStylePosition* aStyle);

  




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

#ifdef DEBUG
  void SanityCheckAnonymousGridItems() const;
#endif 

private:
  



  uint32_t mExplicitGridColEnd;
  



  uint32_t mExplicitGridRowEnd;
  
  uint32_t mGridColEnd; 
  uint32_t mGridRowEnd; 
};

#endif 
