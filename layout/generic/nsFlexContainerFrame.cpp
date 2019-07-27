








#include "nsFlexContainerFrame.h"
#include "nsContentUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsDisplayList.h"
#include "nsIFrameInlines.h"
#include "nsLayoutUtils.h"
#include "nsPlaceholderFrame.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsStyleContext.h"
#include "prlog.h"
#include <algorithm>
#include "mozilla/LinkedList.h"
#include "mozilla/FloatingPoint.h"
#include "WritingModes.h"

using namespace mozilla;
using namespace mozilla::layout;



typedef nsFlexContainerFrame::FlexItem FlexItem;
typedef nsFlexContainerFrame::FlexLine FlexLine;
typedef nsFlexContainerFrame::FlexboxAxisTracker FlexboxAxisTracker;
typedef nsFlexContainerFrame::StrutInfo StrutInfo;

#ifdef PR_LOGGING
static PRLogModuleInfo*
GetFlexContainerLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("nsFlexContainerFrame");
  return sLog;
}
#endif 














enum AxisOrientationType {
  eAxis_LR,
  eAxis_RL,
  eAxis_TB,
  eAxis_BT,
  eNumAxisOrientationTypes 
};





enum AxisEdgeType {
  eAxisEdge_Start,
  eAxisEdge_End,
  eNumAxisEdges 
};



static const mozilla::Side
kAxisOrientationToSidesMap[eNumAxisOrientationTypes][eNumAxisEdges] = {
  { eSideLeft,   eSideRight  },  
  { eSideRight,  eSideLeft   },  
  { eSideTop,    eSideBottom },  
  { eSideBottom, eSideTop }      
};






static inline bool
AxisGrowsInPositiveDirection(AxisOrientationType aAxis)
{
  return eAxis_LR == aAxis || eAxis_TB == aAxis;
}



static inline AxisOrientationType
GetReverseAxis(AxisOrientationType aAxis)
{
  AxisOrientationType reversedAxis;

  if (aAxis % 2 == 0) {
    
    reversedAxis = AxisOrientationType(aAxis + 1);
  } else {
    
    reversedAxis = AxisOrientationType(aAxis - 1);
  }

  
  MOZ_ASSERT(reversedAxis >= eAxis_LR &&
             reversedAxis <= eAxis_BT);

  return reversedAxis;
}












static nscoord
PhysicalCoordFromFlexRelativeCoord(nscoord aFlexRelativeCoord,
                                   nscoord aContainerSize,
                                   AxisOrientationType aAxis) {
  if (AxisGrowsInPositiveDirection(aAxis)) {
    return aFlexRelativeCoord;
  }
  return aContainerSize - aFlexRelativeCoord;
}









#define GET_MAIN_COMPONENT(axisTracker_, width_, height_)  \
  (axisTracker_).IsMainAxisHorizontal() ? (width_) : (height_)

#define GET_CROSS_COMPONENT(axisTracker_, width_, height_)  \
  (axisTracker_).IsCrossAxisHorizontal() ? (width_) : (height_)


class MOZ_STACK_CLASS nsFlexContainerFrame::FlexboxAxisTracker {
public:
  FlexboxAxisTracker(const nsStylePosition* aStylePosition,
                     const WritingMode& aWM);

  
  
  AxisOrientationType GetMainAxis() const  { return mMainAxis;  }
  AxisOrientationType GetCrossAxis() const { return mCrossAxis; }

  bool IsMainAxisHorizontal() const {
    
    
    
    return mIsRowOriented != mWM.IsVertical();
  }
  bool IsCrossAxisHorizontal() const {
    return !IsMainAxisHorizontal();
  }
  

  
  
  bool IsMainAxisReversed() const {
    return mIsMainAxisReversed;
  }
  
  
  bool IsCrossAxisReversed() const {
    return mIsCrossAxisReversed;
  }

  bool IsRowOriented() const { return mIsRowOriented; }

  nscoord GetMainComponent(const nsSize& aSize) const {
    return GET_MAIN_COMPONENT(*this, aSize.width, aSize.height);
  }
  int32_t GetMainComponent(const LayoutDeviceIntSize& aIntSize) const {
    return GET_MAIN_COMPONENT(*this, aIntSize.width, aIntSize.height);
  }

  nscoord GetCrossComponent(const nsSize& aSize) const {
    return GET_CROSS_COMPONENT(*this, aSize.width, aSize.height);
  }
  int32_t GetCrossComponent(const LayoutDeviceIntSize& aIntSize) const {
    return GET_CROSS_COMPONENT(*this, aIntSize.width, aIntSize.height);
  }

  nscoord GetMarginSizeInMainAxis(const nsMargin& aMargin) const {
    return IsMainAxisHorizontal() ?
      aMargin.LeftRight() :
      aMargin.TopBottom();
  }
  nscoord GetMarginSizeInCrossAxis(const nsMargin& aMargin) const {
    return IsCrossAxisHorizontal() ?
      aMargin.LeftRight() :
      aMargin.TopBottom();
  }

  
  
  
  const nsStyleCoord& ComputedCrossSize(const nsIFrame* aFrame) const {
    const nsStylePosition* stylePos = aFrame->StylePosition();

    return IsCrossAxisHorizontal() ?
      stylePos->mWidth :
      stylePos->mHeight;
  }

  














  nsPoint PhysicalPointFromFlexRelativePoint(nscoord aMainCoord,
                                             nscoord aCrossCoord,
                                             nscoord aContainerMainSize,
                                             nscoord aContainerCrossSize) const {
    nscoord physicalCoordInMainAxis =
      PhysicalCoordFromFlexRelativeCoord(aMainCoord, aContainerMainSize,
                                         mMainAxis);
    nscoord physicalCoordInCrossAxis =
      PhysicalCoordFromFlexRelativeCoord(aCrossCoord, aContainerCrossSize,
                                         mCrossAxis);

    return IsMainAxisHorizontal() ?
      nsPoint(physicalCoordInMainAxis, physicalCoordInCrossAxis) :
      nsPoint(physicalCoordInCrossAxis, physicalCoordInMainAxis);
  }


  nsSize PhysicalSizeFromFlexRelativeSizes(nscoord aMainSize,
                                           nscoord aCrossSize) const {
    return IsMainAxisHorizontal() ?
      nsSize(aMainSize, aCrossSize) :
      nsSize(aCrossSize, aMainSize);
  }

  
  
  
  bool AreAxesInternallyReversed() const
  {
    return mAreAxesInternallyReversed;
  }

private:
  
  
  FlexboxAxisTracker(const FlexboxAxisTracker&) = delete;
  FlexboxAxisTracker& operator=(const FlexboxAxisTracker&) = delete;

  
  AxisOrientationType mMainAxis;
  AxisOrientationType mCrossAxis;
  

  const WritingMode mWM; 

  bool mIsRowOriented; 
                       

  bool mIsMainAxisReversed; 
                            
  bool mIsCrossAxisReversed; 
                             

  
  
  
  bool mAreAxesInternallyReversed;
};






class nsFlexContainerFrame::FlexItem : public LinkedListElement<FlexItem>
{
public:
  
  FlexItem(nsHTMLReflowState& aFlexItemReflowState,
           float aFlexGrow, float aFlexShrink, nscoord aMainBaseSize,
           nscoord aMainMinSize, nscoord aMainMaxSize,
           nscoord aTentativeCrossSize,
           nscoord aCrossMinSize, nscoord aCrossMaxSize,
           const FlexboxAxisTracker& aAxisTracker);

  
  FlexItem(nsIFrame* aChildFrame, nscoord aCrossSize);

  
  nsIFrame* Frame() const          { return mFrame; }
  nscoord GetFlexBaseSize() const  { return mFlexBaseSize; }

  nscoord GetMainMinSize() const   {
    MOZ_ASSERT(!mNeedsMinSizeAutoResolution,
               "Someone's using an unresolved 'auto' main min-size");
    return mMainMinSize;
  }
  nscoord GetMainMaxSize() const   { return mMainMaxSize; }

  
  nscoord GetMainSize() const      { return mMainSize; }
  nscoord GetMainPosition() const  { return mMainPosn; }

  nscoord GetCrossMinSize() const  { return mCrossMinSize; }
  nscoord GetCrossMaxSize() const  { return mCrossMaxSize; }

  
  nscoord GetCrossSize() const     { return mCrossSize;  }
  nscoord GetCrossPosition() const { return mCrossPosn; }

  nscoord ResolvedAscent() const {
    if (mAscent == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
      
      
      
      
      
      
      
      WritingMode wm = mFrame->GetWritingMode();
      
      if (!nsLayoutUtils::GetFirstLineBaseline(wm, mFrame, &mAscent)) {
        mAscent = mFrame->GetLogicalBaseline(wm);
      }
    }
    return mAscent;
  }

  
  
  
  nscoord GetOuterMainSize(AxisOrientationType aMainAxis) const
  {
    return mMainSize + GetMarginBorderPaddingSizeInAxis(aMainAxis);
  }

  nscoord GetOuterCrossSize(AxisOrientationType aCrossAxis) const
  {
    return mCrossSize + GetMarginBorderPaddingSizeInAxis(aCrossAxis);
  }

  
  
  
  
  nscoord GetBaselineOffsetFromOuterCrossEdge(
    AxisEdgeType aEdge,
    const FlexboxAxisTracker& aAxisTracker) const;

  float GetShareOfWeightSoFar() const { return mShareOfWeightSoFar; }

  bool IsFrozen() const            { return mIsFrozen; }

  bool HadMinViolation() const     { return mHadMinViolation; }
  bool HadMaxViolation() const     { return mHadMaxViolation; }

  
  
  bool HadMeasuringReflow() const  { return mHadMeasuringReflow; }

  
  
  
  bool IsStretched() const         { return mIsStretched; }

  
  
  bool NeedsMinSizeAutoResolution() const
    { return mNeedsMinSizeAutoResolution; }

  
  
  bool IsStrut() const             { return mIsStrut; }

  uint8_t GetAlignSelf() const     { return mAlignSelf; }

  
  
  
  
  
  float GetFlexFactor(bool aIsUsingFlexGrow)
  {
    MOZ_ASSERT(!IsFrozen(), "shouldn't need flex factor after item is frozen");

    return aIsUsingFlexGrow ? mFlexGrow : mFlexShrink;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  float GetWeight(bool aIsUsingFlexGrow)
  {
    MOZ_ASSERT(!IsFrozen(), "shouldn't need weight after item is frozen");

    if (aIsUsingFlexGrow) {
      return mFlexGrow;
    }

    
    if (mFlexBaseSize == 0) {
      
      
      
      
      return 0.0f;
    }
    return mFlexShrink * mFlexBaseSize;
  }

  
  
  const nsMargin& GetMargin() const { return mMargin; }

  
  nscoord GetMarginComponentForSide(mozilla::Side aSide) const
  { return mMargin.Side(aSide); }

  
  nscoord GetMarginSizeInAxis(AxisOrientationType aAxis) const
  {
    mozilla::Side startSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_Start];
    mozilla::Side endSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_End];
    return GetMarginComponentForSide(startSide) +
      GetMarginComponentForSide(endSide);
  }

  
  
  const nsMargin& GetBorderPadding() const { return mBorderPadding; }

  
  nscoord GetBorderPaddingComponentForSide(mozilla::Side aSide) const
  { return mBorderPadding.Side(aSide); }

  
  
  nscoord GetBorderPaddingSizeInAxis(AxisOrientationType aAxis) const
  {
    mozilla::Side startSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_Start];
    mozilla::Side endSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_End];
    return GetBorderPaddingComponentForSide(startSide) +
      GetBorderPaddingComponentForSide(endSide);
  }

  
  
  
  
  nscoord GetMarginBorderPaddingSizeInAxis(AxisOrientationType aAxis) const
  {
    return GetMarginSizeInAxis(aAxis) + GetBorderPaddingSizeInAxis(aAxis);
  }

  
  
  
  
  void UpdateMainMinSize(nscoord aNewMinSize)
  {
    NS_ASSERTION(aNewMinSize >= 0,
                 "How did we end up with a negative min-size?");
    MOZ_ASSERT(mMainMaxSize >= aNewMinSize,
               "Should only use this function for resolving min-size:auto, "
               "and main max-size should be an upper-bound for resolved val");
    MOZ_ASSERT(mNeedsMinSizeAutoResolution &&
               (mMainMinSize == 0 || mFrame->IsThemed(mFrame->StyleDisplay())),
               "Should only use this function for resolving min-size:auto, "
               "so we shouldn't already have a nonzero min-size established "
               "(unless it's a themed-widget-imposed minimum size)");

    if (aNewMinSize > mMainMinSize) {
      mMainMinSize = aNewMinSize;
      
      mMainSize = std::max(mMainSize, aNewMinSize);
    }
    mNeedsMinSizeAutoResolution = false;
  }

  
  
  
  void SetFlexBaseSizeAndMainSize(nscoord aNewFlexBaseSize)
  {
    MOZ_ASSERT(!mIsFrozen || mFlexBaseSize == NS_INTRINSICSIZE,
               "flex base size shouldn't change after we're frozen "
               "(unless we're just resolving an intrinsic size)");
    mFlexBaseSize = aNewFlexBaseSize;

    
    
    
    mMainSize = NS_CSS_MINMAX(mFlexBaseSize, mMainMinSize, mMainMaxSize);
  }

  
  

  
  void SetMainSize(nscoord aNewMainSize)
  {
    MOZ_ASSERT(!mIsFrozen, "main size shouldn't change after we're frozen");
    mMainSize = aNewMainSize;
  }

  void SetShareOfWeightSoFar(float aNewShare)
  {
    MOZ_ASSERT(!mIsFrozen || aNewShare == 0.0f,
               "shouldn't be giving this item any share of the weight "
               "after it's frozen");
    mShareOfWeightSoFar = aNewShare;
  }

  void Freeze() { mIsFrozen = true; }

  void SetHadMinViolation()
  {
    MOZ_ASSERT(!mIsFrozen,
               "shouldn't be changing main size & having violations "
               "after we're frozen");
    mHadMinViolation = true;
  }
  void SetHadMaxViolation()
  {
    MOZ_ASSERT(!mIsFrozen,
               "shouldn't be changing main size & having violations "
               "after we're frozen");
    mHadMaxViolation = true;
  }
  void ClearViolationFlags()
  { mHadMinViolation = mHadMaxViolation = false; }

  
  

  
  
  
  void SetMainPosition(nscoord aPosn) {
    MOZ_ASSERT(mIsFrozen, "main size should be resolved before this");
    mMainPosn  = aPosn;
  }

  
  void SetCrossSize(nscoord aCrossSize) {
    MOZ_ASSERT(!mIsStretched,
               "Cross size shouldn't be modified after it's been stretched");
    mCrossSize = aCrossSize;
  }

  
  
  
  void SetCrossPosition(nscoord aPosn) {
    MOZ_ASSERT(mIsFrozen, "main size should be resolved before this");
    mCrossPosn = aPosn;
  }

  
  
  
  
  
  
  
  void SetAscent(nscoord aAscent) const {
    mAscent = aAscent; 
  }

  void SetHadMeasuringReflow() {
    mHadMeasuringReflow = true;
  }

  void SetIsStretched() {
    MOZ_ASSERT(mIsFrozen, "main size should be resolved before this");
    mIsStretched = true;
  }

  
  void SetMarginComponentForSide(mozilla::Side aSide, nscoord aLength)
  {
    MOZ_ASSERT(mIsFrozen, "main size should be resolved before this");
    mMargin.Side(aSide) = aLength;
  }

  void ResolveStretchedCrossSize(nscoord aLineCrossSize,
                                 const FlexboxAxisTracker& aAxisTracker);

  uint32_t GetNumAutoMarginsInAxis(AxisOrientationType aAxis) const;

