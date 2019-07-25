











































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsFrameList.h"
#include "nsPlaceholderFrame.h"
#include "nsLineLayout.h"
#include "nsIContent.h"
#include "nsContentUtils.h"
#include "nsIAtom.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsStyleContext.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIScrollableFrame.h"
#include "nsPresContext.h"
#include "nsCRT.h"
#include "nsGUIEvent.h"
#include "nsIDOMEvent.h"
#include "nsPLDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIPresShell.h"
#include "prlog.h"
#include "prprf.h"
#include <stdarg.h>
#include "nsFrameManager.h"
#include "nsCSSRendering.h"
#include "nsLayoutUtils.h"
#ifdef ACCESSIBILITY
#include "nsIAccessible.h"
#endif

#include "nsIDOMText.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMHTMLHRElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDeviceContext.h"
#include "nsIEditorDocShell.h"
#include "nsIEventStateManager.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsFrameSelection.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#include "nsIHTMLContentSink.h" 
#include "nsCSSFrameConstructor.h"

#include "nsFrameTraversal.h"
#include "nsStyleChangeList.h"
#include "nsIDOMRange.h"
#include "nsITableLayout.h"    
#include "nsITableCellLayout.h"
#include "nsITextControlFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIPercentHeightObserver.h"
#include "nsStyleStructInlines.h"

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif


#include "nsIServiceManager.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsILookAndFeel.h"
#include "nsLayoutCID.h"
#include "nsWidgetsCID.h"     
#include "nsUnicharUtils.h"
#include "nsLayoutErrors.h"
#include "nsContentErrors.h"
#include "nsHTMLContainerFrame.h"
#include "nsBoxLayoutState.h"
#include "nsBlockFrame.h"
#include "nsDisplayList.h"
#include "nsIObjectLoadingContent.h"
#include "nsExpirationTracker.h"
#ifdef MOZ_SVG
#include "nsSVGIntegrationUtils.h"
#include "nsSVGEffects.h"
#endif

#include "gfxContext.h"
#include "CSSCalc.h"

using namespace mozilla;
using namespace mozilla::layers;

static NS_DEFINE_CID(kLookAndFeelCID,  NS_LOOKANDFEEL_CID);


struct nsBoxLayoutMetrics
{
  nsSize mPrefSize;
  nsSize mMinSize;
  nsSize mMaxSize;

  nsSize mBlockMinSize;
  nsSize mBlockPrefSize;
  nscoord mBlockAscent;

  nscoord mFlex;
  nscoord mAscent;

  nsSize mLastSize;
};

struct nsContentAndOffset
{
  nsIContent* mContent;
  PRInt32 mOffset;
};


#define SELECTION_DEBUG        0
#define FORCE_SELECTION_UPDATE 1
#define CALC_DEBUG             0


#include "nsILineIterator.h"


#if 0
static void RefreshContentFrames(nsPresContext* aPresContext, nsIContent * aStartContent, nsIContent * aEndContent);
#endif

#include "prenv.h"



#ifdef NS_DEBUG
static PRBool gShowFrameBorders = PR_FALSE;

void nsFrame::ShowFrameBorders(PRBool aEnable)
{
  gShowFrameBorders = aEnable;
}

PRBool nsFrame::GetShowFrameBorders()
{
  return gShowFrameBorders;
}

static PRBool gShowEventTargetFrameBorder = PR_FALSE;

void nsFrame::ShowEventTargetFrameBorder(PRBool aEnable)
{
  gShowEventTargetFrameBorder = aEnable;
}

PRBool nsFrame::GetShowEventTargetFrameBorder()
{
  return gShowEventTargetFrameBorder;
}





static PRLogModuleInfo* gLogModule;

static PRLogModuleInfo* gStyleVerifyTreeLogModuleInfo;

static PRBool gStyleVerifyTreeEnable = PRBool(0x55);

PRBool
nsFrame::GetVerifyStyleTreeEnable()
{
  if (gStyleVerifyTreeEnable == PRBool(0x55)) {
    if (nsnull == gStyleVerifyTreeLogModuleInfo) {
      gStyleVerifyTreeLogModuleInfo = PR_NewLogModule("styleverifytree");
      gStyleVerifyTreeEnable = 0 != gStyleVerifyTreeLogModuleInfo->level;
    }
  }
  return gStyleVerifyTreeEnable;
}

void
nsFrame::SetVerifyStyleTreeEnable(PRBool aEnabled)
{
  gStyleVerifyTreeEnable = aEnabled;
}

PRLogModuleInfo*
nsFrame::GetLogModuleInfo()
{
  if (nsnull == gLogModule) {
    gLogModule = PR_NewLogModule("frame");
  }
  return gLogModule;
}

void
nsFrame::DumpFrameTree(nsIFrame* aFrame)
{
    RootFrameList(aFrame->PresContext(), stdout, 0);
}

void
nsFrame::RootFrameList(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent)
{
  if (!aPresContext || !out)
    return;

  nsIPresShell *shell = aPresContext->GetPresShell();
  if (shell) {
    nsIFrame* frame = shell->FrameManager()->GetRootFrame();
    if(frame) {
      frame->List(out, aIndent);
    }
  }
}
#endif

void
NS_MergeReflowStatusInto(nsReflowStatus* aPrimary, nsReflowStatus aSecondary)
{
  *aPrimary |= aSecondary &
    (NS_FRAME_NOT_COMPLETE | NS_FRAME_OVERFLOW_INCOMPLETE |
     NS_FRAME_TRUNCATED | NS_FRAME_REFLOW_NEXTINFLOW);
  if (*aPrimary & NS_FRAME_NOT_COMPLETE) {
    *aPrimary &= ~NS_FRAME_OVERFLOW_INCOMPLETE;
  }
}

void
nsWeakFrame::InitInternal(nsIFrame* aFrame)
{
  Clear(mFrame ? mFrame->PresContext()->GetPresShell() : nsnull);
  mFrame = aFrame;
  if (mFrame) {
    nsIPresShell* shell = mFrame->PresContext()->GetPresShell();
    NS_WARN_IF_FALSE(shell, "Null PresShell in nsWeakFrame!");
    if (shell) {
      shell->AddWeakFrame(this);
    } else {
      mFrame = nsnull;
    }
  }
}

nsIFrame*
NS_NewEmptyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFrame(aContext);
}

nsFrame::nsFrame(nsStyleContext* aContext)
{
  MOZ_COUNT_CTOR(nsFrame);

  mState = NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY;
  mStyleContext = aContext;
  mStyleContext->AddRef();
}

nsFrame::~nsFrame()
{
  MOZ_COUNT_DTOR(nsFrame);

  NS_IF_RELEASE(mContent);
  if (mStyleContext)
    mStyleContext->Release();
}

NS_IMPL_FRAMEARENA_HELPERS(nsFrame)



void
nsFrame::operator delete(void *, size_t)
{
  NS_RUNTIMEABORT("nsFrame::operator delete should never be called");
}

NS_QUERYFRAME_HEAD(nsFrame)
  NS_QUERYFRAME_ENTRY(nsIFrame)
NS_QUERYFRAME_TAIL_INHERITANCE_ROOT




NS_IMETHODIMP
nsFrame::Init(nsIContent*      aContent,
              nsIFrame*        aParent,
              nsIFrame*        aPrevInFlow)
{
  NS_PRECONDITION(!mContent, "Double-initing a frame?");
  NS_ASSERTION(IsFrameOfType(eDEBUGAllFrames) &&
               !IsFrameOfType(eDEBUGNoFrames),
               "IsFrameOfType implementation that doesn't call base class");

  mContent = aContent;
  mParent = aParent;

  if (aContent) {
    NS_ADDREF(aContent);
  }

  if (aPrevInFlow) {
    
    nsFrameState state = aPrevInFlow->GetStateBits();

    
    mState |= state & (NS_FRAME_SELECTED_CONTENT |
                       NS_FRAME_INDEPENDENT_SELECTION |
                       NS_FRAME_IS_SPECIAL |
                       NS_FRAME_MAY_BE_TRANSFORMED);
  }
  if (mParent) {
    nsFrameState state = mParent->GetStateBits();

    
    mState |= state & (NS_FRAME_INDEPENDENT_SELECTION |
                       NS_FRAME_GENERATED_CONTENT);
  }
  if (GetStyleDisplay()->HasTransform()) {
    
    
    mState |= NS_FRAME_MAY_BE_TRANSFORMED;
  }
  
  DidSetStyleContext(nsnull);

  if (IsBoxWrapped())
    InitBoxMetrics(PR_FALSE);

  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetInitialChildList(nsIAtom*        aListName,
                                           nsFrameList&    aChildList)
{
  
  
#if 0
  NS_ERROR("not a container");
  return NS_ERROR_UNEXPECTED;
#else
  NS_ASSERTION(aChildList.IsEmpty(), "not a container");
  return NS_OK;
#endif
}

NS_IMETHODIMP
nsFrame::AppendFrames(nsIAtom*        aListName,
                      nsFrameList&    aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::InsertFrames(nsIAtom*        aListName,
                      nsIFrame*       aPrevFrame,
                      nsFrameList&    aFrameList)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::RemoveFrame(nsIAtom*        aListName,
                     nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(PR_FALSE, "not a container");
  return NS_ERROR_UNEXPECTED;
}

void
nsFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
    "destroy called on frame while scripts not blocked");
  NS_ASSERTION(!GetNextSibling() && !GetPrevSibling(),
               "Frames should be removed before destruction.");
  NS_ASSERTION(aDestructRoot, "Must specify destruct root");

#ifdef MOZ_SVG
  nsSVGEffects::InvalidateDirectRenderingObservers(this);
#endif

  
  
  nsIView* view = GetView();
  nsPresContext* presContext = PresContext();

  nsIPresShell *shell = presContext->GetPresShell();
  if (mState & NS_FRAME_OUT_OF_FLOW) {
    nsPlaceholderFrame* placeholder =
      shell->FrameManager()->GetPlaceholderFrameFor(this);
    NS_ASSERTION(!placeholder || (aDestructRoot != this),
                 "Don't call Destroy() on OOFs, call Destroy() on the placeholder.");
    NS_ASSERTION(!placeholder ||
                 nsLayoutUtils::IsProperAncestorFrame(aDestructRoot, placeholder),
                 "Placeholder relationship should have been torn down already; "
                 "this might mean we have a stray placeholder in the tree.");
    if (placeholder) {
      shell->FrameManager()->UnregisterPlaceholderFrame(placeholder);
      placeholder->SetOutOfFlowFrame(nsnull);
    }
  }

  shell->NotifyDestroyingFrame(this);

  if ((mState & NS_FRAME_EXTERNAL_REFERENCE) ||
      (mState & NS_FRAME_SELECTED_CONTENT)) {
    shell->ClearFrameRefs(this);
  }

  if (view) {
    
    view->SetClientData(nsnull);

    
    view->Destroy();
  }

  
  if (mContent && mContent->GetPrimaryFrame() == this) {
    mContent->SetPrimaryFrame(nsnull);
  }

  
  
  
  
  
  
  

  nsQueryFrame::FrameIID id = GetFrameId();
  this->~nsFrame();

  
  
  shell->FreeFrame(id, this);
}

NS_IMETHODIMP
nsFrame::GetOffsets(PRInt32 &aStart, PRInt32 &aEnd) const
{
  aStart = 0;
  aEnd = 0;
  return NS_OK;
}

static PRBool
EqualImages(imgIRequest *aOldImage, imgIRequest *aNewImage)
{
  if (aOldImage == aNewImage)
    return PR_TRUE;

  if (!aOldImage || !aNewImage)
    return PR_FALSE;

  nsCOMPtr<nsIURI> oldURI, newURI;
  aOldImage->GetURI(getter_AddRefs(oldURI));
  aNewImage->GetURI(getter_AddRefs(newURI));
  PRBool equal;
  return NS_SUCCEEDED(oldURI->Equals(newURI, &equal)) && equal;
}


 void
nsFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (aOldStyleContext) {
    
    
    
    
    
    
    
    
    const nsStyleBackground *oldBG = aOldStyleContext->GetStyleBackground();
    const nsStyleBackground *newBG = GetStyleBackground();
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, oldBG) {
      if (i >= newBG->mImageCount ||
          oldBG->mLayers[i].mImage != newBG->mLayers[i].mImage) {
        
        PresContext()->SetImageLoaders(this,
          nsPresContext::BACKGROUND_IMAGE, nsnull);
        break;
      }
    }

    
    
    
    
    
    FrameProperties props = Properties();
    nsMargin oldValue(0, 0, 0, 0);
    nsMargin newValue(0, 0, 0, 0);
    const nsStyleMargin* oldMargin = aOldStyleContext->PeekStyleMargin();
    if (oldMargin && oldMargin->GetMargin(oldValue)) {
      if ((!GetStyleMargin()->GetMargin(newValue) || oldValue != newValue) &&
          !props.Get(UsedMarginProperty())) {
        props.Set(UsedMarginProperty(), new nsMargin(oldValue));
      }
    }

    const nsStylePadding* oldPadding = aOldStyleContext->PeekStylePadding();
    if (oldPadding && oldPadding->GetPadding(oldValue)) {
      if ((!GetStylePadding()->GetPadding(newValue) || oldValue != newValue) &&
          !props.Get(UsedPaddingProperty())) {
        props.Set(UsedPaddingProperty(), new nsMargin(oldValue));
      }
    }

    const nsStyleBorder* oldBorder = aOldStyleContext->PeekStyleBorder();
    if (oldBorder) {
      oldValue = oldBorder->GetActualBorder();
      newValue = GetStyleBorder()->GetActualBorder();
      if (oldValue != newValue &&
          !props.Get(UsedBorderProperty())) {
        props.Set(UsedBorderProperty(), new nsMargin(oldValue));
      }
    }
  }

  imgIRequest *oldBorderImage = aOldStyleContext
    ? aOldStyleContext->GetStyleBorder()->GetBorderImage()
    : nsnull;
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (!EqualImages(oldBorderImage, GetStyleBorder()->GetBorderImage())) {
    
    PresContext()->SetupBorderImageLoaders(this, GetStyleBorder());
  }

  
  
  
  
  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    PresContext()->SetBidiEnabled();
  }
}

 nsMargin
nsIFrame::GetUsedMargin() const
{
  nsMargin margin(0, 0, 0, 0);
  if ((mState & NS_FRAME_FIRST_REFLOW) &&
      !(mState & NS_FRAME_IN_REFLOW))
    return margin;

  nsMargin *m = static_cast<nsMargin*>
                           (Properties().Get(UsedMarginProperty()));
  if (m) {
    margin = *m;
  } else {
#ifdef DEBUG
    PRBool hasMargin = 
#endif
    GetStyleMargin()->GetMargin(margin);
    NS_ASSERTION(hasMargin, "We should have a margin here! (out of memory?)");
  }
  return margin;
}

 nsMargin
nsIFrame::GetUsedBorder() const
{
  nsMargin border(0, 0, 0, 0);
  if ((mState & NS_FRAME_FIRST_REFLOW) &&
      !(mState & NS_FRAME_IN_REFLOW))
    return border;

  
  nsIFrame *mutable_this = const_cast<nsIFrame*>(this);

  const nsStyleDisplay *disp = GetStyleDisplay();
  if (mutable_this->IsThemed(disp)) {
    nsIntMargin result;
    nsPresContext *presContext = PresContext();
    presContext->GetTheme()->GetWidgetBorder(presContext->DeviceContext(),
                                             mutable_this, disp->mAppearance,
                                             &result);
    border.left = presContext->DevPixelsToAppUnits(result.left);
    border.top = presContext->DevPixelsToAppUnits(result.top);
    border.right = presContext->DevPixelsToAppUnits(result.right);
    border.bottom = presContext->DevPixelsToAppUnits(result.bottom);
    return border;
  }

  nsMargin *b = static_cast<nsMargin*>
                           (Properties().Get(UsedBorderProperty()));
  if (b) {
    border = *b;
  } else {
    border = GetStyleBorder()->GetActualBorder();
  }
  return border;
}

 nsMargin
nsIFrame::GetUsedPadding() const
{
  nsMargin padding(0, 0, 0, 0);
  if ((mState & NS_FRAME_FIRST_REFLOW) &&
      !(mState & NS_FRAME_IN_REFLOW))
    return padding;

  
  nsIFrame *mutable_this = const_cast<nsIFrame*>(this);

  const nsStyleDisplay *disp = GetStyleDisplay();
  if (mutable_this->IsThemed(disp)) {
    nsPresContext *presContext = PresContext();
    nsIntMargin widget;
    if (presContext->GetTheme()->GetWidgetPadding(presContext->DeviceContext(),
                                                  mutable_this,
                                                  disp->mAppearance,
                                                  &widget)) {
      padding.top = presContext->DevPixelsToAppUnits(widget.top);
      padding.right = presContext->DevPixelsToAppUnits(widget.right);
      padding.bottom = presContext->DevPixelsToAppUnits(widget.bottom);
      padding.left = presContext->DevPixelsToAppUnits(widget.left);
      return padding;
    }
  }

  nsMargin *p = static_cast<nsMargin*>
                           (Properties().Get(UsedPaddingProperty()));
  if (p) {
    padding = *p;
  } else {
#ifdef DEBUG
    PRBool hasPadding = 
#endif
    GetStylePadding()->GetPadding(padding);
    NS_ASSERTION(hasPadding, "We should have padding here! (out of memory?)");
  }
  return padding;
}

void
nsIFrame::ApplySkipSides(nsMargin& aMargin) const
{
  PRIntn skipSides = GetSkipSides();
  if (skipSides & (1 << NS_SIDE_TOP))
    aMargin.top = 0;
  if (skipSides & (1 << NS_SIDE_RIGHT))
    aMargin.right = 0;
  if (skipSides & (1 << NS_SIDE_BOTTOM))
    aMargin.bottom = 0;
  if (skipSides & (1 << NS_SIDE_LEFT))
    aMargin.left = 0;
}

nsRect
nsIFrame::GetPaddingRectRelativeToSelf() const
{
  nsMargin bp(GetUsedBorder());
  ApplySkipSides(bp);
  nsRect r(0, 0, mRect.width, mRect.height);
  r.Deflate(bp);
  return r;
}

nsRect
nsIFrame::GetPaddingRect() const
{
  return GetPaddingRectRelativeToSelf() + GetPosition();
}

PRBool
nsIFrame::IsTransformed() const
{
  return (mState & NS_FRAME_MAY_BE_TRANSFORMED) &&
    GetStyleDisplay()->HasTransform();
}

nsRect
nsIFrame::GetContentRectRelativeToSelf() const
{
  nsMargin bp(GetUsedBorderAndPadding());
  ApplySkipSides(bp);
  nsRect r(0, 0, mRect.width, mRect.height);
  r.Deflate(bp);
  return r;
}

nsRect
nsIFrame::GetContentRect() const
{
  return GetContentRectRelativeToSelf() + GetPosition();
}

PRBool
nsIFrame::ComputeBorderRadii(const nsStyleCorners& aBorderRadius,
                             const nsSize& aFrameSize,
                             const nsSize& aBorderArea,
                             PRIntn aSkipSides,
                             nscoord aRadii[8])
{
  
  NS_FOR_CSS_HALF_CORNERS(i) {
    const nsStyleCoord c = aBorderRadius.Get(i);
    nscoord axis =
      NS_HALF_CORNER_IS_X(i) ? aFrameSize.width : aFrameSize.height;

    if (c.IsCoordPercentCalcUnit()) {
      aRadii[i] = nsRuleNode::ComputeCoordPercentCalc(c, axis);
      if (aRadii[i] < 0) {
        
        aRadii[i] = 0;
      }
    } else {
      NS_NOTREACHED("ComputeBorderRadii: bad unit");
      aRadii[i] = 0;
    }
  }

  if (aSkipSides & (1 << NS_SIDE_TOP)) {
    aRadii[NS_CORNER_TOP_LEFT_X] = 0;
    aRadii[NS_CORNER_TOP_LEFT_Y] = 0;
    aRadii[NS_CORNER_TOP_RIGHT_X] = 0;
    aRadii[NS_CORNER_TOP_RIGHT_Y] = 0;
  }

  if (aSkipSides & (1 << NS_SIDE_RIGHT)) {
    aRadii[NS_CORNER_TOP_RIGHT_X] = 0;
    aRadii[NS_CORNER_TOP_RIGHT_Y] = 0;
    aRadii[NS_CORNER_BOTTOM_RIGHT_X] = 0;
    aRadii[NS_CORNER_BOTTOM_RIGHT_Y] = 0;
  }

  if (aSkipSides & (1 << NS_SIDE_BOTTOM)) {
    aRadii[NS_CORNER_BOTTOM_RIGHT_X] = 0;
    aRadii[NS_CORNER_BOTTOM_RIGHT_Y] = 0;
    aRadii[NS_CORNER_BOTTOM_LEFT_X] = 0;
    aRadii[NS_CORNER_BOTTOM_LEFT_Y] = 0;
  }

  if (aSkipSides & (1 << NS_SIDE_LEFT)) {
    aRadii[NS_CORNER_BOTTOM_LEFT_X] = 0;
    aRadii[NS_CORNER_BOTTOM_LEFT_Y] = 0;
    aRadii[NS_CORNER_TOP_LEFT_X] = 0;
    aRadii[NS_CORNER_TOP_LEFT_Y] = 0;
  }

  
  
  PRBool haveRadius = PR_FALSE;
  double ratio = 1.0f;
  NS_FOR_CSS_SIDES(side) {
    PRUint32 hc1 = NS_SIDE_TO_HALF_CORNER(side, PR_FALSE, PR_TRUE);
    PRUint32 hc2 = NS_SIDE_TO_HALF_CORNER(side, PR_TRUE, PR_TRUE);
    nscoord length =
      NS_SIDE_IS_VERTICAL(side) ? aBorderArea.height : aBorderArea.width;
    nscoord sum = aRadii[hc1] + aRadii[hc2];
    if (sum)
      haveRadius = PR_TRUE;

    
    if (length < sum)
      ratio = NS_MIN(ratio, double(length)/sum);
  }
  if (ratio < 1.0) {
    NS_FOR_CSS_HALF_CORNERS(corner) {
      aRadii[corner] *= ratio;
    }
  }

  return haveRadius;
}

 void
nsIFrame::InsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets)
{
  NS_FOR_CSS_SIDES(side) {
    nscoord offset = aOffsets.side(side);
    PRUint32 hc1 = NS_SIDE_TO_HALF_CORNER(side, PR_FALSE, PR_FALSE);
    PRUint32 hc2 = NS_SIDE_TO_HALF_CORNER(side, PR_TRUE, PR_FALSE);
    aRadii[hc1] = NS_MAX(0, aRadii[hc1] - offset);
    aRadii[hc2] = NS_MAX(0, aRadii[hc2] - offset);
  }
}

 void
nsIFrame::OutsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets)
{
  NS_FOR_CSS_SIDES(side) {
    nscoord offset = aOffsets.side(side);
    PRUint32 hc1 = NS_SIDE_TO_HALF_CORNER(side, PR_FALSE, PR_FALSE);
    PRUint32 hc2 = NS_SIDE_TO_HALF_CORNER(side, PR_TRUE, PR_FALSE);
    if (aRadii[hc1] > 0)
      aRadii[hc1] += offset;
    if (aRadii[hc2] > 0)
      aRadii[hc2] += offset;
  }
}

 PRBool
nsIFrame::GetBorderRadii(nscoord aRadii[8]) const
{
  if (IsThemed()) {
    
    
    
    
    
    
    
    NS_FOR_CSS_HALF_CORNERS(corner) {
      aRadii[corner] = 0;
    }
    return PR_FALSE;
  }
  nsSize size = GetSize();
  return ComputeBorderRadii(GetStyleBorder()->mBorderRadius, size, size,
                            GetSkipSides(), aRadii);
}

PRBool
nsIFrame::GetPaddingBoxBorderRadii(nscoord aRadii[8]) const
{
  if (!GetBorderRadii(aRadii))
    return PR_FALSE;
  InsetBorderRadii(aRadii, GetUsedBorder());
  NS_FOR_CSS_HALF_CORNERS(corner) {
    if (aRadii[corner])
      return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsIFrame::GetContentBoxBorderRadii(nscoord aRadii[8]) const
{
  if (!GetBorderRadii(aRadii))
    return PR_FALSE;
  InsetBorderRadii(aRadii, GetUsedBorderAndPadding());
  NS_FOR_CSS_HALF_CORNERS(corner) {
    if (aRadii[corner])
      return PR_TRUE;
  }
  return PR_FALSE;
}

nsStyleContext*
nsFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  return nsnull;
}

void
nsFrame::SetAdditionalStyleContext(PRInt32 aIndex, 
                                   nsStyleContext* aStyleContext)
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
}

nscoord
nsFrame::GetBaseline() const
{
  NS_ASSERTION(!NS_SUBTREE_DIRTY(this),
               "frame must not be dirty");
  
  
  return mRect.height + GetUsedMargin().bottom;
}



nsIAtom*
nsFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  return nsnull;
}

nsFrameList
nsFrame::GetChildList(nsIAtom* aListName) const
{
  return nsFrameList::EmptyList();
}

static nsIFrame*
GetActiveSelectionFrame(nsPresContext* aPresContext, nsIFrame* aFrame)
{
  nsIContent* capturingContent = nsIPresShell::GetCapturingContent();
  if (capturingContent) {
    nsIFrame* activeFrame = aPresContext->GetPrimaryFrameFor(capturingContent);
    return activeFrame ? activeFrame : aFrame;
  }

  return aFrame;
}

PRInt16
nsFrame::DisplaySelection(nsPresContext* aPresContext, PRBool isOkToTurnOn)
{
  PRInt16 selType = nsISelectionController::SELECTION_OFF;

  nsCOMPtr<nsISelectionController> selCon;
  nsresult result = GetSelectionController(aPresContext, getter_AddRefs(selCon));
  if (NS_SUCCEEDED(result) && selCon) {
    result = selCon->GetDisplaySelection(&selType);
    if (NS_SUCCEEDED(result) && (selType != nsISelectionController::SELECTION_OFF)) {
      
      PRBool selectable;
      IsSelectable(&selectable, nsnull);
      if (!selectable) {
        selType = nsISelectionController::SELECTION_OFF;
        isOkToTurnOn = PR_FALSE;
      }
    }
    if (isOkToTurnOn && (selType == nsISelectionController::SELECTION_OFF)) {
      selCon->SetDisplaySelection(nsISelectionController::SELECTION_ON);
      selType = nsISelectionController::SELECTION_ON;
    }
  }
  return selType;
}

