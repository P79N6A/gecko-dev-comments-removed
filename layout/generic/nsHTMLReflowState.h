






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
  typedef mozilla::WritingMode WritingMode;
  typedef mozilla::LogicalMargin LogicalMargin;

  
  nsIFrame*           frame;

  
  nsRenderingContext* rendContext;

  const nsMargin& ComputedPhysicalMargin() const { return mComputedMargin; }
  const nsMargin& ComputedPhysicalBorderPadding() const { return mComputedBorderPadding; }
  const nsMargin& ComputedPhysicalPadding() const { return mComputedPadding; }

  
  
  nsMargin& ComputedPhysicalMargin() { return mComputedMargin; }
  nsMargin& ComputedPhysicalBorderPadding() { return mComputedBorderPadding; }
  nsMargin& ComputedPhysicalPadding() { return mComputedPadding; }

  const LogicalMargin ComputedLogicalMargin() const
    { return LogicalMargin(mWritingMode, mComputedMargin); }
  const LogicalMargin ComputedLogicalBorderPadding() const
    { return LogicalMargin(mWritingMode, mComputedBorderPadding); }
  const LogicalMargin ComputedLogicalPadding() const
    { return LogicalMargin(mWritingMode, mComputedPadding); }

  void SetComputedLogicalMargin(const LogicalMargin& aMargin)
    { mComputedMargin = aMargin.GetPhysicalMargin(mWritingMode); }
  void SetComputedLogicalBorderPadding(const LogicalMargin& aMargin)
    { mComputedBorderPadding = aMargin.GetPhysicalMargin(mWritingMode); }
  void SetComputedLogicalPadding(const LogicalMargin& aMargin)
    { mComputedPadding = aMargin.GetPhysicalMargin(mWritingMode); }

  WritingMode GetWritingMode() const { return mWritingMode; }

protected:
  
  WritingMode      mWritingMode;

  
  

  
  nsMargin         mComputedMargin;

  
  nsMargin         mComputedBorderPadding;

  
  nsMargin         mComputedPadding;

public:
  
  nsCSSOffsetState(nsIFrame *aFrame, nsRenderingContext *aRenderingContext)
    : frame(aFrame)
    , rendContext(aRenderingContext)
    , mWritingMode(aFrame->GetWritingMode())
  {
  }

  nsCSSOffsetState(nsIFrame *aFrame, nsRenderingContext *aRenderingContext,
                   nscoord aContainingBlockWidth);

#ifdef DEBUG
  
  
  static void* DisplayInitOffsetsEnter(nsIFrame* aFrame,
                                       nsCSSOffsetState* aState,
                                       nscoord aInlineDirPercentBasis,
                                       nscoord aBlockDirPercentBasis,
                                       const nsMargin* aBorder,
                                       const nsMargin* aPadding);
  static void DisplayInitOffsetsExit(nsIFrame* aFrame,
                                     nsCSSOffsetState* aState,
                                     void* aValue);
#endif

private:
  














  bool ComputeMargin(nscoord aInlineDirPercentBasis,
                     nscoord aBlockDirPercentBasis);
  
  













   bool ComputePadding(nscoord aInlineDirPercentBasis,
                       nscoord aBlockDirPercentBasis, nsIAtom* aFrameType);