protected:
  
  void CheckForMinSizeAuto(const nsHTMLReflowState& aFlexItemReflowState,
                           const FlexboxAxisTracker& aAxisTracker);

  
  nsIFrame* const mFrame;

  
  const float mFlexGrow;
  const float mFlexShrink;

  const nsMargin mBorderPadding;
  nsMargin mMargin; 

  
  
  
  nscoord mFlexBaseSize;
  nscoord mMainMinSize;
  nscoord mMainMaxSize;

  const nscoord mCrossMinSize;
  const nscoord mCrossMaxSize;

  
  nscoord mMainSize;
  nscoord mMainPosn;
  nscoord mCrossSize;
  nscoord mCrossPosn;
  mutable nscoord mAscent; 
                           

  
  
  
  
  
  float mShareOfWeightSoFar;
  bool mIsFrozen;
  bool mHadMinViolation;
  bool mHadMaxViolation;

  
  bool mHadMeasuringReflow; 
                            
  bool mIsStretched; 
  bool mIsStrut;     
                     

  
  bool mNeedsMinSizeAutoResolution;

  uint8_t mAlignSelf; 
                      
                      
};





class nsFlexContainerFrame::FlexLine : public LinkedListElement<FlexLine>
{
public:
  FlexLine()
  : mNumItems(0),
    mNumFrozenItems(0),
    mTotalInnerHypotheticalMainSize(0),
    mTotalOuterHypotheticalMainSize(0),
    mLineCrossSize(0),
    mBaselineOffset(nscoord_MIN)
  {}

  
  
  nscoord GetTotalOuterHypotheticalMainSize() const {
    return mTotalOuterHypotheticalMainSize;
  }

  
  FlexItem* GetFirstItem()
  {
    MOZ_ASSERT(mItems.isEmpty() == (mNumItems == 0),
               "mNumItems bookkeeping is off");
    return mItems.getFirst();
  }

  const FlexItem* GetFirstItem() const
  {
    MOZ_ASSERT(mItems.isEmpty() == (mNumItems == 0),
               "mNumItems bookkeeping is off");
    return mItems.getFirst();
  }

  bool IsEmpty() const
  {
    MOZ_ASSERT(mItems.isEmpty() == (mNumItems == 0),
               "mNumItems bookkeeping is off");
    return mItems.isEmpty();
  }

  uint32_t NumItems() const
  {
    MOZ_ASSERT(mItems.isEmpty() == (mNumItems == 0),
               "mNumItems bookkeeping is off");
    return mNumItems;
  }

  
  
  
  
  void AddItem(FlexItem* aItem,
               bool aShouldInsertAtFront,
               nscoord aItemInnerHypotheticalMainSize,
               nscoord aItemOuterHypotheticalMainSize)
  {
    if (aShouldInsertAtFront) {
      mItems.insertFront(aItem);
    } else {
      mItems.insertBack(aItem);
    }

    
    mNumItems++;
    if (aItem->IsFrozen()) {
      mNumFrozenItems++;
    }
    mTotalInnerHypotheticalMainSize += aItemInnerHypotheticalMainSize;
    mTotalOuterHypotheticalMainSize += aItemOuterHypotheticalMainSize;
  }

  
  
  void ComputeCrossSizeAndBaseline(const FlexboxAxisTracker& aAxisTracker);

  
  nscoord GetLineCrossSize() const { return mLineCrossSize; }

  
  
  
  void SetLineCrossSize(nscoord aLineCrossSize) {
    mLineCrossSize = aLineCrossSize;
  }

  








  nscoord GetBaselineOffset() const {
    return mBaselineOffset;
  }

  
  
  void ResolveFlexibleLengths(nscoord aFlexContainerMainSize);

  void PositionItemsInMainAxis(uint8_t aJustifyContent,
                               nscoord aContentBoxMainSize,
                               const FlexboxAxisTracker& aAxisTracker);

  void PositionItemsInCrossAxis(nscoord aLineStartPosition,
                                const FlexboxAxisTracker& aAxisTracker);

  friend class AutoFlexLineListClearer; 

private:
  
  void FreezeItemsEarly(bool aIsUsingFlexGrow);

  void FreezeOrRestoreEachFlexibleSize(const nscoord aTotalViolation,
                                       bool aIsFinalIteration);

  LinkedList<FlexItem> mItems; 

  uint32_t mNumItems; 
                      
                      
                      
                      

  
  
  
  uint32_t mNumFrozenItems;

  nscoord mTotalInnerHypotheticalMainSize;
  nscoord mTotalOuterHypotheticalMainSize;
  nscoord mLineCrossSize;
  nscoord mBaselineOffset;
};



struct nsFlexContainerFrame::StrutInfo {
  StrutInfo(uint32_t aItemIdx, nscoord aStrutCrossSize)
    : mItemIdx(aItemIdx),
      mStrutCrossSize(aStrutCrossSize)
  {
  }

  uint32_t mItemIdx;      
  nscoord mStrutCrossSize; 
};

static void
BuildStrutInfoFromCollapsedItems(const FlexLine* aFirstLine,
                                 nsTArray<StrutInfo>& aStruts)
{
  MOZ_ASSERT(aFirstLine, "null first line pointer");
  MOZ_ASSERT(aStruts.IsEmpty(),
             "We should only build up StrutInfo once per reflow, so "
             "aStruts should be empty when this is called");

  uint32_t itemIdxInContainer = 0;
  for (const FlexLine* line = aFirstLine; line; line = line->getNext()) {
    for (const FlexItem* item = line->GetFirstItem(); item;
         item = item->getNext()) {
      if (NS_STYLE_VISIBILITY_COLLAPSE ==
          item->Frame()->StyleVisibility()->mVisible) {
        
        aStruts.AppendElement(StrutInfo(itemIdxInContainer,
                                        line->GetLineCrossSize()));
      }
      itemIdxInContainer++;
    }
  }
}


static nsIFrame*
GetFirstNonAnonBoxDescendant(nsIFrame* aFrame)
{
  while (aFrame) {
    nsIAtom* pseudoTag = aFrame->StyleContext()->GetPseudo();

    
    if (!pseudoTag ||                                 
        !nsCSSAnonBoxes::IsAnonBox(pseudoTag) ||      
        pseudoTag == nsCSSAnonBoxes::mozNonElement) { 
      break;
    }

    

    
    
    
    
    
    
    
    if (MOZ_UNLIKELY(aFrame->GetType() == nsGkAtoms::tableOuterFrame)) {
      nsIFrame* captionDescendant =
        GetFirstNonAnonBoxDescendant(aFrame->GetFirstChild(kCaptionList));
      if (captionDescendant) {
        return captionDescendant;
      }
    } else if (MOZ_UNLIKELY(aFrame->GetType() == nsGkAtoms::tableFrame)) {
      nsIFrame* colgroupDescendant =
        GetFirstNonAnonBoxDescendant(aFrame->GetFirstChild(kColGroupList));
      if (colgroupDescendant) {
        return colgroupDescendant;
      }
    }

    
    aFrame = aFrame->GetFirstPrincipalChild();
  }
  return aFrame;
}
















bool
IsOrderLEQWithDOMFallback(nsIFrame* aFrame1,
                          nsIFrame* aFrame2)
{
  MOZ_ASSERT(aFrame1->IsFlexItem() && aFrame2->IsFlexItem(),
             "this method only intended for comparing flex items");

  if (aFrame1 == aFrame2) {
    
    
    NS_ERROR("Why are we checking if a frame is LEQ itself?");
    return true;
  }

  
  {
    nsIFrame* aRealFrame1 = nsPlaceholderFrame::GetRealFrameFor(aFrame1);
    nsIFrame* aRealFrame2 = nsPlaceholderFrame::GetRealFrameFor(aFrame2);

    int32_t order1 = aRealFrame1->StylePosition()->mOrder;
    int32_t order2 = aRealFrame2->StylePosition()->mOrder;

    if (order1 != order2) {
      return order1 < order2;
    }
  }

  
  
  
  aFrame1 = GetFirstNonAnonBoxDescendant(aFrame1);
  aFrame2 = GetFirstNonAnonBoxDescendant(aFrame2);
  MOZ_ASSERT(aFrame1 && aFrame2,
             "why do we have an anonymous box without any "
             "non-anonymous descendants?");


  
  
  
  
  
  
  nsIAtom* pseudo1 = aFrame1->StyleContext()->GetPseudo();
  nsIAtom* pseudo2 = aFrame2->StyleContext()->GetPseudo();
  if (pseudo1 == nsCSSPseudoElements::before ||
      pseudo2 == nsCSSPseudoElements::after) {
    
    return true;
  }
  if (pseudo1 == nsCSSPseudoElements::after ||
      pseudo2 == nsCSSPseudoElements::before) {
    
    return false;
  }

  
  nsIContent* content1 = aFrame1->GetContent();
  nsIContent* content2 = aFrame2->GetContent();
  MOZ_ASSERT(content1 != content2,
             "Two different flex items are using the same nsIContent node for "
             "comparison, so we may be sorting them in an arbitrary order");

  return nsContentUtils::PositionIsBefore(content1, content2);
}













bool
IsOrderLEQ(nsIFrame* aFrame1,
           nsIFrame* aFrame2)
{
  MOZ_ASSERT(aFrame1->IsFlexItem() && aFrame2->IsFlexItem(),
             "this method only intended for comparing flex items");

  
  nsIFrame* aRealFrame1 = nsPlaceholderFrame::GetRealFrameFor(aFrame1);
  nsIFrame* aRealFrame2 = nsPlaceholderFrame::GetRealFrameFor(aFrame2);

  int32_t order1 = aRealFrame1->StylePosition()->mOrder;
  int32_t order2 = aRealFrame2->StylePosition()->mOrder;

  return order1 <= order2;
}

bool
nsFlexContainerFrame::IsHorizontal()
{
  const FlexboxAxisTracker axisTracker(StylePosition(), GetWritingMode());
  return axisTracker.IsMainAxisHorizontal();
}

FlexItem*
nsFlexContainerFrame::GenerateFlexItemForChild(
  nsPresContext* aPresContext,
  nsIFrame*      aChildFrame,
  const nsHTMLReflowState& aParentReflowState,
  const FlexboxAxisTracker& aAxisTracker)
{
  
  
  
  nsHTMLReflowState
    childRS(aPresContext, aParentReflowState, aChildFrame,
            aParentReflowState.ComputedSize(aChildFrame->GetWritingMode()));

  
  
  const nsStylePosition* stylePos = aChildFrame->StylePosition();
  float flexGrow   = stylePos->mFlexGrow;
  float flexShrink = stylePos->mFlexShrink;

  
  
  nscoord flexBaseSize = GET_MAIN_COMPONENT(aAxisTracker,
                                            childRS.ComputedWidth(),
                                            childRS.ComputedHeight());
  nscoord mainMinSize = GET_MAIN_COMPONENT(aAxisTracker,
                                           childRS.ComputedMinWidth(),
                                           childRS.ComputedMinHeight());
  nscoord mainMaxSize = GET_MAIN_COMPONENT(aAxisTracker,
                                           childRS.ComputedMaxWidth(),
                                           childRS.ComputedMaxHeight());
  
  MOZ_ASSERT(mainMinSize <= mainMaxSize, "min size is larger than max size");

  
  
  
  
  
  nscoord tentativeCrossSize = GET_CROSS_COMPONENT(aAxisTracker,
                                                   childRS.ComputedWidth(),
                                                   childRS.ComputedHeight());
  nscoord crossMinSize = GET_CROSS_COMPONENT(aAxisTracker,
                                             childRS.ComputedMinWidth(),
                                             childRS.ComputedMinHeight());
  nscoord crossMaxSize = GET_CROSS_COMPONENT(aAxisTracker,
                                             childRS.ComputedMaxWidth(),
                                             childRS.ComputedMaxHeight());

  
  
  
  
  bool isFixedSizeWidget = false;
  const nsStyleDisplay* disp = aChildFrame->StyleDisplay();
  if (aChildFrame->IsThemed(disp)) {
    LayoutDeviceIntSize widgetMinSize;
    bool canOverride = true;
    aPresContext->GetTheme()->
      GetMinimumWidgetSize(aPresContext, aChildFrame,
                           disp->mAppearance,
                           &widgetMinSize, &canOverride);

    nscoord widgetMainMinSize =
      aPresContext->DevPixelsToAppUnits(
        aAxisTracker.GetMainComponent(widgetMinSize));
    nscoord widgetCrossMinSize =
      aPresContext->DevPixelsToAppUnits(
        aAxisTracker.GetCrossComponent(widgetMinSize));

    
    
    nsMargin& bp = childRS.ComputedPhysicalBorderPadding();
    widgetMainMinSize = std::max(widgetMainMinSize -
                                 aAxisTracker.GetMarginSizeInMainAxis(bp), 0);
    widgetCrossMinSize = std::max(widgetCrossMinSize -
                                  aAxisTracker.GetMarginSizeInCrossAxis(bp), 0);

    if (!canOverride) {
      
      
      
      flexBaseSize = mainMinSize = mainMaxSize = widgetMainMinSize;
      tentativeCrossSize = crossMinSize = crossMaxSize = widgetCrossMinSize;
      isFixedSizeWidget = true;
    } else {
      
      
      mainMinSize = std::max(mainMinSize, widgetMainMinSize);
      mainMaxSize = std::max(mainMaxSize, widgetMainMinSize);

      if (tentativeCrossSize != NS_INTRINSICSIZE) {
        tentativeCrossSize = std::max(tentativeCrossSize, widgetCrossMinSize);
      }
      crossMinSize = std::max(crossMinSize, widgetCrossMinSize);
      crossMaxSize = std::max(crossMaxSize, widgetCrossMinSize);
    }
  }

  
  FlexItem* item = new FlexItem(childRS,
                                flexGrow, flexShrink, flexBaseSize,
                                mainMinSize, mainMaxSize,
                                tentativeCrossSize,
                                crossMinSize, crossMaxSize,
                                aAxisTracker);

  
  
  
  if (isFixedSizeWidget || (flexGrow == 0.0f && flexShrink == 0.0f)) {
    item->Freeze();
  }

  
  
  ResolveAutoFlexBasisAndMinSize(aPresContext, *item,
                                 childRS, aAxisTracker);
  return item;
}