class nsDisplaySelectionOverlay : public nsDisplayItem {
public:
  nsDisplaySelectionOverlay(nsDisplayListBuilder* aBuilder,
                            nsFrame* aFrame, PRInt16 aSelectionValue)
    : nsDisplayItem(aBuilder, aFrame), mSelectionValue(aSelectionValue) {
    MOZ_COUNT_CTOR(nsDisplaySelectionOverlay);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySelectionOverlay() {
    MOZ_COUNT_DTOR(nsDisplaySelectionOverlay);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("SelectionOverlay", TYPE_SELECTION_OVERLAY)
private:
  PRInt16 mSelectionValue;
};

void nsDisplaySelectionOverlay::Paint(nsDisplayListBuilder* aBuilder,
                                      nsRenderingContext* aCtx)
{
  nscolor color = NS_RGB(255, 255, 255);
  
  nsILookAndFeel::nsColorID colorID;
  nsresult result;
  if (mSelectionValue == nsISelectionController::SELECTION_ON) {
    colorID = nsILookAndFeel::eColor_TextSelectBackground;
  } else if (mSelectionValue == nsISelectionController::SELECTION_ATTENTION) {
    colorID = nsILookAndFeel::eColor_TextSelectBackgroundAttention;
  } else {
    colorID = nsILookAndFeel::eColor_TextSelectBackgroundDisabled;
  }

  nsCOMPtr<nsILookAndFeel> look;
  look = do_GetService(kLookAndFeelCID, &result);
  if (NS_SUCCEEDED(result) && look)
    look->GetColor(colorID, color);

  gfxRGBA c(color);
  c.a = .5;

  gfxContext *ctx = aCtx->ThebesContext();
  ctx->SetColor(c);

  nsIntRect pxRect =
    mVisibleRect.ToOutsidePixels(mFrame->PresContext()->AppUnitsPerDevPixel());
  ctx->NewPath();
  ctx->Rectangle(gfxRect(pxRect.x, pxRect.y, pxRect.width, pxRect.height), PR_TRUE);
  ctx->Fill();
}





nsresult
nsFrame::DisplaySelectionOverlay(nsDisplayListBuilder*   aBuilder,
                                 nsDisplayList*          aList,
                                 PRUint16                aContentType)
{

  if ((GetStateBits() & NS_FRAME_SELECTED_CONTENT) != NS_FRAME_SELECTED_CONTENT)
    return NS_OK;
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;
    
  nsPresContext* presContext = PresContext();
  nsIPresShell *shell = presContext->PresShell();
  if (!shell)
    return NS_OK;

  PRInt16 displaySelection = shell->GetSelectionFlags();
  if (!(displaySelection & aContentType))
    return NS_OK;

  const nsFrameSelection* frameSelection = GetConstFrameSelection();
  PRInt16 selectionValue = frameSelection->GetDisplaySelection();

  if (selectionValue <= nsISelectionController::SELECTION_HIDDEN)
    return NS_OK; 

  nsIContent *newContent = mContent->GetParent();

  
  PRInt32 offset = 0;
  if (newContent) {
    
    offset = newContent->IndexOf(mContent);
  }

  SelectionDetails *details;
  
  details = frameSelection->LookUpSelection(newContent, offset, 1, PR_FALSE);
  
  
  if (!details)
    return NS_OK;
  
  while (details) {
    SelectionDetails *next = details->mNext;
    delete details;
    details = next;
  }

  return aList->AppendNewToTop(new (aBuilder)
      nsDisplaySelectionOverlay(aBuilder, this, selectionValue));
}

nsresult
nsFrame::DisplayOutlineUnconditional(nsDisplayListBuilder*   aBuilder,
                                     const nsDisplayListSet& aLists)
{
  if (GetStyleOutline()->GetOutlineStyle() == NS_STYLE_BORDER_STYLE_NONE)
    return NS_OK;
    
  return aLists.Outlines()->AppendNewToTop(
      new (aBuilder) nsDisplayOutline(aBuilder, this));
}

nsresult
nsFrame::DisplayOutline(nsDisplayListBuilder*   aBuilder,
                        const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  return DisplayOutlineUnconditional(aBuilder, aLists);
}

nsresult
nsIFrame::DisplayCaret(nsDisplayListBuilder* aBuilder,
                       const nsRect& aDirtyRect, nsDisplayList* aList)
{
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  return aList->AppendNewToTop(
      new (aBuilder) nsDisplayCaret(aBuilder, this, aBuilder->GetCaret()));
}

nscolor
nsIFrame::GetCaretColorAt(PRInt32 aOffset)
{
  
  return GetStyleColor()->mColor;
}

PRBool
nsIFrame::HasBorder() const
{
  
  
  return (GetUsedBorder() != nsMargin(0,0,0,0) ||
          GetStyleBorder()->IsBorderImageLoaded());
}

nsresult
nsFrame::DisplayBackgroundUnconditional(nsDisplayListBuilder*   aBuilder,
                                        const nsDisplayListSet& aLists,
                                        PRBool                  aForceBackground)
{
  
  
  
  if (aBuilder->IsForEventDelivery() || aForceBackground ||
      !GetStyleBackground()->IsTransparent() || GetStyleDisplay()->mAppearance) {
    return aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBackground(aBuilder, this));
  }
  return NS_OK;
}

nsresult
nsFrame::DisplayBorderBackgroundOutline(nsDisplayListBuilder*   aBuilder,
                                        const nsDisplayListSet& aLists,
                                        PRBool                  aForceBackground)
{
  
  
  
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  PRBool hasBoxShadow = GetStyleBorder()->mBoxShadow != nsnull;
  if (hasBoxShadow) {
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBoxShadowOuter(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsresult rv =
    DisplayBackgroundUnconditional(aBuilder, aLists, aForceBackground);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasBoxShadow) {
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBoxShadowInner(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  if (HasBorder()) {
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBorder(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return DisplayOutlineUnconditional(aBuilder, aLists);
}

PRBool
nsIFrame::GetAbsPosClipRect(const nsStyleDisplay* aDisp, nsRect* aRect,
                            const nsSize& aSize)
{
  NS_PRECONDITION(aRect, "Must have aRect out parameter");

  if (!aDisp->IsAbsolutelyPositioned() ||
      !(aDisp->mClipFlags & NS_STYLE_CLIP_RECT))
    return PR_FALSE;

  *aRect = aDisp->mClip;
  if (NS_STYLE_CLIP_RIGHT_AUTO & aDisp->mClipFlags) {
    aRect->width = aSize.width - aRect->x;
  }
  if (NS_STYLE_CLIP_BOTTOM_AUTO & aDisp->mClipFlags) {
    aRect->height = aSize.height - aRect->y;
  }
  return PR_TRUE;
}

static PRBool ApplyAbsPosClipping(nsDisplayListBuilder* aBuilder,
                                  const nsStyleDisplay* aDisp, nsIFrame* aFrame,
                                  nsRect* aRect) {
  if (!aFrame->GetAbsPosClipRect(aDisp, aRect, aFrame->GetSize()))
    return PR_FALSE;

  *aRect += aBuilder->ToReferenceFrame(aFrame);
  return PR_TRUE;
}





static inline PRBool ApplyOverflowHiddenClipping(nsIFrame* aFrame,
                                                 const nsStyleDisplay* aDisp)
{
  if (aDisp->mOverflowX != NS_STYLE_OVERFLOW_HIDDEN)
    return PR_FALSE;
    
  nsIAtom* type = aFrame->GetType();
  
  
  
  
  
  
  return type == nsGkAtoms::tableFrame ||
       type == nsGkAtoms::tableCellFrame ||
       type == nsGkAtoms::bcTableCellFrame;
}

static PRBool ApplyOverflowClipping(nsDisplayListBuilder* aBuilder,
                                    nsIFrame* aFrame,
                                    const nsStyleDisplay* aDisp, nsRect* aRect) {
  
  
  
  

  
  
  
  if (!ApplyOverflowHiddenClipping(aFrame, aDisp) &&
      !nsFrame::ApplyPaginatedOverflowClipping(aFrame)) {
    PRBool clip = aDisp->mOverflowX == NS_STYLE_OVERFLOW_CLIP;
    if (!clip)
      return PR_FALSE;
    
    
    
  }
  
  *aRect = aFrame->GetPaddingRect() - aFrame->GetPosition() +
    aBuilder->ToReferenceFrame(aFrame);
  return PR_TRUE;
}

class nsOverflowClipWrapper : public nsDisplayWrapper
{
public:
  






  nsOverflowClipWrapper(nsIFrame* aContainer, const nsRect& aRect,
                        const nscoord aRadii[8],
                        PRBool aClipBorderBackground, PRBool aClipAll)
    : mContainer(aContainer), mRect(aRect),
      mClipBorderBackground(aClipBorderBackground), mClipAll(aClipAll),
      mHaveRadius(PR_FALSE)
  {
    memcpy(mRadii, aRadii, sizeof(mRadii));
    NS_FOR_CSS_HALF_CORNERS(corner) {
      if (aRadii[corner] > 0) {
        mHaveRadius = PR_TRUE;
        break;
      }
    }
  }
  virtual PRBool WrapBorderBackground() { return mClipBorderBackground; }
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) {
    
    
    
    if (mHaveRadius) {
      return new (aBuilder) nsDisplayClipRoundedRect(aBuilder, nsnull, aList,
                                                     mRect, mRadii);
    }
    return new (aBuilder) nsDisplayClip(aBuilder, nsnull, aList, mRect);
  }
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) {
    nsIFrame* f = aItem->GetUnderlyingFrame();
    if (mClipAll ||
        nsLayoutUtils::IsProperAncestorFrame(mContainer, f, nsnull)) {
      if (mHaveRadius) {
        return new (aBuilder) nsDisplayClipRoundedRect(aBuilder, f, aItem,
                                                       mRect, mRadii);
      }
      return new (aBuilder) nsDisplayClip(aBuilder, f, aItem, mRect);
    }
    return aItem;
  }
protected:
  nsIFrame*    mContainer;
  nsRect       mRect;
  nscoord      mRadii[8];
  PRPackedBool mClipBorderBackground;
  PRPackedBool mClipAll;
  PRPackedBool mHaveRadius;
};

class nsAbsPosClipWrapper : public nsDisplayWrapper
{
public:
  nsAbsPosClipWrapper(const nsRect& aRect)
    : mRect(aRect) {}
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) {
    
    
    return new (aBuilder) nsDisplayClip(aBuilder, nsnull, aList, mRect);
  }
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) {
    return new (aBuilder) nsDisplayClip(aBuilder, aItem->GetUnderlyingFrame(),
                                        aItem, mRect);
  }
protected:
  nsRect    mRect;
};

nsresult
nsIFrame::OverflowClip(nsDisplayListBuilder*   aBuilder,
                       const nsDisplayListSet& aFromSet,
                       const nsDisplayListSet& aToSet,
                       const nsRect&           aClipRect,
                       const nscoord           aClipRadii[8],
                       PRBool                  aClipBorderBackground,
                       PRBool                  aClipAll)
{
  nsOverflowClipWrapper wrapper(this, aClipRect, aClipRadii,
                                aClipBorderBackground, aClipAll);
  return wrapper.WrapLists(aBuilder, this, aFromSet, aToSet);
}

static nsresult
BuildDisplayListWithOverflowClip(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
    const nsRect& aDirtyRect, const nsDisplayListSet& aSet,
    const nsRect& aClipRect, const nscoord aClipRadii[8])
{
  nsDisplayListCollection set;
  nsresult rv = aFrame->BuildDisplayList(aBuilder, aDirtyRect, set);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aBuilder->DisplayCaret(aFrame, aDirtyRect, aSet.Content());
  NS_ENSURE_SUCCESS(rv, rv);

  return aFrame->OverflowClip(aBuilder, set, aSet, aClipRect, aClipRadii);
}

#ifdef NS_DEBUG
static void PaintDebugBorder(nsIFrame* aFrame, nsRenderingContext* aCtx,
     const nsRect& aDirtyRect, nsPoint aPt) {
  nsRect r(aPt, aFrame->GetSize());
  if (aFrame->HasView()) {
    aCtx->SetColor(NS_RGB(0,0,255));
  } else {
    aCtx->SetColor(NS_RGB(255,0,0));
  }
  aCtx->DrawRect(r);
}

static void PaintEventTargetBorder(nsIFrame* aFrame, nsRenderingContext* aCtx,
     const nsRect& aDirtyRect, nsPoint aPt) {
  nsRect r(aPt, aFrame->GetSize());
  aCtx->SetColor(NS_RGB(128,0,128));
  aCtx->DrawRect(r);
}

static void
DisplayDebugBorders(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    const nsDisplayListSet& aLists) {
  
  
  if (nsFrame::GetShowFrameBorders() && !aFrame->GetRect().IsEmpty()) {
    aLists.Outlines()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(aBuilder, aFrame, PaintDebugBorder, "DebugBorder",
                         nsDisplayItem::TYPE_DEBUG_BORDER));
  }
  
  if (nsFrame::GetShowEventTargetFrameBorder() &&
      aFrame->PresContext()->PresShell()->GetDrawEventTargetFrame() == aFrame) {
    aLists.Outlines()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(aBuilder, aFrame, PaintEventTargetBorder, "EventTargetBorder",
                         nsDisplayItem::TYPE_EVENT_TARGET_BORDER));
  }
}
#endif

