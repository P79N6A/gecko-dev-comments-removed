

































































#include "nsBoxLayoutState.h"
#include "nsBoxFrame.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsHTMLParts.h"
#include "nsIViewManager.h"
#include "nsIView.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsIServiceManager.h"
#include "nsIBoxLayout.h"
#include "nsSprocketLayout.h"
#include "nsIDocument.h"
#include "nsIScrollableFrame.h"
#include "nsWidgetsCID.h"
#include "nsCSSAnonBoxes.h"
#include "nsIScrollableView.h"
#include "nsHTMLContainerFrame.h"
#include "nsIEventStateManager.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsITheme.h"
#include "nsTransform2D.h"
#include "nsIEventStateManager.h"
#include "nsEventDispatcher.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"


#include "nsIDocument.h"
#include "nsIURI.h"


static NS_DEFINE_IID(kWidgetCID, NS_CHILD_CID);



#define DEBUG_SPRING_SIZE 8
#define DEBUG_BORDER_SIZE 2
#define COIL_SIZE 8



#ifdef DEBUG_rods

#endif

#ifdef DEBUG_LAYOUT
PRBool nsBoxFrame::gDebug = PR_FALSE;
nsIBox* nsBoxFrame::mDebugChild = nsnull;
#endif

nsIFrame*
NS_NewBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot, nsIBoxLayout* aLayoutManager)
{
  return new (aPresShell) nsBoxFrame(aPresShell, aContext, aIsRoot, aLayoutManager);
}

nsIFrame*
NS_NewBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsBoxFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsBoxFrame)

nsBoxFrame::nsBoxFrame(nsIPresShell* aPresShell,
                       nsStyleContext* aContext,
                       PRBool aIsRoot,
                       nsIBoxLayout* aLayoutManager) :
  nsContainerFrame(aContext),
  mMouseThrough(unset)
{
  mState |= NS_STATE_IS_HORIZONTAL;
  mState |= NS_STATE_AUTO_STRETCH;

  if (aIsRoot) 
     mState |= NS_STATE_IS_ROOT;

  mValign = vAlign_Top;
  mHalign = hAlign_Left;
  
  
  nsCOMPtr<nsIBoxLayout> layout = aLayoutManager;

  if (layout == nsnull) {
    NS_NewSprocketLayout(aPresShell, layout);
  }

  SetLayoutManager(layout);
}

nsBoxFrame::~nsBoxFrame()
{
}

NS_IMETHODIMP
nsBoxFrame::SetInitialChildList(nsIAtom*        aListName,
                                nsFrameList&    aChildList)
{
  nsresult r = nsContainerFrame::SetInitialChildList(aListName, aChildList);
  if (r == NS_OK) {
    
    nsBoxLayoutState state(PresContext());
    CheckBoxOrder(state);
    if (mLayoutManager)
      mLayoutManager->ChildrenSet(this, state, mFrames.FirstChild());
  } else {
    NS_WARNING("Warning add child failed!!\n");
  }

  return r;
}

 void
nsBoxFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsContainerFrame::DidSetStyleContext(aOldStyleContext);

  
  
  CacheAttributes();
}




NS_IMETHODIMP
nsBoxFrame::Init(nsIContent*      aContent,
                 nsIFrame*        aParent,
                 nsIFrame*        aPrevInFlow)
{
  nsresult  rv = nsContainerFrame::Init(aContent, aParent, aPrevInFlow);
  NS_ENSURE_SUCCESS(rv, rv);

  MarkIntrinsicWidthsDirty();

  CacheAttributes();

#ifdef DEBUG_LAYOUT
    
  if (mState & NS_STATE_IS_ROOT) 
      GetDebugPref(GetPresContext());
#endif

  mMouseThrough = unset;

  UpdateMouseThrough();

  
  rv = RegUnregAccessKey(PR_TRUE);

  return rv;
}

void nsBoxFrame::UpdateMouseThrough()
{
  if (mContent) {
    static nsIContent::AttrValuesArray strings[] =
      {&nsGkAtoms::never, &nsGkAtoms::always, nsnull};
    static const eMouseThrough values[] = {never, always};
    PRInt32 index = mContent->FindAttrValueIn(kNameSpaceID_None,
        nsGkAtoms::mousethrough, strings, eCaseMatters);
    if (index >= 0) {
      mMouseThrough = values[index];
    }
  }
}

PRBool
nsBoxFrame::GetMouseThrough() const
{
  switch(mMouseThrough)
  {
    case always:
      return PR_TRUE;
    case never:
      return PR_FALSE;
    case unset:
      if (mParent && mParent->IsBoxFrame())
        return mParent->GetMouseThrough();
  }

  return PR_FALSE;
}

