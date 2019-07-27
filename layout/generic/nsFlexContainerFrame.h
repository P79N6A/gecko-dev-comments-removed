








#ifndef nsFlexContainerFrame_h___
#define nsFlexContainerFrame_h___

#include "nsContainerFrame.h"

namespace mozilla {
template <class T> class LinkedList;
class LogicalPoint;
} 

nsContainerFrame* NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);

typedef nsContainerFrame nsFlexContainerFrameSuper;

class nsFlexContainerFrame : public nsFlexContainerFrameSuper {
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsFlexContainerFrame)
  NS_DECL_QUERYFRAME

  
  friend nsContainerFrame* NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                                                    nsStyleContext* aContext);

  
  class FlexItem;
  class FlexLine;
  class FlexboxAxisTracker;
  struct StrutInfo;

  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual nscoord
    GetMinISize(nsRenderingContext* aRenderingContext) override;
  virtual nscoord
    GetPrefISize(nsRenderingContext* aRenderingContext) override;

  virtual nsIAtom* GetType() const override;
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif
  
  bool IsHorizontal();

protected:
  
  explicit nsFlexContainerFrame(nsStyleContext* aContext) :
    nsFlexContainerFrameSuper(aContext)
  {}
  virtual ~nsFlexContainerFrame();

  













  void DoFlexLayout(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus,
                    nscoord aContentBoxMainSize,
                    nscoord aAvailableBSizeForContent,
                    nsTArray<StrutInfo>& aStruts,
                    const FlexboxAxisTracker& aAxisTracker);

  








  template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
  bool SortChildrenIfNeeded();

  
#ifdef DEBUG
  void SanityCheckAnonymousFlexItems() const;
#endif 

  










  FlexItem* GenerateFlexItemForChild(nsPresContext* aPresContext,
                                     nsIFrame* aChildFrame,
                                     const nsHTMLReflowState& aParentReflowState,
                                     const FlexboxAxisTracker& aAxisTracker);

  





  nscoord MeasureFlexItemContentHeight(nsPresContext* aPresContext,
                                       FlexItem& aFlexItem,
                                       bool aForceVerticalResizeForMeasuringReflow,
                                       const nsHTMLReflowState& aParentReflowState);

  




  void ResolveAutoFlexBasisAndMinSize(nsPresContext* aPresContext,
                                      FlexItem& aFlexItem,
                                      const nsHTMLReflowState& aItemReflowState,
                                      const FlexboxAxisTracker& aAxisTracker);

  
  
  
  
  void GenerateFlexLines(nsPresContext* aPresContext,
                         const nsHTMLReflowState& aReflowState,
                         nscoord aContentBoxMainSize,
                         nscoord aAvailableBSizeForContent,
                         const nsTArray<StrutInfo>& aStruts,
                         const FlexboxAxisTracker& aAxisTracker,
                         mozilla::LinkedList<FlexLine>& aLines);

  nscoord GetMainSizeFromReflowState(const nsHTMLReflowState& aReflowState,
                                     const FlexboxAxisTracker& aAxisTracker);

  nscoord ComputeCrossSize(const nsHTMLReflowState& aReflowState,
                           const FlexboxAxisTracker& aAxisTracker,
                           nscoord aSumLineCrossSizes,
                           nscoord aAvailableBSizeForContent,
                           bool* aIsDefinite,
                           nsReflowStatus& aStatus);

  void SizeItemInCrossAxis(nsPresContext* aPresContext,
                           const FlexboxAxisTracker& aAxisTracker,
                           nsHTMLReflowState& aChildReflowState,
                           FlexItem& aItem);

  














  void MoveFlexItemToFinalPosition(const nsHTMLReflowState& aReflowState,
                                   const FlexItem& aItem,
                                   mozilla::LogicalPoint& aFramePos,
                                   const nsSize& aContainerSize);
  












  void ReflowFlexItem(nsPresContext* aPresContext,
                      const FlexboxAxisTracker& aAxisTracker,
                      const nsHTMLReflowState& aReflowState,
                      const FlexItem& aItem,
                      mozilla::LogicalPoint& aFramePos,
                      const nsSize& aContainerSize);

  bool mChildrenHaveBeenReordered; 
                                   
};

#endif 
