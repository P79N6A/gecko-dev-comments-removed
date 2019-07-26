








#include "nsFlexContainerFrame.h"
#include "nsContentUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsDisplayList.h"
#include "nsIFrameInlines.h"
#include "nsLayoutUtils.h"
#include "nsPlaceholderFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "prlog.h"
#include <algorithm>
#include "mozilla/LinkedList.h"

using namespace mozilla;
using namespace mozilla::css;
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


class MOZ_STACK_CLASS nsFlexContainerFrame::FlexboxAxisTracker {
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

  
  
  
  bool AreAxesInternallyReversed() const
  {
    return mAreAxesInternallyReversed;
  }

private:
  AxisOrientationType mMainAxis;
  AxisOrientationType mCrossAxis;
  bool mAreAxesInternallyReversed;
};






class nsFlexContainerFrame::FlexItem : public LinkedListElement<FlexItem>
{
public:
  
  FlexItem(nsIFrame* aChildFrame,
           float aFlexGrow, float aFlexShrink, nscoord aMainBaseSize,
           nscoord aMainMinSize, nscoord aMainMaxSize,
           nscoord aCrossMinSize, nscoord aCrossMaxSize,
           nsMargin aMargin, nsMargin aBorderPadding,
           const FlexboxAxisTracker& aAxisTracker);

  
  FlexItem(nsIFrame* aChildFrame, nscoord aCrossSize);

  
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

  
  
  
  nscoord GetOuterMainSize(AxisOrientationType aMainAxis) const
  {
    return mMainSize + GetMarginBorderPaddingSizeInAxis(aMainAxis);
  }

  nscoord GetOuterCrossSize(AxisOrientationType aCrossAxis) const
  {
    return mCrossSize + GetMarginBorderPaddingSizeInAxis(aCrossAxis);
  }

  
  
  
  
  nscoord GetBaselineOffsetFromOuterCrossStart(
            AxisOrientationType aCrossAxis) const;

  float GetShareOfFlexWeightSoFar() const { return mShareOfFlexWeightSoFar; }

  bool IsFrozen() const            { return mIsFrozen; }

  bool HadMinViolation() const     { return mHadMinViolation; }
  bool HadMaxViolation() const     { return mHadMaxViolation; }

  
  
  bool HadMeasuringReflow() const  { return mHadMeasuringReflow; }

  
  
  
  bool IsStretched() const         { return mIsStretched; }

  
  
  bool IsStrut() const             { return mIsStrut; }

  uint8_t GetAlignSelf() const     { return mAlignSelf; }

  
  
  
  
  
  float GetFlexWeightToUse(bool aIsUsingFlexGrow)
  {
    if (IsFrozen()) {
      return 0.0f;
    }

    if (aIsUsingFlexGrow) {
      return mFlexGrow;
    }

    
    if (mFlexBaseSize == 0) {
      
      
      
      
      return 0.0f;
    }
    return mFlexShrink * mFlexBaseSize;
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
  bool mIsStrut;     
                     
  uint8_t mAlignSelf; 
                      
                      
};





class nsFlexContainerFrame::FlexLine : public LinkedListElement<FlexLine>
{
public:
  FlexLine()
  : mNumItems(0),
    mTotalInnerHypotheticalMainSize(0),
    mTotalOuterHypotheticalMainSize(0),
    mLineCrossSize(0),
    mBaselineOffsetFromCrossStart(nscoord_MIN)
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

  friend class AutoFlexLineListClearer; 
 
private:
  
  void FreezeOrRestoreEachFlexibleSize(const nscoord aTotalViolation,
                                       bool aIsFinalIteration);

  LinkedList<FlexItem> mItems; 

  uint32_t mNumItems; 
                      
                      
                      
                      