void
nsBoxFrame::CacheAttributes()
{
  





  mValign = vAlign_Top;
  mHalign = hAlign_Left;

  PRBool orient = PR_FALSE;
  GetInitialOrientation(orient); 
  if (orient)
    mState |= NS_STATE_IS_HORIZONTAL;
  else
    mState &= ~NS_STATE_IS_HORIZONTAL;

  PRBool normal = PR_TRUE;
  GetInitialDirection(normal); 
  if (normal)
    mState |= NS_STATE_IS_DIRECTION_NORMAL;
  else
    mState &= ~NS_STATE_IS_DIRECTION_NORMAL;

  GetInitialVAlignment(mValign);
  GetInitialHAlignment(mHalign);
  
  PRBool equalSize = PR_FALSE;
  GetInitialEqualSize(equalSize); 
  if (equalSize)
        mState |= NS_STATE_EQUAL_SIZE;
    else
        mState &= ~NS_STATE_EQUAL_SIZE;

  PRBool autostretch = !!(mState & NS_STATE_AUTO_STRETCH);
  GetInitialAutoStretch(autostretch);
  if (autostretch)
        mState |= NS_STATE_AUTO_STRETCH;
     else
        mState &= ~NS_STATE_AUTO_STRETCH;


#ifdef DEBUG_LAYOUT
  PRBool debug = mState & NS_STATE_SET_TO_DEBUG;
  PRBool debugSet = GetInitialDebug(debug); 
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
PRBool
nsBoxFrame::GetInitialDebug(PRBool& aDebug)
{
  if (!GetContent())
    return PR_FALSE;

  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_false, &nsGkAtoms::_true, nsnull};
  PRInt32 index = GetContent()->FindAttrValueIn(kNameSpaceID_None,
      nsGkAtoms::debug, strings, eCaseMatters);
  if (index >= 0) {
    aDebug = index == 1;
    return PR_TRUE;
  }

  return PR_FALSE;
}
#endif

PRBool
nsBoxFrame::GetInitialHAlignment(nsBoxFrame::Halignment& aHalign)
{
  if (!GetContent())
    return PR_FALSE;

  
  static nsIContent::AttrValuesArray alignStrings[] =
    {&nsGkAtoms::left, &nsGkAtoms::right, nsnull};
  static const Halignment alignValues[] = {hAlign_Left, hAlign_Right};
  PRInt32 index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::align,
      alignStrings, eCaseMatters);
  if (index >= 0) {
    aHalign = alignValues[index];
    return PR_TRUE;
  }
      
  
  
  
  nsIAtom* attrName = IsHorizontal() ? nsGkAtoms::pack : nsGkAtoms::align;
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::start, &nsGkAtoms::center, &nsGkAtoms::end, nsnull};
  static const Halignment values[] =
    {hAlign_Left, hAlign_Left, hAlign_Center, hAlign_Right};
  index = GetContent()->FindAttrValueIn(kNameSpaceID_None, attrName,
      strings, eCaseMatters);

  if (index == nsIContent::ATTR_VALUE_NO_MATCH) {
    
    return PR_FALSE;
  }
  if (index > 0) {    
    aHalign = values[index];
    return PR_TRUE;
  }

  
  
  
  const nsStyleXUL* boxInfo = GetStyleXUL();
  if (IsHorizontal()) {
    switch (boxInfo->mBoxPack) {
      case NS_STYLE_BOX_PACK_START:
        aHalign = nsBoxFrame::hAlign_Left;
        return PR_TRUE;
      case NS_STYLE_BOX_PACK_CENTER:
        aHalign = nsBoxFrame::hAlign_Center;
        return PR_TRUE;
      case NS_STYLE_BOX_PACK_END:
        aHalign = nsBoxFrame::hAlign_Right;
        return PR_TRUE;
      default: 
        return PR_FALSE;
    }
  }
  else {
    switch (boxInfo->mBoxAlign) {
      case NS_STYLE_BOX_ALIGN_START:
        aHalign = nsBoxFrame::hAlign_Left;
        return PR_TRUE;
      case NS_STYLE_BOX_ALIGN_CENTER:
        aHalign = nsBoxFrame::hAlign_Center;
        return PR_TRUE;
      case NS_STYLE_BOX_ALIGN_END:
        aHalign = nsBoxFrame::hAlign_Right;
        return PR_TRUE;
      default: 
        return PR_FALSE;
    }
  }

  return PR_FALSE;
}

PRBool
nsBoxFrame::GetInitialVAlignment(nsBoxFrame::Valignment& aValign)
{
  if (!GetContent())
    return PR_FALSE;

  static nsIContent::AttrValuesArray valignStrings[] =
    {&nsGkAtoms::top, &nsGkAtoms::baseline, &nsGkAtoms::middle, &nsGkAtoms::bottom, nsnull};
  static const Valignment valignValues[] =
    {vAlign_Top, vAlign_BaseLine, vAlign_Middle, vAlign_Bottom};
  PRInt32 index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::valign,
      valignStrings, eCaseMatters);
  if (index >= 0) {
    aValign = valignValues[index];
    return PR_TRUE;
  }

  
  
  
  nsIAtom* attrName = IsHorizontal() ? nsGkAtoms::align : nsGkAtoms::pack;
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::start, &nsGkAtoms::center,
     &nsGkAtoms::baseline, &nsGkAtoms::end, nsnull};
  static const Valignment values[] =
    {vAlign_Top, vAlign_Top, vAlign_Middle, vAlign_BaseLine, vAlign_Bottom};
  index = GetContent()->FindAttrValueIn(kNameSpaceID_None, attrName,
      strings, eCaseMatters);
  if (index == nsIContent::ATTR_VALUE_NO_MATCH) {
    
    return PR_FALSE;
  }
  if (index > 0) {
    aValign = values[index];
    return PR_TRUE;
  }

  
  
  
  const nsStyleXUL* boxInfo = GetStyleXUL();
  if (IsHorizontal()) {
    switch (boxInfo->mBoxAlign) {
      case NS_STYLE_BOX_ALIGN_START:
        aValign = nsBoxFrame::vAlign_Top;
        return PR_TRUE;
      case NS_STYLE_BOX_ALIGN_CENTER:
        aValign = nsBoxFrame::vAlign_Middle;
        return PR_TRUE;
      case NS_STYLE_BOX_ALIGN_BASELINE:
        aValign = nsBoxFrame::vAlign_BaseLine;
        return PR_TRUE;
      case NS_STYLE_BOX_ALIGN_END:
        aValign = nsBoxFrame::vAlign_Bottom;
        return PR_TRUE;
      default: 
        return PR_FALSE;
    }
  }
  else {
    switch (boxInfo->mBoxPack) {
      case NS_STYLE_BOX_PACK_START:
        aValign = nsBoxFrame::vAlign_Top;
        return PR_TRUE;
      case NS_STYLE_BOX_PACK_CENTER:
        aValign = nsBoxFrame::vAlign_Middle;
        return PR_TRUE;
      case NS_STYLE_BOX_PACK_END:
        aValign = nsBoxFrame::vAlign_Bottom;
        return PR_TRUE;
      default: 
        return PR_FALSE;
    }
  }

  return PR_FALSE;
}

