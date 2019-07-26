








#ifndef nsFlexContainerFrame_h___
#define nsFlexContainerFrame_h___

#include "nsContainerFrame.h"
#include "nsTArray.h"
#include "mozilla/Types.h"

nsIFrame* NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                                   nsStyleContext* aContext);

typedef nsContainerFrame nsFlexContainerFrameSuper;

class FlexItem;
class FlexboxAxisTracker;
class MainAxisPositionTracker;
class SingleLineCrossAxisPositionTracker;

class nsFlexContainerFrame : public nsFlexContainerFrameSuper {
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsFlexContainerFrame)
  NS_DECL_QUERYFRAME

  
  friend nsIFrame* NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                                            nsStyleContext* aContext);

public:
  
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
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
    mCachedContentBoxCrossSize(nscoord_MIN),
    mCachedAscent(nscoord_MIN),
    mChildrenHaveBeenReordered(false)
  {}
  virtual ~nsFlexContainerFrame();

  








  template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
  bool SortChildrenIfNeeded();

  
#ifdef DEBUG
  void SanityCheckAnonymousFlexItems() const;
#endif 


  
  
  
  nsresult AppendFlexItemForChild(nsPresContext* aPresContext,
                                  nsIFrame* aChildFrame,
                                  const nsHTMLReflowState& aParentReflowState,
                                  const FlexboxAxisTracker& aAxisTracker,
                                  nsTArray<FlexItem>& aFlexItems);

  
  
  void ResolveFlexibleLengths(const FlexboxAxisTracker& aAxisTracker,
                              nscoord aFlexContainerMainSize,
                              nsTArray<FlexItem>& aItems);

  nsresult GenerateFlexItems(nsPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             const FlexboxAxisTracker& aAxisTracker,
                             nsTArray<FlexItem>& aItems);

  nscoord ComputeFlexContainerMainSize(const nsHTMLReflowState& aReflowState,
                                       const FlexboxAxisTracker& aAxisTracker,
                                       const nsTArray<FlexItem>& aFlexItems);

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

  
  
  
  nscoord mCachedContentBoxCrossSize; 
  nscoord mCachedAscent;              
  bool    mChildrenHaveBeenReordered; 
                                      
};

#endif 