  nscoord mTotalInnerHypotheticalMainSize;
  nscoord mTotalOuterHypotheticalMainSize;
  nscoord mLineCrossSize;
  nscoord mBaselineOffsetFromCrossStart;
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
  const FlexboxAxisTracker axisTracker(this);
  return IsAxisHorizontal(axisTracker.GetMainAxis());
}

FlexItem*
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
                                           childRS.ComputedMinWidth(),
                                           childRS.ComputedMinHeight());
  nscoord mainMaxSize = GET_MAIN_COMPONENT(aAxisTracker,
                                           childRS.ComputedMaxWidth(),
                                           childRS.ComputedMaxHeight());
  
  MOZ_ASSERT(mainMinSize <= mainMaxSize, "min size is larger than max size");

  
  

  nscoord crossMinSize = GET_CROSS_COMPONENT(aAxisTracker,
                                             childRS.ComputedMinWidth(),
                                             childRS.ComputedMinHeight());
  nscoord crossMaxSize = GET_CROSS_COMPONENT(aAxisTracker,
                                             childRS.ComputedMaxWidth(),
                                             childRS.ComputedMaxHeight());

  
  
  
  
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

    
    
    nsMargin& bp = childRS.ComputedPhysicalBorderPadding();
    widgetMainMinSize = std::max(widgetMainMinSize -
                                 aAxisTracker.GetMarginSizeInMainAxis(bp), 0);
    widgetCrossMinSize = std::max(widgetCrossMinSize -
                                  aAxisTracker.GetMarginSizeInCrossAxis(bp), 0);

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

  
  FlexItem* item = new FlexItem(aChildFrame,
                                flexGrow, flexShrink, flexBaseSize,
                                mainMinSize, mainMaxSize,
                                crossMinSize, crossMaxSize,
                                childRS.ComputedPhysicalMargin(),
                                childRS.ComputedPhysicalBorderPadding(),
                                aAxisTracker);

  
  
  
  if (isFixedSizeWidget || (flexGrow == 0.0f && flexShrink == 0.0f)) {
    item->Freeze();
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

  nsHTMLReflowMetrics childDesiredSize(childRSForMeasuringHeight);
  nsReflowStatus childReflowStatus;
  const uint32_t flags = NS_FRAME_NO_MOVE_FRAME;
  nsresult rv = ReflowChild(aFlexItem.Frame(), aPresContext,
                            childDesiredSize, childRSForMeasuringHeight,
                            0, 0, flags, childReflowStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
             "We gave flex item unconstrained available height, so it "
             "should be complete");

  rv = FinishReflowChild(aFlexItem.Frame(), aPresContext,
                         childDesiredSize, &childRSForMeasuringHeight,
                         0, 0, flags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nscoord childDesiredHeight = childDesiredSize.Height() -
    childRSForMeasuringHeight.ComputedPhysicalBorderPadding().TopBottom();
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
    mIsStrut(false),
    mAlignSelf(aChildFrame->StylePosition()->mAlignSelf)
{
  MOZ_ASSERT(mFrame, "expecting a non-null child frame");
  MOZ_ASSERT(mFrame->GetType() != nsGkAtoms::placeholderFrame,
             "placeholder frames should not be treated as flex items");
  MOZ_ASSERT(!(mFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW),
             "out-of-flow frames should not be treated as flex items");

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
    mShareOfFlexWeightSoFar(0.0f),
    mIsFrozen(true),
    mHadMinViolation(false),
    mHadMaxViolation(false),
    mHadMeasuringReflow(false),
    mIsStretched(false),
    mIsStrut(true), 
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

  
  
  
  
  return GetOuterCrossSize(aCrossAxis) - marginTopToBaseline;
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
                              const FlexItem& aItem,
                              const FlexboxAxisTracker& aAxisTracker);

  
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

  for (FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
    MOZ_ASSERT(!item->HadMinViolation() || !item->HadMaxViolation(),
               "Can have either min or max violation, but not both");

    if (!item->IsFrozen()) {
      if (eFreezeEverything == freezeType ||
          (eFreezeMinViolations == freezeType && item->HadMinViolation()) ||
          (eFreezeMaxViolations == freezeType && item->HadMaxViolation())) {

        MOZ_ASSERT(item->GetMainSize() >= item->GetMainMinSize(),
                   "Freezing item at a size below its minimum");
        MOZ_ASSERT(item->GetMainSize() <= item->GetMainMaxSize(),
                   "Freezing item at a size above its maximum");

        item->Freeze();
      } else if (MOZ_UNLIKELY(aIsFinalIteration)) {
        
        
        NS_ERROR("Final iteration still has unfrozen items, this shouldn't"
                 " happen unless there was nscoord under/overflow.");
        item->Freeze();
      } 
        

      
      item->ClearViolationFlags();
    }
  }
}







void
FlexLine::ResolveFlexibleLengths(nscoord aFlexContainerMainSize)
{
  PR_LOG(GetFlexContainerLog(), PR_LOG_DEBUG, ("ResolveFlexibleLengths\n"));
  if (IsEmpty()) {
    return;
  }

  
  
  
  nscoord spaceReservedForMarginBorderPadding =
    mTotalOuterHypotheticalMainSize - mTotalInnerHypotheticalMainSize;

  nscoord spaceAvailableForFlexItemsContentBoxes =
    aFlexContainerMainSize - spaceReservedForMarginBorderPadding;

  
  const bool isUsingFlexGrow =
    (mTotalOuterHypotheticalMainSize < aFlexContainerMainSize);

  
  
  
  
  
  
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

    
    
    if ((availableFreeSpace > 0 && isUsingFlexGrow) ||
        (availableFreeSpace < 0 && !isUsingFlexGrow)) {

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      float runningFlexWeightSum = 0.0f;
      float largestFlexWeight = 0.0f;
      uint32_t numItemsWithLargestFlexWeight = 0;
      for (FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
        float curFlexWeight = item->GetFlexWeightToUse(isUsingFlexGrow);
        MOZ_ASSERT(curFlexWeight >= 0.0f, "weights are non-negative");

        runningFlexWeightSum += curFlexWeight;
        if (NS_finite(runningFlexWeightSum)) {
          if (curFlexWeight == 0.0f) {
            item->SetShareOfFlexWeightSoFar(0.0f);
          } else {
            item->SetShareOfFlexWeightSoFar(curFlexWeight /
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
        
        
        
        for (FlexItem* item = mItems.getLast(); item;
             item = item->getPrevious()) {

          if (!item->IsFrozen()) {
            
            
            nscoord sizeDelta = 0;
            if (NS_finite(runningFlexWeightSum)) {
              float myShareOfRemainingSpace =
                item->GetShareOfFlexWeightSoFar();

              MOZ_ASSERT(myShareOfRemainingSpace >= 0.0f &&
                         myShareOfRemainingSpace <= 1.0f,
                         "my share should be nonnegative fractional amount");

              if (myShareOfRemainingSpace == 1.0f) {
                
                
                sizeDelta = availableFreeSpace;
              } else if (myShareOfRemainingSpace > 0.0f) {
                sizeDelta = NSToCoordRound(availableFreeSpace *
                                           myShareOfRemainingSpace);
              }
            } else if (item->GetFlexWeightToUse(isUsingFlexGrow) ==
                       largestFlexWeight) {
              
              
              
              sizeDelta =
                NSToCoordRound(availableFreeSpace /
                               float(numItemsWithLargestFlexWeight));
              numItemsWithLargestFlexWeight--;
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

    for (FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
      if (!item->IsFrozen()) {
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

    if (totalViolation == 0) {
      break;
    }
  }

  
#ifdef DEBUG
  for (const FlexItem* item = mItems.getFirst(); item; item = item->getNext()) {
    MOZ_ASSERT(item->IsFrozen(),
               "All flexible lengths should've been resolved");
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
        item->GetBaselineOffsetFromOuterCrossStart(aAxisTracker.GetCrossAxis());
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

FlexboxAxisTracker::FlexboxAxisTracker(
  nsFlexContainerFrame* aFlexContainerFrame)
  : mAreAxesInternallyReversed(false)
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

  
  
  
  
  
  
  
  
  
  static bool sPreventBottomToTopChildOrdering = false;

  if (sPreventBottomToTopChildOrdering) {
    
    
    if (eAxis_BT == mMainAxis || eAxis_BT == mCrossAxis) {
      mMainAxis = GetReverseAxis(mMainAxis);
      mCrossAxis = GetReverseAxis(mCrossAxis);
      mAreAxesInternallyReversed = true;
    }
  }

  MOZ_ASSERT(IsAxisHorizontal(mMainAxis) != IsAxisHorizontal(mCrossAxis),
             "main & cross axes should be in different dimensions");
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

nsresult
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

    
    
    if (!IsAxisHorizontal(aAxisTracker.GetMainAxis()) &&
        aAvailableHeightForContent != NS_UNCONSTRAINEDSIZE) {
      wrapThreshold = std::min(wrapThreshold, aAvailableHeightForContent);
    }
  }

  
  
  uint32_t nextStrutIdx = 0;

  
  
  uint32_t itemIdxInContainer = 0;

  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nsIFrame* childFrame = e.get();

    
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

      nsresult rv = ResolveFlexItemMaxContentSizing(aPresContext, *item,
                                                    aReflowState, aAxisTracker);
      NS_ENSURE_SUCCESS(rv,rv);
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

  if (IsAxisHorizontal(aAxisTracker.GetMainAxis())) {
    
    
    
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
    if (aSumLineCrossSizes <= aAvailableHeightForContent) {
      return aAvailableHeightForContent;
    }
    return std::min(effectiveComputedHeight, aSumLineCrossSizes);
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



static void
ResolveReflowedChildAscent(nsIFrame* aFrame,
                           nsHTMLReflowMetrics& aChildDesiredSize)
{
  if (aChildDesiredSize.TopAscent() == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    
    nscoord ascent;
    if (nsLayoutUtils::GetFirstLineBaseline(aFrame, &ascent)) {
      aChildDesiredSize.SetTopAscent(ascent);
    } else {
      aChildDesiredSize.SetTopAscent(aFrame->GetBaseline());
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
  nsHTMLReflowMetrics childDesiredSize(aChildReflowState);
  nsReflowStatus childReflowStatus;
  const uint32_t flags = NS_FRAME_NO_MOVE_FRAME;
  nsresult rv = ReflowChild(aItem.Frame(), aPresContext,
                            childDesiredSize, aChildReflowState,
                            0, 0, flags, childReflowStatus);
  aItem.SetHadMeasuringReflow();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
             "We gave flex item unconstrained available height, so it "
             "should be complete");

  
  
  rv = FinishReflowChild(aItem.Frame(), aPresContext,
                         childDesiredSize, &aChildReflowState, 0, 0, flags);
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  
  
  
  
  
  
  nscoord crossAxisBorderPadding = aItem.GetBorderPadding().TopBottom();
  if (childDesiredSize.Height() < crossAxisBorderPadding) {
    
    
    
    
    
    
    NS_WARN_IF_FALSE(!aItem.Frame()->GetType(),
                     "Child should at least request space for border/padding");
    aItem.SetCrossSize(0);
  } else {
    
    aItem.SetCrossSize(childDesiredSize.Height() - crossAxisBorderPadding);
  }

  
  if (aItem.GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_BASELINE) {
    ResolveReflowedChildAscent(aItem.Frame(), childDesiredSize);
    aItem.SetAscent(childDesiredSize.TopAscent());
  }

  return NS_OK;
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

nsresult
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

  
  
  
  
  
  
  
  
  

  if (!HasAnyStateBits(NS_STATE_FLEX_CHILDREN_REORDERED)) {
    if (SortChildrenIfNeeded<IsOrderLEQ>()) {
      AddStateBits(NS_STATE_FLEX_CHILDREN_REORDERED);
    }
  } else {
    SortChildrenIfNeeded<IsOrderLEQWithDOMFallback>();
  }

  const FlexboxAxisTracker axisTracker(this);

  
  
  
  
  nscoord availableHeightForContent = aReflowState.AvailableHeight();
  if (availableHeightForContent != NS_UNCONSTRAINEDSIZE &&
      !(GetSkipSides() & (1 << NS_SIDE_TOP))) {
    availableHeightForContent -= aReflowState.ComputedPhysicalBorderPadding().top;
    
    availableHeightForContent = std::max(availableHeightForContent, 0);
  }

  nscoord contentBoxMainSize = GetMainSizeFromReflowState(aReflowState,
                                                          axisTracker);

  nsAutoTArray<StrutInfo, 1> struts;
  nsresult rv = DoFlexLayout(aPresContext, aDesiredSize, aReflowState, aStatus,
                             contentBoxMainSize, availableHeightForContent,
                             struts, axisTracker);

  if (NS_SUCCEEDED(rv) && !struts.IsEmpty()) {
    
    rv = DoFlexLayout(aPresContext, aDesiredSize, aReflowState, aStatus,
                      contentBoxMainSize, availableHeightForContent,
                      struts, axisTracker);
  }

  return rv;
}




class MOZ_STACK_CLASS AutoFlexLineListClearer
{
public:
  AutoFlexLineListClearer(LinkedList<FlexLine>& aLines
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

nsresult
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

  nsresult rv = GenerateFlexLines(aPresContext, aReflowState,
                                  aContentBoxMainSize,
                                  aAvailableHeightForContent,
                                  aStruts, aAxisTracker, lines);
  NS_ENSURE_SUCCESS(rv, rv);

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
      
      
      if (!item->IsStretched() && !item->IsStrut()) {
        nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                           item->Frame(),
                                           nsSize(aReflowState.ComputedWidth(),
                                                  NS_UNCONSTRAINEDSIZE));
        
        if (IsAxisHorizontal(aAxisTracker.GetMainAxis())) {
          childReflowState.SetComputedWidth(item->GetMainSize());
        } else {
          childReflowState.SetComputedHeight(item->GetMainSize());
        }

        nsresult rv = SizeItemInCrossAxis(aPresContext, aAxisTracker,
                                          childReflowState, *item);
        NS_ENSURE_SUCCESS(rv, rv);
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
      
      return NS_OK;
    }
  }

  
  
  nscoord flexContainerAscent;
  nscoord firstLineBaselineOffset =
    lines.getFirst()->GetBaselineOffsetFromCrossStart();
  if (firstLineBaselineOffset == nscoord_MIN) {
    
    
    flexContainerAscent = nscoord_MIN;
  } else {
    
    
    
    nscoord firstLineBaselineOffsetWRTContainer =
      firstLineBaselineOffset + crossAxisPosnTracker.GetPosition();

    
    
    
    flexContainerAscent = aReflowState.ComputedPhysicalBorderPadding().top +
      PhysicalPosFromLogicalPos(firstLineBaselineOffsetWRTContainer,
                                contentBoxCrossSize,
                                aAxisTracker.GetCrossAxis());
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

  
  
  
  nsMargin containerBorderPadding(aReflowState.ComputedPhysicalBorderPadding());
  ApplySkipSides(containerBorderPadding, &aReflowState);
  const nsPoint containerContentBoxOrigin(containerBorderPadding.left,
                                          containerBorderPadding.top);

  
  
  for (const FlexLine* line = lines.getFirst(); line; line = line->getNext()) {
    for (const FlexItem* item = line->GetFirstItem(); item;
         item = item->getNext()) {
      nsPoint physicalPosn = aAxisTracker.PhysicalPointFromLogicalPoint(
                               item->GetMainPosition(),
                               item->GetCrossPosition(),
                               aContentBoxMainSize,
                               contentBoxCrossSize);
      
      
      physicalPosn += containerContentBoxOrigin;

      nsHTMLReflowState childReflowState(aPresContext, aReflowState,
                                         item->Frame(),
                                         nsSize(aReflowState.ComputedWidth(),
                                                NS_UNCONSTRAINEDSIZE));

      
      
      bool didOverrideComputedWidth = false;
      bool didOverrideComputedHeight = false;

      
      if (IsAxisHorizontal(aAxisTracker.GetMainAxis())) {
        childReflowState.SetComputedWidth(item->GetMainSize());
        didOverrideComputedWidth = true;
      } else {
        childReflowState.SetComputedHeight(item->GetMainSize());
        didOverrideComputedHeight = true;
      }

      
      if (item->IsStretched()) {
        MOZ_ASSERT(item->GetAlignSelf() == NS_STYLE_ALIGN_ITEMS_STRETCH,
                   "stretched item w/o 'align-self: stretch'?");
        if (IsAxisHorizontal(aAxisTracker.GetCrossAxis())) {
          childReflowState.SetComputedWidth(item->GetCrossSize());
          didOverrideComputedWidth = true;
        } else {
          
          item->Frame()->AddStateBits(NS_FRAME_CONTAINS_RELATIVE_HEIGHT);
          childReflowState.SetComputedHeight(item->GetCrossSize());
          didOverrideComputedHeight = true;
        }
      }

      
      
      

      
      
      
      if (item->HadMeasuringReflow()) {
        if (didOverrideComputedWidth) {
          
          
          
          
          childReflowState.mFlags.mHResize = true;
        }
        if (didOverrideComputedHeight) {
          childReflowState.mFlags.mVResize = true;
        }
      }
      
      
      

      nsHTMLReflowMetrics childDesiredSize(childReflowState);
      nsReflowStatus childReflowStatus;
      nsresult rv = ReflowChild(item->Frame(), aPresContext,
                                childDesiredSize, childReflowState,
                                physicalPosn.x, physicalPosn.y,
                                0, childReflowStatus);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      MOZ_ASSERT(NS_FRAME_IS_COMPLETE(childReflowStatus),
                 "We gave flex item unconstrained available height, so it "
                 "should be complete");

      childReflowState.ApplyRelativePositioning(&physicalPosn);

      rv = FinishReflowChild(item->Frame(), aPresContext,
                             childDesiredSize, &childReflowState,
                             physicalPosn.x, physicalPosn.y, 0);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      if (item->Frame() == mFrames.FirstChild() &&
          flexContainerAscent == nscoord_MIN) {
        ResolveReflowedChildAscent(item->Frame(), childDesiredSize);

        
        
        
        flexContainerAscent = item->Frame()->GetNormalPosition().y +
          childDesiredSize.TopAscent();
      }
    }
  }

  nsSize desiredContentBoxSize =
    aAxisTracker.PhysicalSizeFromLogicalSizes(aContentBoxMainSize,
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
  aDesiredSize.SetTopAscent(flexContainerAscent);

  
  
  
  
  
  
  
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
  nscoord minWidth = 0;
  DISPLAY_MIN_WIDTH(this, minWidth);

  FlexboxAxisTracker axisTracker(this);

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
  nscoord prefWidth = 0;
  DISPLAY_PREF_WIDTH(this, prefWidth);

  
  
  
  
  
  FlexboxAxisTracker axisTracker(this);

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