void
nsBoxFrame::GetInitialOrientation(PRBool& aIsHorizontal)
{
 
  if (!GetContent())
    return;

  
  const nsStyleXUL* boxInfo = GetStyleXUL();
  if (boxInfo->mBoxOrient == NS_STYLE_BOX_ORIENT_HORIZONTAL)
    aIsHorizontal = PR_TRUE;
  else 
    aIsHorizontal = PR_FALSE;

  
  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::vertical, &nsGkAtoms::horizontal, nsnull};
  PRInt32 index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::orient,
      strings, eCaseMatters);
  if (index >= 0) {
    aIsHorizontal = index == 1;
  }
}

void
nsBoxFrame::GetInitialDirection(PRBool& aIsNormal)
{
  if (!GetContent())
    return;

  if (IsHorizontal()) {
    
    
    aIsNormal = (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR); 
  }
  else
    aIsNormal = PR_TRUE; 

  
  const nsStyleXUL* boxInfo = GetStyleXUL();
  if (boxInfo->mBoxDirection == NS_STYLE_BOX_DIRECTION_REVERSE)
    aIsNormal = !aIsNormal; 
  
  
  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::reverse, &nsGkAtoms::ltr, &nsGkAtoms::rtl, nsnull};
  PRInt32 index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::dir,
      strings, eCaseMatters);
  if (index >= 0) {
    PRPackedBool values[] = {!aIsNormal, PR_TRUE, PR_FALSE};
    aIsNormal = values[index];
  }
}



PRBool
nsBoxFrame::GetInitialEqualSize(PRBool& aEqualSize)
{
 
  if (!GetContent())
     return PR_FALSE;

  if (GetContent()->AttrValueIs(kNameSpaceID_None, nsGkAtoms::equalsize,
                           nsGkAtoms::always, eCaseMatters)) {
    aEqualSize = PR_TRUE;
    return PR_TRUE;
  }

  return PR_FALSE;
}



PRBool
nsBoxFrame::GetInitialAutoStretch(PRBool& aStretch)
{
  if (!GetContent())
     return PR_FALSE;
  
  
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::_empty, &nsGkAtoms::stretch, nsnull};
  PRInt32 index = GetContent()->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::align,
      strings, eCaseMatters);
  if (index != nsIContent::ATTR_MISSING && index != 0) {
    aStretch = index == 1;
    return PR_TRUE;
  }

  
  const nsStyleXUL* boxInfo = GetStyleXUL();
  aStretch = (boxInfo->mBoxAlign == NS_STYLE_BOX_ALIGN_STRETCH);

  return PR_TRUE;
}

NS_IMETHODIMP
nsBoxFrame::DidReflow(nsPresContext*           aPresContext,
                      const nsHTMLReflowState*  aReflowState,
                      nsDidReflowStatus         aStatus)
{
  nsFrameState preserveBits =
    mState & (NS_FRAME_IS_DIRTY | NS_FRAME_HAS_DIRTY_CHILDREN);
  nsresult rv = nsFrame::DidReflow(aPresContext, aReflowState, aStatus);
  mState |= preserveBits;
  return rv;
}

PRBool
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
nsBoxFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  nsBoxLayoutState state(PresContext(), aRenderingContext);
  nsSize minSize = GetMinSize(state);

  
  
  
  
  nsMargin bp;
  GetBorderAndPadding(bp);

  result = minSize.width - bp.LeftRight();
  result = NS_MAX(result, 0);

  return result;
}

 nscoord
nsBoxFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  nsBoxLayoutState state(PresContext(), aRenderingContext);
  nsSize prefSize = GetPrefSize(state);

  
  
  
  
  nsMargin bp;
  GetBorderAndPadding(bp);

  result = prefSize.width - bp.LeftRight();
  result = NS_MAX(result, 0);

  return result;
}