static bool
IsCrossSizeDefinite(const nsHTMLReflowState& aItemReflowState,
                    const FlexboxAxisTracker& aAxisTracker)
{
  const nsStylePosition* pos = aItemReflowState.mStylePosition;
  if (aAxisTracker.IsCrossAxisHorizontal()) {
    return pos->mWidth.GetUnit() != eStyleUnit_Auto;
  }
  
  
  nscoord cbHeight = aItemReflowState.mCBReflowState->ComputedHeight();
  return !nsLayoutUtils::IsAutoHeight(pos->mHeight, cbHeight);
}














static nscoord
CrossSizeToUseWithRatio(const FlexItem& aFlexItem,
                        const nsHTMLReflowState& aItemReflowState,
                        bool aMinSizeFallback,
                        const FlexboxAxisTracker& aAxisTracker)
{
  if (aFlexItem.IsStretched()) {
    
    return aFlexItem.GetCrossSize();
  }

  if (IsCrossSizeDefinite(aItemReflowState, aAxisTracker)) {
    
    return GET_CROSS_COMPONENT(aAxisTracker,
                               aItemReflowState.ComputedWidth(),
                               aItemReflowState.ComputedHeight());
  }

  if (aMinSizeFallback) {
    
    
    return GET_CROSS_COMPONENT(aAxisTracker,
                               aItemReflowState.ComputedMinWidth(),
                               aItemReflowState.ComputedMinHeight());
  }

  
  return NS_AUTOHEIGHT;
}




static nscoord
MainSizeFromAspectRatio(nscoord aCrossSize,
                        const nsSize& aIntrinsicRatio,
                        const FlexboxAxisTracker& aAxisTracker)
{
  MOZ_ASSERT(aAxisTracker.GetCrossComponent(aIntrinsicRatio) != 0,
             "Invalid ratio; will divide by 0! Caller should've checked...");

  if (aAxisTracker.IsCrossAxisHorizontal()) {
    
    return NSCoordMulDiv(aCrossSize, aIntrinsicRatio.height, aIntrinsicRatio.width);
  }
  
  return NSCoordMulDiv(aCrossSize, aIntrinsicRatio.width, aIntrinsicRatio.height);
}








static nscoord
PartiallyResolveAutoMinSize(const FlexItem& aFlexItem,
                            const nsHTMLReflowState& aItemReflowState,
                            const nsSize& aIntrinsicRatio,
                            const FlexboxAxisTracker& aAxisTracker)
{
  MOZ_ASSERT(aFlexItem.NeedsMinSizeAutoResolution(),
             "only call for FlexItems that need min-size auto resolution");

  nscoord minMainSize = nscoord_MAX; 
                                     

  
  
  
  if (eStyleUnit_Auto ==
      aItemReflowState.mStylePosition->mFlexBasis.GetUnit() &&
      aFlexItem.GetFlexBaseSize() != NS_AUTOHEIGHT) {
    
    
    
    
    minMainSize = std::min(minMainSize, aFlexItem.GetFlexBaseSize());
  }

  
  nscoord maxSize =
    GET_MAIN_COMPONENT(aAxisTracker,
                       aItemReflowState.ComputedMaxWidth(),
                       aItemReflowState.ComputedMaxHeight());
  if (maxSize != NS_UNCONSTRAINEDSIZE) {
    minMainSize = std::min(minMainSize, maxSize);
  }

  
  

  
  
  
  if (aAxisTracker.GetCrossComponent(aIntrinsicRatio) != 0) {
    
    const bool useMinSizeIfCrossSizeIsIndefinite = true;
    nscoord crossSizeToUseWithRatio =
      CrossSizeToUseWithRatio(aFlexItem, aItemReflowState,
                              useMinSizeIfCrossSizeIsIndefinite,
                              aAxisTracker);
    nscoord minMainSizeFromRatio =
      MainSizeFromAspectRatio(crossSizeToUseWithRatio,
                              aIntrinsicRatio, aAxisTracker);
    minMainSize = std::min(minMainSize, minMainSizeFromRatio);
  }

  return minMainSize;
}





static bool
ResolveAutoFlexBasisFromRatio(FlexItem& aFlexItem,
                              const nsHTMLReflowState& aItemReflowState,
                              const nsSize& aIntrinsicRatio,
                              const FlexboxAxisTracker& aAxisTracker)
{
  MOZ_ASSERT(NS_AUTOHEIGHT == aFlexItem.GetFlexBaseSize(),
             "Should only be called to resolve an 'auto' flex-basis");
  
  
  
  
  
  
  if (aAxisTracker.GetCrossComponent(aIntrinsicRatio) != 0) {
    
    const bool useMinSizeIfCrossSizeIsIndefinite = false;
    nscoord crossSizeToUseWithRatio =
      CrossSizeToUseWithRatio(aFlexItem, aItemReflowState,
                              useMinSizeIfCrossSizeIsIndefinite,
                              aAxisTracker);
    if (crossSizeToUseWithRatio != NS_AUTOHEIGHT) {
      
      nscoord mainSizeFromRatio =
        MainSizeFromAspectRatio(crossSizeToUseWithRatio,
                                aIntrinsicRatio, aAxisTracker);
      aFlexItem.SetFlexBaseSizeAndMainSize(mainSizeFromRatio);
      return true;
    }
  }
  return false;
}



void
nsFlexContainerFrame::
  ResolveAutoFlexBasisAndMinSize(nsPresContext* aPresContext,
                                 FlexItem& aFlexItem,
                                 const nsHTMLReflowState& aItemReflowState,
                                 const FlexboxAxisTracker& aAxisTracker)
{
  
  
  const bool isMainSizeAuto = (!aAxisTracker.IsMainAxisHorizontal() &&
                               NS_AUTOHEIGHT == aFlexItem.GetFlexBaseSize());

  const bool isMainMinSizeAuto = aFlexItem.NeedsMinSizeAutoResolution();

  if (!isMainSizeAuto && !isMainMinSizeAuto) {
    
    
    return;
  }

  
  
  
  
  
  
  
  
  
  const nsHTMLReflowState* flexContainerRS = aItemReflowState.parentReflowState;
  MOZ_ASSERT(flexContainerRS,
             "flex item's reflow state should have ptr to container's state");
  if (NS_STYLE_FLEX_WRAP_NOWRAP == flexContainerRS->mStylePosition->mFlexWrap) {
    
    
    nscoord containerCrossSize =
      GET_CROSS_COMPONENT(aAxisTracker,
                          flexContainerRS->ComputedWidth(),
                          flexContainerRS->ComputedHeight());
    
    
    
    if (aAxisTracker.IsCrossAxisHorizontal() ||
        containerCrossSize != NS_AUTOHEIGHT) {
      aFlexItem.ResolveStretchedCrossSize(containerCrossSize, aAxisTracker);
    }
  }

  
  
  const nsSize ratio = aFlexItem.Frame()->GetIntrinsicRatio();

  nscoord resolvedMinSize; 
  bool minSizeNeedsToMeasureContent = false; 
  if (isMainMinSizeAuto) {
    
    
    resolvedMinSize = PartiallyResolveAutoMinSize(aFlexItem, aItemReflowState,
                                                  ratio, aAxisTracker);
    if (resolvedMinSize > 0 &&
        aAxisTracker.GetCrossComponent(ratio) == 0) {
      
      
      
      
      
      minSizeNeedsToMeasureContent = true;
    }
  }

  bool flexBasisNeedsToMeasureContent = false; 
  if (isMainSizeAuto) {
    if (!ResolveAutoFlexBasisFromRatio(aFlexItem, aItemReflowState,
                                       ratio, aAxisTracker)) {
      flexBasisNeedsToMeasureContent = true;
    }
  }

  
  if (minSizeNeedsToMeasureContent || flexBasisNeedsToMeasureContent) {
    if (aAxisTracker.IsMainAxisHorizontal()) {
      nsRenderingContext rctx(
        aPresContext->PresShell()->CreateReferenceRenderingContext());
      if (minSizeNeedsToMeasureContent) {
        resolvedMinSize = std::min(resolvedMinSize, aFlexItem.Frame()->GetMinISize(&rctx));
      }
      NS_ASSERTION(!flexBasisNeedsToMeasureContent,
                   "flex-basis:auto should have been resolved in the "
                   "reflow state, for horizontal flexbox. It shouldn't need "
                   "special handling here");
    } else {
      
      
      
      
      
      
      
      
      
      
      bool forceVerticalResizeForMeasuringReflow =
        !aFlexItem.IsFrozen() ||         
        !flexBasisNeedsToMeasureContent; 
                                         

      nscoord contentHeight =
        MeasureFlexItemContentHeight(aPresContext, aFlexItem,
                                     forceVerticalResizeForMeasuringReflow,
                                     *flexContainerRS);
      if (minSizeNeedsToMeasureContent) {
        resolvedMinSize = std::min(resolvedMinSize, contentHeight);
      }
      if (flexBasisNeedsToMeasureContent) {
        aFlexItem.SetFlexBaseSizeAndMainSize(contentHeight);
      }
    }
  }

  if (isMainMinSizeAuto) {
    aFlexItem.UpdateMainMinSize(resolvedMinSize);
  }
}

nscoord
nsFlexContainerFrame::
  MeasureFlexItemContentHeight(nsPresContext* aPresContext,
                               FlexItem& aFlexItem,
                               bool aForceVerticalResizeForMeasuringReflow,
                               const nsHTMLReflowState& aParentReflowState)
{
  
  WritingMode wm = aFlexItem.Frame()->GetWritingMode();
  LogicalSize availSize = aParentReflowState.ComputedSize(wm);
  availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
  nsHTMLReflowState
    childRSForMeasuringHeight(aPresContext, aParentReflowState,
                              aFlexItem.Frame(), availSize,
                              -1, -1, nsHTMLReflowState::CALLER_WILL_INIT);
  childRSForMeasuringHeight.mFlags.mIsFlexContainerMeasuringHeight = true;
  childRSForMeasuringHeight.Init(aPresContext);

  if (aFlexItem.IsStretched()) {
    childRSForMeasuringHeight.SetComputedWidth(aFlexItem.GetCrossSize());
    childRSForMeasuringHeight.SetHResize(true);
  }

  if (aForceVerticalResizeForMeasuringReflow) {
    childRSForMeasuringHeight.SetVResize(true);
  }

  nsHTMLReflowMetrics childDesiredSize(childRSForMeasuringHeight);
  nsReflowStatus childReflowStatus;
  const uint32_t flags = NS_FRAME_NO_MOVE_FRAME;
  ReflowChild(aFlexItem.Frame(), aPresContext,
              childDesiredSize, childRSForMeasuringHeight,
              0, 0, flags, childReflowStatus);

  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
             "We gave flex item unconstrained available height, so it "
             "should be complete");

  FinishReflowChild(aFlexItem.Frame(), aPresContext,
                    childDesiredSize, &childRSForMeasuringHeight,
                    0, 0, flags);

  aFlexItem.SetHadMeasuringReflow();

  
  
  
  if (aFlexItem.Frame() == mFrames.FirstChild() ||
      aFlexItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE) {
    aFlexItem.SetAscent(childDesiredSize.BlockStartAscent());
  }

  
  
  nscoord childDesiredHeight = childDesiredSize.Height() -
    childRSForMeasuringHeight.ComputedPhysicalBorderPadding().TopBottom();

  return std::max(0, childDesiredHeight);
}

FlexItem::FlexItem(nsHTMLReflowState& aFlexItemReflowState,
                   float aFlexGrow, float aFlexShrink, nscoord aFlexBaseSize,
                   nscoord aMainMinSize,  nscoord aMainMaxSize,
                   nscoord aTentativeCrossSize,
                   nscoord aCrossMinSize, nscoord aCrossMaxSize,
                   const FlexboxAxisTracker& aAxisTracker)
  : mFrame(aFlexItemReflowState.frame),
    mFlexGrow(aFlexGrow),
    mFlexShrink(aFlexShrink),
    mBorderPadding(aFlexItemReflowState.ComputedPhysicalBorderPadding()),
    mMargin(aFlexItemReflowState.ComputedPhysicalMargin()),
    mMainMinSize(aMainMinSize),
    mMainMaxSize(aMainMaxSize),
    mCrossMinSize(aCrossMinSize),
    mCrossMaxSize(aCrossMaxSize),
    mMainPosn(0),
    mCrossSize(aTentativeCrossSize),
    mCrossPosn(0),
    mAscent(0),
    mShareOfWeightSoFar(0.0f),
    mIsFrozen(false),
    mHadMinViolation(false),
    mHadMaxViolation(false),
    mHadMeasuringReflow(false),
    mIsStretched(false),
    mIsStrut(false),
    
    mAlignSelf(aFlexItemReflowState.mStylePosition->mAlignSelf)
{
  MOZ_ASSERT(mFrame, "expecting a non-null child frame");
  MOZ_ASSERT(mFrame->GetType() != nsGkAtoms::placeholderFrame,
             "placeholder frames should not be treated as flex items");
  MOZ_ASSERT(!(mFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW),
             "out-of-flow frames should not be treated as flex items");

  SetFlexBaseSizeAndMainSize(aFlexBaseSize);
  CheckForMinSizeAuto(aFlexItemReflowState, aAxisTracker);

  
  
#ifdef DEBUG
  {
    const nsStyleSides& styleMargin =
      aFlexItemReflowState.mStyleMargin->mMargin;
    NS_FOR_CSS_SIDES(side) {
      if (styleMargin.GetUnit(side) == eStyleUnit_Auto) {
        MOZ_ASSERT(GetMarginComponentForSide(side) == 0,
                   "Someone else tried to resolve our auto margin");
      }
    }
  }
#endif 

  
  if (mAlignSelf == NS_STYLE_ALIGN_SELF_AUTO) {
    mAlignSelf =
      mFrame->StyleContext()->GetParent()->StylePosition()->mAlignItems;
  }

  
  
  
  
  
  
  
  
  
  
  if (mAlignSelf == NS_STYLE_ALIGN_ITEMS_BASELINE &&
      aAxisTracker.IsCrossAxisHorizontal()) {
    mAlignSelf = NS_STYLE_ALIGN_ITEMS_FLEX_START;
  }
}