nsresult
nsIFrame::BuildDisplayListForStackingContext(nsDisplayListBuilder* aBuilder,
                                             const nsRect&         aDirtyRect,
                                             nsDisplayList*        aList) {
  if (GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
    return NS_OK;

  
  
  if (IsFrameOfType(eReplaced) && !IsVisibleForPainting(aBuilder))
    return NS_OK;

  nsRect absPosClip;
  const nsStyleDisplay* disp = GetStyleDisplay();
  
  
  if (disp->mOpacity == 0.0 && !aBuilder->IsForEventDelivery())
    return NS_OK;

  PRBool applyAbsPosClipping =
      ApplyAbsPosClipping(aBuilder, disp, this, &absPosClip);
  nsRect dirtyRect = aDirtyRect;

  PRBool inTransform = aBuilder->IsInTransform();
  


  if ((mState & NS_FRAME_MAY_BE_TRANSFORMED) &&
      disp->HasTransform()) {
    dirtyRect = nsDisplayTransform::UntransformRect(dirtyRect, this, nsPoint(0, 0));
    inTransform = PR_TRUE;
  }

  if (applyAbsPosClipping) {
    dirtyRect.IntersectRect(dirtyRect,
                            absPosClip - aBuilder->ToReferenceFrame(this));
  }

#ifdef MOZ_SVG
  PRBool usingSVGEffects = nsSVGIntegrationUtils::UsingEffectsForFrame(this);
  if (usingSVGEffects) {
    dirtyRect =
      nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(this, dirtyRect);
  }
#endif

  nsDisplayListCollection set;
  nsresult rv;
  {    
    nsDisplayListBuilder::AutoIsRootSetter rootSetter(aBuilder, PR_TRUE);
    nsDisplayListBuilder::AutoInTransformSetter
      inTransformSetter(aBuilder, inTransform);
    rv = BuildDisplayList(aBuilder, dirtyRect, set);
  }
  NS_ENSURE_SUCCESS(rv, rv);
    
  if (aBuilder->IsBackgroundOnly()) {
    set.BlockBorderBackgrounds()->DeleteAll();
    set.Floats()->DeleteAll();
    set.Content()->DeleteAll();
    set.PositionedDescendants()->DeleteAll();
    set.Outlines()->DeleteAll();
  }
  
  
  
  
  
  
  
  set.PositionedDescendants()->SortByZOrder(aBuilder, GetContent());
  
  nsRect overflowClip;
  if (ApplyOverflowClipping(aBuilder, this, disp, &overflowClip)) {
    nscoord radii[8];
    this->GetPaddingBoxBorderRadii(radii);
    nsOverflowClipWrapper wrapper(this, overflowClip, radii,
                                  PR_FALSE, PR_FALSE);
    rv = wrapper.WrapListsInPlace(aBuilder, this, set);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  
  

  nsDisplayList resultList;
  
  
  resultList.AppendToTop(set.BorderBackground());
  
  for (;;) {
    nsDisplayItem* item = set.PositionedDescendants()->GetBottom();
    if (item) {
      nsIFrame* f = item->GetUnderlyingFrame();
      NS_ASSERTION(f, "After sorting, every item in the list should have an underlying frame");
      if (nsLayoutUtils::GetZIndex(f) < 0) {
        set.PositionedDescendants()->RemoveBottom();
        resultList.AppendToTop(item);
        continue;
      }
    }
    break;
  }
  
  resultList.AppendToTop(set.BlockBorderBackgrounds());
  
  resultList.AppendToTop(set.Floats());
  
  resultList.AppendToTop(set.Content());
  
  
  
  
  
  set.Outlines()->SortByContentOrder(aBuilder, GetContent());
#ifdef NS_DEBUG
  DisplayDebugBorders(aBuilder, this, set);
#endif
  resultList.AppendToTop(set.Outlines());
  
  resultList.AppendToTop(set.PositionedDescendants());

  if (applyAbsPosClipping) {
    nsAbsPosClipWrapper wrapper(absPosClip);
    nsDisplayItem* item = wrapper.WrapList(aBuilder, this, &resultList);
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;
    
    resultList.AppendToTop(item);
  }
 
#ifdef MOZ_SVG
  
  if (usingSVGEffects) {
    
    rv = resultList.AppendNewToTop(
        new (aBuilder) nsDisplaySVGEffects(aBuilder, this, &resultList));
    if (NS_FAILED(rv))
      return rv;
  } else
#endif

  


  if (disp->mOpacity < 1.0f && !resultList.IsEmpty()) {
    rv = resultList.AppendNewToTop(
        new (aBuilder) nsDisplayOpacity(aBuilder, this, &resultList));
    if (NS_FAILED(rv))
      return rv;
  }

  


  if ((mState & NS_FRAME_MAY_BE_TRANSFORMED) &&
      disp->HasTransform() && !resultList.IsEmpty()) {
    rv = resultList.AppendNewToTop(
        new (aBuilder) nsDisplayTransform(aBuilder, this, &resultList));
    if (NS_FAILED(rv))
      return rv;
  }

  aList->AppendToTop(&resultList);
  return rv;
}

nsresult
nsIFrame::BuildDisplayListForChild(nsDisplayListBuilder*   aBuilder,
                                   nsIFrame*               aChild,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists,
                                   PRUint32                aFlags) {
  
  
  if (aBuilder->IsBackgroundOnly())
    return NS_OK;

  if (aChild->GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
    return NS_OK;
  
  const nsStyleDisplay* disp = aChild->GetStyleDisplay();
  
  PRBool pseudoStackingContext =
    (aFlags & DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT) != 0;
  
  if ((aFlags & DISPLAY_CHILD_INLINE) &&
      (disp->mDisplay != NS_STYLE_DISPLAY_INLINE ||
       aChild->IsContainingBlock() ||
       (aChild->IsFrameOfType(eReplaced)))) {
    
    
    
    pseudoStackingContext = PR_TRUE;
  }

  
  nsRect dirty = aDirtyRect - aChild->GetOffsetTo(this);

  nsIAtom* childType = aChild->GetType();
  if (childType == nsGkAtoms::placeholderFrame) {
    nsPlaceholderFrame* placeholder = static_cast<nsPlaceholderFrame*>(aChild);
    aChild = placeholder->GetOutOfFlowFrame();
    NS_ASSERTION(aChild, "No out of flow frame?");
    if (!aChild || nsLayoutUtils::IsPopup(aChild))
      return NS_OK;
    
    disp = aChild->GetStyleDisplay();
    
    
    
    childType = nsnull;
    
    if (aChild->GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
      return NS_OK;
    nsRect* savedDirty = static_cast<nsRect*>
      (aChild->Properties().Get(nsDisplayListBuilder::OutOfFlowDirtyRectProperty()));
    if (savedDirty) {
      dirty = *savedDirty;
    } else {
      
      
      
      dirty.Empty();
    }
    pseudoStackingContext = PR_TRUE;
  } else if (aBuilder->GetSelectedFramesOnly() &&
             aChild->IsLeaf() &&
             !(aChild->GetStateBits() & NS_FRAME_SELECTED_CONTENT)) {
    return NS_OK;
  }

  if (aBuilder->GetIncludeAllOutOfFlows() &&
      (aChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    dirty = aChild->GetVisualOverflowRect();
  } else if (!(aChild->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO)) {
    
    

    
    
    
    
    
    if (aChild != aBuilder->GetIgnoreScrollFrame()) {
      nsRect childDirty;
      if (!childDirty.IntersectRect(dirty, aChild->GetVisualOverflowRect()))
        return NS_OK;
      
      
      
      
      
    }
  }

  
  
  const nsStyleDisplay* ourDisp = GetStyleDisplay();
  
  
  if (IsThemed(ourDisp) &&
      !PresContext()->GetTheme()->WidgetIsContainer(ourDisp->mAppearance))
    return NS_OK;

  
  
  PRBool isComposited = disp->mOpacity != 1.0f || aChild->IsTransformed()
#ifdef MOZ_SVG
    || nsSVGIntegrationUtils::UsingEffectsForFrame(aChild)
#endif
    ;
  PRBool isPositioned = disp->IsPositioned();
  if (isComposited || isPositioned || disp->IsFloating() ||
      (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    pseudoStackingContext = PR_TRUE;
  }
  
  nsRect overflowClip;
  nscoord overflowClipRadii[8];
  PRBool applyOverflowClip =
    ApplyOverflowClipping(aBuilder, aChild, disp, &overflowClip);
  if (applyOverflowClip) {
    aChild->GetPaddingBoxBorderRadii(overflowClipRadii);
  }
  
  
  
  
  
  

  nsDisplayListBuilder::AutoIsRootSetter rootSetter(aBuilder, pseudoStackingContext);
  nsresult rv;
  if (!pseudoStackingContext) {
    
    
    
    if (applyOverflowClip) {
      rv = BuildDisplayListWithOverflowClip(aBuilder, aChild, dirty, aLists,
                                            overflowClip, overflowClipRadii);
    } else {
      rv = aChild->BuildDisplayList(aBuilder, dirty, aLists);
      if (NS_SUCCEEDED(rv)) {
        rv = aBuilder->DisplayCaret(aChild, dirty, aLists.Content());
      }
    }
#ifdef NS_DEBUG
    DisplayDebugBorders(aBuilder, aChild, aLists);
#endif
    return rv;
  }
  
  nsDisplayList list;
  nsDisplayList extraPositionedDescendants;
  const nsStylePosition* pos = aChild->GetStylePosition();
  if ((isPositioned && pos->mZIndex.GetUnit() == eStyleUnit_Integer) ||
      isComposited || (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    rv = aChild->BuildDisplayListForStackingContext(aBuilder, dirty, &list);
    if (NS_SUCCEEDED(rv)) {
      rv = aBuilder->DisplayCaret(aChild, dirty, &list);
    }
  } else {
    nsRect clipRect;
    PRBool applyAbsPosClipping =
        ApplyAbsPosClipping(aBuilder, disp, aChild, &clipRect);
    
    
    
    
    nsDisplayListCollection pseudoStack;
    nsRect clippedDirtyRect = dirty;
    if (applyAbsPosClipping) {
      
      
      clippedDirtyRect.IntersectRect(clippedDirtyRect,
                                     clipRect - aBuilder->ToReferenceFrame(aChild));
    }
    
    if (applyOverflowClip) {
      rv = BuildDisplayListWithOverflowClip(aBuilder, aChild, clippedDirtyRect,
                                            pseudoStack, overflowClip,
                                            overflowClipRadii);
    } else {
      rv = aChild->BuildDisplayList(aBuilder, clippedDirtyRect, pseudoStack);
      if (NS_SUCCEEDED(rv)) {
        rv = aBuilder->DisplayCaret(aChild, dirty, pseudoStack.Content());
      }
    }
    
    if (NS_SUCCEEDED(rv)) {
      if (isPositioned && applyAbsPosClipping) {
        nsAbsPosClipWrapper wrapper(clipRect);
        rv = wrapper.WrapListsInPlace(aBuilder, aChild, pseudoStack);
      }
    }
    list.AppendToTop(pseudoStack.BorderBackground());
    list.AppendToTop(pseudoStack.BlockBorderBackgrounds());
    list.AppendToTop(pseudoStack.Floats());
    list.AppendToTop(pseudoStack.Content());
    list.AppendToTop(pseudoStack.Outlines());
    extraPositionedDescendants.AppendToTop(pseudoStack.PositionedDescendants());
#ifdef NS_DEBUG
    DisplayDebugBorders(aBuilder, aChild, aLists);
#endif
  }
  NS_ENSURE_SUCCESS(rv, rv);
    
  if (isPositioned || isComposited ||
      (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    
    rv = aLists.PositionedDescendants()->AppendNewToTop(new (aBuilder)
        nsDisplayWrapList(aBuilder, aChild, &list));
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (disp->IsFloating()) {
    rv = aLists.Floats()->AppendNewToTop(new (aBuilder)
        nsDisplayWrapList(aBuilder, aChild, &list));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    aLists.Content()->AppendToTop(&list);
  }
  
  
  
  
  
  aLists.PositionedDescendants()->AppendToTop(&extraPositionedDescendants);
  return NS_OK;
}

void
nsIFrame::WrapReplacedContentForBorderRadius(nsDisplayListBuilder* aBuilder,
                                             nsDisplayList* aFromList,
                                             const nsDisplayListSet& aToLists)
{
  nscoord radii[8];
  if (GetContentBoxBorderRadii(radii)) {
    
    
    nsDisplayListCollection set;
    set.Content()->AppendToTop(aFromList);
    nsRect clipRect = GetContentRect() - GetPosition() +
                      aBuilder->ToReferenceFrame(this);
    OverflowClip(aBuilder, set, aToLists, clipRect, radii, PR_FALSE, PR_TRUE);

    return;
  }

  aToLists.Content()->AppendToTop(aFromList);
}

NS_IMETHODIMP  
nsFrame::GetContentForEvent(nsPresContext* aPresContext,
                            nsEvent* aEvent,
                            nsIContent** aContent)
{
  nsIFrame* f = nsLayoutUtils::GetNonGeneratedAncestor(this);
  *aContent = f->GetContent();
  NS_IF_ADDREF(*aContent);
  return NS_OK;
}

void
nsFrame::FireDOMEvent(const nsAString& aDOMEventName, nsIContent *aContent)
{
  nsIContent* target = aContent ? aContent : mContent;

  if (target) {
    nsRefPtr<nsPLDOMEvent> event =
      new nsPLDOMEvent(target, aDOMEventName, PR_TRUE, PR_FALSE);
    if (!event || NS_FAILED(event->PostDOMEvent()))
      NS_WARNING("Failed to dispatch nsPLDOMEvent");
  }
}

NS_IMETHODIMP
nsFrame::HandleEvent(nsPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{

  if (aEvent->message == NS_MOUSE_MOVE) {
    return HandleDrag(aPresContext, aEvent, aEventStatus);
  }

  if (aEvent->eventStructType == NS_MOUSE_EVENT &&
      static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton) {
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      HandlePress(aPresContext, aEvent, aEventStatus);
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      HandleRelease(aPresContext, aEvent, aEventStatus);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetDataForTableSelection(const nsFrameSelection *aFrameSelection,
                                  nsIPresShell *aPresShell, nsMouseEvent *aMouseEvent, 
                                  nsIContent **aParentContent, PRInt32 *aContentOffset, PRInt32 *aTarget)
{
  if (!aFrameSelection || !aPresShell || !aMouseEvent || !aParentContent || !aContentOffset || !aTarget)
    return NS_ERROR_NULL_POINTER;

  *aParentContent = nsnull;
  *aContentOffset = 0;
  *aTarget = 0;

  PRInt16 displaySelection = aPresShell->GetSelectionFlags();

  PRBool selectingTableCells = aFrameSelection->GetTableCellSelection();

  
  
  
  
  
  PRBool doTableSelection =
     displaySelection == nsISelectionDisplay::DISPLAY_ALL && selectingTableCells &&
     (aMouseEvent->message == NS_MOUSE_MOVE ||
      (aMouseEvent->message == NS_MOUSE_BUTTON_UP &&
       aMouseEvent->button == nsMouseEvent::eLeftButton) ||
      aMouseEvent->isShift);

  if (!doTableSelection)
  {  
    
    
#ifdef XP_MACOSX
    doTableSelection = aMouseEvent->isMeta || (aMouseEvent->isShift && selectingTableCells);
#else
    doTableSelection = aMouseEvent->isControl || (aMouseEvent->isShift && selectingTableCells);
#endif
  }
  if (!doTableSelection) 
    return NS_OK;

  
  nsIFrame *frame = this;
  PRBool foundCell = PR_FALSE;
  PRBool foundTable = PR_FALSE;

  
  nsIContent* limiter = aFrameSelection->GetLimiter();

  
  
  if (limiter && nsContentUtils::ContentIsDescendantOf(limiter, GetContent()))
    return NS_OK;

  
  
  
  
  
  while (frame)
  {
    
    nsITableCellLayout *cellElement = do_QueryFrame(frame);
    if (cellElement)
    {
      foundCell = PR_TRUE;
      
      
      break;
    }
    else
    {
      
      
      
      nsITableLayout *tableElement = do_QueryFrame(frame);
      if (tableElement)
      {
        foundTable = PR_TRUE;
        
        
        break;
      } else {
        frame = frame->GetParent();
        
        if (frame && frame->GetContent() == limiter)
          break;
      }
    }
  }
  
  if (!foundCell && !foundTable) return NS_OK;

  nsIContent* tableOrCellContent = frame->GetContent();
  if (!tableOrCellContent) return NS_ERROR_FAILURE;

  nsCOMPtr<nsIContent> parentContent = tableOrCellContent->GetParent();
  if (!parentContent) return NS_ERROR_FAILURE;

  PRInt32 offset = parentContent->IndexOf(tableOrCellContent);
  
  if (offset < 0) return NS_ERROR_FAILURE;

  
  *aParentContent = parentContent;
  NS_ADDREF(*aParentContent);

  *aContentOffset = offset;

#if 0
  if (selectRow)
    *aTarget = nsISelectionPrivate::TABLESELECTION_ROW;
  else if (selectColumn)
    *aTarget = nsISelectionPrivate::TABLESELECTION_COLUMN;
  else 
#endif
  if (foundCell)
    *aTarget = nsISelectionPrivate::TABLESELECTION_CELL;
  else if (foundTable)
    *aTarget = nsISelectionPrivate::TABLESELECTION_TABLE;

  return NS_OK;
}

NS_IMETHODIMP
nsFrame::IsSelectable(PRBool* aSelectable, PRUint8* aSelectStyle) const
{
  if (!aSelectable) 
    return NS_ERROR_NULL_POINTER;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRUint8 selectStyle  = NS_STYLE_USER_SELECT_AUTO;
  nsIFrame* frame      = (nsIFrame*)this;

  while (frame) {
    const nsStyleUIReset* userinterface = frame->GetStyleUIReset();
    switch (userinterface->mUserSelect) {
      case NS_STYLE_USER_SELECT_ALL:
      case NS_STYLE_USER_SELECT_NONE:
      case NS_STYLE_USER_SELECT_MOZ_ALL:
        
        selectStyle = userinterface->mUserSelect;
        break;
      default:
        
        if (selectStyle == NS_STYLE_USER_SELECT_AUTO) {
          selectStyle = userinterface->mUserSelect;
        }
        break;
    }
    frame = frame->GetParent();
  }

  
  if (selectStyle == NS_STYLE_USER_SELECT_AUTO)
    selectStyle = NS_STYLE_USER_SELECT_TEXT;
  else
  if (selectStyle == NS_STYLE_USER_SELECT_MOZ_ALL)
    selectStyle = NS_STYLE_USER_SELECT_ALL;
  else
  if (selectStyle == NS_STYLE_USER_SELECT_MOZ_NONE)
    selectStyle = NS_STYLE_USER_SELECT_NONE;

  
  if (aSelectable)
    *aSelectable = (selectStyle != NS_STYLE_USER_SELECT_NONE);
  if (aSelectStyle)
    *aSelectStyle = selectStyle;
  if (mState & NS_FRAME_GENERATED_CONTENT)
    *aSelectable = PR_FALSE;
  return NS_OK;
}




NS_IMETHODIMP
nsFrame::HandlePress(nsPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  
  
  
  PRBool eventOK;
  aPresContext->EventStateManager()->EventStatusOK(aEvent, &eventOK);
  if (!eventOK) 
    return NS_OK;

  nsresult rv;
  nsIPresShell *shell = aPresContext->GetPresShell();
  if (!shell)
    return NS_ERROR_FAILURE;

  
  
  
  PRInt16 isEditor = shell->GetSelectionFlags();
  
  isEditor = isEditor == nsISelectionDisplay::DISPLAY_ALL;

  nsInputEvent* keyEvent = (nsInputEvent*)aEvent;
  if (!keyEvent->isAlt) {
    
    for (nsIContent* content = mContent; content;
         content = content->GetParent()) {
      if (nsContentUtils::ContentIsDraggable(content) &&
          !content->IsEditable()) {
        
        if ((mRect - GetPosition()).Contains(
               nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this)))
          return NS_OK;
      }
    }
  }

  
  
  PRBool  selectable;
  PRUint8 selectStyle;
  rv = IsSelectable(&selectable, &selectStyle);
  if (NS_FAILED(rv)) return rv;
  
  
  if (!selectable)
    return NS_OK;

  
  
  PRBool useFrameSelection = (selectStyle == NS_STYLE_USER_SELECT_TEXT);

  
  
  
  
  
  if (!nsIPresShell::GetCapturingContent()) {
    nsIFrame* checkFrame = this;
    nsIScrollableFrame *scrollFrame = nsnull;
    while (checkFrame) {
      scrollFrame = do_QueryFrame(checkFrame);
      if (scrollFrame) {
        nsIPresShell::SetCapturingContent(checkFrame->GetContent(), CAPTURE_IGNOREALLOWED);
        break;
      }
      checkFrame = checkFrame->GetParent();
    }
  }

  
  
  const nsFrameSelection* frameselection = nsnull;
  if (useFrameSelection)
    frameselection = GetConstFrameSelection();
  else
    frameselection = shell->ConstFrameSelection();

  if (frameselection->GetDisplaySelection() == nsISelectionController::SELECTION_OFF)
    return NS_OK;

  nsMouseEvent *me = (nsMouseEvent *)aEvent;

#ifdef XP_MACOSX
  if (me->isControl)
    return NS_OK;
  PRBool control = me->isMeta;
#else
  PRBool control = me->isControl;
#endif

  nsCOMPtr<nsFrameSelection> fc = const_cast<nsFrameSelection*>(frameselection);
  if (me->clickCount >1 )
  {
    
    
    fc->SetMouseDownState(PR_TRUE);
    fc->SetMouseDoubleDown(PR_TRUE);
    return HandleMultiplePress(aPresContext, aEvent, aEventStatus, control);
  }

  nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
  ContentOffsets offsets = GetContentOffsetsFromPoint(pt);

  if (!offsets.content)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIContent>parentContent;
  PRInt32  contentOffset;
  PRInt32 target;
  rv = GetDataForTableSelection(frameselection, shell, me, getter_AddRefs(parentContent), &contentOffset, &target);
  if (NS_SUCCEEDED(rv) && parentContent)
  {
    fc->SetMouseDownState(PR_TRUE);
    return fc->HandleTableSelection(parentContent, contentOffset, target, me);
  }

  fc->SetDelayedCaretData(0);

  
  
  
  

  SelectionDetails *details = 0;
  PRBool isSelected = ((GetStateBits() & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT);

  if (isSelected)
  {
    PRBool inSelection = PR_FALSE;
    details = frameselection->LookUpSelection(offsets.content, 0,
        offsets.EndOffset(), PR_FALSE);

    
    
    
    

    SelectionDetails *curDetail = details;

    while (curDetail)
    {
      
      
      
      
      
      
      if (curDetail->mType != nsISelectionController::SELECTION_SPELLCHECK &&
          curDetail->mType != nsISelectionController::SELECTION_FIND &&
          curDetail->mStart <= offsets.StartOffset() &&
          offsets.EndOffset() <= curDetail->mEnd)
      {
        inSelection = PR_TRUE;
      }

      SelectionDetails *nextDetail = curDetail->mNext;
      delete curDetail;
      curDetail = nextDetail;
    }

    if (inSelection) {
      fc->SetMouseDownState(PR_FALSE);
      fc->SetDelayedCaretData(me);
      return NS_OK;
    }
  }

  fc->SetMouseDownState(PR_TRUE);

  
  
  rv = fc->HandleClick(offsets.content, offsets.StartOffset(),
                       offsets.EndOffset(), me->isShift, control,
                       offsets.associateWithNext);

  if (NS_FAILED(rv))
    return rv;

  if (offsets.offset != offsets.secondaryOffset)
    fc->MaintainSelection();

  if (isEditor && !me->isShift &&
      (offsets.EndOffset() - offsets.StartOffset()) == 1)
  {
    
    
    
    
    
    fc->SetMouseDownState(PR_FALSE);
  }

  return rv;
}





NS_IMETHODIMP
nsFrame::HandleMultiplePress(nsPresContext* aPresContext, 
                             nsGUIEvent*    aEvent,
                             nsEventStatus* aEventStatus,
                             PRBool         aControlHeld)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }

  
  
  
  
  nsSelectionAmount beginAmount, endAmount;
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  if (!me) return NS_OK;

  if (me->clickCount == 4) {
    beginAmount = endAmount = eSelectParagraph;
  } else if (me->clickCount == 3) {
    if (nsContentUtils::GetBoolPref("browser.triple_click_selects_paragraph")) {
      beginAmount = endAmount = eSelectParagraph;
    } else {
      beginAmount = eSelectBeginLine;
      endAmount = eSelectEndLine;
    }
  } else if (me->clickCount == 2) {
    
    beginAmount = endAmount = eSelectWord;
  } else {
    return NS_OK;
  }

  nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
  ContentOffsets offsets = GetContentOffsetsFromPoint(pt);
  if (!offsets.content) return NS_ERROR_FAILURE;

  nsIFrame* theFrame;
  PRInt32 offset;
  
  const nsFrameSelection* frameSelection =
    PresContext()->GetPresShell()->ConstFrameSelection();
  theFrame = frameSelection->
    GetFrameForNodeOffset(offsets.content, offsets.offset,
                          nsFrameSelection::HINT(offsets.associateWithNext),
                          &offset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  nsFrame* frame = static_cast<nsFrame*>(theFrame);

  return frame->PeekBackwardAndForward(beginAmount, endAmount,
                                       offsets.offset, aPresContext,
                                       beginAmount != eSelectWord,
                                       aControlHeld);
}

NS_IMETHODIMP
nsFrame::PeekBackwardAndForward(nsSelectionAmount aAmountBack,
                                nsSelectionAmount aAmountForward,
                                PRInt32 aStartPos,
                                nsPresContext* aPresContext,
                                PRBool aJumpLines,
                                PRBool aMultipleSelection)
{
  nsIFrame* baseFrame = this;
  PRInt32 baseOffset = aStartPos;
  nsresult rv;

  if (aAmountBack == eSelectWord) {
    
    
    nsPeekOffsetStruct pos;
    pos.SetData(eSelectCharacter,
                eDirNext,
                aStartPos,
                0,
                aJumpLines,
                PR_TRUE,  
                PR_FALSE,
                PR_FALSE);
    rv = PeekOffset(&pos);
    if (NS_SUCCEEDED(rv)) {
      baseFrame = pos.mResultFrame;
      baseOffset = pos.mContentOffset;
    }
  }

  
  nsPeekOffsetStruct startpos;
  startpos.SetData(aAmountBack,
                   eDirPrevious,
                   baseOffset, 
                   0,
                   aJumpLines,
                   PR_TRUE,  
                   PR_FALSE,
                   PR_FALSE);
  rv = baseFrame->PeekOffset(&startpos);
  if (NS_FAILED(rv))
    return rv;

  nsPeekOffsetStruct endpos;
  endpos.SetData(aAmountForward,
                 eDirNext,
                 aStartPos, 
                 0,
                 aJumpLines,
                 PR_TRUE,  
                 PR_FALSE,
                 PR_FALSE);
  rv = PeekOffset(&endpos);
  if (NS_FAILED(rv))
    return rv;

  
  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();

  rv = frameSelection->HandleClick(startpos.mResultContent,
                                   startpos.mContentOffset, startpos.mContentOffset,
                                   PR_FALSE, aMultipleSelection,
                                   nsFrameSelection::HINTRIGHT);
  if (NS_FAILED(rv))
    return rv;

  rv = frameSelection->HandleClick(endpos.mResultContent,
                                   endpos.mContentOffset, endpos.mContentOffset,
                                   PR_TRUE, PR_FALSE,
                                   nsFrameSelection::HINTLEFT);
  if (NS_FAILED(rv))
    return rv;

  
  return frameSelection->MaintainSelection(aAmountBack);
}

NS_IMETHODIMP nsFrame::HandleDrag(nsPresContext* aPresContext, 
                                  nsGUIEvent*     aEvent,
                                  nsEventStatus*  aEventStatus)
{
  PRBool  selectable;
  PRUint8 selectStyle;
  IsSelectable(&selectable, &selectStyle);
  
  
  
  if (!selectable)
    return NS_OK;
  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }
  nsIPresShell *presShell = aPresContext->PresShell();

  nsCOMPtr<nsFrameSelection> frameselection = GetFrameSelection();
  PRBool mouseDown = frameselection->GetMouseDownState();
  if (!mouseDown)
    return NS_OK;

  frameselection->StopAutoScrollTimer();

  
  nsCOMPtr<nsIContent> parentContent;
  PRInt32 contentOffset;
  PRInt32 target;
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  nsresult result;
  result = GetDataForTableSelection(frameselection, presShell, me,
                                    getter_AddRefs(parentContent),
                                    &contentOffset, &target);      

  nsWeakFrame weakThis = this;
  if (NS_SUCCEEDED(result) && parentContent) {
    frameselection->HandleTableSelection(parentContent, contentOffset, target, me);
  } else {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
    frameselection->HandleDrag(this, pt);
  }

  
  
  if (!weakThis.IsAlive()) {
    return NS_OK;
  }

  
  nsIFrame* checkFrame = this;
  nsIScrollableFrame *scrollFrame = nsnull;
  while (checkFrame) {
    scrollFrame = do_QueryFrame(checkFrame);
    if (scrollFrame) {
      break;
    }
    checkFrame = checkFrame->GetParent();
  }

  if (scrollFrame) {
    nsIFrame* capturingFrame = scrollFrame->GetScrolledFrame();
    if (capturingFrame) {
      nsPoint pt =
        nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, capturingFrame);
      frameselection->StartAutoScrollTimer(capturingFrame, pt, 30);
    }
  }

  return NS_OK;
}





static nsresult
HandleFrameSelection(nsFrameSelection*         aFrameSelection,
                     nsIFrame::ContentOffsets& aOffsets,
                     PRBool                    aHandleTableSel,
                     PRInt32                   aContentOffsetForTableSel,
                     PRInt32                   aTargetForTableSel,
                     nsIContent*               aParentContentForTableSel,
                     nsGUIEvent*               aEvent,
                     nsEventStatus*            aEventStatus)
{
  if (!aFrameSelection) {
    return NS_OK;
  }

  nsresult rv = NS_OK;

  if (nsEventStatus_eConsumeNoDefault != *aEventStatus) {
    if (!aHandleTableSel) {
      nsMouseEvent *me = aFrameSelection->GetDelayedCaretData();
      if (!aOffsets.content || !me) {
        return NS_ERROR_FAILURE;
      }

      
      
      
      
      
      
      
      
      
      aFrameSelection->SetMouseDownState(PR_TRUE);

      rv = aFrameSelection->HandleClick(aOffsets.content,
                                        aOffsets.StartOffset(),
                                        aOffsets.EndOffset(),
                                        me->isShift, PR_FALSE,
                                        aOffsets.associateWithNext);
      if (NS_FAILED(rv)) {
        return rv;
      }
    } else if (aParentContentForTableSel) {
      aFrameSelection->SetMouseDownState(PR_FALSE);
      rv = aFrameSelection->HandleTableSelection(aParentContentForTableSel,
                                                 aContentOffsetForTableSel,
                                                 aTargetForTableSel,
                                                 (nsMouseEvent *)aEvent);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
    aFrameSelection->SetDelayedCaretData(0);
  }

  aFrameSelection->SetMouseDownState(PR_FALSE);
  aFrameSelection->StopAutoScrollTimer();

  return NS_OK;
}

NS_IMETHODIMP nsFrame::HandleRelease(nsPresContext* aPresContext,
                                     nsGUIEvent*    aEvent,
                                     nsEventStatus* aEventStatus)
{
  nsIFrame* activeFrame = GetActiveSelectionFrame(aPresContext, this);

  nsCOMPtr<nsIContent> captureContent = nsIPresShell::GetCapturingContent();

  
  
  nsIPresShell::SetCapturingContent(nsnull, 0);

  PRBool selectionOff =
    (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF);

  nsRefPtr<nsFrameSelection> frameselection;
  ContentOffsets offsets;
  nsCOMPtr<nsIContent> parentContent;
  PRInt32 contentOffsetForTableSel = 0;
  PRInt32 targetForTableSel = 0;
  PRBool handleTableSelection = PR_TRUE;

  if (!selectionOff) {
    frameselection = GetFrameSelection();
    if (nsEventStatus_eConsumeNoDefault != *aEventStatus && frameselection) {
      
      
      

      PRBool mouseDown = frameselection->GetMouseDownState();
      nsMouseEvent *me = frameselection->GetDelayedCaretData();

      if (!mouseDown && me && me->clickCount < 2) {
        nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
        offsets = GetContentOffsetsFromPoint(pt);
        handleTableSelection = PR_FALSE;
      } else {
        GetDataForTableSelection(frameselection, PresContext()->PresShell(),
                                 (nsMouseEvent *)aEvent,
                                 getter_AddRefs(parentContent),
                                 &contentOffsetForTableSel,
                                 &targetForTableSel);
      }
    }
  }

  
  
  
  nsRefPtr<nsFrameSelection> frameSelection;
  if (activeFrame != this &&
      static_cast<nsFrame*>(activeFrame)->DisplaySelection(activeFrame->PresContext())
        != nsISelectionController::SELECTION_OFF) {
      frameSelection = activeFrame->GetFrameSelection();
  }

  
  
  if (!frameSelection && captureContent) {
    nsIDocument* doc = captureContent->GetCurrentDoc();
    if (doc) {
      nsIPresShell* capturingShell = doc->GetShell();
      if (capturingShell && capturingShell != PresContext()->GetPresShell()) {
        frameSelection = capturingShell->FrameSelection();
      }
    }
  }

  if (frameSelection) {
    frameSelection->SetMouseDownState(PR_FALSE);
    frameSelection->StopAutoScrollTimer();
  }

  
  

  return selectionOff
    ? NS_OK
    : HandleFrameSelection(frameselection, offsets, handleTableSelection,
                           contentOffsetForTableSel, targetForTableSel,
                           parentContent, aEvent, aEventStatus);
}

struct NS_STACK_CLASS FrameContentRange {
  FrameContentRange(nsIContent* aContent, PRInt32 aStart, PRInt32 aEnd) :
    content(aContent), start(aStart), end(aEnd) { }
  nsCOMPtr<nsIContent> content;
  PRInt32 start;
  PRInt32 end;
};


static FrameContentRange GetRangeForFrame(nsIFrame* aFrame) {
  nsCOMPtr<nsIContent> content, parent;
  content = aFrame->GetContent();
  if (!content) {
    NS_WARNING("Frame has no content");
    return FrameContentRange(nsnull, -1, -1);
  }
  nsIAtom* type = aFrame->GetType();
  if (type == nsGkAtoms::textFrame) {
    PRInt32 offset, offsetEnd;
    aFrame->GetOffsets(offset, offsetEnd);
    return FrameContentRange(content, offset, offsetEnd);
  }
  if (type == nsGkAtoms::brFrame) {
    parent = content->GetParent();
    PRInt32 beginOffset = parent->IndexOf(content);
    return FrameContentRange(parent, beginOffset, beginOffset);
  }
  
  
  do {
    parent  = content->GetParent();
    if (parent) {
      PRInt32 beginOffset = parent->IndexOf(content);
      if (beginOffset >= 0)
        return FrameContentRange(parent, beginOffset, beginOffset + 1);
      content = parent;
    }
  } while (parent);

  
  return FrameContentRange(content, 0, content->GetChildCount());
}





struct FrameTarget {
  FrameTarget(nsIFrame* aFrame, PRBool aFrameEdge, PRBool aAfterFrame,
              PRBool aEmptyBlock = PR_FALSE) :
    frame(aFrame), frameEdge(aFrameEdge), afterFrame(aAfterFrame),
    emptyBlock(aEmptyBlock) { }
  static FrameTarget Null() {
    return FrameTarget(nsnull, PR_FALSE, PR_FALSE);
  }
  PRBool IsNull() {
    return !frame;
  }
  nsIFrame* frame;
  PRPackedBool frameEdge;
  PRPackedBool afterFrame;
  PRPackedBool emptyBlock;
};


static FrameTarget GetSelectionClosestFrame(nsIFrame* aFrame, nsPoint aPoint);

static PRBool SelfIsSelectable(nsIFrame* aFrame)
{
  return !(aFrame->IsGeneratedContentFrame() ||
           aFrame->GetStyleUIReset()->mUserSelect == NS_STYLE_USER_SELECT_NONE);
}

static PRBool SelectionDescendToKids(nsIFrame* aFrame) {
  PRUint8 style = aFrame->GetStyleUIReset()->mUserSelect;
  nsIFrame* parent = aFrame->GetParent();
  
  
  
  
  
  
  
  
  return !aFrame->IsGeneratedContentFrame() &&
         style != NS_STYLE_USER_SELECT_ALL  &&
         style != NS_STYLE_USER_SELECT_NONE &&
         ((parent->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION) ||
          !(aFrame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION));
}

static FrameTarget GetSelectionClosestFrameForChild(nsIFrame* aChild,
                                                    nsPoint aPoint)
{
  nsIFrame* parent = aChild->GetParent();
  if (SelectionDescendToKids(aChild)) {
    nsPoint pt = aPoint - aChild->GetOffsetTo(parent);
    return GetSelectionClosestFrame(aChild, pt);
  }
  return FrameTarget(aChild, PR_FALSE, PR_FALSE);
}






static FrameTarget DrillDownToSelectionFrame(nsIFrame* aFrame,
                                             PRBool aEndFrame) {
  if (SelectionDescendToKids(aFrame)) {
    nsIFrame* result = nsnull;
    nsIFrame *frame = aFrame->GetFirstChild(nsnull);
    if (!aEndFrame) {
      while (frame && (!SelfIsSelectable(frame) ||
                        frame->IsEmpty()))
        frame = frame->GetNextSibling();
      if (frame)
        result = frame;
    } else {
      
      
      
      
      while (frame) {
        if (!frame->IsEmpty() && SelfIsSelectable(frame))
          result = frame;
        frame = frame->GetNextSibling();
      }
    }
    if (result)
      return DrillDownToSelectionFrame(result, aEndFrame);
  }
  
  return FrameTarget(aFrame, PR_TRUE, aEndFrame);
}



static FrameTarget GetSelectionClosestFrameForLine(
                      nsBlockFrame* aParent,
                      nsBlockFrame::line_iterator aLine,
                      nsPoint aPoint)
{
  nsIFrame *frame = aLine->mFirstChild;
  
  if (aLine == aParent->end_lines())
    return DrillDownToSelectionFrame(aParent, PR_TRUE);
  nsIFrame *closestFromLeft = nsnull, *closestFromRight = nsnull;
  nsRect rect = aLine->mBounds;
  nscoord closestLeft = rect.x, closestRight = rect.XMost();
  for (PRInt32 n = aLine->GetChildCount(); n;
       --n, frame = frame->GetNextSibling()) {
    if (!SelfIsSelectable(frame) || frame->IsEmpty())
      continue;
    nsRect frameRect = frame->GetRect();
    if (aPoint.x >= frameRect.x) {
      if (aPoint.x < frameRect.XMost()) {
        return GetSelectionClosestFrameForChild(frame, aPoint);
      }
      if (frameRect.XMost() >= closestLeft) {
        closestFromLeft = frame;
        closestLeft = frameRect.XMost();
      }
    } else {
      if (frameRect.x <= closestRight) {
        closestFromRight = frame;
        closestRight = frameRect.x;
      }
    }
  }
  if (!closestFromLeft && !closestFromRight) {
    
    
    return FrameTarget::Null();
  }
  if (closestFromLeft &&
      (!closestFromRight ||
       (abs(aPoint.x - closestLeft) <= abs(aPoint.x - closestRight)))) {
    return GetSelectionClosestFrameForChild(closestFromLeft, aPoint);
  }
  return GetSelectionClosestFrameForChild(closestFromRight, aPoint);
}






static FrameTarget GetSelectionClosestFrameForBlock(nsIFrame* aFrame,
                                                    nsPoint aPoint)
{
  nsBlockFrame* bf = nsLayoutUtils::GetAsBlock(aFrame); 
  if (!bf)
    return FrameTarget::Null();

  
  nsBlockFrame::line_iterator firstLine = bf->begin_lines();
  nsBlockFrame::line_iterator end = bf->end_lines();
  if (firstLine == end) {
    nsIContent *blockContent = aFrame->GetContent();
    if (blockContent && blockContent->IsEditable()) {
      
      
      return FrameTarget(aFrame, PR_FALSE, PR_FALSE, PR_TRUE);
    }
    return FrameTarget::Null();
  }
  nsBlockFrame::line_iterator curLine = firstLine;
  nsBlockFrame::line_iterator closestLine = end;
  while (curLine != end) {
    
    nscoord y = aPoint.y - curLine->mBounds.y;
    nscoord height = curLine->mBounds.height;
    if (y >= 0 && y < height) {
      closestLine = curLine;
      break; 
    }
    if (y < 0)
      break;
    ++curLine;
  }

  if (closestLine == end) {
    nsBlockFrame::line_iterator prevLine = curLine.prev();
    nsBlockFrame::line_iterator nextLine = curLine;
    
    while (nextLine != end && nextLine->IsEmpty())
      ++nextLine;
    while (prevLine != end && prevLine->IsEmpty())
      --prevLine;

    
    
    
    PRInt32 dragOutOfFrame =
            nsContentUtils::GetIntPref("browser.drag_out_of_frame_style");

    if (prevLine == end) {
      if (dragOutOfFrame == 1 || nextLine == end)
        return DrillDownToSelectionFrame(aFrame, PR_FALSE);
      closestLine = nextLine;
    } else if (nextLine == end) {
      if (dragOutOfFrame == 1)
        return DrillDownToSelectionFrame(aFrame, PR_TRUE);
      closestLine = prevLine;
    } else { 
      if (aPoint.y - prevLine->mBounds.YMost() < nextLine->mBounds.y - aPoint.y)
        closestLine = prevLine;
      else
        closestLine = nextLine;
    }
  }

  do {
    FrameTarget target = GetSelectionClosestFrameForLine(bf, closestLine,
                                                         aPoint);
    if (!target.IsNull())
      return target;
    ++closestLine;
  } while (closestLine != end);
  
  return DrillDownToSelectionFrame(aFrame, PR_TRUE);
}








static FrameTarget GetSelectionClosestFrame(nsIFrame* aFrame, nsPoint aPoint)
{
  {
    
    FrameTarget target = GetSelectionClosestFrameForBlock(aFrame, aPoint);
    if (!target.IsNull())
      return target;
  }

  nsIFrame *kid = aFrame->GetFirstChild(nsnull);

  if (kid) {
    

    
    const nscoord HUGE_DISTANCE = nscoord_MAX;
    nscoord closestXDistance = HUGE_DISTANCE;
    nscoord closestYDistance = HUGE_DISTANCE;
    nsIFrame *closestFrame = nsnull;

    for (; kid; kid = kid->GetNextSibling()) {
      if (!SelfIsSelectable(kid) || kid->IsEmpty())
        continue;

      nsRect rect = kid->GetRect();

      nscoord fromLeft = aPoint.x - rect.x;
      nscoord fromRight = aPoint.x - rect.XMost();

      nscoord xDistance;
      if (fromLeft >= 0 && fromRight <= 0) {
        xDistance = 0;
      } else {
        xDistance = NS_MIN(abs(fromLeft), abs(fromRight));
      }

      if (xDistance <= closestXDistance)
      {
        if (xDistance < closestXDistance)
          closestYDistance = HUGE_DISTANCE;

        nscoord fromTop = aPoint.y - rect.y;
        nscoord fromBottom = aPoint.y - rect.YMost();

        nscoord yDistance;
        if (fromTop >= 0 && fromBottom <= 0)
          yDistance = 0;
        else
          yDistance = NS_MIN(abs(fromTop), abs(fromBottom));

        if (yDistance < closestYDistance)
        {
          closestXDistance = xDistance;
          closestYDistance = yDistance;
          closestFrame = kid;
        }
      }
    }
    if (closestFrame)
      return GetSelectionClosestFrameForChild(closestFrame, aPoint);
  }
  return FrameTarget(aFrame, PR_FALSE, PR_FALSE);
}

nsIFrame::ContentOffsets OffsetsForSingleFrame(nsIFrame* aFrame, nsPoint aPoint)
{
  nsIFrame::ContentOffsets offsets;
  FrameContentRange range = GetRangeForFrame(aFrame);
  offsets.content = range.content;
  
  
  if (aFrame->GetNextContinuation() || aFrame->GetPrevContinuation()) {
    offsets.offset = range.start;
    offsets.secondaryOffset = range.end;
    offsets.associateWithNext = PR_TRUE;
    return offsets;
  }

  
  nsRect rect(nsPoint(0, 0), aFrame->GetSize());

  PRBool isBlock = (aFrame->GetStyleDisplay()->mDisplay != NS_STYLE_DISPLAY_INLINE);
  PRBool isRtl = (aFrame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL);
  if ((isBlock && rect.y < aPoint.y) ||
      (!isBlock && ((isRtl  && rect.x + rect.width / 2 > aPoint.x) || 
                    (!isRtl && rect.x + rect.width / 2 < aPoint.x)))) {
    offsets.offset = range.end;
    if (rect.Contains(aPoint))
      offsets.secondaryOffset = range.start;
    else
      offsets.secondaryOffset = range.end;
  } else {
    offsets.offset = range.start;
    if (rect.Contains(aPoint))
      offsets.secondaryOffset = range.end;
    else
      offsets.secondaryOffset = range.start;
  }
  offsets.associateWithNext = (offsets.offset == range.start);
  return offsets;
}

static nsIFrame* AdjustFrameForSelectionStyles(nsIFrame* aFrame) {
  nsIFrame* adjustedFrame = aFrame;
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent())
  {
    
    
    if (frame->GetStyleUIReset()->mUserSelect == NS_STYLE_USER_SELECT_NONE || 
        frame->GetStyleUIReset()->mUserSelect == NS_STYLE_USER_SELECT_ALL || 
        frame->IsGeneratedContentFrame()) {
      adjustedFrame = frame;
    }
  }
  return adjustedFrame;
}
  

nsIFrame::ContentOffsets nsIFrame::GetContentOffsetsFromPoint(nsPoint aPoint,
                                                              PRBool aIgnoreSelectionStyle)
{
  nsIFrame *adjustedFrame;
  if (aIgnoreSelectionStyle) {
    adjustedFrame = this;
  }
  else {
    
    
    
    
    

    adjustedFrame = AdjustFrameForSelectionStyles(this);

    
    
    if (adjustedFrame && adjustedFrame->GetStyleUIReset()->mUserSelect ==
        NS_STYLE_USER_SELECT_ALL) {
      return OffsetsForSingleFrame(adjustedFrame, aPoint +
                                   this->GetOffsetTo(adjustedFrame));
    }

    
    
    if (adjustedFrame != this)
      adjustedFrame = adjustedFrame->GetParent();
  }

  nsPoint adjustedPoint = aPoint + this->GetOffsetTo(adjustedFrame);

  FrameTarget closest = GetSelectionClosestFrame(adjustedFrame, adjustedPoint);

  if (closest.emptyBlock) {
    ContentOffsets offsets;
    NS_ASSERTION(closest.frame,
                 "closest.frame must not be null when it's empty");
    offsets.content = closest.frame->GetContent();
    offsets.offset = 0;
    offsets.secondaryOffset = 0;
    offsets.associateWithNext = PR_TRUE;
    return offsets;
  }

  
  
  if (closest.frameEdge) {
    ContentOffsets offsets;
    FrameContentRange range = GetRangeForFrame(closest.frame);
    offsets.content = range.content;
    if (closest.afterFrame)
      offsets.offset = range.end;
    else
      offsets.offset = range.start;
    offsets.secondaryOffset = offsets.offset;
    offsets.associateWithNext = (offsets.offset == range.start);
    return offsets;
  }
  nsPoint pt = aPoint - closest.frame->GetOffsetTo(this);
  return static_cast<nsFrame*>(closest.frame)->CalcContentOffsetsFromFramePoint(pt);

  
  
  
  
}

nsIFrame::ContentOffsets nsFrame::CalcContentOffsetsFromFramePoint(nsPoint aPoint)
{
  return OffsetsForSingleFrame(this, aPoint);
}

NS_IMETHODIMP
nsFrame::GetCursor(const nsPoint& aPoint,
                   nsIFrame::Cursor& aCursor)
{
  FillCursorInformationFromStyle(GetStyleUserInterface(), aCursor);
  if (NS_STYLE_CURSOR_AUTO == aCursor.mCursor) {
    aCursor.mCursor = NS_STYLE_CURSOR_DEFAULT;
  }


  return NS_OK;
}



 void
nsFrame::MarkIntrinsicWidthsDirty()
{
  
  
  if (IsBoxWrapped()) {
    nsBoxLayoutMetrics *metrics = BoxMetrics();

    SizeNeedsRecalc(metrics->mPrefSize);
    SizeNeedsRecalc(metrics->mMinSize);
    SizeNeedsRecalc(metrics->mMaxSize);
    SizeNeedsRecalc(metrics->mBlockPrefSize);
    SizeNeedsRecalc(metrics->mBlockMinSize);
    CoordNeedsRecalc(metrics->mFlex);
    CoordNeedsRecalc(metrics->mAscent);
  }
}

 nscoord
nsFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
nsFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);
  return result;
}

 void
nsFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                           nsIFrame::InlineMinWidthData *aData)
{
  NS_ASSERTION(GetParent(), "Must have a parent if we get here!");
  PRBool canBreak = !CanContinueTextRun() &&
    GetParent()->GetStyleText()->WhiteSpaceCanWrap();
  
  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = PR_FALSE;
  aData->trailingTextFrame = nsnull;
  aData->currentLine += nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                            this, nsLayoutUtils::MIN_WIDTH);
  aData->atStartOfLine = PR_FALSE;
  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
}

 void
nsFrame::AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                            nsIFrame::InlinePrefWidthData *aData)
{
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = PR_FALSE;
  nscoord myPref = nsLayoutUtils::IntrinsicForContainer(aRenderingContext, 
                       this, nsLayoutUtils::PREF_WIDTH);
  aData->currentLine = NSCoordSaturatingAdd(aData->currentLine, myPref);
}

void
nsIFrame::InlineMinWidthData::ForceBreak(nsRenderingContext *aRenderingContext)
{
  currentLine -= trailingWhitespace;
  prevLines = NS_MAX(prevLines, currentLine);
  currentLine = trailingWhitespace = 0;

  for (PRUint32 i = 0, i_end = floats.Length(); i != i_end; ++i) {
    nsIFrame *floatFrame = floats[i];
    nscoord float_min =
      nsLayoutUtils::IntrinsicForContainer(aRenderingContext, floatFrame,
                                           nsLayoutUtils::MIN_WIDTH);
    if (float_min > prevLines)
      prevLines = float_min;
  }
  floats.Clear();
  trailingTextFrame = nsnull;
  skipWhitespace = PR_TRUE;
}

void
nsIFrame::InlineMinWidthData::OptionallyBreak(nsRenderingContext *aRenderingContext,
                                              nscoord aHyphenWidth)
{
  trailingTextFrame = nsnull;

  
  
  
  
  
  if (currentLine + aHyphenWidth < 0 || atStartOfLine)
    return;
  currentLine += aHyphenWidth;
  ForceBreak(aRenderingContext);
}

void
nsIFrame::InlinePrefWidthData::ForceBreak(nsRenderingContext *aRenderingContext)
{
  if (floats.Length() != 0) {
            
            
    nscoord floats_done = 0,
            
            
            floats_cur_left = 0,
            floats_cur_right = 0;

    for (PRUint32 i = 0, i_end = floats.Length(); i != i_end; ++i) {
      nsIFrame *floatFrame = floats[i];
      const nsStyleDisplay *floatDisp = floatFrame->GetStyleDisplay();
      if (floatDisp->mBreakType == NS_STYLE_CLEAR_LEFT ||
          floatDisp->mBreakType == NS_STYLE_CLEAR_RIGHT ||
          floatDisp->mBreakType == NS_STYLE_CLEAR_LEFT_AND_RIGHT) {
        nscoord floats_cur = NSCoordSaturatingAdd(floats_cur_left,
                                                  floats_cur_right);
        if (floats_cur > floats_done)
          floats_done = floats_cur;
        if (floatDisp->mBreakType != NS_STYLE_CLEAR_RIGHT)
          floats_cur_left = 0;
        if (floatDisp->mBreakType != NS_STYLE_CLEAR_LEFT)
          floats_cur_right = 0;
      }

      nscoord &floats_cur = floatDisp->mFloats == NS_STYLE_FLOAT_LEFT
                              ? floats_cur_left : floats_cur_right;
      nscoord floatWidth =
          nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                                               floatFrame,
                                               nsLayoutUtils::PREF_WIDTH);
      
      
      floats_cur =
        NSCoordSaturatingAdd(floats_cur, NS_MAX(0, floatWidth));
    }

    nscoord floats_cur =
      NSCoordSaturatingAdd(floats_cur_left, floats_cur_right);
    if (floats_cur > floats_done)
      floats_done = floats_cur;

    currentLine = NSCoordSaturatingAdd(currentLine, floats_done);

    floats.Clear();
  }

  currentLine =
    NSCoordSaturatingSubtract(currentLine, trailingWhitespace, nscoord_MAX);
  prevLines = NS_MAX(prevLines, currentLine);
  currentLine = trailingWhitespace = 0;
  skipWhitespace = PR_TRUE;
}

static void
AddCoord(const nsStyleCoord& aStyle,
         nsRenderingContext* aRenderingContext,
         nsIFrame* aFrame,
         nscoord* aCoord, float* aPercent,
         PRBool aClampNegativeToZero)
{
  switch (aStyle.GetUnit()) {
    case eStyleUnit_Coord: {
      NS_ASSERTION(!aClampNegativeToZero || aStyle.GetCoordValue() >= 0,
                   "unexpected negative value");
      *aCoord += aStyle.GetCoordValue();
      return;
    }
    case eStyleUnit_Percent: {
      NS_ASSERTION(!aClampNegativeToZero || aStyle.GetPercentValue() >= 0.0f,
                   "unexpected negative value");
      *aPercent += aStyle.GetPercentValue();
      return;
    }
    case eStyleUnit_Calc: {
      const nsStyleCoord::Calc *calc = aStyle.GetCalcValue();
      if (aClampNegativeToZero) {
        
        *aCoord += NS_MAX(calc->mLength, 0);
        *aPercent += NS_MAX(calc->mPercent, 0.0f);
      } else {
        *aCoord += calc->mLength;
        *aPercent += calc->mPercent;
      }
      return;
    }
    default: {
      return;
    }
  }
}

 nsIFrame::IntrinsicWidthOffsetData
nsFrame::IntrinsicWidthOffsets(nsRenderingContext* aRenderingContext)
{
  IntrinsicWidthOffsetData result;

  const nsStyleMargin *styleMargin = GetStyleMargin();
  AddCoord(styleMargin->mMargin.GetLeft(), aRenderingContext, this,
           &result.hMargin, &result.hPctMargin, PR_FALSE);
  AddCoord(styleMargin->mMargin.GetRight(), aRenderingContext, this,
           &result.hMargin, &result.hPctMargin, PR_FALSE);

  const nsStylePadding *stylePadding = GetStylePadding();
  AddCoord(stylePadding->mPadding.GetLeft(), aRenderingContext, this,
           &result.hPadding, &result.hPctPadding, PR_TRUE);
  AddCoord(stylePadding->mPadding.GetRight(), aRenderingContext, this,
           &result.hPadding, &result.hPctPadding, PR_TRUE);

  const nsStyleBorder *styleBorder = GetStyleBorder();
  result.hBorder += styleBorder->GetActualBorderWidth(NS_SIDE_LEFT);
  result.hBorder += styleBorder->GetActualBorderWidth(NS_SIDE_RIGHT);

  const nsStyleDisplay *disp = GetStyleDisplay();
  if (IsThemed(disp)) {
    nsPresContext *presContext = PresContext();

    nsIntMargin border;
    presContext->GetTheme()->GetWidgetBorder(presContext->DeviceContext(),
                                             this, disp->mAppearance,
                                             &border);
    result.hBorder = presContext->DevPixelsToAppUnits(border.LeftRight());

    nsIntMargin padding;
    if (presContext->GetTheme()->GetWidgetPadding(presContext->DeviceContext(),
                                                  this, disp->mAppearance,
                                                  &padding)) {
      result.hPadding = presContext->DevPixelsToAppUnits(padding.LeftRight());
      result.hPctPadding = 0;
    }
  }

  return result;
}

 nsIFrame::IntrinsicSize
nsFrame::GetIntrinsicSize()
{
  return IntrinsicSize(); 
}

 nsSize
nsFrame::GetIntrinsicRatio()
{
  return nsSize(0, 0);
}

 nsSize
nsFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                     nsSize aCBSize, nscoord aAvailableWidth,
                     nsSize aMargin, nsSize aBorder, nsSize aPadding,
                     PRBool aShrinkWrap)
{
  nsSize result = ComputeAutoSize(aRenderingContext, aCBSize, aAvailableWidth,
                                  aMargin, aBorder, aPadding, aShrinkWrap);
  nsSize boxSizingAdjust(0,0);
  const nsStylePosition *stylePos = GetStylePosition();

  switch (stylePos->mBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      boxSizingAdjust += aBorder;
      
    case NS_STYLE_BOX_SIZING_PADDING:
      boxSizingAdjust += aPadding;
  }
  nscoord boxSizingToMarginEdgeWidth =
    aMargin.width + aBorder.width + aPadding.width - boxSizingAdjust.width;

  

  if (stylePos->mWidth.GetUnit() != eStyleUnit_Auto) {
    result.width =
      nsLayoutUtils::ComputeWidthValue(aRenderingContext, this,
        aCBSize.width, boxSizingAdjust.width, boxSizingToMarginEdgeWidth,
        stylePos->mWidth);
  }

  if (stylePos->mMaxWidth.GetUnit() != eStyleUnit_None) {
    nscoord maxWidth =
      nsLayoutUtils::ComputeWidthValue(aRenderingContext, this,
        aCBSize.width, boxSizingAdjust.width, boxSizingToMarginEdgeWidth,
        stylePos->mMaxWidth);
    if (maxWidth < result.width)
      result.width = maxWidth;
  }

  nscoord minWidth =
    nsLayoutUtils::ComputeWidthValue(aRenderingContext, this,
      aCBSize.width, boxSizingAdjust.width, boxSizingToMarginEdgeWidth,
      stylePos->mMinWidth);
  if (minWidth > result.width)
    result.width = minWidth;

  

  if (!nsLayoutUtils::IsAutoHeight(stylePos->mHeight, aCBSize.height)) {
    result.height =
      nsLayoutUtils::ComputeHeightValue(aCBSize.height, stylePos->mHeight) -
      boxSizingAdjust.height;
  }

  if (result.height != NS_UNCONSTRAINEDSIZE) {
    if (!nsLayoutUtils::IsAutoHeight(stylePos->mMaxHeight, aCBSize.height)) {
      nscoord maxHeight =
        nsLayoutUtils::ComputeHeightValue(aCBSize.height, stylePos->mMaxHeight) -
        boxSizingAdjust.height;
      if (maxHeight < result.height)
        result.height = maxHeight;
    }

    if (!nsLayoutUtils::IsAutoHeight(stylePos->mMinHeight, aCBSize.height)) {
      nscoord minHeight =
        nsLayoutUtils::ComputeHeightValue(aCBSize.height, stylePos->mMinHeight) -
        boxSizingAdjust.height;
      if (minHeight > result.height)
        result.height = minHeight;
    }
  }

  const nsStyleDisplay *disp = GetStyleDisplay();
  if (IsThemed(disp)) {
    nsIntSize widget(0, 0);
    PRBool canOverride = PR_TRUE;
    nsPresContext *presContext = PresContext();
    presContext->GetTheme()->
      GetMinimumWidgetSize(aRenderingContext, this, disp->mAppearance,
                           &widget, &canOverride);

    nsSize size;
    size.width = presContext->DevPixelsToAppUnits(widget.width);
    size.height = presContext->DevPixelsToAppUnits(widget.height);

    
    size.width -= aBorder.width + aPadding.width;
    size.height -= aBorder.height + aPadding.height;

    if (size.height > result.height || !canOverride)
      result.height = size.height;
    if (size.width > result.width || !canOverride)
      result.width = size.width;
  }

  if (result.width < 0)
    result.width = 0;

  if (result.height < 0)
    result.height = 0;

  return result;
}

nsRect
nsIFrame::ComputeTightBounds(gfxContext* aContext) const
{
  return GetVisualOverflowRect();
}

nsRect
nsFrame::ComputeSimpleTightBounds(gfxContext* aContext) const
{
  if (GetStyleOutline()->GetOutlineStyle() != NS_STYLE_BORDER_STYLE_NONE ||
      HasBorder() || !GetStyleBackground()->IsTransparent() ||
      GetStyleDisplay()->mAppearance) {
    
    
    return GetVisualOverflowRect();
  }

  nsRect r(0, 0, 0, 0);
  PRInt32 listIndex = 0;
  nsIAtom* childList = nsnull;
  do {
    nsIFrame* child = GetFirstChild(childList);
    while (child) {
       r.UnionRect(r, child->ComputeTightBounds(aContext) + child->GetPosition());
       child = child->GetNextSibling();
    }
    childList = GetAdditionalChildListName(listIndex++);
  } while (childList);
  return r;
}

 nsSize
nsFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                         nsSize aCBSize, nscoord aAvailableWidth,
                         nsSize aMargin, nsSize aBorder, nsSize aPadding,
                         PRBool aShrinkWrap)
{
  
  nsSize result(0xdeadbeef, NS_UNCONSTRAINEDSIZE);

  
  if (GetStylePosition()->mWidth.GetUnit() == eStyleUnit_Auto) {
    nscoord availBased = aAvailableWidth - aMargin.width - aBorder.width -
                         aPadding.width;
    result.width = ShrinkWidthToFit(aRenderingContext, availBased);
  }
  return result;
}

nscoord
nsFrame::ShrinkWidthToFit(nsRenderingContext *aRenderingContext,
                          nscoord aWidthInCB)
{
  nscoord result;
  nscoord minWidth = GetMinWidth(aRenderingContext);
  if (minWidth > aWidthInCB) {
    result = minWidth;
  } else {
    nscoord prefWidth = GetPrefWidth(aRenderingContext);
    if (prefWidth > aWidthInCB) {
      result = aWidthInCB;
    } else {
      result = prefWidth;
    }
  }
  return result;
}

NS_IMETHODIMP
nsFrame::WillReflow(nsPresContext* aPresContext)
{
#ifdef DEBUG_dbaron_off
  
  NS_ASSERTION(!(mState & NS_FRAME_IN_REFLOW),
               "nsFrame::WillReflow: frame is already in reflow");
#endif

  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     ("WillReflow: oldState=%x", mState));
  mState |= NS_FRAME_IN_REFLOW;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::DidReflow(nsPresContext*           aPresContext,
                   const nsHTMLReflowState*  aReflowState,
                   nsDidReflowStatus         aStatus)
{
  NS_FRAME_TRACE_MSG(NS_FRAME_TRACE_CALLS,
                     ("nsFrame::DidReflow: aStatus=%d", aStatus));
  if (NS_FRAME_REFLOW_FINISHED == aStatus) {
    mState &= ~(NS_FRAME_IN_REFLOW | NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
                NS_FRAME_HAS_DIRTY_CHILDREN);
  }

  
  
  
  
  if (aReflowState && aReflowState->mPercentHeightObserver &&
      !GetPrevInFlow()) {
    const nsStyleCoord &height = aReflowState->mStylePosition->mHeight;
    if (height.HasPercent()) {
      aReflowState->mPercentHeightObserver->NotifyPercentHeight(*aReflowState);
    }
  }

  return NS_OK;
}

 PRBool
nsFrame::CanContinueTextRun() const
{
  
  
  return PR_FALSE;
}

NS_IMETHODIMP
nsFrame::Reflow(nsPresContext*          aPresContext,
                nsHTMLReflowMetrics&     aDesiredSize,
                const nsHTMLReflowState& aReflowState,
                nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFrame");
  aDesiredSize.width = 0;
  aDesiredSize.height = 0;
  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::CharacterDataChanged(CharacterDataChangeInfo* aInfo)
{
  NS_NOTREACHED("should only be called for text frames");
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::AttributeChanged(PRInt32         aNameSpaceID,
                          nsIAtom*        aAttribute,
                          PRInt32         aModType)
{
  return NS_OK;
}



nsSplittableType
nsFrame::GetSplittableType() const
{
  return NS_FRAME_NOT_SPLITTABLE;
}

nsIFrame* nsFrame::GetPrevContinuation() const
{
  return nsnull;
}

NS_IMETHODIMP nsFrame::SetPrevContinuation(nsIFrame* aPrevContinuation)
{
  
  if (aPrevContinuation) {
    NS_ERROR("not splittable");
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  
  return NS_OK;
}

nsIFrame* nsFrame::GetNextContinuation() const
{
  return nsnull;
}

NS_IMETHODIMP nsFrame::SetNextContinuation(nsIFrame*)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsIFrame* nsFrame::GetPrevInFlowVirtual() const
{
  return nsnull;
}

NS_IMETHODIMP nsFrame::SetPrevInFlow(nsIFrame* aPrevInFlow)
{
  
  if (aPrevInFlow) {
    NS_ERROR("not splittable");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  return NS_OK;
}

nsIFrame* nsFrame::GetNextInFlowVirtual() const
{
  return nsnull;
}

NS_IMETHODIMP nsFrame::SetNextInFlow(nsIFrame*)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsIFrame* nsIFrame::GetTailContinuation()
{
  nsIFrame* frame = this;
  while (frame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
    frame = frame->GetPrevContinuation();
    NS_ASSERTION(frame, "first continuation can't be overflow container");
  }
  for (nsIFrame* next = frame->GetNextContinuation();
       next && !(next->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER);
       next = frame->GetNextContinuation())  {
    frame = next;
  }
  NS_POSTCONDITION(frame, "illegal state in continuation chain.");
  return frame;
}

NS_DECLARE_FRAME_PROPERTY(ViewProperty, nsnull)


nsIView*
nsIFrame::GetView() const
{
  
  if (!(GetStateBits() & NS_FRAME_HAS_VIEW))
    return nsnull;

  
  void* value = Properties().Get(ViewProperty());
  NS_ASSERTION(value, "frame state bit was set but frame has no view");
  return static_cast<nsIView*>(value);
}

 nsIView*
nsIFrame::GetViewExternal() const
{
  return GetView();
}

nsresult
nsIFrame::SetView(nsIView* aView)
{
  if (aView) {
    aView->SetClientData(this);

    
    Properties().Set(ViewProperty(), aView);

    
    AddStateBits(NS_FRAME_HAS_VIEW);

    
    for (nsIFrame* f = GetParent();
         f && !(f->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW);
         f = f->GetParent())
      f->AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
  }

  return NS_OK;
}

nsIFrame* nsIFrame::GetAncestorWithViewExternal() const
{
  return GetAncestorWithView();
}


nsIFrame* nsIFrame::GetAncestorWithView() const
{
  for (nsIFrame* f = mParent; nsnull != f; f = f->GetParent()) {
    if (f->HasView()) {
      return f;
    }
  }
  return nsnull;
}


nsPoint nsIFrame::GetOffsetToExternal(const nsIFrame* aOther) const
{
  return GetOffsetTo(aOther);
}

nsPoint nsIFrame::GetOffsetTo(const nsIFrame* aOther) const
{
  NS_PRECONDITION(aOther,
                  "Must have frame for destination coordinate system!");

  NS_ASSERTION(PresContext() == aOther->PresContext(),
               "GetOffsetTo called on frames in different documents");

  nsPoint offset(0, 0);
  const nsIFrame* f;
  for (f = this; f != aOther && f; f = f->GetParent()) {
    offset += f->GetPosition();
  }

  if (f != aOther) {
    
    
    
    while (aOther) {
      offset -= aOther->GetPosition();
      aOther = aOther->GetParent();
    }
  }

  return offset;
}

nsPoint nsIFrame::GetOffsetToCrossDoc(const nsIFrame* aOther) const
{
  return GetOffsetToCrossDoc(aOther, PresContext()->AppUnitsPerDevPixel());
}

nsPoint
nsIFrame::GetOffsetToCrossDoc(const nsIFrame* aOther, const PRInt32 aAPD) const
{
  NS_PRECONDITION(aOther,
                  "Must have frame for destination coordinate system!");
  NS_ASSERTION(PresContext()->GetRootPresContext() ==
                 aOther->PresContext()->GetRootPresContext(),
               "trying to get the offset between frames in different document "
               "hierarchies?");

  const nsIFrame* root = nsnull;
  
  
  
  nsPoint offset(0, 0), docOffset(0, 0);
  const nsIFrame* f = this;
  PRInt32 currAPD = PresContext()->AppUnitsPerDevPixel();
  while (f && f != aOther) {
    docOffset += f->GetPosition();
    nsIFrame* parent = f->GetParent();
    if (parent) {
      f = parent;
    } else {
      nsPoint newOffset(0, 0);
      root = f;
      f = nsLayoutUtils::GetCrossDocParentFrame(f, &newOffset);
      PRInt32 newAPD = f ? f->PresContext()->AppUnitsPerDevPixel() : 0;
      if (!f || newAPD != currAPD) {
        
        offset += docOffset.ConvertAppUnits(currAPD, aAPD);
        docOffset.x = docOffset.y = 0;
      }
      currAPD = newAPD;
      docOffset += newOffset;
    }
  }
  if (f == aOther) {
    offset += docOffset.ConvertAppUnits(currAPD, aAPD);
  } else {
    
    
    
    
    
    nsPoint negOffset = aOther->GetOffsetToCrossDoc(root, aAPD);
    offset -= negOffset;
  }

  return offset;
}


nsIntRect nsIFrame::GetScreenRectExternal() const
{
  return GetScreenRect();
}

nsIntRect nsIFrame::GetScreenRect() const
{
  return GetScreenRectInAppUnits().ToNearestPixels(PresContext()->AppUnitsPerCSSPixel());
}


nsRect nsIFrame::GetScreenRectInAppUnitsExternal() const
{
  return GetScreenRectInAppUnits();
}

nsRect nsIFrame::GetScreenRectInAppUnits() const
{
  nsPresContext* presContext = PresContext();
  nsIFrame* rootFrame =
    presContext->PresShell()->FrameManager()->GetRootFrame();
  nsPoint rootScreenPos(0, 0);
  nsPoint rootFrameOffsetInParent(0, 0);
  nsIFrame* rootFrameParent =
    nsLayoutUtils::GetCrossDocParentFrame(rootFrame, &rootFrameOffsetInParent);
  if (rootFrameParent) {
    nsRect parentScreenRectAppUnits = rootFrameParent->GetScreenRectInAppUnits();
    nsPresContext* parentPresContext = rootFrameParent->PresContext();
    double parentScale = double(presContext->AppUnitsPerDevPixel())/
        parentPresContext->AppUnitsPerDevPixel();
    nsPoint rootPt = parentScreenRectAppUnits.TopLeft() + rootFrameOffsetInParent;
    rootScreenPos.x = NS_round(parentScale*rootPt.x);
    rootScreenPos.y = NS_round(parentScale*rootPt.y);
  } else {
    nsCOMPtr<nsIWidget> rootWidget;
    presContext->PresShell()->GetViewManager()->GetRootWidget(getter_AddRefs(rootWidget));
    if (rootWidget) {
      nsIntPoint rootDevPx = rootWidget->WidgetToScreenOffset();
      rootScreenPos.x = presContext->DevPixelsToAppUnits(rootDevPx.x);
      rootScreenPos.y = presContext->DevPixelsToAppUnits(rootDevPx.y);
    }
  }

  return nsRect(rootScreenPos + GetOffsetTo(rootFrame), GetSize());
}



NS_IMETHODIMP nsFrame::GetOffsetFromView(nsPoint&  aOffset,
                                         nsIView** aView) const
{
  NS_PRECONDITION(nsnull != aView, "null OUT parameter pointer");
  nsIFrame* frame = (nsIFrame*)this;

  *aView = nsnull;
  aOffset.MoveTo(0, 0);
  do {
    aOffset += frame->GetPosition();
    frame = frame->GetParent();
  } while (frame && !frame->HasView());
  if (frame)
    *aView = frame->GetView();
  return NS_OK;
}

 PRBool
nsIFrame::AreAncestorViewsVisible() const
{
  const nsIFrame* parent;
  for (const nsIFrame* f = this; f; f = parent) {
    nsIView* view = f->GetView();
    if (view && view->GetVisibility() == nsViewVisibility_kHide) {
      return PR_FALSE;
    }
    parent = f->GetParent();
    if (!parent) {
      parent = nsLayoutUtils::GetCrossDocParentFrame(f);
      if (parent && parent->PresContext()->IsChrome() &&
          !f->PresContext()->IsChrome()) {
        
        
        
        break;
      }
    }
  }
  return PR_TRUE;
}

nsIWidget*
nsIFrame::GetNearestWidget() const
{
  return GetClosestView()->GetNearestWidget(nsnull);
}

nsIWidget*
nsIFrame::GetNearestWidget(nsPoint& aOffset) const
{
  nsPoint offsetToView;
  nsPoint offsetToWidget;
  nsIWidget* widget =
    GetClosestView(&offsetToView)->GetNearestWidget(&offsetToWidget);
  aOffset = offsetToView + offsetToWidget;
  return widget;
}

nsIAtom*
nsFrame::GetType() const
{
  return nsnull;
}

PRBool
nsIFrame::IsLeaf() const
{
  return PR_TRUE;
}

Layer*
nsIFrame::InvalidateLayer(const nsRect& aDamageRect, PRUint32 aDisplayItemKey)
{
  NS_ASSERTION(aDisplayItemKey > 0, "Need a key");

  Layer* layer = FrameLayerBuilder::GetDedicatedLayer(this, aDisplayItemKey);
  if (!layer) {
    Invalidate(aDamageRect);
    return nsnull;
  }

  PRUint32 flags = INVALIDATE_NO_THEBES_LAYERS;
  if (aDisplayItemKey == nsDisplayItem::TYPE_VIDEO ||
      aDisplayItemKey == nsDisplayItem::TYPE_PLUGIN ||
      aDisplayItemKey == nsDisplayItem::TYPE_CANVAS) {
    flags |= INVALIDATE_NO_UPDATE_LAYER_TREE;
  }

  InvalidateWithFlags(aDamageRect, flags);
  return layer;
}

void
nsIFrame::InvalidateTransformLayer()
{
  NS_ASSERTION(mParent, "How can a viewport frame have a transform?");

  PRBool hasLayer =
      FrameLayerBuilder::GetDedicatedLayer(this, nsDisplayItem::TYPE_TRANSFORM) != nsnull;
  
  
  
  
  
  
  mParent->InvalidateInternal(GetVisualOverflowRect() + GetPosition(),
                              0, 0, this,
                              hasLayer ? INVALIDATE_NO_THEBES_LAYERS : 0);
}

class LayerActivity {
public:
  LayerActivity(nsIFrame* aFrame) : mFrame(aFrame) {}
  ~LayerActivity();
  nsExpirationState* GetExpirationState() { return &mState; }

  nsIFrame* mFrame;
  nsExpirationState mState;
};

class LayerActivityTracker : public nsExpirationTracker<LayerActivity,4> {
public:
  
  enum { GENERATION_MS = 100 };
  LayerActivityTracker()
    : nsExpirationTracker<LayerActivity,4>(GENERATION_MS) {}
  ~LayerActivityTracker() {
    AgeAllGenerations();
  }

  virtual void NotifyExpired(LayerActivity* aObject);
};

static LayerActivityTracker* gLayerActivityTracker = nsnull;

LayerActivity::~LayerActivity()
{
  if (mFrame) {
    NS_ASSERTION(gLayerActivityTracker, "Should still have a tracker");
    gLayerActivityTracker->RemoveObject(this);
  }
}

static void DestroyLayerActivity(void* aPropertyValue)
{
  delete static_cast<LayerActivity*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(LayerActivityProperty, DestroyLayerActivity)

void
LayerActivityTracker::NotifyExpired(LayerActivity* aObject)
{
  RemoveObject(aObject);

  nsIFrame* f = aObject->mFrame;
  aObject->mFrame = nsnull;
  f->Properties().Delete(LayerActivityProperty());
  f->InvalidateFrameSubtree();
}

void
nsIFrame::MarkLayersActive()
{
  FrameProperties properties = Properties();
  LayerActivity* layerActivity =
    static_cast<LayerActivity*>(properties.Get(LayerActivityProperty()));
  if (layerActivity) {
    gLayerActivityTracker->MarkUsed(layerActivity);
  } else {
    if (!gLayerActivityTracker) {
      gLayerActivityTracker = new LayerActivityTracker();
    }
    layerActivity = new LayerActivity(this);
    gLayerActivityTracker->AddObject(layerActivity);
    properties.Set(LayerActivityProperty(), layerActivity);
  }
}

PRBool
nsIFrame::AreLayersMarkedActive()
{
  return Properties().Get(LayerActivityProperty()) != nsnull;
}

 void
nsFrame::ShutdownLayerActivityTimer()
{
  delete gLayerActivityTracker;
  gLayerActivityTracker = nsnull;
}

void
nsIFrame::InvalidateWithFlags(const nsRect& aDamageRect, PRUint32 aFlags)
{
  if (aDamageRect.IsEmpty()) {
    return;
  }

  
  
  nsIPresShell *shell = PresContext()->GetPresShell();
  if (shell) {
    if (shell->IsPaintingSuppressed())
      return;
  }

  InvalidateInternal(aDamageRect, 0, 0, nsnull, aFlags);
}












void
nsIFrame::InvalidateInternalAfterResize(const nsRect& aDamageRect, nscoord aX,
                                        nscoord aY, PRUint32 aFlags)
{
  















  if ((mState & NS_FRAME_HAS_CONTAINER_LAYER) &&
      !(aFlags & INVALIDATE_NO_THEBES_LAYERS)) {
    
    
    
    
    
    
    
    FrameLayerBuilder::InvalidateThebesLayerContents(this,
        aDamageRect + nsPoint(aX, aY));
    
    aFlags |= INVALIDATE_NO_THEBES_LAYERS;
    if (aFlags & INVALIDATE_ONLY_THEBES_LAYERS) {
      return;
    }
  }
  if (IsTransformed()) {
    nsRect newDamageRect;
    newDamageRect.UnionRect(nsDisplayTransform::TransformRectOut
                            (aDamageRect, this, nsPoint(-aX, -aY)), aDamageRect);
    GetParent()->
      InvalidateInternal(newDamageRect, aX + mRect.x, aY + mRect.y, this,
                         aFlags);
  }
  else 
    GetParent()->
      InvalidateInternal(aDamageRect, aX + mRect.x, aY + mRect.y, this, aFlags);
}

void
nsIFrame::InvalidateInternal(const nsRect& aDamageRect, nscoord aX, nscoord aY,
                             nsIFrame* aForChild, PRUint32 aFlags)
{
#ifdef MOZ_SVG
  nsSVGEffects::InvalidateDirectRenderingObservers(this);
  if (nsSVGIntegrationUtils::UsingEffectsForFrame(this)) {
    nsRect r = nsSVGIntegrationUtils::GetInvalidAreaForChangedSource(this,
            aDamageRect + nsPoint(aX, aY));
    



    InvalidateInternalAfterResize(r, 0, 0, aFlags);
    return;
  }
#endif
  
  InvalidateInternalAfterResize(aDamageRect, aX, aY, aFlags);
}

gfxMatrix
nsIFrame::GetTransformMatrix(nsIFrame **aOutAncestor)
{
  NS_PRECONDITION(aOutAncestor, "Need a place to put the ancestor!");

  


  *aOutAncestor = nsLayoutUtils::GetCrossDocParentFrame(this);

  



  if (IsTransformed()) {
    


    NS_ASSERTION(*aOutAncestor, "Cannot transform the viewport frame!");
    nsPoint delta = GetOffsetToCrossDoc(*aOutAncestor);
    PRInt32 scaleFactor = PresContext()->AppUnitsPerDevPixel();

    gfxMatrix result =
      nsDisplayTransform::GetResultingTransformMatrix(this, nsPoint(0, 0),
                                                      scaleFactor);
    
    result *= gfxMatrix().Translate
      (gfxPoint(NSAppUnitsToFloatPixels(delta.x, scaleFactor),
                NSAppUnitsToFloatPixels(delta.y, scaleFactor)));
    return result;
  }
  
  







  if (!*aOutAncestor)
    return gfxMatrix();
  
  
  while (!(*aOutAncestor)->IsTransformed()) {
    
    nsIFrame* parent = nsLayoutUtils::GetCrossDocParentFrame(*aOutAncestor);
    if (!parent)
      break;

    *aOutAncestor = parent;
  }

  NS_ASSERTION(*aOutAncestor, "Somehow ended up with a null ancestor...?");

  


  nsPoint delta = GetOffsetToCrossDoc(*aOutAncestor);
  PRInt32 scaleFactor = PresContext()->AppUnitsPerDevPixel();
  return gfxMatrix().Translate
    (gfxPoint(NSAppUnitsToFloatPixels(delta.x, scaleFactor),
              NSAppUnitsToFloatPixels(delta.y, scaleFactor)));
}

void
nsIFrame::InvalidateRectDifference(const nsRect& aR1, const nsRect& aR2)
{
  nsRect sizeHStrip, sizeVStrip;
  nsLayoutUtils::GetRectDifferenceStrips(aR1, aR2, &sizeHStrip, &sizeVStrip);
  Invalidate(sizeVStrip);
  Invalidate(sizeHStrip);
}

void
nsIFrame::InvalidateFrameSubtree()
{
  Invalidate(GetVisualOverflowRectRelativeToSelf());
  FrameLayerBuilder::InvalidateThebesLayersInSubtree(this);
}

void
nsIFrame::InvalidateOverflowRect()
{
  Invalidate(GetVisualOverflowRectRelativeToSelf());
}

NS_DECLARE_FRAME_PROPERTY(DeferInvalidatesProperty, nsIFrame::DestroyRegion)

void
nsIFrame::InvalidateRoot(const nsRect& aDamageRect, PRUint32 aFlags)
{
  NS_ASSERTION(nsLayoutUtils::GetDisplayRootFrame(this) == this,
               "Can only call this on display roots");

  if ((mState & NS_FRAME_HAS_CONTAINER_LAYER) &&
      !(aFlags & INVALIDATE_NO_THEBES_LAYERS)) {
    FrameLayerBuilder::InvalidateThebesLayerContents(this, aDamageRect);
    if (aFlags & INVALIDATE_ONLY_THEBES_LAYERS) {
      return;
    }
  }

  PRUint32 flags =
    (aFlags & INVALIDATE_IMMEDIATE) ? NS_VMREFRESH_IMMEDIATE : NS_VMREFRESH_NO_SYNC;

  nsRect rect = aDamageRect;
  nsRegion* excludeRegion = static_cast<nsRegion*>
    (Properties().Get(DeferInvalidatesProperty()));
  if (excludeRegion) {
    flags = NS_VMREFRESH_DEFERRED;

    if (aFlags & INVALIDATE_EXCLUDE_CURRENT_PAINT) {
      nsRegion r;
      r.Sub(rect, *excludeRegion);
      if (r.IsEmpty())
        return;
      rect = r.GetBounds();
    }
  }

  if (!(aFlags & INVALIDATE_NO_UPDATE_LAYER_TREE)) {
    AddStateBits(NS_FRAME_UPDATE_LAYER_TREE);
  }

  nsIView* view = GetView();
  NS_ASSERTION(view, "This can only be called on frames with views");
  view->GetViewManager()->UpdateViewNoSuppression(view, rect, flags);
}

void
nsIFrame::BeginDeferringInvalidatesForDisplayRoot(const nsRegion& aExcludeRegion)
{
  NS_ASSERTION(nsLayoutUtils::GetDisplayRootFrame(this) == this,
               "Can only call this on display roots");
  Properties().Set(DeferInvalidatesProperty(), new nsRegion(aExcludeRegion));
}

void
nsIFrame::EndDeferringInvalidatesForDisplayRoot()
{
  NS_ASSERTION(nsLayoutUtils::GetDisplayRootFrame(this) == this,
               "Can only call this on display roots");
  Properties().Delete(DeferInvalidatesProperty());
}






static nsRect
ComputeOutlineAndEffectsRect(nsIFrame* aFrame, PRBool* aAnyOutlineOrEffects,
                             const nsRect& aOverflowRect,
                             const nsSize& aNewSize,
                             PRBool aStoreRectProperties) {
  nsRect r = aOverflowRect;
  *aAnyOutlineOrEffects = PR_FALSE;

  
  nsCSSShadowArray* boxShadows = aFrame->GetStyleBorder()->mBoxShadow;
  if (boxShadows) {
    nsRect shadows;
    PRInt32 A2D = aFrame->PresContext()->AppUnitsPerDevPixel();
    for (PRUint32 i = 0; i < boxShadows->Length(); ++i) {
      nsRect tmpRect(nsPoint(0, 0), aNewSize);
      nsCSSShadowItem* shadow = boxShadows->ShadowAt(i);

      
      if (shadow->mInset)
        continue;

      tmpRect.MoveBy(nsPoint(shadow->mXOffset, shadow->mYOffset));
      tmpRect.Inflate(shadow->mSpread, shadow->mSpread);
      tmpRect.Inflate(
        nsContextBoxBlur::GetBlurRadiusMargin(shadow->mRadius, A2D));

      shadows.UnionRect(shadows, tmpRect);
    }
    r.UnionRect(r, shadows);
    *aAnyOutlineOrEffects = PR_TRUE;
  }

  const nsStyleOutline* outline = aFrame->GetStyleOutline();
  PRUint8 outlineStyle = outline->GetOutlineStyle();
  if (outlineStyle != NS_STYLE_BORDER_STYLE_NONE) {
    nscoord width;
#ifdef DEBUG
    PRBool result = 
#endif
      outline->GetOutlineWidth(width);
    NS_ASSERTION(result, "GetOutlineWidth had no cached outline width");
    if (width > 0) {
      if (aStoreRectProperties) {
        aFrame->Properties().
          Set(nsIFrame::OutlineInnerRectProperty(), new nsRect(r));
      }

      nscoord offset = outline->mOutlineOffset;
      nscoord inflateBy = NS_MAX(width + offset, 0);
      
      
      
      
      
      r.Inflate(inflateBy, inflateBy);
      *aAnyOutlineOrEffects = PR_TRUE;
    }
  }
  
  
  
  
  
  
  

#ifdef MOZ_SVG
  if (nsSVGIntegrationUtils::UsingEffectsForFrame(aFrame)) {
    *aAnyOutlineOrEffects = PR_TRUE;
    if (aStoreRectProperties) {
      aFrame->Properties().
        Set(nsIFrame::PreEffectsBBoxProperty(), new nsRect(r));
    }
    r = nsSVGIntegrationUtils::ComputeFrameEffectsRect(aFrame, r);
  }
#endif

  return r;
}

nsPoint
nsIFrame::GetRelativeOffset(const nsStyleDisplay* aDisplay) const
{
  if (!aDisplay || NS_STYLE_POSITION_RELATIVE == aDisplay->mPosition) {
    nsPoint *offsets = static_cast<nsPoint*>
      (Properties().Get(ComputedOffsetProperty()));
    if (offsets) {
      return *offsets;
    }
  }
  return nsPoint(0,0);
}

nsRect
nsIFrame::GetOverflowRect(nsOverflowType aType) const
{
  NS_ABORT_IF_FALSE(aType == eVisualOverflow || aType == eScrollableOverflow,
                    "unexpected type");

  
  
  
  
  

  if (mOverflow.mType == NS_FRAME_OVERFLOW_LARGE) {
    
    
    return static_cast<nsOverflowAreas*>(const_cast<nsIFrame*>(this)->
             GetOverflowAreasProperty())->Overflow(aType);
  }

  if (aType == eVisualOverflow &&
      mOverflow.mType != NS_FRAME_OVERFLOW_NONE) {
    return GetVisualOverflowFromDeltas();
  }

  return nsRect(nsPoint(0, 0), GetSize());
}

nsOverflowAreas
nsIFrame::GetOverflowAreas() const
{
  if (mOverflow.mType == NS_FRAME_OVERFLOW_LARGE) {
    
    
    return *const_cast<nsIFrame*>(this)->GetOverflowAreasProperty();
  }

  return nsOverflowAreas(GetVisualOverflowFromDeltas(),
                         nsRect(nsPoint(0, 0), GetSize()));
}

nsRect
nsIFrame::GetScrollableOverflowRectRelativeToParent() const
{
  return GetScrollableOverflowRect() + mRect.TopLeft();
}

nsRect
nsIFrame::GetVisualOverflowRectRelativeToSelf() const
{
  if (IsTransformed()) {
    nsRect* preTransformBBox = static_cast<nsRect*>
      (Properties().Get(PreTransformBBoxProperty()));
    if (preTransformBBox)
      return *preTransformBBox;
  }
  return GetVisualOverflowRect();
}

void
nsFrame::CheckInvalidateSizeChange(nsHTMLReflowMetrics& aNewDesiredSize)
{
  nsIFrame::CheckInvalidateSizeChange(mRect, GetVisualOverflowRect(),
      nsSize(aNewDesiredSize.width, aNewDesiredSize.height));
}

static void
InvalidateRectForFrameSizeChange(nsIFrame* aFrame, const nsRect& aRect)
{
  nsStyleContext *bgSC;
  if (!nsCSSRendering::FindBackground(aFrame->PresContext(), aFrame, &bgSC)) {
    nsIFrame* rootFrame =
      aFrame->PresContext()->PresShell()->FrameManager()->GetRootFrame();
    rootFrame->Invalidate(nsRect(nsPoint(0, 0), rootFrame->GetSize()));
  }

  aFrame->Invalidate(aRect);
}

void
nsIFrame::CheckInvalidateSizeChange(const nsRect& aOldRect,
                                    const nsRect& aOldVisualOverflowRect,
                                    const nsSize& aNewDesiredSize)
{
  if (aNewDesiredSize == aOldRect.Size())
    return;

  
  
  
  
  
  
  
  
  

  
  
  
  
  

  
  PRBool anyOutlineOrEffects;
  nsRect r = ComputeOutlineAndEffectsRect(this, &anyOutlineOrEffects,
                                          aOldVisualOverflowRect,
                                          aNewDesiredSize,
                                          PR_FALSE);
  if (anyOutlineOrEffects) {
    r.UnionRect(aOldVisualOverflowRect, r);
    InvalidateRectForFrameSizeChange(this, r);
    return;
  }

  
  
  const nsStyleBorder* border = GetStyleBorder();
  NS_FOR_CSS_SIDES(side) {
    if (border->GetActualBorderWidth(side) != 0) {
      if ((side == NS_SIDE_LEFT || side == NS_SIDE_TOP) &&
          !nsLayoutUtils::HasNonZeroCornerOnSide(border->mBorderRadius, side) &&
          !border->GetBorderImage() &&
          border->GetBorderStyle(side) == NS_STYLE_BORDER_STYLE_SOLID) {
        
        
        
        
        
        continue;
      }
      InvalidateRectForFrameSizeChange(this, nsRect(0, 0, aOldRect.width, aOldRect.height));
      return;
    }
  }

  const nsStyleBackground *bg = GetStyleBackground();
  if (!bg->IsTransparent()) {
    
    
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, bg) {
      const nsStyleBackground::Layer &layer = bg->mLayers[i];
      if (layer.RenderingMightDependOnFrameSize()) {
        InvalidateRectForFrameSizeChange(this, nsRect(0, 0, aOldRect.width, aOldRect.height));
        return;
      }
    }

    
    
    
    
    
    if (nsLayoutUtils::HasNonZeroCorner(border->mBorderRadius)) {
      InvalidateRectForFrameSizeChange(this, nsRect(0, 0, aOldRect.width, aOldRect.height));
      return;
    }
  }
}




#define MAX_FRAME_DEPTH (MAX_REFLOW_DEPTH+4)

PRBool
nsFrame::IsFrameTreeTooDeep(const nsHTMLReflowState& aReflowState,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus)
{
  if (aReflowState.mReflowDepth >  MAX_FRAME_DEPTH) {
    NS_WARNING("frame tree too deep; setting zero size and returning");
    mState |= NS_FRAME_TOO_DEEP_IN_FRAME_TREE;
    ClearOverflowRects();
    aMetrics.width = 0;
    aMetrics.height = 0;
    aMetrics.ascent = 0;
    aMetrics.mCarriedOutBottomMargin.Zero();
    aMetrics.mOverflowAreas.Clear();

    if (GetNextInFlow()) {
      
      
      
      aStatus = NS_FRAME_NOT_COMPLETE;
    } else {
      aStatus = NS_FRAME_COMPLETE;
    }

    return PR_TRUE;
  }
  mState &= ~NS_FRAME_TOO_DEEP_IN_FRAME_TREE;
  return PR_FALSE;
}

 PRBool nsFrame::IsContainingBlock() const
{
  const nsStyleDisplay* display = GetStyleDisplay();

  
  
  return display->mDisplay == NS_STYLE_DISPLAY_BLOCK || 
         display->mDisplay == NS_STYLE_DISPLAY_INLINE_BLOCK || 
         display->mDisplay == NS_STYLE_DISPLAY_LIST_ITEM ||
         display->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL;
}

#ifdef NS_DEBUG

PRInt32 nsFrame::ContentIndexInContainer(const nsIFrame* aFrame)
{
  PRInt32 result = -1;

  nsIContent* content = aFrame->GetContent();
  if (content) {
    nsIContent* parentContent = content->GetParent();
    if (parentContent) {
      result = parentContent->IndexOf(content);
    }
  }

  return result;
}




void
DebugListFrameTree(nsIFrame* aFrame)
{
  ((nsFrame*)aFrame)->List(stdout, 0);
}



NS_IMETHODIMP
nsFrame::List(FILE* out, PRInt32 aIndent) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", static_cast<void*>(mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%016llx]", mState);
  }
  nsIFrame* prevInFlow = GetPrevInFlow();
  nsIFrame* nextInFlow = GetNextInFlow();
  if (nsnull != prevInFlow) {
    fprintf(out, " prev-in-flow=%p", static_cast<void*>(prevInFlow));
  }
  if (nsnull != nextInFlow) {
    fprintf(out, " next-in-flow=%p", static_cast<void*>(nextInFlow));
  }
  fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  nsFrame* f = const_cast<nsFrame*>(this);
  if (f->HasOverflowAreas()) {
    nsRect overflowArea = f->GetVisualOverflowRect();
    fprintf(out, " [vis-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
    overflowArea = f->GetScrollableOverflowRect();
    fprintf(out, " [scr-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  fprintf(out, " [sc=%p]", static_cast<void*>(mStyleContext));
  fputs("\n", out);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Frame"), aResult);
}

NS_IMETHODIMP_(nsFrameState)
nsFrame::GetDebugStateBits() const
{
  
  
  
  
  
  
  
#define IRRELEVANT_FRAME_STATE_FLAGS NS_FRAME_EXTERNAL_REFERENCE

#define FRAME_STATE_MASK (~(IRRELEVANT_FRAME_STATE_FLAGS))

  return GetStateBits() & FRAME_STATE_MASK;
}

nsresult
nsFrame::MakeFrameName(const nsAString& aType, nsAString& aResult) const
{
  aResult = aType;
  if (mContent && !mContent->IsNodeOfType(nsINode::eTEXT)) {
    nsAutoString buf;
    mContent->Tag()->ToString(buf);
    aResult.Append(NS_LITERAL_STRING("(") + buf + NS_LITERAL_STRING(")"));
  }
  char buf[40];
  PR_snprintf(buf, sizeof(buf), "(%d)", ContentIndexInContainer(this));
  AppendASCIItoUTF16(buf, aResult);
  return NS_OK;
}

void
nsFrame::XMLQuote(nsString& aString)
{
  PRInt32 i, len = aString.Length();
  for (i = 0; i < len; i++) {
    PRUnichar ch = aString.CharAt(i);
    if (ch == '<') {
      nsAutoString tmp(NS_LITERAL_STRING("&lt;"));
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 3;
      i += 3;
    }
    else if (ch == '>') {
      nsAutoString tmp(NS_LITERAL_STRING("&gt;"));
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 3;
      i += 3;
    }
    else if (ch == '\"') {
      nsAutoString tmp(NS_LITERAL_STRING("&quot;"));
      aString.Cut(i, 1);
      aString.Insert(tmp, i);
      len += 5;
      i += 5;
    }
  }
}
#endif

PRBool
nsIFrame::IsVisibleForPainting(nsDisplayListBuilder* aBuilder) {
  if (!GetStyleVisibility()->IsVisible())
    return PR_FALSE;
  nsISelection* sel = aBuilder->GetBoundingSelection();
  return !sel || IsVisibleInSelection(sel);
}

PRBool
nsIFrame::IsVisibleForPainting() {
  if (!GetStyleVisibility()->IsVisible())
    return PR_FALSE;

  nsPresContext* pc = PresContext();
  if (!pc->IsRenderingOnlySelection())
    return PR_TRUE;

  nsCOMPtr<nsISelectionController> selcon(do_QueryInterface(pc->PresShell()));
  if (selcon) {
    nsCOMPtr<nsISelection> sel;
    selcon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                         getter_AddRefs(sel));
    if (sel)
      return IsVisibleInSelection(sel);
  }
  return PR_TRUE;
}

PRBool
nsIFrame::IsVisibleInSelection(nsDisplayListBuilder* aBuilder) {
  nsISelection* sel = aBuilder->GetBoundingSelection();
  return !sel || IsVisibleInSelection(sel);
}

PRBool
nsIFrame::IsVisibleOrCollapsedForPainting(nsDisplayListBuilder* aBuilder) {
  if (!GetStyleVisibility()->IsVisibleOrCollapsed())
    return PR_FALSE;
  nsISelection* sel = aBuilder->GetBoundingSelection();
  return !sel || IsVisibleInSelection(sel);
}

PRBool
nsIFrame::IsVisibleInSelection(nsISelection* aSelection)
{
  if ((mState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT)
    return PR_TRUE;
  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  PRBool vis;
  nsresult rv = aSelection->ContainsNode(node, PR_TRUE, &vis);
  return NS_FAILED(rv) || vis;
}

 PRBool
nsFrame::IsEmpty()
{
  return PR_FALSE;
}

PRBool
nsIFrame::CachedIsEmpty()
{
  NS_PRECONDITION(!(GetStateBits() & NS_FRAME_IS_DIRTY),
                  "Must only be called on reflowed lines");
  return IsEmpty();
}

 PRBool
nsFrame::IsSelfEmpty()
{
  return PR_FALSE;
}

NS_IMETHODIMP
nsFrame::GetSelectionController(nsPresContext *aPresContext, nsISelectionController **aSelCon)
{
  if (!aPresContext || !aSelCon)
    return NS_ERROR_INVALID_ARG;

  nsIFrame *frame = this;
  while (frame && (frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION)) {
    nsITextControlFrame *tcf = do_QueryFrame(frame);
    if (tcf) {
      return tcf->GetOwnedSelectionController(aSelCon);
    }
    frame = frame->GetParent();
  }

  return CallQueryInterface(aPresContext->GetPresShell(), aSelCon);
}

already_AddRefed<nsFrameSelection>
nsIFrame::GetFrameSelection()
{
  nsFrameSelection* fs =
    const_cast<nsFrameSelection*>(GetConstFrameSelection());
  NS_IF_ADDREF(fs);
  return fs;
}

const nsFrameSelection*
nsIFrame::GetConstFrameSelection()
{
  nsIFrame *frame = this;
  while (frame && (frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION)) {
    nsITextControlFrame *tcf = do_QueryFrame(frame);
    if (tcf) {
      return tcf->GetOwnedFrameSelection();
    }
    frame = frame->GetParent();
  }

  return PresContext()->PresShell()->ConstFrameSelection();
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsFrame::DumpRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent)
{
  IndentBy(out, aIndent);
  fprintf(out, "<frame va=\"%ld\" type=\"", PRUptrdiff(this));
  nsAutoString name;
  GetFrameName(name);
  XMLQuote(name);
  fputs(NS_LossyConvertUTF16toASCII(name).get(), out);
  fprintf(out, "\" state=\"%016llx\" parent=\"%ld\">\n",
          GetDebugStateBits(), PRUptrdiff(mParent));

  aIndent++;
  DumpBaseRegressionData(aPresContext, out, aIndent);
  aIndent--;

  IndentBy(out, aIndent);
  fprintf(out, "</frame>\n");

  return NS_OK;
}

void
nsFrame::DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent)
{
  if (GetNextSibling()) {
    IndentBy(out, aIndent);
    fprintf(out, "<next-sibling va=\"%ld\"/>\n", PRUptrdiff(GetNextSibling()));
  }

  if (HasView()) {
    IndentBy(out, aIndent);
    fprintf(out, "<view va=\"%ld\">\n", PRUptrdiff(GetView()));
    aIndent++;
    
    aIndent--;
    IndentBy(out, aIndent);
    fprintf(out, "</view>\n");
  }

  IndentBy(out, aIndent);
  fprintf(out, "<bbox x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\"/>\n",
          mRect.x, mRect.y, mRect.width, mRect.height);

  
  nsIFrame* kid;
  nsIAtom* list = nsnull;
  PRInt32 listIndex = 0;
  do {
    kid = GetFirstChild(list);
    if (kid) {
      IndentBy(out, aIndent);
      if (nsnull != list) {
        nsAutoString listName;
        list->ToString(listName);
        fprintf(out, "<child-list name=\"");
        XMLQuote(listName);
        fputs(NS_LossyConvertUTF16toASCII(listName).get(), out);
        fprintf(out, "\">\n");
      }
      else {
        fprintf(out, "<child-list>\n");
      }
      aIndent++;
      while (kid) {
        kid->DumpRegressionData(aPresContext, out, aIndent);
        kid = kid->GetNextSibling();
      }
      aIndent--;
      IndentBy(out, aIndent);
      fprintf(out, "</child-list>\n");
    }
    list = GetAdditionalChildListName(listIndex++);
  } while (nsnull != list);
}
#endif

void
nsIFrame::SetSelected(PRBool aSelected, SelectionType aType)
{
  NS_ASSERTION(!GetPrevContinuation(),
               "Should only be called on first in flow");
  if (aType != nsISelectionController::SELECTION_NORMAL)
    return;

  
  PRBool selectable;
  IsSelectable(&selectable, nsnull);
  if (!selectable)
    return;

  for (nsIFrame* f = this; f; f = f->GetNextContinuation()) {
    if (aSelected) {
      AddStateBits(NS_FRAME_SELECTED_CONTENT);
    } else {
      RemoveStateBits(NS_FRAME_SELECTED_CONTENT);
    }

    
    InvalidateFrameSubtree();
  }
}

NS_IMETHODIMP
nsFrame::GetSelected(PRBool *aSelected) const
{
  if (!aSelected )
    return NS_ERROR_NULL_POINTER;
  *aSelected = !!(mState & NS_FRAME_SELECTED_CONTENT);
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetPointFromOffset(PRInt32 inOffset, nsPoint* outPoint)
{
  NS_PRECONDITION(outPoint != nsnull, "Null parameter");
  nsRect contentRect = GetContentRect() - GetPosition();
  nsPoint pt = contentRect.TopLeft();
  if (mContent)
  {
    nsIContent* newContent = mContent->GetParent();
    if (newContent){
      PRInt32 newOffset = newContent->IndexOf(mContent);

      PRBool isRTL = (NS_GET_EMBEDDING_LEVEL(this) & 1) == 1;
      if ((!isRTL && inOffset > newOffset) ||
          (isRTL && inOffset <= newOffset)) {
        pt = contentRect.TopRight();
      }
    }
  }
  *outPoint = pt;
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetChildFrameContainingOffset(PRInt32 inContentOffset, PRBool inHint, PRInt32* outFrameContentOffset, nsIFrame **outChildFrame)
{
  NS_PRECONDITION(outChildFrame && outFrameContentOffset, "Null parameter");
  *outFrameContentOffset = (PRInt32)inHint;
  
  
  nsRect rect = GetRect();
  if (!rect.width || !rect.height)
  {
    
    
    nsIFrame* nextFlow = GetNextInFlow();
    if (nextFlow)
      return nextFlow->GetChildFrameContainingOffset(inContentOffset, inHint, outFrameContentOffset, outChildFrame);
  }
  *outChildFrame = this;
  return NS_OK;
}









nsresult
nsFrame::GetNextPrevLineFromeBlockFrame(nsPresContext* aPresContext,
                                        nsPeekOffsetStruct *aPos,
                                        nsIFrame *aBlockFrame, 
                                        PRInt32 aLineStart, 
                                        PRInt8 aOutSideLimit
                                        )
{
  
  if (!aBlockFrame || !aPos)
    return NS_ERROR_NULL_POINTER;

  aPos->mResultFrame = nsnull;
  aPos->mResultContent = nsnull;
  aPos->mAttachForward = (aPos->mDirection == eDirNext);

  nsAutoLineIterator it = aBlockFrame->GetLineIterator();
  if (!it)
    return NS_ERROR_FAILURE;
  PRInt32 searchingLine = aLineStart;
  PRInt32 countLines = it->GetNumLines();
  if (aOutSideLimit > 0) 
    searchingLine = countLines;
  else if (aOutSideLimit <0)
    searchingLine = -1;
  else 
    if ((aPos->mDirection == eDirPrevious && searchingLine == 0) || 
       (aPos->mDirection == eDirNext && searchingLine >= (countLines -1) )){
      
           return NS_ERROR_FAILURE;
    }
  PRInt32 lineFrameCount;
  nsIFrame *resultFrame = nsnull;
  nsIFrame *farStoppingFrame = nsnull; 
  nsIFrame *nearStoppingFrame = nsnull; 
  nsIFrame *firstFrame;
  nsIFrame *lastFrame;
  nsRect  rect;
  PRBool isBeforeFirstFrame, isAfterLastFrame;
  PRBool found = PR_FALSE;

  nsresult result = NS_OK;
  while (!found)
  {
    if (aPos->mDirection == eDirPrevious)
      searchingLine --;
    else
      searchingLine ++;
    if ((aPos->mDirection == eDirPrevious && searchingLine < 0) || 
       (aPos->mDirection == eDirNext && searchingLine >= countLines ))
    {
      
      return NS_ERROR_FAILURE;
    }
    PRUint32 lineFlags;
    result = it->GetLine(searchingLine, &firstFrame, &lineFrameCount,
                         rect, &lineFlags);
    if (!lineFrameCount) 
      continue;
    if (NS_SUCCEEDED(result)){
      lastFrame = firstFrame;
      for (;lineFrameCount > 1;lineFrameCount --){
        
        result = it->GetNextSiblingOnLine(lastFrame, searchingLine);
        if (NS_FAILED(result) || !lastFrame){
          NS_ERROR("GetLine promised more frames than could be found");
          return NS_ERROR_FAILURE;
        }
      }
      GetLastLeaf(aPresContext, &lastFrame);

      if (aPos->mDirection == eDirNext){
        nearStoppingFrame = firstFrame;
        farStoppingFrame = lastFrame;
      }
      else{
        nearStoppingFrame = lastFrame;
        farStoppingFrame = firstFrame;
      }
      nsPoint offset;
      nsIView * view; 
      aBlockFrame->GetOffsetFromView(offset,&view);
      nscoord newDesiredX  = aPos->mDesiredX - offset.x;
      result = it->FindFrameAt(searchingLine, newDesiredX, &resultFrame, &isBeforeFirstFrame, &isAfterLastFrame);
      if(NS_FAILED(result))
        continue;
    }

    if (NS_SUCCEEDED(result) && resultFrame)
    {
      
      nsAutoLineIterator newIt = resultFrame->GetLineIterator();
      if (newIt)
      {
        aPos->mResultFrame = resultFrame;
        return NS_OK;
      }
      
      result = NS_ERROR_FAILURE;

      nsCOMPtr<nsIFrameEnumerator> frameTraversal;
      result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                    aPresContext, resultFrame,
                                    ePostOrder,
                                    PR_FALSE, 
                                    aPos->mScrollViewStop,
                                    PR_FALSE  
                                    );
      if (NS_FAILED(result))
        return result;
      nsIFrame *storeOldResultFrame = resultFrame;
      while ( !found ){
        nsPoint point;
        point.x = aPos->mDesiredX;

        nsRect tempRect = resultFrame->GetRect();
        nsPoint offset;
        nsIView * view; 
        result = resultFrame->GetOffsetFromView(offset, &view);
        if (NS_FAILED(result))
          return result;
        point.y = tempRect.height + offset.y;

        
        
        nsIPresShell *shell = aPresContext->GetPresShell();
        if (!shell)
          return NS_ERROR_FAILURE;
        PRInt16 isEditor = shell->GetSelectionFlags();
        isEditor = isEditor == nsISelectionDisplay::DISPLAY_ALL;
        if ( isEditor )
        {
          if (resultFrame->GetType() == nsGkAtoms::tableOuterFrame)
          {
            if (((point.x - offset.x + tempRect.x)<0) ||  ((point.x - offset.x+ tempRect.x)>tempRect.width))
            {
              nsIContent* content = resultFrame->GetContent();
              if (content)
              {
                nsIContent* parent = content->GetParent();
                if (parent)
                {
                  aPos->mResultContent = parent;
                  aPos->mContentOffset = parent->IndexOf(content);
                  aPos->mAttachForward = PR_FALSE;
                  if ((point.x - offset.x+ tempRect.x)>tempRect.width)
                  {
                    aPos->mContentOffset++;
                    aPos->mAttachForward = PR_TRUE;
                  }
                  
                  aPos->mResultFrame = resultFrame->GetParent();
                  return NS_POSITION_BEFORE_TABLE;
                }
              }
            }
          }
        }

        if (!resultFrame->HasView())
        {
          nsIView* view;
          nsPoint offset;
          resultFrame->GetOffsetFromView(offset, &view);
          ContentOffsets offsets =
              resultFrame->GetContentOffsetsFromPoint(point - offset);
          aPos->mResultContent = offsets.content;
          aPos->mContentOffset = offsets.offset;
          aPos->mAttachForward = offsets.associateWithNext;
          if (offsets.content)
          {
            PRBool selectable;
            resultFrame->IsSelectable(&selectable, nsnull);
            if (selectable)
            {
              found = PR_TRUE;
              break;
            }
          }
        }

        if (aPos->mDirection == eDirPrevious && (resultFrame == farStoppingFrame))
          break;
        if (aPos->mDirection == eDirNext && (resultFrame == nearStoppingFrame))
          break;
        
        frameTraversal->Prev();
        resultFrame = frameTraversal->CurrentItem();
        if (!resultFrame)
          return NS_ERROR_FAILURE;
      }

      if (!found){
        resultFrame = storeOldResultFrame;
        result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                      aPresContext, resultFrame,
                                      eLeaf,
                                      PR_FALSE, 
                                      aPos->mScrollViewStop,
                                      PR_FALSE  
                                      );
      }
      while ( !found ){
        nsPoint point(aPos->mDesiredX, 0);
        nsIView* view;
        nsPoint offset;
        resultFrame->GetOffsetFromView(offset, &view);
        ContentOffsets offsets =
            resultFrame->GetContentOffsetsFromPoint(point - offset);
        aPos->mResultContent = offsets.content;
        aPos->mContentOffset = offsets.offset;
        aPos->mAttachForward = offsets.associateWithNext;
        if (offsets.content)
        {
          PRBool selectable;
          resultFrame->IsSelectable(&selectable, nsnull);
          if (selectable)
          {
            found = PR_TRUE;
            if (resultFrame == farStoppingFrame)
              aPos->mAttachForward = PR_FALSE;
            else
              aPos->mAttachForward = PR_TRUE;
            break;
          }
        }
        if (aPos->mDirection == eDirPrevious && (resultFrame == nearStoppingFrame))
          break;
        if (aPos->mDirection == eDirNext && (resultFrame == farStoppingFrame))
          break;
        
        frameTraversal->Next();
        nsIFrame *tempFrame = frameTraversal->CurrentItem();
        if (!tempFrame)
          break;
        resultFrame = tempFrame;
      }
      aPos->mResultFrame = resultFrame;
    }
    else {
        
      aPos->mAmount = eSelectLine;
      aPos->mStartOffset = 0;
      aPos->mAttachForward = !(aPos->mDirection == eDirNext);
      if (aPos->mDirection == eDirPrevious)
        aPos->mStartOffset = -1;
     return aBlockFrame->PeekOffset(aPos);
    }
  }
  return NS_OK;
}

nsPeekOffsetStruct nsIFrame::GetExtremeCaretPosition(PRBool aStart)
{
  nsPeekOffsetStruct result;

  FrameTarget targetFrame = DrillDownToSelectionFrame(this, !aStart);
  FrameContentRange range = GetRangeForFrame(targetFrame.frame);
  result.mResultContent = range.content;
  result.mContentOffset = aStart ? range.start : range.end;
  result.mAttachForward = (result.mContentOffset == range.start);
  return result;
}



static nsContentAndOffset
FindBlockFrameOrBR(nsIFrame* aFrame, nsDirection aDirection)
{
  nsContentAndOffset result;
  result.mContent =  nsnull;
  result.mOffset = 0;

  if (aFrame->IsGeneratedContentFrame())
    return result;

  
  
  nsIFormControlFrame* fcf = do_QueryFrame(aFrame);
  if (fcf)
    return result;
  
  
  
  
  
  if ((nsLayoutUtils::GetAsBlock(aFrame) && !(aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL)) ||
      aFrame->GetType() == nsGkAtoms::brFrame) {
    nsIContent* content = aFrame->GetContent();
    result.mContent = content->GetParent();
    
    
    
    NS_ASSERTION(result.mContent, "Unexpected orphan content");
    if (result.mContent)
      result.mOffset = result.mContent->IndexOf(content) + 
        (aDirection == eDirPrevious ? 1 : 0);
    return result;
  }

  
  if (aFrame->HasTerminalNewline() &&
      aFrame->GetStyleContext()->GetStyleText()->NewlineIsSignificant()) {
    PRInt32 startOffset, endOffset;
    aFrame->GetOffsets(startOffset, endOffset);
    result.mContent = aFrame->GetContent();
    result.mOffset = endOffset - (aDirection == eDirPrevious ? 0 : 1);
    return result;
  }

  
  if (aDirection == eDirPrevious) {
    nsIFrame* child = aFrame->GetChildList(nsnull).LastChild();
    while(child && !result.mContent) {
      result = FindBlockFrameOrBR(child, aDirection);
      child = child->GetPrevSibling();
    }
  } else { 
    nsIFrame* child = aFrame->GetFirstChild(nsnull);
    while(child && !result.mContent) {
      result = FindBlockFrameOrBR(child, aDirection);
      child = child->GetNextSibling();
    }
  }
  return result;
}

nsresult
nsIFrame::PeekOffsetParagraph(nsPeekOffsetStruct *aPos)
{
  nsIFrame* frame = this;
  nsContentAndOffset blockFrameOrBR;
  blockFrameOrBR.mContent = nsnull;
  PRBool reachedBlockAncestor = PR_FALSE;

  
  
  
  
  
  if (aPos->mDirection == eDirPrevious) {
    while (!reachedBlockAncestor) {
      nsIFrame* parent = frame->GetParent();
      
      if (!frame->mContent || !frame->mContent->GetParent()) {
        reachedBlockAncestor = PR_TRUE;
        break;
      }
      nsIFrame* sibling = frame->GetPrevSibling();
      while (sibling && !blockFrameOrBR.mContent) {
        blockFrameOrBR = FindBlockFrameOrBR(sibling, eDirPrevious);
        sibling = sibling->GetPrevSibling();
      }
      if (blockFrameOrBR.mContent) {
        aPos->mResultContent = blockFrameOrBR.mContent;
        aPos->mContentOffset = blockFrameOrBR.mOffset;
        break;
      }
      frame = parent;
      reachedBlockAncestor = (nsLayoutUtils::GetAsBlock(frame) != nsnull);
    }
    if (reachedBlockAncestor) { 
      aPos->mResultContent = frame->GetContent();
      aPos->mContentOffset = 0;
    }
  } else { 
    while (!reachedBlockAncestor) {
      nsIFrame* parent = frame->GetParent();
      
      if (!frame->mContent || !frame->mContent->GetParent()) {
        reachedBlockAncestor = PR_TRUE;
        break;
      }
      nsIFrame* sibling = frame;
      while (sibling && !blockFrameOrBR.mContent) {
        blockFrameOrBR = FindBlockFrameOrBR(sibling, eDirNext);
        sibling = sibling->GetNextSibling();
      }
      if (blockFrameOrBR.mContent) {
        aPos->mResultContent = blockFrameOrBR.mContent;
        aPos->mContentOffset = blockFrameOrBR.mOffset;
        break;
      }
      frame = parent;
      reachedBlockAncestor = (nsLayoutUtils::GetAsBlock(frame) != nsnull);
    }
    if (reachedBlockAncestor) { 
      aPos->mResultContent = frame->GetContent();
      if (aPos->mResultContent)
        aPos->mContentOffset = aPos->mResultContent->GetChildCount();
    }
  }
  return NS_OK;
}


static PRBool IsMovingInFrameDirection(nsIFrame* frame, nsDirection aDirection, PRBool aVisual)
{
  PRBool isReverseDirection = aVisual ?
    (NS_GET_EMBEDDING_LEVEL(frame) & 1) != (NS_GET_BASE_LEVEL(frame) & 1) : PR_FALSE;
  return aDirection == (isReverseDirection ? eDirPrevious : eDirNext);
}

NS_IMETHODIMP
nsIFrame::PeekOffset(nsPeekOffsetStruct* aPos)
{
  if (!aPos)
    return NS_ERROR_NULL_POINTER;
  nsresult result = NS_ERROR_FAILURE;

  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;

  
  FrameContentRange range = GetRangeForFrame(this);
  PRInt32 offset = aPos->mStartOffset - range.start;
  nsIFrame* current = this;
  
  switch (aPos->mAmount) {
    case eSelectCharacter:
    case eSelectCluster:
    {
      PRBool eatingNonRenderableWS = PR_FALSE;
      PRBool done = PR_FALSE;
      PRBool jumpedLine = PR_FALSE;
      
      while (!done) {
        PRBool movingInFrameDirection =
          IsMovingInFrameDirection(current, aPos->mDirection, aPos->mVisual);

        if (eatingNonRenderableWS)
          done = current->PeekOffsetNoAmount(movingInFrameDirection, &offset); 
        else
          done = current->PeekOffsetCharacter(movingInFrameDirection, &offset,
                                              aPos->mAmount == eSelectCluster);

        if (!done) {
          result =
            current->GetFrameFromDirection(aPos->mDirection, aPos->mVisual,
                                           aPos->mJumpLines, aPos->mScrollViewStop,
                                           &current, &offset, &jumpedLine);
          if (NS_FAILED(result))
            return result;

          
          
          if (jumpedLine)
            eatingNonRenderableWS = PR_TRUE;
        }
      }

      
      range = GetRangeForFrame(current);
      aPos->mResultFrame = current;
      aPos->mResultContent = range.content;
      
      aPos->mContentOffset = offset < 0 ? range.end : range.start + offset;
      
      
      
      if (offset < 0 && jumpedLine &&
          aPos->mDirection == eDirPrevious &&
          current->GetStyleText()->NewlineIsSignificant() &&
          current->HasTerminalNewline()) {
        --aPos->mContentOffset;
      }
      
      break;
    }
    case eSelectWordNoSpace:
      
      
      
      
      if (aPos->mDirection == eDirPrevious) {
        aPos->mWordMovementType = eStartWord;
      } else {
        aPos->mWordMovementType = eEndWord;
      }
      
    case eSelectWord:
    {
      
      
      
      
      PRBool wordSelectEatSpace;
      if (aPos->mWordMovementType != eDefaultBehavior) {
        
        
        
        wordSelectEatSpace = ((aPos->mWordMovementType == eEndWord) == (aPos->mDirection == eDirPrevious));
      }
      else {
        
        
        
        wordSelectEatSpace = aPos->mDirection == eDirNext &&
          nsContentUtils::GetBoolPref("layout.word_select.eat_space_to_next_word");
      }
      
      
      
      
      
      
      
      
      
      PeekWordState state;
      PRInt32 offsetAdjustment = 0;
      PRBool done = PR_FALSE;
      while (!done) {
        PRBool movingInFrameDirection =
          IsMovingInFrameDirection(current, aPos->mDirection, aPos->mVisual);
        
        done = current->PeekOffsetWord(movingInFrameDirection, wordSelectEatSpace,
                                       aPos->mIsKeyboardSelect, &offset, &state);
        
        if (!done) {
          nsIFrame* nextFrame;
          PRInt32 nextFrameOffset;
          PRBool jumpedLine;
          result =
            current->GetFrameFromDirection(aPos->mDirection, aPos->mVisual,
                                           aPos->mJumpLines, aPos->mScrollViewStop,
                                           &nextFrame, &nextFrameOffset, &jumpedLine);
          
          
          if (NS_FAILED(result) ||
              (jumpedLine && !wordSelectEatSpace && state.mSawBeforeType)) {
            done = PR_TRUE;
            
            
            if (jumpedLine && wordSelectEatSpace &&
                current->HasTerminalNewline() &&
                current->GetStyleText()->NewlineIsSignificant()) {
              offsetAdjustment = -1;
            }
          } else {
            if (jumpedLine) {
              state.mContext.Truncate();
            }
            current = nextFrame;
            offset = nextFrameOffset;
            
            if (wordSelectEatSpace && jumpedLine)
              state.SetSawBeforeType();
          }
        }
      }
      
      
      range = GetRangeForFrame(current);
      aPos->mResultFrame = current;
      aPos->mResultContent = range.content;
      
      aPos->mContentOffset = (offset < 0 ? range.end : range.start + offset) + offsetAdjustment;
      break;
    }
    case eSelectLine :
    {
      nsAutoLineIterator iter;
      nsIFrame *blockFrame = this;

      while (NS_FAILED(result)){
        PRInt32 thisLine = nsFrame::GetLineNumber(blockFrame, aPos->mScrollViewStop, &blockFrame);
        if (thisLine < 0) 
          return  NS_ERROR_FAILURE;
        iter = blockFrame->GetLineIterator();
        NS_ASSERTION(iter, "GetLineNumber() succeeded but no block frame?");
        result = NS_OK;

        int edgeCase = 0;
        
        PRBool doneLooping = PR_FALSE;
        
        
        nsIFrame *lastFrame = this;
        do {
          result = nsFrame::GetNextPrevLineFromeBlockFrame(PresContext(),
                                                           aPos, 
                                                           blockFrame, 
                                                           thisLine, 
                                                           edgeCase 
            );
          if (NS_SUCCEEDED(result) && (!aPos->mResultFrame || aPos->mResultFrame == lastFrame))
          {
            aPos->mResultFrame = nsnull;
            if (aPos->mDirection == eDirPrevious)
              thisLine--;
            else
              thisLine++;
          }
          else 
            doneLooping = PR_TRUE; 

          lastFrame = aPos->mResultFrame; 

          if (NS_SUCCEEDED(result) && aPos->mResultFrame 
            && blockFrame != aPos->mResultFrame)
          {





            PRBool searchTableBool = PR_FALSE;
            if (aPos->mResultFrame->GetType() == nsGkAtoms::tableOuterFrame ||
                aPos->mResultFrame->GetType() == nsGkAtoms::tableCellFrame)
            {
              nsIFrame *frame = aPos->mResultFrame->GetFirstChild(nsnull);
              
              while(frame) 
              {
                iter = frame->GetLineIterator();
                if (iter)
                {
                  aPos->mResultFrame = frame;
                  searchTableBool = PR_TRUE;
                  result = NS_OK;
                  break; 
                }
                result = NS_ERROR_FAILURE;
                frame = frame->GetFirstChild(nsnull);
              }
            }

            if (!searchTableBool) {
              iter = aPos->mResultFrame->GetLineIterator();
              result = iter ? NS_OK : NS_ERROR_FAILURE;
            }
            if (NS_SUCCEEDED(result) && iter)
            {
              doneLooping = PR_FALSE;
              if (aPos->mDirection == eDirPrevious)
                edgeCase = 1;
              else
                edgeCase = -1;
              thisLine=0;
              
              blockFrame = aPos->mResultFrame;

            }
            else
            {
              result = NS_OK;
              break;
            }
          }
        } while (!doneLooping);
      }
      return result;
    }

    case eSelectParagraph:
      return PeekOffsetParagraph(aPos);

    case eSelectBeginLine:
    case eSelectEndLine:
    {
      
      nsIFrame* blockFrame = AdjustFrameForSelectionStyles(this);
      PRInt32 thisLine = nsFrame::GetLineNumber(blockFrame, aPos->mScrollViewStop, &blockFrame);
      if (thisLine < 0)
        return NS_ERROR_FAILURE;
      nsAutoLineIterator it = blockFrame->GetLineIterator();
      NS_ASSERTION(it, "GetLineNumber() succeeded but no block frame?");

      PRInt32 lineFrameCount;
      nsIFrame *firstFrame;
      nsRect usedRect;
      PRUint32 lineFlags;
      nsIFrame* baseFrame = nsnull;
      PRBool endOfLine = (eSelectEndLine == aPos->mAmount);
      
#ifdef IBMBIDI
      if (aPos->mVisual && PresContext()->BidiEnabled()) {
        PRBool lineIsRTL = it->GetDirection();
        PRBool isReordered;
        nsIFrame *lastFrame;
        result = it->CheckLineOrder(thisLine, &isReordered, &firstFrame, &lastFrame);
        baseFrame = endOfLine ? lastFrame : firstFrame;
        if (baseFrame) {
          nsBidiLevel embeddingLevel = nsBidiPresUtils::GetFrameEmbeddingLevel(baseFrame);
          
          
          if ((embeddingLevel & 1) == !lineIsRTL)
            endOfLine = !endOfLine;
        }
      } else
#endif
      {
        it->GetLine(thisLine, &firstFrame, &lineFrameCount, usedRect, &lineFlags);

        nsIFrame* frame = firstFrame;
        for (PRInt32 count = lineFrameCount; count;
             --count, frame = frame->GetNextSibling()) {
          if (!frame->IsGeneratedContentFrame()) {
            baseFrame = frame;
            if (!endOfLine)
              break;
          }
        }
      }
      if (!baseFrame)
        return NS_ERROR_FAILURE;
      FrameTarget targetFrame = DrillDownToSelectionFrame(baseFrame,
                                                          endOfLine);
      FrameContentRange range = GetRangeForFrame(targetFrame.frame);
      aPos->mResultContent = range.content;
      aPos->mContentOffset = endOfLine ? range.end : range.start;
      if (endOfLine && targetFrame.frame->HasTerminalNewline()) {
        
        
        --aPos->mContentOffset;
      }
      aPos->mResultFrame = targetFrame.frame;
      aPos->mAttachForward = (aPos->mContentOffset == range.start);
      if (!range.content)
        return NS_ERROR_FAILURE;
      return NS_OK;
    }

    default: 
    {
      NS_ASSERTION(PR_FALSE, "Invalid amount");
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

PRBool
nsFrame::PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return PR_TRUE;
}

PRBool
nsFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset,
                             PRBool aRespectClusters)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  PRInt32 startOffset = *aOffset;
  
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    *aOffset = 1 - startOffset;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsFrame::PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                        PRInt32* aOffset, PeekWordState* aState)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  PRInt32 startOffset = *aOffset;
  
  aState->mContext.Truncate();
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    if (!aState->mAtStart) {
      if (aState->mLastCharWasPunctuation) {
        
        if (BreakWordBetweenPunctuation(aState, aForward, PR_FALSE, PR_FALSE, aIsKeyboardSelect))
          return PR_TRUE;
      } else {
        
        if (aWordSelectEatSpace && aState->mSawBeforeType)
          return PR_TRUE;
      }
    }
    
    *aOffset = 1 - startOffset;
    aState->Update(PR_FALSE, 
                   PR_FALSE  
                   );
    if (!aWordSelectEatSpace)
      aState->SetSawBeforeType();
  }
  return PR_FALSE;
}

PRBool
nsFrame::BreakWordBetweenPunctuation(const PeekWordState* aState,
                                     PRBool aForward,
                                     PRBool aPunctAfter, PRBool aWhitespaceAfter,
                                     PRBool aIsKeyboardSelect)
{
  NS_ASSERTION(aPunctAfter != aState->mLastCharWasPunctuation,
               "Call this only at punctuation boundaries");
  if (aState->mLastCharWasWhitespace) {
    
    return PR_TRUE;
  }
  if (!nsContentUtils::GetBoolPref("layout.word_select.stop_at_punctuation")) {
    
    
    return PR_FALSE;
  }
  if (!aIsKeyboardSelect) {
    
    return PR_TRUE;
  }
  PRBool afterPunct = aForward ? aState->mLastCharWasPunctuation : aPunctAfter;
  if (!afterPunct) {
    
    return PR_FALSE;
  }
  
  
  return aState->mSeenNonPunctuationSinceWhitespace;
}

NS_IMETHODIMP
nsFrame::CheckVisibility(nsPresContext* , PRInt32 , PRInt32 , PRBool , PRBool *, PRBool *)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


PRInt32
nsFrame::GetLineNumber(nsIFrame *aFrame, PRBool aLockScroll, nsIFrame** aContainingBlock)
{
  NS_ASSERTION(aFrame, "null aFrame");
  nsFrameManager* frameManager = aFrame->PresContext()->FrameManager();
  nsIFrame *blockFrame = aFrame;
  nsIFrame *thisBlock;
  nsAutoLineIterator it;
  nsresult result = NS_ERROR_FAILURE;
  while (NS_FAILED(result) && blockFrame)
  {
    thisBlock = blockFrame;
    if (thisBlock->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
      
      
      if (thisBlock->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
        
        thisBlock = thisBlock->GetFirstInFlow();
      }
      thisBlock = frameManager->GetPlaceholderFrameFor(thisBlock);
      if (!thisBlock)
        return -1;
    }  
    blockFrame = thisBlock->GetParent();
    result = NS_OK;
    if (blockFrame) {
      if (aLockScroll && blockFrame->GetType() == nsGkAtoms::scrollFrame)
        return -1;
      it = blockFrame->GetLineIterator();
      if (!it)
        result = NS_ERROR_FAILURE;
    }
  }
  if (!blockFrame || !it)
    return -1;

  if (aContainingBlock)
    *aContainingBlock = blockFrame;
  return it->FindLineContaining(thisBlock);
}

nsresult
nsIFrame::GetFrameFromDirection(nsDirection aDirection, PRBool aVisual,
                                PRBool aJumpLines, PRBool aScrollViewStop, 
                                nsIFrame** aOutFrame, PRInt32* aOutOffset, PRBool* aOutJumpedLine)
{
  nsresult result;

  if (!aOutFrame || !aOutOffset || !aOutJumpedLine)
    return NS_ERROR_NULL_POINTER;
  
  nsPresContext* presContext = PresContext();
  *aOutFrame = nsnull;
  *aOutOffset = 0;
  *aOutJumpedLine = PR_FALSE;

  
  PRBool selectable = PR_FALSE;
  nsIFrame *traversedFrame = this;
  while (!selectable) {
    nsIFrame *blockFrame;
    
    PRInt32 thisLine = nsFrame::GetLineNumber(traversedFrame, aScrollViewStop, &blockFrame);
    if (thisLine < 0)
      return NS_ERROR_FAILURE;

    nsAutoLineIterator it = blockFrame->GetLineIterator();
    NS_ASSERTION(it, "GetLineNumber() succeeded but no block frame?");

    PRBool atLineEdge;
    nsIFrame *firstFrame;
    nsIFrame *lastFrame;
#ifdef IBMBIDI
    if (aVisual && presContext->BidiEnabled()) {
      PRBool lineIsRTL = it->GetDirection();
      PRBool isReordered;
      result = it->CheckLineOrder(thisLine, &isReordered, &firstFrame, &lastFrame);
      nsIFrame** framePtr = aDirection == eDirPrevious ? &firstFrame : &lastFrame;
      if (*framePtr) {
        nsBidiLevel embeddingLevel = nsBidiPresUtils::GetFrameEmbeddingLevel(*framePtr);
        if ((((embeddingLevel & 1) && lineIsRTL) || (!(embeddingLevel & 1) && !lineIsRTL)) ==
            (aDirection == eDirPrevious)) {
          nsFrame::GetFirstLeaf(presContext, framePtr);
        } else {
          nsFrame::GetLastLeaf(presContext, framePtr);
        }
        atLineEdge = *framePtr == traversedFrame;
      } else {
        atLineEdge = PR_TRUE;
      }
    } else
#endif
    {
      nsRect  nonUsedRect;
      PRInt32 lineFrameCount;
      PRUint32 lineFlags;
      result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,nonUsedRect,
                           &lineFlags);
      if (NS_FAILED(result))
        return result;

      if (aDirection == eDirPrevious) {
        nsFrame::GetFirstLeaf(presContext, &firstFrame);
        atLineEdge = firstFrame == traversedFrame;
      } else { 
        lastFrame = firstFrame;
        for (;lineFrameCount > 1;lineFrameCount --){
          result = it->GetNextSiblingOnLine(lastFrame, thisLine);
          if (NS_FAILED(result) || !lastFrame){
            NS_ERROR("should not be reached nsFrame");
            return NS_ERROR_FAILURE;
          }
        }
        nsFrame::GetLastLeaf(presContext, &lastFrame);
        atLineEdge = lastFrame == traversedFrame;
      }
    }

    if (atLineEdge) {
      *aOutJumpedLine = PR_TRUE;
      if (!aJumpLines)
        return NS_ERROR_FAILURE; 
    }

    nsCOMPtr<nsIFrameEnumerator> frameTraversal;
    result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                  presContext, traversedFrame,
                                  eLeaf,
                                  aVisual && presContext->BidiEnabled(),
                                  aScrollViewStop,
                                  PR_TRUE  
                                  );
    if (NS_FAILED(result))
      return result;

    if (aDirection == eDirNext)
      frameTraversal->Next();
    else
      frameTraversal->Prev();

    traversedFrame = frameTraversal->CurrentItem();
    if (!traversedFrame)
      return NS_ERROR_FAILURE;
    traversedFrame->IsSelectable(&selectable, nsnull);
  } 

  *aOutOffset = (aDirection == eDirNext) ? 0 : -1;

#ifdef IBMBIDI
  if (aVisual) {
    PRUint8 newLevel = NS_GET_EMBEDDING_LEVEL(traversedFrame);
    PRUint8 newBaseLevel = NS_GET_BASE_LEVEL(traversedFrame);
    if ((newLevel & 1) != (newBaseLevel & 1)) 
      *aOutOffset = -1 - *aOutOffset;
  }
#endif
  *aOutFrame = traversedFrame;
  return NS_OK;
}

