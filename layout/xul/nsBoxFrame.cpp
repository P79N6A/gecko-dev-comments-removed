
































#include "nsBoxLayoutState.h"
#include "nsBoxFrame.h"
#include "mozilla/dom/Touch.h"
#include "nsStyleContext.h"
#include "nsPlaceholderFrame.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsNameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsHTMLParts.h"
#include "nsViewManager.h"
#include "nsView.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsIServiceManager.h"
#include "nsBoxLayout.h"
#include "nsSprocketLayout.h"
#include "nsIScrollableFrame.h"
#include "nsWidgetsCID.h"
#include "nsCSSAnonBoxes.h"
#include "nsContainerFrame.h"
#include "nsIDOMElement.h"
#include "nsITheme.h"
#include "nsTransform2D.h"
#include "mozilla/EventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsDisplayList.h"
#include "mozilla/Preferences.h"
#include "nsThemeConstants.h"
#include "nsLayoutUtils.h"
#include <algorithm>


#include "nsIURI.h"

#include "mozilla/TouchEvents.h"

using namespace mozilla;
using namespace mozilla::dom;



#define DEBUG_SPRING_SIZE 8
#define DEBUG_BORDER_SIZE 2
#define COIL_SIZE 8



#ifdef DEBUG_rods

#endif

#ifdef DEBUG_LAYOUT
bool nsBoxFrame::gDebug = false;
nsIFrame* nsBoxFrame::mDebugChild = nullptr;
#endif

nsIFrame*
NS_NewBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, bool aIsRoot, nsBoxLayout* aLayoutManager)
{
  return new (aPresShell) nsBoxFrame(aPresShell, aContext, aIsRoot, aLayoutManager);
}

nsIFrame*
NS_NewBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsBoxFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsBoxFrame)

#ifdef DEBUG
NS_QUERYFRAME_HEAD(nsBoxFrame)
  NS_QUERYFRAME_ENTRY(nsBoxFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)
#endif

nsBoxFrame::nsBoxFrame(nsIPresShell* aPresShell,
                       nsStyleContext* aContext,
                       bool aIsRoot,
                       nsBoxLayout* aLayoutManager) :
  nsContainerFrame(aContext)
{
  mState |= NS_STATE_IS_HORIZONTAL;
  mState |= NS_STATE_AUTO_STRETCH;

  if (aIsRoot) 
     mState |= NS_STATE_IS_ROOT;

  mValign = vAlign_Top;
  mHalign = hAlign_Left;
  
  
  nsCOMPtr<nsBoxLayout> layout = aLayoutManager;

  if (layout == nullptr) {
    NS_NewSprocketLayout(aPresShell, layout);
  }

  SetLayoutManager(layout);
}

nsBoxFrame::~nsBoxFrame()
{
}

void
nsBoxFrame::SetInitialChildList(ChildListID     aListID,
                                nsFrameList&    aChildList)
{
  nsContainerFrame::SetInitialChildList(aListID, aChildList);
  
  nsBoxLayoutState state(PresContext());
  CheckBoxOrder();
  if (mLayoutManager)
    mLayoutManager->ChildrenSet(this, state, mFrames.FirstChild());
}

 void
nsBoxFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsContainerFrame::DidSetStyleContext(aOldStyleContext);

  
  
  CacheAttributes();
}




void
nsBoxFrame::Init(nsIContent*       aContent,
                 nsContainerFrame* aParent,
                 nsIFrame*         aPrevInFlow)
{
  nsContainerFrame::Init(aContent, aParent, aPrevInFlow);

  if (GetStateBits() & NS_FRAME_FONT_INFLATION_CONTAINER) {
    AddStateBits(NS_FRAME_FONT_INFLATION_FLOW_ROOT);
  }

  MarkIntrinsicWidthsDirty();

  CacheAttributes();

#ifdef DEBUG_LAYOUT
    
  if (mState & NS_STATE_IS_ROOT) 
      GetDebugPref(GetPresContext());
#endif

  UpdateMouseThrough();

  
  RegUnregAccessKey(true);
}

void nsBoxFrame::UpdateMouseThrough()
{
  if (mContent) {
    static nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::never, &nsGkAtoms::always, nullptr};
    switch (mContent->FindAttrValueIn(kNameSpaceID_None,
              nsGkAtoms::mousethrough, strings, eCaseMatters)) {
      case 0: AddStateBits(NS_FRAME_MOUSE_THROUGH_NEVER); break;
      case 1: AddStateBits(NS_FRAME_MOUSE_THROUGH_ALWAYS); break;
      case 2: {
        RemoveStateBits(NS_FRAME_MOUSE_THROUGH_ALWAYS);
        RemoveStateBits(NS_FRAME_MOUSE_THROUGH_NEVER);
        break;
      }
    }
  }
}

void
nsBoxFrame::CacheAttributes()
{
  





  mValign = vAlign_Top;
  mHalign = hAlign_Left;

  bool orient = false;
  GetInitialOrientation(orient); 
  if (orient)
    mState |= NS_STATE_IS_HORIZONTAL;
  else
    mState &= ~NS_STATE_IS_HORIZONTAL;

  bool normal = true;
  GetInitialDirection(normal); 
  if (normal)
    mState |= NS_STATE_IS_DIRECTION_NORMAL;
  else
    mState &= ~NS_STATE_IS_DIRECTION_NORMAL;

  GetInitialVAlignment(mValign);
  GetInitialHAlignment(mHalign);
  
  bool equalSize = false;
  GetInitialEqualSize(equalSize); 
  if (equalSize)
        mState |= NS_STATE_EQUAL_SIZE;
    else
        mState &= ~NS_STATE_EQUAL_SIZE;

  bool autostretch = !!(mState & NS_STATE_AUTO_STRETCH);
  GetInitialAutoStretch(autostretch);
  if (autostretch)
        mState |= NS_STATE_AUTO_STRETCH;
     else
        mState &= ~NS_STATE_AUTO_STRETCH;


#ifdef DEBUG_LAYOUT
  bool debug = mState & NS_STATE_SET_TO_DEBUG;
  bool debugSet = GetInitialDebug(debug); 
  if (debugSet) {
        mState |= NS_STATE_DEBUG_WAS_SET;
        if (debug)
            mState |= NS_STATE_SET_TO_DEBUG;
        else
            mState &= ~NS_STATE_SET_TO_DEBUG;
  } else {
        mState &= ~NS_STATE_DEBUG_WAS_SET;
  }
#endif
}

#ifdef DEBUG_LAYOUT
bool
nsBoxFrame::GetInitialDebug(bool& aDebug)
{
  if (!GetContent())
    return false;

  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_false, &nsGkAtoms::_true, nullptr};
  int32_t index = GetContent()->FindAttrValueIn(kNameSpaceID_None,
      nsGkAtoms::debug, strings, eCaseMatters);
  if (index >= 0) {
    aDebug = index == 1;
    return true;
  }

  return false;
}
#endif