NS_IMETHODIMP
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
  
  printSize("AW", aReflowState.availableWidth);
  printSize("AH", aReflowState.availableHeight);
  printSize("CW", aReflowState.ComputedWidth());
  printSize("CH", aReflowState.ComputedHeight());

  printf(" *\n");

#endif

  aStatus = NS_FRAME_COMPLETE;

  
  nsBoxLayoutState state(aPresContext, aReflowState.rendContext,
                         aReflowState.mReflowDepth);

  nsSize computedSize(aReflowState.ComputedWidth(),aReflowState.ComputedHeight());

  nsMargin m;
  m = aReflowState.mComputedBorderPadding;
  

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
    
    
    nscoord outsideBoxSizing = 0;
    switch (GetStylePosition()->mBoxSizing) {
      case NS_STYLE_BOX_SIZING_CONTENT:
        outsideBoxSizing = aReflowState.mComputedBorderPadding.TopBottom();
        
      case NS_STYLE_BOX_SIZING_PADDING:
        outsideBoxSizing -= aReflowState.mComputedPadding.TopBottom();
        break;
    }
    computedSize.height -= outsideBoxSizing;
    
    
    aReflowState.ApplyMinMaxConstraints(nsnull, &computedSize.height);
    computedSize.height += outsideBoxSizing;
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

  aDesiredSize.width  = mRect.width;
  aDesiredSize.height = mRect.height;
  aDesiredSize.ascent = ascent;

  aDesiredSize.mOverflowArea = GetOverflowRect();

#ifdef DO_NOISY_REFLOW
  {
    printf("%p ** nsBF(done) W:%d H:%d  ", this, aDesiredSize.width, aDesiredSize.height);

    if (maxElementSize) {
      printf("MW:%d\n", *maxElementWidth); 
    } else {
      printf("MW:?\n"); 
    }

  }
#endif

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

nsSize
nsBoxFrame::GetPrefSize(nsBoxLayoutState& aBoxLayoutState)
{
  NS_ASSERTION(aBoxLayoutState.GetRenderingContext(),
               "must have rendering context");

  nsSize size(0,0);
  DISPLAY_PREF_SIZE(this, size);
  if (!DoesNeedRecalc(mPrefSize)) {
     size = mPrefSize;
     return size;
  }

#ifdef DEBUG_LAYOUT
  PropagateDebug(aBoxLayoutState);
#endif

  if (IsCollapsed(aBoxLayoutState))
    return size;

  
  if (!nsIBox::AddCSSPrefSize(aBoxLayoutState, this, size))
  {
    if (mLayoutManager) {
      size = mLayoutManager->GetPrefSize(this, aBoxLayoutState);
      nsIBox::AddCSSPrefSize(aBoxLayoutState, this, size);
    } else
      size = nsBox::GetPrefSize(aBoxLayoutState);
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

  if (IsCollapsed(aBoxLayoutState))
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
    size = mMinSize;
    return size;
  }

#ifdef DEBUG_LAYOUT
  PropagateDebug(aBoxLayoutState);
#endif

  if (IsCollapsed(aBoxLayoutState))
    return size;

  
  if (!nsIBox::AddCSSMinSize(aBoxLayoutState, this, size))
  {
    if (mLayoutManager) {
      size = mLayoutManager->GetMinSize(this, aBoxLayoutState);
      nsIBox::AddCSSMinSize(aBoxLayoutState, this, size);
    } else {
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
    size = mMaxSize;
    return size;
  }

#ifdef DEBUG_LAYOUT
  PropagateDebug(aBoxLayoutState);
#endif

  if (IsCollapsed(aBoxLayoutState))
    return size;

  
  if (!nsIBox::AddCSSMaxSize(aBoxLayoutState, this, size))
  {
    if (mLayoutManager) {
      size = mLayoutManager->GetMaxSize(this, aBoxLayoutState);
      nsIBox::AddCSSMaxSize(aBoxLayoutState, this, size);
    } else {
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
  PRUint32 oldFlags = aState.LayoutFlags();
  aState.SetLayoutFlags(0);

  nsresult rv = NS_OK;
  if (mLayoutManager)
    rv = mLayoutManager->Layout(this, aState);

  aState.SetLayoutFlags(oldFlags);

  return rv;
}

void
nsBoxFrame::Destroy()
{
  
  RegUnregAccessKey(PR_FALSE);

  
  SetLayoutManager(nsnull);

  nsContainerFrame::Destroy();
} 

#ifdef DEBUG_LAYOUT
NS_IMETHODIMP
nsBoxFrame::SetDebug(nsBoxLayoutState& aState, PRBool aDebug)
{
  
  PRBool debugSet = mState & NS_STATE_CURRENTLY_IN_DEBUG;
  PRBool debugChanged = (!aDebug && debugSet) || (aDebug && !debugSet);

  
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

NS_IMETHODIMP
nsBoxFrame::RemoveFrame(nsIAtom*        aListName,
                        nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(!aListName, "We don't support out-of-flow kids");
  nsPresContext* presContext = PresContext();
  nsBoxLayoutState state(presContext);

  
  mFrames.RemoveFrame(aOldFrame);

  
  if (mLayoutManager)
    mLayoutManager->ChildrenRemoved(this, state, aOldFrame);

  
  aOldFrame->Destroy();

  
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
  return NS_OK;
}

NS_IMETHODIMP
nsBoxFrame::InsertFrames(nsIAtom*        aListName,
                         nsIFrame*       aPrevFrame,
                         nsFrameList&    aFrameList)
{
   NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
                "inserting after sibling frame with different parent");
   NS_ASSERTION(!aPrevFrame || mFrames.ContainsFrame(aPrevFrame),
                "inserting after sibling frame not in our child list");
   NS_PRECONDITION(!aListName, "We don't support out-of-flow kids");
   nsBoxLayoutState state(PresContext());

   
   const nsFrameList::Slice& newFrames =
     mFrames.InsertFrames(this, aPrevFrame, aFrameList);

   
   if (mLayoutManager)
     mLayoutManager->ChildrenInserted(this, state, aPrevFrame, newFrames);

#ifdef DEBUG_LAYOUT
   
   if (mState & NS_STATE_CURRENTLY_IN_DEBUG)
       SetDebugOnChildList(state, mFrames.FirstChild(), PR_TRUE);
#endif

   PresContext()->PresShell()->
     FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                      NS_FRAME_HAS_DIRTY_CHILDREN);
   return NS_OK;
}


NS_IMETHODIMP
nsBoxFrame::AppendFrames(nsIAtom*        aListName,
                         nsFrameList&    aFrameList)
{
   NS_PRECONDITION(!aListName, "We don't support out-of-flow kids");
   nsBoxLayoutState state(PresContext());

   
   const nsFrameList::Slice& newFrames = mFrames.AppendFrames(this, aFrameList);

   
   if (mLayoutManager)
     mLayoutManager->ChildrenAppended(this, state, newFrames);

#ifdef DEBUG_LAYOUT
   
   if (mState & NS_STATE_CURRENTLY_IN_DEBUG)
       SetDebugOnChildList(state, mFrames.FirstChild(), PR_TRUE);
#endif

   
   if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
     PresContext()->PresShell()->
       FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                        NS_FRAME_HAS_DIRTY_CHILDREN);
   }
   return NS_OK;
}

 nsIFrame*