nsIView* nsIFrame::GetClosestView(nsPoint* aOffset) const
{
  nsPoint offset(0,0);
  for (const nsIFrame *f = this; f; f = f->GetParent()) {
    if (f->HasView()) {
      if (aOffset)
        *aOffset = offset;
      return f->GetView();
    }
    offset += f->GetPosition();
  }

  NS_NOTREACHED("No view on any parent?  How did that happen?");
  return nsnull;
}


 void
nsFrame::ChildIsDirty(nsIFrame* aChild)
{
  NS_NOTREACHED("should never be called on a frame that doesn't inherit from "
                "nsContainerFrame");
}


#ifdef ACCESSIBILITY
already_AddRefed<nsAccessible>
nsFrame::CreateAccessible()
{
  return nsnull;
}
#endif

NS_DECLARE_FRAME_PROPERTY(OverflowAreasProperty,
                          nsIFrame::DestroyOverflowAreas)

void
nsIFrame::ClearOverflowRects()
{
  if (mOverflow.mType == NS_FRAME_OVERFLOW_LARGE) {
    Properties().Delete(OverflowAreasProperty());
  }
  mOverflow.mType = NS_FRAME_OVERFLOW_NONE;
}





nsOverflowAreas*
nsIFrame::GetOverflowAreasProperty()
{
  FrameProperties props = Properties();
  nsOverflowAreas *overflow =
    static_cast<nsOverflowAreas*>(props.Get(OverflowAreasProperty()));

  if (overflow) {
    return overflow; 
  }

  
  
  overflow = new nsOverflowAreas;
  props.Set(OverflowAreasProperty(), overflow);
  return overflow;
}