bool
nsBoxFrame::GetInitialHAlignment(nsBoxFrame::Halignment& aHalign)
{
  if (!GetContent())
    return false;

  
  static nsIContent::AttrValuesArray alignStrings[] =
    {&nsGkAtoms::left, &nsGkAtoms::right, nullptr};
  static const Halignment alignValues[] = {hAlign_Left, hAlign_Right};
  int32_t index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::align,
      alignStrings, eCaseMatters);
  if (index >= 0) {
    aHalign = alignValues[index];
    return true;
  }
      
  
  
  
  nsIAtom* attrName = IsHorizontal() ? nsGkAtoms::pack : nsGkAtoms::align;
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::start, &nsGkAtoms::center, &nsGkAtoms::end, nullptr};
  static const Halignment values[] =
    {hAlign_Left, hAlign_Left, hAlign_Center, hAlign_Right};
  index = GetContent()->FindAttrValueIn(kNameSpaceID_None, attrName,
      strings, eCaseMatters);

  if (index == nsIContent::ATTR_VALUE_NO_MATCH) {
    
    return false;
  }
  if (index > 0) {    
    aHalign = values[index];
    return true;
  }

  
  
  
  const nsStyleXUL* boxInfo = StyleXUL();
  if (IsHorizontal()) {
    switch (boxInfo->mBoxPack) {
      case NS_STYLE_BOX_PACK_START:
        aHalign = nsBoxFrame::hAlign_Left;
        return true;
      case NS_STYLE_BOX_PACK_CENTER:
        aHalign = nsBoxFrame::hAlign_Center;
        return true;
      case NS_STYLE_BOX_PACK_END:
        aHalign = nsBoxFrame::hAlign_Right;
        return true;
      default: 
        return false;
    }
  }
  else {
    switch (boxInfo->mBoxAlign) {
      case NS_STYLE_BOX_ALIGN_START:
        aHalign = nsBoxFrame::hAlign_Left;
        return true;
      case NS_STYLE_BOX_ALIGN_CENTER:
        aHalign = nsBoxFrame::hAlign_Center;
        return true;
      case NS_STYLE_BOX_ALIGN_END:
        aHalign = nsBoxFrame::hAlign_Right;
        return true;
      default: 
        return false;
    }
  }

  return false;
}

bool
nsBoxFrame::GetInitialVAlignment(nsBoxFrame::Valignment& aValign)
{
  if (!GetContent())
    return false;

  static nsIContent::AttrValuesArray valignStrings[] =
    {&nsGkAtoms::top, &nsGkAtoms::baseline, &nsGkAtoms::middle, &nsGkAtoms::bottom, nullptr};
  static const Valignment valignValues[] =
    {vAlign_Top, vAlign_BaseLine, vAlign_Middle, vAlign_Bottom};
  int32_t index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::valign,
      valignStrings, eCaseMatters);
  if (index >= 0) {
    aValign = valignValues[index];
    return true;
  }

  
  
  
  nsIAtom* attrName = IsHorizontal() ? nsGkAtoms::align : nsGkAtoms::pack;
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::start, &nsGkAtoms::center,
     &nsGkAtoms::baseline, &nsGkAtoms::end, nullptr};
  static const Valignment values[] =
    {vAlign_Top, vAlign_Top, vAlign_Middle, vAlign_BaseLine, vAlign_Bottom};
  index = GetContent()->FindAttrValueIn(kNameSpaceID_None, attrName,
      strings, eCaseMatters);
  if (index == nsIContent::ATTR_VALUE_NO_MATCH) {
    
    return false;
  }
  if (index > 0) {
    aValign = values[index];
    return true;
  }

  
  
  
  const nsStyleXUL* boxInfo = StyleXUL();
  if (IsHorizontal()) {
    switch (boxInfo->mBoxAlign) {
      case NS_STYLE_BOX_ALIGN_START:
        aValign = nsBoxFrame::vAlign_Top;
        return true;
      case NS_STYLE_BOX_ALIGN_CENTER:
        aValign = nsBoxFrame::vAlign_Middle;
        return true;
      case NS_STYLE_BOX_ALIGN_BASELINE:
        aValign = nsBoxFrame::vAlign_BaseLine;
        return true;
      case NS_STYLE_BOX_ALIGN_END:
        aValign = nsBoxFrame::vAlign_Bottom;
        return true;
      default: 
        return false;
    }
  }
  else {
    switch (boxInfo->mBoxPack) {
      case NS_STYLE_BOX_PACK_START:
        aValign = nsBoxFrame::vAlign_Top;
        return true;
      case NS_STYLE_BOX_PACK_CENTER:
        aValign = nsBoxFrame::vAlign_Middle;
        return true;
      case NS_STYLE_BOX_PACK_END:
        aValign = nsBoxFrame::vAlign_Bottom;
        return true;
      default: 
        return false;
    }
  }

  return false;
}

void
nsBoxFrame::GetInitialOrientation(bool& aIsHorizontal)
{
 
  if (!GetContent())
    return;

  
  const nsStyleXUL* boxInfo = StyleXUL();
  if (boxInfo->mBoxOrient == NS_STYLE_BOX_ORIENT_HORIZONTAL)
    aIsHorizontal = true;
  else 
    aIsHorizontal = false;

  
  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::vertical, &nsGkAtoms::horizontal, nullptr};
  int32_t index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::orient,
      strings, eCaseMatters);
  if (index >= 0) {
    aIsHorizontal = index == 1;
  }
}

void
nsBoxFrame::GetInitialDirection(bool& aIsNormal)
{
  if (!GetContent())
    return;

  if (IsHorizontal()) {
    
    
    aIsNormal = (StyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR); 
  }
  else
    aIsNormal = true; 

  
  const nsStyleXUL* boxInfo = StyleXUL();
  if (boxInfo->mBoxDirection == NS_STYLE_BOX_DIRECTION_REVERSE)
    aIsNormal = !aIsNormal; 
  
  
  
  if (IsHorizontal()) {
    static nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::reverse, &nsGkAtoms::ltr, &nsGkAtoms::rtl, nullptr};
    int32_t index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::dir,
        strings, eCaseMatters);
    if (index >= 0) {
      bool values[] = {!aIsNormal, true, false};
      aIsNormal = values[index];
    }
  } else if (GetContent()->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                                       nsGkAtoms::reverse, eCaseMatters)) {
    aIsNormal = !aIsNormal;
  }
}



bool
nsBoxFrame::GetInitialEqualSize(bool& aEqualSize)
{
 
  if (!GetContent())
     return false;

  if (GetContent()->AttrValueIs(kNameSpaceID_None, nsGkAtoms::equalsize,
                           nsGkAtoms::always, eCaseMatters)) {
    aEqualSize = true;
    return true;
  }

  return false;
}



bool
nsBoxFrame::GetInitialAutoStretch(bool& aStretch)
{
  if (!GetContent())
     return false;
  
  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::stretch, nullptr};
  int32_t index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::align,
      strings, eCaseMatters);
  if (index != nsIContent::ATTR_MISSING && index != 0) {
    aStretch = index == 1;
    return true;
  }

  
  const nsStyleXUL* boxInfo = StyleXUL();
  aStretch = (boxInfo->mBoxAlign == NS_STYLE_BOX_ALIGN_STRETCH);

  return true;
}

void
nsBoxFrame::DidReflow(nsPresContext*           aPresContext,
                      const nsHTMLReflowState*  aReflowState,
                      nsDidReflowStatus         aStatus)
{
  nsFrameState preserveBits =
    mState & (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN);
  nsFrame::DidReflow(aPresContext, aReflowState, aStatus);
  mState |= preserveBits;
}

