








#include "nsFlexContainerFrame.h"
#include "nsContentUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"
#include "nsPlaceholderFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "prlog.h"
#include <algorithm>

using namespace mozilla::css;
using namespace mozilla::layout;

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



static const Side
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


static inline bool
IsAxisHorizontal(AxisOrientationType aAxis)
{
  return eAxis_LR == aAxis || eAxis_RL == aAxis;
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



static inline const nsStyleCoord&
GetSizePropertyForAxis(const nsIFrame* aFrame, AxisOrientationType aAxis)
{
  const nsStylePosition* stylePos = aFrame->StylePosition();

  return IsAxisHorizontal(aAxis) ?
    stylePos->mWidth :
    stylePos->mHeight;
}










static nscoord
PhysicalPosFromLogicalPos(nscoord aLogicalPosn,
                          nscoord aLogicalContainerSize,
                          AxisOrientationType aAxis) {
  if (AxisGrowsInPositiveDirection(aAxis)) {
    return aLogicalPosn;
  }
  return aLogicalContainerSize - aLogicalPosn;
}

static nscoord
MarginComponentForSide(const nsMargin& aMargin, Side aSide)
{
  switch (aSide) {
    case eSideLeft:
      return aMargin.left;
    case eSideRight:
      return aMargin.right;
    case eSideTop:
      return aMargin.top;
    case eSideBottom:
      return aMargin.bottom;
  }

  NS_NOTREACHED("unexpected Side enum");
  return aMargin.left; 
                       
}

static nscoord&
MarginComponentForSide(nsMargin& aMargin, Side aSide)
{
  switch (aSide) {
    case eSideLeft:
      return aMargin.left;
    case eSideRight:
      return aMargin.right;
    case eSideTop:
      return aMargin.top;
    case eSideBottom:
      return aMargin.bottom;
  }

  NS_NOTREACHED("unexpected Side enum");
  return aMargin.left; 
                       
}









#define GET_MAIN_COMPONENT(axisTracker_, width_, height_)  \
  IsAxisHorizontal((axisTracker_).GetMainAxis()) ? (width_) : (height_)

#define GET_CROSS_COMPONENT(axisTracker_, width_, height_)  \
  IsAxisHorizontal((axisTracker_).GetCrossAxis()) ? (width_) : (height_)


class MOZ_STACK_CLASS FlexboxAxisTracker {
public:
  FlexboxAxisTracker(nsFlexContainerFrame* aFlexContainerFrame);

  
  AxisOrientationType GetMainAxis() const  { return mMainAxis;  }
  AxisOrientationType GetCrossAxis() const { return mCrossAxis; }

  nscoord GetMainComponent(const nsSize& aSize) const {
    return GET_MAIN_COMPONENT(*this, aSize.width, aSize.height);
  }
  int32_t GetMainComponent(const nsIntSize& aIntSize) const {
    return GET_MAIN_COMPONENT(*this, aIntSize.width, aIntSize.height);
  }

  nscoord GetCrossComponent(const nsSize& aSize) const {
    return GET_CROSS_COMPONENT(*this, aSize.width, aSize.height);
  }
  int32_t GetCrossComponent(const nsIntSize& aIntSize) const {
    return GET_CROSS_COMPONENT(*this, aIntSize.width, aIntSize.height);
  }

  nscoord GetMarginSizeInMainAxis(const nsMargin& aMargin) const {
    return IsAxisHorizontal(mMainAxis) ?
      aMargin.LeftRight() :
      aMargin.TopBottom();
  }
  nscoord GetMarginSizeInCrossAxis(const nsMargin& aMargin) const {
    return IsAxisHorizontal(mCrossAxis) ?
      aMargin.LeftRight() :
      aMargin.TopBottom();
  }

  













  nsPoint PhysicalPointFromLogicalPoint(nscoord aMainPosn,
                                        nscoord aCrossPosn,
                                        nscoord aContainerMainSize,
                                        nscoord aContainerCrossSize) const {
    nscoord physicalPosnInMainAxis =
      PhysicalPosFromLogicalPos(aMainPosn, aContainerMainSize, mMainAxis);
    nscoord physicalPosnInCrossAxis =
      PhysicalPosFromLogicalPos(aCrossPosn, aContainerCrossSize, mCrossAxis);

    return IsAxisHorizontal(mMainAxis) ?
      nsPoint(physicalPosnInMainAxis, physicalPosnInCrossAxis) :
      nsPoint(physicalPosnInCrossAxis, physicalPosnInMainAxis);
  }
  nsSize PhysicalSizeFromLogicalSizes(nscoord aMainSize,
                                      nscoord aCrossSize) const {
    return IsAxisHorizontal(mMainAxis) ?
      nsSize(aMainSize, aCrossSize) :
      nsSize(aCrossSize, aMainSize);
  }

private:
  AxisOrientationType mMainAxis;
  AxisOrientationType mCrossAxis;
};




class FlexItem {
public:
  FlexItem(nsIFrame* aChildFrame,
           float aFlexGrow, float aFlexShrink, nscoord aMainBaseSize,
           nscoord aMainMinSize, nscoord aMainMaxSize,
           nscoord aCrossMinSize, nscoord aCrossMaxSize,
           nsMargin aMargin, nsMargin aBorderPadding,
           const FlexboxAxisTracker& aAxisTracker);

  
  nsIFrame* Frame() const          { return mFrame; }
  nscoord GetFlexBaseSize() const  { return mFlexBaseSize; }

  nscoord GetMainMinSize() const   { return mMainMinSize; }
  nscoord GetMainMaxSize() const   { return mMainMaxSize; }

  
  nscoord GetMainSize() const      { return mMainSize; }
  nscoord GetMainPosition() const  { return mMainPosn; }

  nscoord GetCrossMinSize() const  { return mCrossMinSize; }
  nscoord GetCrossMaxSize() const  { return mCrossMaxSize; }

  
  nscoord GetCrossSize() const     { return mCrossSize;  }
  nscoord GetCrossPosition() const { return mCrossPosn; }

  
  
  
  
  nscoord GetBaselineOffsetFromOuterCrossStart(
            AxisOrientationType aCrossAxis) const;

  float GetShareOfFlexWeightSoFar() const { return mShareOfFlexWeightSoFar; }

  bool IsFrozen() const            { return mIsFrozen; }

  bool HadMinViolation() const     { return mHadMinViolation; }
  bool HadMaxViolation() const     { return mHadMaxViolation; }

  
  
  bool HadMeasuringReflow() const  { return mHadMeasuringReflow; }

  
  
  
  bool IsStretched() const         { return mIsStretched; }

  uint8_t GetAlignSelf() const     { return mAlignSelf; }

  
  
  
  
  
  float GetFlexWeightToUse(bool aIsUsingFlexGrow)
  {
    if (IsFrozen()) {
      return 0.0f;
    }

    return aIsUsingFlexGrow ?
      mFlexGrow :
      mFlexShrink * mFlexBaseSize;
  }

  
  
  const nsMargin& GetMargin() const { return mMargin; }

  
  nscoord GetMarginComponentForSide(Side aSide) const
  { return MarginComponentForSide(mMargin, aSide); }

  
  nscoord GetMarginSizeInAxis(AxisOrientationType aAxis) const
  {
    Side startSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_Start];
    Side endSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_End];
    return GetMarginComponentForSide(startSide) +
      GetMarginComponentForSide(endSide);
  }

  
  
  const nsMargin& GetBorderPadding() const { return mBorderPadding; }

  
  nscoord GetBorderPaddingComponentForSide(Side aSide) const
  { return MarginComponentForSide(mBorderPadding, aSide); }

  
  
  nscoord GetBorderPaddingSizeInAxis(AxisOrientationType aAxis) const
  {
    Side startSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_Start];
    Side endSide = kAxisOrientationToSidesMap[aAxis][eAxisEdge_End];
    return GetBorderPaddingComponentForSide(startSide) +
      GetBorderPaddingComponentForSide(endSide);
  }

  
  
  
  
  nscoord GetMarginBorderPaddingSizeInAxis(AxisOrientationType aAxis) const
  {
    return GetMarginSizeInAxis(aAxis) + GetBorderPaddingSizeInAxis(aAxis);
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

  void SetShareOfFlexWeightSoFar(float aNewShare)
  {
    MOZ_ASSERT(!mIsFrozen || aNewShare == 0.0f,
               "shouldn't be giving this item any share of the weight "
               "after it's frozen");
    mShareOfFlexWeightSoFar = aNewShare;
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

  void SetAscent(nscoord aAscent) {
    mAscent = aAscent;
  }

  void SetHadMeasuringReflow() {
    mHadMeasuringReflow = true;
  }

  void SetIsStretched() {
    MOZ_ASSERT(mIsFrozen, "main size should be resolved before this");
    mIsStretched = true;
  }

  
  void SetMarginComponentForSide(Side aSide, nscoord aLength)
  {
    MOZ_ASSERT(mIsFrozen, "main size should be resolved before this");
    MarginComponentForSide(mMargin, aSide) = aLength;
  }

  void ResolveStretchedCrossSize(nscoord aLineCrossSize,
                                 const FlexboxAxisTracker& aAxisTracker);

  uint32_t GetNumAutoMarginsInAxis(AxisOrientationType aAxis) const;

protected:
  
  nsIFrame* const mFrame;

  
  const float mFlexGrow;
  const float mFlexShrink;

  const nsMargin mBorderPadding;
  nsMargin mMargin; 

  nscoord mFlexBaseSize;

  const nscoord mMainMinSize;
  const nscoord mMainMaxSize;
  const nscoord mCrossMinSize;
  const nscoord mCrossMaxSize;

  
  nscoord mMainSize;
  nscoord mMainPosn;
  nscoord mCrossSize;
  nscoord mCrossPosn;
  nscoord mAscent;

  
  
  
  
  
  float mShareOfFlexWeightSoFar;
  bool mIsFrozen;
  bool mHadMinViolation;
  bool mHadMaxViolation;

  
  bool mHadMeasuringReflow; 
                            
  bool mIsStretched; 
  uint8_t mAlignSelf; 
                      
                      
};



class FlexLine {
public:
  FlexLine()
  : mTotalInnerHypotheticalMainSize(0),
    mTotalOuterHypotheticalMainSize(0),
    mLineCrossSize(0),
    mBaselineOffsetFromCrossStart(nscoord_MIN)
  {}

  
  
  nscoord GetTotalOuterHypotheticalMainSize() const {
    return mTotalOuterHypotheticalMainSize;
  }

  
  
  void AddToMainSizeTotals(nscoord aItemInnerHypotheticalMainSize,
                           nscoord aItemOuterHypotheticalMainSize) {
    mTotalInnerHypotheticalMainSize += aItemInnerHypotheticalMainSize;
    mTotalOuterHypotheticalMainSize += aItemOuterHypotheticalMainSize;
  }

  
  
  void ComputeCrossSizeAndBaseline(const FlexboxAxisTracker& aAxisTracker);

  
  nscoord GetLineCrossSize() const { return mLineCrossSize; }

  
  
  
  void SetLineCrossSize(nscoord aLineCrossSize) {
    mLineCrossSize = aLineCrossSize;
  }

  
  
  
  nscoord GetBaselineOffsetFromCrossStart() const {
    return mBaselineOffsetFromCrossStart;
  }

  
  
  void ResolveFlexibleLengths(nscoord aFlexContainerMainSize);

  void PositionItemsInMainAxis(uint8_t aJustifyContent,
                               nscoord aContentBoxMainSize,
                               const FlexboxAxisTracker& aAxisTracker);

  void PositionItemsInCrossAxis(nscoord aLineStartPosition,
                                const FlexboxAxisTracker& aAxisTracker);

  nsTArray<FlexItem> mItems; 

private:
  nscoord mTotalInnerHypotheticalMainSize;
  nscoord mTotalOuterHypotheticalMainSize;
  nscoord mLineCrossSize;
  nscoord mBaselineOffsetFromCrossStart;
};


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
  const FlexboxAxisTracker axisTracker(this);
  return IsAxisHorizontal(axisTracker.GetMainAxis());
}

