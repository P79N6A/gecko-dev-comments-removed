








#ifndef nsFlexContainerFrame_h___
#define nsFlexContainerFrame_h___

#include "nsContainerFrame.h"

namespace mozilla {
template <class T> class LinkedList;
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
  class StrutInfo;

  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nscoord
    GetMinWidth(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord
    GetPrefWidth(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  
  bool IsHorizontal();

protected:
  
  nsFlexContainerFrame(nsStyleContext* aContext) :
    nsFlexContainerFrameSuper(aContext)
  {}
  virtual ~nsFlexContainerFrame();

  













  nsresult DoFlexLayout(nsPresContext*           aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus,
                        nscoord aContentBoxMainSize,
                        nscoord aAvailableHeightForContent,
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

  
  
  
  nsresult ResolveFlexItemMaxContentSizing(nsPresContext* aPresContext,
                                           FlexItem& aFlexItem,
                                           const nsHTMLReflowState& aParentReflowState,
                                           const FlexboxAxisTracker& aAxisTracker);

  
  
  
  
  nsresult GenerateFlexLines(nsPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nscoord aContentBoxMainSize,
                             nscoord aAvailableHeightForContent,
                             const nsTArray<StrutInfo>& aStruts,
                             const FlexboxAxisTracker& aAxisTracker,
                             mozilla::LinkedList<FlexLine>& aLines);

  nscoord GetMainSizeFromReflowState(const nsHTMLReflowState& aReflowState,
                                     const FlexboxAxisTracker& aAxisTracker);

  nscoord ComputeCrossSize(const nsHTMLReflowState& aReflowState,
                           const FlexboxAxisTracker& aAxisTracker,
                           nscoord aSumLineCrossSizes,
                           nscoord aAvailableHeightForContent,
                           bool* aIsDefinite,
                           nsReflowStatus& aStatus);

  nsresult SizeItemInCrossAxis(nsPresContext* aPresContext,
                               const FlexboxAxisTracker& aAxisTracker,
                               nsHTMLReflowState& aChildReflowState,
                               FlexItem& aItem);

  bool mChildrenHaveBeenReordered; 
                                   
};

#endif 