bool
nsBoxFrame::HonorPrintBackgroundSettings()
{
  return (!mContent || !mContent->IsInNativeAnonymousSubtree()) &&
    nsContainerFrame::HonorPrintBackgroundSettings();
}

#ifdef DO_NOISY_REFLOW
static int myCounter = 0;
static void printSize(char * aDesc, nscoord aSize) 
{
  printf(" %s: ", aDesc);
  if (aSize == NS_UNCONSTRAINEDSIZE) {
    printf("UC");
  } else {
    printf("%d", aSize);
  }
}
#endif

 nscoord
nsBoxFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  nsBoxLayoutState state(PresContext(), aRenderingContext);
  nsSize minSize = GetMinSize(state);

  
  
  
  
  nsMargin bp;
  GetBorderAndPadding(bp);

  result = minSize.width - bp.LeftRight();
  result = std::max(result, 0);

  return result;
}

 nscoord
nsBoxFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  nsBoxLayoutState state(PresContext(), aRenderingContext);
  nsSize prefSize = GetPrefSize(state);

  
  
  
  
  nsMargin bp;
  GetBorderAndPadding(bp);

  result = prefSize.width - bp.LeftRight();
  result = std::max(result, 0);

  return result;
}

void
nsBoxFrame::Reflow(nsPresContext*          aPresContext,
                   nsHTMLReflowMetrics&     aDesiredSize,
                   const nsHTMLReflowState& aReflowState,
                   nsReflowStatus&          aStatus)
{
  
  

  DO_GLOBAL_REFLOW_COUNT("nsBoxFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_ASSERTION(aReflowState.ComputedWidth() >=0 &&
               aReflowState.ComputedHeight() >= 0, "Computed Size < 0");

#ifdef DO_NOISY_REFLOW
  printf("\n-------------Starting BoxFrame Reflow ----------------------------\n");
  printf("%p ** nsBF::Reflow %d ", this, myCounter++);
  
  printSize("AW", aReflowState.AvailableWidth());
  printSize("AH", aReflowState.AvailableHeight());
  printSize("CW", aReflowState.ComputedWidth());
  printSize("CH", aReflowState.ComputedHeight());

  printf(" *\n");

#endif

  aStatus = NS_FRAME_COMPLETE;

  
  nsBoxLayoutState state(aPresContext, aReflowState.rendContext,
                         &aReflowState, aReflowState.mReflowDepth);

  nsSize computedSize(aReflowState.ComputedWidth(),aReflowState.ComputedHeight());

  nsMargin m;
  m = aReflowState.ComputedPhysicalBorderPadding();
  

  nsSize prefSize(0,0);

  
  NS_ASSERTION(computedSize.width != NS_INTRINSICSIZE,
               "computed width should always be computed");
  if (computedSize.height == NS_INTRINSICSIZE) {
    prefSize = GetPrefSize(state);
    nsSize minSize = GetMinSize(state);
    nsSize maxSize = GetMaxSize(state);
    
    prefSize = BoundsCheck(minSize, prefSize, maxSize);
  }

  
  computedSize.width += m.left + m.right;

  if (aReflowState.ComputedHeight() == NS_INTRINSICSIZE) {
    computedSize.height = prefSize.height;
    
    nscoord verticalBorderPadding =
      aReflowState.ComputedPhysicalBorderPadding().TopBottom();
    nscoord contentHeight = computedSize.height - verticalBorderPadding;
    
    
    computedSize.height = aReflowState.ApplyMinMaxHeight(contentHeight) +
                          verticalBorderPadding;
  } else {
    computedSize.height += m.top + m.bottom;
  }

  nsRect r(mRect.x, mRect.y, computedSize.width, computedSize.height);

  SetBounds(state, r);
 
  
  Layout(state);
  
  
  
  
  nscoord ascent = mRect.height;

  
  
  if (!(mState & NS_STATE_IS_ROOT)) {
    ascent = GetBoxAscent(state);
  }

  aDesiredSize.Width() = mRect.width;
  aDesiredSize.Height() = mRect.height;
  aDesiredSize.SetBlockStartAscent(ascent);

  aDesiredSize.mOverflowAreas = GetOverflowAreas();

#ifdef DO_NOISY_REFLOW
  {
    printf("%p ** nsBF(done) W:%d H:%d  ", this, aDesiredSize.Width(), aDesiredSize.Height());

    if (maxElementSize) {
      printf("MW:%d\n", *maxElementWidth); 
    } else {
      printf("MW:?\n"); 
    }

  }
#endif

  ReflowAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsSize
nsBoxFrame::GetPrefSize(nsBoxLayoutState& aBoxLayoutState)
{
  NS_ASSERTION(aBoxLayoutState.GetRenderingContext(),
               "must have rendering context");

  nsSize size(0,0);
  DISPLAY_PREF_SIZE(this, size);
  if (!DoesNeedRecalc(mPrefSize)) {
     return mPrefSize;
  }

#ifdef DEBUG_LAYOUT
  PropagateDebug(aBoxLayoutState);
#endif

  if (IsCollapsed())
    return size;

  
  bool widthSet, heightSet;
  if (!nsIFrame::AddCSSPrefSize(this, size, widthSet, heightSet))
  {
    if (mLayoutManager) {
      nsSize layoutSize = mLayoutManager->GetPrefSize(this, aBoxLayoutState);
      if (!widthSet)
        size.width = layoutSize.width;
      if (!heightSet)
        size.height = layoutSize.height;
    }
    else {
      size = nsBox::GetPrefSize(aBoxLayoutState);
    }
  }

  nsSize minSize = GetMinSize(aBoxLayoutState);
  nsSize maxSize = GetMaxSize(aBoxLayoutState);
  mPrefSize = BoundsCheck(minSize, size, maxSize);
 
  return mPrefSize;
}

nscoord
nsBoxFrame::GetBoxAscent(nsBoxLayoutState& aBoxLayoutState)
{
  if (!DoesNeedRecalc(mAscent))
     return mAscent;

#ifdef DEBUG_LAYOUT
  PropagateDebug(aBoxLayoutState);
#endif

  if (IsCollapsed())
    return 0;

  if (mLayoutManager)
    mAscent = mLayoutManager->GetAscent(this, aBoxLayoutState);
  else
    mAscent = nsBox::GetBoxAscent(aBoxLayoutState);

  return mAscent;
}

nsSize
nsBoxFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState)
{
  NS_ASSERTION(aBoxLayoutState.GetRenderingContext(),
               "must have rendering context");

  nsSize size(0,0);
  DISPLAY_MIN_SIZE(this, size);
  if (!DoesNeedRecalc(mMinSize)) {
    return mMinSize;
  }

#ifdef DEBUG_LAYOUT
  PropagateDebug(aBoxLayoutState);
#endif

  if (IsCollapsed())
    return size;

  
  bool widthSet, heightSet;
  if (!nsIFrame::AddCSSMinSize(aBoxLayoutState, this, size, widthSet, heightSet))
  {
    if (mLayoutManager) {
      nsSize layoutSize = mLayoutManager->GetMinSize(this, aBoxLayoutState);
      if (!widthSet)
        size.width = layoutSize.width;
      if (!heightSet)
        size.height = layoutSize.height;
    }
    else {
      size = nsBox::GetMinSize(aBoxLayoutState);
    }
  }
  
  mMinSize = size;

  return size;
}