FlexItem
nsFlexContainerFrame::GenerateFlexItemForChild(
  nsPresContext* aPresContext,
  nsIFrame*      aChildFrame,
  const nsHTMLReflowState& aParentReflowState,
  const FlexboxAxisTracker& aAxisTracker)
{
  
  
  
  nsHTMLReflowState childRS(aPresContext, aParentReflowState, aChildFrame,
                            nsSize(aParentReflowState.ComputedWidth(),
                                   aParentReflowState.ComputedHeight()));

  
  
  const nsStylePosition* stylePos = aChildFrame->StylePosition();
  float flexGrow   = stylePos->mFlexGrow;
  float flexShrink = stylePos->mFlexShrink;

  
  
  nscoord flexBaseSize = GET_MAIN_COMPONENT(aAxisTracker,
                                            childRS.ComputedWidth(),
                                            childRS.ComputedHeight());
  nscoord mainMinSize = GET_MAIN_COMPONENT(aAxisTracker,
                                           childRS.mComputedMinWidth,
                                           childRS.mComputedMinHeight);
  nscoord mainMaxSize = GET_MAIN_COMPONENT(aAxisTracker,
                                           childRS.mComputedMaxWidth,
                                           childRS.mComputedMaxHeight);
  
  MOZ_ASSERT(mainMinSize <= mainMaxSize, "min size is larger than max size");

  
  

  nscoord crossMinSize = GET_CROSS_COMPONENT(aAxisTracker,
                                             childRS.mComputedMinWidth,
                                             childRS.mComputedMinHeight);
  nscoord crossMaxSize = GET_CROSS_COMPONENT(aAxisTracker,
                                             childRS.mComputedMaxWidth,
                                             childRS.mComputedMaxHeight);

  
  
  
  
  bool isFixedSizeWidget = false;
  const nsStyleDisplay* disp = aChildFrame->StyleDisplay();
  if (aChildFrame->IsThemed(disp)) {
    nsIntSize widgetMinSize(0, 0);
    bool canOverride = true;
    aPresContext->GetTheme()->
      GetMinimumWidgetSize(childRS.rendContext, aChildFrame,
                           disp->mAppearance,
                           &widgetMinSize, &canOverride);

    nscoord widgetMainMinSize =
      aPresContext->DevPixelsToAppUnits(
        aAxisTracker.GetMainComponent(widgetMinSize));
    nscoord widgetCrossMinSize =
      aPresContext->DevPixelsToAppUnits(
        aAxisTracker.GetCrossComponent(widgetMinSize));

    
    widgetMainMinSize -=
      aAxisTracker.GetMarginSizeInMainAxis(childRS.mComputedBorderPadding);
    widgetCrossMinSize -=
      aAxisTracker.GetMarginSizeInCrossAxis(childRS.mComputedBorderPadding);

    if (!canOverride) {
      
      
      
      flexBaseSize = mainMinSize = mainMaxSize = widgetMainMinSize;
      crossMinSize = crossMaxSize = widgetCrossMinSize;
      isFixedSizeWidget = true;
    } else {
      
      
      mainMinSize = std::max(mainMinSize, widgetMainMinSize);
      mainMaxSize = std::max(mainMaxSize, widgetMainMinSize);

      crossMinSize = std::max(crossMinSize, widgetCrossMinSize);
      crossMaxSize = std::max(crossMaxSize, widgetCrossMinSize);
    }
  }

  
  FlexItem item(aChildFrame,
                flexGrow, flexShrink, flexBaseSize,
                mainMinSize, mainMaxSize,
                crossMinSize, crossMaxSize,
                childRS.mComputedMargin,
                childRS.mComputedBorderPadding,
                aAxisTracker);

  
  
  
  if (isFixedSizeWidget || (flexGrow == 0.0f && flexShrink == 0.0f)) {
    item.Freeze();
  }

  return item;
}