FlexItem::FlexItem(nsIFrame* aChildFrame, nscoord aCrossSize)
  : mFrame(aChildFrame),
    mFlexGrow(0.0f),
    mFlexShrink(0.0f),
    
    
    mFlexBaseSize(0),
    mMainMinSize(0),
    mMainMaxSize(0),
    mCrossMinSize(0),
    mCrossMaxSize(0),
    mMainSize(0),
    mMainPosn(0),
    mCrossSize(aCrossSize),
    mCrossPosn(0),
    mAscent(0),
    mShareOfWeightSoFar(0.0f),
    mIsFrozen(true),
    mHadMinViolation(false),
    mHadMaxViolation(false),
    mHadMeasuringReflow(false),
    mIsStretched(false),
    mIsStrut(true), 
    mNeedsMinSizeAutoResolution(false),
    mAlignSelf(NS_STYLE_ALIGN_ITEMS_FLEX_START)
{
  MOZ_ASSERT(mFrame, "expecting a non-null child frame");
  MOZ_ASSERT(NS_STYLE_VISIBILITY_COLLAPSE ==
             mFrame->StyleVisibility()->mVisible,
             "Should only make struts for children with 'visibility:collapse'");
  MOZ_ASSERT(mFrame->GetType() != nsGkAtoms::placeholderFrame,
             "placeholder frames should not be treated as flex items");
  MOZ_ASSERT(!(mFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW),
             "out-of-flow frames should not be treated as flex items");
}

void
FlexItem::CheckForMinSizeAuto(const nsHTMLReflowState& aFlexItemReflowState,
                              const FlexboxAxisTracker& aAxisTracker)
{
  const nsStylePosition* pos = aFlexItemReflowState.mStylePosition;
  const nsStyleDisplay* disp = aFlexItemReflowState.mStyleDisplay;

  
  
  
  
  
  const nsStyleCoord& minSize = GET_MAIN_COMPONENT(aAxisTracker,
                                                   pos->mMinWidth,
                                                   pos->mMinHeight);

  const uint8_t overflowVal = GET_MAIN_COMPONENT(aAxisTracker,
                                                 disp->mOverflowX,
                                                 disp->mOverflowY);

  mNeedsMinSizeAutoResolution = (minSize.GetUnit() == eStyleUnit_Auto &&
                                 overflowVal == NS_STYLE_OVERFLOW_VISIBLE);
}

nscoord
FlexItem::GetBaselineOffsetFromOuterCrossEdge(
  AxisEdgeType aEdge,
  const FlexboxAxisTracker& aAxisTracker) const
{
  
  
  
  
  
  MOZ_ASSERT(!aAxisTracker.IsCrossAxisHorizontal(),
             "Only expecting to be doing baseline computations when the "
             "cross axis is vertical");

  AxisOrientationType crossAxis = aAxisTracker.GetCrossAxis();
  mozilla::Side sideToMeasureFrom = kAxisOrientationToSidesMap[crossAxis][aEdge];

  nscoord marginTopToBaseline = ResolvedAscent() + mMargin.top;

  if (sideToMeasureFrom == eSideTop) {
    
    
    return marginTopToBaseline;
  }

  MOZ_ASSERT(sideToMeasureFrom == eSideBottom,
             "We already checked that we're dealing with a vertical axis, and "
             "we're not using the top side, so that only leaves the bottom...");

  
  
  
  return GetOuterCrossSize(crossAxis) - marginTopToBaseline;
}

uint32_t
FlexItem::GetNumAutoMarginsInAxis(AxisOrientationType aAxis) const
{
  uint32_t numAutoMargins = 0;
  const nsStyleSides& styleMargin = mFrame->StyleMargin()->mMargin;
  for (uint32_t i = 0; i < eNumAxisEdges; i++) {
    mozilla::Side side = kAxisOrientationToSidesMap[aAxis][i];
    if (styleMargin.GetUnit(side) == eStyleUnit_Auto) {
      numAutoMargins++;
    }
  }

  
  MOZ_ASSERT(numAutoMargins <= 2,
             "We're just looking at one item along one dimension, so we "
             "should only have examined 2 margins");

  return numAutoMargins;
}





class MOZ_STACK_CLASS PositionTracker {
public:
  
  inline nscoord GetPosition() const { return mPosition; }
  inline AxisOrientationType GetAxis() const { return mAxis; }

  
  
  void EnterMargin(const nsMargin& aMargin)
  {
    mozilla::Side side = kAxisOrientationToSidesMap[mAxis][eAxisEdge_Start];
    mPosition += aMargin.Side(side);
  }

  
  
  void ExitMargin(const nsMargin& aMargin)
  {
    mozilla::Side side = kAxisOrientationToSidesMap[mAxis][eAxisEdge_End];
    mPosition += aMargin.Side(side);
  }

  
  
  
  void EnterChildFrame(nscoord aChildFrameSize)
  {
    if (!AxisGrowsInPositiveDirection(mAxis))
      mPosition += aChildFrameSize;
  }

  
  
  
  
  void ExitChildFrame(nscoord aChildFrameSize)
  {
    if (AxisGrowsInPositiveDirection(mAxis))
      mPosition += aChildFrameSize;
  }

protected:
  
  explicit PositionTracker(AxisOrientationType aAxis)
    : mPosition(0),
      mAxis(aAxis)
  {}

  
  
  PositionTracker(const PositionTracker&) = delete;
  PositionTracker& operator=(const PositionTracker&) = delete;

  
  nscoord mPosition;               
  const AxisOrientationType mAxis; 
};




class MOZ_STACK_CLASS MainAxisPositionTracker : public PositionTracker {
public:
  MainAxisPositionTracker(const FlexboxAxisTracker& aAxisTracker,
                          const FlexLine* aLine,
                          uint8_t aJustifyContent,
                          nscoord aContentBoxMainSize);

  ~MainAxisPositionTracker() {
    MOZ_ASSERT(mNumPackingSpacesRemaining == 0,
               "miscounted the number of packing spaces");
    MOZ_ASSERT(mNumAutoMarginsInMainAxis == 0,
               "miscounted the number of auto margins");
  }

  
  void TraversePackingSpace();

  
  
  void ResolveAutoMarginsInMainAxis(FlexItem& aItem);

private:
  nscoord  mPackingSpaceRemaining;
  uint32_t mNumAutoMarginsInMainAxis;
  uint32_t mNumPackingSpacesRemaining;
  uint8_t  mJustifyContent;
};





class MOZ_STACK_CLASS CrossAxisPositionTracker : public PositionTracker {
public:
  CrossAxisPositionTracker(FlexLine* aFirstLine,
                           uint8_t aAlignContent,
                           nscoord aContentBoxCrossSize,
                           bool aIsCrossSizeDefinite,
                           const FlexboxAxisTracker& aAxisTracker);

  
  void TraversePackingSpace();

  
  void TraverseLine(FlexLine& aLine) { mPosition += aLine.GetLineCrossSize(); }

private:
  
  
  
  
  void EnterMargin(const nsMargin& aMargin) = delete;
  void ExitMargin(const nsMargin& aMargin) = delete;
  void EnterChildFrame(nscoord aChildFrameSize) = delete;
  void ExitChildFrame(nscoord aChildFrameSize) = delete;

  nscoord  mPackingSpaceRemaining;
  uint32_t mNumPackingSpacesRemaining;
  uint8_t  mAlignContent;
};



class MOZ_STACK_CLASS SingleLineCrossAxisPositionTracker : public PositionTracker {
public:
  explicit SingleLineCrossAxisPositionTracker(const FlexboxAxisTracker& aAxisTracker);

  void ResolveAutoMarginsInCrossAxis(const FlexLine& aLine,
                                     FlexItem& aItem);

  void EnterAlignPackingSpace(const FlexLine& aLine,
                              const FlexItem& aItem,
                              const FlexboxAxisTracker& aAxisTracker);

  
  inline void ResetPosition() { mPosition = 0; }
};






NS_QUERYFRAME_HEAD(nsFlexContainerFrame)
  NS_QUERYFRAME_ENTRY(nsFlexContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFlexContainerFrameSuper)

NS_IMPL_FRAMEARENA_HELPERS(nsFlexContainerFrame)

nsContainerFrame*
NS_NewFlexContainerFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext)
{
  return new (aPresShell) nsFlexContainerFrame(aContext);
}







nsFlexContainerFrame::~nsFlexContainerFrame()
{
}

template<bool IsLessThanOrEqual(nsIFrame*, nsIFrame*)>
 bool
nsFlexContainerFrame::SortChildrenIfNeeded()
{
  if (nsIFrame::IsFrameListSorted<IsLessThanOrEqual>(mFrames)) {
    return false;
  }

  nsIFrame::SortFrameList<IsLessThanOrEqual>(mFrames);
  return true;
}


nsIAtom*
nsFlexContainerFrame::GetType() const
{
  return nsGkAtoms::flexContainerFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsFlexContainerFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("FlexContainer"), aResult);
}
#endif







uint32_t
GetDisplayFlagsForFlexItem(nsIFrame* aFrame)
{
  MOZ_ASSERT(aFrame->IsFlexItem(), "Should only be called on flex items");

  const nsStylePosition* pos = aFrame->StylePosition();
  if (pos->mZIndex.GetUnit() == eStyleUnit_Integer) {
    return nsIFrame::DISPLAY_CHILD_FORCE_STACKING_CONTEXT;
  }
  return nsIFrame::DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT;
}

void
nsFlexContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  
  
  
  
  NS_ASSERTION(
    (!mFrames.IsEmpty() &&
     mFrames.FirstChild()->GetContent()->GetContainingShadow()) ||
    nsIFrame::IsFrameListSorted<IsOrderLEQWithDOMFallback>(mFrames),
    "Child frames aren't sorted correctly");

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  
  
  nsDisplayListSet childLists(aLists, aLists.BlockBorderBackgrounds());
  for (nsIFrame* childFrame : mFrames) {
    BuildDisplayListForChild(aBuilder, childFrame, aDirtyRect, childLists,
                             GetDisplayFlagsForFlexItem(childFrame));
  }
}

#ifdef DEBUG

bool
FrameWantsToBeInAnonymousFlexItem(nsIFrame* aFrame)
{
  
  
  return (aFrame->IsFrameOfType(nsIFrame::eLineParticipant) ||
          nsGkAtoms::placeholderFrame == aFrame->GetType());
}












void
nsFlexContainerFrame::SanityCheckAnonymousFlexItems() const
{
  bool prevChildWasAnonFlexItem = false;
  for (nsIFrame* child : mFrames) {
    MOZ_ASSERT(!FrameWantsToBeInAnonymousFlexItem(child),
               "frame wants to be inside an anonymous flex item, "
               "but it isn't");
    if (child->StyleContext()->GetPseudo() ==
        nsCSSAnonBoxes::anonymousFlexItem) {
      MOZ_ASSERT(!prevChildWasAnonFlexItem ||
                 HasAnyStateBits(NS_STATE_FLEX_CHILDREN_REORDERED),
                 "two anon flex items in a row (shouldn't happen, unless our "
                 "children have been reordered with the 'order' property)");

      nsIFrame* firstWrappedChild = child->GetFirstPrincipalChild();
      MOZ_ASSERT(firstWrappedChild,
                 "anonymous flex item is empty (shouldn't happen)");
      prevChildWasAnonFlexItem = true;
    } else {
      prevChildWasAnonFlexItem = false;
    }
  }
}
#endif 

void
FlexLine::FreezeItemsEarly(bool aIsUsingFlexGrow)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  uint32_t numUnfrozenItemsToBeSeen = mNumItems - mNumFrozenItems;
  for (FlexItem* item = mItems.getFirst();
       numUnfrozenItemsToBeSeen > 0; item = item->getNext()) {
    MOZ_ASSERT(item, "numUnfrozenItemsToBeSeen says items remain to be seen");

    if (!item->IsFrozen()) {
      numUnfrozenItemsToBeSeen--;
      bool shouldFreeze = (0.0f == item->GetFlexFactor(aIsUsingFlexGrow));
      if (!shouldFreeze) {
        if (aIsUsingFlexGrow) {
          if (item->GetFlexBaseSize() > item->GetMainSize()) {
            shouldFreeze = true;
          }
        } else { 
          if (item->GetFlexBaseSize() < item->GetMainSize()) {
            shouldFreeze = true;
          }
        }
      }
      if (shouldFreeze) {
        
        item->Freeze();
        mNumFrozenItems++;
      }
    }
  }
}



void
FlexLine::FreezeOrRestoreEachFlexibleSize(const nscoord aTotalViolation,
                                          bool aIsFinalIteration)
{
  enum FreezeType {
    eFreezeEverything,
    eFreezeMinViolations,
    eFreezeMaxViolations
  };

  FreezeType freezeType;
  if (aTotalViolation == 0) {
    freezeType = eFreezeEverything;
  } else if (aTotalViolation > 0) {
    freezeType = eFreezeMinViolations;
  } else { 
    freezeType = eFreezeMaxViolations;
  }

  
  
  uint32_t numUnfrozenItemsToBeSeen = mNumItems - mNumFrozenItems;
  for (FlexItem* item = mItems.getFirst();
       numUnfrozenItemsToBeSeen > 0; item = item->getNext()) {
    MOZ_ASSERT(item, "numUnfrozenItemsToBeSeen says items remain to be seen");
    if (!item->IsFrozen()) {
      numUnfrozenItemsToBeSeen--;

      MOZ_ASSERT(!item->HadMinViolation() || !item->HadMaxViolation(),
                 "Can have either min or max violation, but not both");

      if (eFreezeEverything == freezeType ||
          (eFreezeMinViolations == freezeType && item->HadMinViolation()) ||
          (eFreezeMaxViolations == freezeType && item->HadMaxViolation())) {

        MOZ_ASSERT(item->GetMainSize() >= item->GetMainMinSize(),
                   "Freezing item at a size below its minimum");
        MOZ_ASSERT(item->GetMainSize() <= item->GetMainMaxSize(),
                   "Freezing item at a size above its maximum");

        item->Freeze();
        mNumFrozenItems++;
      } else if (MOZ_UNLIKELY(aIsFinalIteration)) {
        
        
        NS_ERROR("Final iteration still has unfrozen items, this shouldn't"
                 " happen unless there was nscoord under/overflow.");
        item->Freeze();
        mNumFrozenItems++;
      } 
        

      
      item->ClearViolationFlags();
    }
  }
}