nsSize
nsBoxFrame::GetMaxSize(nsBoxLayoutState& aBoxLayoutState)
{
  NS_ASSERTION(aBoxLayoutState.GetRenderingContext(),
               "must have rendering context");

  nsSize size(NS_INTRINSICSIZE, NS_INTRINSICSIZE);
  DISPLAY_MAX_SIZE(this, size);
  if (!DoesNeedRecalc(mMaxSize)) {
    return mMaxSize;
  }

#ifdef DEBUG_LAYOUT
  PropagateDebug(aBoxLayoutState);
#endif

  if (IsCollapsed())
    return size;

  
  bool widthSet, heightSet;
  if (!nsIFrame::AddCSSMaxSize(this, size, widthSet, heightSet))
  {
    if (mLayoutManager) {
      nsSize layoutSize = mLayoutManager->GetMaxSize(this, aBoxLayoutState);
      if (!widthSet)
        size.width = layoutSize.width;
      if (!heightSet)
        size.height = layoutSize.height;
    }
    else {
      size = nsBox::GetMaxSize(aBoxLayoutState);
    }
  }

  mMaxSize = size;

  return size;
}

nscoord
nsBoxFrame::GetFlex(nsBoxLayoutState& aBoxLayoutState)
{
  if (!DoesNeedRecalc(mFlex))
     return mFlex;

  mFlex = nsBox::GetFlex(aBoxLayoutState);

  return mFlex;
}





NS_IMETHODIMP
nsBoxFrame::DoLayout(nsBoxLayoutState& aState)
{
  uint32_t oldFlags = aState.LayoutFlags();
  aState.SetLayoutFlags(0);

  nsresult rv = NS_OK;
  if (mLayoutManager) {
    CoordNeedsRecalc(mAscent);
    rv = mLayoutManager->Layout(this, aState);
  }

  aState.SetLayoutFlags(oldFlags);

  if (HasAbsolutelyPositionedChildren()) {
    
    WritingMode wm = GetWritingMode();
    nsHTMLReflowState reflowState(aState.PresContext(), this,
                                  aState.GetRenderingContext(),
                                  LogicalSize(wm, GetLogicalSize().ISize(wm),
                                              NS_UNCONSTRAINEDSIZE));

    
    nsHTMLReflowMetrics desiredSize(reflowState);
    desiredSize.Width() = mRect.width;
    desiredSize.Height() = mRect.height;

    
    nscoord ascent = mRect.height;

    
    
    if (!(mState & NS_STATE_IS_ROOT)) {
      ascent = GetBoxAscent(aState);
    }
    desiredSize.SetBlockStartAscent(ascent);
    desiredSize.mOverflowAreas = GetOverflowAreas();

    AddStateBits(NS_FRAME_IN_REFLOW);
    
    
    nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;
    ReflowAbsoluteFrames(aState.PresContext(), desiredSize,
                         reflowState, reflowStatus);
    RemoveStateBits(NS_FRAME_IN_REFLOW);
  }

  return rv;
}

void
nsBoxFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  RegUnregAccessKey(false);

  
  SetLayoutManager(nullptr);

  nsContainerFrame::DestroyFrom(aDestructRoot);
} 

#ifdef DEBUG_LAYOUT
nsresult
nsBoxFrame::SetDebug(nsBoxLayoutState& aState, bool aDebug)
{
  
  bool debugSet = mState & NS_STATE_CURRENTLY_IN_DEBUG;
  bool debugChanged = (!aDebug && debugSet) || (aDebug && !debugSet);

  
  if (debugChanged)
  {
     if (aDebug) {
         mState |= NS_STATE_CURRENTLY_IN_DEBUG;
     } else {
         mState &= ~NS_STATE_CURRENTLY_IN_DEBUG;
     }
 
     SetDebugOnChildList(aState, mFirstChild, aDebug);

    MarkIntrinsicWidthsDirty();
  }

  return NS_OK;
}
#endif

 void
nsBoxFrame::MarkIntrinsicWidthsDirty()
{
  SizeNeedsRecalc(mPrefSize);
  SizeNeedsRecalc(mMinSize);
  SizeNeedsRecalc(mMaxSize);
  CoordNeedsRecalc(mFlex);
  CoordNeedsRecalc(mAscent);

  if (mLayoutManager) {
    nsBoxLayoutState state(PresContext());
    mLayoutManager->IntrinsicWidthsDirty(this, state);
  }

  
  
}

void
nsBoxFrame::RemoveFrame(ChildListID     aListID,
                        nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(aListID == kPrincipalList, "We don't support out-of-flow kids");
  nsPresContext* presContext = PresContext();
  nsBoxLayoutState state(presContext);

  
  mFrames.RemoveFrame(aOldFrame);

  
  if (mLayoutManager)
    mLayoutManager->ChildrenRemoved(this, state, aOldFrame);

  
  aOldFrame->Destroy();

  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsBoxFrame::InsertFrames(ChildListID     aListID,
                         nsIFrame*       aPrevFrame,
                         nsFrameList&    aFrameList)
{
   NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
                "inserting after sibling frame with different parent");
   NS_ASSERTION(!aPrevFrame || mFrames.ContainsFrame(aPrevFrame),
                "inserting after sibling frame not in our child list");
   NS_PRECONDITION(aListID == kPrincipalList, "We don't support out-of-flow kids");
   nsBoxLayoutState state(PresContext());

   
   const nsFrameList::Slice& newFrames =
     mFrames.InsertFrames(this, aPrevFrame, aFrameList);

   
   if (mLayoutManager)
     mLayoutManager->ChildrenInserted(this, state, aPrevFrame, newFrames);

   
   
   
   
   CheckBoxOrder();

#ifdef DEBUG_LAYOUT
   
   if (mState & NS_STATE_CURRENTLY_IN_DEBUG)
       SetDebugOnChildList(state, mFrames.FirstChild(), true);
#endif

   PresContext()->PresShell()->
     FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                      NS_FRAME_HAS_DIRTY_CHILDREN);
}


void
nsBoxFrame::AppendFrames(ChildListID     aListID,
                         nsFrameList&    aFrameList)
{
   NS_PRECONDITION(aListID == kPrincipalList, "We don't support out-of-flow kids");
   nsBoxLayoutState state(PresContext());

   
   const nsFrameList::Slice& newFrames = mFrames.AppendFrames(this, aFrameList);

   
   if (mLayoutManager)
     mLayoutManager->ChildrenAppended(this, state, newFrames);

   
   
   
   
   CheckBoxOrder();

#ifdef DEBUG_LAYOUT
   
   if (mState & NS_STATE_CURRENTLY_IN_DEBUG)
       SetDebugOnChildList(state, mFrames.FirstChild(), true);
#endif

   
   if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
     PresContext()->PresShell()->
       FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                        NS_FRAME_HAS_DIRTY_CHILDREN);
   }
}

 nsContainerFrame*
nsBoxFrame::GetContentInsertionFrame()
{
  if (GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK)
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  return nsContainerFrame::GetContentInsertionFrame();
}