nsresult
nsFlexContainerFrame::
  ResolveFlexItemMaxContentSizing(nsPresContext* aPresContext,
                                  FlexItem& aFlexItem,
                                  const nsHTMLReflowState& aParentReflowState,
                                  const FlexboxAxisTracker& aAxisTracker)
{
  if (IsAxisHorizontal(aAxisTracker.GetMainAxis())) {
    
    
    return NS_OK;
  }

  if (NS_AUTOHEIGHT != aFlexItem.GetFlexBaseSize()) {
    
    
    
    
    return NS_OK;
  }

  
  
  
  
  
  

  
  
  
  nsHTMLReflowState
    childRSForMeasuringHeight(aPresContext, aParentReflowState,
                              aFlexItem.Frame(),
                              nsSize(aParentReflowState.ComputedWidth(),
                                     NS_UNCONSTRAINEDSIZE),
                              -1, -1, nsHTMLReflowState::CALLER_WILL_INIT);
  childRSForMeasuringHeight.mFlags.mIsFlexContainerMeasuringHeight = true;
  childRSForMeasuringHeight.Init(aPresContext);

  aFlexItem.ResolveStretchedCrossSize(aParentReflowState.ComputedWidth(),
                                      aAxisTracker);
  if (aFlexItem.IsStretched()) {
    childRSForMeasuringHeight.SetComputedWidth(aFlexItem.GetCrossSize());
    childRSForMeasuringHeight.mFlags.mHResize = true;
  }

  
  
  
  
  
  
  
  
  
  if (!aFlexItem.IsFrozen()) {  
    childRSForMeasuringHeight.mFlags.mVResize = true;
  }

  nsHTMLReflowMetrics childDesiredSize;
  nsReflowStatus childReflowStatus;
  nsresult rv = ReflowChild(aFlexItem.Frame(), aPresContext,
                            childDesiredSize, childRSForMeasuringHeight,
                            0, 0, NS_FRAME_NO_MOVE_FRAME,
                            childReflowStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
             "We gave flex item unconstrained available height, so it "
             "should be complete");

  rv = FinishReflowChild(aFlexItem.Frame(), aPresContext,
                         &childRSForMeasuringHeight, childDesiredSize,
                         0, 0, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nscoord childDesiredHeight = childDesiredSize.height -
    childRSForMeasuringHeight.mComputedBorderPadding.TopBottom();
  childDesiredHeight = std::max(0, childDesiredHeight);

  aFlexItem.SetFlexBaseSizeAndMainSize(childDesiredHeight);
  aFlexItem.SetHadMeasuringReflow();

  return NS_OK;
}

FlexItem::FlexItem(nsIFrame* aChildFrame,
                   float aFlexGrow, float aFlexShrink, nscoord aFlexBaseSize,
                   nscoord aMainMinSize,  nscoord aMainMaxSize,
                   nscoord aCrossMinSize, nscoord aCrossMaxSize,
                   nsMargin aMargin, nsMargin aBorderPadding,
                   const FlexboxAxisTracker& aAxisTracker)
  : mFrame(aChildFrame),
    mFlexGrow(aFlexGrow),
    mFlexShrink(aFlexShrink),
    mBorderPadding(aBorderPadding),
    mMargin(aMargin),
    mMainMinSize(aMainMinSize),
    mMainMaxSize(aMainMaxSize),
    mCrossMinSize(aCrossMinSize),
    mCrossMaxSize(aCrossMaxSize),
    mMainPosn(0),
    mCrossSize(0),
    mCrossPosn(0),
    mAscent(0),
    mShareOfFlexWeightSoFar(0.0f),
    mIsFrozen(false),
    mHadMinViolation(false),
    mHadMaxViolation(false),
    mHadMeasuringReflow(false),
    mIsStretched(false),
    mAlignSelf(aChildFrame->StylePosition()->mAlignSelf)
{
  MOZ_ASSERT(aChildFrame, "expecting a non-null child frame");

  SetFlexBaseSizeAndMainSize(aFlexBaseSize);

  
  
#ifdef DEBUG
  {
    const nsStyleSides& styleMargin = mFrame->StyleMargin()->mMargin;
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
      IsAxisHorizontal(aAxisTracker.GetCrossAxis())) {
    mAlignSelf = NS_STYLE_ALIGN_ITEMS_FLEX_START;
  }
}

nscoord
FlexItem::GetBaselineOffsetFromOuterCrossStart(
  AxisOrientationType aCrossAxis) const
{
  
  
  
  
  
  MOZ_ASSERT(!IsAxisHorizontal(aCrossAxis),
             "Only expecting to be doing baseline computations when the "
             "cross axis is vertical");
 
  nscoord marginTopToBaseline = mAscent + mMargin.top;

  if (aCrossAxis == eAxis_TB) {
    
    
    return marginTopToBaseline;
  }

  
  
  
  
  nscoord outerCrossSize = mCrossSize +
    GetMarginBorderPaddingSizeInAxis(aCrossAxis);

  return outerCrossSize - marginTopToBaseline;
}

uint32_t
FlexItem::GetNumAutoMarginsInAxis(AxisOrientationType aAxis) const
{
  uint32_t numAutoMargins = 0;
  const nsStyleSides& styleMargin = mFrame->StyleMargin()->mMargin;
  for (uint32_t i = 0; i < eNumAxisEdges; i++) {
    Side side = kAxisOrientationToSidesMap[aAxis][i];
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
    Side side = kAxisOrientationToSidesMap[mAxis][eAxisEdge_Start];
    mPosition += MarginComponentForSide(aMargin, side);
  }

  
  
  void ExitMargin(const nsMargin& aMargin)
  {
    Side side = kAxisOrientationToSidesMap[mAxis][eAxisEdge_End];
    mPosition += MarginComponentForSide(aMargin, side);
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
  
  PositionTracker(AxisOrientationType aAxis)
    : mPosition(0),
      mAxis(aAxis)
  {}

private:
  
  
  PositionTracker(const PositionTracker& aOther)
    : mPosition(aOther.mPosition),
      mAxis(aOther.mAxis)
  {}

protected:
  
  nscoord mPosition;               
  const AxisOrientationType mAxis; 
};




class MOZ_STACK_CLASS MainAxisPositionTracker : public PositionTracker {
public:
  MainAxisPositionTracker(const FlexboxAxisTracker& aAxisTracker,
                          const nsTArray<FlexItem>& aItems,
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
  CrossAxisPositionTracker(nsTArray<FlexLine>& aLines,
                           uint8_t aAlignContent,
                           nscoord aContentBoxCrossSize,
                           bool aIsCrossSizeDefinite,
                           const FlexboxAxisTracker& aAxisTracker);

  
  void TraversePackingSpace();

  
  void TraverseLine(FlexLine& aLine) { mPosition += aLine.GetLineCrossSize(); }

private:
  
  
  
  
  void EnterMargin(const nsMargin& aMargin) MOZ_DELETE;
  void ExitMargin(const nsMargin& aMargin) MOZ_DELETE;
  void EnterChildFrame(nscoord aChildFrameSize) MOZ_DELETE;
  void ExitChildFrame(nscoord aChildFrameSize) MOZ_DELETE;

  nscoord  mPackingSpaceRemaining;
  uint32_t mNumPackingSpacesRemaining;
  uint8_t  mAlignContent;
};



class MOZ_STACK_CLASS SingleLineCrossAxisPositionTracker : public PositionTracker {
public:
  SingleLineCrossAxisPositionTracker(const FlexboxAxisTracker& aAxisTracker);

  void ResolveAutoMarginsInCrossAxis(const FlexLine& aLine,
                                     FlexItem& aItem);

  void EnterAlignPackingSpace(const FlexLine& aLine,
                              const FlexItem& aItem);

  
  inline void ResetPosition() { mPosition = 0; }
};






NS_QUERYFRAME_HEAD(nsFlexContainerFrame)
  NS_QUERYFRAME_ENTRY(nsFlexContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFlexContainerFrameSuper)

NS_IMPL_FRAMEARENA_HELPERS(nsFlexContainerFrame)

nsIFrame*
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

#ifdef DEBUG
NS_IMETHODIMP
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
    nsIFrame::IsFrameListSorted<IsOrderLEQWithDOMFallback>(mFrames),
    "Child frames aren't sorted correctly");

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  
  
  nsDisplayListSet childLists(aLists, aLists.BlockBorderBackgrounds());
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    BuildDisplayListForChild(aBuilder, e.get(), aDirtyRect, childLists,
                             GetDisplayFlagsForFlexItem(e.get()));
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
  for (nsIFrame* child = mFrames.FirstChild(); child;
       child = child->GetNextSibling()) {
    MOZ_ASSERT(!FrameWantsToBeInAnonymousFlexItem(child),
               "frame wants to be inside an anonymous flex item, "
               "but it isn't");
    if (child->StyleContext()->GetPseudo() ==
        nsCSSAnonBoxes::anonymousFlexItem) {
      MOZ_ASSERT(!prevChildWasAnonFlexItem,
                 "two anon flex items in a row (shouldn't happen)");

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



static void
FreezeOrRestoreEachFlexibleSize(
  const nscoord aTotalViolation,
  nsTArray<FlexItem>& aItems)
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

  for (uint32_t i = 0; i < aItems.Length(); i++) {
    FlexItem& item = aItems[i];
    MOZ_ASSERT(!item.HadMinViolation() || !item.HadMaxViolation(),
               "Can have either min or max violation, but not both");

    if (!item.IsFrozen()) {
      if (eFreezeEverything == freezeType ||
          (eFreezeMinViolations == freezeType && item.HadMinViolation()) ||
          (eFreezeMaxViolations == freezeType && item.HadMaxViolation())) {

        MOZ_ASSERT(item.GetMainSize() >= item.GetMainMinSize(),
                   "Freezing item at a size below its minimum");
        MOZ_ASSERT(item.GetMainSize() <= item.GetMainMaxSize(),
                   "Freezing item at a size above its maximum");

        item.Freeze();
      } 
        

      
      item.ClearViolationFlags();
    }
  }
}







void
FlexLine::ResolveFlexibleLengths(nscoord aFlexContainerMainSize)
{
  PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG, ("ResolveFlexibleLengths\n"));
  if (mItems.IsEmpty()) {
    return;
  }

  
  
  
  nscoord spaceReservedForMarginBorderPadding =
    mTotalOuterHypotheticalMainSize - mTotalInnerHypotheticalMainSize;

  nscoord spaceAvailableForFlexItemsContentBoxes =
    aFlexContainerMainSize - spaceReservedForMarginBorderPadding;

  
  const bool isUsingFlexGrow =
    (mTotalOuterHypotheticalMainSize < aFlexContainerMainSize);

  
  
  
  
  
  
  for (uint32_t iterationCounter = 0;
       iterationCounter < mItems.Length(); iterationCounter++) {
    
    
    
    
    nscoord availableFreeSpace = spaceAvailableForFlexItemsContentBoxes;
    for (uint32_t i = 0; i < mItems.Length(); i++) {
      FlexItem& item = mItems[i];
      if (!item.IsFrozen()) {
        item.SetMainSize(item.GetFlexBaseSize());
      }
      availableFreeSpace -= item.GetMainSize();
    }

    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" available free space = %d\n", availableFreeSpace));

    
    
    if ((availableFreeSpace > 0 && isUsingFlexGrow) ||
        (availableFreeSpace < 0 && !isUsingFlexGrow)) {

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      float runningFlexWeightSum = 0.0f;
      float largestFlexWeight = 0.0f;
      uint32_t numItemsWithLargestFlexWeight = 0;
      for (uint32_t i = 0; i < mItems.Length(); i++) {
        FlexItem& item = mItems[i];
        float curFlexWeight = item.GetFlexWeightToUse(isUsingFlexGrow);
        MOZ_ASSERT(curFlexWeight >= 0.0f, "weights are non-negative");

        runningFlexWeightSum += curFlexWeight;
        if (NS_finite(runningFlexWeightSum)) {
          if (curFlexWeight == 0.0f) {
            item.SetShareOfFlexWeightSoFar(0.0f);
          } else {
            item.SetShareOfFlexWeightSoFar(curFlexWeight /
                                           runningFlexWeightSum);
          }
        } 
          
          
          

        
        if (curFlexWeight > largestFlexWeight) {
          largestFlexWeight = curFlexWeight;
          numItemsWithLargestFlexWeight = 1;
        } else if (curFlexWeight == largestFlexWeight) {
          numItemsWithLargestFlexWeight++;
        }
      }

      if (runningFlexWeightSum != 0.0f) { 
        PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
               (" Distributing available space:"));
        for (uint32_t i = mItems.Length() - 1; i < mItems.Length(); --i) {
          FlexItem& item = mItems[i];

          if (!item.IsFrozen()) {
            
            
            nscoord sizeDelta = 0;
            if (NS_finite(runningFlexWeightSum)) {
              float myShareOfRemainingSpace =
                item.GetShareOfFlexWeightSoFar();

              MOZ_ASSERT(myShareOfRemainingSpace >= 0.0f &&
                         myShareOfRemainingSpace <= 1.0f,
                         "my share should be nonnegative fractional amount");

              if (myShareOfRemainingSpace == 1.0f) {
                
                
                sizeDelta = availableFreeSpace;
              } else if (myShareOfRemainingSpace > 0.0f) {
                sizeDelta = NSToCoordRound(availableFreeSpace *
                                           myShareOfRemainingSpace);
              }
            } else if (item.GetFlexWeightToUse(isUsingFlexGrow) ==
                       largestFlexWeight) {
              
              
              
              sizeDelta =
                NSToCoordRound(availableFreeSpace /
                               float(numItemsWithLargestFlexWeight));
              numItemsWithLargestFlexWeight--;
            }

            availableFreeSpace -= sizeDelta;

            item.SetMainSize(item.GetMainSize() + sizeDelta);
            PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
                   ("  child %d receives %d, for a total of %d\n",
                    i, sizeDelta, item.GetMainSize()));
          }
        }
      }
    }

    
    nscoord totalViolation = 0; 
    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" Checking for violations:"));

    for (uint32_t i = 0; i < mItems.Length(); i++) {
      FlexItem& item = mItems[i];
      if (!item.IsFrozen()) {
        if (item.GetMainSize() < item.GetMainMinSize()) {
          
          totalViolation += item.GetMainMinSize() - item.GetMainSize();
          item.SetMainSize(item.GetMainMinSize());
          item.SetHadMinViolation();
        } else if (item.GetMainSize() > item.GetMainMaxSize()) {
          
          totalViolation += item.GetMainMaxSize() - item.GetMainSize();
          item.SetMainSize(item.GetMainMaxSize());
          item.SetHadMaxViolation();
        }
      }
    }

    FreezeOrRestoreEachFlexibleSize(totalViolation, mItems);

    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" Total violation: %d\n", totalViolation));

    if (totalViolation == 0) {
      break;
    }
  }

  