void
nsIFrame::SetOverflowAreas(const nsOverflowAreas& aOverflowAreas)
{
  if (mOverflow.mType == NS_FRAME_OVERFLOW_LARGE) {
    nsOverflowAreas *overflow =
      static_cast<nsOverflowAreas*>(Properties().Get(OverflowAreasProperty()));
    *overflow = aOverflowAreas;

    
    
    return;
  }

  const nsRect& vis = aOverflowAreas.VisualOverflow();
  PRUint32 l = -vis.x, 
           t = -vis.y, 
           r = vis.XMost() - mRect.width, 
           b = vis.YMost() - mRect.height; 
  if (aOverflowAreas.ScrollableOverflow() == nsRect(nsPoint(0, 0), GetSize()) &&
      l <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      t <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      r <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      b <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      
      
      
      
      
      
      
      
      (l | t | r | b) != 0) {
    
    
    
    
    mOverflow.mVisualDeltas.mLeft   = l;
    mOverflow.mVisualDeltas.mTop    = t;
    mOverflow.mVisualDeltas.mRight  = r;
    mOverflow.mVisualDeltas.mBottom = b;
  } else {
    
    mOverflow.mType = NS_FRAME_OVERFLOW_LARGE;
    nsOverflowAreas* overflow = GetOverflowAreasProperty();
    NS_ASSERTION(overflow, "should have created areas");
    *overflow = aOverflowAreas;
  }
}