nsresult
nsBoxFrame::AttributeChanged(int32_t aNameSpaceID,
                             nsIAtom* aAttribute,
                             int32_t aModType)
{
  nsresult rv = nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                                   aModType);

  
  
  nsIAtom *tag = mContent->Tag();
  if ((tag == nsGkAtoms::window ||
       tag == nsGkAtoms::page ||
       tag == nsGkAtoms::dialog ||
       tag == nsGkAtoms::wizard) &&
      (nsGkAtoms::width == aAttribute ||
       nsGkAtoms::height == aAttribute ||
       nsGkAtoms::screenX == aAttribute ||
       nsGkAtoms::screenY == aAttribute ||
       nsGkAtoms::sizemode == aAttribute)) {
    return rv;
  }

  if (aAttribute == nsGkAtoms::width       ||
      aAttribute == nsGkAtoms::height      ||
      aAttribute == nsGkAtoms::align       ||
      aAttribute == nsGkAtoms::valign      ||
      aAttribute == nsGkAtoms::left        ||
      aAttribute == nsGkAtoms::top         ||
      aAttribute == nsGkAtoms::right        ||
      aAttribute == nsGkAtoms::bottom       ||
      aAttribute == nsGkAtoms::start        ||
      aAttribute == nsGkAtoms::end          ||
      aAttribute == nsGkAtoms::minwidth     ||
      aAttribute == nsGkAtoms::maxwidth     ||
      aAttribute == nsGkAtoms::minheight    ||
      aAttribute == nsGkAtoms::maxheight    ||
      aAttribute == nsGkAtoms::flex         ||
      aAttribute == nsGkAtoms::orient       ||
      aAttribute == nsGkAtoms::pack         ||
      aAttribute == nsGkAtoms::dir          ||
      aAttribute == nsGkAtoms::mousethrough ||
      aAttribute == nsGkAtoms::equalsize) {

    if (aAttribute == nsGkAtoms::align  ||
        aAttribute == nsGkAtoms::valign ||
        aAttribute == nsGkAtoms::orient  ||
        aAttribute == nsGkAtoms::pack    ||
#ifdef DEBUG_LAYOUT
        aAttribute == nsGkAtoms::debug   ||
#endif
        aAttribute == nsGkAtoms::dir) {

      mValign = nsBoxFrame::vAlign_Top;
      mHalign = nsBoxFrame::hAlign_Left;

      bool orient = true;
      GetInitialOrientation(orient); 
      if (orient)
        mState |= NS_STATE_IS_HORIZONTAL;
      else
        mState &= ~NS_STATE_IS_HORIZONTAL;

      bool normal = true;
      GetInitialDirection(normal);
      if (normal)
        mState |= NS_STATE_IS_DIRECTION_NORMAL;
      else
        mState &= ~NS_STATE_IS_DIRECTION_NORMAL;

      GetInitialVAlignment(mValign);
      GetInitialHAlignment(mHalign);

      bool equalSize = false;
      GetInitialEqualSize(equalSize); 
      if (equalSize)
        mState |= NS_STATE_EQUAL_SIZE;
      else
        mState &= ~NS_STATE_EQUAL_SIZE;

#ifdef DEBUG_LAYOUT
      bool debug = mState & NS_STATE_SET_TO_DEBUG;
      bool debugSet = GetInitialDebug(debug);
      if (debugSet) {
        mState |= NS_STATE_DEBUG_WAS_SET;

        if (debug)
          mState |= NS_STATE_SET_TO_DEBUG;
        else
          mState &= ~NS_STATE_SET_TO_DEBUG;
      } else {
        mState &= ~NS_STATE_DEBUG_WAS_SET;
      }
#endif

      bool autostretch = !!(mState & NS_STATE_AUTO_STRETCH);
      GetInitialAutoStretch(autostretch);
      if (autostretch)
        mState |= NS_STATE_AUTO_STRETCH;
      else
        mState &= ~NS_STATE_AUTO_STRETCH;
    }
    else if (aAttribute == nsGkAtoms::left ||
             aAttribute == nsGkAtoms::top ||
             aAttribute == nsGkAtoms::right ||
             aAttribute == nsGkAtoms::bottom ||
             aAttribute == nsGkAtoms::start ||
             aAttribute == nsGkAtoms::end) {
      mState &= ~NS_STATE_STACK_NOT_POSITIONED;
    }
    else if (aAttribute == nsGkAtoms::mousethrough) {
      UpdateMouseThrough();
    }

    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  }
  else if (aAttribute == nsGkAtoms::ordinal) {
    nsBoxLayoutState state(PresContext());
    nsIFrame* parent = GetParentBox(this);
    
    
    
    
    
    if (parent && !(GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
        StyleDisplay()->mDisplay != NS_STYLE_DISPLAY_POPUP) {
      parent->RelayoutChildAtOrdinal(state, this);
      
      PresContext()->PresShell()->
        FrameNeedsReflow(parent, nsIPresShell::eStyleChange,
                         NS_FRAME_IS_DIRTY);
    }
  }
  
  
  else if (aAttribute == nsGkAtoms::accesskey) {
    RegUnregAccessKey(true);
  }
  else if (aAttribute == nsGkAtoms::rows &&
           tag == nsGkAtoms::tree) {
    
    
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  }

  return rv;
}

#ifdef DEBUG_LAYOUT
void
nsBoxFrame::GetDebugPref(nsPresContext* aPresContext)
{
    gDebug = Preferences::GetBool("xul.debug.box");
}

class nsDisplayXULDebug : public nsDisplayItem {
public:
  nsDisplayXULDebug(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayXULDebug);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULDebug() {
    MOZ_COUNT_DTOR(nsDisplayXULDebug);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, nsRect aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {
    nsPoint rectCenter(aRect.x + aRect.width / 2, aRect.y + aRect.height / 2);
    static_cast<nsBoxFrame*>(mFrame)->
      DisplayDebugInfoFor(this, rectCenter - ToReferenceFrame());
    aOutFrames->AppendElement(this);
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("XULDebug", TYPE_XUL_DEBUG)
};

void
nsDisplayXULDebug::Paint(nsDisplayListBuilder* aBuilder,
                         nsRenderingContext* aCtx)
{
  static_cast<nsBoxFrame*>(mFrame)->
    PaintXULDebugOverlay(*aCtx, ToReferenceFrame());
}

static void
PaintXULDebugBackground(nsIFrame* aFrame, nsRenderingContext* aCtx,
                        const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsBoxFrame*>(aFrame)->PaintXULDebugBackground(*aCtx, aPt);
}
#endif

void
nsBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                             const nsRect&           aDirtyRect,
                             const nsDisplayListSet& aLists)
{
  bool forceLayer = false;
  uint32_t flags = 0;
  mozilla::layers::FrameMetrics::ViewID scrollTargetId =
    mozilla::layers::FrameMetrics::NULL_SCROLL_ID;

  if (GetContent()->IsXUL()) {
    
    if (GetContent()->HasAttr(kNameSpaceID_None, nsGkAtoms::layer)) {
      forceLayer = true;
    } else {
      nsIFrame* parent = GetParentBox(this);
      if (parent && parent->GetType() == nsGkAtoms::sliderFrame) {
        aBuilder->GetScrollbarInfo(&scrollTargetId, &flags);
        forceLayer = (scrollTargetId != layers::FrameMetrics::NULL_SCROLL_ID);
        nsLayoutUtils::SetScrollbarThumbLayerization(this, forceLayer);
      }
    }
    
    
    const nsStyleDisplay* styles = StyleDisplay();
    if (styles && styles->mAppearance == NS_THEME_WIN_EXCLUDE_GLASS) {
      aBuilder->AddWindowOpaqueRegion(
          nsRect(aBuilder->ToReferenceFrame(this), GetSize()));
    }
  }

  nsDisplayListCollection tempLists;
  const nsDisplayListSet& destination = forceLayer ? tempLists : aLists;

  DisplayBorderBackgroundOutline(aBuilder, destination);

#ifdef DEBUG_LAYOUT
  if (mState & NS_STATE_CURRENTLY_IN_DEBUG) {
    destination.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayGeneric(aBuilder, this, PaintXULDebugBackground,
                       "XULDebugBackground"));
    destination.Outlines()->AppendNewToTop(new (aBuilder)
      nsDisplayXULDebug(aBuilder, this));
  }
#endif

