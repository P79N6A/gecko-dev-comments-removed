






#ifndef nsHTMLReflowState_h___
#define nsHTMLReflowState_h___

#include "nsMargin.h"
#include "nsStyleCoord.h"
#include "nsIFrame.h"
#include "mozilla/Assertions.h"
#include <algorithm>

class nsPresContext;
class nsRenderingContext;
class nsFloatManager;
class nsLineLayout;
class nsIPercentHeightObserver;
struct nsHypotheticalBox;










template <class NumericType>
NumericType
NS_CSS_MINMAX(NumericType aValue, NumericType aMinValue, NumericType aMaxValue)
{
  NumericType result = aValue;
  if (aMaxValue < result)
    result = aMaxValue;
  if (aMinValue > result)
    result = aMinValue;
  return result;
}




typedef uint32_t  nsCSSFrameType;

#define NS_CSS_FRAME_TYPE_UNKNOWN         0
#define NS_CSS_FRAME_TYPE_INLINE          1
#define NS_CSS_FRAME_TYPE_BLOCK           2  /* block-level in normal flow */
#define NS_CSS_FRAME_TYPE_FLOATING        3
#define NS_CSS_FRAME_TYPE_ABSOLUTE        4
#define NS_CSS_FRAME_TYPE_INTERNAL_TABLE  5  /* row group frame, row frame, cell frame, ... */





#define NS_CSS_FRAME_TYPE_REPLACED                0x08000







#define NS_CSS_FRAME_TYPE_REPLACED_CONTAINS_BLOCK 0x10000




#define NS_FRAME_IS_REPLACED_NOBLOCK(_ft) \
  (NS_CSS_FRAME_TYPE_REPLACED == ((_ft) & NS_CSS_FRAME_TYPE_REPLACED))

#define NS_FRAME_IS_REPLACED(_ft)            \
  (NS_FRAME_IS_REPLACED_NOBLOCK(_ft) ||      \
   NS_FRAME_IS_REPLACED_CONTAINS_BLOCK(_ft))

#define NS_FRAME_REPLACED(_ft) \
  (NS_CSS_FRAME_TYPE_REPLACED | (_ft))

#define NS_FRAME_IS_REPLACED_CONTAINS_BLOCK(_ft)         \
  (NS_CSS_FRAME_TYPE_REPLACED_CONTAINS_BLOCK ==         \
   ((_ft) & NS_CSS_FRAME_TYPE_REPLACED_CONTAINS_BLOCK))

#define NS_FRAME_REPLACED_CONTAINS_BLOCK(_ft) \
  (NS_CSS_FRAME_TYPE_REPLACED_CONTAINS_BLOCK | (_ft))




#define NS_FRAME_GET_TYPE(_ft)                           \
  ((_ft) & ~(NS_CSS_FRAME_TYPE_REPLACED |                \
             NS_CSS_FRAME_TYPE_REPLACED_CONTAINS_BLOCK))



struct nsCSSOffsetState {
public:
  
  nsIFrame*           frame;

  
  nsRenderingContext* rendContext;

  
  nsMargin         mComputedMargin;

  
  nsMargin         mComputedBorderPadding;

  
  nsMargin         mComputedPadding;

  
  nsCSSOffsetState(nsIFrame *aFrame, nsRenderingContext *aRenderingContext)
    : frame(aFrame)
    , rendContext(aRenderingContext)
  {
  }

  
  
  
  
  nsCSSOffsetState(nsIFrame *aFrame, nsRenderingContext *aRenderingContext,
                   nscoord aContainingBlockWidth)
    : frame(aFrame)
    , rendContext(aRenderingContext)
  {
    MOZ_ASSERT(!aFrame->IsFlexItem(),
               "We're about to resolve vertical percent margin & padding "
               "values against CB width, which is incorrect for flex items");
    InitOffsets(aContainingBlockWidth, aContainingBlockWidth, frame->GetType());
  }

#ifdef DEBUG
  
  
  static void* DisplayInitOffsetsEnter(nsIFrame* aFrame,
                                       nsCSSOffsetState* aState,
                                       nscoord aHorizontalPercentBasis,
                                       nscoord aVerticalPercentBasis,
                                       const nsMargin* aBorder,
                                       const nsMargin* aPadding);
  static void DisplayInitOffsetsExit(nsIFrame* aFrame,
                                     nsCSSOffsetState* aState,
                                     void* aValue);
#endif

private:
  












  bool ComputeMargin(nscoord aHorizontalPercentBasis,
                     nscoord aVerticalPercentBasis);
  
  












   bool ComputePadding(nscoord aHorizontalPercentBasis,
                       nscoord aVerticalPercentBasis, nsIAtom* aFrameType);

protected:

  void InitOffsets(nscoord aHorizontalPercentBasis,
                   nscoord aVerticalPercentBasis,
                   nsIAtom* aFrameType,
                   const nsMargin *aBorder = nullptr,
                   const nsMargin *aPadding = nullptr);

  