nsBoxFrame::GetContentInsertionFrame()
{
  if (GetStateBits() & NS_STATE_BOX_WRAPS_KIDS_IN_BLOCK)
    return GetFirstChild(nsnull)->GetContentInsertionFrame();
  return nsContainerFrame::GetContentInsertionFrame();
}

NS_IMETHODIMP
nsBoxFrame::AttributeChanged(PRInt32 aNameSpaceID,
                             nsIAtom* aAttribute,
                             PRInt32 aModType)
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

      PRBool orient = PR_TRUE;
      GetInitialOrientation(orient); 
      if (orient)
        mState |= NS_STATE_IS_HORIZONTAL;
      else
        mState &= ~NS_STATE_IS_HORIZONTAL;

      PRBool normal = PR_TRUE;
      GetInitialDirection(normal);
      if (normal)
        mState |= NS_STATE_IS_DIRECTION_NORMAL;
      else
        mState &= ~NS_STATE_IS_DIRECTION_NORMAL;

      GetInitialVAlignment(mValign);
      GetInitialHAlignment(mHalign);

      PRBool equalSize = PR_FALSE;
      GetInitialEqualSize(equalSize); 
      if (equalSize)
        mState |= NS_STATE_EQUAL_SIZE;
      else
        mState &= ~NS_STATE_EQUAL_SIZE;

#ifdef DEBUG_LAYOUT
      PRBool debug = mState & NS_STATE_SET_TO_DEBUG;
      PRBool debugSet = GetInitialDebug(debug);
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

      PRBool autostretch = !!(mState & NS_STATE_AUTO_STRETCH);
      GetInitialAutoStretch(autostretch);
      if (autostretch)
        mState |= NS_STATE_AUTO_STRETCH;
      else
        mState &= ~NS_STATE_AUTO_STRETCH;
    }
    else if (aAttribute == nsGkAtoms::left ||
             aAttribute == nsGkAtoms::top ||
             aAttribute == nsGkAtoms::right ||
             aAttribute == nsGkAtoms::bottom) {
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
    nsIBox* parent = GetParentBox();
    
    
    
    
    
    if (parent && !(GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
        GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_POPUP) {
      parent->RelayoutChildAtOrdinal(state, this);
      
      PresContext()->PresShell()->
        FrameNeedsReflow(parent, nsIPresShell::eStyleChange,
                         NS_FRAME_IS_DIRTY);
    }
  }
  
  
  else if (aAttribute == nsGkAtoms::accesskey) {
    RegUnregAccessKey(PR_TRUE);
  }

  return rv;
}

#ifdef DEBUG_LAYOUT
void
nsBoxFrame::GetDebugPref(nsPresContext* aPresContext)
{
    gDebug = nsContentUtils::GetBoolPref("xul.debug.box");
}

class nsDisplayXULDebug : public nsDisplayItem {
public:
  nsDisplayXULDebug(nsIFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayXULDebug);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULDebug() {
    MOZ_COUNT_DTOR(nsDisplayXULDebug);
  }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState) {
    static_cast<nsBoxFrame*>(mFrame)->
      DisplayDebugInfoFor(this, aPt - aBuilder->ToReferenceFrame(mFrame));
    return PR_TRUE;
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder
                     nsIRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("XULDebug")
};