inline PRBool
IsInlineFrame(nsIFrame *aFrame)
{
  nsIAtom *type = aFrame->GetType();
  return type == nsGkAtoms::inlineFrame ||
         type == nsGkAtoms::positionedInlineFrame;
}

void 
nsIFrame::FinishAndStoreOverflow(nsOverflowAreas& aOverflowAreas,
                                 nsSize aNewSize)
{
  nsRect bounds(nsPoint(0, 0), aNewSize);

  
  
  
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    NS_ASSERTION(aNewSize.width == 0 || aNewSize.height == 0 ||
                 aOverflowAreas.Overflow(otype).Contains(nsRect(nsPoint(0,0), aNewSize)),
                 "Computed overflow area must contain frame bounds");
  }

  
  
  
  
  const nsStyleDisplay *disp = GetStyleDisplay();
  NS_ASSERTION((disp->mOverflowY == NS_STYLE_OVERFLOW_CLIP) ==
               (disp->mOverflowX == NS_STYLE_OVERFLOW_CLIP),
               "If one overflow is clip, the other should be too");
  if (disp->mOverflowX == NS_STYLE_OVERFLOW_CLIP ||
      nsFrame::ApplyPaginatedOverflowClipping(this)) {
    
    aOverflowAreas.SetAllTo(bounds);
  }

  
  
  
  
  if (aNewSize.width != 0 || !IsInlineFrame(this)) {
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o.UnionRectIncludeEmpty(o, bounds);
    }
  }

  
  
  if (!IsBoxWrapped() && IsThemed(disp)) {
    nsRect r(bounds);
    nsPresContext *presContext = PresContext();
    if (presContext->GetTheme()->
          GetWidgetOverflow(presContext->DeviceContext(), this,
                            disp->mAppearance, &r)) {
      NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
        nsRect& o = aOverflowAreas.Overflow(otype);
        o.UnionRectIncludeEmpty(o, r);
      }
    }
  }

  
  PRBool hasOutlineOrEffects;
  aOverflowAreas.VisualOverflow() =
    ComputeOutlineAndEffectsRect(this, &hasOutlineOrEffects,
                                 aOverflowAreas.VisualOverflow(), aNewSize,
                                 PR_TRUE);

  
  PRBool didHaveAbsPosClip = (GetStateBits() & NS_FRAME_HAS_CLIP) != 0;
  nsRect absPosClipRect;
  PRBool hasAbsPosClip = GetAbsPosClipRect(disp, &absPosClipRect, aNewSize);
  if (hasAbsPosClip) {
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o.IntersectRect(o, absPosClipRect);
    }
    AddStateBits(NS_FRAME_HAS_CLIP);
  } else {
    RemoveStateBits(NS_FRAME_HAS_CLIP);
  }

  
  PRBool hasTransform = IsTransformed();
  if (hasTransform) {
    Properties().Set(nsIFrame::PreTransformBBoxProperty(),
                     new nsRect(aOverflowAreas.VisualOverflow()));
    



    nsRect newBounds(nsPoint(0, 0), aNewSize);
    
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o = nsDisplayTransform::TransformRect(o, this, nsPoint(0, 0), &newBounds);
    }
  }

  PRBool visualOverflowChanged =
    GetVisualOverflowRect() != aOverflowAreas.VisualOverflow();

  if (aOverflowAreas != nsOverflowAreas(bounds, bounds)) {
    SetOverflowAreas(aOverflowAreas);
  } else {
    ClearOverflowRects();
  }

  if (visualOverflowChanged) {
    if (hasOutlineOrEffects) {
      
      
      
      
      
      
      
      
      
      
      
      Invalidate(aOverflowAreas.VisualOverflow());
    } else if (hasAbsPosClip || didHaveAbsPosClip) {
      
      
      
      
      
      
      Invalidate(aOverflowAreas.VisualOverflow());
    } else if (hasTransform) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      InvalidateLayer(aOverflowAreas.VisualOverflow(),
                      nsDisplayItem::TYPE_TRANSFORM);
    }
  }
}

void
nsFrame::ConsiderChildOverflow(nsOverflowAreas& aOverflowAreas,
                               nsIFrame* aChildFrame)
{
  const nsStyleDisplay* disp = GetStyleDisplay();
  
  
  
  
  if (!disp->IsTableClip()) {
    aOverflowAreas.UnionWith(aChildFrame->GetOverflowAreas() +
                             aChildFrame->GetPosition());
  }
}

NS_IMETHODIMP 
nsFrame::GetParentStyleContextFrame(nsPresContext* aPresContext,
                                    nsIFrame**      aProviderFrame,
                                    PRBool*         aIsChild)
{
  return DoGetParentStyleContextFrame(aPresContext, aProviderFrame, aIsChild);
}