#ifdef DEBUG
  for (uint32_t i = 0; i < mItems.Length(); ++i) {
    MOZ_ASSERT(mItems[i].IsFrozen(),
               "All flexible lengths should've been resolved");
  }
#endif 
}

MainAxisPositionTracker::
  MainAxisPositionTracker(const FlexboxAxisTracker& aAxisTracker,
                          const nsTArray<FlexItem>& aItems,
                          uint8_t aJustifyContent,
                          nscoord aContentBoxMainSize)
  : PositionTracker(aAxisTracker.GetMainAxis()),
    mPackingSpaceRemaining(aContentBoxMainSize), 
    mNumAutoMarginsInMainAxis(0),
    mNumPackingSpacesRemaining(0),
    mJustifyContent(aJustifyContent)
{
  
  
  
  for (uint32_t i = 0; i < aItems.Length(); i++) {
    const FlexItem& curItem = aItems[i];
    nscoord itemMarginBoxMainSize =
      curItem.GetMainSize() +
      curItem.GetMarginBorderPaddingSizeInAxis(aAxisTracker.GetMainAxis());
    mPackingSpaceRemaining -= itemMarginBoxMainSize;
    mNumAutoMarginsInMainAxis += curItem.GetNumAutoMarginsInAxis(mAxis);
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

  
  
  if (mNumAutoMarginsInMainAxis == 0 &&
      mPackingSpaceRemaining != 0 &&
      !aItems.IsEmpty()) {
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
        
        mNumPackingSpacesRemaining = aItems.Length() - 1;
        break;
      case NS_STYLE_JUSTIFY_CONTENT_SPACE_AROUND:
        MOZ_ASSERT(mPackingSpaceRemaining >= 0,
                   "negative packing space should make us use 'center' "
                   "instead of 'space-around'");
        
        
        
        mNumPackingSpacesRemaining = aItems.Length();
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
      Side side = kAxisOrientationToSidesMap[mAxis][i];
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
  CrossAxisPositionTracker(nsTArray<FlexLine>& aLines,
                           uint8_t aAlignContent,
                           nscoord aContentBoxCrossSize,
                           bool aIsCrossSizeDefinite,
                           const FlexboxAxisTracker& aAxisTracker)
  : PositionTracker(aAxisTracker.GetCrossAxis()),
    mPackingSpaceRemaining(0),
    mNumPackingSpacesRemaining(0),
    mAlignContent(aAlignContent)
{
  MOZ_ASSERT(!aLines.IsEmpty(), "We should have at least 1 line");

  if (aIsCrossSizeDefinite && aLines.Length() == 1) {
    
    
    
    
    
    
    
    
    aLines[0].SetLineCrossSize(aContentBoxCrossSize);
    return;
  }

  
  
  
  

  
  
  mPackingSpaceRemaining = aContentBoxCrossSize;
  for (uint32_t i = 0; i < aLines.Length(); i++) {
    const FlexLine& line = aLines[i];
    mPackingSpaceRemaining -= line.GetLineCrossSize();
  }

  
  
  
  
  if (mPackingSpaceRemaining < 0) {
    if (mAlignContent == NS_STYLE_ALIGN_CONTENT_SPACE_BETWEEN ||
        mAlignContent == NS_STYLE_ALIGN_CONTENT_STRETCH) {
      mAlignContent = NS_STYLE_ALIGN_CONTENT_FLEX_START;
    } else if (mAlignContent == NS_STYLE_ALIGN_CONTENT_SPACE_AROUND) {
      mAlignContent = NS_STYLE_ALIGN_CONTENT_CENTER;
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
        
        mNumPackingSpacesRemaining = aLines.Length() - 1;
        break;
      case NS_STYLE_ALIGN_CONTENT_SPACE_AROUND: {
        MOZ_ASSERT(mPackingSpaceRemaining >= 0,
                   "negative packing space should make us use 'center' "
                   "instead of 'space-around'");
        
        
        
        mNumPackingSpacesRemaining = aLines.Length();
        
        nscoord totalEdgePackingSpace =
          mPackingSpaceRemaining / mNumPackingSpacesRemaining;

        
        mPosition += totalEdgePackingSpace / 2;
        
        
        mPackingSpaceRemaining -= totalEdgePackingSpace;
        mNumPackingSpacesRemaining--;
        break;
      }
      case NS_STYLE_ALIGN_CONTENT_STRETCH:
        
        MOZ_ASSERT(mPackingSpaceRemaining > 0,
                   "negative packing space should make us use 'flex-start' "
                   "instead of 'stretch' (and we shouldn't bother with this "
                   "code if we have 0 packing space)");

        for (uint32_t i = 0; i < aLines.Length(); i++) {
          FlexLine& line = aLines[i];
          
          
          nscoord shareOfExtraSpace =
            mPackingSpaceRemaining / (aLines.Length() - i);
          nscoord newSize = line.GetLineCrossSize() + shareOfExtraSpace;
          line.SetLineCrossSize(newSize);
          mPackingSpaceRemaining -= shareOfExtraSpace;
        }
        break;
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
  nscoord crossStartToFurthestBaseline= nscoord_MIN;
  nscoord crossEndToFurthestBaseline = nscoord_MIN;
  nscoord largestOuterCrossSize = 0;
  for (uint32_t i = 0; i < mItems.Length(); ++i) {
    const FlexItem& curItem = mItems[i];
    nscoord curOuterCrossSize = curItem.GetCrossSize() +
      curItem.GetMarginBorderPaddingSizeInAxis(aAxisTracker.GetCrossAxis());

    if (curItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE &&
        curItem.GetNumAutoMarginsInAxis(aAxisTracker.GetCrossAxis()) == 0) {
      
      

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      nscoord crossStartToBaseline =
        curItem.GetBaselineOffsetFromOuterCrossStart(aAxisTracker.GetCrossAxis());
      nscoord crossEndToBaseline = curOuterCrossSize - crossStartToBaseline;

      
      
      
      crossStartToFurthestBaseline = std::max(crossStartToFurthestBaseline,
                                              crossStartToBaseline);
      crossEndToFurthestBaseline = std::max(crossEndToFurthestBaseline,
                                            crossEndToBaseline);
    } else {
      largestOuterCrossSize = std::max(largestOuterCrossSize, curOuterCrossSize);
    }
  }

  
  
  
  mBaselineOffsetFromCrossStart = crossStartToFurthestBaseline;

  
  
  
  
  
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
      eStyleUnit_Auto != GetSizePropertyForAxis(mFrame, crossAxis).GetUnit()) {
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
    (aItem.GetCrossSize() + aItem.GetMarginBorderPaddingSizeInAxis(mAxis));

  if (spaceForAutoMargins <= 0) {
    return; 
  }

  uint32_t numAutoMargins = aItem.GetNumAutoMarginsInAxis(mAxis);
  if (numAutoMargins == 0) {
    return; 
  }

  
  
  const nsStyleSides& styleMargin = aItem.Frame()->StyleMargin()->mMargin;
  for (uint32_t i = 0; i < eNumAxisEdges; i++) {
    Side side = kAxisOrientationToSidesMap[mAxis][i];
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
                         const FlexItem& aItem)
{
  
  
  if (aItem.GetNumAutoMarginsInAxis(mAxis)) {
    return;
  }

  switch (aItem.GetAlignSelf()) {
    case NS_STYLE_ALIGN_ITEMS_FLEX_START:
    case NS_STYLE_ALIGN_ITEMS_STRETCH:
      
      
      
      break;
    case NS_STYLE_ALIGN_ITEMS_FLEX_END:
      mPosition +=
        aLine.GetLineCrossSize() -
        (aItem.GetCrossSize() +
         aItem.GetMarginBorderPaddingSizeInAxis(mAxis));
      break;
    case NS_STYLE_ALIGN_ITEMS_CENTER:
      
      mPosition +=
        (aLine.GetLineCrossSize() -
         (aItem.GetCrossSize() +
          aItem.GetMarginBorderPaddingSizeInAxis(mAxis))) / 2;
      break;
    case NS_STYLE_ALIGN_ITEMS_BASELINE: {
      nscoord lineBaselineOffset =
        aLine.GetBaselineOffsetFromCrossStart();
      nscoord itemBaselineOffset =
        aItem.GetBaselineOffsetFromOuterCrossStart(mAxis);
      MOZ_ASSERT(lineBaselineOffset >= itemBaselineOffset,
                 "failed at finding largest baseline offset");

      
      mPosition += (lineBaselineOffset - itemBaselineOffset);
      break;
    }
    default:
      NS_NOTREACHED("Unexpected align-self value");
      break;
  }
}

FlexboxAxisTracker::FlexboxAxisTracker(nsFlexContainerFrame* aFlexContainerFrame)
{
  const nsStylePosition* pos = aFlexContainerFrame->StylePosition();
  uint32_t flexDirection = pos->mFlexDirection;
  uint32_t cssDirection =
    aFlexContainerFrame->StyleVisibility()->mDirection;

  MOZ_ASSERT(cssDirection == NS_STYLE_DIRECTION_LTR ||
             cssDirection == NS_STYLE_DIRECTION_RTL,
             "Unexpected computed value for 'direction' property");
  

  
  
  
  
  
  
  
  
  

  
  AxisOrientationType inlineDimension =
    cssDirection == NS_STYLE_DIRECTION_RTL ? eAxis_RL : eAxis_LR;

  
  AxisOrientationType blockDimension = eAxis_TB;

  
  switch (flexDirection) {
    case NS_STYLE_FLEX_DIRECTION_ROW:
      mMainAxis = inlineDimension;
      break;
    case NS_STYLE_FLEX_DIRECTION_ROW_REVERSE:
      mMainAxis = GetReverseAxis(inlineDimension);
      break;
    case NS_STYLE_FLEX_DIRECTION_COLUMN:
      mMainAxis = blockDimension;
      break;
    case NS_STYLE_FLEX_DIRECTION_COLUMN_REVERSE:
      mMainAxis = GetReverseAxis(blockDimension);
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

  
  if (pos->mFlexWrap == NS_STYLE_FLEX_WRAP_WRAP_REVERSE) {
    mCrossAxis = GetReverseAxis(mCrossAxis);
  }

  MOZ_ASSERT(IsAxisHorizontal(mMainAxis) != IsAxisHorizontal(mCrossAxis),
             "main & cross axes should be in different dimensions");
}

nsresult
nsFlexContainerFrame::GenerateFlexLines(
  nsPresContext* aPresContext,
  const nsHTMLReflowState& aReflowState,
  nscoord aContentBoxMainSize,
  nscoord aAvailableHeightForContent,
  const FlexboxAxisTracker& aAxisTracker,
  nsTArray<FlexLine>& aLines)
{
  MOZ_ASSERT(aLines.IsEmpty(), "Expecting outparam to start out empty");

  const bool isSingleLine =
    NS_STYLE_FLEX_WRAP_NOWRAP == aReflowState.mStylePosition->mFlexWrap;

  
  
  FlexLine* curLine = aLines.AppendElement();

  nscoord wrapThreshold;
  if (isSingleLine) {
    
    wrapThreshold = NS_UNCONSTRAINEDSIZE;

    
    
    curLine->mItems.SetCapacity(mFrames.GetLength());
  } else {
    
    wrapThreshold = aContentBoxMainSize;

    
    
    
    if (wrapThreshold == NS_UNCONSTRAINEDSIZE) {
      const nscoord flexContainerMaxMainSize =
        GET_MAIN_COMPONENT(aAxisTracker,
                           aReflowState.mComputedMaxWidth,
                           aReflowState.mComputedMaxHeight);

      wrapThreshold = flexContainerMaxMainSize;
    }

    
    
    if (!IsAxisHorizontal(aAxisTracker.GetMainAxis()) &&
        aAvailableHeightForContent != NS_UNCONSTRAINEDSIZE) {
      wrapThreshold = std::min(wrapThreshold, aAvailableHeightForContent);
    }
  }

  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nsIFrame* childFrame = e.get();

    
    if (!isSingleLine && !curLine->mItems.IsEmpty() &&
        childFrame->StyleDisplay()->mBreakBefore) {
      curLine = aLines.AppendElement();
    }

    FlexItem* item = curLine->mItems.AppendElement(
                       GenerateFlexItemForChild(aPresContext, childFrame,
                                                aReflowState, aAxisTracker));

    nsresult rv = ResolveFlexItemMaxContentSizing(aPresContext, *item,
                                                  aReflowState, aAxisTracker);
    NS_ENSURE_SUCCESS(rv,rv);
    nscoord itemInnerHypotheticalMainSize = item->GetMainSize();
    nscoord itemOuterHypotheticalMainSize = item->GetMainSize() +
      item->GetMarginBorderPaddingSizeInAxis(aAxisTracker.GetMainAxis());

    
    
    
    if (wrapThreshold != NS_UNCONSTRAINEDSIZE &&
        curLine->mItems.Length() > 1 && 
        wrapThreshold < (curLine->GetTotalOuterHypotheticalMainSize() +
                         itemOuterHypotheticalMainSize)) {
      
      
      curLine = aLines.AppendElement();
      
      
      
      item = nullptr;

      FlexLine& prevLine = aLines[aLines.Length() - 2];
      uint32_t itemIdxInPrevLine = prevLine.mItems.Length() - 1;
      FlexItem& itemToCopy = prevLine.mItems[itemIdxInPrevLine];

      
      curLine->mItems.AppendElement(itemToCopy);
      
      prevLine.mItems.RemoveElementAt(itemIdxInPrevLine);
    }

    curLine->AddToMainSizeTotals(itemInnerHypotheticalMainSize,
                                 itemOuterHypotheticalMainSize);

    
    if (!isSingleLine && childFrame->GetNextSibling() &&
        childFrame->StyleDisplay()->mBreakAfter) {
      curLine = aLines.AppendElement();
    }
  }

  return NS_OK;
}




nscoord
nsFlexContainerFrame::GetMainSizeFromReflowState(
  const nsHTMLReflowState& aReflowState,
  const FlexboxAxisTracker& aAxisTracker)
{
  if (IsAxisHorizontal(aAxisTracker.GetMainAxis())) {
    
    
    return aReflowState.ComputedWidth();
  }

  return GetEffectiveComputedHeight(aReflowState);
}



static nscoord
GetLargestLineMainSize(const nsTArray<FlexLine>& aLines)
{
  nscoord largestLineOuterSize = 0;
  for (uint32_t lineIdx = 0; lineIdx < aLines.Length(); lineIdx++) {
    largestLineOuterSize =
      std::max(largestLineOuterSize,
               aLines[lineIdx].GetTotalOuterHypotheticalMainSize());
  }
  return largestLineOuterSize;
}



static nscoord
ClampFlexContainerMainSize(const nsHTMLReflowState& aReflowState,
                           const FlexboxAxisTracker& aAxisTracker,
                           nscoord aUnclampedMainSize,
                           nscoord aAvailableHeightForContent,
                           const nsTArray<FlexLine>& aLines,
                           nsReflowStatus& aStatus)
{
  if (IsAxisHorizontal(aAxisTracker.GetMainAxis())) {
    
    
    
    return aUnclampedMainSize;
  }

  if (aUnclampedMainSize != NS_INTRINSICSIZE) {
    
    if (aAvailableHeightForContent == NS_UNCONSTRAINEDSIZE ||
        aUnclampedMainSize < aAvailableHeightForContent) {
      
      
      
      
      return aUnclampedMainSize;
    }

    
    
    
    
    
    
    
    NS_FRAME_SET_INCOMPLETE(aStatus);
    nscoord largestLineOuterSize = GetLargestLineMainSize(aLines);

    if (largestLineOuterSize <= aAvailableHeightForContent) {
      return aAvailableHeightForContent;
    }
    return std::min(aUnclampedMainSize, largestLineOuterSize);
  }

  
  
  
  
  nscoord largestLineOuterSize = GetLargestLineMainSize(aLines);
  return NS_CSS_MINMAX(largestLineOuterSize,
                       aReflowState.mComputedMinHeight,
                       aReflowState.mComputedMaxHeight);
}


static nscoord
SumLineCrossSizes(const nsTArray<FlexLine>& aLines)
{
  nscoord sum = 0;
  for (uint32_t lineIdx = 0; lineIdx < aLines.Length(); lineIdx++) {
    sum += aLines[lineIdx].GetLineCrossSize();
  }
  return sum;
}

nscoord
nsFlexContainerFrame::ComputeFlexContainerCrossSize(
  const nsHTMLReflowState& aReflowState,
  const FlexboxAxisTracker& aAxisTracker,
  const nsTArray<FlexLine>& aLines,
  nscoord aAvailableHeightForContent,
  bool* aIsDefinite,
  nsReflowStatus& aStatus)
{
  MOZ_ASSERT(aIsDefinite, "outparam pointer must be non-null"); 

  if (IsAxisHorizontal(aAxisTracker.GetCrossAxis())) {
    
    
    *aIsDefinite = true;
    return aReflowState.ComputedWidth();
  }

  nscoord effectiveComputedHeight = GetEffectiveComputedHeight(aReflowState);
  if (effectiveComputedHeight != NS_INTRINSICSIZE) {
    
    *aIsDefinite = true;
    if (aAvailableHeightForContent == NS_UNCONSTRAINEDSIZE ||
        effectiveComputedHeight < aAvailableHeightForContent) {
      
      
      
      
      return effectiveComputedHeight;
    }

    
    
    
    
    
    
    
    NS_FRAME_SET_INCOMPLETE(aStatus);
    nscoord sumOfLineCrossSizes = SumLineCrossSizes(aLines);
    if (sumOfLineCrossSizes <= aAvailableHeightForContent) {
      return aAvailableHeightForContent;
    }
    return std::min(effectiveComputedHeight, sumOfLineCrossSizes);
  }

  
  
  
  *aIsDefinite = false;
  return NS_CSS_MINMAX(SumLineCrossSizes(aLines),
                       aReflowState.mComputedMinHeight,
                       aReflowState.mComputedMaxHeight);
}

void
FlexLine::PositionItemsInMainAxis(uint8_t aJustifyContent,
                                  nscoord aContentBoxMainSize,
                                  const FlexboxAxisTracker& aAxisTracker)
{
  MainAxisPositionTracker mainAxisPosnTracker(aAxisTracker, mItems,
                                              aJustifyContent,
                                              aContentBoxMainSize);
  for (uint32_t i = 0; i < mItems.Length(); ++i) {
    FlexItem& item = mItems[i];

    nscoord itemMainBorderBoxSize =
      item.GetMainSize() +
      item.GetBorderPaddingSizeInAxis(mainAxisPosnTracker.GetAxis());

    
    mainAxisPosnTracker.ResolveAutoMarginsInMainAxis(item);

    
    
    mainAxisPosnTracker.EnterMargin(item.GetMargin());
    mainAxisPosnTracker.EnterChildFrame(itemMainBorderBoxSize);

    item.SetMainPosition(mainAxisPosnTracker.GetPosition());

    mainAxisPosnTracker.ExitChildFrame(itemMainBorderBoxSize);
    mainAxisPosnTracker.ExitMargin(item.GetMargin());
    mainAxisPosnTracker.TraversePackingSpace();
  }
}



static void
ResolveReflowedChildAscent(nsIFrame* aFrame,
                           nsHTMLReflowMetrics& aChildDesiredSize)
{
  if (aChildDesiredSize.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    
    if (!nsLayoutUtils::GetFirstLineBaseline(aFrame,
                                             &aChildDesiredSize.ascent)) {
      aChildDesiredSize.ascent = aFrame->GetBaseline();
    }
  }
}

nsresult
nsFlexContainerFrame::SizeItemInCrossAxis(
  nsPresContext* aPresContext,
  const FlexboxAxisTracker& aAxisTracker,
  nsHTMLReflowState& aChildReflowState,
  FlexItem& aItem)
{
  
  
  
  
  
  
  
  if (IsAxisHorizontal(aAxisTracker.GetCrossAxis())) {
    MOZ_ASSERT(aItem.GetAlignSelf() != NS_STYLE_ALIGN_ITEMS_BASELINE,
               "In vert flex container, we depend on FlexItem constructor to "
               "convert 'align-self: baseline' to 'align-self: flex-start'");
    aItem.SetCrossSize(aChildReflowState.ComputedWidth());
    return NS_OK;
  }

  MOZ_ASSERT(!aItem.HadMeasuringReflow(),
             "We shouldn't need more than one measuring reflow");

  if (aItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_STRETCH) {
    
    
    
    
    
    aChildReflowState.mFlags.mVResize = true;
  }
  nsHTMLReflowMetrics childDesiredSize;
  nsReflowStatus childReflowStatus;
  nsresult rv = ReflowChild(aItem.Frame(), aPresContext,
                            childDesiredSize, aChildReflowState,
                            0, 0, NS_FRAME_NO_MOVE_FRAME,
                            childReflowStatus);
  aItem.SetHadMeasuringReflow();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
             "We gave flex item unconstrained available height, so it "
             "should be complete");

  
  
  rv = FinishReflowChild(aItem.Frame(), aPresContext,
                         &aChildReflowState, childDesiredSize, 0, 0, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  
  
  
  
  
  
  nscoord crossAxisBorderPadding = aItem.GetBorderPadding().TopBottom();
  if (childDesiredSize.height < crossAxisBorderPadding) {
    
    
    
    
    
    
    NS_WARN_IF_FALSE(!aItem.Frame()->GetType(),
                     "Child should at least request space for border/padding");
    aItem.SetCrossSize(0);
  } else {
    
    aItem.SetCrossSize(childDesiredSize.height - crossAxisBorderPadding);
  }

  
  if (aItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE) {
    ResolveReflowedChildAscent(aItem.Frame(), childDesiredSize);
    aItem.SetAscent(childDesiredSize.ascent);
  }

  return NS_OK;
}

void
FlexLine::PositionItemsInCrossAxis(nscoord aLineStartPosition,
                                   const FlexboxAxisTracker& aAxisTracker)
{
  SingleLineCrossAxisPositionTracker lineCrossAxisPosnTracker(aAxisTracker);

  for (uint32_t i = 0; i < mItems.Length(); ++i) {
    FlexItem& item = mItems[i];
    
    
    item.ResolveStretchedCrossSize(mLineCrossSize, aAxisTracker);
    lineCrossAxisPosnTracker.ResolveAutoMarginsInCrossAxis(*this, item);

    
    nscoord itemCrossBorderBoxSize =
      item.GetCrossSize() +
      item.GetBorderPaddingSizeInAxis(aAxisTracker.GetCrossAxis());
    lineCrossAxisPosnTracker.EnterAlignPackingSpace(*this, item);
    lineCrossAxisPosnTracker.EnterMargin(item.GetMargin());
    lineCrossAxisPosnTracker.EnterChildFrame(itemCrossBorderBoxSize);

    item.SetCrossPosition(aLineStartPosition +
                          lineCrossAxisPosnTracker.GetPosition());

    
    lineCrossAxisPosnTracker.ResetPosition();
  }
}

NS_IMETHODIMP
nsFlexContainerFrame::Reflow(nsPresContext*           aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFlexContainerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
         ("Reflow() for nsFlexContainerFrame %p\n", this));

  if (IsFrameTreeTooDeep(aReflowState, aDesiredSize, aStatus)) {
    return NS_OK;
  }

  aStatus = NS_FRAME_COMPLETE;

  
  
  
  
  
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

  
  
  
  
  
  
  
  
  
  if (!mChildrenHaveBeenReordered) {
    mChildrenHaveBeenReordered =
      SortChildrenIfNeeded<IsOrderLEQ>();
  } else {
    SortChildrenIfNeeded<IsOrderLEQWithDOMFallback>();
  }

  const FlexboxAxisTracker axisTracker(this);

  nscoord contentBoxMainSize = GetMainSizeFromReflowState(aReflowState,
                                                          axisTracker);

  
  
  
  
  nscoord availableHeightForContent = aReflowState.availableHeight;
  if (availableHeightForContent != NS_UNCONSTRAINEDSIZE &&
      !(GetSkipSides() & (1 << NS_SIDE_TOP))) {
    availableHeightForContent -= aReflowState.mComputedBorderPadding.top;
    
    availableHeightForContent = std::max(availableHeightForContent, 0);
  }

  
  nsAutoTArray<FlexLine, 1> lines;
  nsresult rv = GenerateFlexLines(aPresContext, aReflowState,
                                  contentBoxMainSize, availableHeightForContent,
                                  axisTracker, lines);
  NS_ENSURE_SUCCESS(rv, rv);

  contentBoxMainSize =
    ClampFlexContainerMainSize(aReflowState, axisTracker,
                               contentBoxMainSize, availableHeightForContent,
                               lines, aStatus);

  for (uint32_t i = 0; i < lines.Length(); i++) {
    lines[i].ResolveFlexibleLengths(contentBoxMainSize);
  }

  
  
  
  for (uint32_t lineIdx = 0; lineIdx < lines.Length(); ++lineIdx) {
    FlexLine& line = lines[lineIdx];
    for (uint32_t i = 0; i < line.mItems.Length(); ++i) {
      FlexItem& curItem = line.mItems[i];

      
      
      if (!curItem.IsStretched()) {
        nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                           curItem.Frame(),
                                           nsSize(aReflowState.ComputedWidth(),
                                                  NS_UNCONSTRAINEDSIZE));
        
        if (IsAxisHorizontal(axisTracker.GetMainAxis())) {
          childReflowState.SetComputedWidth(curItem.GetMainSize());
        } else {
          childReflowState.SetComputedHeight(curItem.GetMainSize());
        }

        nsresult rv = SizeItemInCrossAxis(aPresContext, axisTracker,
                                          childReflowState, curItem);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  
  
  for (uint32_t lineIdx = 0; lineIdx < lines.Length(); ++lineIdx) {
    lines[lineIdx].ComputeCrossSizeAndBaseline(axisTracker);
  }

  bool isCrossSizeDefinite;
  const nscoord contentBoxCrossSize =
    ComputeFlexContainerCrossSize(aReflowState, axisTracker,
                                  lines,
                                  availableHeightForContent,
                                  &isCrossSizeDefinite, aStatus);

  
  
  CrossAxisPositionTracker
    crossAxisPosnTracker(lines, aReflowState.mStylePosition->mAlignContent,
                         contentBoxCrossSize, isCrossSizeDefinite, axisTracker);

  
  
  nscoord flexContainerAscent;
  nscoord firstLineBaselineOffset = lines[0].GetBaselineOffsetFromCrossStart();
  if (firstLineBaselineOffset == nscoord_MIN) {
    
    
    flexContainerAscent = nscoord_MIN;
  } else {
    
    
    
    nscoord firstLineBaselineOffsetWRTContainer =
      firstLineBaselineOffset + crossAxisPosnTracker.GetPosition();

    
    
    
    flexContainerAscent = aReflowState.mComputedBorderPadding.top +
      PhysicalPosFromLogicalPos(firstLineBaselineOffsetWRTContainer,
                                contentBoxCrossSize,
                                axisTracker.GetCrossAxis());
  }

  for (uint32_t lineIdx = 0; lineIdx < lines.Length(); ++lineIdx) {
    FlexLine& line = lines[lineIdx];

    
    
    line.PositionItemsInMainAxis(aReflowState.mStylePosition->mJustifyContent,
                                 contentBoxMainSize,
                                 axisTracker);

    
    
    line.PositionItemsInCrossAxis(crossAxisPosnTracker.GetPosition(),
                                  axisTracker);
    crossAxisPosnTracker.TraverseLine(line);
    crossAxisPosnTracker.TraversePackingSpace();
  }

  
  
  
  nsMargin containerBorderPadding(aReflowState.mComputedBorderPadding);
  ApplySkipSides(containerBorderPadding, &aReflowState);
  const nsPoint containerContentBoxOrigin(containerBorderPadding.left,
                                          containerBorderPadding.top);

  
  
  for (uint32_t lineIdx = 0; lineIdx < lines.Length(); ++lineIdx) {
    FlexLine& line = lines[lineIdx];
    for (uint32_t i = 0; i < line.mItems.Length(); ++i) {
      FlexItem& curItem = line.mItems[i];

      nsPoint physicalPosn = axisTracker.PhysicalPointFromLogicalPoint(
                               curItem.GetMainPosition(),
                               curItem.GetCrossPosition(),
                               contentBoxMainSize,
                               contentBoxCrossSize);
      
      
      physicalPosn += containerContentBoxOrigin;

      nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                         curItem.Frame(),
                                         nsSize(aReflowState.ComputedWidth(),
                                                NS_UNCONSTRAINEDSIZE));

      
      
      bool didOverrideComputedWidth = false;
      bool didOverrideComputedHeight = false;

      
      if (IsAxisHorizontal(axisTracker.GetMainAxis())) {
        childReflowState.SetComputedWidth(curItem.GetMainSize());
        didOverrideComputedWidth = true;
      } else {
        childReflowState.SetComputedHeight(curItem.GetMainSize());
        didOverrideComputedHeight = true;
      }

      
      if (curItem.IsStretched()) {
        MOZ_ASSERT(curItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_STRETCH,
                   "stretched item w/o 'align-self: stretch'?");
        if (IsAxisHorizontal(axisTracker.GetCrossAxis())) {
          childReflowState.SetComputedWidth(curItem.GetCrossSize());
          didOverrideComputedWidth = true;
        } else {
          
          curItem.Frame()->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
          childReflowState.SetComputedHeight(curItem.GetCrossSize());
          didOverrideComputedHeight = true;
        }
      }

      
      
      

      
      
      
      if (curItem.HadMeasuringReflow()) {
        if (didOverrideComputedWidth) {
          
          
          
          
          childReflowState.mFlags.mHResize = true;
        }
        if (didOverrideComputedHeight) {
          childReflowState.mFlags.mVResize = true;
        }
      }
      
      
      

      nsHTMLReflowMetrics childDesiredSize;
      nsReflowStatus childReflowStatus;
      nsresult rv = ReflowChild(curItem.Frame(), aPresContext,
                                childDesiredSize, childReflowState,
                                physicalPosn.x, physicalPosn.y,
                                0, childReflowStatus);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
                 "We gave flex item unconstrained available height, so it "
                 "should be complete");

      childReflowState.ApplyRelativePositioning(&physicalPosn);

      rv = FinishReflowChild(curItem.Frame(), aPresContext,
                             &childReflowState, childDesiredSize,
                             physicalPosn.x, physicalPosn.y, 0);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      if (lineIdx == 0 && i == 0 && flexContainerAscent == nscoord_MIN) {
        ResolveReflowedChildAscent(curItem.Frame(), childDesiredSize);

        
        
        
        flexContainerAscent = curItem.Frame()->GetNormalPosition().y +
          childDesiredSize.ascent;
      }
    }
  }

  nsSize desiredContentBoxSize =
    axisTracker.PhysicalSizeFromLogicalSizes(contentBoxMainSize,
                                             contentBoxCrossSize);

  aDesiredSize.width = desiredContentBoxSize.width +
    containerBorderPadding.LeftRight();
  
  aDesiredSize.height = desiredContentBoxSize.height +
    containerBorderPadding.top;

  if (flexContainerAscent == nscoord_MIN) {
    
    
    
    
    NS_WARN_IF_FALSE(lines[0].mItems.IsEmpty(),
                     "Have flex items but didn't get an ascent - that's odd "
                     "(or there are just gigantic sizes involved)");
    
    flexContainerAscent = aDesiredSize.height;
  }
  aDesiredSize.ascent = flexContainerAscent;

  
  
  
  
  
  
  
  if (NS_FRAME_IS_COMPLETE(aStatus)) {
    
    
    
    
    nscoord desiredHeightWithBottomBP =
      aDesiredSize.height + aReflowState.mComputedBorderPadding.bottom;

    if (aReflowState.availableHeight == NS_UNCONSTRAINEDSIZE ||
        aDesiredSize.height == 0 ||
        desiredHeightWithBottomBP <= aReflowState.availableHeight ||
        aReflowState.ComputedHeight() == NS_INTRINSICSIZE) {
      
      aDesiredSize.height = desiredHeightWithBottomBP;
    } else {
      
      NS_FRAME_SET_INCOMPLETE(aStatus);
    }
  }

  
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, e.get());
  }

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize,
                                 aReflowState, aStatus);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize)
  return NS_OK;
}

 nscoord
nsFlexContainerFrame::GetMinWidth(nsRenderingContext* aRenderingContext)
{
  FlexboxAxisTracker axisTracker(this);

  nscoord minWidth = 0;
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nscoord childMinWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, e.get(),
                                           nsLayoutUtils::MIN_WIDTH);
    
    
    
    
    if (IsAxisHorizontal(axisTracker.GetMainAxis()) &&
        NS_STYLE_FLEX_WRAP_NOWRAP == StylePosition()->mFlexWrap) {
      minWidth += childMinWidth;
    } else {
      minWidth = std::max(minWidth, childMinWidth);
    }
  }
  return minWidth;
}

 nscoord
nsFlexContainerFrame::GetPrefWidth(nsRenderingContext* aRenderingContext)
{
  
  
  
  
  
  FlexboxAxisTracker axisTracker(this);

  nscoord prefWidth = 0;
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nscoord childPrefWidth =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, e.get(),
                                           nsLayoutUtils::PREF_WIDTH);
    if (IsAxisHorizontal(axisTracker.GetMainAxis())) {
      prefWidth += childPrefWidth;
    } else {
      prefWidth = std::max(prefWidth, childPrefWidth);
    }
  }
  return prefWidth;
}
