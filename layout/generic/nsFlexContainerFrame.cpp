








#include "nsFlexContainerFrame.h"
#include "nsContentUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "prlog.h"
#include <algorithm>

using namespace mozilla::css;

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


NS_STACK_CLASS class FlexboxAxisTracker {
public:
  FlexboxAxisTracker(nsFlexContainerFrame* aFlexContainerFrame);

  
  AxisOrientationType GetMainAxis() const  { return mMainAxis;  }
  AxisOrientationType GetCrossAxis() const { return mCrossAxis; }

  nscoord GetMainComponent(const nsSize& aSize) const {
    return IsAxisHorizontal(mMainAxis) ?
      aSize.width : aSize.height;
  }
  int32_t GetMainComponent(const nsIntSize& aIntSize) const {
    return IsAxisHorizontal(mMainAxis) ?
      aIntSize.width : aIntSize.height;
  }
  nscoord GetMainComponent(const nsHTMLReflowMetrics& aMetrics) const {
    return IsAxisHorizontal(mMainAxis) ?
      aMetrics.width : aMetrics.height;
  }

  nscoord GetCrossComponent(const nsSize& aSize) const {
    return IsAxisHorizontal(mCrossAxis) ?
      aSize.width : aSize.height;
  }
  int32_t GetCrossComponent(const nsIntSize& aIntSize) const {
    return IsAxisHorizontal(mCrossAxis) ?
      aIntSize.width : aIntSize.height;
  }
  nscoord GetCrossComponent(const nsHTMLReflowMetrics& aMetrics) const {
    return IsAxisHorizontal(mCrossAxis) ?
      aMetrics.width : aMetrics.height;
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

  nsPoint PhysicalPositionFromLogicalPosition(nscoord aMainPosn,
                                              nscoord aCrossPosn) const {
    return IsAxisHorizontal(mMainAxis) ?
      nsPoint(aMainPosn, aCrossPosn) :
      nsPoint(aCrossPosn, aMainPosn);
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

  nscoord GetAscent() const        { return mAscent; }

  float GetShareOfFlexWeightSoFar() const { return mShareOfFlexWeightSoFar; }

  bool IsFrozen() const            { return mIsFrozen; }

  bool HadMinViolation() const     { return mHadMinViolation; }
  bool HadMaxViolation() const     { return mHadMaxViolation; }

  
  
  bool HadMeasuringReflow() const  { return mHadMeasuringReflow; }

  
  
  
  bool IsStretched() const         { return mIsStretched; }

  uint8_t GetAlignSelf() const     { return mAlignSelf; }

  
  
  
  
  float GetFlexWeightToUse(bool aHavePositiveFreeSpace)
  {
    if (IsFrozen()) {
      return 0.0f;
    }

    return aHavePositiveFreeSpace ?
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
    MOZ_ASSERT(mIsFrozen, "main size should be resolved before this");
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

  uint32_t GetNumAutoMarginsInAxis(AxisOrientationType aAxis) const;

protected:
  
  nsIFrame* const mFrame;

  
  const float mFlexGrow;
  const float mFlexShrink;

  const nsMargin mBorderPadding;
  nsMargin mMargin; 

  const nscoord mFlexBaseSize;

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









static nsIContent*
GetContentForComparison(const nsIFrame* aFrame)
{
  MOZ_ASSERT(aFrame, "null frame passed to GetContentForComparison()");
  MOZ_ASSERT(aFrame->IsFlexItem(), "only intended for flex items");

  while (true) {
    nsIAtom* pseudoTag = aFrame->StyleContext()->GetPseudo();

    
    if (!pseudoTag ||                                 
        !nsCSSAnonBoxes::IsAnonBox(pseudoTag) ||      
        pseudoTag == nsCSSAnonBoxes::mozNonElement) { 
      return aFrame->GetContent();
    }

    
    aFrame = aFrame->GetFirstPrincipalChild();
    MOZ_ASSERT(aFrame, "why do we have an anonymous box without any children?");
  }
}
















bool
IsOrderLEQWithDOMFallback(nsIFrame* aFrame1,
                          nsIFrame* aFrame2)
{
  if (aFrame1 == aFrame2) {
    
    
    NS_ERROR("Why are we checking if a frame is LEQ itself?");
    return true;
  }

  int32_t order1 = aFrame1->StylePosition()->mOrder;
  int32_t order2 = aFrame2->StylePosition()->mOrder;

  if (order1 != order2) {
    return order1 < order2;
  }

  
  nsIContent* content1 = GetContentForComparison(aFrame1);
  nsIContent* content2 = GetContentForComparison(aFrame2);
  MOZ_ASSERT(content1 != content2,
             "Two different flex items are using the same nsIContent node for "
             "comparison, so we may be sorting them in an arbitrary order");

  return nsContentUtils::PositionIsBefore(content1, content2);
}













bool
IsOrderLEQ(nsIFrame* aFrame1,
           nsIFrame* aFrame2)
{
  int32_t order1 = aFrame1->StylePosition()->mOrder;
  int32_t order2 = aFrame2->StylePosition()->mOrder;

  return order1 <= order2;
}

bool
nsFlexContainerFrame::IsHorizontal()
{
  const FlexboxAxisTracker axisTracker(this);
  return IsAxisHorizontal(axisTracker.GetMainAxis());
}

nsresult
nsFlexContainerFrame::AppendFlexItemForChild(
  nsPresContext* aPresContext,
  nsIFrame*      aChildFrame,
  const nsHTMLReflowState& aParentReflowState,
  const FlexboxAxisTracker& aAxisTracker,
  nsTArray<FlexItem>& aFlexItems)
{
  
  
  
  nsHTMLReflowState childRS(aPresContext, aParentReflowState, aChildFrame,
                            nsSize(aParentReflowState.ComputedWidth(),
                                   aParentReflowState.ComputedHeight()));

  
  
  const nsStylePosition* stylePos = aChildFrame->StylePosition();
  float flexGrow   = stylePos->mFlexGrow;
  float flexShrink = stylePos->mFlexShrink;

  
  
  nscoord flexBaseSize =
    aAxisTracker.GetMainComponent(nsSize(childRS.ComputedWidth(),
                                         childRS.ComputedHeight()));
  nscoord mainMinSize =
    aAxisTracker.GetMainComponent(nsSize(childRS.mComputedMinWidth,
                                         childRS.mComputedMinHeight));
  nscoord mainMaxSize =
    aAxisTracker.GetMainComponent(nsSize(childRS.mComputedMaxWidth,
                                         childRS.mComputedMaxHeight));
  
  MOZ_ASSERT(mainMinSize <= mainMaxSize, "min size is larger than max size");

  
  
  
  
  
  
  
  bool needToMeasureMaxContentHeight = false;
  if (!IsAxisHorizontal(aAxisTracker.GetMainAxis())) {
    bool isMainSizeAuto = (NS_UNCONSTRAINEDSIZE == flexBaseSize);
    bool isMainMinSizeAuto =
      (eStyleUnit_Auto ==
       aChildFrame->StylePosition()->mMinHeight.GetUnit());

    needToMeasureMaxContentHeight = isMainSizeAuto || isMainMinSizeAuto;

    if (needToMeasureMaxContentHeight) {
      
      
      
      nsHTMLReflowState
        childRSForMeasuringHeight(aPresContext, aParentReflowState,
                                  aChildFrame,
                                  nsSize(aParentReflowState.ComputedWidth(),
                                         NS_UNCONSTRAINEDSIZE),
                                  -1, -1, false);
      childRSForMeasuringHeight.mFlags.mIsFlexContainerMeasuringHeight = true;
      childRSForMeasuringHeight.Init(aPresContext);

      
      
      
      
      
      
      
      
      
      
      if (flexGrow != 0.0f || flexShrink != 0.0f ||  
          !isMainSizeAuto) {  
        childRSForMeasuringHeight.mFlags.mVResize = true;
      }

      nsHTMLReflowMetrics childDesiredSize;
      nsReflowStatus childReflowStatus;
      nsresult rv = ReflowChild(aChildFrame, aPresContext,
                                childDesiredSize, childRSForMeasuringHeight,
                                0, 0, NS_FRAME_NO_MOVE_FRAME,
                                childReflowStatus);
      NS_ENSURE_SUCCESS(rv, rv);

      MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
                 "We gave flex item unconstrained available height, so it "
                 "should be complete");

      rv = FinishReflowChild(aChildFrame, aPresContext,
                             &childRSForMeasuringHeight, childDesiredSize,
                             0, 0, 0);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      nscoord childDesiredHeight = childDesiredSize.height -
        childRS.mComputedBorderPadding.TopBottom();
      childDesiredHeight = std::max(0, childDesiredHeight);

      if (isMainSizeAuto) {
        flexBaseSize = childDesiredHeight;
      }
      if (isMainMinSizeAuto) {
        mainMinSize = childDesiredHeight;
        mainMaxSize = std::max(mainMaxSize, mainMinSize);
      }
    }
  }

  
  

  nscoord crossMinSize =
    aAxisTracker.GetCrossComponent(nsSize(childRS.mComputedMinWidth,
                                          childRS.mComputedMinHeight));
  nscoord crossMaxSize =
    aAxisTracker.GetCrossComponent(nsSize(childRS.mComputedMaxWidth,
                                          childRS.mComputedMaxHeight));

  
  
  
  
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

  aFlexItems.AppendElement(FlexItem(aChildFrame,
                                    flexGrow, flexShrink, flexBaseSize,
                                    mainMinSize, mainMaxSize,
                                    crossMinSize, crossMaxSize,
                                    childRS.mComputedMargin,
                                    childRS.mComputedBorderPadding,
                                    aAxisTracker));

  
  
  
  if (isFixedSizeWidget || (flexGrow == 0.0f && flexShrink == 0.0f)) {
    aFlexItems.LastElement().Freeze();
  }

  
  
  if (needToMeasureMaxContentHeight) {
    aFlexItems.LastElement().SetHadMeasuringReflow();
  }

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
    mFlexBaseSize(aFlexBaseSize),
    mMainMinSize(aMainMinSize),
    mMainMaxSize(aMainMaxSize),
    mCrossMinSize(aCrossMinSize),
    mCrossMaxSize(aCrossMaxSize),
    
    
    mMainSize(NS_CSS_MINMAX(aFlexBaseSize, aMainMinSize, aMainMaxSize)),
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





NS_STACK_CLASS
class PositionTracker {
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


NS_STACK_CLASS
class MainAxisPositionTracker : public PositionTracker {
public:
  MainAxisPositionTracker(nsFlexContainerFrame* aFlexContainerFrame,
                          const FlexboxAxisTracker& aAxisTracker,
                          const nsHTMLReflowState& aReflowState,
                          const nsTArray<FlexItem>& aItems);

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



class SingleLineCrossAxisPositionTracker;
NS_STACK_CLASS
class CrossAxisPositionTracker : public PositionTracker {
public:
  CrossAxisPositionTracker(nsFlexContainerFrame* aFlexContainerFrame,
                           const FlexboxAxisTracker& aAxisTracker,
                           const nsHTMLReflowState& aReflowState);

  
  
  
  
  
};



NS_STACK_CLASS
class SingleLineCrossAxisPositionTracker : public PositionTracker {
public:
  SingleLineCrossAxisPositionTracker(nsFlexContainerFrame* aFlexContainerFrame,
                                     const FlexboxAxisTracker& aAxisTracker,
                                     const nsTArray<FlexItem>& aItems);

  void ComputeLineCrossSize(const nsTArray<FlexItem>& aItems);
  inline nscoord GetLineCrossSize() const { return mLineCrossSize; }

  
  
  
  
  inline void SetLineCrossSize(nscoord aNewLineCrossSize) {
    mLineCrossSize = aNewLineCrossSize;
  }

  void ResolveStretchedCrossSize(FlexItem& aItem);
  void ResolveAutoMarginsInCrossAxis(FlexItem& aItem);

  void EnterAlignPackingSpace(const FlexItem& aItem);

  
  inline void ResetPosition() { mPosition = 0; }

private:
  
  
  nscoord GetBaselineOffsetFromCrossStart(const FlexItem& aItem) const;

  nscoord mLineCrossSize;

  
  
  
  
  nscoord mCrossStartToFurthestBaseline;
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
  if (nsLayoutUtils::IsFrameListSorted<IsLessThanOrEqual>(mFrames)) {
    return false;
  }

  nsLayoutUtils::SortFrameList<IsLessThanOrEqual>(mFrames);
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
  return 0;
}

void
nsFlexContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists)
{
  MOZ_ASSERT(nsLayoutUtils::IsFrameListSorted<IsOrderLEQWithDOMFallback>(mFrames),
             "Frame list should've been sorted in reflow");

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




static bool
ShouldUseFlexGrow(nscoord aTotalFreeSpace,
                  nsTArray<FlexItem>& aItems)
{
  
  
  
  for (uint32_t i = 0; i < aItems.Length(); i++) {
    aTotalFreeSpace -= aItems[i].GetMainSize();
    if (aTotalFreeSpace <= 0) {
      return false;
    }
  }
  MOZ_ASSERT(aTotalFreeSpace > 0,
             "if we used up all the space, should've already returned");
  return true;
}







void
nsFlexContainerFrame::ResolveFlexibleLengths(
  const FlexboxAxisTracker& aAxisTracker,
  nscoord aFlexContainerMainSize,
  nsTArray<FlexItem>& aItems)
{
  PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG, ("ResolveFlexibleLengths\n"));
  if (aItems.IsEmpty()) {
    return;
  }

  
  
  
  nscoord spaceAvailableForFlexItemsContentBoxes = aFlexContainerMainSize;
  for (uint32_t i = 0; i < aItems.Length(); i++) {
    spaceAvailableForFlexItemsContentBoxes -=
      aItems[i].GetMarginBorderPaddingSizeInAxis(aAxisTracker.GetMainAxis());
  }

  
  bool havePositiveFreeSpace =
    ShouldUseFlexGrow(spaceAvailableForFlexItemsContentBoxes, aItems);

  
  
  
  
  
  
  for (uint32_t iterationCounter = 0;
       iterationCounter < aItems.Length(); iterationCounter++) {
    
    
    
    
    nscoord availableFreeSpace = spaceAvailableForFlexItemsContentBoxes;
    for (uint32_t i = 0; i < aItems.Length(); i++) {
      FlexItem& item = aItems[i];
      if (!item.IsFrozen()) {
        item.SetMainSize(item.GetFlexBaseSize());
      }
      availableFreeSpace -= item.GetMainSize();
    }

    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" available free space = %d\n", availableFreeSpace));

    
    
    if ((availableFreeSpace > 0 && havePositiveFreeSpace) ||
        (availableFreeSpace < 0 && !havePositiveFreeSpace)) {

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      float runningFlexWeightSum = 0.0f;
      float largestFlexWeight = 0.0f;
      uint32_t numItemsWithLargestFlexWeight = 0;
      for (uint32_t i = 0; i < aItems.Length(); i++) {
        FlexItem& item = aItems[i];
        float curFlexWeight = item.GetFlexWeightToUse(havePositiveFreeSpace);
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
        for (uint32_t i = aItems.Length() - 1; i < aItems.Length(); --i) {
          FlexItem& item = aItems[i];

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
            } else if (item.GetFlexWeightToUse(havePositiveFreeSpace) ==
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

    for (uint32_t i = 0; i < aItems.Length(); i++) {
      FlexItem& item = aItems[i];
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

    FreezeOrRestoreEachFlexibleSize(totalViolation, aItems);

    PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG,
           (" Total violation: %d\n", totalViolation));

    if (totalViolation == 0) {
      break;
    }
  }

  
#ifdef DEBUG
  for (uint32_t i = 0; i < aItems.Length(); ++i) {
    MOZ_ASSERT(aItems[i].IsFrozen(),
               "All flexible lengths should've been resolved");
  }
#endif 
}

MainAxisPositionTracker::
  MainAxisPositionTracker(nsFlexContainerFrame* aFlexContainerFrame,
                          const FlexboxAxisTracker& aAxisTracker,
                          const nsHTMLReflowState& aReflowState,
                          const nsTArray<FlexItem>& aItems)
  : PositionTracker(aAxisTracker.GetMainAxis()),
    mNumAutoMarginsInMainAxis(0),
    mNumPackingSpacesRemaining(0)
{
  MOZ_ASSERT(aReflowState.frame == aFlexContainerFrame,
             "Expecting the reflow state for the flex container frame");

  
  
  EnterMargin(aReflowState.mComputedBorderPadding);

  
  
  
  
  
  mPackingSpaceRemaining =
    aAxisTracker.GetMainComponent(nsSize(aReflowState.ComputedWidth(),
                                         aReflowState.ComputedHeight()));
  if (mPackingSpaceRemaining == NS_UNCONSTRAINEDSIZE) {
    mPackingSpaceRemaining = 0;
  } else {
    for (uint32_t i = 0; i < aItems.Length(); i++) {
      nscoord itemMarginBoxMainSize =
        aItems[i].GetMainSize() +
        aItems[i].GetMarginBorderPaddingSizeInAxis(aAxisTracker.GetMainAxis());
      mPackingSpaceRemaining -= itemMarginBoxMainSize;
    }
  }

  if (mPackingSpaceRemaining > 0) {
    for (uint32_t i = 0; i < aItems.Length(); i++) {
      mNumAutoMarginsInMainAxis += aItems[i].GetNumAutoMarginsInAxis(mAxis);
    }
  }

  mJustifyContent = aFlexContainerFrame->StylePosition()->mJustifyContent;
  
  
  
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
        MOZ_NOT_REACHED("Unexpected justify-content value");
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
  CrossAxisPositionTracker(nsFlexContainerFrame* aFlexContainerFrame,
                           const FlexboxAxisTracker& aAxisTracker,
                           const nsHTMLReflowState& aReflowState)
    : PositionTracker(aAxisTracker.GetCrossAxis())
{
  
  EnterMargin(aReflowState.mComputedBorderPadding);
}

SingleLineCrossAxisPositionTracker::
  SingleLineCrossAxisPositionTracker(nsFlexContainerFrame* aFlexContainerFrame,
                                     const FlexboxAxisTracker& aAxisTracker,
                                     const nsTArray<FlexItem>& aItems)
  : PositionTracker(aAxisTracker.GetCrossAxis()),
    mLineCrossSize(0),
    mCrossStartToFurthestBaseline(nscoord_MIN) 
                                               
{
}

void
SingleLineCrossAxisPositionTracker::
  ComputeLineCrossSize(const nsTArray<FlexItem>& aItems)
{
  
  
  
  MOZ_ASSERT(mCrossStartToFurthestBaseline == nscoord_MIN,
             "Computing largest baseline offset more than once");

  nscoord crossEndToFurthestBaseline = nscoord_MIN;
  nscoord largestOuterCrossSize = 0;
  for (uint32_t i = 0; i < aItems.Length(); ++i) {
    const FlexItem& curItem = aItems[i];
    nscoord curOuterCrossSize = curItem.GetCrossSize() +
      curItem.GetMarginBorderPaddingSizeInAxis(mAxis);

    if (curItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE &&
        curItem.GetNumAutoMarginsInAxis(mAxis) == 0) {
      
      
      
      
      MOZ_ASSERT(mAxis == eAxis_TB,
                 "Only expecting to do baseline-alignment in horizontal "
                 "flex containers, with top-to-bottom cross axis");

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      

      nscoord crossStartToBaseline = GetBaselineOffsetFromCrossStart(curItem);
      nscoord crossEndToBaseline = curOuterCrossSize - crossStartToBaseline;

      
      
      
      mCrossStartToFurthestBaseline = std::max(mCrossStartToFurthestBaseline,
                                               crossStartToBaseline);
      crossEndToFurthestBaseline = std::max(crossEndToFurthestBaseline,
                                            crossEndToBaseline);
    } else {
      largestOuterCrossSize = std::max(largestOuterCrossSize, curOuterCrossSize);
    }
  }

  
  
  
  
  
  mLineCrossSize = std::max(mCrossStartToFurthestBaseline +
                            crossEndToFurthestBaseline,
                            largestOuterCrossSize);
}

nscoord
SingleLineCrossAxisPositionTracker::
  GetBaselineOffsetFromCrossStart(const FlexItem& aItem) const
{
  Side crossStartSide = kAxisOrientationToSidesMap[mAxis][eAxisEdge_Start];

  
  
  
  return NSCoordSaturatingAdd(aItem.GetAscent(),
                              aItem.GetMarginComponentForSide(crossStartSide));
}

void
SingleLineCrossAxisPositionTracker::
  ResolveStretchedCrossSize(FlexItem& aItem)
{
  
  
  
  if (aItem.GetAlignSelf() != NS_STYLE_ALIGN_ITEMS_STRETCH ||
      aItem.GetNumAutoMarginsInAxis(mAxis) != 0 ||
      GetSizePropertyForAxis(aItem.Frame(), mAxis).GetUnit() !=
        eStyleUnit_Auto) {
    return;
  }

  
  
  nscoord stretchedSize = mLineCrossSize -
    aItem.GetMarginBorderPaddingSizeInAxis(mAxis);

  stretchedSize = NS_CSS_MINMAX(stretchedSize,
                                aItem.GetCrossMinSize(),
                                aItem.GetCrossMaxSize());

  
  
  aItem.SetCrossSize(stretchedSize);
  aItem.SetIsStretched();
}

void
SingleLineCrossAxisPositionTracker::
  ResolveAutoMarginsInCrossAxis(FlexItem& aItem)
{
  
  
  nscoord spaceForAutoMargins = mLineCrossSize -
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
  EnterAlignPackingSpace(const FlexItem& aItem)
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
        mLineCrossSize -
        (aItem.GetCrossSize() +
         aItem.GetMarginBorderPaddingSizeInAxis(mAxis));
      break;
    case NS_STYLE_ALIGN_ITEMS_CENTER:
      
      mPosition +=
        (mLineCrossSize -
         (aItem.GetCrossSize() +
          aItem.GetMarginBorderPaddingSizeInAxis(mAxis))) / 2;
      break;
    case NS_STYLE_ALIGN_ITEMS_BASELINE:
      NS_WARN_IF_FALSE(mCrossStartToFurthestBaseline != nscoord_MIN,
                       "using uninitialized baseline offset (or working with "
                       "content that has bogus huge values)");
      MOZ_ASSERT(mCrossStartToFurthestBaseline >=
                 GetBaselineOffsetFromCrossStart(aItem),
                 "failed at finding largest ascent");

      
      
      mPosition += (mCrossStartToFurthestBaseline -
                    GetBaselineOffsetFromCrossStart(aItem));
      break;
    default:
      NS_NOTREACHED("Unexpected align-self value");
      break;
  }
}

FlexboxAxisTracker::FlexboxAxisTracker(nsFlexContainerFrame* aFlexContainerFrame)
{
  uint32_t flexDirection =
    aFlexContainerFrame->StylePosition()->mFlexDirection;
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
      MOZ_NOT_REACHED("Unexpected computed value for 'flex-flow' property");
      mMainAxis = inlineDimension;
      break;
  }

  
  
  
  if (flexDirection == NS_STYLE_FLEX_DIRECTION_COLUMN ||
      flexDirection == NS_STYLE_FLEX_DIRECTION_COLUMN_REVERSE) {
    mCrossAxis = inlineDimension;
  } else {
    mCrossAxis = blockDimension;
  }
      
  
  
  MOZ_ASSERT(IsAxisHorizontal(mMainAxis) != IsAxisHorizontal(mCrossAxis),
             "main & cross axes should be in different dimensions");


  
  
  
  
  
  
  MOZ_ASSERT(mCrossAxis != eAxis_BT, "Not expecting bottom-to-top cross axis");
}

nsresult
nsFlexContainerFrame::GenerateFlexItems(
  nsPresContext* aPresContext,
  const nsHTMLReflowState& aReflowState,
  const FlexboxAxisTracker& aAxisTracker,
  nsTArray<FlexItem>& aFlexItems)
{
  MOZ_ASSERT(aFlexItems.IsEmpty(), "Expecting outparam to start out empty");

  
  
  aFlexItems.SetCapacity(mFrames.GetLength());
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nsresult rv = AppendFlexItemForChild(aPresContext, e.get(),
                                         aReflowState, aAxisTracker,
                                         aFlexItems);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}


nscoord
nsFlexContainerFrame::ComputeFlexContainerMainSize(
  const nsHTMLReflowState& aReflowState,
  const FlexboxAxisTracker& aAxisTracker,
  const nsTArray<FlexItem>& aItems)
{
  
  nscoord mainSize =
    aAxisTracker.GetMainComponent(nsSize(aReflowState.ComputedWidth(),
                                         aReflowState.ComputedHeight()));
  if (mainSize != NS_UNCONSTRAINEDSIZE) {
    return mainSize;
  }

  NS_WARN_IF_FALSE(!IsAxisHorizontal(aAxisTracker.GetMainAxis()),
                   "Computed width should always be constrained, so horizontal "
                   "flex containers should have a constrained main-size");

  
  
  mainSize = 0;
  for (uint32_t i = 0; i < aItems.Length(); ++i) {
    mainSize +=
      aItems[i].GetMainSize() +
      aItems[i].GetMarginBorderPaddingSizeInAxis(aAxisTracker.GetMainAxis());
  }

  nscoord minMainSize =
    aAxisTracker.GetMainComponent(nsSize(aReflowState.mComputedMinWidth,
                                         aReflowState.mComputedMinHeight));
  nscoord maxMainSize =
    aAxisTracker.GetMainComponent(nsSize(aReflowState.mComputedMaxWidth,
                                         aReflowState.mComputedMaxHeight));

  return NS_CSS_MINMAX(mainSize, minMainSize, maxMainSize);
}

void
nsFlexContainerFrame::PositionItemInMainAxis(
  MainAxisPositionTracker& aMainAxisPosnTracker,
  FlexItem& aItem)
{
  nscoord itemMainBorderBoxSize =
    aItem.GetMainSize() +
    aItem.GetBorderPaddingSizeInAxis(aMainAxisPosnTracker.GetAxis());

  
  aMainAxisPosnTracker.ResolveAutoMarginsInMainAxis(aItem);

  
  
  aMainAxisPosnTracker.EnterMargin(aItem.GetMargin());
  aMainAxisPosnTracker.EnterChildFrame(itemMainBorderBoxSize);

  aItem.SetMainPosition(aMainAxisPosnTracker.GetPosition());

  aMainAxisPosnTracker.ExitChildFrame(itemMainBorderBoxSize);
  aMainAxisPosnTracker.ExitMargin(aItem.GetMargin());
  aMainAxisPosnTracker.TraversePackingSpace();
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

  
  

  
  
  MOZ_ASSERT(childDesiredSize.height >=
             aItem.GetBorderPaddingSizeInAxis(aAxisTracker.GetCrossAxis()),
             "Child should ask for at least enough space for border/padding");
  nscoord crossSize =
    aAxisTracker.GetCrossComponent(childDesiredSize) -
    aItem.GetBorderPaddingSizeInAxis(aAxisTracker.GetCrossAxis());
  aItem.SetCrossSize(crossSize);

  
  if (aItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE) {
    if (childDesiredSize.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
      
      if (!nsLayoutUtils::GetFirstLineBaseline(aItem.Frame(),
                                               &childDesiredSize.ascent)) {
        childDesiredSize.ascent = aItem.Frame()->GetBaseline();
      }
    }
    aItem.SetAscent(childDesiredSize.ascent);
  }

  return NS_OK;
}

void
nsFlexContainerFrame::PositionItemInCrossAxis(
  nscoord aLineStartPosition,
  SingleLineCrossAxisPositionTracker& aLineCrossAxisPosnTracker,
  FlexItem& aItem)
{
  MOZ_ASSERT(aLineCrossAxisPosnTracker.GetPosition() == 0,
             "per-line cross-axis position tracker wasn't correctly reset");

  
  aLineCrossAxisPosnTracker.ResolveStretchedCrossSize(aItem);
  aLineCrossAxisPosnTracker.ResolveAutoMarginsInCrossAxis(aItem);

  
  nscoord itemCrossBorderBoxSize =
    aItem.GetCrossSize() +
    aItem.GetBorderPaddingSizeInAxis(aLineCrossAxisPosnTracker.GetAxis());
  aLineCrossAxisPosnTracker.EnterAlignPackingSpace(aItem);
  aLineCrossAxisPosnTracker.EnterMargin(aItem.GetMargin());
  aLineCrossAxisPosnTracker.EnterChildFrame(itemCrossBorderBoxSize);

  aItem.SetCrossPosition(aLineStartPosition +
                         aLineCrossAxisPosnTracker.GetPosition());

  
  aLineCrossAxisPosnTracker.ResetPosition();
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

  
  
  nsTArray<FlexItem> items;
  nsresult rv = GenerateFlexItems(aPresContext, aReflowState,
                                  axisTracker, items);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  nscoord flexContainerMainSize =
    ComputeFlexContainerMainSize(aReflowState, axisTracker, items);

  ResolveFlexibleLengths(axisTracker, flexContainerMainSize, items);

  
  nscoord frameMainSize = flexContainerMainSize +
    axisTracker.GetMarginSizeInMainAxis(aReflowState.mComputedBorderPadding);

  nscoord frameCrossSize;

  MainAxisPositionTracker mainAxisPosnTracker(this, axisTracker,
                                              aReflowState, items);

  
  for (uint32_t i = 0; i < items.Length(); ++i) {
    FlexItem& curItem = items[i];

    nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                       curItem.Frame(),
                                       nsSize(aReflowState.ComputedWidth(),
                                              NS_UNCONSTRAINEDSIZE));
    
    if (IsAxisHorizontal(axisTracker.GetMainAxis())) {
      childReflowState.SetComputedWidth(curItem.GetMainSize());
    } else {
      childReflowState.SetComputedHeight(curItem.GetMainSize());
    }

    PositionItemInMainAxis(mainAxisPosnTracker, curItem);

    nsresult rv =
      SizeItemInCrossAxis(aPresContext, axisTracker,
                          childReflowState, curItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  CrossAxisPositionTracker
    crossAxisPosnTracker(this, axisTracker, aReflowState);

  
  
  SingleLineCrossAxisPositionTracker
    lineCrossAxisPosnTracker(this, axisTracker, items);

  lineCrossAxisPosnTracker.ComputeLineCrossSize(items);
  
  
  
  
  

  
  mCachedContentBoxCrossSize =
    axisTracker.GetCrossComponent(nsSize(aReflowState.ComputedWidth(),
                                         aReflowState.ComputedHeight()));

  if (mCachedContentBoxCrossSize == NS_AUTOHEIGHT) {
    
    
    nscoord minCrossSize =
      axisTracker.GetCrossComponent(nsSize(aReflowState.mComputedMinWidth,
                                           aReflowState.mComputedMinHeight));
    nscoord maxCrossSize =
      axisTracker.GetCrossComponent(nsSize(aReflowState.mComputedMaxWidth,
                                           aReflowState.mComputedMaxHeight));
    mCachedContentBoxCrossSize =
      NS_CSS_MINMAX(lineCrossAxisPosnTracker.GetLineCrossSize(),
                    minCrossSize, maxCrossSize);
  }
  if (lineCrossAxisPosnTracker.GetLineCrossSize() !=
      mCachedContentBoxCrossSize) {
    
    
    
    
    
    
    lineCrossAxisPosnTracker.SetLineCrossSize(mCachedContentBoxCrossSize);
  }
  frameCrossSize = mCachedContentBoxCrossSize +
    axisTracker.GetMarginSizeInCrossAxis(aReflowState.mComputedBorderPadding);

  
  
  
  
  
  
  
  mCachedAscent = mCachedContentBoxCrossSize +
    aReflowState.mComputedBorderPadding.top;

  
  for (uint32_t i = 0; i < items.Length(); ++i) {
    PositionItemInCrossAxis(crossAxisPosnTracker.GetPosition(),
                            lineCrossAxisPosnTracker, items[i]);
  }

  
  
  for (uint32_t i = 0; i < items.Length(); ++i) {
    FlexItem& curItem = items[i];
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
    
    
    

    nscoord mainPosn = curItem.GetMainPosition();
    nscoord crossPosn = curItem.GetCrossPosition();
    if (!AxisGrowsInPositiveDirection(axisTracker.GetMainAxis())) {
      mainPosn = frameMainSize - mainPosn;
    }
    if (!AxisGrowsInPositiveDirection(axisTracker.GetCrossAxis())) {
      crossPosn = frameCrossSize - crossPosn;
    }

    nsPoint physicalPosn =
      axisTracker.PhysicalPositionFromLogicalPosition(mainPosn, crossPosn);

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

    
    const nsStyleDisplay* styleDisp = curItem.Frame()->StyleDisplay();
    if (NS_STYLE_POSITION_RELATIVE == styleDisp->mPosition) {
      physicalPosn.x += childReflowState.mComputedOffsets.left;
      physicalPosn.y += childReflowState.mComputedOffsets.top;
    }

    rv = FinishReflowChild(curItem.Frame(), aPresContext,
                           &childReflowState, childDesiredSize,
                           physicalPosn.x, physicalPosn.y, 0);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  aDesiredSize.width =
    IsAxisHorizontal(axisTracker.GetMainAxis()) ?
    frameMainSize : frameCrossSize;
  aDesiredSize.height =
    IsAxisHorizontal(axisTracker.GetCrossAxis()) ?
    frameMainSize : frameCrossSize;

  aDesiredSize.ascent = mCachedAscent;

  
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, e.get());
  }

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize)

  aStatus = NS_FRAME_COMPLETE;

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize,
                                 aReflowState, aStatus);

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
    if (IsAxisHorizontal(axisTracker.GetMainAxis())) {
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