  inline nscoord ComputeWidthValue(nscoord aContainingBlockWidth,
                                   nscoord aContentEdgeToBoxSizing,
                                   nscoord aBoxSizingToMarginEdge,
                                   const nsStyleCoord& aCoord);
  
  
  nscoord ComputeWidthValue(nscoord aContainingBlockWidth,
                            uint8_t aBoxSizing,
                            const nsStyleCoord& aCoord);

  nscoord ComputeHeightValue(nscoord aContainingBlockHeight,
                             uint8_t aBoxSizing,
                             const nsStyleCoord& aCoord);
};









struct nsHTMLReflowState : public nsCSSOffsetState {
  
  
  const nsHTMLReflowState* parentReflowState;

  
  nsFloatManager* mFloatManager;

  
  nsLineLayout*    mLineLayout;

  
  
  const nsHTMLReflowState *mCBReflowState;

  
  
  
  
  nscoord              availableWidth;

  
  
  
  
  
  
  
  
  nscoord              availableHeight;

  
  
  nsCSSFrameType   mFrameType;

  
  
  
  
  
  
  
  
  nscoord mBlockDelta;

private:
  
  
  
  
  
  
  
  
  nscoord          mComputedWidth; 

  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord          mComputedHeight;

public:
  
  
  nsMargin         mComputedOffsets;

  
  
  
  nscoord          mComputedMinWidth, mComputedMaxWidth;
  nscoord          mComputedMinHeight, mComputedMaxHeight;

  
  const nsStyleDisplay*    mStyleDisplay;
  const nsStyleVisibility* mStyleVisibility;
  const nsStylePosition*   mStylePosition;
  const nsStyleBorder*     mStyleBorder;
  const nsStyleMargin*     mStyleMargin;
  const nsStylePadding*    mStylePadding;
  const nsStyleText*       mStyleText;

  bool IsFloating() const;

  uint8_t GetDisplay() const;

  
  
  nsIPercentHeightObserver* mPercentHeightObserver;

  
  
  
  
  
  nsIFrame** mDiscoveredClearance;

  
  
  int16_t mReflowDepth;

  struct ReflowStateFlags {
    uint16_t mSpecialHeightReflow:1; 
                                     
    uint16_t mNextInFlowUntouched:1; 
                                     
    uint16_t mIsTopOfPage:1;         
                                     
                                     
                                     
    uint16_t mHasClearance:1;        
    uint16_t mAssumingHScrollbar:1;  
                                     
    uint16_t mAssumingVScrollbar:1;  
                                     

    uint16_t mHResize:1;             
                                     

    uint16_t mVResize:1;             
                                     
                                     
                                     
                                     
    uint16_t mTableIsSplittable:1;   
                                     
    uint16_t mHeightDependsOnAncestorCell:1;   
                                               
    uint16_t mIsColumnBalancing:1;   
    uint16_t mIsFlexContainerMeasuringHeight:1; 
                                                
                                                
    uint16_t mDummyParentReflowState:1; 
                                        
                                        
    uint16_t mMustReflowPlaceholders:1; 
                                        
                                        
                                        
                                        
                                        
  } mFlags;

  
  
  

  










  nsHTMLReflowState(nsPresContext*           aPresContext,
                    nsIFrame*                aFrame,
                    nsRenderingContext*      aRenderingContext,
                    const nsSize&            aAvailableSpace,
                    uint32_t                 aFlags = 0);

  


















  nsHTMLReflowState(nsPresContext*           aPresContext,
                    const nsHTMLReflowState& aParentReflowState,
                    nsIFrame*                aFrame,
                    const nsSize&            aAvailableSpace,
                    nscoord                  aContainingBlockWidth = -1,
                    nscoord                  aContainingBlockHeight = -1,
                    uint32_t                 aFlags = 0);

  
  enum {
    
    
    DUMMY_PARENT_REFLOW_STATE = (1<<0),

    
    
    CALLER_WILL_INIT = (1<<1)
  };

  
  
  void Init(nsPresContext* aPresContext,
            nscoord         aContainingBlockWidth = -1,
            nscoord         aContainingBlockHeight = -1,
            const nsMargin* aBorder = nullptr,
            const nsMargin* aPadding = nullptr);
  


  static nscoord
    GetContainingBlockContentWidth(const nsHTMLReflowState* aReflowState);

  


  nscoord CalcLineHeight() const;

  












  static nscoord CalcLineHeight(nsStyleContext* aStyleContext,
                                nscoord aBlockHeight,
                                float aFontSizeInflation);


  void ComputeContainingBlockRectangle(nsPresContext*          aPresContext,
                                       const nsHTMLReflowState* aContainingBlockRS,
                                       nscoord&                 aContainingBlockWidth,
                                       nscoord&                 aContainingBlockHeight);

  