  BuildDisplayListForChildren(aBuilder, aDirtyRect, destination);

  
  DisplaySelectionOverlay(aBuilder, destination.Content());

  if (forceLayer) {
    
    
    
    
    nsDisplayList masterList;
    masterList.AppendToTop(tempLists.BorderBackground());
    masterList.AppendToTop(tempLists.BlockBorderBackgrounds());
    masterList.AppendToTop(tempLists.Floats());
    masterList.AppendToTop(tempLists.Content());
    masterList.AppendToTop(tempLists.PositionedDescendants());
    masterList.AppendToTop(tempLists.Outlines());

    
    aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayOwnLayer(aBuilder, this, &masterList, flags, scrollTargetId));
  }
}

void
nsBoxFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  nsIFrame* kid = mFrames.FirstChild();
  
  
  nsDisplayListSet set(aLists, aLists.BlockBorderBackgrounds());
  
  while (kid) {
    BuildDisplayListForChild(aBuilder, kid, aDirtyRect, set);
    kid = kid->GetNextSibling();
  }
}














#ifdef DEBUG_LAYOUT
void
nsBoxFrame::PaintXULDebugBackground(nsRenderingContext& aRenderingContext,
                                    nsPoint aPt)
{
  nsMargin border;
  GetBorder(border);

  nsMargin debugBorder;
  nsMargin debugMargin;
  nsMargin debugPadding;

  bool isHorizontal = IsHorizontal();

  GetDebugBorder(debugBorder);
  PixelMarginToTwips(GetPresContext(), debugBorder);

  GetDebugMargin(debugMargin);
  PixelMarginToTwips(GetPresContext(), debugMargin);

  GetDebugPadding(debugPadding);
  PixelMarginToTwips(GetPresContext(), debugPadding);

  nsRect inner(mRect);
  inner.MoveTo(aPt);
  inner.Deflate(debugMargin);
  inner.Deflate(border);
  

  nscolor color;
  if (isHorizontal) {
    color = NS_RGB(0,0,255);
  } else {
    color = NS_RGB(255,0,0);
  }

  aRenderingContext.SetColor(color);

  
  nsRect r(inner);
  r.width = debugBorder.left;
  aRenderingContext.FillRect(r);

  
  r = inner;
  r.height = debugBorder.top;
  aRenderingContext.FillRect(r);

  
  r = inner;
  r.x = r.x + r.width - debugBorder.right;
  r.width = debugBorder.right;
  aRenderingContext.FillRect(r);

  
  r = inner;
  r.y = r.y + r.height - debugBorder.bottom;
  r.height = debugBorder.bottom;
  aRenderingContext.FillRect(r);

  
  
  
  if (NS_SUBTREE_DIRTY(this)) {
     nsRect dirtyr(inner);
     aRenderingContext.SetColor(NS_RGB(0,255,0));
     aRenderingContext.DrawRect(dirtyr);
     aRenderingContext.SetColor(color);
  }
}

void
nsBoxFrame::PaintXULDebugOverlay(nsRenderingContext& aRenderingContext,
                                 nsPoint aPt)
  nsMargin border;
  GetBorder(border);

  nsMargin debugMargin;
  GetDebugMargin(debugMargin);
  PixelMarginToTwips(GetPresContext(), debugMargin);

  nsRect inner(mRect);
  inner.MoveTo(aPt);
  inner.Deflate(debugMargin);
  inner.Deflate(border);

  nscoord onePixel = GetPresContext()->IntScaledPixelsToTwips(1);

  kid = nsBox::GetChildBox(this);
  while (nullptr != kid) {
    bool isHorizontal = IsHorizontal();

    nscoord x, y, borderSize, spacerSize;
    
    nsRect cr(kid->mRect);
    nsMargin margin;
    kid->GetMargin(margin);
    cr.Inflate(margin);
    
    if (isHorizontal) 
    {
        cr.y = inner.y;
        x = cr.x;
        y = cr.y + onePixel;
        spacerSize = debugBorder.top - onePixel*4;
    } else {
        cr.x = inner.x;
        x = cr.y;
        y = cr.x + onePixel;
        spacerSize = debugBorder.left - onePixel*4;
    }

    nsBoxLayoutState state(GetPresContext());
    nscoord flex = kid->GetFlex(state);

    if (!kid->IsCollapsed()) {
      aRenderingContext.SetColor(NS_RGB(255,255,255));

      if (isHorizontal) 
          borderSize = cr.width;
      else 
          borderSize = cr.height;
    
      DrawSpacer(GetPresContext(), aRenderingContext, isHorizontal, flex, x, y, borderSize, spacerSize);
    }

    kid = GetNextBox(kid);
  }
}
#endif

#ifdef DEBUG_LAYOUT
void
nsBoxFrame::GetBoxName(nsAutoString& aName)
{
   GetFrameName(aName);
}
#endif

#ifdef DEBUG_FRAME_DUMP
nsresult
nsBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Box"), aResult);
}
#endif

nsIAtom*
nsBoxFrame::GetType() const
{
  return nsGkAtoms::boxFrame;
}

#ifdef DEBUG_LAYOUT
nsresult
nsBoxFrame::GetDebug(bool& aDebug)
{
  aDebug = (mState & NS_STATE_CURRENTLY_IN_DEBUG);
  return NS_OK;
}
#endif



































#ifdef DEBUG_LAYOUT
void
nsBoxFrame::DrawLine(nsRenderingContext& aRenderingContext, bool aHorizontal, nscoord x1, nscoord y1, nscoord x2, nscoord y2)
{
    if (aHorizontal)
       aRenderingContext.DrawLine(x1,y1,x2,y2);
    else
       aRenderingContext.DrawLine(y1,x1,y2,x2);
}

void
nsBoxFrame::FillRect(nsRenderingContext& aRenderingContext, bool aHorizontal, nscoord x, nscoord y, nscoord width, nscoord height)
{
    if (aHorizontal)
       aRenderingContext.FillRect(x,y,width,height);
    else
       aRenderingContext.FillRect(y,x,height,width);
}