void
FlexLine::ResolveFlexibleLengths(nscoord aFlexContainerMainSize)
{
  PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG, ("ResolveFlexibleLengths\n"));

  
  const bool isUsingFlexGrow =
    (mTotalOuterHypotheticalMainSize < aFlexContainerMainSize);

  
  
  FreezeItemsEarly(isUsingFlexGrow);

  if (mNumFrozenItems == mNumItems) {
    
    return;
  }
  MOZ_ASSERT(!IsEmpty(), "empty lines should take the early-return above");

  
  
  
  nscoord spaceReservedForMarginBorderPadding =
    mTotalOuterHypotheticalMainSize - mTotalInnerHypotheticalMainSize;

  nscoord spaceAvailableForFlexItemsContentBoxes =
    aFlexContainerMainSize - spaceReservedForMarginBorderPadding;

  nscoord origAvailableFreeSpace;
  bool isOrigAvailFreeSpaceInitialized = false;

  
  
  
  
  
  
  for (uint32_t iterationCounter = 0;
       iterationCounter < mNumItems; iterationCounter++) {
    
    
    
    
    nscoord availableFreeSpace = spaceAvailableForFlexItemsContentBoxes;
    for (FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
      if (!item->IsFrozen()) {
        item->SetMainSize(item->GetFlexBaseSize());
      }
      availableFreeSpace -= item->GetMainSize();
    }

    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" available free space = %d\n", availableFreeSpace));


    
    
    
    
    
    
    NS_ASSERTION((isUsingFlexGrow && availableFreeSpace >= 0) ||
                 (!isUsingFlexGrow && availableFreeSpace <= 0),
                 "availableFreeSpace's sign should match isUsingFlexGrow");

    
    
    if (availableFreeSpace != 0) {
      
      if (!isOrigAvailFreeSpaceInitialized) {
        origAvailableFreeSpace = availableFreeSpace;
        isOrigAvailFreeSpaceInitialized = true;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      float weightSum = 0.0f;
      float flexFactorSum = 0.0f;
      float largestWeight = 0.0f;
      uint32_t numItemsWithLargestWeight = 0;

      
      
      uint32_t numUnfrozenItemsToBeSeen = mNumItems - mNumFrozenItems;
      for (FlexItem* item = mItems.getFirst();
           numUnfrozenItemsToBeSeen > 0; item = item->getNext()) {
        MOZ_ASSERT(item,
                   "numUnfrozenItemsToBeSeen says items remain to be seen");
        if (!item->IsFrozen()) {
          numUnfrozenItemsToBeSeen--;

          float curWeight = item->GetWeight(isUsingFlexGrow);
          float curFlexFactor = item->GetFlexFactor(isUsingFlexGrow);
          MOZ_ASSERT(curWeight >= 0.0f, "weights are non-negative");
          MOZ_ASSERT(curFlexFactor >= 0.0f, "flex factors are non-negative");

          weightSum += curWeight;
          flexFactorSum += curFlexFactor;

          if (IsFinite(weightSum)) {
            if (curWeight == 0.0f) {
              item->SetShareOfWeightSoFar(0.0f);
            } else {
              item->SetShareOfWeightSoFar(curWeight / weightSum);
            }
          } 
            
            
            

          
          if (curWeight > largestWeight) {
            largestWeight = curWeight;
            numItemsWithLargestWeight = 1;
          } else if (curWeight == largestWeight) {
            numItemsWithLargestWeight++;
          }
        }
      }

      if (weightSum != 0.0f) {
        MOZ_ASSERT(flexFactorSum != 0.0f,
                   "flex factor sum can't be 0, if a weighted sum "
                   "of its components (weightSum) is nonzero");
        if (flexFactorSum < 1.0f) {
          
          
          
          
          nscoord totalDesiredPortionOfOrigFreeSpace =
            NSToCoordRound(origAvailableFreeSpace * flexFactorSum);

          
          
          
          
          MOZ_ASSERT(totalDesiredPortionOfOrigFreeSpace == 0 ||
                     ((totalDesiredPortionOfOrigFreeSpace > 0) ==
                      (availableFreeSpace > 0)),
                     "When we reduce available free space for flex factors < 1,"
                     "we shouldn't change the sign of the free space...");

          if (availableFreeSpace > 0) {
            availableFreeSpace = std::min(availableFreeSpace,
                                          totalDesiredPortionOfOrigFreeSpace);
          } else {
            availableFreeSpace = std::max(availableFreeSpace,
                                          totalDesiredPortionOfOrigFreeSpace);
          }
        }

        PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
               (" Distributing available space:"));
        
        
        numUnfrozenItemsToBeSeen = mNumItems - mNumFrozenItems;

        
        
        
        for (FlexItem* item = mItems.getLast();
             numUnfrozenItemsToBeSeen > 0; item = item->getPrevious()) {
          MOZ_ASSERT(item,
                     "numUnfrozenItemsToBeSeen says items remain to be seen");
          if (!item->IsFrozen()) {
            numUnfrozenItemsToBeSeen--;

            
            
            nscoord sizeDelta = 0;
            if (IsFinite(weightSum)) {
              float myShareOfRemainingSpace =
                item->GetShareOfWeightSoFar();

              MOZ_ASSERT(myShareOfRemainingSpace >= 0.0f &&
                         myShareOfRemainingSpace <= 1.0f,
                         "my share should be nonnegative fractional amount");

              if (myShareOfRemainingSpace == 1.0f) {
                
                
                sizeDelta = availableFreeSpace;
              } else if (myShareOfRemainingSpace > 0.0f) {
                sizeDelta = NSToCoordRound(availableFreeSpace *
                                           myShareOfRemainingSpace);
              }
            } else if (item->GetWeight(isUsingFlexGrow) == largestWeight) {
              
              
              
              sizeDelta =
                NSToCoordRound(availableFreeSpace /
                               float(numItemsWithLargestWeight));
              numItemsWithLargestWeight--;
            }

            availableFreeSpace -= sizeDelta;

            item->SetMainSize(item->GetMainSize() + sizeDelta);
            PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
                   ("  child %p receives %d, for a total of %d\n",
                    item, sizeDelta, item->GetMainSize()));
          }
        }
      }
    }

    
    nscoord totalViolation = 0; 
    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" Checking for violations:"));

    
    
    uint32_t numUnfrozenItemsToBeSeen = mNumItems - mNumFrozenItems;
    for (FlexItem* item = mItems.getFirst();
         numUnfrozenItemsToBeSeen > 0; item = item->getNext()) {
      MOZ_ASSERT(item, "numUnfrozenItemsToBeSeen says items remain to be seen");
      if (!item->IsFrozen()) {
        numUnfrozenItemsToBeSeen--;

        if (item->GetMainSize() < item->GetMainMinSize()) {
          
          totalViolation += item->GetMainMinSize() - item->GetMainSize();
          item->SetMainSize(item->GetMainMinSize());
          item->SetHadMinViolation();
        } else if (item->GetMainSize() > item->GetMainMaxSize()) {
          
          totalViolation += item->GetMainMaxSize() - item->GetMainSize();
          item->SetMainSize(item->GetMainMaxSize());
          item->SetHadMaxViolation();
        }
      }
    }

    FreezeOrRestoreEachFlexibleSize(totalViolation,
                                    iterationCounter + 1 == mNumItems);

    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" Total violation: %d\n", totalViolation));

    if (mNumFrozenItems == mNumItems) {
      break;
    }

    MOZ_ASSERT(totalViolation != 0,
               "Zero violation should've made us freeze all items & break");
  }

#ifdef DEBUG
  
  
  MOZ_ASSERT(mNumFrozenItems == mNumItems, "All items should be frozen");

  
  for (const FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
    MOZ_ASSERT(item->IsFrozen(), "All items should be frozen");
  }
#endif 
}

MainAxisPositionTracker::
  MainAxisPositionTracker(const FlexboxAxisTracker& aAxisTracker,
                          const FlexLine* aLine,
                          uint8_t aJustifyContent,
                          nscoord aContentBoxMainSize)
  : PositionTracker(aAxisTracker.GetMainAxis()),
    mPackingSpaceRemaining(aContentBoxMainSize), 
    mNumAutoMarginsInMainAxis(0),
    mNumPackingSpacesRemaining(0),
    mJustifyContent(aJustifyContent)
{
  
  
  
  for (const FlexItem* item = aLine->GetFirstItem(); item;
       item = item->getNext()) {
    mPackingSpaceRemaining -= item->GetOuterMainSize(mAxis);
    mNumAutoMarginsInMainAxis += item->GetNumAutoMarginsInAxis(mAxis);
  }

  if (mPackingSpaceRemaining <= 0) {
    
    mNumAutoMarginsInMainAxis = 0;
  }

  
  
  
  if (mPackingSpaceRemaining < 0) {
    if (mJustifyContent == NS_STYLE_JUSTIFY_CONTENT_SPACE_BETWEEN) {
      mJustifyContent = NS_STYLE_JUSTIFY_CONTENT_FLEX_START;
    } else if (mJustifyContent == NS_STYLE_JUSTIFY_CONTENT_SPACE_AROUND) {
      mJustifyContent = NS_STYLE_JUSTIFY_CONTENT_CENTER;
    }
  }

  
  
  if (aAxisTracker.AreAxesInternallyReversed()) {
    if (mJustifyContent == NS_STYLE_JUSTIFY_CONTENT_FLEX_START) {
      mJustifyContent = NS_STYLE_JUSTIFY_CONTENT_FLEX_END;
    } else if (mJustifyContent == NS_STYLE_JUSTIFY_CONTENT_FLEX_END) {
      mJustifyContent = NS_STYLE_JUSTIFY_CONTENT_FLEX_START;
    }
  }

  
  
  if (mNumAutoMarginsInMainAxis == 0 &&
      mPackingSpaceRemaining != 0 &&
      !aLine->IsEmpty()) {
    switch (mJustifyContent) {
      case NS_STYLE_JUSTIFY_CONTENT_FLEX_START:
        
        break;
      case NS_STYLE_JUSTIFY_CONTENT_FLEX_END:
        
        mPosition += mPackingSpaceRemaining;
        break;
      case NS_STYLE_JUSTIFY_CONTENT_CENTER:
        
        mPosition += mPackingSpaceRemaining / 2;
        break;
      case NS_STYLE_JUSTIFY_CONTENT_SPACE_BETWEEN:
        MOZ_ASSERT(mPackingSpaceRemaining >= 0,
                   "negative packing space should make us use 'flex-start' "
                   "instead of 'space-between'");
        
        mNumPackingSpacesRemaining = aLine->NumItems() - 1;
        break;
      case NS_STYLE_JUSTIFY_CONTENT_SPACE_AROUND:
        MOZ_ASSERT(mPackingSpaceRemaining >= 0,
                   "negative packing space should make us use 'center' "
                   "instead of 'space-around'");
        
        
        
        mNumPackingSpacesRemaining = aLine->NumItems();
        if (mNumPackingSpacesRemaining > 0) {
          
          nscoord totalEdgePackingSpace =
            mPackingSpaceRemaining / mNumPackingSpacesRemaining;

          
          mPosition += totalEdgePackingSpace / 2;
          
          
          mPackingSpaceRemaining -= totalEdgePackingSpace;
          mNumPackingSpacesRemaining--;
        }
        break;
      default:
        MOZ_CRASH("Unexpected justify-content value");
    }
  }

  MOZ_ASSERT(mNumPackingSpacesRemaining == 0 ||
             mNumAutoMarginsInMainAxis == 0,
             "extra space should either go to packing space or to "
             "auto margins, but not to both");
}

void
MainAxisPositionTracker::ResolveAutoMarginsInMainAxis(FlexItem& aItem)
{
  if (mNumAutoMarginsInMainAxis) {
    const nsStyleSides& styleMargin = aItem.Frame()->StyleMargin()->mMargin;
    for (uint32_t i = 0; i < eNumAxisEdges; i++) {
      mozilla::Side side = kAxisOrientationToSidesMap[mAxis][i];
      if (styleMargin.GetUnit(side) == eStyleUnit_Auto) {
        
        
        nscoord curAutoMarginSize =
          mPackingSpaceRemaining / mNumAutoMarginsInMainAxis;

        MOZ_ASSERT(aItem.GetMarginComponentForSide(side) == 0,
                   "Expecting auto margins to have value '0' before we "
                   "resolve them");
        aItem.SetMarginComponentForSide(side, curAutoMarginSize);

        mNumAutoMarginsInMainAxis--;
        mPackingSpaceRemaining -= curAutoMarginSize;
      }
    }
  }
}

void
MainAxisPositionTracker::TraversePackingSpace()
{
  if (mNumPackingSpacesRemaining) {
    MOZ_ASSERT(mJustifyContent == NS_STYLE_JUSTIFY_CONTENT_SPACE_BETWEEN ||
               mJustifyContent == NS_STYLE_JUSTIFY_CONTENT_SPACE_AROUND,
               "mNumPackingSpacesRemaining only applies for "
               "space-between/space-around");

    MOZ_ASSERT(mPackingSpaceRemaining >= 0,
               "ran out of packing space earlier than we expected");

    
    
    nscoord curPackingSpace =
      mPackingSpaceRemaining / mNumPackingSpacesRemaining;

    mPosition += curPackingSpace;
    mNumPackingSpacesRemaining--;
    mPackingSpaceRemaining -= curPackingSpace;
  }
}

