






































#ifndef nsHTMLReflowState_h___
#define nsHTMLReflowState_h___

#include "nsMargin.h"
#include "nsStyleCoord.h"
#include "nsIFrame.h"

class nsPresContext;
class nsRenderingContext;
class nsFloatManager;
class nsLineLayout;
class nsIPercentHeightObserver;

struct nsStyleDisplay;
struct nsStyleVisibility;
struct nsStylePosition;
struct nsStyleBorder;
struct nsStyleMargin;
struct nsStylePadding;
struct nsStyleText;
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






#define NS_UNCONSTRAINEDSIZE NS_MAXSIZE




typedef PRUint32  nsCSSFrameType;

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

#define NS_INTRINSICSIZE    NS_UNCONSTRAINEDSIZE
#define NS_AUTOHEIGHT       NS_UNCONSTRAINEDSIZE
#define NS_AUTOMARGIN       NS_UNCONSTRAINEDSIZE
#define NS_AUTOOFFSET       NS_UNCONSTRAINEDSIZE






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
    InitOffsets(aContainingBlockWidth);
  }

#ifdef DEBUG
  
  
  static void* DisplayInitOffsetsEnter(nsIFrame* aFrame,
                                       nsCSSOffsetState* aState,
                                       nscoord aCBWidth,
                                       const nsMargin* aBorder,
                                       const nsMargin* aPadding);
  static void DisplayInitOffsetsExit(nsIFrame* aFrame,
                                     nsCSSOffsetState* aState,
                                     void* aValue);
#endif

private:
  




  PRBool ComputeMargin(nscoord aContainingBlockWidth);
  
  




   PRBool ComputePadding(nscoord aContainingBlockWidth);

protected:

  void InitOffsets(nscoord aContainingBlockWidth,
                   const nsMargin *aBorder = nsnull,
                   const nsMargin *aPadding = nsnull);

  




  inline nscoord ComputeWidthValue(nscoord aContainingBlockWidth,
                                   nscoord aContentEdgeToBoxSizing,
                                   nscoord aBoxSizingToMarginEdge,
                                   const nsStyleCoord& aCoord);
  
  
  nscoord ComputeWidthValue(nscoord aContainingBlockWidth,
                            PRUint8 aBoxSizing,
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

  
  
  nsIPercentHeightObserver* mPercentHeightObserver;

  
  
  
  
  
  nsIFrame** mDiscoveredClearance;

  
  
  PRInt16 mReflowDepth;

  struct ReflowStateFlags {
    PRUint16 mSpecialHeightReflow:1; 
                                     
    PRUint16 mNextInFlowUntouched:1; 
                                     
    PRUint16 mIsTopOfPage:1;         
                                     
                                     
                                     
    PRUint16 mBlinks:1;              
    PRUint16 mHasClearance:1;        
    PRUint16 mAssumingHScrollbar:1;  
                                     
    PRUint16 mAssumingVScrollbar:1;  
                                     

    PRUint16 mHResize:1;             
                                     

    PRUint16 mVResize:1;             
                                     
                                     
                                     
                                     
    PRUint16 mTableIsSplittable:1;   
                                     
    PRUint16 mHeightDependsOnAncestorCell:1;   
                                               
    
  } mFlags;

  
  
  

  
  
  nsHTMLReflowState(nsPresContext*           aPresContext,
                    nsIFrame*                aFrame,
                    nsRenderingContext*     aRenderingContext,
                    const nsSize&            aAvailableSpace);

  
  
  
  nsHTMLReflowState(nsPresContext*           aPresContext,
                    const nsHTMLReflowState& aParentReflowState,
                    nsIFrame*                aFrame,
                    const nsSize&            aAvailableSpace,
                    
                    
                    nscoord                  aContainingBlockWidth = -1,
                    nscoord                  aContainingBlockHeight = -1,
                    PRBool                   aInit = PR_TRUE);

  
  
  void Init(nsPresContext* aPresContext,
            nscoord         aContainingBlockWidth = -1,
            nscoord         aContainingBlockHeight = -1,
            const nsMargin* aBorder = nsnull,
            const nsMargin* aPadding = nsnull);
  


  static nscoord
    GetContainingBlockContentWidth(const nsHTMLReflowState* aReflowState);

  




  static nsIFrame* GetContainingBlockFor(const nsIFrame* aFrame);

  


  nscoord CalcLineHeight() const;

  








  static nscoord CalcLineHeight(nsStyleContext* aStyleContext,
                                nscoord aBlockHeight);


  void ComputeContainingBlockRectangle(nsPresContext*          aPresContext,
                                       const nsHTMLReflowState* aContainingBlockRS,
                                       nscoord&                 aContainingBlockWidth,
                                       nscoord&                 aContainingBlockHeight);

  




  void ApplyMinMaxConstraints(nscoord* aContentWidth, nscoord* aContentHeight) const;

  PRBool ShouldReflowAllKids() const {
    
    
    
    
    
    
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

  PRBool WillReflowAgainForClearance() const {
    return mDiscoveredClearance && *mDiscoveredClearance;
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
  void InitFrameType();
  void InitCBReflowState();
  void InitResizeFlags(nsPresContext* aPresContext);

  void InitConstraints(nsPresContext* aPresContext,
                       nscoord         aContainingBlockWidth,
                       nscoord         aContainingBlockHeight,
                       const nsMargin* aBorder,
                       const nsMargin* aPadding);

  
  
  
  
  nsIFrame* GetHypotheticalBoxContainer(nsIFrame* aFrame,
                                        nscoord& aCBLeftEdge,
                                        nscoord& aCBWidth);

  void CalculateHypotheticalBox(nsPresContext*    aPresContext,
                                nsIFrame*         aPlaceholderFrame,
                                nsIFrame*         aContainingBlock,
                                nscoord           aBlockLeftContentEdge,
                                nscoord           aBlockContentWidth,
                                const nsHTMLReflowState* cbrs,
                                nsHypotheticalBox& aHypotheticalBox);

  void InitAbsoluteConstraints(nsPresContext* aPresContext,
                               const nsHTMLReflowState* cbrs,
                               nscoord aContainingBlockWidth,
                               nscoord aContainingBlockHeight);

  void ComputeRelativeOffsets(const nsHTMLReflowState* cbrs,
                              nscoord aContainingBlockWidth,
                              nscoord aContainingBlockHeight,
                              nsPresContext* aPresContext);

  
  
  
  void ComputeMinMaxValues(nscoord                  aContainingBlockWidth,
                           nscoord                  aContainingBlockHeight,
                           const nsHTMLReflowState* aContainingBlockRS);

  void CalculateHorizBorderPaddingMargin(nscoord aContainingBlockWidth,
                                         nscoord* aInsideBoxSizing,
                                         nscoord* aOutsideBoxSizing);

  void CalculateBlockSideMargins(nscoord aAvailWidth,
                                 nscoord aComputedWidth);
};

#endif 