void 
nsBoxFrame::DrawSpacer(nsPresContext* aPresContext, nsRenderingContext& aRenderingContext, bool aHorizontal, int32_t flex, nscoord x, nscoord y, nscoord size, nscoord spacerSize)
{    
         nscoord onePixel = aPresContext->IntScaledPixelsToTwips(1);

     
        int distance = 0;
        int center = 0;
        int offset = 0;
        int coilSize = COIL_SIZE*onePixel;
        int halfSpacer = spacerSize/2;

        distance = size;
        center = y + halfSpacer;
        offset = x;

        int coils = distance/coilSize;

        int halfCoilSize = coilSize/2;

        if (flex == 0) {
            DrawLine(aRenderingContext, aHorizontal, x,y + spacerSize/2, x + size, y + spacerSize/2);
        } else {
            for (int i=0; i < coils; i++)
            {
                   DrawLine(aRenderingContext, aHorizontal, offset, center+halfSpacer, offset+halfCoilSize, center-halfSpacer);
                   DrawLine(aRenderingContext, aHorizontal, offset+halfCoilSize, center-halfSpacer, offset+coilSize, center+halfSpacer);

                   offset += coilSize;
            }
        }

        FillRect(aRenderingContext, aHorizontal, x + size - spacerSize/2, y, spacerSize/2, spacerSize);
        FillRect(aRenderingContext, aHorizontal, x, y, spacerSize/2, spacerSize);

        
}

void
nsBoxFrame::GetDebugBorder(nsMargin& aInset)
{
    aInset.SizeTo(2,2,2,2);

    if (IsHorizontal()) 
       aInset.top = 10;
    else 
       aInset.left = 10;
}

void
nsBoxFrame::GetDebugMargin(nsMargin& aInset)
{
    aInset.SizeTo(2,2,2,2);
}

void
nsBoxFrame::GetDebugPadding(nsMargin& aPadding)
{
    aPadding.SizeTo(2,2,2,2);
}

void 
nsBoxFrame::PixelMarginToTwips(nsPresContext* aPresContext, nsMargin& aMarginPixels)
{
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  aMarginPixels.left   *= onePixel;
  aMarginPixels.right  *= onePixel;
  aMarginPixels.top    *= onePixel;
  aMarginPixels.bottom *= onePixel;
}

void
nsBoxFrame::GetValue(nsPresContext* aPresContext, const nsSize& a, const nsSize& b, char* ch) 
{
    float p2t = aPresContext->ScaledPixelsToTwips();

    char width[100];
    char height[100];
    
    if (a.width == NS_INTRINSICSIZE)
        sprintf(width,"%s","INF");
    else
        sprintf(width,"%d", nscoord(a.width));
    
    if (a.height == NS_INTRINSICSIZE)
        sprintf(height,"%s","INF");
    else 
        sprintf(height,"%d", nscoord(a.height));
    

    sprintf(ch, "(%s%s, %s%s)", width, (b.width != NS_INTRINSICSIZE ? "[SET]" : ""),
                    height, (b.height != NS_INTRINSICSIZE ? "[SET]" : ""));

}

void
nsBoxFrame::GetValue(nsPresContext* aPresContext, int32_t a, int32_t b, char* ch) 
{
    if (a == NS_INTRINSICSIZE)
      sprintf(ch, "%d[SET]", b);             
    else
      sprintf(ch, "%d", a);             
}

nsresult
nsBoxFrame::DisplayDebugInfoFor(nsIFrame*  aBox,
                                nsPoint& aPoint)
{
    nsBoxLayoutState state(GetPresContext());

    nscoord x = aPoint.x;
    nscoord y = aPoint.y;

    
    nsRect insideBorder(aBox->mRect);
    insideBorder.MoveTo(0,0):
    nsMargin border(0,0,0,0);
    aBox->GetBorderAndPadding(border);
    insideBorder.Deflate(border);

    bool isHorizontal = IsHorizontal();

    if (!insideBorder.Contains(nsPoint(x,y)))
        return NS_ERROR_FAILURE;

    

    int count = 0;
    nsIFrame* child = nsBox::GetChildBox(aBox);

    nsMargin m;
    nsMargin m2;
    GetDebugBorder(m);
    PixelMarginToTwips(aPresContext, m);

    GetDebugMargin(m2);
    PixelMarginToTwips(aPresContext, m2);

    m += m2;

    if ((isHorizontal && y < insideBorder.y + m.top) ||
        (!isHorizontal && x < insideBorder.x + m.left)) {
        
        while (child) 
        {
            const nsRect& r = child->mRect;

            
            if ((isHorizontal && x >= r.x && x < r.x + r.width) ||
                (!isHorizontal && y >= r.y && y < r.y + r.height)) {
                aCursor = NS_STYLE_CURSOR_POINTER;
                   
                    if (mDebugChild == child)
                        return NS_OK;

                    if (aBox->GetContent()) {
                      printf("---------------\n");
                      DumpBox(stdout);
                      printf("\n");
                    }

                    if (child->GetContent()) {
                        printf("child #%d: ", count);
                        child->DumpBox(stdout);
                        printf("\n");
                    }

                    mDebugChild = child;

                    nsSize prefSizeCSS(NS_INTRINSICSIZE, NS_INTRINSICSIZE);
                    nsSize minSizeCSS (NS_INTRINSICSIZE, NS_INTRINSICSIZE);
                    nsSize maxSizeCSS (NS_INTRINSICSIZE, NS_INTRINSICSIZE);
                    nscoord flexCSS = NS_INTRINSICSIZE;

                    bool widthSet, heightSet;
                    nsIFrame::AddCSSPrefSize(child, prefSizeCSS, widthSet, heightSet);
                    nsIFrame::AddCSSMinSize (state, child, minSizeCSS, widthSet, heightSet);
                    nsIFrame::AddCSSMaxSize (child, maxSizeCSS, widthSet, heightSet);
                    nsIFrame::AddCSSFlex    (state, child, flexCSS);

                    nsSize prefSize = child->GetPrefSize(state);
                    nsSize minSize = child->GetMinSize(state);
                    nsSize maxSize = child->GetMaxSize(state);
                    nscoord flexSize = child->GetFlex(state);
                    nscoord ascentSize = child->GetBoxAscent(state);

                    char min[100];
                    char pref[100];
                    char max[100];
                    char calc[100];
                    char flex[100];
                    char ascent[100];
                  
                    nsSize actualSize;
                    GetFrameSizeWithMargin(child, actualSize);
                    nsSize actualSizeCSS (NS_INTRINSICSIZE, NS_INTRINSICSIZE);

                    GetValue(aPresContext, minSize,  minSizeCSS, min);
                    GetValue(aPresContext, prefSize, prefSizeCSS, pref);
                    GetValue(aPresContext, maxSize,  maxSizeCSS, max);
                    GetValue(aPresContext, actualSize, actualSizeCSS, calc);
                    GetValue(aPresContext, flexSize,  flexCSS, flex);
                    GetValue(aPresContext, ascentSize,  NS_INTRINSICSIZE, ascent);


                    printf("min%s, pref%s, max%s, actual%s, flex=%s, ascent=%s\n\n", 
                        min,
                        pref,
                        max,
                        calc,
                        flex,
                        ascent
                    );

                    return NS_OK;   
            }

          child = GetNextBox(child);
          count++;
        }
    } else {
    }

    mDebugChild = nullptr;

    return NS_OK;
}