protected:

  void InitOffsets(nscoord aInlineDirPercentBasis,
                   nscoord aBlockDirPercentBasis,
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

  
  
  nsCSSFrameType   mFrameType;

  
  
  
  
  
  
  
  
  nscoord mBlockDelta;

  
  
  
  
  
  
  
  nscoord mOrthogonalLimit;

  
  
  
  
  
  nscoord AvailableWidth() const { return mAvailableWidth; }
  nscoord AvailableHeight() const { return mAvailableHeight; }
  nscoord ComputedWidth() const { return mComputedWidth; }
  nscoord ComputedHeight() const { return mComputedHeight; }
  nscoord ComputedMinWidth() const { return mComputedMinWidth; }
  nscoord ComputedMaxWidth() const { return mComputedMaxWidth; }
  nscoord ComputedMinHeight() const { return mComputedMinHeight; }
  nscoord ComputedMaxHeight() const { return mComputedMaxHeight; }

  nscoord& AvailableWidth() { return mAvailableWidth; }
  nscoord& AvailableHeight() { return mAvailableHeight; }
  nscoord& ComputedWidth() { return mComputedWidth; }
  nscoord& ComputedHeight() { return mComputedHeight; }
  nscoord& ComputedMinWidth() { return mComputedMinWidth; }
  nscoord& ComputedMaxWidth() { return mComputedMaxWidth; }
  nscoord& ComputedMinHeight() { return mComputedMinHeight; }
  nscoord& ComputedMaxHeight() { return mComputedMaxHeight; }

  
  
  
  
  nscoord AvailableISize() const
    { return mWritingMode.IsVertical() ? mAvailableHeight : mAvailableWidth; }
  nscoord AvailableBSize() const
    { return mWritingMode.IsVertical() ? mAvailableWidth : mAvailableHeight; }
  nscoord ComputedISize() const
    { return mWritingMode.IsVertical() ? mComputedHeight : mComputedWidth; }
  nscoord ComputedBSize() const
    { return mWritingMode.IsVertical() ? mComputedWidth : mComputedHeight; }
  nscoord ComputedMinISize() const
    { return mWritingMode.IsVertical() ? mComputedMinHeight : mComputedMinWidth; }
  nscoord ComputedMaxISize() const
    { return mWritingMode.IsVertical() ? mComputedMaxHeight : mComputedMaxWidth; }
  nscoord ComputedMinBSize() const
    { return mWritingMode.IsVertical() ? mComputedMinWidth : mComputedMinHeight; }
  nscoord ComputedMaxBSize() const
    { return mWritingMode.IsVertical() ? mComputedMaxWidth : mComputedMaxHeight; }

  nscoord& AvailableISize()
    { return mWritingMode.IsVertical() ? mAvailableHeight : mAvailableWidth; }
  nscoord& AvailableBSize()
    { return mWritingMode.IsVertical() ? mAvailableWidth : mAvailableHeight; }
  nscoord& ComputedISize()
    { return mWritingMode.IsVertical() ? mComputedHeight : mComputedWidth; }
  nscoord& ComputedBSize()
    { return mWritingMode.IsVertical() ? mComputedWidth : mComputedHeight; }
  nscoord& ComputedMinISize()
    { return mWritingMode.IsVertical() ? mComputedMinHeight : mComputedMinWidth; }
  nscoord& ComputedMaxISize()
    { return mWritingMode.IsVertical() ? mComputedMaxHeight : mComputedMaxWidth; }
  nscoord& ComputedMinBSize()
    { return mWritingMode.IsVertical() ? mComputedMinWidth : mComputedMinHeight; }
  nscoord& ComputedMaxBSize()
    { return mWritingMode.IsVertical() ? mComputedMaxWidth : mComputedMaxHeight; }

  mozilla::LogicalSize AvailableSize() const {
    return mozilla::LogicalSize(mWritingMode,
                                AvailableISize(), AvailableBSize());
  }
  mozilla::LogicalSize ComputedSize() const {
    return mozilla::LogicalSize(mWritingMode,
                                ComputedISize(), ComputedBSize());
  }
  mozilla::LogicalSize ComputedMinSize() const {
    return mozilla::LogicalSize(mWritingMode,
                                ComputedMinISize(), ComputedMinBSize());
  }
  mozilla::LogicalSize ComputedMaxSize() const {
    return mozilla::LogicalSize(mWritingMode,
                                ComputedMaxISize(), ComputedMaxBSize());
  }

  mozilla::LogicalSize AvailableSize(mozilla::WritingMode aWM) const
  { return AvailableSize().ConvertTo(aWM, mWritingMode); }
  mozilla::LogicalSize ComputedSize(mozilla::WritingMode aWM) const
    { return ComputedSize().ConvertTo(aWM, mWritingMode); }
  mozilla::LogicalSize ComputedMinSize(mozilla::WritingMode aWM) const
    { return ComputedMinSize().ConvertTo(aWM, mWritingMode); }
  mozilla::LogicalSize ComputedMaxSize(mozilla::WritingMode aWM) const
    { return ComputedMaxSize().ConvertTo(aWM, mWritingMode); }

  mozilla::LogicalSize ComputedSizeWithPadding() const {
    mozilla::WritingMode wm = GetWritingMode();
    return mozilla::LogicalSize(wm,
                                ComputedISize() +
                                ComputedLogicalPadding().IStartEnd(wm),
                                ComputedBSize() +
                                ComputedLogicalPadding().BStartEnd(wm));
  }

  mozilla::LogicalSize ComputedSizeWithPadding(mozilla::WritingMode aWM) const {
    return ComputedSizeWithPadding().ConvertTo(aWM, GetWritingMode());
  }

  mozilla::LogicalSize ComputedSizeWithBorderPadding() const {
    mozilla::WritingMode wm = GetWritingMode();
    return mozilla::LogicalSize(wm,
                                ComputedISize() +
                                ComputedLogicalBorderPadding().IStartEnd(wm),
                                ComputedBSize() +
                                ComputedLogicalBorderPadding().BStartEnd(wm));
  }

  mozilla::LogicalSize
  ComputedSizeWithBorderPadding(mozilla::WritingMode aWM) const {
    return ComputedSizeWithBorderPadding().ConvertTo(aWM, GetWritingMode());
  }

  mozilla::LogicalSize
  ComputedSizeWithMarginBorderPadding() const {
    mozilla::WritingMode wm = GetWritingMode();
    return mozilla::LogicalSize(wm,
                                ComputedISize() +
                                ComputedLogicalMargin().IStartEnd(wm) +
                                ComputedLogicalBorderPadding().IStartEnd(wm),
                                ComputedBSize() +
                                ComputedLogicalMargin().BStartEnd(wm) +
                                ComputedLogicalBorderPadding().BStartEnd(wm));
  }

  mozilla::LogicalSize
  ComputedSizeWithMarginBorderPadding(mozilla::WritingMode aWM) const {
    return ComputedSizeWithMarginBorderPadding().ConvertTo(aWM,
                                                           GetWritingMode());
  }

  
  
  const nsMargin& ComputedPhysicalOffsets() const { return mComputedOffsets; }
  nsMargin& ComputedPhysicalOffsets() { return mComputedOffsets; }

  LogicalMargin ComputedLogicalOffsets() const
    { return LogicalMargin(mWritingMode, mComputedOffsets); }

private:
  
  
  
  
  nscoord              mAvailableWidth;

  
  
  
  
  
  
  
  
  nscoord              mAvailableHeight;

  
  
  
  
  
  
  
  
  nscoord          mComputedWidth; 

  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord          mComputedHeight;

  
  
  nsMargin         mComputedOffsets;

  
  
  
  nscoord          mComputedMinWidth, mComputedMaxWidth;
  nscoord          mComputedMinHeight, mComputedMaxHeight;

public:
  
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
                                     

    uint16_t mIsHResize:1;           
                                     

    uint16_t mIsVResize:1;           
                                     
                                     
                                     
                                     
    uint16_t mTableIsSplittable:1;   
                                     
    uint16_t mHeightDependsOnAncestorCell:1;   
                                               
    uint16_t mIsColumnBalancing:1;   
    uint16_t mIsFlexContainerMeasuringHeight:1; 
                                                
                                                
    uint16_t mDummyParentReflowState:1; 
                                        
                                        
    uint16_t mMustReflowPlaceholders:1; 
                                        
                                        
                                        
                                        
                                        
  } mFlags;

  
  
  
  bool IsHResize() const {
    return mFlags.mIsHResize;
  }
  bool IsVResize() const {
    return mFlags.mIsVResize;
  }
  bool IsIResize() const {
    return mWritingMode.IsVertical() ? mFlags.mIsVResize : mFlags.mIsHResize;
  }
  bool IsBResize() const {
    return mWritingMode.IsVertical() ? mFlags.mIsHResize : mFlags.mIsVResize;
  }
  void SetHResize(bool aValue) {
    mFlags.mIsHResize = aValue;
  }
  void SetVResize(bool aValue) {
    mFlags.mIsVResize = aValue;
  }
  void SetIResize(bool aValue) {
    if (mWritingMode.IsVertical()) {
      mFlags.mIsVResize = aValue;
    } else {
      mFlags.mIsHResize = aValue;
    }
  }
  void SetBResize(bool aValue) {
    if (mWritingMode.IsVertical()) {
      mFlags.mIsHResize = aValue;
    } else {
      mFlags.mIsVResize = aValue;
    }
  }

  
  
  

  










  nsHTMLReflowState(nsPresContext*              aPresContext,
                    nsIFrame*                   aFrame,
                    nsRenderingContext*         aRenderingContext,
                    const mozilla::LogicalSize& aAvailableSpace,
                    uint32_t                    aFlags = 0);

  


















  nsHTMLReflowState(nsPresContext*              aPresContext,
                    const nsHTMLReflowState&    aParentReflowState,
                    nsIFrame*                   aFrame,
                    const mozilla::LogicalSize& aAvailableSpace,
                    nscoord                     aContainingBlockWidth = -1,
                    nscoord                     aContainingBlockHeight = -1,
                    uint32_t                    aFlags = 0);

  
  enum {
    
    
    DUMMY_PARENT_REFLOW_STATE = (1<<0),

    
    
    CALLER_WILL_INIT = (1<<1)
  };

  
  
  void Init(nsPresContext* aPresContext,
            nscoord         aContainingBlockISize = -1,
            nscoord         aContainingBlockBSize = -1,
            const nsMargin* aBorder = nullptr,
            const nsMargin* aPadding = nullptr);

  



  nscoord GetContainingBlockContentISize(mozilla::WritingMode aWritingMode) const;

  


  nscoord CalcLineHeight() const;

  












  static nscoord CalcLineHeight(nsIContent* aContent,
                                nsStyleContext* aStyleContext,
                                nscoord aBlockBSize,
                                float aFontSizeInflation);


  void ComputeContainingBlockRectangle(nsPresContext*          aPresContext,
                                       const nsHTMLReflowState* aContainingBlockRS,
                                       nscoord&                 aContainingBlockWidth,
                                       nscoord&                 aContainingBlockHeight);

  



  nscoord ApplyMinMaxWidth(nscoord aWidth) const {
    if (NS_UNCONSTRAINEDSIZE != ComputedMaxWidth()) {
      aWidth = std::min(aWidth, ComputedMaxWidth());
    }
    return std::max(aWidth, ComputedMinWidth());
  }

  



  nscoord ApplyMinMaxISize(nscoord aISize) const {
    if (NS_UNCONSTRAINEDSIZE != ComputedMaxISize()) {
      aISize = std::min(aISize, ComputedMaxISize());
    }
    return std::max(aISize, ComputedMinISize());
  }

  








  nscoord ApplyMinMaxHeight(nscoord aHeight, nscoord aConsumed = 0) const {
    aHeight += aConsumed;

    if (NS_UNCONSTRAINEDSIZE != ComputedMaxHeight()) {
      aHeight = std::min(aHeight, ComputedMaxHeight());
    }

    if (NS_UNCONSTRAINEDSIZE != ComputedMinHeight()) {
      aHeight = std::max(aHeight, ComputedMinHeight());
    }

    return aHeight - aConsumed;
  }

  








  nscoord ApplyMinMaxBSize(nscoord aBSize, nscoord aConsumed = 0) const {
    aBSize += aConsumed;

    if (NS_UNCONSTRAINEDSIZE != ComputedMaxBSize()) {
      aBSize = std::min(aBSize, ComputedMaxBSize());
    }

    if (NS_UNCONSTRAINEDSIZE != ComputedMinBSize()) {
      aBSize = std::max(aBSize, ComputedMinBSize());
    }

    return aBSize - aConsumed;
  }

  bool ShouldReflowAllKids() const {
    
    
    
    
    
    
    return (frame->GetStateBits() & NS_FRAME_IS_DIRTY) ||
           IsHResize() ||
           (IsVResize() && 
            (frame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT));
  }

  
  void SetComputedWidth(nscoord aComputedWidth);

  
  void SetComputedHeight(nscoord aComputedHeight);

  void SetComputedISize(nscoord aComputedISize) {
    if (mWritingMode.IsVertical()) {
      SetComputedHeight(aComputedISize);
    } else {
      SetComputedWidth(aComputedISize);
    }
  }

  void SetComputedBSize(nscoord aComputedBSize) {
    if (mWritingMode.IsVertical()) {
      SetComputedWidth(aComputedBSize);
    } else {
      SetComputedHeight(aComputedBSize);
    }
  }

  void SetComputedHeightWithoutResettingResizeFlags(nscoord aComputedHeight) {
    
    
    
    
    ComputedHeight() = aComputedHeight;
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
    ApplyRelativePositioning(frame, ComputedPhysicalOffsets(), aPosition);
  }

  static void
  ApplyRelativePositioning(nsIFrame* aFrame,
                           mozilla::WritingMode aWritingMode,
                           const mozilla::LogicalMargin& aComputedOffsets,
                           mozilla::LogicalPoint* aPosition,
                           nscoord aContainerWidth) {
    
    
    
    
    
    nscoord frameWidth = aFrame->GetSize().width;
    nsPoint pos = aPosition->GetPhysicalPoint(aWritingMode,
                                              aContainerWidth - frameWidth);
    ApplyRelativePositioning(aFrame,
                             aComputedOffsets.GetPhysicalMargin(aWritingMode),
                             &pos);
    *aPosition = mozilla::LogicalPoint(aWritingMode, pos,
                                       aContainerWidth - frameWidth);
  }

  void ApplyRelativePositioning(mozilla::LogicalPoint* aPosition,
                                nscoord aContainerWidth) const {
    ApplyRelativePositioning(frame, mWritingMode,
                             ComputedLogicalOffsets(), aPosition,
                             aContainerWidth);
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

  void CalculateBlockSideMargins(nsIAtom* aFrameType);
};

#endif 