  nscoord ApplyMinMaxWidth(nscoord aWidth) const {
    if (NS_UNCONSTRAINEDSIZE != mComputedMaxWidth) {
      aWidth = std::min(aWidth, mComputedMaxWidth);
    }
    return std::max(aWidth, mComputedMinWidth);
  }

  








  nscoord ApplyMinMaxHeight(nscoord aHeight, nscoord aConsumed = 0) const {
    aHeight += aConsumed;

    if (NS_UNCONSTRAINEDSIZE != mComputedMaxHeight) {
      aHeight = std::min(aHeight, mComputedMaxHeight);
    }

    if (NS_UNCONSTRAINEDSIZE != mComputedMinHeight) {
      aHeight = std::max(aHeight, mComputedMinHeight);
    }

    return aHeight - aConsumed;
  }

  bool ShouldReflowAllKids() const {
    
    
    
    
    
    
    return (frame->GetStateBits() & NS_FRAME_IS_DIRTY) ||
           mFlags.mHResize ||
           (mFlags.mVResize && 
            (frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT));
  }

  nscoord ComputedWidth() const { return mComputedWidth; }
  
  void SetComputedWidth(nscoord aComputedWidth);

  nscoord ComputedHeight() const { return mComputedHeight; }
  
  void SetComputedHeight(nscoord aComputedHeight);

  void SetComputedHeightWithoutResettingResizeFlags(nscoord aComputedHeight) {
    
    
    
    
    mComputedHeight = aComputedHeight;
  }

  void SetTruncated(const nsHTMLReflowMetrics& aMetrics, nsReflowStatus* aStatus) const;

  bool WillReflowAgainForClearance() const {
    return mDiscoveredClearance && *mDiscoveredClearance;
  }

  
  static void ComputeRelativeOffsets(uint8_t aCBDirection,
                                     nsIFrame* aFrame,
                                     nscoord aContainingBlockWidth,
                                     nscoord aContainingBlockHeight,
                                     nsMargin& aComputedOffsets);

  
  static void ApplyRelativePositioning(nsIFrame* aFrame,
                                       const nsMargin& aComputedOffsets,
                                       nsPoint* aPosition);

  void ApplyRelativePositioning(nsPoint* aPosition) const {
    ApplyRelativePositioning(frame, mComputedOffsets, aPosition);
  }

#ifdef DEBUG
  
  
  static void* DisplayInitConstraintsEnter(nsIFrame* aFrame,
                                           nsHTMLReflowState* aState,
                                           nscoord aCBWidth,
                                           nscoord aCBHeight,
                                           const nsMargin* aBorder,
                                           const nsMargin* aPadding);
  static void DisplayInitConstraintsExit(nsIFrame* aFrame,
                                         nsHTMLReflowState* aState,
                                         void* aValue);
  static void* DisplayInitFrameTypeEnter(nsIFrame* aFrame,
                                         nsHTMLReflowState* aState);
  static void DisplayInitFrameTypeExit(nsIFrame* aFrame,
                                       nsHTMLReflowState* aState,
                                       void* aValue);
#endif

protected:
  void InitFrameType(nsIAtom* aFrameType);
  void InitCBReflowState();
  void InitResizeFlags(nsPresContext* aPresContext, nsIAtom* aFrameType);

  void InitConstraints(nsPresContext* aPresContext,
                       nscoord         aContainingBlockWidth,
                       nscoord         aContainingBlockHeight,
                       const nsMargin* aBorder,
                       const nsMargin* aPadding,
                       nsIAtom*        aFrameType);

  
  
  
  
  nsIFrame* GetHypotheticalBoxContainer(nsIFrame* aFrame,
                                        nscoord& aCBLeftEdge,
                                        nscoord& aCBWidth);

  void CalculateHypotheticalBox(nsPresContext*    aPresContext,
                                nsIFrame*         aPlaceholderFrame,
                                nsIFrame*         aContainingBlock,
                                nscoord           aBlockLeftContentEdge,
                                nscoord           aBlockContentWidth,
                                const nsHTMLReflowState* cbrs,
                                nsHypotheticalBox& aHypotheticalBox,
                                nsIAtom*          aFrameType);

  void InitAbsoluteConstraints(nsPresContext* aPresContext,
                               const nsHTMLReflowState* cbrs,
                               nscoord aContainingBlockWidth,
                               nscoord aContainingBlockHeight,
                               nsIAtom* aFrameType);

  
  
  
  void ComputeMinMaxValues(nscoord                  aContainingBlockWidth,
                           nscoord                  aContainingBlockHeight,
                           const nsHTMLReflowState* aContainingBlockRS);

  void CalculateHorizBorderPaddingMargin(nscoord aContainingBlockWidth,
                                         nscoord* aInsideBoxSizing,
                                         nscoord* aOutsideBoxSizing);

  void CalculateBlockSideMargins(nscoord aAvailWidth,
                                 nscoord aComputedWidth,
                                 nsIAtom* aFrameType);
};

#endif 