void
nsBoxFrame::SetDebugOnChildList(nsBoxLayoutState& aState, nsIFrame* aChild, bool aDebug)
{
    nsIFrame* child = nsBox::GetChildBox(this);
     while (child)
     {
        child->SetDebug(aState, aDebug);
        child = GetNextBox(child);
     }
}

nsresult
nsBoxFrame::GetFrameSizeWithMargin(nsIFrame* aBox, nsSize& aSize)
{
  nsRect rect(aBox->GetRect());
  nsMargin margin(0,0,0,0);
  aBox->GetMargin(margin);
  rect.Inflate(margin);
  aSize.width = rect.width;
  aSize.height = rect.height;
  return NS_OK;
}
#endif



void
nsBoxFrame::RegUnregAccessKey(bool aDoReg)
{
  MOZ_ASSERT(mContent);

  
  nsIAtom *atom = mContent->Tag();

  
  if (atom != nsGkAtoms::button &&
      atom != nsGkAtoms::toolbarbutton &&
      atom != nsGkAtoms::checkbox &&
      atom != nsGkAtoms::textbox &&
      atom != nsGkAtoms::tab &&
      atom != nsGkAtoms::radio) {
    return;
  }

  nsAutoString accessKey;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);

  if (accessKey.IsEmpty())
    return;

  
  
  EventStateManager* esm = PresContext()->EventStateManager();

  uint32_t key = accessKey.First();
  if (aDoReg)
    esm->RegisterAccessKey(mContent, key);
  else
    esm->UnregisterAccessKey(mContent, key);
}

bool
nsBoxFrame::SupportsOrdinalsInChildren()
{
  return true;
}



bool
IsBoxOrdinalLEQ(nsIFrame* aFrame1,
                nsIFrame* aFrame2)
{
  
  nsIFrame* aRealFrame1 = nsPlaceholderFrame::GetRealFrameFor(aFrame1);
  nsIFrame* aRealFrame2 = nsPlaceholderFrame::GetRealFrameFor(aFrame2);
  return aRealFrame1->GetOrdinal() <= aRealFrame2->GetOrdinal();
}

void 
nsBoxFrame::CheckBoxOrder()
{
  if (SupportsOrdinalsInChildren() &&
      !nsIFrame::IsFrameListSorted<IsBoxOrdinalLEQ>(mFrames)) {
    nsIFrame::SortFrameList<IsBoxOrdinalLEQ>(mFrames);
  }
}

nsresult
nsBoxFrame::LayoutChildAt(nsBoxLayoutState& aState, nsIFrame* aBox, const nsRect& aRect)
{
  
  nsRect oldRect(aBox->GetRect());
  aBox->SetBounds(aState, aRect);

  bool layout = NS_SUBTREE_DIRTY(aBox);
  
  if (layout || (oldRect.width != aRect.width || oldRect.height != aRect.height))  {
    return aBox->Layout(aState);
  }

  return NS_OK;
}

nsresult
nsBoxFrame::RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIFrame* aChild)
{
  if (!SupportsOrdinalsInChildren())
    return NS_OK;

  uint32_t ord = aChild->GetOrdinal();
  
  nsIFrame* child = mFrames.FirstChild();
  nsIFrame* newPrevSib = nullptr;

  while (child) {
    if (ord < child->GetOrdinal()) {
      break;
    }

    if (child != aChild) {
      newPrevSib = child;
    }

    child = GetNextBox(child);
  }

  if (aChild->GetPrevSibling() == newPrevSib) {
    
    return NS_OK;
  }

  
  mFrames.RemoveFrame(aChild);

  
  mFrames.InsertFrame(nullptr, newPrevSib, aChild);

  return NS_OK;
}























class nsDisplayXULEventRedirector : public nsDisplayWrapList {
public:
  nsDisplayXULEventRedirector(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame, nsDisplayItem* aItem,
                              nsIFrame* aTargetFrame)
    : nsDisplayWrapList(aBuilder, aFrame, aItem), mTargetFrame(aTargetFrame) {}
  nsDisplayXULEventRedirector(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame, nsDisplayList* aList,
                              nsIFrame* aTargetFrame)
    : nsDisplayWrapList(aBuilder, aFrame, aList), mTargetFrame(aTargetFrame) {}
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE {
    return false;
  }
  NS_DISPLAY_DECL_NAME("XULEventRedirector", TYPE_XUL_EVENT_REDIRECTOR)
private:
  nsIFrame* mTargetFrame;
};

void nsDisplayXULEventRedirector::HitTest(nsDisplayListBuilder* aBuilder,
    const nsRect& aRect, HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsTArray<nsIFrame*> outFrames;
  mList.HitTest(aBuilder, aRect, aState, &outFrames);

  bool topMostAdded = false;
  uint32_t localLength = outFrames.Length();

  for (uint32_t i = 0; i < localLength; i++) {

    for (nsIContent* content = outFrames.ElementAt(i)->GetContent();
         content && content != mTargetFrame->GetContent();
         content = content->GetParent()) {
      if (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::allowevents,
                               nsGkAtoms::_true, eCaseMatters)) {
        
        aOutFrames->AppendElement(outFrames.ElementAt(i));
        topMostAdded = true;
      }
    }

    
    
    if (!topMostAdded) {
      topMostAdded = true;
      aOutFrames->AppendElement(mTargetFrame);
    }
  }
}

class nsXULEventRedirectorWrapper : public nsDisplayWrapper
{
public:
  nsXULEventRedirectorWrapper(nsIFrame* aTargetFrame)
      : mTargetFrame(aTargetFrame) {}
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame,
                                  nsDisplayList* aList) MOZ_OVERRIDE {
    return new (aBuilder)
        nsDisplayXULEventRedirector(aBuilder, aFrame, aList, mTargetFrame);
  }
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) MOZ_OVERRIDE {
    return new (aBuilder)
        nsDisplayXULEventRedirector(aBuilder, aItem->Frame(), aItem,
                                    mTargetFrame);
  }
private:
  nsIFrame* mTargetFrame;
};

void
nsBoxFrame::WrapListsInRedirector(nsDisplayListBuilder*   aBuilder,
                                  const nsDisplayListSet& aIn,
                                  const nsDisplayListSet& aOut)
{
  nsXULEventRedirectorWrapper wrapper(this);
  wrapper.WrapLists(aBuilder, this, aIn, aOut);
}

bool
nsBoxFrame::GetEventPoint(WidgetGUIEvent* aEvent, nsPoint &aPoint) {
  nsIntPoint refPoint;
  bool res = GetEventPoint(aEvent, refPoint);
  aPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, refPoint, this);
  return res;
}

bool
nsBoxFrame::GetEventPoint(WidgetGUIEvent* aEvent, nsIntPoint &aPoint) {
  NS_ENSURE_TRUE(aEvent, false);

  WidgetTouchEvent* touchEvent = aEvent->AsTouchEvent();
  if (touchEvent) {
    
    
    if (touchEvent->touches.Length() != 1) {
      return false;
    }

    dom::Touch* touch = touchEvent->touches.SafeElementAt(0);
    if (!touch) {
      return false;
    }
    aPoint = touch->mRefPoint;
  } else {
    aPoint = LayoutDeviceIntPoint::ToUntyped(aEvent->refPoint);
  }
  return true;
}