void
nsDisplayXULDebug::Paint(nsDisplayListBuilder* aBuilder,
                         nsIRenderingContext* aCtx)
{
  static_cast<nsBoxFrame*>(mFrame)->
    PaintXULDebugOverlay(*aCtx, aBuilder->ToReferenceFrame(mFrame));
}

static void
PaintXULDebugBackground(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                        const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsBoxFrame*>(aFrame)->PaintXULDebugBackground(*aCtx, aPt);
}
#endif

nsresult
nsBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                             const nsRect&           aDirtyRect,
                             const nsDisplayListSet& aLists)
{
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG_LAYOUT
  
  if (mState & NS_STATE_CURRENTLY_IN_DEBUG) {
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(this, PaintXULDebugBackground, "XULDebugBackground"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aLists.Outlines()->AppendNewToTop(new (aBuilder)
        nsDisplayXULDebug(this));
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif

  rv = BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return DisplaySelectionOverlay(aBuilder, aLists);
}

NS_IMETHODIMP
nsBoxFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                        const nsRect&           aDirtyRect,
                                        const nsDisplayListSet& aLists)
{
  nsIFrame* kid = mFrames.FirstChild();
  
  
  nsDisplayListSet set(aLists, aLists.BlockBorderBackgrounds());
  
  while (kid) {
    nsresult rv = BuildDisplayListForChild(aBuilder, kid, aDirtyRect, set);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  return NS_OK;
}














#ifdef DEBUG_LAYOUT
void
nsBoxFrame::PaintXULDebugBackground(nsIRenderingContext& aRenderingContext,
                                    nsPoint aPt)
{
  nsMargin border;
  GetBorder(border);

  nsMargin debugBorder;
  nsMargin debugMargin;
  nsMargin debugPadding;

  PRBool isHorizontal = IsHorizontal();

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
nsBoxFrame::PaintXULDebugOverlay(nsIRenderingContext& aRenderingContext,
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

  kid = GetChildBox();
  while (nsnull != kid) {
    PRBool isHorizontal = IsHorizontal();

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

    if (!kid->IsCollapsed(state)) {
      aRenderingContext.SetColor(NS_RGB(255,255,255));

      if (isHorizontal) 
          borderSize = cr.width;
      else 
          borderSize = cr.height;
    
      DrawSpacer(GetPresContext(), aRenderingContext, isHorizontal, flex, x, y, borderSize, spacerSize);
    }

    kid = kid->GetNextBox();
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

#ifdef DEBUG
NS_IMETHODIMP
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
NS_IMETHODIMP
nsBoxFrame::GetDebug(PRBool& aDebug)
{
  aDebug = (mState & NS_STATE_CURRENTLY_IN_DEBUG);
  return NS_OK;
}
#endif



































#ifdef DEBUG_LAYOUT
void
nsBoxFrame::DrawLine(nsIRenderingContext& aRenderingContext, PRBool aHorizontal, nscoord x1, nscoord y1, nscoord x2, nscoord y2)
{
    if (aHorizontal)
       aRenderingContext.DrawLine(x1,y1,x2,y2);
    else
       aRenderingContext.DrawLine(y1,x1,y2,x2);
}

void
nsBoxFrame::FillRect(nsIRenderingContext& aRenderingContext, PRBool aHorizontal, nscoord x, nscoord y, nscoord width, nscoord height)
{
    if (aHorizontal)
       aRenderingContext.FillRect(x,y,width,height);
    else
       aRenderingContext.FillRect(y,x,height,width);
}

void 
nsBoxFrame::DrawSpacer(nsPresContext* aPresContext, nsIRenderingContext& aRenderingContext, PRBool aHorizontal, PRInt32 flex, nscoord x, nscoord y, nscoord size, nscoord spacerSize)
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
nsBoxFrame::GetValue(nsPresContext* aPresContext, PRInt32 a, PRInt32 b, char* ch) 
{
    if (a == NS_INTRINSICSIZE)
      sprintf(ch, "%d[SET]", b);             
    else
      sprintf(ch, "%d", a);             
}

nsresult
nsBoxFrame::DisplayDebugInfoFor(nsIBox*  aBox,
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

    PRBool isHorizontal = IsHorizontal();

    if (!insideBorder.Contains(nsPoint(x,y)))
        return NS_ERROR_FAILURE;

    

    int count = 0;
    nsIBox* child = aBox->GetChildBox();

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

                    nsIBox::AddCSSPrefSize(state, child, prefSizeCSS);
                    nsIBox::AddCSSMinSize (state, child, minSizeCSS);
                    nsIBox::AddCSSMaxSize (state, child, maxSizeCSS);
                    nsIBox::AddCSSFlex    (state, child, flexCSS);

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

          child = child->GetNextBox();
          count++;
        }
    } else {
    }

    mDebugChild = nsnull;

    return NS_OK;
}

void
nsBoxFrame::SetDebugOnChildList(nsBoxLayoutState& aState, nsIBox* aChild, PRBool aDebug)
{
    nsIBox* child = GetChildBox();
     while (child)
     {
        child->SetDebug(aState, aDebug);
        child = child->GetNextBox();
     }
}

nsresult
nsBoxFrame::GetFrameSizeWithMargin(nsIBox* aBox, nsSize& aSize)
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






nsresult
nsBoxFrame::CreateViewForFrame(nsPresContext*  aPresContext,
                               nsIFrame*        aFrame,
                               nsStyleContext*  aStyleContext,
                               PRBool           aForce,
                               PRBool           aIsPopup)
{
  NS_ASSERTION(aForce, "We only get called to force view creation now");
  
  if (!aFrame->HasView()) {
    nsViewVisibility visibility = nsViewVisibility_kShow;
    PRInt32 zIndex = 0;
    PRBool  autoZIndex = PR_FALSE;

    if (aForce) {
      nsIView* parentView;
      nsIViewManager* viewManager = aPresContext->GetPresShell()->GetViewManager();
      NS_ASSERTION(nsnull != viewManager, "null view manager");

      
      if (aIsPopup) {
        viewManager->GetRootView(parentView);
        visibility = nsViewVisibility_kHide;
        zIndex = PR_INT32_MAX;
      }
      else {
        parentView = aFrame->GetParent()->GetParentViewForChildFrame(aFrame);
      }

      NS_ASSERTION(parentView, "no parent view");

      
      nsIView *view = viewManager->CreateView(aFrame->GetRect(), parentView, visibility);
      if (view) {
        
        
        nsIScrollableView*  scrollingView = parentView->ToScrollableView();
        if (scrollingView) {
          scrollingView->SetScrolledView(view);
        } else {
          viewManager->SetViewZIndex(view, autoZIndex, zIndex);
          
          viewManager->InsertChild(parentView, view, nsnull, PR_TRUE);
        }
      }

      
      aFrame->SetView(view);

      NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
        ("nsBoxFrame::CreateViewForFrame: frame=%p view=%p",
         aFrame));
      if (!view)
        return NS_ERROR_OUT_OF_MEMORY;
    }
  }
  return NS_OK;
}



nsresult
nsBoxFrame::RegUnregAccessKey(PRBool aDoReg)
{
  
  if (!mContent)
    return NS_ERROR_FAILURE;

  
  nsIAtom *atom = mContent->Tag();

  
  if (atom != nsGkAtoms::button &&
      atom != nsGkAtoms::toolbarbutton &&
      atom != nsGkAtoms::checkbox &&
      atom != nsGkAtoms::textbox &&
      atom != nsGkAtoms::tab &&
      atom != nsGkAtoms::radio) {
    return NS_OK;
  }

  nsAutoString accessKey;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);

  if (accessKey.IsEmpty())
    return NS_OK;

  
  
  nsIEventStateManager *esm = PresContext()->EventStateManager();

  nsresult rv;

  PRUint32 key = accessKey.First();
  if (aDoReg)
    rv = esm->RegisterAccessKey(mContent, key);
  else
    rv = esm->UnregisterAccessKey(mContent, key);

  return rv;
}

void
nsBoxFrame::FireDOMEventSynch(const nsAString& aDOMEventName, nsIContent *aContent)
{
  
  
  nsPresContext *presContext = PresContext();
  nsIContent *content = aContent ? aContent : mContent;
  if (content && presContext) {
    
    nsCOMPtr<nsIDOMEvent> event;
    if (NS_SUCCEEDED(nsEventDispatcher::CreateEvent(presContext, nsnull,
                                                    NS_LITERAL_STRING("Events"),
                                                    getter_AddRefs(event)))) {
      event->InitEvent(aDOMEventName, PR_TRUE, PR_TRUE);

      nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
      privateEvent->SetTrusted(PR_TRUE);

      nsEventDispatcher::DispatchDOMEvent(content, nsnull, event,
                                          presContext, nsnull);
    }
  }
}

PRBool
nsBoxFrame::SupportsOrdinalsInChildren()
{
  return PR_TRUE;
}

static nsIFrame*
SortedMerge(nsBoxLayoutState& aState, nsIFrame *aLeft, nsIFrame *aRight)
{
  NS_PRECONDITION(aLeft && aRight, "SortedMerge must have non-empty lists");

  nsIFrame *result;
  
  if (aLeft->GetOrdinal(aState) <= aRight->GetOrdinal(aState)) {
    result = aLeft;
    aLeft = aLeft->GetNextSibling();
    if (!aLeft) {
      result->SetNextSibling(aRight);
      return result;
    }
  }
  else {
    result = aRight;
    aRight = aRight->GetNextSibling();
    if (!aRight) {
      result->SetNextSibling(aLeft);
      return result;
    }
  }

  nsIFrame *last = result;
  for (;;) {
    if (aLeft->GetOrdinal(aState) <= aRight->GetOrdinal(aState)) {
      last->SetNextSibling(aLeft);
      last = aLeft;
      aLeft = aLeft->GetNextSibling();
      if (!aLeft) {
        last->SetNextSibling(aRight);
        return result;
      }
    }
    else {
      last->SetNextSibling(aRight);
      last = aRight;
      aRight = aRight->GetNextSibling();
      if (!aRight) {
        last->SetNextSibling(aLeft);
        return result;
      }
    }
  }
}

static nsIFrame*
MergeSort(nsBoxLayoutState& aState, nsIFrame *aSource)
{
  NS_PRECONDITION(aSource, "MergeSort null arg");

  nsIFrame *sorted[32] = { nsnull };
  nsIFrame **fill = &sorted[0];
  nsIFrame **left;
  nsIFrame *rest = aSource;

  do {
    nsIFrame *current = rest;
    rest = rest->GetNextSibling();
    current->SetNextSibling(nsnull);

    
    
    
    
    for (left = &sorted[0]; left != fill && *left; ++left) {
      current = SortedMerge(aState, *left, current);
      *left = nsnull;
    }

    
    *left = current;

    if (left == fill)
      ++fill;
  } while (rest);

  
  nsIFrame *result = nsnull;
  for (left = &sorted[0]; left != fill; ++left) {
    if (*left) {
      result = result ? SortedMerge(aState, *left, result) : *left;
    }
  }
  return result;
}

void 
nsBoxFrame::CheckBoxOrder(nsBoxLayoutState& aState)
{
  nsIFrame *child = mFrames.FirstChild();
  if (!child)
    return;

  if (!SupportsOrdinalsInChildren())
    return;

  
  
  PRUint32 maxOrdinal = child->GetOrdinal(aState);
  child = child->GetNextSibling();
  for ( ; child; child = child->GetNextSibling()) {
    PRUint32 ordinal = child->GetOrdinal(aState);
    if (ordinal < maxOrdinal)
      break;
    maxOrdinal = ordinal;
  }

  if (!child)
    return;

  nsIFrame* head = MergeSort(aState, mFrames.FirstChild());
  mFrames = nsFrameList(head, nsLayoutUtils::GetLastSibling(head));
}

NS_IMETHODIMP
nsBoxFrame::SetLayoutManager(nsIBoxLayout* aLayout)
{
  mLayoutManager = aLayout;
  return NS_OK;
}

NS_IMETHODIMP
nsBoxFrame::GetLayoutManager(nsIBoxLayout** aLayout)
{
  *aLayout = mLayoutManager;
  NS_IF_ADDREF(*aLayout);
  return NS_OK;
}

nsresult
nsBoxFrame::LayoutChildAt(nsBoxLayoutState& aState, nsIBox* aBox, const nsRect& aRect)
{
  
  nsRect oldRect(aBox->GetRect());
  aBox->SetBounds(aState, aRect);

  PRBool layout = NS_SUBTREE_DIRTY(aBox);
  
  if (layout || (oldRect.width != aRect.width || oldRect.height != aRect.height))  {
    return aBox->Layout(aState);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsBoxFrame::RelayoutChildAtOrdinal(nsBoxLayoutState& aState, nsIBox* aChild)
{
  if (!SupportsOrdinalsInChildren())
    return NS_OK;

  PRUint32 ord = aChild->GetOrdinal(aState);
  
  nsIFrame* child = mFrames.FirstChild();
  nsIFrame* newPrevSib = nsnull;

  while (child) {
    if (ord < child->GetOrdinal(aState)) {
      break;
    }

    if (child != aChild) {
      newPrevSib = child;
    }

    child = child->GetNextBox();
  }

  if (aChild->GetPrevSibling() == newPrevSib) {
    
    return NS_OK;
  }

  
  mFrames.RemoveFrame(aChild);

  
  mFrames.InsertFrame(nsnull, newPrevSib, aChild);

  return NS_OK;
}























class nsDisplayXULEventRedirector : public nsDisplayWrapList {
public:
  nsDisplayXULEventRedirector(nsIFrame* aFrame, nsDisplayItem* aItem,
                              nsIFrame* aTargetFrame)
    : nsDisplayWrapList(aFrame, aItem), mTargetFrame(aTargetFrame) {}
  nsDisplayXULEventRedirector(nsIFrame* aFrame, nsDisplayList* aList,
                              nsIFrame* aTargetFrame)
    : nsDisplayWrapList(aFrame, aList), mTargetFrame(aTargetFrame) {}
  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState);
  NS_DISPLAY_DECL_NAME("XULEventRedirector")
private:
  nsIFrame* mTargetFrame;
};

nsIFrame* nsDisplayXULEventRedirector::HitTest(nsDisplayListBuilder* aBuilder,
    nsPoint aPt, HitTestState* aState)
{
  nsIFrame* frame = mList.HitTest(aBuilder, aPt, aState);
  if (!frame)
    return nsnull;

  for (nsIContent* content = frame->GetContent();
       content && content != mTargetFrame->GetContent();
       content = content->GetParent()) {
    if (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::allowevents,
                             nsGkAtoms::_true, eCaseMatters)) {
      
      return frame;
    }
  }
  
  return mTargetFrame;
}

class nsXULEventRedirectorWrapper : public nsDisplayWrapper
{
public:
  nsXULEventRedirectorWrapper(nsIFrame* aTargetFrame)
      : mTargetFrame(aTargetFrame) {}
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) {
    return new (aBuilder)
        nsDisplayXULEventRedirector(aFrame, aList, mTargetFrame);
  }
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) {
    return new (aBuilder)
        nsDisplayXULEventRedirector(aItem->GetUnderlyingFrame(), aItem,
                                    mTargetFrame);
  }
private:
  nsIFrame* mTargetFrame;
};

nsresult
nsBoxFrame::WrapListsInRedirector(nsDisplayListBuilder*   aBuilder,
                                  const nsDisplayListSet& aIn,
                                  const nsDisplayListSet& aOut)
{
  nsXULEventRedirectorWrapper wrapper(this);
  return wrapper.WrapLists(aBuilder, this, aIn, aOut);
}