CrossAxisPositionTracker::
  CrossAxisPositionTracker(FlexLine* aFirstLine,
                           uint8_t aAlignContent,
                           nscoord aContentBoxCrossSize,
                           bool aIsCrossSizeDefinite,
                           const FlexboxAxisTracker& aAxisTracker)
  : PositionTracker(aAxisTracker.GetCrossAxis()),
    mPackingSpaceRemaining(0),
    mNumPackingSpacesRemaining(0),
    mAlignContent(aAlignContent)
{
  MOZ_ASSERT(aFirstLine, "null first line pointer");

  if (aIsCrossSizeDefinite && !aFirstLine->getNext()) {
    
    
    
    
    
    
    
    
    aFirstLine->SetLineCrossSize(aContentBoxCrossSize);
    return;
  }

  
  
  
  

  
  
  
  mPackingSpaceRemaining = aContentBoxCrossSize;
  uint32_t numLines = 0;
  for (FlexLine* line = aFirstLine; line; line = line->getNext()) {
    mPackingSpaceRemaining -= line->GetLineCrossSize();
    numLines++;
  }

  
  
  
  
  if (mPackingSpaceRemaining < 0) {
    if (mAlignContent == NS_STYLE_ALIGN_CONTENT_SPACE_BETWEEN ||
        mAlignContent == NS_STYLE_ALIGN_CONTENT_STRETCH) {
      mAlignContent = NS_STYLE_ALIGN_CONTENT_FLEX_START;
    } else if (mAlignContent == NS_STYLE_ALIGN_CONTENT_SPACE_AROUND) {
      mAlignContent = NS_STYLE_ALIGN_CONTENT_CENTER;
    }
  }

  
  
  if (aAxisTracker.AreAxesInternallyReversed()) {
    if (mAlignContent == NS_STYLE_ALIGN_CONTENT_FLEX_START) {
      mAlignContent = NS_STYLE_ALIGN_CONTENT_FLEX_END;
    } else if (mAlignContent == NS_STYLE_ALIGN_CONTENT_FLEX_END) {
      mAlignContent = NS_STYLE_ALIGN_CONTENT_FLEX_START;
    }
  }

  
  
  if (mPackingSpaceRemaining != 0) {
    switch (mAlignContent) {
      case NS_STYLE_ALIGN_CONTENT_FLEX_START:
        
        break;
      case NS_STYLE_ALIGN_CONTENT_FLEX_END:
        
        mPosition += mPackingSpaceRemaining;
        break;
      case NS_STYLE_ALIGN_CONTENT_CENTER:
        
        mPosition += mPackingSpaceRemaining / 2;
        break;
      case NS_STYLE_ALIGN_CONTENT_SPACE_BETWEEN:
        MOZ_ASSERT(mPackingSpaceRemaining >= 0,
                   "negative packing space should make us use 'flex-start' "
                   "instead of 'space-between'");
        
        mNumPackingSpacesRemaining = numLines - 1;
        break;
      case NS_STYLE_ALIGN_CONTENT_SPACE_AROUND: {
        MOZ_ASSERT(mPackingSpaceRemaining >= 0,
                   "negative packing space should make us use 'center' "
                   "instead of 'space-around'");
        
        
        
        mNumPackingSpacesRemaining = numLines;
        
        nscoord totalEdgePackingSpace =
          mPackingSpaceRemaining / mNumPackingSpacesRemaining;

        
        mPosition += totalEdgePackingSpace / 2;
        
        
        mPackingSpaceRemaining -= totalEdgePackingSpace;
        mNumPackingSpacesRemaining--;
        break;
      }
      case NS_STYLE_ALIGN_CONTENT_STRETCH: {
        
        MOZ_ASSERT(mPackingSpaceRemaining > 0,
                   "negative packing space should make us use 'flex-start' "
                   "instead of 'stretch' (and we shouldn't bother with this "
                   "code if we have 0 packing space)");

        uint32_t numLinesLeft = numLines;
        for (FlexLine* line = aFirstLine; line; line = line->getNext()) {
          
          
          MOZ_ASSERT(numLinesLeft > 0, "miscalculated num lines");
          nscoord shareOfExtraSpace = mPackingSpaceRemaining / numLinesLeft;
          nscoord newSize = line->GetLineCrossSize() + shareOfExtraSpace;
          line->SetLineCrossSize(newSize);

          mPackingSpaceRemaining -= shareOfExtraSpace;
          numLinesLeft--;
        }
        MOZ_ASSERT(numLinesLeft == 0, "miscalculated num lines");
        break;
      }
      default:
        MOZ_CRASH("Unexpected align-content value");
    }
  }
}

void
CrossAxisPositionTracker::TraversePackingSpace()
{
  if (mNumPackingSpacesRemaining) {
    MOZ_ASSERT(mAlignContent == NS_STYLE_ALIGN_CONTENT_SPACE_BETWEEN ||
               mAlignContent == NS_STYLE_ALIGN_CONTENT_SPACE_AROUND,
               "mNumPackingSpacesRemaining only applies for "
               "space-between/space-around");

    MOZ_ASSERT(mPackingSpaceRemaining >= 0,
               "ran out of packing space earlier than we expected");

    
    
    nscoord curPackingSpace =
      mPackingSpaceRemaining / mNumPackingSpacesRemaining;

    mPosition += curPackingSpace;
    mNumPackingSpacesRemaining--;
    mPackingSpaceRemaining -= curPackingSpace;
  }
}

SingleLineCrossAxisPositionTracker::
  SingleLineCrossAxisPositionTracker(const FlexboxAxisTracker& aAxisTracker)
  : PositionTracker(aAxisTracker.GetCrossAxis())
{
}

void
FlexLine::ComputeCrossSizeAndBaseline(const FlexboxAxisTracker& aAxisTracker)
{
  nscoord crossStartToFurthestBaseline = nscoord_MIN;
  nscoord crossEndToFurthestBaseline = nscoord_MIN;
  nscoord largestOuterCrossSize = 0;
  for (const FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
    nscoord curOuterCrossSize =
      item->GetOuterCrossSize(aAxisTracker.GetCrossAxis());

    if (item->GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE &&
        item->GetNumAutoMarginsInAxis(aAxisTracker.GetCrossAxis()) == 0) {
      
      

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      nscoord crossStartToBaseline =
        item->GetBaselineOffsetFromOuterCrossEdge(eAxisEdge_Start,
                                                  aAxisTracker);
      nscoord crossEndToBaseline = curOuterCrossSize - crossStartToBaseline;

      
      
      
      crossStartToFurthestBaseline = std::max(crossStartToFurthestBaseline,
                                              crossStartToBaseline);
      crossEndToFurthestBaseline = std::max(crossEndToFurthestBaseline,
                                            crossEndToBaseline);
    } else {
      largestOuterCrossSize = std::max(largestOuterCrossSize, curOuterCrossSize);
    }
  }

  
  
  
  
  mBaselineOffset = aAxisTracker.AreAxesInternallyReversed() ?
    crossEndToFurthestBaseline : crossStartToFurthestBaseline;

  
  
  
  
  
  mLineCrossSize = std::max(crossStartToFurthestBaseline +
                            crossEndToFurthestBaseline,
                            largestOuterCrossSize);
}

void
FlexItem::ResolveStretchedCrossSize(nscoord aLineCrossSize,
                                    const FlexboxAxisTracker& aAxisTracker)
{
  AxisOrientationType crossAxis = aAxisTracker.GetCrossAxis();
  
  
  
  if (mAlignSelf != NS_STYLE_ALIGN_ITEMS_STRETCH ||
      GetNumAutoMarginsInAxis(crossAxis) != 0 ||
      eStyleUnit_Auto != aAxisTracker.ComputedCrossSize(mFrame).GetUnit()) {
    return;
  }

  
  
  if (mIsStretched) {
    return;
  }

  
  
  nscoord stretchedSize = aLineCrossSize -
    GetMarginBorderPaddingSizeInAxis(crossAxis);

  stretchedSize = NS_CSS_MINMAX(stretchedSize, mCrossMinSize, mCrossMaxSize);

  
  
  SetCrossSize(stretchedSize);
  mIsStretched = true;
}

void
SingleLineCrossAxisPositionTracker::
  ResolveAutoMarginsInCrossAxis(const FlexLine& aLine,
                                FlexItem& aItem)
{
  
  
  nscoord spaceForAutoMargins = aLine.GetLineCrossSize() -
    aItem.GetOuterCrossSize(mAxis);

  if (spaceForAutoMargins <= 0) {
    return; 
  }

  uint32_t numAutoMargins = aItem.GetNumAutoMarginsInAxis(mAxis);
  if (numAutoMargins == 0) {
    return; 
  }

  
  
  const nsStyleSides& styleMargin = aItem.Frame()->StyleMargin()->mMargin;
  for (uint32_t i = 0; i < eNumAxisEdges; i++) {
    mozilla::Side side = kAxisOrientationToSidesMap[mAxis][i];
    if (styleMargin.GetUnit(side) == eStyleUnit_Auto) {
      MOZ_ASSERT(aItem.GetMarginComponentForSide(side) == 0,
                 "Expecting auto margins to have value '0' before we "
                 "update them");

      
      
      nscoord curAutoMarginSize = spaceForAutoMargins / numAutoMargins;
      aItem.SetMarginComponentForSide(side, curAutoMarginSize);
      numAutoMargins--;
      spaceForAutoMargins -= curAutoMarginSize;
    }
  }
}

void
SingleLineCrossAxisPositionTracker::
  EnterAlignPackingSpace(const FlexLine& aLine,
                         const FlexItem& aItem,
                         const FlexboxAxisTracker& aAxisTracker)
{
  
  
  if (aItem.GetNumAutoMarginsInAxis(mAxis)) {
    return;
  }

  uint8_t alignSelf = aItem.GetAlignSelf();
  
  
  if (alignSelf == NS_STYLE_ALIGN_ITEMS_STRETCH) {
    alignSelf = NS_STYLE_ALIGN_ITEMS_FLEX_START;
  }

  
  
  if (aAxisTracker.AreAxesInternallyReversed()) {
    if (alignSelf == NS_STYLE_ALIGN_ITEMS_FLEX_START) {
      alignSelf = NS_STYLE_ALIGN_ITEMS_FLEX_END;
    } else if (alignSelf == NS_STYLE_ALIGN_ITEMS_FLEX_END) {
      alignSelf = NS_STYLE_ALIGN_ITEMS_FLEX_START;
    }
  }

  switch (alignSelf) {
    case NS_STYLE_ALIGN_ITEMS_FLEX_START:
      
      break;
    case NS_STYLE_ALIGN_ITEMS_FLEX_END:
      mPosition += aLine.GetLineCrossSize() - aItem.GetOuterCrossSize(mAxis);
      break;
    case NS_STYLE_ALIGN_ITEMS_CENTER:
      
      mPosition +=
        (aLine.GetLineCrossSize() - aItem.GetOuterCrossSize(mAxis)) / 2;
      break;
    case NS_STYLE_ALIGN_ITEMS_BASELINE: {
      
      
      
      AxisEdgeType baselineAlignEdge =
        aAxisTracker.AreAxesInternallyReversed() ?
        eAxisEdge_End : eAxisEdge_Start;

      nscoord itemBaselineOffset =
        aItem.GetBaselineOffsetFromOuterCrossEdge(baselineAlignEdge,
                                                  aAxisTracker);

      nscoord lineBaselineOffset = aLine.GetBaselineOffset();

      NS_ASSERTION(lineBaselineOffset >= itemBaselineOffset,
                   "failed at finding largest baseline offset");

      
      
      nscoord baselineDiff = lineBaselineOffset - itemBaselineOffset;

      if (aAxisTracker.AreAxesInternallyReversed()) {
        
        mPosition += aLine.GetLineCrossSize() - aItem.GetOuterCrossSize(mAxis);
        
        mPosition -= baselineDiff;
      } else {
        
        
        mPosition += baselineDiff;
      }
      break;
    }
    default:
      NS_NOTREACHED("Unexpected align-self value");
      break;
  }
}


static inline AxisOrientationType
InlineDirToAxisOrientation(WritingMode::InlineDir aInlineDir)
{
  switch (aInlineDir) {
    case WritingMode::eInlineLTR:
      return eAxis_LR;
    case WritingMode::eInlineRTL:
      return eAxis_RL;
    case WritingMode::eInlineTTB:
      return eAxis_TB;
    case WritingMode::eInlineBTT:
      return eAxis_BT;
  }

  MOZ_ASSERT_UNREACHABLE("Unhandled InlineDir");
  return eAxis_LR; 
}


static inline AxisOrientationType
BlockDirToAxisOrientation(WritingMode::BlockDir aBlockDir)
{
  switch (aBlockDir) {
    case WritingMode::eBlockLR:
      return eAxis_LR;
    case WritingMode::eBlockRL:
      return eAxis_RL;
    case WritingMode::eBlockTB:
      return eAxis_TB;
    
  }

  MOZ_ASSERT_UNREACHABLE("Unhandled BlockDir");
  return eAxis_TB; 
}

FlexboxAxisTracker::FlexboxAxisTracker(const nsStylePosition* aStylePosition,
                                       const WritingMode& aWM)
  : mWM(aWM),
    mAreAxesInternallyReversed(false)
{
  uint32_t flexDirection = aStylePosition->mFlexDirection;

  
  
  
  
  AxisOrientationType inlineDimension =
    InlineDirToAxisOrientation(mWM.GetInlineDir());
  AxisOrientationType blockDimension =
    BlockDirToAxisOrientation(mWM.GetBlockDir());

  
  switch (flexDirection) {
    case NS_STYLE_FLEX_DIRECTION_ROW:
      mMainAxis = inlineDimension;
      mIsRowOriented = true;
      mIsMainAxisReversed = false;
      break;
    case NS_STYLE_FLEX_DIRECTION_ROW_REVERSE:
      mMainAxis = GetReverseAxis(inlineDimension);
      mIsRowOriented = true;
      mIsMainAxisReversed = true;
      break;
    case NS_STYLE_FLEX_DIRECTION_COLUMN:
      mMainAxis = blockDimension;
      mIsRowOriented = false;
      mIsMainAxisReversed = false;
      break;
    case NS_STYLE_FLEX_DIRECTION_COLUMN_REVERSE:
      mMainAxis = GetReverseAxis(blockDimension);
      mIsRowOriented = false;
      mIsMainAxisReversed = true;
      break;
    default:
      MOZ_CRASH("Unexpected computed value for 'flex-flow' property");
  }

  
  
  
  if (flexDirection == NS_STYLE_FLEX_DIRECTION_COLUMN ||
      flexDirection == NS_STYLE_FLEX_DIRECTION_COLUMN_REVERSE) {
    mCrossAxis = inlineDimension;
  } else {
    mCrossAxis = blockDimension;
  }

  
  if (aStylePosition->mFlexWrap == NS_STYLE_FLEX_WRAP_WRAP_REVERSE) {
    mCrossAxis = GetReverseAxis(mCrossAxis);
    mIsCrossAxisReversed = true;
  } else {
    mIsCrossAxisReversed = false;
  }

  
  
  
  
  static bool sPreventBottomToTopChildOrdering = true;

  if (sPreventBottomToTopChildOrdering) {
    
    
    if (eAxis_BT == mMainAxis || eAxis_BT == mCrossAxis) {
      mMainAxis = GetReverseAxis(mMainAxis);
      mCrossAxis = GetReverseAxis(mCrossAxis);
      mAreAxesInternallyReversed = true;
      mIsMainAxisReversed = !mIsMainAxisReversed;
      mIsCrossAxisReversed = !mIsCrossAxisReversed;
    }
  }
}



static FlexLine*
AddNewFlexLineToList(LinkedList<FlexLine>& aLines,
                     bool aShouldInsertAtFront)
{
  FlexLine* newLine = new FlexLine();
  if (aShouldInsertAtFront) {
    aLines.insertFront(newLine);
  } else {
    aLines.insertBack(newLine);
  }
  return newLine;
}

