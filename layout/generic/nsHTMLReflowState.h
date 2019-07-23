






































#ifndef nsHTMLReflowState_h___
#define nsHTMLReflowState_h___

#include "nsMargin.h"
#include "nsStyleCoord.h"
#include "nsIFrame.h"

class nsPresContext;
class nsIRenderingContext;
class nsSpaceManager;
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

#define NS_CSS_MINMAX(_value,_min,_max) \
    ((_value) < (_min)           \
     ? (_min)                    \
     : ((_value) > (_max)        \
        ? (_max)                 \
        : (_value)))






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

  
  nsIRenderingContext* rendContext;

  
  nsMargin         mComputedMargin;

  
  nsMargin         mComputedBorderPadding;

  
  nsMargin         mComputedPadding;

  
  nsCSSOffsetState(nsIFrame *aFrame, nsIRenderingContext *aRenderingContext)
    : frame(aFrame)
    , rendContext(aRenderingContext)
  {
  }

  nsCSSOffsetState(nsIFrame *aFrame, nsIRenderingContext *aRenderingContext,
                   nscoord aContainingBlockWidth)
    : frame(aFrame)
    , rendContext(aRenderingContext)
  {
    InitOffsets(aContainingBlockWidth);
  }

  void InitOffsets(nscoord aContainingBlockWidth,
                   const nsMargin *aBorder = nsnull,
                   const nsMargin *aPadding = nsnull);

  
  static void DestroyMarginFunc(void*    aFrame,
                                nsIAtom* aPropertyName,
                                void*    aPropertyValue,
                                void*    aDtorData);

private:
  
  
  void ComputeMargin(nscoord aContainingBlockWidth);
  
  
  
  void ComputePadding(nscoord aContainingBlockWidth);

protected:

  



  
  inline void ComputeWidthDependentValue(nscoord aContainingBlockWidth,
                                         const nsStyleCoord& aCoord,
                                         nscoord& aResult);

  




  inline nscoord ComputeWidthValue(nscoord aContainingBlockWidth,
                                   nscoord aContentEdgeToBoxSizing,
                                   nscoord aBoxSizingToMarginEdge,
                                   const nsStyleCoord& aCoord);
  
  
  nscoord ComputeWidthValue(nscoord aContainingBlockWidth,
                            PRUint8 aBoxSizing,
                            const nsStyleCoord& aCoord);

  



  
  inline void ComputeHeightDependentValue(nscoord aContainingBlockHeight,
                                          const nsStyleCoord& aCoord,
                                          nscoord& aResult);
};









struct nsHTMLReflowState : public nsCSSOffsetState {
  
  
  const nsHTMLReflowState* parentReflowState;

  
  
  
  
  
  nscoord              availableWidth;

  
  
  
  
  
  
  
  
  nscoord              availableHeight;

  
  
  nsCSSFrameType   mFrameType;

  
  nsSpaceManager* mSpaceManager;

  
  nsLineLayout*    mLineLayout;

  
  
  const nsHTMLReflowState *mCBReflowState;

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

  
  nsIFrame* mPercentHeightReflowInitiator;

  
  
  
  
  
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
                                     
  } mFlags;

#ifdef IBMBIDI
  nscoord mRightEdge;
#endif

  
  
  

  
  
  nsHTMLReflowState(nsPresContext*           aPresContext,
                    nsIFrame*                aFrame,
                    nsIRenderingContext*     aRenderingContext,
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

  



  static nscoord CalcLineHeight(nsIRenderingContext* aRenderingContext,
                                nsIFrame* aFrame);
  


  static nscoord CalcLineHeight(nsStyleContext* aStyleContext,
                                nsIDeviceContext* aDeviceContext);

  void InitFrameType();

  void ComputeContainingBlockRectangle(nsPresContext*          aPresContext,
                                       const nsHTMLReflowState* aContainingBlockRS,
                                       nscoord&                 aContainingBlockWidth,
                                       nscoord&                 aContainingBlockHeight);

  void CalculateBlockSideMargins(nscoord aAvailWidth,
                                 nscoord aComputedWidth);

  




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

  void SetTruncated(const nsHTMLReflowMetrics& aMetrics, nsReflowStatus* aStatus) const;

protected:

  void InitCBReflowState();
  void InitResizeFlags(nsPresContext* aPresContext);

  void InitConstraints(nsPresContext* aPresContext,
                       nscoord         aContainingBlockWidth,
                       nscoord         aContainingBlockHeight,
                       const nsMargin* aBorder,
                       const nsMargin* aPadding);

  
  
  
  nsIFrame* GetNearestContainingBlock(nsIFrame* aFrame, nscoord& aCBLeftEdge,
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
                              nscoord aContainingBlockHeight);

  
  
  
  void ComputeMinMaxValues(nscoord                  aContainingBlockWidth,
                           nscoord                  aContainingBlockHeight,
                           const nsHTMLReflowState* aContainingBlockRS);

  void CalculateHorizBorderPaddingMargin(nscoord aContainingBlockWidth,
                                         nscoord* aInsideBoxSizing,
                                         nscoord* aOutsideBoxSizing);

};

#endif 