static nsIFrame*
GetIBSpecialSiblingForAnonymousBlock(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Must have a non-null frame!");
  NS_ASSERTION(aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL,
               "GetIBSpecialSibling should not be called on a non-special frame");

  nsIAtom* type = aFrame->GetStyleContext()->GetPseudo();
  if (type != nsCSSAnonBoxes::mozAnonymousBlock &&
      type != nsCSSAnonBoxes::mozAnonymousPositionedBlock) {
    
    return nsnull;
  }

  
  
  aFrame = aFrame->GetFirstContinuation();

  



  nsIFrame *specialSibling = static_cast<nsIFrame*>
    (aFrame->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
  NS_ASSERTION(specialSibling, "Broken frame tree?");
  return specialSibling;
}











static nsresult
GetCorrectedParent(nsPresContext* aPresContext, nsIFrame* aFrame,
                   nsIFrame** aSpecialParent)
{
  nsIFrame *parent = aFrame->GetParent();
  if (!parent) {
    *aSpecialParent = nsnull;
  } else {
    nsIAtom* pseudo = aFrame->GetStyleContext()->GetPseudo();
    
    
    
    if (pseudo == nsCSSAnonBoxes::tableOuter) {
      pseudo =
        aFrame->GetFirstChild(nsnull)->GetStyleContext()->GetPseudo();
    }
    *aSpecialParent = nsFrame::CorrectStyleParentFrame(parent, pseudo);
  }

  return NS_OK;
}


nsIFrame*
nsFrame::CorrectStyleParentFrame(nsIFrame* aProspectiveParent,
                                 nsIAtom* aChildPseudo)
{
  NS_PRECONDITION(aProspectiveParent, "Must have a prospective parent");

  
  
  if (aChildPseudo && aChildPseudo != nsCSSAnonBoxes::mozNonElement &&
      nsCSSAnonBoxes::IsAnonBox(aChildPseudo)) {
    NS_ASSERTION(aChildPseudo != nsCSSAnonBoxes::mozAnonymousBlock &&
                 aChildPseudo != nsCSSAnonBoxes::mozAnonymousPositionedBlock,
                 "Should have dealt with kids that have NS_FRAME_IS_SPECIAL "
                 "elsewhere");
    return aProspectiveParent;
  }

  
  
  
  nsIFrame* parent = aProspectiveParent;
  do {
    if (parent->GetStateBits() & NS_FRAME_IS_SPECIAL) {
      nsIFrame* sibling = GetIBSpecialSiblingForAnonymousBlock(parent);

      if (sibling) {
        
        
        parent = sibling;
      }
    }
      
    nsIAtom* parentPseudo = parent->GetStyleContext()->GetPseudo();
    if (!parentPseudo ||
        (!nsCSSAnonBoxes::IsAnonBox(parentPseudo) &&
         
         
         
         
         aChildPseudo != nsGkAtoms::placeholderFrame)) {
      return parent;
    }

    parent = parent->GetParent();
  } while (parent);

  if (aProspectiveParent->GetStyleContext()->GetPseudo() ==
      nsCSSAnonBoxes::viewportScroll) {
    
    
    return aProspectiveParent;
  }

  
  
  
  NS_ASSERTION(aProspectiveParent->GetType() == nsGkAtoms::canvasFrame,
               "Should have found a parent before this");
  return nsnull;
}

nsresult
nsFrame::DoGetParentStyleContextFrame(nsPresContext* aPresContext,
                                      nsIFrame**      aProviderFrame,
                                      PRBool*         aIsChild)
{
  *aIsChild = PR_FALSE;
  *aProviderFrame = nsnull;
  if (mContent && !mContent->GetParent() &&
      !GetStyleContext()->GetPseudo()) {
    
    return NS_OK;
  }
  
  if (!(mState & NS_FRAME_OUT_OF_FLOW)) {
    




    if (mState & NS_FRAME_IS_SPECIAL) {
      *aProviderFrame = GetIBSpecialSiblingForAnonymousBlock(this);

      if (*aProviderFrame) {
        return NS_OK;
      }
    }

    
    
    
    return GetCorrectedParent(aPresContext, this, aProviderFrame);
  }

  
  
  nsIFrame* oofFrame = this;
  if ((oofFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
      GetPrevInFlow()) {
    
    
    oofFrame = oofFrame->GetFirstInFlow();
  }
  nsIFrame *placeholder =
    aPresContext->FrameManager()->GetPlaceholderFrameFor(oofFrame);
  if (!placeholder) {
    NS_NOTREACHED("no placeholder frame for out-of-flow frame");
    GetCorrectedParent(aPresContext, this, aProviderFrame);
    return NS_ERROR_FAILURE;
  }
  return static_cast<nsFrame*>(placeholder)->
    GetParentStyleContextFrame(aPresContext, aProviderFrame, aIsChild);
}






void
nsFrame::GetLastLeaf(nsPresContext* aPresContext, nsIFrame **aFrame)
{
  if (!aFrame || !*aFrame)
    return;
  nsIFrame *child = *aFrame;
  
  while (1){
    child = child->GetFirstChild(nsnull);
    if (!child)
      return;
    nsIFrame* siblingFrame;
    nsIContent* content;
    
    
    while ((siblingFrame = child->GetNextSibling()) &&
           (content = siblingFrame->GetContent()) &&
           !content->IsRootOfNativeAnonymousSubtree())
      child = siblingFrame;
    *aFrame = child;
  }
}

void
nsFrame::GetFirstLeaf(nsPresContext* aPresContext, nsIFrame **aFrame)
{
  if (!aFrame || !*aFrame)
    return;
  nsIFrame *child = *aFrame;
  while (1){
    child = child->GetFirstChild(nsnull);
    if (!child)
      return;
    *aFrame = child;
  }
}

 const void*
nsFrame::GetStyleDataExternal(nsStyleStructID aSID) const
{
  NS_ASSERTION(mStyleContext, "unexpected null pointer");
  return mStyleContext->GetStyleData(aSID);
}

 PRBool
nsIFrame::IsFocusable(PRInt32 *aTabIndex, PRBool aWithMouse)
{
  PRInt32 tabIndex = -1;
  if (aTabIndex) {
    *aTabIndex = -1; 
  }
  PRBool isFocusable = PR_FALSE;

  if (mContent && mContent->IsElement() && AreAncestorViewsVisible()) {
    const nsStyleVisibility* vis = GetStyleVisibility();
    if (vis->mVisible != NS_STYLE_VISIBILITY_COLLAPSE &&
        vis->mVisible != NS_STYLE_VISIBILITY_HIDDEN) {
      const nsStyleUserInterface* ui = GetStyleUserInterface();
      if (ui->mUserFocus != NS_STYLE_USER_FOCUS_IGNORE &&
          ui->mUserFocus != NS_STYLE_USER_FOCUS_NONE) {
        
        tabIndex = 0;
      }
      isFocusable = mContent->IsFocusable(&tabIndex, aWithMouse);
      if (!isFocusable && !aWithMouse &&
          GetType() == nsGkAtoms::scrollFrame &&
          mContent->IsHTML() &&
          !mContent->IsRootOfNativeAnonymousSubtree() &&
          mContent->GetParent() &&
          !mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
        
        
        
        
        
        
        
        nsIScrollableFrame *scrollFrame = do_QueryFrame(this);
        if (scrollFrame &&
            scrollFrame->GetActualScrollbarSizes() != nsMargin(0,0,0,0)) {
            
            isFocusable = PR_TRUE;
            tabIndex = 0;
        }
      }
    }
  }

  if (aTabIndex) {
    *aTabIndex = tabIndex;
  }
  return isFocusable;
}





PRBool
nsIFrame::HasTerminalNewline() const
{
  return PR_FALSE;
}


void nsFrame::FillCursorInformationFromStyle(const nsStyleUserInterface* ui,
                                             nsIFrame::Cursor& aCursor)
{
  aCursor.mCursor = ui->mCursor;
  aCursor.mHaveHotspot = PR_FALSE;
  aCursor.mHotspotX = aCursor.mHotspotY = 0.0f;

  for (nsCursorImage *item = ui->mCursorArray,
                 *item_end = ui->mCursorArray + ui->mCursorArrayLength;
       item < item_end; ++item) {
    PRUint32 status;
    nsresult rv = item->GetImage()->GetImageStatus(&status);
    if (NS_SUCCEEDED(rv) && (status & imgIRequest::STATUS_LOAD_COMPLETE)) {
      
      item->GetImage()->GetImage(getter_AddRefs(aCursor.mContainer));
      aCursor.mHaveHotspot = item->mHaveHotspot;
      aCursor.mHotspotX = item->mHotspotX;
      aCursor.mHotspotY = item->mHotspotY;
      break;
    }
  }
}

NS_IMETHODIMP
nsFrame::RefreshSizeCache(nsBoxLayoutState& aState)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsresult rv = NS_OK;
  nsRenderingContext* rendContext = aState.GetRenderingContext();
  if (rendContext) {
    nsPresContext* presContext = aState.PresContext();

    
    
    nsBoxLayoutMetrics* metrics = BoxMetrics();
    if (!DoesNeedRecalc(metrics->mBlockPrefSize))
      return NS_OK;

    
    nsRect oldRect = GetRect();

    
    nsRect rect(oldRect);

    nsMargin bp(0,0,0,0);
    GetBorderAndPadding(bp);

    metrics->mBlockPrefSize.width = GetPrefWidth(rendContext) + bp.LeftRight();
    metrics->mBlockMinSize.width = GetMinWidth(rendContext) + bp.LeftRight();

    
    nsHTMLReflowMetrics desiredSize;
    rv = BoxReflow(aState, presContext, desiredSize, rendContext,
                   rect.x, rect.y,
                   metrics->mBlockPrefSize.width, NS_UNCONSTRAINEDSIZE);

    nsRect newRect = GetRect();

    
    if (oldRect.width != newRect.width || oldRect.height != newRect.height) {
      newRect.x = 0;
      newRect.y = 0;
      Redraw(aState, &newRect);
    }

    metrics->mBlockMinSize.height = 0;
    
    
    nsAutoLineIterator lines = GetLineIterator();
    if (lines) 
    {
      metrics->mBlockMinSize.height = 0;
      int count = 0;
      nsIFrame* firstFrame = nsnull;
      PRInt32 framesOnLine;
      nsRect lineBounds;
      PRUint32 lineFlags;

      do {
         lines->GetLine(count, &firstFrame, &framesOnLine, lineBounds, &lineFlags);

         if (lineBounds.height > metrics->mBlockMinSize.height)
           metrics->mBlockMinSize.height = lineBounds.height;

         count++;
      } while(firstFrame);
    } else {
      metrics->mBlockMinSize.height = desiredSize.height;
    }

    metrics->mBlockPrefSize.height = metrics->mBlockMinSize.height;

    if (desiredSize.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
      if (!nsLayoutUtils::GetFirstLineBaseline(this, &metrics->mBlockAscent))
        metrics->mBlockAscent = GetBaseline();
    } else {
      metrics->mBlockAscent = desiredSize.ascent;
    }

#ifdef DEBUG_adaptor
    printf("min=(%d,%d), pref=(%d,%d), ascent=%d\n", metrics->mBlockMinSize.width,
                                                     metrics->mBlockMinSize.height,
                                                     metrics->mBlockPrefSize.width,
                                                     metrics->mBlockPrefSize.height,
                                                     metrics->mBlockAscent);
#endif
  }

  return rv;
}

 nsILineIterator*
nsFrame::GetLineIterator()
{
  return nsnull;
}

nsSize
nsFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  nsSize size(0,0);
  DISPLAY_PREF_SIZE(this, size);
  
  
  nsBoxLayoutMetrics *metrics = BoxMetrics();
  if (!DoesNeedRecalc(metrics->mPrefSize)) {
    return metrics->mPrefSize;
  }

  if (IsCollapsed(aState))
    return size;

  
  PRBool widthSet, heightSet;
  PRBool completelyRedefined = nsIBox::AddCSSPrefSize(this, size, widthSet, heightSet);

  
  if (!completelyRedefined) {
    RefreshSizeCache(aState);
    nsSize blockSize = metrics->mBlockPrefSize;

    
    
    if (!widthSet)
      size.width = blockSize.width;
    if (!heightSet)
      size.height = blockSize.height;
  }

  metrics->mPrefSize = size;
  return size;
}

nsSize
nsFrame::GetMinSize(nsBoxLayoutState& aState)
{
  nsSize size(0,0);
  DISPLAY_MIN_SIZE(this, size);
  
  nsBoxLayoutMetrics *metrics = BoxMetrics();
  if (!DoesNeedRecalc(metrics->mMinSize)) {
    size = metrics->mMinSize;
    return size;
  }

  if (IsCollapsed(aState))
    return size;

  
  PRBool widthSet, heightSet;
  PRBool completelyRedefined =
    nsIBox::AddCSSMinSize(aState, this, size, widthSet, heightSet);

  
  if (!completelyRedefined) {
    RefreshSizeCache(aState);
    nsSize blockSize = metrics->mBlockMinSize;

    if (!widthSet)
      size.width = blockSize.width;
    if (!heightSet)
      size.height = blockSize.height;
  }

  metrics->mMinSize = size;
  return size;
}

nsSize
nsFrame::GetMaxSize(nsBoxLayoutState& aState)
{
  nsSize size(NS_INTRINSICSIZE, NS_INTRINSICSIZE);
  DISPLAY_MAX_SIZE(this, size);
  
  nsBoxLayoutMetrics *metrics = BoxMetrics();
  if (!DoesNeedRecalc(metrics->mMaxSize)) {
    size = metrics->mMaxSize;
    return size;
  }

  if (IsCollapsed(aState))
    return size;

  size = nsBox::GetMaxSize(aState);
  metrics->mMaxSize = size;

  return size;
}

nscoord
nsFrame::GetFlex(nsBoxLayoutState& aState)
{
  nsBoxLayoutMetrics *metrics = BoxMetrics();
  if (!DoesNeedRecalc(metrics->mFlex))
     return metrics->mFlex;

  metrics->mFlex = nsBox::GetFlex(aState);

  return metrics->mFlex;
}

nscoord
nsFrame::GetBoxAscent(nsBoxLayoutState& aState)
{
  nsBoxLayoutMetrics *metrics = BoxMetrics();
  if (!DoesNeedRecalc(metrics->mAscent))
    return metrics->mAscent;

  if (IsCollapsed(aState)) {
    metrics->mAscent = 0;
  } else {
    
    RefreshSizeCache(aState);
    metrics->mAscent = metrics->mBlockAscent;
  }

  return metrics->mAscent;
}

nsresult
nsFrame::DoLayout(nsBoxLayoutState& aState)
{
  nsRect ourRect(mRect);

  nsRenderingContext* rendContext = aState.GetRenderingContext();
  nsPresContext* presContext = aState.PresContext();
  nsHTMLReflowMetrics desiredSize;
  nsresult rv = NS_OK;
 
  if (rendContext) {

    rv = BoxReflow(aState, presContext, desiredSize, rendContext,
                   ourRect.x, ourRect.y, ourRect.width, ourRect.height);

    if (IsCollapsed(aState)) {
      SetSize(nsSize(0, 0));
    } else {

      
      
      
      if (desiredSize.width > ourRect.width ||
          desiredSize.height > ourRect.height) {

#ifdef DEBUG_GROW
        DumpBox(stdout);
        printf(" GREW from (%d,%d) -> (%d,%d)\n",
               ourRect.width, ourRect.height,
               desiredSize.width, desiredSize.height);
#endif

        if (desiredSize.width > ourRect.width)
          ourRect.width = desiredSize.width;

        if (desiredSize.height > ourRect.height)
          ourRect.height = desiredSize.height;
      }

      
      
      SetSize(nsSize(ourRect.width, ourRect.height));
    }
  }

  
  nsSize size(GetSize());
  desiredSize.width = size.width;
  desiredSize.height = size.height;
  desiredSize.UnionOverflowAreasWithDesiredBounds();
  FinishAndStoreOverflow(desiredSize.mOverflowAreas, size);

  SyncLayout(aState);

  return rv;
}

nsresult
nsFrame::BoxReflow(nsBoxLayoutState&        aState,
                   nsPresContext*           aPresContext,
                   nsHTMLReflowMetrics&     aDesiredSize,
                   nsRenderingContext*     aRenderingContext,
                   nscoord                  aX,
                   nscoord                  aY,
                   nscoord                  aWidth,
                   nscoord                  aHeight,
                   PRBool                   aMoveFrame)
{
  DO_GLOBAL_REFLOW_COUNT("nsBoxToBlockAdaptor");

#ifdef DEBUG_REFLOW
  nsAdaptorAddIndents();
  printf("Reflowing: ");
  nsFrame::ListTag(stdout, mFrame);
  printf("\n");
  gIndent2++;
#endif

  
  







  nsBoxLayoutMetrics *metrics = BoxMetrics();
  nsReflowStatus status = NS_FRAME_COMPLETE;

  PRBool redrawAfterReflow = PR_FALSE;
  PRBool redrawNow = PR_FALSE;

  PRBool needsReflow = NS_SUBTREE_DIRTY(this);

  if (redrawNow)
     Redraw(aState);

  
  
  if (!needsReflow) {
      
      if (aWidth != NS_INTRINSICSIZE && aHeight != NS_INTRINSICSIZE) {
      
          
          if ((metrics->mLastSize.width == 0 || metrics->mLastSize.height == 0) && (aWidth == 0 || aHeight == 0)) {
               needsReflow = PR_FALSE;
               aDesiredSize.width = aWidth; 
               aDesiredSize.height = aHeight; 
               SetSize(nsSize(aDesiredSize.width, aDesiredSize.height));
          } else {
            aDesiredSize.width = metrics->mLastSize.width;
            aDesiredSize.height = metrics->mLastSize.height;

            
            
            if (metrics->mLastSize.width == aWidth && metrics->mLastSize.height == aHeight)
                  needsReflow = PR_FALSE;
            else
                  needsReflow = PR_TRUE;
   
          }
      } else {
          
          
         needsReflow = PR_TRUE;
      }
  }
                             
  
  if (needsReflow) {

    aDesiredSize.width = 0;
    aDesiredSize.height = 0;

    

    
    
    nsMargin margin(0,0,0,0);
    GetMargin(margin);

    nsSize parentSize(aWidth, aHeight);
    if (parentSize.height != NS_INTRINSICSIZE)
      parentSize.height += margin.TopBottom();
    if (parentSize.width != NS_INTRINSICSIZE)
      parentSize.width += margin.LeftRight();

    nsIFrame *parentFrame = GetParent();
    nsFrameState savedState = parentFrame->GetStateBits();
    nsHTMLReflowState parentReflowState(aPresContext, parentFrame,
                                        aRenderingContext,
                                        parentSize);
    parentFrame->RemoveStateBits(~nsFrameState(0));
    parentFrame->AddStateBits(savedState);

    
    if (parentSize.width != NS_INTRINSICSIZE)
      parentReflowState.SetComputedWidth(NS_MAX(parentSize.width, 0));
    if (parentSize.height != NS_INTRINSICSIZE)
      parentReflowState.SetComputedHeight(NS_MAX(parentSize.height, 0));
    parentReflowState.mComputedMargin.SizeTo(0, 0, 0, 0);
    
    parentFrame->GetPadding(parentReflowState.mComputedPadding);
    parentFrame->GetBorder(parentReflowState.mComputedBorderPadding);
    parentReflowState.mComputedBorderPadding +=
      parentReflowState.mComputedPadding;

    
    
    nsSize availSize(aWidth, NS_INTRINSICSIZE);
    nsHTMLReflowState reflowState(aPresContext, this, aRenderingContext,
                                  availSize);

    
    
    reflowState.parentReflowState = &parentReflowState;
    reflowState.mCBReflowState = &parentReflowState;
    reflowState.mReflowDepth = aState.GetReflowDepth();

    
    
    if (aWidth != NS_INTRINSICSIZE) {
      nscoord computedWidth =
        aWidth - reflowState.mComputedBorderPadding.LeftRight();
      computedWidth = NS_MAX(computedWidth, 0);
      reflowState.SetComputedWidth(computedWidth);
    }

    
    
    
    
    
    if (!IsFrameOfType(eBlockFrame)) {
      if (aHeight != NS_INTRINSICSIZE) {
        nscoord computedHeight =
          aHeight - reflowState.mComputedBorderPadding.TopBottom();
        computedHeight = NS_MAX(computedHeight, 0);
        reflowState.SetComputedHeight(computedHeight);
      } else {
        reflowState.SetComputedHeight(
          ComputeSize(aRenderingContext, availSize, availSize.width,
                      nsSize(reflowState.mComputedMargin.LeftRight(),
                             reflowState.mComputedMargin.TopBottom()),
                      nsSize(reflowState.mComputedBorderPadding.LeftRight() -
                               reflowState.mComputedPadding.LeftRight(),
                             reflowState.mComputedBorderPadding.TopBottom() -
                               reflowState.mComputedPadding.TopBottom()),
                      nsSize(reflowState.mComputedPadding.LeftRight(),
                               reflowState.mComputedPadding.TopBottom()),
                      PR_FALSE).height
          );
      }
    }

    
    
    
    
    
    
    if (metrics->mLastSize.width != aWidth)
      reflowState.mFlags.mHResize = PR_TRUE;
    if (metrics->mLastSize.height != aHeight)
      reflowState.mFlags.mVResize = PR_TRUE;

    #ifdef DEBUG_REFLOW
      nsAdaptorAddIndents();
      printf("Size=(%d,%d)\n",reflowState.ComputedWidth(),
             reflowState.ComputedHeight());
      nsAdaptorAddIndents();
      nsAdaptorPrintReason(reflowState);
      printf("\n");
    #endif

       
    WillReflow(aPresContext);

    Reflow(aPresContext, aDesiredSize, reflowState, status);

    NS_ASSERTION(NS_FRAME_IS_COMPLETE(status), "bad status");

    if (redrawAfterReflow) {
       nsRect r = GetRect();
       r.width = aDesiredSize.width;
       r.height = aDesiredSize.height;
       Redraw(aState, &r);
    }

    PRUint32 layoutFlags = aState.LayoutFlags();
    nsContainerFrame::FinishReflowChild(this, aPresContext, &reflowState,
                                        aDesiredSize, aX, aY, layoutFlags | NS_FRAME_NO_MOVE_FRAME);

    
    if (IsCollapsed(aState)) {
      metrics->mAscent = 0;
    } else {
      if (aDesiredSize.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
        if (!nsLayoutUtils::GetFirstLineBaseline(this, &metrics->mAscent))
          metrics->mAscent = GetBaseline();
      } else
        metrics->mAscent = aDesiredSize.ascent;
    }

  } else {
    aDesiredSize.ascent = metrics->mBlockAscent;
  }

#ifdef DEBUG_REFLOW
  if (aHeight != NS_INTRINSICSIZE && aDesiredSize.height != aHeight)
  {
          nsAdaptorAddIndents();
          printf("*****got taller!*****\n");
         
  }
  if (aWidth != NS_INTRINSICSIZE && aDesiredSize.width != aWidth)
  {
          nsAdaptorAddIndents();
          printf("*****got wider!******\n");
         
  }
#endif

  if (aWidth == NS_INTRINSICSIZE)
     aWidth = aDesiredSize.width;

  if (aHeight == NS_INTRINSICSIZE)
     aHeight = aDesiredSize.height;

  metrics->mLastSize.width = aDesiredSize.width;
  metrics->mLastSize.height = aDesiredSize.height;

#ifdef DEBUG_REFLOW
  gIndent2--;
#endif

  return NS_OK;
}

static void
DestroyBoxMetrics(void* aPropertyValue)
{
  delete static_cast<nsBoxLayoutMetrics*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(BoxMetricsProperty, DestroyBoxMetrics)

nsBoxLayoutMetrics*
nsFrame::BoxMetrics() const
{
  nsBoxLayoutMetrics* metrics =
    static_cast<nsBoxLayoutMetrics*>(Properties().Get(BoxMetricsProperty()));
  NS_ASSERTION(metrics, "A box layout method was called but InitBoxMetrics was never called");
  return metrics;
}

void
nsFrame::SetParent(nsIFrame* aParent)
{
  PRBool wasBoxWrapped = IsBoxWrapped();
  mParent = aParent;
  if (!wasBoxWrapped && IsBoxWrapped()) {
    InitBoxMetrics(PR_TRUE);
  } else if (wasBoxWrapped && !IsBoxWrapped()) {
    Properties().Delete(BoxMetricsProperty());
  }

  if (GetStateBits() & (NS_FRAME_HAS_VIEW | NS_FRAME_HAS_CHILD_WITH_VIEW)) {
    for (nsIFrame* f = aParent;
         f && !(f->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW);
         f = f->GetParent()) {
      f->AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
    }
  }

  if (GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT) {
    for (nsIFrame* f = aParent;
         f && !(f->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT);
         f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
      f->AddStateBits(NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT);
    }
  }
}

void
nsFrame::InitBoxMetrics(PRBool aClear)
{
  FrameProperties props = Properties();
  if (aClear) {
    props.Delete(BoxMetricsProperty());
  }

  nsBoxLayoutMetrics *metrics = new nsBoxLayoutMetrics();
  props.Set(BoxMetricsProperty(), metrics);

  nsFrame::MarkIntrinsicWidthsDirty();
  metrics->mBlockAscent = 0;
  metrics->mLastSize.SizeTo(0, 0);
}


#ifdef DEBUG_REFLOW
PRInt32 gIndent2 = 0;

void
nsAdaptorAddIndents()
{
    for(PRInt32 i=0; i < gIndent2; i++)
    {
        printf(" ");
    }
}

void
nsAdaptorPrintReason(nsHTMLReflowState& aReflowState)
{
    char* reflowReasonString;

    switch(aReflowState.reason) 
    {
        case eReflowReason_Initial:
          reflowReasonString = "initial";
          break;

        case eReflowReason_Resize:
          reflowReasonString = "resize";
          break;
        case eReflowReason_Dirty:
          reflowReasonString = "dirty";
          break;
        case eReflowReason_StyleChange:
          reflowReasonString = "stylechange";
          break;
        case eReflowReason_Incremental: 
        {
            switch (aReflowState.reflowCommand->Type()) {
              case eReflowType_StyleChanged:
                 reflowReasonString = "incremental (StyleChanged)";
              break;
              case eReflowType_ReflowDirty:
                 reflowReasonString = "incremental (ReflowDirty)";
              break;
              default:
                 reflowReasonString = "incremental (Unknown)";
            }
        }                             
        break;
        default:
          reflowReasonString = "unknown";
          break;
    }

    printf("%s",reflowReasonString);
}

#endif
#ifdef DEBUG_LAYOUT
void
nsFrame::GetBoxName(nsAutoString& aName)
{
  GetFrameName(aName);
}
#endif

#ifdef NS_DEBUG
static void
GetTagName(nsFrame* aFrame, nsIContent* aContent, PRIntn aResultSize,
           char* aResult)
{
  if (aContent) {
    PR_snprintf(aResult, aResultSize, "%s@%p",
                nsAtomCString(aContent->Tag()).get(), aFrame);
  }
  else {
    PR_snprintf(aResult, aResultSize, "@%p", aFrame);
  }
}

void
nsFrame::Trace(const char* aMethod, PRBool aEnter)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s", tagbuf, aEnter ? "enter" : "exit", aMethod);
  }
}

void
nsFrame::Trace(const char* aMethod, PRBool aEnter, nsReflowStatus aStatus)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s, status=%scomplete%s",
                tagbuf, aEnter ? "enter" : "exit", aMethod,
                NS_FRAME_IS_NOT_COMPLETE(aStatus) ? "not" : "",
                (NS_FRAME_REFLOW_NEXTINFLOW & aStatus) ? "+reflow" : "");
  }
}

void
nsFrame::TraceMsg(const char* aFormatString, ...)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    
    char argbuf[200];
    va_list ap;
    va_start(ap, aFormatString);
    PR_vsnprintf(argbuf, sizeof(argbuf), aFormatString, ap);
    va_end(ap);

    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s", tagbuf, argbuf);
  }
}

void
nsFrame::VerifyDirtyBitSet(const nsFrameList& aFrameList)
{
  for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->GetStateBits() & NS_FRAME_IS_DIRTY,
                 "dirty bit not set");
  }
}


#ifdef DEBUG

DR_cookie::DR_cookie(nsPresContext*          aPresContext,
                     nsIFrame*                aFrame, 
                     const nsHTMLReflowState& aReflowState,
                     nsHTMLReflowMetrics&     aMetrics,
                     nsReflowStatus&          aStatus)
  :mPresContext(aPresContext), mFrame(aFrame), mReflowState(aReflowState), mMetrics(aMetrics), mStatus(aStatus)
{
  MOZ_COUNT_CTOR(DR_cookie);
  mValue = nsFrame::DisplayReflowEnter(aPresContext, mFrame, mReflowState);
}

DR_cookie::~DR_cookie()
{
  MOZ_COUNT_DTOR(DR_cookie);
  nsFrame::DisplayReflowExit(mPresContext, mFrame, mMetrics, mStatus, mValue);
}

DR_layout_cookie::DR_layout_cookie(nsIFrame* aFrame)
  : mFrame(aFrame)
{
  MOZ_COUNT_CTOR(DR_layout_cookie);
  mValue = nsFrame::DisplayLayoutEnter(mFrame);
}

DR_layout_cookie::~DR_layout_cookie()
{
  MOZ_COUNT_DTOR(DR_layout_cookie);
  nsFrame::DisplayLayoutExit(mFrame, mValue);
}

DR_intrinsic_width_cookie::DR_intrinsic_width_cookie(
                     nsIFrame*                aFrame, 
                     const char*              aType,
                     nscoord&                 aResult)
  : mFrame(aFrame)
  , mType(aType)
  , mResult(aResult)
{
  MOZ_COUNT_CTOR(DR_intrinsic_width_cookie);
  mValue = nsFrame::DisplayIntrinsicWidthEnter(mFrame, mType);
}

DR_intrinsic_width_cookie::~DR_intrinsic_width_cookie()
{
  MOZ_COUNT_DTOR(DR_intrinsic_width_cookie);
  nsFrame::DisplayIntrinsicWidthExit(mFrame, mType, mResult, mValue);
}

DR_intrinsic_size_cookie::DR_intrinsic_size_cookie(
                     nsIFrame*                aFrame, 
                     const char*              aType,
                     nsSize&                  aResult)
  : mFrame(aFrame)
  , mType(aType)
  , mResult(aResult)
{
  MOZ_COUNT_CTOR(DR_intrinsic_size_cookie);
  mValue = nsFrame::DisplayIntrinsicSizeEnter(mFrame, mType);
}

DR_intrinsic_size_cookie::~DR_intrinsic_size_cookie()
{
  MOZ_COUNT_DTOR(DR_intrinsic_size_cookie);
  nsFrame::DisplayIntrinsicSizeExit(mFrame, mType, mResult, mValue);
}

DR_init_constraints_cookie::DR_init_constraints_cookie(
                     nsIFrame*                aFrame,
                     nsHTMLReflowState*       aState,
                     nscoord                  aCBWidth,
                     nscoord                  aCBHeight,
                     const nsMargin*          aMargin,
                     const nsMargin*          aPadding)
  : mFrame(aFrame)
  , mState(aState)
{
  MOZ_COUNT_CTOR(DR_init_constraints_cookie);
  mValue = nsHTMLReflowState::DisplayInitConstraintsEnter(mFrame, mState,
                                                          aCBWidth, aCBHeight,
                                                          aMargin, aPadding);
}

DR_init_constraints_cookie::~DR_init_constraints_cookie()
{
  MOZ_COUNT_DTOR(DR_init_constraints_cookie);
  nsHTMLReflowState::DisplayInitConstraintsExit(mFrame, mState, mValue);
}

DR_init_offsets_cookie::DR_init_offsets_cookie(
                     nsIFrame*                aFrame,
                     nsCSSOffsetState*        aState,
                     nscoord                  aCBWidth,
                     const nsMargin*          aMargin,
                     const nsMargin*          aPadding)
  : mFrame(aFrame)
  , mState(aState)
{
  MOZ_COUNT_CTOR(DR_init_offsets_cookie);
  mValue = nsCSSOffsetState::DisplayInitOffsetsEnter(mFrame, mState, aCBWidth,
                                                     aMargin, aPadding);
}

DR_init_offsets_cookie::~DR_init_offsets_cookie()
{
  MOZ_COUNT_DTOR(DR_init_offsets_cookie);
  nsCSSOffsetState::DisplayInitOffsetsExit(mFrame, mState, mValue);
}

DR_init_type_cookie::DR_init_type_cookie(
                     nsIFrame*                aFrame,
                     nsHTMLReflowState*       aState)
  : mFrame(aFrame)
  , mState(aState)
{
  MOZ_COUNT_CTOR(DR_init_type_cookie);
  mValue = nsHTMLReflowState::DisplayInitFrameTypeEnter(mFrame, mState);
}

DR_init_type_cookie::~DR_init_type_cookie()
{
  MOZ_COUNT_DTOR(DR_init_type_cookie);
  nsHTMLReflowState::DisplayInitFrameTypeExit(mFrame, mState, mValue);
}

struct DR_FrameTypeInfo;
struct DR_FrameTreeNode;
struct DR_Rule;

struct DR_State
{
  DR_State();
  ~DR_State();
  void Init();
  void AddFrameTypeInfo(nsIAtom* aFrameType,
                        const char* aFrameNameAbbrev,
                        const char* aFrameName);
  DR_FrameTypeInfo* GetFrameTypeInfo(nsIAtom* aFrameType);
  DR_FrameTypeInfo* GetFrameTypeInfo(char* aFrameName);
  void InitFrameTypeTable();
  DR_FrameTreeNode* CreateTreeNode(nsIFrame*                aFrame,
                                   const nsHTMLReflowState* aReflowState);
  void FindMatchingRule(DR_FrameTreeNode& aNode);
  PRBool RuleMatches(DR_Rule&          aRule,
                     DR_FrameTreeNode& aNode);
  PRBool GetToken(FILE* aFile,
                  char* aBuf,
                  size_t aBufSize);
  DR_Rule* ParseRule(FILE* aFile);
  void ParseRulesFile();
  void AddRule(nsTArray<DR_Rule*>& aRules,
               DR_Rule&            aRule);
  PRBool IsWhiteSpace(int c);
  PRBool GetNumber(char*    aBuf, 
                 PRInt32&  aNumber);
  void PrettyUC(nscoord aSize,
                char*   aBuf);
  void PrintMargin(const char* tag, const nsMargin* aMargin);
  void DisplayFrameTypeInfo(nsIFrame* aFrame,
                            PRInt32   aIndent);
  void DeleteTreeNode(DR_FrameTreeNode& aNode);

  PRBool      mInited;
  PRBool      mActive;
  PRInt32     mCount;
  PRInt32     mAssert;
  PRInt32     mIndent;
  PRBool      mIndentUndisplayedFrames;
  PRBool      mDisplayPixelErrors;
  nsTArray<DR_Rule*>          mWildRules;
  nsTArray<DR_FrameTypeInfo>  mFrameTypeTable;
  
  nsTArray<DR_FrameTreeNode*> mFrameTreeLeaves;
};

static DR_State *DR_state; 

struct DR_RulePart 
{
  DR_RulePart(nsIAtom* aFrameType) : mFrameType(aFrameType), mNext(0) {}
  void Destroy();

  nsIAtom*     mFrameType;
  DR_RulePart* mNext;
};

void DR_RulePart::Destroy()
{
  if (mNext) {
    mNext->Destroy();
  }
  delete this;
}

struct DR_Rule 
{
  DR_Rule() : mLength(0), mTarget(nsnull), mDisplay(PR_FALSE) {
    MOZ_COUNT_CTOR(DR_Rule);
  }
  ~DR_Rule() {
    if (mTarget) mTarget->Destroy();
    MOZ_COUNT_DTOR(DR_Rule);
  }
  void AddPart(nsIAtom* aFrameType);

  PRUint32      mLength;
  DR_RulePart*  mTarget;
  PRBool        mDisplay;
};

void DR_Rule::AddPart(nsIAtom* aFrameType)
{
  DR_RulePart* newPart = new DR_RulePart(aFrameType);
  newPart->mNext = mTarget;
  mTarget = newPart;
  mLength++;
}

struct DR_FrameTypeInfo
{
  DR_FrameTypeInfo(nsIAtom* aFrmeType, const char* aFrameNameAbbrev, const char* aFrameName);
  ~DR_FrameTypeInfo() { 
      PRInt32 numElements;
      numElements = mRules.Length();
      for (PRInt32 i = numElements - 1; i >= 0; i--) {
        delete mRules.ElementAt(i);
      }
   }

  nsIAtom*    mType;
  char        mNameAbbrev[16];
  char        mName[32];
  nsTArray<DR_Rule*> mRules;
private:
  DR_FrameTypeInfo& operator=(const DR_FrameTypeInfo&); 
};

DR_FrameTypeInfo::DR_FrameTypeInfo(nsIAtom* aFrameType, 
                                   const char* aFrameNameAbbrev, 
                                   const char* aFrameName)
{
  mType = aFrameType;
  PL_strncpyz(mNameAbbrev, aFrameNameAbbrev, sizeof(mNameAbbrev));
  PL_strncpyz(mName, aFrameName, sizeof(mName));
}