void
nsFlexContainerFrame::GenerateFlexLines(
  nsPresContext* aPresContext,
  const nsHTMLReflowState& aReflowState,
  nscoord aContentBoxMainSize,
  nscoord aAvailableHeightForContent,
  const nsTArray<StrutInfo>& aStruts,
  const FlexboxAxisTracker& aAxisTracker,
  LinkedList<FlexLine>& aLines)
{
  MOZ_ASSERT(aLines.isEmpty(), "Expecting outparam to start out empty");

  const bool isSingleLine =
    NS_STYLE_FLEX_WRAP_NOWRAP == aReflowState.mStylePosition->mFlexWrap;

  
  
  
  
  
  
  const bool shouldInsertAtFront = aAxisTracker.AreAxesInternallyReversed();

  
  
  FlexLine* curLine = AddNewFlexLineToList(aLines, shouldInsertAtFront);

  nscoord wrapThreshold;
  if (isSingleLine) {
    
    wrapThreshold = NS_UNCONSTRAINEDSIZE;
  } else {
    
    wrapThreshold = aContentBoxMainSize;

    
    
    
    if (wrapThreshold == NS_UNCONSTRAINEDSIZE) {
      const nscoord flexContainerMaxMainSize =
        GET_MAIN_COMPONENT(aAxisTracker,
                           aReflowState.ComputedMaxWidth(),
                           aReflowState.ComputedMaxHeight());

      wrapThreshold = flexContainerMaxMainSize;
    }

    
    
    if (!aAxisTracker.IsMainAxisHorizontal() &&
        aAvailableHeightForContent != NS_UNCONSTRAINEDSIZE) {
      wrapThreshold = std::min(wrapThreshold, aAvailableHeightForContent);
    }
  }

  
  
  uint32_t nextStrutIdx = 0;

  
  
  uint32_t itemIdxInContainer = 0;

  for (nsIFrame* childFrame : mFrames) {
    
    if (!isSingleLine && !curLine->IsEmpty() &&
        childFrame->StyleDisplay()->mBreakBefore) {
      curLine = AddNewFlexLineToList(aLines, shouldInsertAtFront);
    }

    nsAutoPtr<FlexItem> item;
    if (nextStrutIdx < aStruts.Length() &&
        aStruts[nextStrutIdx].mItemIdx == itemIdxInContainer) {

      
      item = new FlexItem(childFrame, aStruts[nextStrutIdx].mStrutCrossSize);
      nextStrutIdx++;
    } else {
      item = GenerateFlexItemForChild(aPresContext, childFrame,
                                      aReflowState, aAxisTracker);
    }

    nscoord itemInnerHypotheticalMainSize = item->GetMainSize();
    nscoord itemOuterHypotheticalMainSize =
      item->GetOuterMainSize(aAxisTracker.GetMainAxis());

    
    
    
    if (wrapThreshold != NS_UNCONSTRAINEDSIZE &&
        !curLine->IsEmpty() && 
        wrapThreshold < (curLine->GetTotalOuterHypotheticalMainSize() +
                         itemOuterHypotheticalMainSize)) {
      curLine = AddNewFlexLineToList(aLines, shouldInsertAtFront);
    }

    
    
    curLine->AddItem(item.forget(), shouldInsertAtFront,
                     itemInnerHypotheticalMainSize,
                     itemOuterHypotheticalMainSize);

    
    if (!isSingleLine && childFrame->GetNextSibling() &&
        childFrame->StyleDisplay()->mBreakAfter) {
      curLine = AddNewFlexLineToList(aLines, shouldInsertAtFront);
    }
    itemIdxInContainer++;
  }
}




nscoord
nsFlexContainerFrame::GetMainSizeFromReflowState(
  const nsHTMLReflowState& aReflowState,
  const FlexboxAxisTracker& aAxisTracker)
{
  if (aAxisTracker.IsMainAxisHorizontal()) {
    
    
    return aReflowState.ComputedISize();
  }

  return GetEffectiveComputedBSize(aReflowState);
}



static nscoord
GetLargestLineMainSize(const FlexLine* aFirstLine)
{
  nscoord largestLineOuterSize = 0;
  for (const FlexLine* line = aFirstLine; line; line = line->getNext()) {
    largestLineOuterSize = std::max(largestLineOuterSize,
                                    line->GetTotalOuterHypotheticalMainSize());
  }
  return largestLineOuterSize;
}



static nscoord
ClampFlexContainerMainSize(const nsHTMLReflowState& aReflowState,
                           const FlexboxAxisTracker& aAxisTracker,
                           nscoord aUnclampedMainSize,
                           nscoord aAvailableHeightForContent,
                           const FlexLine* aFirstLine,
                           nsReflowStatus& aStatus)
{
  MOZ_ASSERT(aFirstLine, "null first line pointer");

  if (aAxisTracker.IsMainAxisHorizontal()) {
    
    
    
    return aUnclampedMainSize;
  }

  if (aUnclampedMainSize != NS_INTRINSICSIZE) {
    
    if (aAvailableHeightForContent == NS_UNCONSTRAINEDSIZE ||
        aUnclampedMainSize < aAvailableHeightForContent) {
      
      
      
      
      return aUnclampedMainSize;
    }

    
    
    
    
    
    
    
    NS_FRAME_SET_INCOMPLETE(aStatus);
    nscoord largestLineOuterSize = GetLargestLineMainSize(aFirstLine);

    if (largestLineOuterSize <= aAvailableHeightForContent) {
      return aAvailableHeightForContent;
    }
    return std::min(aUnclampedMainSize, largestLineOuterSize);
  }

  
  
  
  
  nscoord largestLineOuterSize = GetLargestLineMainSize(aFirstLine);
  return NS_CSS_MINMAX(largestLineOuterSize,
                       aReflowState.ComputedMinHeight(),
                       aReflowState.ComputedMaxHeight());
}

nscoord
nsFlexContainerFrame::ComputeCrossSize(const nsHTMLReflowState& aReflowState,
                                       const FlexboxAxisTracker& aAxisTracker,
                                       nscoord aSumLineCrossSizes,
                                       nscoord aAvailableHeightForContent,
                                       bool* aIsDefinite,
                                       nsReflowStatus& aStatus)
{
  MOZ_ASSERT(aIsDefinite, "outparam pointer must be non-null");

  if (aAxisTracker.IsCrossAxisHorizontal()) {
    
    
    *aIsDefinite = true;
    return aReflowState.ComputedISize();
  }

  nscoord effectiveComputedBSize = GetEffectiveComputedBSize(aReflowState);
  if (effectiveComputedBSize != NS_INTRINSICSIZE) {
    
    *aIsDefinite = true;
    if (aAvailableHeightForContent == NS_UNCONSTRAINEDSIZE ||
        effectiveComputedBSize < aAvailableHeightForContent) {
      
      
      
      
      return effectiveComputedBSize;
    }

    
    
    
    
    
    
    
    NS_FRAME_SET_INCOMPLETE(aStatus);
    if (aSumLineCrossSizes <= aAvailableHeightForContent) {
      return aAvailableHeightForContent;
    }
    return std::min(effectiveComputedBSize, aSumLineCrossSizes);
  }

  
  
  
  *aIsDefinite = false;
  return NS_CSS_MINMAX(aSumLineCrossSizes,
                       aReflowState.ComputedMinHeight(),
                       aReflowState.ComputedMaxHeight());
}

void
FlexLine::PositionItemsInMainAxis(uint8_t aJustifyContent,
                                  nscoord aContentBoxMainSize,
                                  const FlexboxAxisTracker& aAxisTracker)
{
  MainAxisPositionTracker mainAxisPosnTracker(aAxisTracker, this,
                                              aJustifyContent,
                                              aContentBoxMainSize);
  for (FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
    nscoord itemMainBorderBoxSize =
      item->GetMainSize() +
      item->GetBorderPaddingSizeInAxis(mainAxisPosnTracker.GetAxis());

    
    mainAxisPosnTracker.ResolveAutoMarginsInMainAxis(*item);

    
    
    mainAxisPosnTracker.EnterMargin(item->GetMargin());
    mainAxisPosnTracker.EnterChildFrame(itemMainBorderBoxSize);

    item->SetMainPosition(mainAxisPosnTracker.GetPosition());

    mainAxisPosnTracker.ExitChildFrame(itemMainBorderBoxSize);
    mainAxisPosnTracker.ExitMargin(item->GetMargin());
    mainAxisPosnTracker.TraversePackingSpace();
  }
}







static nscoord
ComputePhysicalAscentFromFlexRelativeAscent(
  nscoord aFlexRelativeAscent,
  nscoord aContentBoxCrossSize,
  const nsHTMLReflowState& aReflowState,
  const FlexboxAxisTracker& aAxisTracker)
{
  return aReflowState.ComputedPhysicalBorderPadding().top +
    PhysicalCoordFromFlexRelativeCoord(aFlexRelativeAscent,
                                       aContentBoxCrossSize,
                                       aAxisTracker.GetCrossAxis());
}

void
nsFlexContainerFrame::SizeItemInCrossAxis(
  nsPresContext* aPresContext,
  const FlexboxAxisTracker& aAxisTracker,
  nsHTMLReflowState& aChildReflowState,
  FlexItem& aItem)
{
  if (aAxisTracker.IsCrossAxisHorizontal()) {
    
    
    
    
    
    
    MOZ_ASSERT_UNREACHABLE("Caller should use tentative cross size instead "
                           "of calling SizeItemInCrossAxis");
    
    
    aItem.SetCrossSize(aChildReflowState.ComputedWidth());
    return;
  }

  MOZ_ASSERT(!aItem.HadMeasuringReflow(),
             "We shouldn't need more than one measuring reflow");

  if (aItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_STRETCH) {
    
    
    
    
    
    aChildReflowState.SetVResize(true);
  }
  nsHTMLReflowMetrics childDesiredSize(aChildReflowState);
  nsReflowStatus childReflowStatus;
  const uint32_t flags = NS_FRAME_NO_MOVE_FRAME;
  ReflowChild(aItem.Frame(), aPresContext,
              childDesiredSize, aChildReflowState,
              0, 0, flags, childReflowStatus);
  aItem.SetHadMeasuringReflow();

  
  
  
  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
             "We gave flex item unconstrained available height, so it "
             "should be complete");

  
  
  FinishReflowChild(aItem.Frame(), aPresContext,
                    childDesiredSize, &aChildReflowState, 0, 0, flags);

  
  

  
  
  
  
  
  
  nscoord crossAxisBorderPadding = aItem.GetBorderPadding().TopBottom();
  if (childDesiredSize.Height() < crossAxisBorderPadding) {
    
    
    
    
    
    
    NS_WARN_IF_FALSE(!aItem.Frame()->GetType(),
                     "Child should at least request space for border/padding");
    aItem.SetCrossSize(0);
  } else {
    
    aItem.SetCrossSize(childDesiredSize.Height() - crossAxisBorderPadding);
  }

  
  
  
  if (aItem.Frame() == mFrames.FirstChild() ||
      aItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE) {
    aItem.SetAscent(childDesiredSize.BlockStartAscent());
  }
}

void
FlexLine::PositionItemsInCrossAxis(nscoord aLineStartPosition,
                                   const FlexboxAxisTracker& aAxisTracker)
{
  SingleLineCrossAxisPositionTracker lineCrossAxisPosnTracker(aAxisTracker);

  for (FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
    
    
    item->ResolveStretchedCrossSize(mLineCrossSize, aAxisTracker);
    lineCrossAxisPosnTracker.ResolveAutoMarginsInCrossAxis(*this, *item);

    
    nscoord itemCrossBorderBoxSize =
      item->GetCrossSize() +
      item->GetBorderPaddingSizeInAxis(aAxisTracker.GetCrossAxis());
    lineCrossAxisPosnTracker.EnterAlignPackingSpace(*this, *item, aAxisTracker);
    lineCrossAxisPosnTracker.EnterMargin(item->GetMargin());
    lineCrossAxisPosnTracker.EnterChildFrame(itemCrossBorderBoxSize);

    item->SetCrossPosition(aLineStartPosition +
                           lineCrossAxisPosnTracker.GetPosition());

    
    lineCrossAxisPosnTracker.ResetPosition();
  }
}

void
nsFlexContainerFrame::Reflow(nsPresContext*           aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsFlexContainerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
         ("Reflow() for nsFlexContainerFrame %p\n", this));

  if (IsFrameTreeTooDeep(aReflowState, aDesiredSize, aStatus)) {
    return;
  }

  
  
  
  
  
  const nsStylePosition* stylePos = StylePosition();
  if (stylePos->mHeight.HasPercent() ||
      (StyleDisplay()->IsAbsolutelyPositionedStyle() &&
       eStyleUnit_Auto == stylePos->mHeight.GetUnit() &&
       eStyleUnit_Auto != stylePos->mOffset.GetTopUnit() &&
       eStyleUnit_Auto != stylePos->mOffset.GetBottomUnit())) {
    AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
  }

#ifdef DEBUG
  SanityCheckAnonymousFlexItems();
#endif 

  
  
  
  
  
  
  
  
  

  if (!HasAnyStateBits(NS_STATE_FLEX_CHILDREN_REORDERED)) {
    if (SortChildrenIfNeeded<IsOrderLEQ>()) {
      AddStateBits(NS_STATE_FLEX_CHILDREN_REORDERED);
    }
  } else {
    SortChildrenIfNeeded<IsOrderLEQWithDOMFallback>();
  }

  const FlexboxAxisTracker axisTracker(aReflowState.mStylePosition,
                                       aReflowState.GetWritingMode());

  
  
  
  
  nscoord availableHeightForContent = aReflowState.AvailableHeight();
  if (availableHeightForContent != NS_UNCONSTRAINEDSIZE &&
      !GetSkipSides().Top()) {
    availableHeightForContent -= aReflowState.ComputedPhysicalBorderPadding().top;
    
    availableHeightForContent = std::max(availableHeightForContent, 0);
  }

  nscoord contentBoxMainSize = GetMainSizeFromReflowState(aReflowState,
                                                          axisTracker);

  nsAutoTArray<StrutInfo, 1> struts;
  DoFlexLayout(aPresContext, aDesiredSize, aReflowState, aStatus,
               contentBoxMainSize, availableHeightForContent,
               struts, axisTracker);

  if (!struts.IsEmpty()) {
    
    DoFlexLayout(aPresContext, aDesiredSize, aReflowState, aStatus,
                 contentBoxMainSize, availableHeightForContent,
                 struts, axisTracker);
  }
}




