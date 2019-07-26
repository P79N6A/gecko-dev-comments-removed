








#ifndef nsFlexContainerFrame_h___
#define nsFlexContainerFrame_h___

#include "nsContainerFrame.h"

nsIFrame* NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                                   nsStyleContext* aContext);

typedef nsContainerFrame nsFlexContainerFrameSuper;

class FlexItem;
class FlexboxAxisTracker;
class MainAxisPositionTracker;
class SingleLineCrossAxisPositionTracker;
template <class T> class nsTArray;

class nsFlexContainerFrame : public nsFlexContainerFrameSuper {
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsFlexContainerFrame)
  NS_DECL_QUERYFRAME

  
  friend nsIFrame* NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                                            nsStyleContext* aContext);

public:
  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nscoord
    GetMinWidth(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord
    GetPrefWidth(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif 
  
  bool IsHorizontal();

protected:
  
  nsFlexContainerFrame(nsStyleContext* aContext) :
    nsFlexContainerFrameSuper(aContext),
    mChildrenHaveBeenReordered(false)
  {}
  virtual ~nsFlexContainerFrame();

  








  template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
  bool SortChildrenIfNeeded();

  
#ifdef DEBUG
  void SanityCheckAnonymousFlexItems() const;
#endif 

  FlexItem GenerateFlexItemForChild(nsPresContext* aPresContext,
                                    nsIFrame* aChildFrame,
                                    const nsHTMLReflowState& aParentReflowState,
                                    const FlexboxAxisTracker& aAxisTracker);

  
  
  
  nsresult ResolveFlexItemMaxContentSizing(nsPresContext* aPresContext,
                                           FlexItem& aFlexItem,
                                           const nsHTMLReflowState& aParentReflowState,
                                           const FlexboxAxisTracker& aAxisTracker);

  
  
  void ResolveFlexibleLengths(const FlexboxAxisTracker& aAxisTracker,
                              nscoord aFlexContainerMainSize,
                              nsTArray<FlexItem>& aItems);

  nsresult GenerateFlexItems(nsPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             const FlexboxAxisTracker& aAxisTracker,
                             nsTArray<FlexItem>& aItems);

  nscoord ComputeFlexContainerMainSize(const nsHTMLReflowState& aReflowState,
                                       const FlexboxAxisTracker& aAxisTracker,
                                       const nsTArray<FlexItem>& aFlexItems,
                                       nscoord aAvailableHeightForContent,
                                       nsReflowStatus& aStatus);

  nscoord ComputeFlexContainerCrossSize(const nsHTMLReflowState& aReflowState,
                                        const FlexboxAxisTracker& aAxisTracker,
                                        nscoord aLineCrossSize,
                                        nscoord aAvailableHeightForContent,
                                        bool* aIsDefinite,
                                        nsReflowStatus& aStatus);

  void PositionItemInMainAxis(MainAxisPositionTracker& aMainAxisPosnTracker,
                              FlexItem& aItem);

  nsresult SizeItemInCrossAxis(nsPresContext* aPresContext,
                               const FlexboxAxisTracker& aAxisTracker,
                               nsHTMLReflowState& aChildReflowState,
                               FlexItem& aItem);

  void PositionItemInCrossAxis(
    nscoord aLineStartPosition,
    SingleLineCrossAxisPositionTracker& aLineCrossAxisPosnTracker,
    FlexItem& aItem);

  bool mChildrenHaveBeenReordered; 
                                   
};

#endif 