struct DR_FrameTreeNode
{
  DR_FrameTreeNode(nsIFrame* aFrame, DR_FrameTreeNode* aParent) : mFrame(aFrame), mParent(aParent), mDisplay(0), mIndent(0)
  {
    MOZ_COUNT_CTOR(DR_FrameTreeNode);
  }

  ~DR_FrameTreeNode()
  {
    MOZ_COUNT_DTOR(DR_FrameTreeNode);
  }

  nsIFrame*         mFrame;
  DR_FrameTreeNode* mParent;
  PRBool            mDisplay;
  PRUint32          mIndent;
};



DR_State::DR_State() 
: mInited(PR_FALSE), mActive(PR_FALSE), mCount(0), mAssert(-1), mIndent(0), 
  mIndentUndisplayedFrames(PR_FALSE), mDisplayPixelErrors(PR_FALSE)
{
  MOZ_COUNT_CTOR(DR_State);
}

void DR_State::Init() 
{
  char* env = PR_GetEnv("GECKO_DISPLAY_REFLOW_ASSERT");
  PRInt32 num;
  if (env) {
    if (GetNumber(env, num)) 
      mAssert = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_ASSERT - invalid value = %s", env);
  }

  env = PR_GetEnv("GECKO_DISPLAY_REFLOW_INDENT_START");
  if (env) {
    if (GetNumber(env, num)) 
      mIndent = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_INDENT_START - invalid value = %s", env);
  }

  env = PR_GetEnv("GECKO_DISPLAY_REFLOW_INDENT_UNDISPLAYED_FRAMES");
  if (env) {
    if (GetNumber(env, num)) 
      mIndentUndisplayedFrames = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_INDENT_UNDISPLAYED_FRAMES - invalid value = %s", env);
  }

  env = PR_GetEnv("GECKO_DISPLAY_REFLOW_FLAG_PIXEL_ERRORS");
  if (env) {
    if (GetNumber(env, num)) 
      mDisplayPixelErrors = num;
    else 
      printf("GECKO_DISPLAY_REFLOW_FLAG_PIXEL_ERRORS - invalid value = %s", env);
  }

  InitFrameTypeTable();
  ParseRulesFile();
  mInited = PR_TRUE;
}

DR_State::~DR_State()
{
  MOZ_COUNT_DTOR(DR_State);
  PRInt32 numElements, i;
  numElements = mWildRules.Length();
  for (i = numElements - 1; i >= 0; i--) {
    delete mWildRules.ElementAt(i);
  }
  numElements = mFrameTreeLeaves.Length();
  for (i = numElements - 1; i >= 0; i--) {
    delete mFrameTreeLeaves.ElementAt(i);
  }
}

PRBool DR_State::GetNumber(char*     aBuf, 
                           PRInt32&  aNumber)
{
  if (sscanf(aBuf, "%d", &aNumber) > 0) 
    return PR_TRUE;
  else 
    return PR_FALSE;
}

PRBool DR_State::IsWhiteSpace(int c) {
  return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

PRBool DR_State::GetToken(FILE* aFile,
                          char* aBuf,
                          size_t aBufSize)
{
  PRBool haveToken = PR_FALSE;
  aBuf[0] = 0;
  
  int c = -1;
  for (c = getc(aFile); (c > 0) && IsWhiteSpace(c); c = getc(aFile)) {
  }

  if (c > 0) {
    haveToken = PR_TRUE;
    aBuf[0] = c;
    
    size_t cX;
    for (cX = 1; cX + 1 < aBufSize ; cX++) {
      c = getc(aFile);
      if (c < 0) { 
        ungetc(' ', aFile); 
        break;
      }
      else {
        if (IsWhiteSpace(c)) {
          break;
        }
        else {
          aBuf[cX] = c;
        }
      }
    }
    aBuf[cX] = 0;
  }
  return haveToken;
}

DR_Rule* DR_State::ParseRule(FILE* aFile)
{
  char buf[128];
  PRInt32 doDisplay;
  DR_Rule* rule = nsnull;
  while (GetToken(aFile, buf, sizeof(buf))) {
    if (GetNumber(buf, doDisplay)) {
      if (rule) { 
        rule->mDisplay = !!doDisplay;
        break;
      }
      else {
        printf("unexpected token - %s \n", buf);
      }
    }
    else {
      if (!rule) {
        rule = new DR_Rule;
      }
      if (strcmp(buf, "*") == 0) {
        rule->AddPart(nsnull);
      }
      else {
        DR_FrameTypeInfo* info = GetFrameTypeInfo(buf);
        if (info) {
          rule->AddPart(info->mType);
        }
        else {
          printf("invalid frame type - %s \n", buf);
        }
      }
    }
  }
  return rule;
}

void DR_State::AddRule(nsTArray<DR_Rule*>& aRules,
                       DR_Rule&            aRule)
{
  PRInt32 numRules = aRules.Length();
  for (PRInt32 ruleX = 0; ruleX < numRules; ruleX++) {
    DR_Rule* rule = aRules.ElementAt(ruleX);
    NS_ASSERTION(rule, "program error");
    if (aRule.mLength > rule->mLength) {
      aRules.InsertElementAt(ruleX, &aRule);
      return;
    }
  }
  aRules.AppendElement(&aRule);
}

void DR_State::ParseRulesFile()
{
  char* path = PR_GetEnv("GECKO_DISPLAY_REFLOW_RULES_FILE");
  if (path) {
    FILE* inFile = fopen(path, "r");
    if (inFile) {
      for (DR_Rule* rule = ParseRule(inFile); rule; rule = ParseRule(inFile)) {
        if (rule->mTarget) {
          nsIAtom* fType = rule->mTarget->mFrameType;
          if (fType) {
            DR_FrameTypeInfo* info = GetFrameTypeInfo(fType);
            if (info) {
              AddRule(info->mRules, *rule);
            }
          }
          else {
            AddRule(mWildRules, *rule);
          }
          mActive = PR_TRUE;
        }
      }
    }
  }
}


void DR_State::AddFrameTypeInfo(nsIAtom* aFrameType,
                                const char* aFrameNameAbbrev,
                                const char* aFrameName)
{
  mFrameTypeTable.AppendElement(DR_FrameTypeInfo(aFrameType, aFrameNameAbbrev, aFrameName));
}

DR_FrameTypeInfo* DR_State::GetFrameTypeInfo(nsIAtom* aFrameType)
{
  PRInt32 numEntries = mFrameTypeTable.Length();
  NS_ASSERTION(numEntries != 0, "empty FrameTypeTable");
  for (PRInt32 i = 0; i < numEntries; i++) {
    DR_FrameTypeInfo& info = mFrameTypeTable.ElementAt(i);
    if (info.mType == aFrameType) {
      return &info;
    }
  }
  return &mFrameTypeTable.ElementAt(numEntries - 1); 
}

DR_FrameTypeInfo* DR_State::GetFrameTypeInfo(char* aFrameName)
{
  PRInt32 numEntries = mFrameTypeTable.Length();
  NS_ASSERTION(numEntries != 0, "empty FrameTypeTable");
  for (PRInt32 i = 0; i < numEntries; i++) {
    DR_FrameTypeInfo& info = mFrameTypeTable.ElementAt(i);
    if ((strcmp(aFrameName, info.mName) == 0) || (strcmp(aFrameName, info.mNameAbbrev) == 0)) {
      return &info;
    }
  }
  return &mFrameTypeTable.ElementAt(numEntries - 1); 
}

void DR_State::InitFrameTypeTable()
{  
  AddFrameTypeInfo(nsGkAtoms::blockFrame,            "block",     "block");
  AddFrameTypeInfo(nsGkAtoms::brFrame,               "br",        "br");
  AddFrameTypeInfo(nsGkAtoms::bulletFrame,           "bullet",    "bullet");
  AddFrameTypeInfo(nsGkAtoms::gfxButtonControlFrame, "button",    "gfxButtonControl");
  AddFrameTypeInfo(nsGkAtoms::HTMLButtonControlFrame, "HTMLbutton",    "HTMLButtonControl");
  AddFrameTypeInfo(nsGkAtoms::HTMLCanvasFrame,       "HTMLCanvas","HTMLCanvas");
  AddFrameTypeInfo(nsGkAtoms::subDocumentFrame,      "subdoc",    "subDocument");
  AddFrameTypeInfo(nsGkAtoms::imageFrame,            "img",       "image");
  AddFrameTypeInfo(nsGkAtoms::inlineFrame,           "inline",    "inline");
  AddFrameTypeInfo(nsGkAtoms::letterFrame,           "letter",    "letter");
  AddFrameTypeInfo(nsGkAtoms::lineFrame,             "line",      "line");
  AddFrameTypeInfo(nsGkAtoms::listControlFrame,      "select",    "select");
  AddFrameTypeInfo(nsGkAtoms::objectFrame,           "obj",       "object");
  AddFrameTypeInfo(nsGkAtoms::pageFrame,             "page",      "page");
  AddFrameTypeInfo(nsGkAtoms::placeholderFrame,      "place",     "placeholder");
  AddFrameTypeInfo(nsGkAtoms::positionedInlineFrame, "posInline", "positionedInline");
  AddFrameTypeInfo(nsGkAtoms::canvasFrame,           "canvas",    "canvas");
  AddFrameTypeInfo(nsGkAtoms::rootFrame,             "root",      "root");
  AddFrameTypeInfo(nsGkAtoms::scrollFrame,           "scroll",    "scroll");
  AddFrameTypeInfo(nsGkAtoms::tableCaptionFrame,     "caption",   "tableCaption");
  AddFrameTypeInfo(nsGkAtoms::tableCellFrame,        "cell",      "tableCell");
  AddFrameTypeInfo(nsGkAtoms::bcTableCellFrame,      "bcCell",    "bcTableCell");
  AddFrameTypeInfo(nsGkAtoms::tableColFrame,         "col",       "tableCol");
  AddFrameTypeInfo(nsGkAtoms::tableColGroupFrame,    "colG",      "tableColGroup");
  AddFrameTypeInfo(nsGkAtoms::tableFrame,            "tbl",       "table");
  AddFrameTypeInfo(nsGkAtoms::tableOuterFrame,       "tblO",      "tableOuter");
  AddFrameTypeInfo(nsGkAtoms::tableRowGroupFrame,    "rowG",      "tableRowGroup");
  AddFrameTypeInfo(nsGkAtoms::tableRowFrame,         "row",       "tableRow");
  AddFrameTypeInfo(nsGkAtoms::textInputFrame,        "textCtl",   "textInput");
  AddFrameTypeInfo(nsGkAtoms::textFrame,             "text",      "text");
  AddFrameTypeInfo(nsGkAtoms::viewportFrame,         "VP",        "viewport");
#ifdef MOZ_XUL
  AddFrameTypeInfo(nsGkAtoms::XULLabelFrame,         "XULLabel",  "XULLabel");
  AddFrameTypeInfo(nsGkAtoms::boxFrame,              "Box",       "Box");
  AddFrameTypeInfo(nsGkAtoms::sliderFrame,           "Slider",    "Slider");
  AddFrameTypeInfo(nsGkAtoms::popupSetFrame,         "PopupSet",  "PopupSet");
#endif
  AddFrameTypeInfo(nsnull,                               "unknown",   "unknown");
}


void DR_State::DisplayFrameTypeInfo(nsIFrame* aFrame,
                                    PRInt32   aIndent)
{ 
  DR_FrameTypeInfo* frameTypeInfo = GetFrameTypeInfo(aFrame->GetType());
  if (frameTypeInfo) {
    for (PRInt32 i = 0; i < aIndent; i++) {
      printf(" ");
    }
    if(!strcmp(frameTypeInfo->mNameAbbrev, "unknown")) {
      if (aFrame) {
       nsAutoString  name;
       aFrame->GetFrameName(name);
       printf("%s %p ", NS_LossyConvertUTF16toASCII(name).get(), (void*)aFrame);
      }
      else {
        printf("%s %p ", frameTypeInfo->mNameAbbrev, (void*)aFrame);
      }
    }
    else {
      printf("%s %p ", frameTypeInfo->mNameAbbrev, (void*)aFrame);
    }
  }
}

PRBool DR_State::RuleMatches(DR_Rule&          aRule,
                             DR_FrameTreeNode& aNode)
{
  NS_ASSERTION(aRule.mTarget, "program error");

  DR_RulePart* rulePart;
  DR_FrameTreeNode* parentNode;
  for (rulePart = aRule.mTarget->mNext, parentNode = aNode.mParent;
       rulePart && parentNode;
       rulePart = rulePart->mNext, parentNode = parentNode->mParent) {
    if (rulePart->mFrameType) {
      if (parentNode->mFrame) {
        if (rulePart->mFrameType != parentNode->mFrame->GetType()) {
          return PR_FALSE;
        }
      }
      else NS_ASSERTION(PR_FALSE, "program error");
    }
    
  }
  return PR_TRUE;
}

void DR_State::FindMatchingRule(DR_FrameTreeNode& aNode)
{
  if (!aNode.mFrame) {
    NS_ASSERTION(PR_FALSE, "invalid DR_FrameTreeNode \n");
    return;
  }

  PRBool matchingRule = PR_FALSE;

  DR_FrameTypeInfo* info = GetFrameTypeInfo(aNode.mFrame->GetType());
  NS_ASSERTION(info, "program error");
  PRInt32 numRules = info->mRules.Length();
  for (PRInt32 ruleX = 0; ruleX < numRules; ruleX++) {
    DR_Rule* rule = info->mRules.ElementAt(ruleX);
    if (rule && RuleMatches(*rule, aNode)) {
      aNode.mDisplay = rule->mDisplay;
      matchingRule = PR_TRUE;
      break;
    }
  }
  if (!matchingRule) {
    PRInt32 numWildRules = mWildRules.Length();
    for (PRInt32 ruleX = 0; ruleX < numWildRules; ruleX++) {
      DR_Rule* rule = mWildRules.ElementAt(ruleX);
      if (rule && RuleMatches(*rule, aNode)) {
        aNode.mDisplay = rule->mDisplay;
        break;
      }
    }
  }
}
    
DR_FrameTreeNode* DR_State::CreateTreeNode(nsIFrame*                aFrame,
                                           const nsHTMLReflowState* aReflowState)
{
  
  nsIFrame* parentFrame;
  if (aReflowState) {
    const nsHTMLReflowState* parentRS = aReflowState->parentReflowState;
    parentFrame = (parentRS) ? parentRS->frame : nsnull;
  } else {
    parentFrame = aFrame->GetParent();
  }

  
  DR_FrameTreeNode* parentNode = nsnull;
  
  DR_FrameTreeNode* lastLeaf = nsnull;
  if(mFrameTreeLeaves.Length())
    lastLeaf = mFrameTreeLeaves.ElementAt(mFrameTreeLeaves.Length() - 1);
  if (lastLeaf) {
    for (parentNode = lastLeaf; parentNode && (parentNode->mFrame != parentFrame); parentNode = parentNode->mParent) {
    }
  }
  DR_FrameTreeNode* newNode = new DR_FrameTreeNode(aFrame, parentNode);
  FindMatchingRule(*newNode);

  newNode->mIndent = mIndent;
  if (newNode->mDisplay || mIndentUndisplayedFrames) {
    ++mIndent;
  }

  if (lastLeaf && (lastLeaf == parentNode)) {
    mFrameTreeLeaves.RemoveElementAt(mFrameTreeLeaves.Length() - 1);
  }
  mFrameTreeLeaves.AppendElement(newNode);
  mCount++;

  return newNode;
}

void DR_State::PrettyUC(nscoord aSize,
                        char*   aBuf)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  }
  else {
    if ((nscoord)0xdeadbeefU == aSize)
    {
      strcpy(aBuf, "deadbeef");
    }
    else {
      sprintf(aBuf, "%d", aSize);
    }
  }
}

void DR_State::PrintMargin(const char *tag, const nsMargin* aMargin)
{
  if (aMargin) {
    char t[16], r[16], b[16], l[16];
    PrettyUC(aMargin->top, t);
    PrettyUC(aMargin->right, r);
    PrettyUC(aMargin->bottom, b);
    PrettyUC(aMargin->left, l);
    printf(" %s=%s,%s,%s,%s", tag, t, r, b, l);
  } else {
    
    printf(" %s=%p", tag, (void*)aMargin);
  }
}

void DR_State::DeleteTreeNode(DR_FrameTreeNode& aNode)
{
  mFrameTreeLeaves.RemoveElement(&aNode);
  PRInt32 numLeaves = mFrameTreeLeaves.Length();
  if ((0 == numLeaves) || (aNode.mParent != mFrameTreeLeaves.ElementAt(numLeaves - 1))) {
    mFrameTreeLeaves.AppendElement(aNode.mParent);
  }

  if (aNode.mDisplay || mIndentUndisplayedFrames) {
    --mIndent;
  }
  
  delete &aNode;
}

static void
CheckPixelError(nscoord aSize,
                PRInt32 aPixelToTwips)
{
  if (NS_UNCONSTRAINEDSIZE != aSize) {
    if ((aSize % aPixelToTwips) > 0) {
      printf("VALUE %d is not a whole pixel \n", aSize);
    }
  }
}

static void DisplayReflowEnterPrint(nsPresContext*          aPresContext,
                                    nsIFrame*                aFrame,
                                    const nsHTMLReflowState& aReflowState,
                                    DR_FrameTreeNode&        aTreeNode,
                                    PRBool                   aChanged)
{
  if (aTreeNode.mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, aTreeNode.mIndent);

    char width[16];
    char height[16];

    DR_state->PrettyUC(aReflowState.availableWidth, width);
    DR_state->PrettyUC(aReflowState.availableHeight, height);
    printf("Reflow a=%s,%s ", width, height);

    DR_state->PrettyUC(aReflowState.ComputedWidth(), width);
    DR_state->PrettyUC(aReflowState.ComputedHeight(), height);
    printf("c=%s,%s ", width, height);

    if (aFrame->GetStateBits() & NS_FRAME_IS_DIRTY)
      printf("dirty ");

    if (aFrame->GetStateBits() & NS_FRAME_HAS_DIRTY_CHILDREN)
      printf("dirty-children ");

    if (aReflowState.mFlags.mSpecialHeightReflow)
      printf("special-height ");

    if (aReflowState.mFlags.mHResize)
      printf("h-resize ");

    if (aReflowState.mFlags.mVResize)
      printf("v-resize ");

    nsIFrame* inFlow = aFrame->GetPrevInFlow();
    if (inFlow) {
      printf("pif=%p ", (void*)inFlow);
    }
    inFlow = aFrame->GetNextInFlow();
    if (inFlow) {
      printf("nif=%p ", (void*)inFlow);
    }
    if (aChanged) 
      printf("CHANGED \n");
    else 
      printf("cnt=%d \n", DR_state->mCount);
    if (DR_state->mDisplayPixelErrors) {
      PRInt32 p2t = aPresContext->AppUnitsPerDevPixel();
      CheckPixelError(aReflowState.availableWidth, p2t);
      CheckPixelError(aReflowState.availableHeight, p2t);
      CheckPixelError(aReflowState.ComputedWidth(), p2t);
      CheckPixelError(aReflowState.ComputedHeight(), p2t);
    }
  }
}

void* nsFrame::DisplayReflowEnter(nsPresContext*          aPresContext,
                                  nsIFrame*                aFrame,
                                  const nsHTMLReflowState& aReflowState)
{
  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, &aReflowState);
  if (treeNode) {
    DisplayReflowEnterPrint(aPresContext, aFrame, aReflowState, *treeNode, PR_FALSE);
  }
  return treeNode;
}

void* nsFrame::DisplayLayoutEnter(nsIFrame* aFrame)
{
  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nsnull);
  if (treeNode && treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    printf("Layout\n");
  }
  return treeNode;
}

void* nsFrame::DisplayIntrinsicWidthEnter(nsIFrame* aFrame,
                                          const char* aType)
{
  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nsnull);
  if (treeNode && treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    printf("Get%sWidth\n", aType);
  }
  return treeNode;
}

void* nsFrame::DisplayIntrinsicSizeEnter(nsIFrame* aFrame,
                                         const char* aType)
{
  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nsnull);
  if (treeNode && treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    printf("Get%sSize\n", aType);
  }
  return treeNode;
}

void nsFrame::DisplayReflowExit(nsPresContext*      aPresContext,
                                nsIFrame*            aFrame,
                                nsHTMLReflowMetrics& aMetrics,
                                nsReflowStatus       aStatus,
                                void*                aFrameTreeNode)
{
  if (!DR_state->mActive) return;

  NS_ASSERTION(aFrame, "DisplayReflowExit - invalid call");
  if (!aFrameTreeNode) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aFrameTreeNode;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);

    char width[16];
    char height[16];
    char x[16];
    char y[16];
    DR_state->PrettyUC(aMetrics.width, width);
    DR_state->PrettyUC(aMetrics.height, height);
    printf("Reflow d=%s,%s", width, height);

    if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
      printf(" status=0x%x", aStatus);
    }
    if (aFrame->HasOverflowAreas()) {
      DR_state->PrettyUC(aMetrics.VisualOverflow().x, x);
      DR_state->PrettyUC(aMetrics.VisualOverflow().y, y);
      DR_state->PrettyUC(aMetrics.VisualOverflow().width, width);
      DR_state->PrettyUC(aMetrics.VisualOverflow().height, height);
      printf(" vis-o=(%s,%s) %s x %s", x, y, width, height);

      nsRect storedOverflow = aFrame->GetVisualOverflowRect();
      DR_state->PrettyUC(storedOverflow.x, x);
      DR_state->PrettyUC(storedOverflow.y, y);
      DR_state->PrettyUC(storedOverflow.width, width);
      DR_state->PrettyUC(storedOverflow.height, height);
      printf(" vis-sto=(%s,%s) %s x %s", x, y, width, height);

      DR_state->PrettyUC(aMetrics.ScrollableOverflow().x, x);
      DR_state->PrettyUC(aMetrics.ScrollableOverflow().y, y);
      DR_state->PrettyUC(aMetrics.ScrollableOverflow().width, width);
      DR_state->PrettyUC(aMetrics.ScrollableOverflow().height, height);
      printf(" scr-o=(%s,%s) %s x %s", x, y, width, height);

      storedOverflow = aFrame->GetScrollableOverflowRect();
      DR_state->PrettyUC(storedOverflow.x, x);
      DR_state->PrettyUC(storedOverflow.y, y);
      DR_state->PrettyUC(storedOverflow.width, width);
      DR_state->PrettyUC(storedOverflow.height, height);
      printf(" scr-sto=(%s,%s) %s x %s", x, y, width, height);
    }
    printf("\n");
    if (DR_state->mDisplayPixelErrors) {
      PRInt32 p2t = aPresContext->AppUnitsPerDevPixel();
      CheckPixelError(aMetrics.width, p2t);
      CheckPixelError(aMetrics.height, p2t);
    }
  }
  DR_state->DeleteTreeNode(*treeNode);
}

void nsFrame::DisplayLayoutExit(nsIFrame*            aFrame,
                                void*                aFrameTreeNode)
{
  if (!DR_state->mActive) return;

  NS_ASSERTION(aFrame, "non-null frame required");
  if (!aFrameTreeNode) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aFrameTreeNode;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    nsRect rect = aFrame->GetRect();
    printf("Layout=%d,%d,%d,%d\n", rect.x, rect.y, rect.width, rect.height);
  }
  DR_state->DeleteTreeNode(*treeNode);
}

void nsFrame::DisplayIntrinsicWidthExit(nsIFrame*            aFrame,
                                        const char*          aType,
                                        nscoord              aResult,
                                        void*                aFrameTreeNode)
{
  if (!DR_state->mActive) return;

  NS_ASSERTION(aFrame, "non-null frame required");
  if (!aFrameTreeNode) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aFrameTreeNode;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    char width[16];
    DR_state->PrettyUC(aResult, width);
    printf("Get%sWidth=%s\n", aType, width);
  }
  DR_state->DeleteTreeNode(*treeNode);
}

void nsFrame::DisplayIntrinsicSizeExit(nsIFrame*            aFrame,
                                       const char*          aType,
                                       nsSize               aResult,
                                       void*                aFrameTreeNode)
{
  if (!DR_state->mActive) return;

  NS_ASSERTION(aFrame, "non-null frame required");
  if (!aFrameTreeNode) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aFrameTreeNode;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);

    char width[16];
    char height[16];
    DR_state->PrettyUC(aResult.width, width);
    DR_state->PrettyUC(aResult.height, height);
    printf("Get%sSize=%s,%s\n", aType, width, height);
  }
  DR_state->DeleteTreeNode(*treeNode);
}

 void
nsFrame::DisplayReflowStartup()
{
  DR_state = new DR_State();
}

 void
nsFrame::DisplayReflowShutdown()
{
  delete DR_state;
  DR_state = nsnull;
}

void DR_cookie::Change() const
{
  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)mValue;
  if (treeNode && treeNode->mDisplay) {
    DisplayReflowEnterPrint(mPresContext, mFrame, mReflowState, *treeNode, PR_TRUE);
  }
}

 void*
nsHTMLReflowState::DisplayInitConstraintsEnter(nsIFrame* aFrame,
                                               nsHTMLReflowState* aState,
                                               nscoord aContainingBlockWidth,
                                               nscoord aContainingBlockHeight,
                                               const nsMargin* aBorder,
                                               const nsMargin* aPadding)
{
  NS_PRECONDITION(aFrame, "non-null frame required");
  NS_PRECONDITION(aState, "non-null state required");

  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, aState);
  if (treeNode && treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);

    printf("InitConstraints parent=%p",
           (void*)aState->parentReflowState);

    char width[16];
    char height[16];

    DR_state->PrettyUC(aContainingBlockWidth, width);
    DR_state->PrettyUC(aContainingBlockHeight, height);
    printf(" cb=%s,%s", width, height);

    DR_state->PrettyUC(aState->availableWidth, width);
    DR_state->PrettyUC(aState->availableHeight, height);
    printf(" as=%s,%s", width, height);

    DR_state->PrintMargin("b", aBorder);
    DR_state->PrintMargin("p", aPadding);
    putchar('\n');
  }
  return treeNode;
}

 void
nsHTMLReflowState::DisplayInitConstraintsExit(nsIFrame* aFrame,
                                              nsHTMLReflowState* aState,
                                              void* aValue)
{
  NS_PRECONDITION(aFrame, "non-null frame required");
  NS_PRECONDITION(aState, "non-null state required");

  if (!DR_state->mActive) return;
  if (!aValue) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aValue;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    char cmiw[16], cw[16], cmxw[16], cmih[16], ch[16], cmxh[16];
    DR_state->PrettyUC(aState->mComputedMinWidth, cmiw);
    DR_state->PrettyUC(aState->mComputedWidth, cw);
    DR_state->PrettyUC(aState->mComputedMaxWidth, cmxw);
    DR_state->PrettyUC(aState->mComputedMinHeight, cmih);
    DR_state->PrettyUC(aState->mComputedHeight, ch);
    DR_state->PrettyUC(aState->mComputedMaxHeight, cmxh);
    printf("InitConstraints= cw=(%s <= %s <= %s) ch=(%s <= %s <= %s)",
           cmiw, cw, cmxw, cmih, ch, cmxh);
    DR_state->PrintMargin("co", &aState->mComputedOffsets);
    putchar('\n');
  }
  DR_state->DeleteTreeNode(*treeNode);
}


 void*
nsCSSOffsetState::DisplayInitOffsetsEnter(nsIFrame* aFrame,
                                          nsCSSOffsetState* aState,
                                          nscoord aContainingBlockWidth,
                                          const nsMargin* aBorder,
                                          const nsMargin* aPadding)
{
  NS_PRECONDITION(aFrame, "non-null frame required");
  NS_PRECONDITION(aState, "non-null state required");

  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  
  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nsnull);
  if (treeNode && treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);

    char width[16];
    DR_state->PrettyUC(aContainingBlockWidth, width);
    printf("InitOffsets cbw=%s", width);
    DR_state->PrintMargin("b", aBorder);
    DR_state->PrintMargin("p", aPadding);
    putchar('\n');
  }
  return treeNode;
}

 void
nsCSSOffsetState::DisplayInitOffsetsExit(nsIFrame* aFrame,
                                         nsCSSOffsetState* aState,
                                         void* aValue)
{
  NS_PRECONDITION(aFrame, "non-null frame required");
  NS_PRECONDITION(aState, "non-null state required");

  if (!DR_state->mActive) return;
  if (!aValue) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aValue;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    printf("InitOffsets=");
    DR_state->PrintMargin("m", &aState->mComputedMargin);
    DR_state->PrintMargin("p", &aState->mComputedPadding);
    DR_state->PrintMargin("p+b", &aState->mComputedBorderPadding);
    putchar('\n');
  }
  DR_state->DeleteTreeNode(*treeNode);
}

 void*
nsHTMLReflowState::DisplayInitFrameTypeEnter(nsIFrame* aFrame,
                                             nsHTMLReflowState* aState)
{
  NS_PRECONDITION(aFrame, "non-null frame required");
  NS_PRECONDITION(aState, "non-null state required");

  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nsnull;

  
  return DR_state->CreateTreeNode(aFrame, aState);
}

 void
nsHTMLReflowState::DisplayInitFrameTypeExit(nsIFrame* aFrame,
                                            nsHTMLReflowState* aState,
                                            void* aValue)
{
  NS_PRECONDITION(aFrame, "non-null frame required");
  NS_PRECONDITION(aState, "non-null state required");

  if (!DR_state->mActive) return;
  if (!aValue) return;

  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)aValue;
  if (treeNode->mDisplay) {
    DR_state->DisplayFrameTypeInfo(aFrame, treeNode->mIndent);
    printf("InitFrameType");

    const nsStyleDisplay *disp = aState->mStyleDisplay;

    if (aFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
      printf(" out-of-flow");
    if (aFrame->GetPrevInFlow())
      printf(" prev-in-flow");
    if (disp->IsAbsolutelyPositioned())
      printf(" abspos");
    if (disp->IsFloating())
      printf(" float");

    
    const char *const displayTypes[] = {
      "none", "block", "inline", "inline-block", "list-item", "marker",
      "run-in", "compact", "table", "inline-table", "table-row-group",
      "table-column", "table-column-group", "table-header-group",
      "table-footer-group", "table-row", "table-cell", "table-caption",
      "box", "inline-box",
#ifdef MOZ_XUL
      "grid", "inline-grid", "grid-group", "grid-line", "stack",
      "inline-stack", "deck", "popup", "groupbox",
#endif
    };
    if (disp->mDisplay >= NS_ARRAY_LENGTH(displayTypes))
      printf(" display=%u", disp->mDisplay);
    else
      printf(" display=%s", displayTypes[disp->mDisplay]);

    
    const char *const cssFrameTypes[] = {
      "unknown", "inline", "block", "floating", "absolute", "internal-table"
    };
    nsCSSFrameType bareType = NS_FRAME_GET_TYPE(aState->mFrameType);
    bool repNoBlock = NS_FRAME_IS_REPLACED_NOBLOCK(aState->mFrameType);
    bool repBlock = NS_FRAME_IS_REPLACED_CONTAINS_BLOCK(aState->mFrameType);

    if (bareType >= NS_ARRAY_LENGTH(cssFrameTypes)) {
      printf(" result=type %u", bareType);
    } else {
      printf(" result=%s", cssFrameTypes[bareType]);
    }
    printf("%s%s\n", repNoBlock ? " +rep" : "", repBlock ? " +repBlk" : "");
  }
  DR_state->DeleteTreeNode(*treeNode);
}

#endif


#endif