class MOZ_STACK_CLASS AutoFlexLineListClearer
{
public:
  explicit AutoFlexLineListClearer(LinkedList<FlexLine>& aLines
                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
  : mLines(aLines)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  }

  ~AutoFlexLineListClearer()
  {
    while (FlexLine* line = mLines.popFirst()) {
      while (FlexItem* item = line->mItems.popFirst()) {
        delete item;
      }
      delete line;
    }
  }

private:
  LinkedList<FlexLine>& mLines;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

void
nsFlexContainerFrame::DoFlexLayout(nsPresContext*           aPresContext,
                                   nsHTMLReflowMetrics&     aDesiredSize,
                                   const nsHTMLReflowState& aReflowState,
                                   nsReflowStatus&          aStatus,
                                   nscoord aContentBoxMainSize,
                                   nscoord aAvailableHeightForContent,
                                   nsTArray<StrutInfo>& aStruts,
                                   const FlexboxAxisTracker& aAxisTracker)
{
  aStatus = NS_FRAME_COMPLETE;

  LinkedList<FlexLine> lines;
  AutoFlexLineListClearer cleanupLines(lines);

  GenerateFlexLines(aPresContext, aReflowState,
                    aContentBoxMainSize,
                    aAvailableHeightForContent,
                    aStruts, aAxisTracker, lines);

  aContentBoxMainSize =
    ClampFlexContainerMainSize(aReflowState, aAxisTracker,
                               aContentBoxMainSize, aAvailableHeightForContent,
                               lines.getFirst(), aStatus);

  for (FlexLine* line = lines.getFirst(); line; line = line->getNext()) {
    line->ResolveFlexibleLengths(aContentBoxMainSize);
  }

  
  
  
  nscoord sumLineCrossSizes = 0;
  for (FlexLine* line = lines.getFirst(); line; line = line->getNext()) {
    for (FlexItem* item = line->GetFirstItem(); item; item = item->getNext()) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (!item->IsStretched() && 
          !item->IsStrut() &&     
          !aAxisTracker.IsCrossAxisHorizontal()) { 
        WritingMode wm = item->Frame()->GetWritingMode();
        LogicalSize availSize = aReflowState.ComputedSize(wm);
        availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
        nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                           item->Frame(), availSize);
        
        if (aAxisTracker.IsMainAxisHorizontal()) {
          childReflowState.SetComputedWidth(item->GetMainSize());
        } else {
          
          
          
          
          childReflowState.SetComputedHeight(item->GetMainSize());
        }
        
        SizeItemInCrossAxis(aPresContext, aAxisTracker,
                            childReflowState, *item);
      }
    }
    
    line->ComputeCrossSizeAndBaseline(aAxisTracker);
    sumLineCrossSizes += line->GetLineCrossSize();
  }

  bool isCrossSizeDefinite;
  const nscoord contentBoxCrossSize =
    ComputeCrossSize(aReflowState, aAxisTracker, sumLineCrossSizes,
                     aAvailableHeightForContent, &isCrossSizeDefinite, aStatus);

  
  
  CrossAxisPositionTracker
    crossAxisPosnTracker(lines.getFirst(),
                         aReflowState.mStylePosition->mAlignContent,
                         contentBoxCrossSize, isCrossSizeDefinite,
                         aAxisTracker);

  
  
  
  
  if (aStruts.IsEmpty()) { 
    BuildStrutInfoFromCollapsedItems(lines.getFirst(), aStruts);
    if (!aStruts.IsEmpty()) {
      
      return;
    }
  }

  
  
  
  
  nscoord flexContainerAscent;
  if (!aAxisTracker.AreAxesInternallyReversed()) {
    nscoord firstLineBaselineOffset = lines.getFirst()->GetBaselineOffset();
    if (firstLineBaselineOffset == nscoord_MIN) {
      
      
      flexContainerAscent = nscoord_MIN;
    } else  {
      flexContainerAscent =
        ComputePhysicalAscentFromFlexRelativeAscent(
          crossAxisPosnTracker.GetPosition() + firstLineBaselineOffset,
          contentBoxCrossSize, aReflowState, aAxisTracker);
    }
  }

  for (FlexLine* line = lines.getFirst(); line; line = line->getNext()) {

    
    
    line->PositionItemsInMainAxis(aReflowState.mStylePosition->mJustifyContent,
                                  aContentBoxMainSize,
                                  aAxisTracker);

    
    
    line->PositionItemsInCrossAxis(crossAxisPosnTracker.GetPosition(),
                                   aAxisTracker);
    crossAxisPosnTracker.TraverseLine(*line);
    crossAxisPosnTracker.TraversePackingSpace();
  }

  
  
  
  
  if (aAxisTracker.AreAxesInternallyReversed()) {
    nscoord lastLineBaselineOffset = lines.getLast()->GetBaselineOffset();
    if (lastLineBaselineOffset == nscoord_MIN) {
      
      
      flexContainerAscent = nscoord_MIN;
    } else {
      flexContainerAscent =
        ComputePhysicalAscentFromFlexRelativeAscent(
          crossAxisPosnTracker.GetPosition() - lastLineBaselineOffset,
          contentBoxCrossSize, aReflowState, aAxisTracker);
    }
  }

  
  
  
  nsMargin containerBorderPadding(aReflowState.ComputedPhysicalBorderPadding());
  containerBorderPadding.ApplySkipSides(GetSkipSides(&aReflowState));
  const nsPoint containerContentBoxOrigin(containerBorderPadding.left,
                                          containerBorderPadding.top);
  WritingMode outerWM = aReflowState.GetWritingMode();
  nscoord containerWidth = aAxisTracker.IsMainAxisHorizontal() ?
                             aContentBoxMainSize : contentBoxCrossSize;

  
  
  for (const FlexLine* line = lines.getFirst(); line; line = line->getNext()) {
    for (const FlexItem* item = line->GetFirstItem(); item;
         item = item->getNext()) {
      nsPoint physicalPosn = aAxisTracker.PhysicalPointFromFlexRelativePoint(
                               item->GetMainPosition(),
                               item->GetCrossPosition(),
                               aContentBoxMainSize,
                               contentBoxCrossSize);
      
      
      physicalPosn += containerContentBoxOrigin;

      
      
      nsSize finalFlexedPhysicalSize =
        aAxisTracker.PhysicalSizeFromFlexRelativeSizes(item->GetMainSize(),
                                                       item->GetCrossSize());
      LogicalPoint framePos(outerWM, physicalPosn,
                            containerWidth - finalFlexedPhysicalSize.width -
                              item->GetBorderPadding().LeftRight());

      
      
      const nscoord itemNormalBPos = framePos.B(outerWM);

      
      
      bool itemNeedsReflow = true; 
      if (item->HadMeasuringReflow()) {
        
        
        if (finalFlexedPhysicalSize ==
            item->Frame()->GetContentRectRelativeToSelf().Size()) {
          
          
          
          
          if (!(item->Frame()->GetStateBits() &
                NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
            
            
            
            itemNeedsReflow = false;
            MoveFlexItemToFinalPosition(aReflowState, *item, framePos,
                                        containerWidth);
          }
        }
      }
      if (itemNeedsReflow) {
        ReflowFlexItem(aPresContext, aAxisTracker, aReflowState,
                       *item, framePos, containerWidth);
      }

      
      
      
      if (item->Frame() == mFrames.FirstChild() &&
          flexContainerAscent == nscoord_MIN) {
        flexContainerAscent = itemNormalBPos + item->ResolvedAscent();
      }
    }
  }

  nsSize desiredContentBoxSize =
    aAxisTracker.PhysicalSizeFromFlexRelativeSizes(aContentBoxMainSize,
                                                   contentBoxCrossSize);

  aDesiredSize.Width() = desiredContentBoxSize.width +
    containerBorderPadding.LeftRight();
  
  aDesiredSize.Height() = desiredContentBoxSize.height +
    containerBorderPadding.top;

  if (flexContainerAscent == nscoord_MIN) {
    
    
    
    
    NS_WARN_IF_FALSE(lines.getFirst()->IsEmpty(),
                     "Have flex items but didn't get an ascent - that's odd "
                     "(or there are just gigantic sizes involved)");
    
    flexContainerAscent = aDesiredSize.Height();
  }
  aDesiredSize.SetBlockStartAscent(flexContainerAscent);

  
  
  
  
  
  
  
  if (NS_FRAME_IS_COMPLETE(aStatus)) {
    
    
    
    
    nscoord desiredHeightWithBottomBP =
      aDesiredSize.Height() + aReflowState.ComputedPhysicalBorderPadding().bottom;

    if (aReflowState.AvailableHeight() == NS_UNCONSTRAINEDSIZE ||
        aDesiredSize.Height() == 0 ||
        desiredHeightWithBottomBP <= aReflowState.AvailableHeight() ||
        aReflowState.ComputedHeight() == NS_INTRINSICSIZE) {
      
      aDesiredSize.Height() = desiredHeightWithBottomBP;
    } else {
      
      NS_FRAME_SET_INCOMPLETE(aStatus);
    }
  }

  
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  for (nsIFrame* childFrame : mFrames) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, childFrame);
  }

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize,
                                 aReflowState, aStatus);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize)
}

void
nsFlexContainerFrame::MoveFlexItemToFinalPosition(
  const nsHTMLReflowState& aReflowState,
  const FlexItem& aItem,
  LogicalPoint& aFramePos,
  nscoord aContainerWidth)
{
  WritingMode outerWM = aReflowState.GetWritingMode();

  
  LogicalMargin logicalOffsets(outerWM);
  if (NS_STYLE_POSITION_RELATIVE == aItem.Frame()->StyleDisplay()->mPosition) {
    FrameProperties props = aItem.Frame()->Properties();
    nsMargin* cachedOffsets =
      static_cast<nsMargin*>(props.Get(nsIFrame::ComputedOffsetProperty()));
    MOZ_ASSERT(cachedOffsets,
               "relpos previously-reflowed frame should've cached its offsets");
    logicalOffsets = LogicalMargin(outerWM, *cachedOffsets);
  }
  nsHTMLReflowState::ApplyRelativePositioning(aItem.Frame(), outerWM,
                                              logicalOffsets, &aFramePos,
                                              aContainerWidth);
  aItem.Frame()->SetPosition(outerWM, aFramePos, aContainerWidth);
  PositionChildViews(aItem.Frame());
}

void
nsFlexContainerFrame::ReflowFlexItem(nsPresContext* aPresContext,
                                     const FlexboxAxisTracker& aAxisTracker,
                                     const nsHTMLReflowState& aReflowState,
                                     const FlexItem& aItem,
                                     LogicalPoint& aFramePos,
                                     nscoord aContainerWidth)
{
  WritingMode outerWM = aReflowState.GetWritingMode();
  WritingMode wm = aItem.Frame()->GetWritingMode();
  LogicalSize availSize = aReflowState.ComputedSize(wm);
  availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
  nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                     aItem.Frame(), availSize);

  
  
  bool didOverrideComputedWidth = false;
  bool didOverrideComputedHeight = false;

  
  if (aAxisTracker.IsMainAxisHorizontal()) {
    childReflowState.SetComputedWidth(aItem.GetMainSize());
    didOverrideComputedWidth = true;
  } else {
    childReflowState.SetComputedHeight(aItem.GetMainSize());
    didOverrideComputedHeight = true;
  }

  
  if (aItem.IsStretched()) {
    MOZ_ASSERT(aItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_STRETCH,
               "stretched item w/o 'align-self: stretch'?");
    if (aAxisTracker.IsCrossAxisHorizontal()) {
      childReflowState.SetComputedWidth(aItem.GetCrossSize());
      didOverrideComputedWidth = true;
    } else {
      
      aItem.Frame()->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
      childReflowState.SetComputedHeight(aItem.GetCrossSize());
      didOverrideComputedHeight = true;
    }
  }

  
  
  

  
  
  
  if (aItem.HadMeasuringReflow()) {
    if (didOverrideComputedWidth) {
      
      
      
      
      childReflowState.SetHResize(true);
    }
    if (didOverrideComputedHeight) {
      childReflowState.SetVResize(true);
    }
  }
  
  
  

  nsHTMLReflowMetrics childDesiredSize(childReflowState);
  nsReflowStatus childReflowStatus;
  ReflowChild(aItem.Frame(), aPresContext,
              childDesiredSize, childReflowState,
              outerWM, aFramePos, aContainerWidth,
              0, childReflowStatus);

  
  
  
  
  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
             "We gave flex item unconstrained available height, so it "
             "should be complete");

  LogicalMargin offsets =
    childReflowState.ComputedLogicalOffsets().ConvertTo(outerWM, wm);
  nsHTMLReflowState::ApplyRelativePositioning(aItem.Frame(), outerWM,
                                              offsets, &aFramePos,
                                              aContainerWidth);

  FinishReflowChild(aItem.Frame(), aPresContext,
                    childDesiredSize, &childReflowState,
                    outerWM, aFramePos, aContainerWidth, 0);

  
  if (aItem.Frame() == mFrames.FirstChild()) {
    aItem.SetAscent(childDesiredSize.BlockStartAscent());
  }
}

 nscoord
nsFlexContainerFrame::GetMinISize(nsRenderingContext* aRenderingContext)
{
  nscoord minWidth = 0;
  DISPLAY_MIN_WIDTH(this, minWidth);

  const nsStylePosition* stylePos = StylePosition();
  const FlexboxAxisTracker axisTracker(stylePos, GetWritingMode());

  for (nsIFrame* childFrame : mFrames) {
    nscoord childMinWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, childFrame,
                                           nsLayoutUtils::MIN_ISIZE);
    
    
    
    
    if (axisTracker.IsMainAxisHorizontal() &&
        NS_STYLE_FLEX_WRAP_NOWRAP == stylePos->mFlexWrap) {
      minWidth += childMinWidth;
    } else {
      minWidth = std::max(minWidth, childMinWidth);
    }
  }
  return minWidth;
}

 nscoord
nsFlexContainerFrame::GetPrefISize(nsRenderingContext* aRenderingContext)
{
  nscoord prefWidth = 0;
  DISPLAY_PREF_WIDTH(this, prefWidth);

  
  
  
  
  
  const FlexboxAxisTracker axisTracker(StylePosition(), GetWritingMode());

  for (nsIFrame* childFrame : mFrames) {
    nscoord childPrefWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, childFrame,
                                           nsLayoutUtils::PREF_ISIZE);
    if (axisTracker.IsMainAxisHorizontal()) {
      prefWidth += childPrefWidth;
    } else {
      prefWidth = std::max(prefWidth, childPrefWidth);
    }
  }
  return prefWidth;
}
