







#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

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
#include "nsTableOuterFrame.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsIScrollableFrame.h"
#include "nsPresContext.h"
#include "nsCRT.h"
#include "nsGUIEvent.h"
#include "nsIDOMEvent.h"
#include "nsAsyncDOMEvent.h"
#include "nsStyleConsts.h"
#include "nsIPresShell.h"
#include "prlog.h"
#include "prprf.h"
#include <stdarg.h>
#include "nsFrameManager.h"
#include "nsCSSRendering.h"
#include "nsLayoutUtils.h"

#include "nsIDOMNode.h"
#include "nsIEditorDocShell.h"
#include "nsEventStateManager.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsFrameSelection.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSFrameConstructor.h"

#include "nsFrameTraversal.h"
#include "nsStyleChangeList.h"
#include "nsIDOMRange.h"
#include "nsRange.h"
#include "nsITableCellLayout.h"
#include "nsITextControlFrame.h"
#include "nsINameSpaceManager.h"
#include "nsIPercentHeightObserver.h"
#include "nsStyleStructInlines.h"
#include <algorithm>

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif


#include "nsIServiceManager.h"
#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsLayoutCID.h"
#include "nsUnicharUtils.h"
#include "nsError.h"
#include "nsContainerFrame.h"
#include "nsBoxLayoutState.h"
#include "nsBlockFrame.h"
#include "nsDisplayList.h"
#include "nsIObjectLoadingContent.h"
#include "nsExpirationTracker.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGEffects.h"
#include "nsChangeHint.h"
#include "nsDeckFrame.h"
#include "nsTableFrame.h"
#include "nsSubDocumentFrame.h"

#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "CSSCalc.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsFontInflationData.h"
#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/css/ImageLoader.h"
#include "mozilla/gfx/Tools.h"

using namespace mozilla;
using namespace mozilla::layers;
using namespace mozilla::layout;
using namespace mozilla::css;


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
  int32_t mOffset;
};


#define SELECTION_DEBUG        0
#define FORCE_SELECTION_UPDATE 1
#define CALC_DEBUG             0


#include "nsILineIterator.h"


#if 0
static void RefreshContentFrames(nsPresContext* aPresContext, nsIContent * aStartContent, nsIContent * aEndContent);
#endif

#include "prenv.h"



#ifdef DEBUG
static bool gShowFrameBorders = false;

void nsFrame::ShowFrameBorders(bool aEnable)
{
  gShowFrameBorders = aEnable;
}

bool nsFrame::GetShowFrameBorders()
{
  return gShowFrameBorders;
}

static bool gShowEventTargetFrameBorder = false;

void nsFrame::ShowEventTargetFrameBorder(bool aEnable)
{
  gShowEventTargetFrameBorder = aEnable;
}

bool nsFrame::GetShowEventTargetFrameBorder()
{
  return gShowEventTargetFrameBorder;
}





static PRLogModuleInfo* gLogModule;

static PRLogModuleInfo* gStyleVerifyTreeLogModuleInfo;

static uint32_t gStyleVerifyTreeEnable = 0x55;

bool
nsFrame::GetVerifyStyleTreeEnable()
{
  if (gStyleVerifyTreeEnable == 0x55) {
    if (nullptr == gStyleVerifyTreeLogModuleInfo) {
      gStyleVerifyTreeLogModuleInfo = PR_NewLogModule("styleverifytree");
      gStyleVerifyTreeEnable = 0 != gStyleVerifyTreeLogModuleInfo->level;
    }
  }
  return gStyleVerifyTreeEnable;
}

void
nsFrame::SetVerifyStyleTreeEnable(bool aEnabled)
{
  gStyleVerifyTreeEnable = aEnabled;
}

PRLogModuleInfo*
nsFrame::GetLogModuleInfo()
{
  if (nullptr == gLogModule) {
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
nsFrame::RootFrameList(nsPresContext* aPresContext, FILE* out, int32_t aIndent)
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

static void
DestroyAbsoluteContainingBlock(void* aPropertyValue)
{
  delete static_cast<nsAbsoluteContainingBlock*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(AbsoluteContainingBlockProperty, DestroyAbsoluteContainingBlock)

bool
nsIFrame::HasAbsolutelyPositionedChildren() const {
  return IsAbsoluteContainer() && GetAbsoluteContainingBlock()->HasAbsoluteFrames();
}

nsAbsoluteContainingBlock*
nsIFrame::GetAbsoluteContainingBlock() const {
  NS_ASSERTION(IsAbsoluteContainer(), "The frame is not marked as an abspos container correctly");
  nsAbsoluteContainingBlock* absCB = static_cast<nsAbsoluteContainingBlock*>
    (Properties().Get(AbsoluteContainingBlockProperty()));
  NS_ASSERTION(absCB, "The frame is marked as an abspos container but doesn't have the property");
  return absCB;
}

void
nsIFrame::MarkAsAbsoluteContainingBlock()
{
  MOZ_ASSERT(GetStateBits() & NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  NS_ASSERTION(!Properties().Get(AbsoluteContainingBlockProperty()),
               "Already has an abs-pos containing block property?");
  NS_ASSERTION(!HasAnyStateBits(NS_FRAME_HAS_ABSPOS_CHILDREN),
               "Already has NS_FRAME_HAS_ABSPOS_CHILDREN state bit?");
  AddStateBits(NS_FRAME_HAS_ABSPOS_CHILDREN);
  Properties().Set(AbsoluteContainingBlockProperty(), new nsAbsoluteContainingBlock(GetAbsoluteListID()));
}

void
nsIFrame::MarkAsNotAbsoluteContainingBlock()
{
  NS_ASSERTION(!HasAbsolutelyPositionedChildren(), "Think of the children!");
  NS_ASSERTION(Properties().Get(AbsoluteContainingBlockProperty()),
               "Should have an abs-pos containing block property");
  NS_ASSERTION(HasAnyStateBits(NS_FRAME_HAS_ABSPOS_CHILDREN),
               "Should have NS_FRAME_HAS_ABSPOS_CHILDREN state bit");
  MOZ_ASSERT(HasAnyStateBits(NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN));
  RemoveStateBits(NS_FRAME_HAS_ABSPOS_CHILDREN);
  Properties().Delete(AbsoluteContainingBlockProperty());
}

bool
nsIFrame::CheckAndClearPaintedState()
{
  bool result = (GetStateBits() & NS_FRAME_PAINTED_THEBES);
  RemoveStateBits(NS_FRAME_PAINTED_THEBES);
  
  nsIFrame::ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if (child->CheckAndClearPaintedState()) {
        result = true;
      }
    }
  }
  return result;
}

bool
nsIFrame::IsVisibleConsideringAncestors(uint32_t aFlags) const
{
  if (!GetStyleVisibility()->IsVisible()) {
    return false;
  }

  const nsIFrame* frame = this;
  while (frame) {
    nsView* view = frame->GetView();
    if (view && view->GetVisibility() == nsViewVisibility_kHide)
      return false;
    
    nsIFrame* parent = frame->GetParent();
    nsDeckFrame* deck = do_QueryFrame(parent);
    if (deck) {
      if (deck->GetSelectedBox() != frame)
        return false;
    }

    if (parent) {
      frame = parent;
    } else {
      parent = nsLayoutUtils::GetCrossDocParentFrame(frame);
      if (!parent)
        break;

      if ((aFlags & nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY) == 0 &&
          parent->PresContext()->IsChrome() && !frame->PresContext()->IsChrome()) {
        break;
      }

      if (!parent->GetStyleVisibility()->IsVisible())
        return false;

      frame = parent;
    }
  }

  return true;
}

void
nsIFrame::FindCloserFrameForSelection(
                                 nsPoint aPoint,
                                 nsIFrame::FrameWithDistance* aCurrentBestFrame)
{
  if (nsLayoutUtils::PointIsCloserToRect(aPoint, mRect,
                                         aCurrentBestFrame->mXDistance,
                                         aCurrentBestFrame->mYDistance)) {
    aCurrentBestFrame->mFrame = this;
  }
}

static bool ApplyOverflowClipping(nsDisplayListBuilder* aBuilder,
                                    const nsIFrame* aFrame,
                                    const nsStyleDisplay* aDisp, 
                                    nsRect* aRect);

static bool ApplyClipPropClipping(nsDisplayListBuilder* aBuilder,
                                  const nsStyleDisplay* aDisp, 
                                  const nsIFrame* aFrame,
                                  nsRect* aRect);

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
  Clear(mFrame ? mFrame->PresContext()->GetPresShell() : nullptr);
  mFrame = aFrame;
  if (mFrame) {
    nsIPresShell* shell = mFrame->PresContext()->GetPresShell();
    NS_WARN_IF_FALSE(shell, "Null PresShell in nsWeakFrame!");
    if (shell) {
      shell->AddWeakFrame(this);
    } else {
      mFrame = nullptr;
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




static bool
IsFontSizeInflationContainer(nsIFrame* aFrame,
                             const nsStyleDisplay* aStyleDisplay)
{
  
































  
  if (!aFrame->GetParent()) {
    return true;
  }

  nsIContent *content = aFrame->GetContent();
  bool isInline = (aFrame->GetDisplay() == NS_STYLE_DISPLAY_INLINE ||
                   (aFrame->IsFloating() &&
                    aFrame->GetType() == nsGkAtoms::letterFrame) ||
                   
                   
                   
                   (aFrame->GetParent()->GetContent() == content) ||
                   (content && (content->IsHTML(nsGkAtoms::option) ||
                                content->IsHTML(nsGkAtoms::optgroup) ||
                                content->IsHTML(nsGkAtoms::select) ||
                                content->IsInNativeAnonymousSubtree()))) &&
                  !(aFrame->IsBoxFrame() && aFrame->GetParent()->IsBoxFrame());
  NS_ASSERTION(!aFrame->IsFrameOfType(nsIFrame::eLineParticipant) ||
               isInline ||
               
               
               
               aFrame->GetType() == nsGkAtoms::brFrame ||
               aFrame->IsFrameOfType(nsIFrame::eMathML),
               "line participants must not be containers");
  NS_ASSERTION(aFrame->GetType() != nsGkAtoms::bulletFrame || isInline,
               "bullets should not be containers");
  return !isInline;
}

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

    
    mState |= state & (NS_FRAME_INDEPENDENT_SELECTION |
                       NS_FRAME_IS_SPECIAL |
                       NS_FRAME_MAY_BE_TRANSFORMED |
                       NS_FRAME_MAY_HAVE_GENERATED_CONTENT |
                       NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  }
  if (mParent) {
    nsFrameState state = mParent->GetStateBits();

    
    mState |= state & (NS_FRAME_INDEPENDENT_SELECTION |
                       NS_FRAME_GENERATED_CONTENT |
                       NS_FRAME_IS_SVG_TEXT |
                       NS_FRAME_IN_POPUP);
  }
  const nsStyleDisplay *disp = GetStyleDisplay();
  if (disp->HasTransform(this)) {
    
    
    mState |= NS_FRAME_MAY_BE_TRANSFORMED;
  }

  if (nsLayoutUtils::FontSizeInflationEnabled(PresContext()) || !GetParent()
#ifdef DEBUG
      
      
      || true
#endif
      ) {
    if (IsFontSizeInflationContainer(this, disp)) {
      AddStateBits(NS_FRAME_FONT_INFLATION_CONTAINER);
      if (!GetParent() ||
          
          disp->IsFloating(this) || disp->IsAbsolutelyPositioned(this)) {
        AddStateBits(NS_FRAME_FONT_INFLATION_FLOW_ROOT);
      }
    }
    NS_ASSERTION(GetParent() ||
                 (GetStateBits() & NS_FRAME_FONT_INFLATION_CONTAINER),
                 "root frame should always be a container");
  }

  DidSetStyleContext(nullptr);

  if (IsBoxWrapped())
    InitBoxMetrics(false);

  return NS_OK;
}

NS_IMETHODIMP nsFrame::SetInitialChildList(ChildListID     aListID,
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
nsFrame::AppendFrames(ChildListID     aListID,
                      nsFrameList&    aFrameList)
{
  NS_PRECONDITION(false, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::InsertFrames(ChildListID     aListID,
                      nsIFrame*       aPrevFrame,
                      nsFrameList&    aFrameList)
{
  NS_PRECONDITION(false, "not a container");
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrame::RemoveFrame(ChildListID     aListID,
                     nsIFrame*       aOldFrame)
{
  NS_PRECONDITION(false, "not a container");
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
  MOZ_ASSERT(!HasAbsolutelyPositionedChildren());

  nsSVGEffects::InvalidateDirectRenderingObservers(this);

  
  
  nsView* view = GetView();
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
      placeholder->SetOutOfFlowFrame(nullptr);
    }
  }

  
  
  
  if (mState & NS_FRAME_IS_SPECIAL) {
    
    nsIFrame* prevSib = static_cast<nsIFrame*>
      (Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
    if (prevSib) {
      NS_WARN_IF_FALSE(this ==
         prevSib->Properties().Get(nsIFrame::IBSplitSpecialSibling()),
         "IB sibling chain is inconsistent");
      prevSib->Properties().Delete(nsIFrame::IBSplitSpecialSibling());
    }

    
    nsIFrame* nextSib = static_cast<nsIFrame*>
      (Properties().Get(nsIFrame::IBSplitSpecialSibling()));
    if (nextSib) {
      NS_WARN_IF_FALSE(this ==
         nextSib->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()),
         "IB sibling chain is inconsistent");
      nextSib->Properties().Delete(nsIFrame::IBSplitSpecialPrevSibling());
    }
  }

  shell->NotifyDestroyingFrame(this);

  if (mState & NS_FRAME_EXTERNAL_REFERENCE) {
    shell->ClearFrameRefs(this);
  }

  if (view) {
    
    view->SetFrame(nullptr);

    
    view->Destroy();
  }

  
  if (mContent && mContent->GetPrimaryFrame() == this) {
    mContent->SetPrimaryFrame(nullptr);
  }

  
  
  
  
  
  
  

  nsQueryFrame::FrameIID id = GetFrameId();
  this->~nsFrame();

  
  
  shell->FreeFrame(id, this);
}

NS_IMETHODIMP
nsFrame::GetOffsets(int32_t &aStart, int32_t &aEnd) const
{
  aStart = 0;
  aEnd = 0;
  return NS_OK;
}

static bool
EqualImages(imgIRequest *aOldImage, imgIRequest *aNewImage)
{
  if (aOldImage == aNewImage)
    return true;

  if (!aOldImage || !aNewImage)
    return false;

  nsCOMPtr<nsIURI> oldURI, newURI;
  aOldImage->GetURI(getter_AddRefs(oldURI));
  aNewImage->GetURI(getter_AddRefs(newURI));
  bool equal;
  return NS_SUCCEEDED(oldURI->Equals(newURI, &equal)) && equal;
}


 void
nsFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  ImageLoader* imageLoader = PresContext()->Document()->StyleImageLoader();

  
  
  
  
  
  
  
  
  const nsStyleBackground *oldBG = aOldStyleContext ?
                                   aOldStyleContext->GetStyleBackground() :
                                   nullptr;
  const nsStyleBackground *newBG = GetStyleBackground();
  if (oldBG) {
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, oldBG) {
      
      if (i >= newBG->mImageCount ||
          oldBG->mLayers[i].mImage != newBG->mLayers[i].mImage) {
        const nsStyleImage& oldImage = oldBG->mLayers[i].mImage;
        if (oldImage.GetType() != eStyleImageType_Image) {
          continue;
        }

        imageLoader->DisassociateRequestFromFrame(oldImage.GetImageData(),
                                                  this);
      }          
    }
  }

  NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, newBG) {
    
    if (!oldBG || i >= oldBG->mImageCount ||
        newBG->mLayers[i].mImage != oldBG->mLayers[i].mImage) {
      const nsStyleImage& newImage = newBG->mLayers[i].mImage;
      if (newImage.GetType() != eStyleImageType_Image) {
        continue;
      }

      imageLoader->AssociateRequestToFrame(newImage.GetImageData(), this);
    }          
  }

  if (aOldStyleContext) {
    
    
    
    
    
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
      oldValue = oldBorder->GetComputedBorder();
      newValue = GetStyleBorder()->GetComputedBorder();
      if (oldValue != newValue &&
          !props.Get(UsedBorderProperty())) {
        props.Set(UsedBorderProperty(), new nsMargin(oldValue));
      }
    }
  }

  imgIRequest *oldBorderImage = aOldStyleContext
    ? aOldStyleContext->GetStyleBorder()->GetBorderImage()
    : nullptr;
  imgIRequest *newBorderImage = GetStyleBorder()->GetBorderImage();
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (!EqualImages(oldBorderImage, newBorderImage)) {
    
    if (oldBorderImage) {
      imageLoader->DisassociateRequestFromFrame(oldBorderImage, this);
    }
    if (newBorderImage) {
      imageLoader->AssociateRequestToFrame(newBorderImage, this);
    }
  }

  
  
  
  
  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    PresContext()->SetBidiEnabled();
  }
}




#ifndef _MSC_VER

const nsIFrame::ChildListID nsIFrame::kPrincipalList;
const nsIFrame::ChildListID nsIFrame::kAbsoluteList;
const nsIFrame::ChildListID nsIFrame::kBulletList;
const nsIFrame::ChildListID nsIFrame::kCaptionList;
const nsIFrame::ChildListID nsIFrame::kColGroupList;
const nsIFrame::ChildListID nsIFrame::kExcessOverflowContainersList;
const nsIFrame::ChildListID nsIFrame::kFixedList;
const nsIFrame::ChildListID nsIFrame::kFloatList;
const nsIFrame::ChildListID nsIFrame::kOverflowContainersList;
const nsIFrame::ChildListID nsIFrame::kOverflowList;
const nsIFrame::ChildListID nsIFrame::kOverflowOutOfFlowList;
const nsIFrame::ChildListID nsIFrame::kPopupList;
const nsIFrame::ChildListID nsIFrame::kPushedFloatsList;
const nsIFrame::ChildListID nsIFrame::kSelectPopupList;
const nsIFrame::ChildListID nsIFrame::kNoReflowPrincipalList;
#endif

 nsMargin
nsIFrame::GetUsedMargin() const
{
  nsMargin margin(0, 0, 0, 0);
  if (((mState & NS_FRAME_FIRST_REFLOW) &&
       !(mState & NS_FRAME_IN_REFLOW)) ||
      (mState & NS_FRAME_IS_SVG_TEXT))
    return margin;

  nsMargin *m = static_cast<nsMargin*>
                           (Properties().Get(UsedMarginProperty()));
  if (m) {
    margin = *m;
  } else {
#ifdef DEBUG
    bool hasMargin = 
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
  if (((mState & NS_FRAME_FIRST_REFLOW) &&
       !(mState & NS_FRAME_IN_REFLOW)) ||
      (mState & NS_FRAME_IS_SVG_TEXT))
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
    border = GetStyleBorder()->GetComputedBorder();
  }
  return border;
}

 nsMargin
nsIFrame::GetUsedPadding() const
{
  nsMargin padding(0, 0, 0, 0);
  if (((mState & NS_FRAME_FIRST_REFLOW) &&
       !(mState & NS_FRAME_IN_REFLOW)) ||
      (mState & NS_FRAME_IS_SVG_TEXT))
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
    bool hasPadding = 
#endif
    GetStylePadding()->GetPadding(padding);
    NS_ASSERTION(hasPadding, "We should have padding here! (out of memory?)");
  }
  return padding;
}

void
nsIFrame::ApplySkipSides(nsMargin& aMargin) const
{
  int skipSides = GetSkipSides();
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

bool
nsIFrame::IsTransformed() const
{
  return ((mState & NS_FRAME_MAY_BE_TRANSFORMED) &&
          (GetStyleDisplay()->HasTransform(this) ||
           IsSVGTransformed() ||
           (mContent &&
            nsLayoutUtils::HasAnimationsForCompositor(mContent,
                                                      eCSSProperty_transform) &&
            IsFrameOfType(eSupportsCSSTransforms) &&
            mContent->GetPrimaryFrame() == this)));
}

bool
nsIFrame::HasOpacity() const
{
  return GetStyleDisplay()->mOpacity < 1.0f || (mContent &&
           nsLayoutUtils::HasAnimationsForCompositor(mContent,
                                                     eCSSProperty_opacity) &&
           mContent->GetPrimaryFrame() == this);
}

bool
nsIFrame::IsSVGTransformed(gfxMatrix *aOwnTransforms,
                           gfxMatrix *aFromParentTransforms) const
{
  return false;
}

bool
nsIFrame::Preserves3DChildren() const
{
  if (GetStyleDisplay()->mTransformStyle != NS_STYLE_TRANSFORM_STYLE_PRESERVE_3D ||
      !GetStyleDisplay()->HasTransform(this))
      return false;

  
  if (GetType() == nsGkAtoms::scrollFrame)
      return false;

  nsRect temp;
  return (!ApplyOverflowClipping(nullptr, this, GetStyleDisplay(), &temp) &&
      !ApplyClipPropClipping(nullptr, GetStyleDisplay(), this, &temp) &&
      !nsSVGIntegrationUtils::UsingEffectsForFrame(this));
}

bool
nsIFrame::Preserves3D() const
{
  if (!GetParent() || !GetParent()->Preserves3DChildren() ||
      !GetStyleDisplay()->HasTransform(this)) {
    return false;
  }
  return true;
}

bool
nsIFrame::HasPerspective() const
{
  if (!IsTransformed()) {
    return false;
  }
  const nsStyleDisplay* parentDisp = nullptr;
  nsStyleContext* parentStyleContext = GetStyleContext()->GetParent();
  if (parentStyleContext) {
    parentDisp = parentStyleContext->GetStyleDisplay();
  }

  if (parentDisp &&
      parentDisp->mChildPerspective.GetUnit() == eStyleUnit_Coord &&
      parentDisp->mChildPerspective.GetCoordValue() > 0.0) {
    return true;
  }
  return false;
}

bool
nsIFrame::ChildrenHavePerspective() const
{
  const nsStyleDisplay *disp = GetStyleContext()->GetStyleDisplay();
  if (disp &&
      disp->mChildPerspective.GetUnit() == eStyleUnit_Coord &&
      disp->mChildPerspective.GetCoordValue() > 0.0) {
    return true;
  }
  return false;
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

bool
nsIFrame::ComputeBorderRadii(const nsStyleCorners& aBorderRadius,
                             const nsSize& aFrameSize,
                             const nsSize& aBorderArea,
                             int aSkipSides,
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

  
  
  bool haveRadius = false;
  double ratio = 1.0f;
  NS_FOR_CSS_SIDES(side) {
    uint32_t hc1 = NS_SIDE_TO_HALF_CORNER(side, false, true);
    uint32_t hc2 = NS_SIDE_TO_HALF_CORNER(side, true, true);
    nscoord length =
      NS_SIDE_IS_VERTICAL(side) ? aBorderArea.height : aBorderArea.width;
    nscoord sum = aRadii[hc1] + aRadii[hc2];
    if (sum)
      haveRadius = true;

    
    if (length < sum)
      ratio = std::min(ratio, double(length)/sum);
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
    nscoord offset = aOffsets.Side(side);
    uint32_t hc1 = NS_SIDE_TO_HALF_CORNER(side, false, false);
    uint32_t hc2 = NS_SIDE_TO_HALF_CORNER(side, true, false);
    aRadii[hc1] = std::max(0, aRadii[hc1] - offset);
    aRadii[hc2] = std::max(0, aRadii[hc2] - offset);
  }
}

 void
nsIFrame::OutsetBorderRadii(nscoord aRadii[8], const nsMargin &aOffsets)
{
  NS_FOR_CSS_SIDES(side) {
    nscoord offset = aOffsets.Side(side);
    uint32_t hc1 = NS_SIDE_TO_HALF_CORNER(side, false, false);
    uint32_t hc2 = NS_SIDE_TO_HALF_CORNER(side, true, false);
    if (aRadii[hc1] > 0)
      aRadii[hc1] += offset;
    if (aRadii[hc2] > 0)
      aRadii[hc2] += offset;
  }
}

 bool
nsIFrame::GetBorderRadii(nscoord aRadii[8]) const
{
  if (IsThemed()) {
    
    
    
    
    
    
    
    NS_FOR_CSS_HALF_CORNERS(corner) {
      aRadii[corner] = 0;
    }
    return false;
  }
  nsSize size = GetSize();
  return ComputeBorderRadii(GetStyleBorder()->mBorderRadius, size, size,
                            GetSkipSides(), aRadii);
}

bool
nsIFrame::GetPaddingBoxBorderRadii(nscoord aRadii[8]) const
{
  if (!GetBorderRadii(aRadii))
    return false;
  InsetBorderRadii(aRadii, GetUsedBorder());
  NS_FOR_CSS_HALF_CORNERS(corner) {
    if (aRadii[corner])
      return true;
  }
  return false;
}

bool
nsIFrame::GetContentBoxBorderRadii(nscoord aRadii[8]) const
{
  if (!GetBorderRadii(aRadii))
    return false;
  InsetBorderRadii(aRadii, GetUsedBorderAndPadding());
  NS_FOR_CSS_HALF_CORNERS(corner) {
    if (aRadii[corner])
      return true;
  }
  return false;
}

nsStyleContext*
nsFrame::GetAdditionalStyleContext(int32_t aIndex) const
{
  NS_PRECONDITION(aIndex >= 0, "invalid index number");
  return nullptr;
}

void
nsFrame::SetAdditionalStyleContext(int32_t aIndex, 
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

const nsFrameList&
nsFrame::GetChildList(ChildListID aListID) const
{
  if (IsAbsoluteContainer() &&
      aListID == GetAbsoluteListID()) {
    return GetAbsoluteContainingBlock()->GetChildList();
  } else {
    return nsFrameList::EmptyList();
  }
}

void
nsFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  if (IsAbsoluteContainer()) {
    nsFrameList absoluteList = GetAbsoluteContainingBlock()->GetChildList();
    absoluteList.AppendIfNonempty(aLists, GetAbsoluteListID());
  }
}

void
nsIFrame::GetCrossDocChildLists(nsTArray<ChildList>* aLists)
{
  nsSubDocumentFrame* subdocumentFrame = do_QueryFrame(this);
  if (subdocumentFrame) {
    
    nsIFrame* root = subdocumentFrame->GetSubdocumentRootFrame();
    if (root) {
      aLists->AppendElement(nsIFrame::ChildList(
        nsFrameList(root, nsLayoutUtils::GetLastSibling(root)),
        nsIFrame::kPrincipalList));
    }
  }

  GetChildLists(aLists);
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

int16_t
nsFrame::DisplaySelection(nsPresContext* aPresContext, bool isOkToTurnOn)
{
  int16_t selType = nsISelectionController::SELECTION_OFF;

  nsCOMPtr<nsISelectionController> selCon;
  nsresult result = GetSelectionController(aPresContext, getter_AddRefs(selCon));
  if (NS_SUCCEEDED(result) && selCon) {
    result = selCon->GetDisplaySelection(&selType);
    if (NS_SUCCEEDED(result) && (selType != nsISelectionController::SELECTION_OFF)) {
      
      bool selectable;
      IsSelectable(&selectable, nullptr);
      if (!selectable) {
        selType = nsISelectionController::SELECTION_OFF;
        isOkToTurnOn = false;
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
                            nsFrame* aFrame, int16_t aSelectionValue)
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
  int16_t mSelectionValue;
};

void nsDisplaySelectionOverlay::Paint(nsDisplayListBuilder* aBuilder,
                                      nsRenderingContext* aCtx)
{
  LookAndFeel::ColorID colorID;
  if (mSelectionValue == nsISelectionController::SELECTION_ON) {
    colorID = LookAndFeel::eColorID_TextSelectBackground;
  } else if (mSelectionValue == nsISelectionController::SELECTION_ATTENTION) {
    colorID = LookAndFeel::eColorID_TextSelectBackgroundAttention;
  } else {
    colorID = LookAndFeel::eColorID_TextSelectBackgroundDisabled;
  }

  nscolor color = LookAndFeel::GetColor(colorID, NS_RGB(255, 255, 255));

  gfxRGBA c(color);
  c.a = .5;

  gfxContext *ctx = aCtx->ThebesContext();
  ctx->SetColor(c);

  nsIntRect pxRect =
    mVisibleRect.ToOutsidePixels(mFrame->PresContext()->AppUnitsPerDevPixel());
  ctx->NewPath();
  ctx->Rectangle(gfxRect(pxRect.x, pxRect.y, pxRect.width, pxRect.height), true);
  ctx->Fill();
}





nsresult
nsFrame::DisplaySelectionOverlay(nsDisplayListBuilder*   aBuilder,
                                 nsDisplayList*          aList,
                                 uint16_t                aContentType)
{
  if (!IsSelected() || !IsVisibleForPainting(aBuilder))
    return NS_OK;
    
  nsPresContext* presContext = PresContext();
  nsIPresShell *shell = presContext->PresShell();
  if (!shell)
    return NS_OK;

  int16_t displaySelection = shell->GetSelectionFlags();
  if (!(displaySelection & aContentType))
    return NS_OK;

  const nsFrameSelection* frameSelection = GetConstFrameSelection();
  int16_t selectionValue = frameSelection->GetDisplaySelection();

  if (selectionValue <= nsISelectionController::SELECTION_HIDDEN)
    return NS_OK; 

  nsIContent *newContent = mContent->GetParent();

  
  int32_t offset = 0;
  if (newContent) {
    
    offset = newContent->IndexOf(mContent);
  }

  SelectionDetails *details;
  
  details = frameSelection->LookUpSelection(newContent, offset, 1, false);
  if (!details)
    return NS_OK;
  
  bool normal = false;
  while (details) {
    if (details->mType == nsISelectionController::SELECTION_NORMAL) {
      normal = true;
    }
    SelectionDetails *next = details->mNext;
    delete details;
    details = next;
  }

  if (!normal && aContentType == nsISelectionDisplay::DISPLAY_IMAGES) {
    
    return NS_OK;
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
nsIFrame::GetCaretColorAt(int32_t aOffset)
{
  
  return GetStyleColor()->mColor;
}

nsresult
nsFrame::DisplayBackgroundUnconditional(nsDisplayListBuilder*   aBuilder,
                                        const nsDisplayListSet& aLists,
                                        bool                    aForceBackground,
                                        nsDisplayBackgroundImage**   aBackground)
{
  *aBackground = nullptr;

  
  
  
  if (aBuilder->IsForEventDelivery() || aForceBackground ||
      !GetStyleBackground()->IsTransparent() || GetStyleDisplay()->mAppearance) {
    return nsDisplayBackgroundImage::AppendBackgroundItemsToTop(aBuilder, this,
                                                           aLists.BorderBackground(),
                                                           aBackground);
  }

  return NS_OK;
}

nsresult
nsFrame::DisplayBorderBackgroundOutline(nsDisplayListBuilder*   aBuilder,
                                        const nsDisplayListSet& aLists,
                                        bool                    aForceBackground)
{
  
  
  
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  nsCSSShadowArray* shadows = GetStyleBorder()->mBoxShadow;
  if (shadows && shadows->HasShadowWithInset(false)) {
    nsresult rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBoxShadowOuter(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsDisplayBackgroundImage* bg;
  nsresult rv =
    DisplayBackgroundUnconditional(aBuilder, aLists, aForceBackground, &bg);
  NS_ENSURE_SUCCESS(rv, rv);

  if (shadows && shadows->HasShadowWithInset(true)) {
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBoxShadowInner(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if ((!bg || !bg->IsThemed()) && GetStyleBorder()->HasBorder()) {
    rv = aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayBorder(aBuilder, this));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return DisplayOutlineUnconditional(aBuilder, aLists);
}

inline static bool IsSVGContentWithCSSClip(const nsIFrame *aFrame)
{
  
  
  
  
  
  nsIAtom *tag = aFrame->GetContent()->Tag();
  return (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) &&
    (tag == nsGkAtoms::svg || tag == nsGkAtoms::foreignObject);
}

bool
nsIFrame::GetClipPropClipRect(const nsStyleDisplay* aDisp, nsRect* aRect,
                              const nsSize& aSize) const
{
  NS_PRECONDITION(aRect, "Must have aRect out parameter");

  if (!(aDisp->mClipFlags & NS_STYLE_CLIP_RECT) ||
      !(aDisp->IsAbsolutelyPositioned(this) || IsSVGContentWithCSSClip(this))) {
    return false;
  }

  *aRect = aDisp->mClip;
  if (NS_STYLE_CLIP_RIGHT_AUTO & aDisp->mClipFlags) {
    aRect->width = aSize.width - aRect->x;
  }
  if (NS_STYLE_CLIP_BOTTOM_AUTO & aDisp->mClipFlags) {
    aRect->height = aSize.height - aRect->y;
  }
  return true;
}

static bool ApplyClipPropClipping(nsDisplayListBuilder* aBuilder,
                                  const nsStyleDisplay* aDisp, const nsIFrame* aFrame,
                                  nsRect* aRect) {
  if (!aFrame->GetClipPropClipRect(aDisp, aRect, aFrame->GetSize()))
    return false;

  if (aBuilder) {
    *aRect += aBuilder->ToReferenceFrame(aFrame);
  }
  return true;
}

static bool ApplyOverflowClipping(nsDisplayListBuilder* aBuilder,
                                  const nsIFrame* aFrame,
                                  const nsStyleDisplay* aDisp, nsRect* aRect) {
  
  
  
  

  
  
  
  
  
  
  if (!nsFrame::ApplyOverflowClipping(aFrame, aDisp)) {
    return false;
  }
  *aRect = aFrame->GetPaddingRect() - aFrame->GetPosition();
  if (aBuilder) {
    *aRect += aBuilder->ToReferenceFrame(aFrame);
  }
  return true;
}

class nsOverflowClipWrapper : public nsDisplayWrapper
{
public:
  






  nsOverflowClipWrapper(nsIFrame* aContainer, const nsRect& aRect,
                        const nscoord aRadii[8],
                        bool aClipBorderBackground, bool aClipAll)
    : mContainer(aContainer), mRect(aRect),
      mClipBorderBackground(aClipBorderBackground), mClipAll(aClipAll),
      mHaveRadius(false)
  {
    memcpy(mRadii, aRadii, sizeof(mRadii));
    NS_FOR_CSS_HALF_CORNERS(corner) {
      if (aRadii[corner] > 0) {
        mHaveRadius = true;
        break;
      }
    }
  }
  virtual bool WrapBorderBackground() { return mClipBorderBackground; }
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) {
    
    
    
    if (mHaveRadius) {
      return new (aBuilder) nsDisplayClipRoundedRect(aBuilder, nullptr, aList,
                                                     mRect, mRadii);
    }
    return new (aBuilder) nsDisplayClip(aBuilder, nullptr, aList, mRect);
  }
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) {
    nsIFrame* f = aItem->GetUnderlyingFrame();
    if (mClipAll ||
        nsLayoutUtils::IsProperAncestorFrame(mContainer, f, nullptr)) {
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
  bool mClipBorderBackground;
  bool mClipAll;
  bool mHaveRadius;
};

class nsDisplayClipPropWrapper : public nsDisplayWrapper
{
public:
  nsDisplayClipPropWrapper(const nsRect& aRect)
    : mRect(aRect) {}
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) {
    
    
    return new (aBuilder) nsDisplayClip(aBuilder, nullptr, aList, mRect);
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
                       bool                    aClipBorderBackground,
                       bool                    aClipAll)
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

#ifdef DEBUG
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

static nsresult
WrapPreserve3DListInternal(nsIFrame* aFrame, nsDisplayListBuilder *aBuilder, nsDisplayList *aList, nsDisplayList *aOutput, uint32_t& aIndex, nsDisplayList* aTemp)
{
  if (aIndex > nsDisplayTransform::INDEX_MAX) {
    return NS_OK;
  }

  nsresult rv = NS_OK;
  while (nsDisplayItem *item = aList->RemoveBottom()) {
    nsIFrame *childFrame = item->GetUnderlyingFrame();

    
    
    

    if (childFrame && (childFrame->GetParent()->Preserves3DChildren() || childFrame == aFrame)) {
      switch (item->GetType()) {
        case nsDisplayItem::TYPE_TRANSFORM: {
          if (!aTemp->IsEmpty()) {
            aOutput->AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder, aFrame, aTemp, aIndex++));
          }
          aOutput->AppendToTop(item);
          break;
        }
        case nsDisplayItem::TYPE_WRAP_LIST: {
          nsDisplayWrapList *list = static_cast<nsDisplayWrapList*>(item);
          rv = WrapPreserve3DListInternal(aFrame, aBuilder,
              list->GetChildren(), aOutput, aIndex, aTemp);
          list->~nsDisplayWrapList();
          break;
        }
        case nsDisplayItem::TYPE_OPACITY: {
          if (!aTemp->IsEmpty()) {
            aOutput->AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder, aFrame, aTemp, aIndex++));
          }
          nsDisplayOpacity *opacity = static_cast<nsDisplayOpacity*>(item);
          nsDisplayList output;
          
          
          
          rv = WrapPreserve3DListInternal(aFrame, aBuilder,
              opacity->GetChildren(), &output, aIndex, aTemp);
          if (!aTemp->IsEmpty()) {
            output.AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder, aFrame, aTemp, aIndex++));
          }
          opacity->GetChildren()->AppendToTop(&output);
          opacity->UpdateBounds(aBuilder);
          aOutput->AppendToTop(item);
          break;
        }
        default: {
          aTemp->AppendToTop(item);
          break;
        }
      } 
    } else {
      aTemp->AppendToTop(item);
    }
 
    if (NS_FAILED(rv) || !item || aIndex > nsDisplayTransform::INDEX_MAX)
      return rv;
  }
    
  return NS_OK;
}

static nsresult
WrapPreserve3DList(nsIFrame* aFrame, nsDisplayListBuilder* aBuilder, nsDisplayList *aList)
{
  uint32_t index = 0;
  nsDisplayList temp;
  nsDisplayList output;
  nsresult rv = WrapPreserve3DListInternal(aFrame, aBuilder, aList, &output, index, &temp);

  if (!temp.IsEmpty()) {
    output.AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder, aFrame, &temp, index++));
  }

  aList->AppendToTop(&output);
  return rv;
}

nsresult
nsIFrame::BuildDisplayListForStackingContext(nsDisplayListBuilder* aBuilder,
                                             const nsRect&         aDirtyRect,
                                             nsDisplayList*        aList) {
  if (GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
    return NS_OK;

  
  
  if (IsFrameOfType(eReplaced) && !IsVisibleForPainting(aBuilder))
    return NS_OK;

  nsRect clipPropClip;
  const nsStyleDisplay* disp = GetStyleDisplay();
  
  
  
  
  if (disp->mOpacity == 0.0 && aBuilder->IsForPainting() &&
      !aBuilder->WillComputePluginGeometry() &&
      !nsLayoutUtils::HasAnimationsForCompositor(mContent,
                                                 eCSSProperty_opacity)) {
    return NS_OK;
  }

  bool applyClipPropClipping =
      ApplyClipPropClipping(aBuilder, disp, this, &clipPropClip);
  nsRect dirtyRect = aDirtyRect;

  bool inTransform = aBuilder->IsInTransform();
  if (IsTransformed()) {
    if (aBuilder->IsForPainting() &&
        nsDisplayTransform::ShouldPrerenderTransformedContent(aBuilder, this)) {
      dirtyRect = GetVisualOverflowRectRelativeToSelf();
    } else {
      
      
      

      
      nsRect overflow = GetVisualOverflowRectRelativeToSelf();
      nsPoint offset = aBuilder->ToReferenceFrame(this);
      overflow += offset;
      overflow = nsDisplayTransform::TransformRect(overflow, this, offset);

      dirtyRect += offset;

      if (dirtyRect.Intersects(overflow)) {
        
        
        dirtyRect = GetVisualOverflowRectRelativeToSelf();
      } else {
        dirtyRect.SetEmpty();
      }
      if (!Preserves3DChildren() && !dirtyRect.Intersects(GetVisualOverflowRectRelativeToSelf())) {
        return NS_OK;
      }
    }
    inTransform = true;
  }

  if (applyClipPropClipping) {
    dirtyRect.IntersectRect(dirtyRect,
                            clipPropClip - aBuilder->ToReferenceFrame(this));
  }

  bool usingSVGEffects = nsSVGIntegrationUtils::UsingEffectsForFrame(this);
  if (usingSVGEffects) {
    dirtyRect =
      nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(this, dirtyRect);
  }

  MarkAbsoluteFramesForDisplayList(aBuilder, dirtyRect);

  
  
  if (Preserves3DChildren()) {
    aBuilder->MarkPreserve3DFramesForDisplayList(this, aDirtyRect);
  }

  nsDisplayListCollection set;
  nsresult rv;
  {    
    nsDisplayListBuilder::AutoBuildingDisplayList rootSetter(aBuilder, true);
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
                                  false, false);
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
#ifdef DEBUG
  DisplayDebugBorders(aBuilder, this, set);
#endif
  resultList.AppendToTop(set.Outlines());
  
  resultList.AppendToTop(set.PositionedDescendants());

  


  if (applyClipPropClipping &&
      (!resultList.IsEmpty() || usingSVGEffects)) {
    nsDisplayClipPropWrapper wrapper(clipPropClip);
    nsDisplayItem* item = wrapper.WrapList(aBuilder, this, &resultList);
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;
    
    resultList.AppendToTop(item);
  }
  




  if (usingSVGEffects) {
    
    rv = resultList.AppendNewToTop(
        new (aBuilder) nsDisplaySVGEffects(aBuilder, this, &resultList));
    if (NS_FAILED(rv))
      return rv;
  }
  


  else if (HasOpacity() &&
           !nsSVGUtils::CanOptimizeOpacity(this) &&
           !resultList.IsEmpty()) {
    rv = resultList.AppendNewToTop(
        new (aBuilder) nsDisplayOpacity(aBuilder, this, &resultList));
    if (NS_FAILED(rv))
      return rv;
  }

  










  if (IsTransformed() && !resultList.IsEmpty()) {
    if (Preserves3DChildren()) {
      rv = WrapPreserve3DList(this, aBuilder, &resultList);
      if (NS_FAILED(rv))
        return rv;
    } else {
      rv = resultList.AppendNewToTop(
        new (aBuilder) nsDisplayTransform(aBuilder, this, &resultList));
      if (NS_FAILED(rv))
        return rv;
    }
  }

  aList->AppendToTop(&resultList);
  return rv;
}

static bool
IsRootScrollFrameActive(nsIPresShell* aPresShell)
{
  nsIScrollableFrame* sf = aPresShell->GetRootScrollFrameAsScrollable();
  return sf && sf->IsScrollingActive();
}

nsresult
nsIFrame::BuildDisplayListForChild(nsDisplayListBuilder*   aBuilder,
                                   nsIFrame*               aChild,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists,
                                   uint32_t                aFlags) {
  
  
  if (aBuilder->IsBackgroundOnly())
    return NS_OK;

  nsIFrame* child = aChild;
  if (child->GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
    return NS_OK;
  
  bool isSVG = (child->GetStateBits() & NS_FRAME_SVG_LAYOUT);

  
  bool pseudoStackingContext =
    (aFlags & DISPLAY_CHILD_FORCE_PSEUDO_STACKING_CONTEXT) != 0;
  if (!isSVG &&
      (aFlags & DISPLAY_CHILD_INLINE) &&
      !child->IsFrameOfType(eLineParticipant)) {
    
    
    
    pseudoStackingContext = true;
  }

  
  nsRect dirty = aDirtyRect - child->GetOffsetTo(this);

  nsIAtom* childType = child->GetType();
  if (childType == nsGkAtoms::placeholderFrame) {
    nsPlaceholderFrame* placeholder = static_cast<nsPlaceholderFrame*>(child);
    child = placeholder->GetOutOfFlowFrame();
    NS_ASSERTION(child, "No out of flow frame?");
    
    
    
    if (!child || nsLayoutUtils::IsPopup(child) ||
        (child->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT))
      return NS_OK;
    
    
    
    childType = nullptr;
    
    if (child->GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
      return NS_OK;
    nsRect* savedDirty = static_cast<nsRect*>
      (child->Properties().Get(nsDisplayListBuilder::OutOfFlowDirtyRectProperty()));
    if (savedDirty) {
      dirty = *savedDirty;
    } else {
      
      
      
      dirty.SetEmpty();
    }
    pseudoStackingContext = true;
  }
  if (child->Preserves3D()) {
    nsRect* savedDirty = static_cast<nsRect*>
      (child->Properties().Get(nsDisplayListBuilder::Preserve3DDirtyRectProperty()));
    if (savedDirty) {
      dirty = *savedDirty;
    } else {
      dirty.SetEmpty();
    }
  }

  child->MarkAbsoluteFramesForDisplayList(aBuilder, dirty);

  if (childType != nsGkAtoms::placeholderFrame &&
      aBuilder->GetSelectedFramesOnly() &&
      child->IsLeaf() &&
      !aChild->IsSelected()) {
    return NS_OK;
  }

  if (aBuilder->GetIncludeAllOutOfFlows() &&
      (child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    dirty = child->GetVisualOverflowRect();
  } else if (!(child->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO)) {
    
    

    
    
    
    
    
    if (child != aBuilder->GetIgnoreScrollFrame()) {
      nsRect childDirty;
      if (!childDirty.IntersectRect(dirty, child->GetVisualOverflowRect()))
        return NS_OK;
      
      
      
      
      
    }
  }

  
  
  const nsStyleDisplay* ourDisp = GetStyleDisplay();
  
  
  if (IsThemed(ourDisp) &&
      !PresContext()->GetTheme()->WidgetIsContainer(ourDisp->mAppearance))
    return NS_OK;

  
  
  const nsStyleDisplay* disp = child->GetStyleDisplay();
  bool isVisuallyAtomic = child->HasOpacity()
    || child->IsTransformed()
    || nsSVGIntegrationUtils::UsingEffectsForFrame(child);

  bool isPositioned = !isSVG && disp->IsPositioned(child);
  if (isVisuallyAtomic || isPositioned || (!isSVG && disp->IsFloating(child)) ||
      ((disp->mClipFlags & NS_STYLE_CLIP_RECT) &&
       IsSVGContentWithCSSClip(child)) ||
      (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    pseudoStackingContext = true;
  }

  
  
  
  
  
  
  bool buildFixedPositionItem = disp->mPosition == NS_STYLE_POSITION_FIXED &&
    !child->GetParent()->GetParent() && !aBuilder->IsInFixedPosition() &&
    IsRootScrollFrameActive(PresContext()->PresShell()) && !isSVG;

  nsDisplayListBuilder::AutoBuildingDisplayList
    buildingForChild(aBuilder, child, pseudoStackingContext, buildFixedPositionItem);

  nsRect overflowClip;
  nscoord overflowClipRadii[8];
  bool applyOverflowClip =
    ApplyOverflowClipping(aBuilder, child, disp, &overflowClip);
  if (applyOverflowClip) {
    child->GetPaddingBoxBorderRadii(overflowClipRadii);
  }
  
  
  
  
  
  

  nsresult rv;
  if (!pseudoStackingContext) {
    
    
    
    if (applyOverflowClip) {
      rv = BuildDisplayListWithOverflowClip(aBuilder, child, dirty, aLists,
                                            overflowClip, overflowClipRadii);
    } else {
      rv = child->BuildDisplayList(aBuilder, dirty, aLists);
      if (NS_SUCCEEDED(rv)) {
        rv = aBuilder->DisplayCaret(child, dirty, aLists.Content());
      }
    }
#ifdef DEBUG
    DisplayDebugBorders(aBuilder, child, aLists);
#endif
    return rv;
  }
  
  nsDisplayList list;
  nsDisplayList extraPositionedDescendants;
  const nsStylePosition* pos = child->GetStylePosition();
  if ((isPositioned && pos->mZIndex.GetUnit() == eStyleUnit_Integer) ||
      isVisuallyAtomic || (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    rv = child->BuildDisplayListForStackingContext(aBuilder, dirty, &list);
    if (NS_SUCCEEDED(rv)) {
      rv = aBuilder->DisplayCaret(child, dirty, &list);
    }
  } else {
    nsRect clipRect;
    bool applyClipPropClipping =
        ApplyClipPropClipping(aBuilder, disp, child, &clipRect);
    
    
    
    
    nsDisplayListCollection pseudoStack;
    nsRect clippedDirtyRect = dirty;
    if (applyClipPropClipping) {
      
      
      clippedDirtyRect.IntersectRect(clippedDirtyRect,
                                     clipRect - aBuilder->ToReferenceFrame(child));
    }
    
    if (applyOverflowClip) {
      rv = BuildDisplayListWithOverflowClip(aBuilder, child, clippedDirtyRect,
                                            pseudoStack, overflowClip,
                                            overflowClipRadii);
    } else {
      rv = child->BuildDisplayList(aBuilder, clippedDirtyRect, pseudoStack);
      if (NS_SUCCEEDED(rv)) {
        rv = aBuilder->DisplayCaret(child, dirty, pseudoStack.Content());
      }
    }
    
    if (NS_SUCCEEDED(rv)) {
      if (applyClipPropClipping) {
        nsDisplayClipPropWrapper wrapper(clipRect);
        rv = wrapper.WrapListsInPlace(aBuilder, child, pseudoStack);
      }
    }
    list.AppendToTop(pseudoStack.BorderBackground());
    list.AppendToTop(pseudoStack.BlockBorderBackgrounds());
    list.AppendToTop(pseudoStack.Floats());
    list.AppendToTop(pseudoStack.Content());
    list.AppendToTop(pseudoStack.Outlines());
    extraPositionedDescendants.AppendToTop(pseudoStack.PositionedDescendants());
#ifdef DEBUG
    DisplayDebugBorders(aBuilder, child, aLists);
#endif
  }
  NS_ENSURE_SUCCESS(rv, rv);
    
  if (isPositioned || isVisuallyAtomic ||
      (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    
    if (!list.IsEmpty()) {
      
      
      nsDisplayItem* item;
      if (buildFixedPositionItem) {
        item = new (aBuilder) nsDisplayFixedPosition(aBuilder, child, child, &list);
      } else {
        item = new (aBuilder) nsDisplayWrapList(aBuilder, child, &list);
      }
      if (isSVG) {
        rv = aLists.Content()->AppendNewToTop(item);
      } else {
        rv = aLists.PositionedDescendants()->AppendNewToTop(item);
      }
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      if (buildFixedPositionItem) {
        while (!extraPositionedDescendants.IsEmpty()) {
          item = extraPositionedDescendants.RemoveBottom();
          nsDisplayList fixedPosDescendantList;
          fixedPosDescendantList.AppendToTop(item);
          aLists.PositionedDescendants()->AppendNewToTop(
              new (aBuilder) nsDisplayFixedPosition(aBuilder, item->GetUnderlyingFrame(),
                                                    child, &fixedPosDescendantList));
        }
      }
    }
  } else if (!isSVG && disp->IsFloating(child)) {
    if (!list.IsEmpty()) {
      rv = aLists.Floats()->AppendNewToTop(new (aBuilder)
          nsDisplayWrapList(aBuilder, child, &list));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    aLists.Content()->AppendToTop(&list);
  }
  
  
  
  
  
  aLists.PositionedDescendants()->AppendToTop(&extraPositionedDescendants);
  return NS_OK;
}

void
nsIFrame::MarkAbsoluteFramesForDisplayList(nsDisplayListBuilder* aBuilder,
                                           const nsRect& aDirtyRect)
{
  if (IsAbsoluteContainer()) {
    aBuilder->MarkFramesForDisplayList(this, GetAbsoluteContainingBlock()->GetChildList(), aDirtyRect);
  }
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
    OverflowClip(aBuilder, set, aToLists, clipRect, radii, false, true);

    return;
  }

  aToLists.Content()->AppendToTop(aFromList);
}

NS_IMETHODIMP  
nsFrame::GetContentForEvent(nsEvent* aEvent,
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
    nsRefPtr<nsAsyncDOMEvent> event =
      new nsAsyncDOMEvent(target, aDOMEventName, true, false);
    if (NS_FAILED(event->PostDOMEvent()))
      NS_WARNING("Failed to dispatch nsAsyncDOMEvent");
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

  if ((aEvent->eventStructType == NS_MOUSE_EVENT &&
      static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton) ||
      aEvent->eventStructType == NS_TOUCH_EVENT) {
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN || aEvent->message == NS_TOUCH_START) {
      HandlePress(aPresContext, aEvent, aEventStatus);
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP || aEvent->message == NS_TOUCH_END) {
      HandleRelease(aPresContext, aEvent, aEventStatus);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetDataForTableSelection(const nsFrameSelection *aFrameSelection,
                                  nsIPresShell *aPresShell, nsMouseEvent *aMouseEvent, 
                                  nsIContent **aParentContent, int32_t *aContentOffset, int32_t *aTarget)
{
  if (!aFrameSelection || !aPresShell || !aMouseEvent || !aParentContent || !aContentOffset || !aTarget)
    return NS_ERROR_NULL_POINTER;

  *aParentContent = nullptr;
  *aContentOffset = 0;
  *aTarget = 0;

  int16_t displaySelection = aPresShell->GetSelectionFlags();

  bool selectingTableCells = aFrameSelection->GetTableCellSelection();

  
  
  
  
  
  bool doTableSelection =
     displaySelection == nsISelectionDisplay::DISPLAY_ALL && selectingTableCells &&
     (aMouseEvent->message == NS_MOUSE_MOVE ||
      (aMouseEvent->message == NS_MOUSE_BUTTON_UP &&
       aMouseEvent->button == nsMouseEvent::eLeftButton) ||
      aMouseEvent->IsShift());

  if (!doTableSelection)
  {  
    
    
#ifdef XP_MACOSX
    doTableSelection = aMouseEvent->IsMeta() || (aMouseEvent->IsShift() && selectingTableCells);
#else
    doTableSelection = aMouseEvent->IsControl() || (aMouseEvent->IsShift() && selectingTableCells);
#endif
  }
  if (!doTableSelection) 
    return NS_OK;

  
  nsIFrame *frame = this;
  bool foundCell = false;
  bool foundTable = false;

  
  nsIContent* limiter = aFrameSelection->GetLimiter();

  
  
  if (limiter && nsContentUtils::ContentIsDescendantOf(limiter, GetContent()))
    return NS_OK;

  
  
  
  
  
  while (frame)
  {
    
    nsITableCellLayout *cellElement = do_QueryFrame(frame);
    if (cellElement)
    {
      foundCell = true;
      
      
      break;
    }
    else
    {
      
      
      
      nsTableOuterFrame *tableFrame = do_QueryFrame(frame);
      if (tableFrame)
      {
        foundTable = true;
        
        
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

  int32_t offset = parentContent->IndexOf(tableOrCellContent);
  
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
nsFrame::IsSelectable(bool* aSelectable, uint8_t* aSelectStyle) const
{
  if (!aSelectable) 
    return NS_ERROR_NULL_POINTER;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint8_t selectStyle  = NS_STYLE_USER_SELECT_AUTO;
  nsIFrame* frame      = (nsIFrame*)this;

  while (frame) {
    const nsStyleUIReset* userinterface = frame->GetStyleUIReset();
    switch (userinterface->mUserSelect) {
      case NS_STYLE_USER_SELECT_ALL:
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

  
  if (aSelectStyle)
    *aSelectStyle = selectStyle;
  if (mState & NS_FRAME_GENERATED_CONTENT)
    *aSelectable = false;
  else
    *aSelectable = (selectStyle != NS_STYLE_USER_SELECT_NONE);
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

  NS_ENSURE_ARG_POINTER(aEvent);
  if (aEvent->eventStructType == NS_TOUCH_EVENT) {
    return NS_OK;
  }

  
  
  
  if (!aPresContext->EventStateManager()->EventStatusOK(aEvent)) 
    return NS_OK;

  nsresult rv;
  nsIPresShell *shell = aPresContext->GetPresShell();
  if (!shell)
    return NS_ERROR_FAILURE;

  
  
  
  int16_t isEditor = shell->GetSelectionFlags();
  
  isEditor = isEditor == nsISelectionDisplay::DISPLAY_ALL;

  nsInputEvent* keyEvent = (nsInputEvent*)aEvent;
  if (!keyEvent->IsAlt()) {
    
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

  
  
  bool    selectable;
  uint8_t selectStyle;
  rv = IsSelectable(&selectable, &selectStyle);
  if (NS_FAILED(rv)) return rv;
  
  
  if (!selectable)
    return NS_OK;

  
  
  bool useFrameSelection = (selectStyle == NS_STYLE_USER_SELECT_TEXT);

  
  
  
  
  
  if (!nsIPresShell::GetCapturingContent()) {
    nsIFrame* checkFrame = this;
    nsIScrollableFrame *scrollFrame = nullptr;
    while (checkFrame) {
      scrollFrame = do_QueryFrame(checkFrame);
      if (scrollFrame) {
        nsIPresShell::SetCapturingContent(checkFrame->GetContent(), CAPTURE_IGNOREALLOWED);
        break;
      }
      checkFrame = checkFrame->GetParent();
    }
  }

  
  
  const nsFrameSelection* frameselection = nullptr;
  if (useFrameSelection)
    frameselection = GetConstFrameSelection();
  else
    frameselection = shell->ConstFrameSelection();

  if (!frameselection || frameselection->GetDisplaySelection() == nsISelectionController::SELECTION_OFF)
    return NS_OK;

  nsMouseEvent *me = (nsMouseEvent *)aEvent;

#ifdef XP_MACOSX
  if (me->IsControl())
    return NS_OK;
  bool control = me->IsMeta();
#else
  bool control = me->IsControl();
#endif

  nsRefPtr<nsFrameSelection> fc = const_cast<nsFrameSelection*>(frameselection);
  if (me->clickCount > 1)
  {
    
    
    fc->SetMouseDownState(true);
    fc->SetMouseDoubleDown(true);
    return HandleMultiplePress(aPresContext, aEvent, aEventStatus, control);
  }

  nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
  ContentOffsets offsets = GetContentOffsetsFromPoint(pt, SKIP_HIDDEN);

  if (!offsets.content)
    return NS_ERROR_FAILURE;

  
  
  
  
  if (!offsets.content->IsEditable() &&
      Preferences::GetBool("browser.ignoreNativeFrameTextSelection", false)) {
    return fc->HandleClick(offsets.content, offsets.StartOffset(),
                           offsets.EndOffset(), false, false,
                           offsets.associateWithNext);
  }

  
  nsCOMPtr<nsIContent>parentContent;
  int32_t  contentOffset;
  int32_t target;
  rv = GetDataForTableSelection(frameselection, shell, me, getter_AddRefs(parentContent), &contentOffset, &target);
  if (NS_SUCCEEDED(rv) && parentContent)
  {
    fc->SetMouseDownState(true);
    return fc->HandleTableSelection(parentContent, contentOffset, target, me);
  }

  fc->SetDelayedCaretData(0);

  
  
  
  

  SelectionDetails *details = 0;
  if (GetContent()->IsSelectionDescendant())
  {
    bool inSelection = false;
    details = frameselection->LookUpSelection(offsets.content, 0,
        offsets.EndOffset(), false);

    
    
    
    

    SelectionDetails *curDetail = details;

    while (curDetail)
    {
      
      
      
      
      
      
      if (curDetail->mType != nsISelectionController::SELECTION_SPELLCHECK &&
          curDetail->mType != nsISelectionController::SELECTION_FIND &&
          curDetail->mType != nsISelectionController::SELECTION_URLSECONDARY &&
          curDetail->mStart <= offsets.StartOffset() &&
          offsets.EndOffset() <= curDetail->mEnd)
      {
        inSelection = true;
      }

      SelectionDetails *nextDetail = curDetail->mNext;
      delete curDetail;
      curDetail = nextDetail;
    }

    if (inSelection) {
      fc->SetMouseDownState(false);
      fc->SetDelayedCaretData(me);
      return NS_OK;
    }
  }

  fc->SetMouseDownState(true);

  
  
  rv = fc->HandleClick(offsets.content, offsets.StartOffset(),
                       offsets.EndOffset(), me->IsShift(), control,
                       offsets.associateWithNext);

  if (NS_FAILED(rv))
    return rv;

  if (offsets.offset != offsets.secondaryOffset)
    fc->MaintainSelection();

  if (isEditor && !me->IsShift() &&
      (offsets.EndOffset() - offsets.StartOffset()) == 1)
  {
    
    
    
    
    
    fc->SetMouseDownState(false);
  }

  return rv;
}















nsresult
nsFrame::SelectByTypeAtPoint(nsPresContext* aPresContext,
                             const nsPoint& aPoint,
                             nsSelectionAmount aBeginAmountType,
                             nsSelectionAmount aEndAmountType,
                             uint32_t aSelectFlags)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  
  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF)
    return NS_OK;

  ContentOffsets offsets = GetContentOffsetsFromPoint(aPoint, SKIP_HIDDEN);
  if (!offsets.content)
    return NS_ERROR_FAILURE;

  int32_t offset;
  const nsFrameSelection* frameSelection =
    PresContext()->GetPresShell()->ConstFrameSelection();
  nsIFrame* theFrame = frameSelection->
    GetFrameForNodeOffset(offsets.content, offsets.offset,
                          nsFrameSelection::HINT(offsets.associateWithNext),
                          &offset);
  if (!theFrame)
    return NS_ERROR_FAILURE;

  nsFrame* frame = static_cast<nsFrame*>(theFrame);
  return frame->PeekBackwardAndForward(aBeginAmountType, aEndAmountType, 
                                       offset, aPresContext,
                                       aBeginAmountType != eSelectWord,
                                       aSelectFlags);
}





NS_IMETHODIMP
nsFrame::HandleMultiplePress(nsPresContext* aPresContext,
                             nsGUIEvent*    aEvent,
                             nsEventStatus* aEventStatus,
                             bool           aControlHeld)
{
  NS_ENSURE_ARG_POINTER(aEvent);
  NS_ENSURE_ARG_POINTER(aEventStatus);

  if (nsEventStatus_eConsumeNoDefault == *aEventStatus ||
      DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }

  
  
  
  
  nsSelectionAmount beginAmount, endAmount;
  nsMouseEvent *me = (nsMouseEvent *)aEvent;
  if (!me) return NS_OK;

  if (me->clickCount == 4) {
    beginAmount = endAmount = eSelectParagraph;
  } else if (me->clickCount == 3) {
    if (Preferences::GetBool("browser.triple_click_selects_paragraph")) {
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

  nsPoint relPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
  return SelectByTypeAtPoint(aPresContext, relPoint, beginAmount, endAmount,
                             (aControlHeld ? SELECT_ACCUMULATE : 0));
}

nsresult
nsFrame::PeekBackwardAndForward(nsSelectionAmount aAmountBack,
                                nsSelectionAmount aAmountForward,
                                int32_t aStartPos,
                                nsPresContext* aPresContext,
                                bool aJumpLines,
                                uint32_t aSelectFlags)
{
  nsIFrame* baseFrame = this;
  int32_t baseOffset = aStartPos;
  nsresult rv;

  if (aAmountBack == eSelectWord) {
    
    
    nsPeekOffsetStruct pos(eSelectCharacter,
                           eDirNext,
                           aStartPos,
                           0,
                           aJumpLines,
                           true,  
                           false,
                           false);
    rv = PeekOffset(&pos);
    if (NS_SUCCEEDED(rv)) {
      baseFrame = pos.mResultFrame;
      baseOffset = pos.mContentOffset;
    }
  }

  
  nsPeekOffsetStruct startpos(aAmountBack,
                              eDirPrevious,
                              baseOffset,
                              0,
                              aJumpLines,
                              true,  
                              false,
                              false);
  rv = baseFrame->PeekOffset(&startpos);
  if (NS_FAILED(rv))
    return rv;

  nsPeekOffsetStruct endpos(aAmountForward,
                            eDirNext,
                            aStartPos,
                            0,
                            aJumpLines,
                            true,  
                            false,
                            false);
  rv = PeekOffset(&endpos);
  if (NS_FAILED(rv))
    return rv;

  
  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();

  rv = frameSelection->HandleClick(startpos.mResultContent,
                                   startpos.mContentOffset, startpos.mContentOffset,
                                   false, (aSelectFlags & SELECT_ACCUMULATE),
                                   nsFrameSelection::HINTRIGHT);
  if (NS_FAILED(rv))
    return rv;

  rv = frameSelection->HandleClick(endpos.mResultContent,
                                   endpos.mContentOffset, endpos.mContentOffset,
                                   true, false,
                                   nsFrameSelection::HINTLEFT);
  if (NS_FAILED(rv))
    return rv;

  
  return frameSelection->MaintainSelection(aAmountBack);
}

NS_IMETHODIMP nsFrame::HandleDrag(nsPresContext* aPresContext, 
                                  nsGUIEvent*     aEvent,
                                  nsEventStatus*  aEventStatus)
{
  MOZ_ASSERT(aEvent->eventStructType == NS_MOUSE_EVENT, "HandleDrag can only handle mouse event");

  bool selectable;
  IsSelectable(&selectable, nullptr);

  
  
  
  if (!selectable)
    return NS_OK;
  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }
  nsIPresShell *presShell = aPresContext->PresShell();

  nsRefPtr<nsFrameSelection> frameselection = GetFrameSelection();
  bool mouseDown = frameselection->GetMouseDownState();
  if (!mouseDown)
    return NS_OK;

  frameselection->StopAutoScrollTimer();

  
  nsCOMPtr<nsIContent> parentContent;
  int32_t contentOffset;
  int32_t target;
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
  nsIScrollableFrame *scrollFrame = nullptr;
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
                     bool                      aHandleTableSel,
                     int32_t                   aContentOffsetForTableSel,
                     int32_t                   aTargetForTableSel,
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
      if (!aOffsets.content || !aFrameSelection->HasDelayedCaretData()) {
        return NS_ERROR_FAILURE;
      }

      
      
      
      
      
      
      
      
      
      aFrameSelection->SetMouseDownState(true);

      rv = aFrameSelection->HandleClick(aOffsets.content,
                                        aOffsets.StartOffset(),
                                        aOffsets.EndOffset(),
                                        aFrameSelection->IsShiftDownInDelayedCaretData(),
                                        false,
                                        aOffsets.associateWithNext);
      if (NS_FAILED(rv)) {
        return rv;
      }
    } else if (aParentContentForTableSel) {
      aFrameSelection->SetMouseDownState(false);
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

  aFrameSelection->SetMouseDownState(false);
  aFrameSelection->StopAutoScrollTimer();

  return NS_OK;
}

NS_IMETHODIMP nsFrame::HandleRelease(nsPresContext* aPresContext,
                                     nsGUIEvent*    aEvent,
                                     nsEventStatus* aEventStatus)
{
  if (aEvent->eventStructType != NS_MOUSE_EVENT) {
    return NS_OK;
  }

  nsIFrame* activeFrame = GetActiveSelectionFrame(aPresContext, this);

  nsCOMPtr<nsIContent> captureContent = nsIPresShell::GetCapturingContent();

  
  
  nsIPresShell::SetCapturingContent(nullptr, 0);

  bool selectionOff =
    (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF);

  nsRefPtr<nsFrameSelection> frameselection;
  ContentOffsets offsets;
  nsCOMPtr<nsIContent> parentContent;
  int32_t contentOffsetForTableSel = 0;
  int32_t targetForTableSel = 0;
  bool handleTableSelection = true;

  if (!selectionOff) {
    frameselection = GetFrameSelection();
    if (nsEventStatus_eConsumeNoDefault != *aEventStatus && frameselection) {
      
      
      

      bool mouseDown = frameselection->GetMouseDownState();

      if (!mouseDown && frameselection->HasDelayedCaretData() &&
          frameselection->GetClickCountInDelayedCaretData() < 2) {
        nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
        offsets = GetContentOffsetsFromPoint(pt, SKIP_HIDDEN);
        handleTableSelection = false;
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
    frameSelection->SetMouseDownState(false);
    frameSelection->StopAutoScrollTimer();
  }

  
  

  return selectionOff
    ? NS_OK
    : HandleFrameSelection(frameselection, offsets, handleTableSelection,
                           contentOffsetForTableSel, targetForTableSel,
                           parentContent, aEvent, aEventStatus);
}

struct NS_STACK_CLASS FrameContentRange {
  FrameContentRange(nsIContent* aContent, int32_t aStart, int32_t aEnd) :
    content(aContent), start(aStart), end(aEnd) { }
  nsCOMPtr<nsIContent> content;
  int32_t start;
  int32_t end;
};


static FrameContentRange GetRangeForFrame(nsIFrame* aFrame) {
  nsCOMPtr<nsIContent> content, parent;
  content = aFrame->GetContent();
  if (!content) {
    NS_WARNING("Frame has no content");
    return FrameContentRange(nullptr, -1, -1);
  }
  nsIAtom* type = aFrame->GetType();
  if (type == nsGkAtoms::textFrame) {
    int32_t offset, offsetEnd;
    aFrame->GetOffsets(offset, offsetEnd);
    return FrameContentRange(content, offset, offsetEnd);
  }
  if (type == nsGkAtoms::brFrame) {
    parent = content->GetParent();
    int32_t beginOffset = parent->IndexOf(content);
    return FrameContentRange(parent, beginOffset, beginOffset);
  }
  
  
  do {
    parent  = content->GetParent();
    if (parent) {
      int32_t beginOffset = parent->IndexOf(content);
      if (beginOffset >= 0)
        return FrameContentRange(parent, beginOffset, beginOffset + 1);
      content = parent;
    }
  } while (parent);

  
  return FrameContentRange(content, 0, content->GetChildCount());
}





struct FrameTarget {
  FrameTarget(nsIFrame* aFrame, bool aFrameEdge, bool aAfterFrame,
              bool aEmptyBlock = false) :
    frame(aFrame), frameEdge(aFrameEdge), afterFrame(aAfterFrame),
    emptyBlock(aEmptyBlock) { }
  static FrameTarget Null() {
    return FrameTarget(nullptr, false, false);
  }
  bool IsNull() {
    return !frame;
  }
  nsIFrame* frame;
  bool frameEdge;
  bool afterFrame;
  bool emptyBlock;
};


static FrameTarget GetSelectionClosestFrame(nsIFrame* aFrame, nsPoint aPoint,
                                            uint32_t aFlags);

static bool SelfIsSelectable(nsIFrame* aFrame, uint32_t aFlags)
{
  if ((aFlags & nsIFrame::SKIP_HIDDEN) &&
      !aFrame->GetStyleVisibility()->IsVisible()) {
    return false;
  }
  return !aFrame->IsGeneratedContentFrame() &&
    aFrame->GetStyleUIReset()->mUserSelect != NS_STYLE_USER_SELECT_NONE;
}

static bool SelectionDescendToKids(nsIFrame* aFrame) {
  uint8_t style = aFrame->GetStyleUIReset()->mUserSelect;
  nsIFrame* parent = aFrame->GetParent();
  
  
  
  
  
  
  
  
  return !aFrame->IsGeneratedContentFrame() &&
         style != NS_STYLE_USER_SELECT_ALL  &&
         style != NS_STYLE_USER_SELECT_NONE &&
         ((parent->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION) ||
          !(aFrame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION));
}

static FrameTarget GetSelectionClosestFrameForChild(nsIFrame* aChild,
                                                    nsPoint aPoint,
                                                    uint32_t aFlags)
{
  nsIFrame* parent = aChild->GetParent();
  if (SelectionDescendToKids(aChild)) {
    nsPoint pt = aPoint - aChild->GetOffsetTo(parent);
    return GetSelectionClosestFrame(aChild, pt, aFlags);
  }
  return FrameTarget(aChild, false, false);
}






static FrameTarget DrillDownToSelectionFrame(nsIFrame* aFrame,
                                             bool aEndFrame, uint32_t aFlags) {
  if (SelectionDescendToKids(aFrame)) {
    nsIFrame* result = nullptr;
    nsIFrame *frame = aFrame->GetFirstPrincipalChild();
    if (!aEndFrame) {
      while (frame && (!SelfIsSelectable(frame, aFlags) ||
                        frame->IsEmpty()))
        frame = frame->GetNextSibling();
      if (frame)
        result = frame;
    } else {
      
      
      
      
      while (frame) {
        if (!frame->IsEmpty() && SelfIsSelectable(frame, aFlags))
          result = frame;
        frame = frame->GetNextSibling();
      }
    }
    if (result)
      return DrillDownToSelectionFrame(result, aEndFrame, aFlags);
  }
  
  return FrameTarget(aFrame, true, aEndFrame);
}



static FrameTarget GetSelectionClosestFrameForLine(
                      nsBlockFrame* aParent,
                      nsBlockFrame::line_iterator aLine,
                      nsPoint aPoint,
                      uint32_t aFlags)
{
  nsIFrame *frame = aLine->mFirstChild;
  
  if (aLine == aParent->end_lines())
    return DrillDownToSelectionFrame(aParent, true, aFlags);
  nsIFrame *closestFromLeft = nullptr, *closestFromRight = nullptr;
  nsRect rect = aLine->mBounds;
  nscoord closestLeft = rect.x, closestRight = rect.XMost();
  for (int32_t n = aLine->GetChildCount(); n;
       --n, frame = frame->GetNextSibling()) {
    if (!SelfIsSelectable(frame, aFlags) || frame->IsEmpty())
      continue;
    nsRect frameRect = frame->GetRect();
    if (aPoint.x >= frameRect.x) {
      if (aPoint.x < frameRect.XMost()) {
        return GetSelectionClosestFrameForChild(frame, aPoint, aFlags);
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
    return GetSelectionClosestFrameForChild(closestFromLeft, aPoint, aFlags);
  }
  return GetSelectionClosestFrameForChild(closestFromRight, aPoint, aFlags);
}






static FrameTarget GetSelectionClosestFrameForBlock(nsIFrame* aFrame,
                                                    nsPoint aPoint,
                                                    uint32_t aFlags)
{
  nsBlockFrame* bf = nsLayoutUtils::GetAsBlock(aFrame); 
  if (!bf)
    return FrameTarget::Null();

  
  nsBlockFrame::line_iterator firstLine = bf->begin_lines();
  nsBlockFrame::line_iterator end = bf->end_lines();
  if (firstLine == end) {
    nsIContent *blockContent = aFrame->GetContent();
    if (blockContent) {
      
      return FrameTarget(aFrame, false, false, true);
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

    
    
    
    int32_t dragOutOfFrame =
      Preferences::GetInt("browser.drag_out_of_frame_style");

    if (prevLine == end) {
      if (dragOutOfFrame == 1 || nextLine == end)
        return DrillDownToSelectionFrame(aFrame, false, aFlags);
      closestLine = nextLine;
    } else if (nextLine == end) {
      if (dragOutOfFrame == 1)
        return DrillDownToSelectionFrame(aFrame, true, aFlags);
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
                                                         aPoint, aFlags);
    if (!target.IsNull())
      return target;
    ++closestLine;
  } while (closestLine != end);
  
  return DrillDownToSelectionFrame(aFrame, true, aFlags);
}








static FrameTarget GetSelectionClosestFrame(nsIFrame* aFrame, nsPoint aPoint,
                                            uint32_t aFlags)
{
  {
    
    FrameTarget target = GetSelectionClosestFrameForBlock(aFrame, aPoint, aFlags);
    if (!target.IsNull())
      return target;
  }

  nsIFrame *kid = aFrame->GetFirstPrincipalChild();

  if (kid) {
    
    nsIFrame::FrameWithDistance closest = { nullptr, nscoord_MAX, nscoord_MAX };
    for (; kid; kid = kid->GetNextSibling()) {
      if (!SelfIsSelectable(kid, aFlags) || kid->IsEmpty())
        continue;

      kid->FindCloserFrameForSelection(aPoint, &closest);
    }
    if (closest.mFrame) {
      if (closest.mFrame->IsSVGText())
        return FrameTarget(closest.mFrame, false, false);
      return GetSelectionClosestFrameForChild(closest.mFrame, aPoint, aFlags);
    }
  }
  return FrameTarget(aFrame, false, false);
}

nsIFrame::ContentOffsets OffsetsForSingleFrame(nsIFrame* aFrame, nsPoint aPoint)
{
  nsIFrame::ContentOffsets offsets;
  FrameContentRange range = GetRangeForFrame(aFrame);
  offsets.content = range.content;
  
  
  if (aFrame->GetNextContinuation() || aFrame->GetPrevContinuation()) {
    offsets.offset = range.start;
    offsets.secondaryOffset = range.end;
    offsets.associateWithNext = true;
    return offsets;
  }

  
  nsRect rect(nsPoint(0, 0), aFrame->GetSize());

  bool isBlock = aFrame->GetDisplay() != NS_STYLE_DISPLAY_INLINE;
  bool isRtl = (aFrame->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL);
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
    
    
    if (frame->GetStyleUIReset()->mUserSelect == NS_STYLE_USER_SELECT_ALL ||
        frame->IsGeneratedContentFrame()) {
      adjustedFrame = frame;
    }
  }
  return adjustedFrame;
}

nsIFrame::ContentOffsets nsIFrame::GetContentOffsetsFromPoint(nsPoint aPoint,
                                                              uint32_t aFlags)
{
  nsIFrame *adjustedFrame;
  if (aFlags & IGNORE_SELECTION_STYLE) {
    adjustedFrame = this;
  }
  else {
    
    
    
    
    

    adjustedFrame = AdjustFrameForSelectionStyles(this);

    
    
    if (adjustedFrame && adjustedFrame->GetStyleUIReset()->mUserSelect ==
        NS_STYLE_USER_SELECT_ALL) {
      nsPoint adjustedPoint = aPoint + this->GetOffsetTo(adjustedFrame);
      return OffsetsForSingleFrame(adjustedFrame, adjustedPoint);
    }

    
    
    if (adjustedFrame != this)
      adjustedFrame = adjustedFrame->GetParent();
  }

  nsPoint adjustedPoint = aPoint + this->GetOffsetTo(adjustedFrame);

  FrameTarget closest =
    GetSelectionClosestFrame(adjustedFrame, adjustedPoint, aFlags);

  if (closest.emptyBlock) {
    ContentOffsets offsets;
    NS_ASSERTION(closest.frame,
                 "closest.frame must not be null when it's empty");
    offsets.content = closest.frame->GetContent();
    offsets.offset = 0;
    offsets.secondaryOffset = 0;
    offsets.associateWithNext = true;
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

  nsPoint pt;
  if (closest.frame != this) {
    if (closest.frame->IsSVGText()) {
      pt = nsLayoutUtils::TransformAncestorPointToFrame(closest.frame,
                                                        aPoint, this);
    } else {
      pt = aPoint - closest.frame->GetOffsetTo(this);
    }
  } else {
    pt = aPoint;
  }
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
    
    aCursor.mCursor =
      (mContent && mContent->IsEditable()) ? NS_STYLE_CURSOR_TEXT :
                                             NS_STYLE_CURSOR_DEFAULT;
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

  if (GetStateBits() & NS_FRAME_FONT_INFLATION_FLOW_ROOT) {
    nsFontInflationData::MarkFontInflationDataTextDirty(this);
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
  bool canBreak = !CanContinueTextRun() &&
    GetParent()->GetStyleText()->WhiteSpaceCanWrap();
  
  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = false;
  aData->trailingTextFrame = nullptr;
  aData->currentLine += nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                            this, nsLayoutUtils::MIN_WIDTH);
  aData->atStartOfLine = false;
  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
}

 void
nsFrame::AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                            nsIFrame::InlinePrefWidthData *aData)
{
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = false;
  nscoord myPref = nsLayoutUtils::IntrinsicForContainer(aRenderingContext, 
                       this, nsLayoutUtils::PREF_WIDTH);
  aData->currentLine = NSCoordSaturatingAdd(aData->currentLine, myPref);
}

void
nsIFrame::InlineMinWidthData::ForceBreak(nsRenderingContext *aRenderingContext)
{
  currentLine -= trailingWhitespace;
  prevLines = std::max(prevLines, currentLine);
  currentLine = trailingWhitespace = 0;

  for (uint32_t i = 0, i_end = floats.Length(); i != i_end; ++i) {
    nscoord float_min = floats[i].Width();
    if (float_min > prevLines)
      prevLines = float_min;
  }
  floats.Clear();
  trailingTextFrame = nullptr;
  skipWhitespace = true;
}

void
nsIFrame::InlineMinWidthData::OptionallyBreak(nsRenderingContext *aRenderingContext,
                                              nscoord aHyphenWidth)
{
  trailingTextFrame = nullptr;

  
  
  
  
  
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

    for (uint32_t i = 0, i_end = floats.Length(); i != i_end; ++i) {
      const FloatInfo& floatInfo = floats[i];
      const nsStyleDisplay *floatDisp = floatInfo.Frame()->GetStyleDisplay();
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
      nscoord floatWidth = floatInfo.Width();
      
      
      floats_cur =
        NSCoordSaturatingAdd(floats_cur, std::max(0, floatWidth));
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
  prevLines = std::max(prevLines, currentLine);
  currentLine = trailingWhitespace = 0;
  skipWhitespace = true;
}

static void
AddCoord(const nsStyleCoord& aStyle,
         nsRenderingContext* aRenderingContext,
         nsIFrame* aFrame,
         nscoord* aCoord, float* aPercent,
         bool aClampNegativeToZero)
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
        
        *aCoord += std::max(calc->mLength, 0);
        *aPercent += std::max(calc->mPercent, 0.0f);
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
           &result.hMargin, &result.hPctMargin, false);
  AddCoord(styleMargin->mMargin.GetRight(), aRenderingContext, this,
           &result.hMargin, &result.hPctMargin, false);

  const nsStylePadding *stylePadding = GetStylePadding();
  AddCoord(stylePadding->mPadding.GetLeft(), aRenderingContext, this,
           &result.hPadding, &result.hPctPadding, true);
  AddCoord(stylePadding->mPadding.GetRight(), aRenderingContext, this,
           &result.hPadding, &result.hPctPadding, true);

  const nsStyleBorder *styleBorder = GetStyleBorder();
  result.hBorder += styleBorder->GetComputedBorderWidth(NS_SIDE_LEFT);
  result.hBorder += styleBorder->GetComputedBorderWidth(NS_SIDE_RIGHT);

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
                     uint32_t aFlags)
{
  nsSize result = ComputeAutoSize(aRenderingContext, aCBSize, aAvailableWidth,
                                  aMargin, aBorder, aPadding,
                                  aFlags & eShrinkWrap);
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
  const nsStyleCoord* widthStyleCoord = &(stylePos->mWidth);
  const nsStyleCoord* heightStyleCoord = &(stylePos->mHeight);

  bool isFlexItem = IsFlexItem();
  bool isHorizontalFlexItem = false;
 
#ifdef MOZ_FLEXBOX
  if (isFlexItem) {
    
    
    
    
    uint32_t flexDirection = mParent->GetStylePosition()->mFlexDirection;
    isHorizontalFlexItem =
      flexDirection == NS_STYLE_FLEX_DIRECTION_ROW ||
      flexDirection == NS_STYLE_FLEX_DIRECTION_ROW_REVERSE;

    
    
    
    const nsStyleCoord* flexBasis = &(stylePos->mFlexBasis);
    if (flexBasis->GetUnit() != eStyleUnit_Auto) {
      if (isHorizontalFlexItem) {
        widthStyleCoord = flexBasis;
      } else {
        
        
        
        
        
        
        
        if (flexBasis->GetUnit() != eStyleUnit_Enumerated) {
          heightStyleCoord = flexBasis;
        }
      }
    }
  }
#endif 

  

  if (widthStyleCoord->GetUnit() != eStyleUnit_Auto) {
    result.width =
      nsLayoutUtils::ComputeWidthValue(aRenderingContext, this,
        aCBSize.width, boxSizingAdjust.width, boxSizingToMarginEdgeWidth,
        *widthStyleCoord);
  }

  
  
  
  if (stylePos->mMaxWidth.GetUnit() != eStyleUnit_None &&
      !(isFlexItem && isHorizontalFlexItem)) {
    nscoord maxWidth =
      nsLayoutUtils::ComputeWidthValue(aRenderingContext, this,
        aCBSize.width, boxSizingAdjust.width, boxSizingToMarginEdgeWidth,
        stylePos->mMaxWidth);
    result.width = std::min(maxWidth, result.width);
  }

  nscoord minWidth;
  if (stylePos->mMinWidth.GetUnit() != eStyleUnit_Auto &&
      !(isFlexItem && isHorizontalFlexItem)) {
    minWidth =
      nsLayoutUtils::ComputeWidthValue(aRenderingContext, this,
        aCBSize.width, boxSizingAdjust.width, boxSizingToMarginEdgeWidth,
        stylePos->mMinWidth);
  } else {
    
    
    
    
    
    minWidth = 0;
  }
  result.width = std::max(minWidth, result.width);

  
  
  
  
  if (!nsLayoutUtils::IsAutoHeight(*heightStyleCoord, aCBSize.height) &&
      !(aFlags & nsIFrame::eUseAutoHeight)) {
    result.height =
      nsLayoutUtils::ComputeHeightValue(aCBSize.height, 
                                        boxSizingAdjust.height,
                                        *heightStyleCoord);
  }

  if (result.height != NS_UNCONSTRAINEDSIZE) {
    if (!nsLayoutUtils::IsAutoHeight(stylePos->mMaxHeight, aCBSize.height) &&
        !(isFlexItem && !isHorizontalFlexItem)) {
      nscoord maxHeight =
        nsLayoutUtils::ComputeHeightValue(aCBSize.height, 
                                          boxSizingAdjust.height,
                                          stylePos->mMaxHeight);
      result.height = std::min(maxHeight, result.height);
    }

    if (!nsLayoutUtils::IsAutoHeight(stylePos->mMinHeight, aCBSize.height) &&
        !(isFlexItem && !isHorizontalFlexItem)) {
      nscoord minHeight =
        nsLayoutUtils::ComputeHeightValue(aCBSize.height, 
                                          boxSizingAdjust.height, 
                                          stylePos->mMinHeight);
      result.height = std::max(minHeight, result.height);
    }
  }

  const nsStyleDisplay *disp = GetStyleDisplay();
  if (IsThemed(disp)) {
    nsIntSize widget(0, 0);
    bool canOverride = true;
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

  result.width = std::max(0, result.width);
  result.height = std::max(0, result.height);

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
      GetStyleBorder()->HasBorder() || !GetStyleBackground()->IsTransparent() ||
      GetStyleDisplay()->mAppearance) {
    
    
    return GetVisualOverflowRect();
  }

  nsRect r(0, 0, 0, 0);
  ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      r.UnionRect(r, child->ComputeTightBounds(aContext) + child->GetPosition());
    }
  }
  return r;
}

 nsSize
nsFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                         nsSize aCBSize, nscoord aAvailableWidth,
                         nsSize aMargin, nsSize aBorder, nsSize aPadding,
                         bool aShrinkWrap)
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
  
  
  AutoMaybeDisableFontInflation an(this);

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
                     ("nsFrame::DidReflow: aStatus=%d", static_cast<uint32_t>(aStatus)));

  nsSVGEffects::InvalidateDirectRenderingObservers(this, nsSVGEffects::INVALIDATE_REFLOW);

  if (nsDidReflowStatus::FINISHED == aStatus) {
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

void
nsFrame::FinishReflowWithAbsoluteFrames(nsPresContext*           aPresContext,
                                        nsHTMLReflowMetrics&     aDesiredSize,
                                        const nsHTMLReflowState& aReflowState,
                                        nsReflowStatus&          aStatus)
{
  ReflowAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus);

  FinishAndStoreOverflow(&aDesiredSize);
}

void
nsFrame::ReflowAbsoluteFrames(nsPresContext*           aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus)
{
  if (HasAbsolutelyPositionedChildren()) {
    nsAbsoluteContainingBlock* absoluteContainer = GetAbsoluteContainingBlock();

    
    

    
    nsMargin computedBorder =
      aReflowState.mComputedBorderPadding - aReflowState.mComputedPadding;
    nscoord containingBlockWidth =
      aDesiredSize.width - computedBorder.LeftRight();
    nscoord containingBlockHeight =
      aDesiredSize.height - computedBorder.TopBottom();

    nsContainerFrame* container = do_QueryFrame(this);
    NS_ASSERTION(container, "Abs-pos children only supported on container frames for now");

    absoluteContainer->Reflow(container, aPresContext, aReflowState, aStatus,
                              containingBlockWidth, containingBlockHeight,
                              true, true, true, 
                              &aDesiredSize.mOverflowAreas);
  }
}

 bool
nsFrame::CanContinueTextRun() const
{
  
  
  return false;
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
nsFrame::AttributeChanged(int32_t         aNameSpaceID,
                          nsIAtom*        aAttribute,
                          int32_t         aModType)
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
  return nullptr;
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
  return nullptr;
}

NS_IMETHODIMP nsFrame::SetNextContinuation(nsIFrame*)
{
  NS_ERROR("not splittable");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsIFrame* nsFrame::GetPrevInFlowVirtual() const
{
  return nullptr;
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
  return nullptr;
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

NS_DECLARE_FRAME_PROPERTY(ViewProperty, nullptr)


nsView*
nsIFrame::GetView() const
{
  
  if (!(GetStateBits() & NS_FRAME_HAS_VIEW))
    return nullptr;

  
  void* value = Properties().Get(ViewProperty());
  NS_ASSERTION(value, "frame state bit was set but frame has no view");
  return static_cast<nsView*>(value);
}

 nsView*
nsIFrame::GetViewExternal() const
{
  return GetView();
}

nsresult
nsIFrame::SetView(nsView* aView)
{
  if (aView) {
    aView->SetFrame(this);

    
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
  for (nsIFrame* f = mParent; nullptr != f; f = f->GetParent()) {
    if (f->HasView()) {
      return f;
    }
  }
  return nullptr;
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
nsIFrame::GetOffsetToCrossDoc(const nsIFrame* aOther, const int32_t aAPD) const
{
  NS_PRECONDITION(aOther,
                  "Must have frame for destination coordinate system!");
  NS_ASSERTION(PresContext()->GetRootPresContext() ==
                 aOther->PresContext()->GetRootPresContext(),
               "trying to get the offset between frames in different document "
               "hierarchies?");
  if (PresContext()->GetRootPresContext() !=
        aOther->PresContext()->GetRootPresContext()) {
    
    NS_RUNTIMEABORT("trying to get the offset between frames in different "
                    "document hierarchies?");
  }

  const nsIFrame* root = nullptr;
  
  
  
  nsPoint offset(0, 0), docOffset(0, 0);
  const nsIFrame* f = this;
  int32_t currAPD = PresContext()->AppUnitsPerDevPixel();
  while (f && f != aOther) {
    docOffset += f->GetPosition();
    nsIFrame* parent = f->GetParent();
    if (parent) {
      f = parent;
    } else {
      nsPoint newOffset(0, 0);
      root = f;
      f = nsLayoutUtils::GetCrossDocParentFrame(f, &newOffset);
      int32_t newAPD = f ? f->PresContext()->AppUnitsPerDevPixel() : 0;
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
                                         nsView** aView) const
{
  NS_PRECONDITION(nullptr != aView, "null OUT parameter pointer");
  nsIFrame* frame = (nsIFrame*)this;

  *aView = nullptr;
  aOffset.MoveTo(0, 0);
  do {
    aOffset += frame->GetPosition();
    frame = frame->GetParent();
  } while (frame && !frame->HasView());
  if (frame)
    *aView = frame->GetView();
  return NS_OK;
}

nsIWidget*
nsIFrame::GetNearestWidget() const
{
  return GetClosestView()->GetNearestWidget(nullptr);
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
  return nullptr;
}

bool
nsIFrame::IsLeaf() const
{
  return true;
}

class LayerActivity {
public:
  LayerActivity(nsIFrame* aFrame) : mFrame(aFrame), mChangeHint(nsChangeHint(0)) {}
  ~LayerActivity();
  nsExpirationState* GetExpirationState() { return &mState; }

  nsIFrame* mFrame;
  nsExpirationState mState;
  
  
  
  
  nsChangeHint mChangeHint;
};

class LayerActivityTracker MOZ_FINAL : public nsExpirationTracker<LayerActivity,4> {
public:
  
  enum { GENERATION_MS = 100 };
  LayerActivityTracker()
    : nsExpirationTracker<LayerActivity,4>(GENERATION_MS) {}
  ~LayerActivityTracker() {
    AgeAllGenerations();
  }

  virtual void NotifyExpired(LayerActivity* aObject);
};

static LayerActivityTracker* gLayerActivityTracker = nullptr;

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
  aObject->mFrame = nullptr;

  
  if (aObject->mChangeHint & ~(nsChangeHint_UpdateOpacityLayer|nsChangeHint_UpdateTransformLayer)) {
    f->InvalidateFrameSubtree();
  } else {
    if (aObject->mChangeHint & nsChangeHint_UpdateOpacityLayer) {
      f->InvalidateFrameSubtree(nsDisplayItem::TYPE_OPACITY);
    } 
    if (aObject->mChangeHint & nsChangeHint_UpdateTransformLayer) {
      f->InvalidateFrameSubtree(nsDisplayItem::TYPE_TRANSFORM);
    }
  } 
  f->Properties().Delete(LayerActivityProperty());
}

void
nsIFrame::MarkLayersActive(nsChangeHint aChangeHint)
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
  NS_UpdateHint(layerActivity->mChangeHint, aChangeHint);
}

bool
nsIFrame::AreLayersMarkedActive()
{
  return Properties().Get(LayerActivityProperty()) != nullptr;
}

bool
nsIFrame::AreLayersMarkedActive(nsChangeHint aChangeHint)
{
  LayerActivity* layerActivity =
    static_cast<LayerActivity*>(Properties().Get(LayerActivityProperty()));
  return layerActivity && (layerActivity->mChangeHint & aChangeHint);
}

 void
nsFrame::ShutdownLayerActivityTimer()
{
  delete gLayerActivityTracker;
  gLayerActivityTracker = nullptr;
}

gfx3DMatrix
nsIFrame::GetTransformMatrix(const nsIFrame* aStopAtAncestor,
                             nsIFrame** aOutAncestor)
{
  NS_PRECONDITION(aOutAncestor, "Need a place to put the ancestor!");

  



  if (IsTransformed()) {
    


    NS_ASSERTION(nsLayoutUtils::GetCrossDocParentFrame(this),
                 "Cannot transform the viewport frame!");
    int32_t scaleFactor = PresContext()->AppUnitsPerDevPixel();

    gfx3DMatrix result =
      nsDisplayTransform::GetResultingTransformMatrix(this, nsPoint(0, 0), scaleFactor, nullptr, aOutAncestor);
    
    nsPoint delta = GetOffsetToCrossDoc(*aOutAncestor);
    
    result *= gfx3DMatrix::Translation
      (NSAppUnitsToFloatPixels(delta.x, scaleFactor),
       NSAppUnitsToFloatPixels(delta.y, scaleFactor),
       0.0f);
    return result;
  }

  if (nsLayoutUtils::IsPopup(this) &&
      GetType() == nsGkAtoms::listControlFrame) {
    nsPresContext* presContext = PresContext();
    nsIFrame* docRootFrame = presContext->PresShell()->GetRootFrame();

    
    
    
    
    nsIWidget* widget = GetView()->GetWidget();
    nsPresContext* rootPresContext = PresContext()->GetRootPresContext();
    
    
    
    if (widget && rootPresContext) {
      nsIWidget* toplevel = rootPresContext->GetNearestWidget();
      if (toplevel) {
        nsIntRect screenBounds;
        widget->GetClientBounds(screenBounds);
        nsIntRect toplevelScreenBounds;
        toplevel->GetClientBounds(toplevelScreenBounds);
        nsIntPoint translation = screenBounds.TopLeft() - toplevelScreenBounds.TopLeft();

        gfx3DMatrix transformToTop;
        transformToTop._41 = translation.x;
        transformToTop._42 = translation.y;

        *aOutAncestor = docRootFrame;
        gfx3DMatrix docRootTransformToTop =
          nsLayoutUtils::GetTransformToAncestor(docRootFrame, nullptr);
        if (docRootTransformToTop.IsSingular()) {
          NS_WARNING("Containing document is invisible, we can't compute a valid transform");
        } else {
          gfx3DMatrix topToDocRootTransform = docRootTransformToTop.Inverse();
          return transformToTop*topToDocRootTransform;
        }
      }
    }
  }

  *aOutAncestor = nsLayoutUtils::GetCrossDocParentFrame(this);

  







  if (!*aOutAncestor)
    return gfx3DMatrix();
  
  
  while (!(*aOutAncestor)->IsTransformed() &&
         !nsLayoutUtils::IsPopup(*aOutAncestor) &&
         *aOutAncestor != aStopAtAncestor) {
    
    nsIFrame* parent = nsLayoutUtils::GetCrossDocParentFrame(*aOutAncestor);
    if (!parent)
      break;

    *aOutAncestor = parent;
  }

  NS_ASSERTION(*aOutAncestor, "Somehow ended up with a null ancestor...?");

  


  nsPoint delta = GetOffsetToCrossDoc(*aOutAncestor);
  int32_t scaleFactor = PresContext()->AppUnitsPerDevPixel();
  return gfx3DMatrix().Translation
    (NSAppUnitsToFloatPixels(delta.x, scaleFactor),
     NSAppUnitsToFloatPixels(delta.y, scaleFactor),
     0.0f);
}

static void InvalidateFrameInternal(nsIFrame *aFrame, bool aHasDisplayItem = true)
{
  if (aHasDisplayItem) {
    aFrame->AddStateBits(NS_FRAME_NEEDS_PAINT);
  }
  nsSVGEffects::InvalidateDirectRenderingObservers(aFrame);
  bool needsSchedulePaint = false;
  if (nsLayoutUtils::IsPopup(aFrame)) {
    needsSchedulePaint = true;
  } else {
    nsIFrame *parent = nsLayoutUtils::GetCrossDocParentFrame(aFrame);
    while (parent && !parent->HasAnyStateBits(NS_FRAME_DESCENDANT_NEEDS_PAINT)) {
      if (aHasDisplayItem) {
        parent->AddStateBits(NS_FRAME_DESCENDANT_NEEDS_PAINT);
      }
      nsSVGEffects::InvalidateDirectRenderingObservers(parent);

      
      
      
      if (nsLayoutUtils::IsPopup(parent)) {
        needsSchedulePaint = true;
        break;
      }
      parent = nsLayoutUtils::GetCrossDocParentFrame(parent);
    }
    if (!parent) {
      needsSchedulePaint = true;
    }
  }
  if (!aHasDisplayItem) {
    return;
  }
  if (needsSchedulePaint) {
    aFrame->SchedulePaint();
  }
  if (aFrame->HasAnyStateBits(NS_FRAME_HAS_INVALID_RECT)) {
    aFrame->Properties().Delete(nsIFrame::InvalidationRect());
    aFrame->RemoveStateBits(NS_FRAME_HAS_INVALID_RECT);
  }
}

void
nsIFrame::InvalidateFrameSubtree(uint32_t aDisplayItemKey)
{
  bool hasDisplayItem = 
    !aDisplayItemKey || FrameLayerBuilder::HasRetainedDataFor(this, aDisplayItemKey);
  InvalidateFrame(aDisplayItemKey);

  if (HasAnyStateBits(NS_FRAME_ALL_DESCENDANTS_NEED_PAINT) || !hasDisplayItem) {
    return;
  }

  AddStateBits(NS_FRAME_ALL_DESCENDANTS_NEED_PAINT);
  
  nsAutoTArray<nsIFrame::ChildList,4> childListArray;
  GetCrossDocChildLists(&childListArray);

  nsIFrame::ChildListArrayIterator lists(childListArray);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      childFrames.get()->InvalidateFrameSubtree();
    }
  }
}

void
nsIFrame::ClearInvalidationStateBits()
{
  if (HasAnyStateBits(NS_FRAME_DESCENDANT_NEEDS_PAINT)) {
    nsAutoTArray<nsIFrame::ChildList,4> childListArray;
    GetCrossDocChildLists(&childListArray);

    nsIFrame::ChildListArrayIterator lists(childListArray);
    for (; !lists.IsDone(); lists.Next()) {
      nsFrameList::Enumerator childFrames(lists.CurrentList());
      for (; !childFrames.AtEnd(); childFrames.Next()) {
        childFrames.get()->ClearInvalidationStateBits();
      }
    }
  }

  RemoveStateBits(NS_FRAME_NEEDS_PAINT | 
                  NS_FRAME_DESCENDANT_NEEDS_PAINT | 
                  NS_FRAME_ALL_DESCENDANTS_NEED_PAINT);
}

void
nsIFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  bool hasDisplayItem = 
    !aDisplayItemKey || FrameLayerBuilder::HasRetainedDataFor(this, aDisplayItemKey);
  InvalidateFrameInternal(this, hasDisplayItem);
}

void
nsIFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  bool hasDisplayItem = 
    !aDisplayItemKey || FrameLayerBuilder::HasRetainedDataFor(this, aDisplayItemKey);
  bool alreadyInvalid = false;
  if (!HasAnyStateBits(NS_FRAME_NEEDS_PAINT)) {
    InvalidateFrameInternal(this, hasDisplayItem);
  } else {
    alreadyInvalid = true;
  } 

  if (!hasDisplayItem) {
    return;
  }

  nsRect *rect = static_cast<nsRect*>(Properties().Get(InvalidationRect()));
  if (!rect) {
    if (alreadyInvalid) {
      return;
    }
    rect = new nsRect();
    Properties().Set(InvalidationRect(), rect);
    AddStateBits(NS_FRAME_HAS_INVALID_RECT);
  }

  *rect = rect->Union(aRect);
}

 uint8_t nsIFrame::sLayerIsPrerenderedDataKey;

bool
nsIFrame::TryUpdateTransformOnly()
{
  Layer* layer = FrameLayerBuilder::GetDedicatedLayer(
    this, nsDisplayItem::TYPE_TRANSFORM);
  if (!layer || !layer->HasUserData(LayerIsPrerenderedDataKey())) {
    
    
    
    return false;
  }

  gfx3DMatrix transform3d;
  if (!nsLayoutUtils::GetLayerTransformForFrame(this, &transform3d)) {
    
    
    
    return false;
  }
  gfxMatrix transform, previousTransform;
  
  
  
  
  
  
  
 static const gfx::Float kError = 0.0001;
  if (!transform3d.Is2D(&transform) ||
      !layer->GetBaseTransform().Is2D(&previousTransform) ||
      !gfx::FuzzyEqual(transform.xx, previousTransform.xx, kError) ||
      !gfx::FuzzyEqual(transform.yy, previousTransform.yy, kError) ||
      !gfx::FuzzyEqual(transform.xy, previousTransform.xy, kError) ||
      !gfx::FuzzyEqual(transform.yx, previousTransform.yx, kError)) {
    return false;
  }
  layer->SetBaseTransformForNextTransaction(transform3d);
  return true;
}

bool 
nsIFrame::IsInvalid(nsRect& aRect)
{
  if (!HasAnyStateBits(NS_FRAME_NEEDS_PAINT)) {
    return false;
  }
  
  if (HasAnyStateBits(NS_FRAME_HAS_INVALID_RECT)) {
    nsRect *rect = static_cast<nsRect*>(Properties().Get(InvalidationRect()));
    NS_ASSERTION(rect, "Must have an invalid rect if NS_FRAME_HAS_INVALID_RECT is set!");
    aRect = *rect;
  } else {
    aRect.SetEmpty();
  }
  return true;
}

void
nsIFrame::SchedulePaint(uint32_t aFlags)
{
  nsIFrame *displayRoot = nsLayoutUtils::GetDisplayRootFrame(this);
  nsPresContext *pres = displayRoot->PresContext()->GetRootPresContext();

  
  
  if (!pres || (pres->Document() && pres->Document()->GetDisplayDocument())) {
    return;
  }
  
  pres->PresShell()->ScheduleViewManagerFlush();
  if (!(aFlags & PAINT_COMPOSITE_ONLY)) {
    displayRoot->AddStateBits(NS_FRAME_UPDATE_LAYER_TREE);
  }
  nsIPresShell* shell = PresContext()->PresShell();
  if (shell) {
    shell->AddInvalidateHiddenPresShellObserver(pres->RefreshDriver());
  }
}

Layer*
nsIFrame::InvalidateLayer(uint32_t aDisplayItemKey, const nsIntRect* aDamageRect)
{
  NS_ASSERTION(aDisplayItemKey > 0, "Need a key");

  Layer* layer = FrameLayerBuilder::GetDedicatedLayer(this, aDisplayItemKey);
  if (aDamageRect && aDamageRect->IsEmpty()) {
    return layer;
  }

  if (!layer) {
    
    
    
    if (aDisplayItemKey == nsDisplayItem::TYPE_PLUGIN) {
      InvalidateFrame();
    } else {
      InvalidateFrame(aDisplayItemKey);
    }
    return nullptr;
  }

  if (aDamageRect) {
    layer->AddInvalidRect(*aDamageRect);
  } else {
    layer->SetInvalidRectToVisibleRegion();
  }

  SchedulePaint(PAINT_COMPOSITE_ONLY);
  return layer;
}

NS_DECLARE_FRAME_PROPERTY(DeferInvalidatesProperty, nsIFrame::DestroyRegion)

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
ComputeOutlineAndEffectsRect(nsIFrame* aFrame,
                             const nsRect& aOverflowRect,
                             const nsSize& aNewSize,
                             bool aStoreRectProperties) {
  nsRect r = aOverflowRect;

  if (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    
    
    
    if (aFrame->GetStyleSVGReset()->mFilter) {
      if (aStoreRectProperties) {
        aFrame->Properties().
          Set(nsIFrame::PreEffectsBBoxProperty(), new nsRect(r));
      }
      r = nsSVGUtils::GetPostFilterVisualOverflowRect(aFrame, aOverflowRect);
    }
    return r;
  }

  
  r.UnionRect(r, nsLayoutUtils::GetBoxShadowRectForFrame(aFrame, aNewSize));

  const nsStyleOutline* outline = aFrame->GetStyleOutline();
  uint8_t outlineStyle = outline->GetOutlineStyle();
  if (outlineStyle != NS_STYLE_BORDER_STYLE_NONE) {
    nscoord width;
#ifdef DEBUG
    bool result = 
#endif
      outline->GetOutlineWidth(width);
    NS_ASSERTION(result, "GetOutlineWidth had no cached outline width");
    if (width > 0) {
      if (aStoreRectProperties) {
        aFrame->Properties().
          Set(nsIFrame::OutlineInnerRectProperty(), new nsRect(r));
      }

      nscoord offset = outline->mOutlineOffset;
      nscoord inflateBy = std::max(width + offset, 0);
      
      
      
      
      
      r.Inflate(inflateBy, inflateBy);
    }
  }

  
  
  

  
  
  
  
  
  
  
  
  const nsStyleBorder* styleBorder = aFrame->GetStyleBorder();
  nsMargin outsetMargin = styleBorder->GetImageOutset();

  if (outsetMargin != nsMargin(0, 0, 0, 0)) {
    nsRect outsetRect(nsPoint(0, 0), aNewSize);
    outsetRect.Inflate(outsetMargin);
    r.UnionRect(r, outsetRect);
  }

  
  
  
  
  
  

  if (nsSVGIntegrationUtils::UsingEffectsForFrame(aFrame)) {
    if (aStoreRectProperties) {
      aFrame->Properties().
        Set(nsIFrame::PreEffectsBBoxProperty(), new nsRect(r));
    }
    r = nsSVGIntegrationUtils::ComputePostEffectsVisualOverflowRect(aFrame, r);
  }

  return r;
}

nsPoint
nsIFrame::GetRelativeOffset(const nsStyleDisplay* aDisplay) const
{
  if (!aDisplay ||
      aDisplay->IsRelativelyPositioned(this)) {
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
    nsOverflowAreas* preTransformOverflows = static_cast<nsOverflowAreas*>
      (Properties().Get(PreTransformOverflowAreasProperty()));
    if (preTransformOverflows)
      return preTransformOverflows->VisualOverflow();
  }
  return GetVisualOverflowRect();
}

nsRect
nsIFrame::GetPreEffectsVisualOverflowRect() const
{
  nsRect* r = static_cast<nsRect*>
    (Properties().Get(nsIFrame::PreEffectsBBoxProperty()));
  return r ? *r : GetVisualOverflowRectRelativeToSelf();
}

 bool
nsFrame::UpdateOverflow()
{
  MOZ_ASSERT(!(mState & NS_FRAME_SVG_LAYOUT) ||
             !(mState & NS_STATE_SVG_NONDISPLAY_CHILD),
             "Non-display SVG do not maintain visual overflow rects");

  nsRect rect(nsPoint(0, 0), GetSize());
  nsOverflowAreas overflowAreas(rect, rect);

  bool isBox = IsBoxFrame() || IsBoxWrapped();
  if (!isBox || (!IsCollapsed() && !DoesClipChildren())) {
    nsLayoutUtils::UnionChildOverflow(this, overflowAreas);
  }

  if (FinishAndStoreOverflow(overflowAreas, GetSize())) {
    nsView* view = GetView();
    if (view) {
      uint32_t flags = 0;
      GetLayoutFlags(flags);

      if ((flags & NS_FRAME_NO_SIZE_VIEW) == 0) {
        
        nsViewManager* vm = view->GetViewManager();
        vm->ResizeView(view, overflowAreas.VisualOverflow(), true);
      }
    }

    return true;
  }

  return false;
}




#define MAX_FRAME_DEPTH (MAX_REFLOW_DEPTH+4)

bool
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

    return true;
  }
  mState &= ~NS_FRAME_TOO_DEEP_IN_FRAME_TREE;
  return false;
}

bool
nsIFrame::IsBlockWrapper() const
{
  nsIAtom *pseudoType = GetStyleContext()->GetPseudo();
  return (pseudoType == nsCSSAnonBoxes::mozAnonymousBlock ||
          pseudoType == nsCSSAnonBoxes::mozAnonymousPositionedBlock ||
          pseudoType == nsCSSAnonBoxes::cellContent);
}

static nsIFrame*
GetNearestBlockContainer(nsIFrame* frame)
{
  
  
  
  
  
  
  while (frame->IsFrameOfType(nsIFrame::eLineParticipant) ||
         frame->IsBlockWrapper() ||
         
         frame->GetType() == nsGkAtoms::tableRowFrame) {
    frame = frame->GetParent();
    NS_ASSERTION(frame, "How come we got to the root frame without seeing a containing block?");
  }
  return frame;
}

nsIFrame*
nsIFrame::GetContainingBlock() const
{
  
  
  
  if (IsAbsolutelyPositioned() &&
      (GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    return GetParent(); 
  }
  return GetNearestBlockContainer(GetParent());
}

#ifdef DEBUG

int32_t nsFrame::ContentIndexInContainer(const nsIFrame* aFrame)
{
  int32_t result = -1;

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
nsFrame::List(FILE* out, int32_t aIndent, uint32_t aFlags) const
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
    fprintf(out, " [state=%016llx]", (unsigned long long)mState);
  }
  nsIFrame* prevInFlow = GetPrevInFlow();
  nsIFrame* nextInFlow = GetNextInFlow();
  if (nullptr != prevInFlow) {
    fprintf(out, " prev-in-flow=%p", static_cast<void*>(prevInFlow));
  }
  if (nullptr != nextInFlow) {
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
    if (GetType() == nsGkAtoms::subDocumentFrame) {
      nsAutoString src;
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src);
      buf.Append(NS_LITERAL_STRING(" src=") + src);
    }
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
  int32_t i, len = aString.Length();
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

bool
nsIFrame::IsVisibleForPainting(nsDisplayListBuilder* aBuilder) {
  if (!GetStyleVisibility()->IsVisible())
    return false;
  nsISelection* sel = aBuilder->GetBoundingSelection();
  return !sel || IsVisibleInSelection(sel);
}

bool
nsIFrame::IsVisibleForPainting() {
  if (!GetStyleVisibility()->IsVisible())
    return false;

  nsPresContext* pc = PresContext();
  if (!pc->IsRenderingOnlySelection())
    return true;

  nsCOMPtr<nsISelectionController> selcon(do_QueryInterface(pc->PresShell()));
  if (selcon) {
    nsCOMPtr<nsISelection> sel;
    selcon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                         getter_AddRefs(sel));
    if (sel)
      return IsVisibleInSelection(sel);
  }
  return true;
}

bool
nsIFrame::IsVisibleInSelection(nsDisplayListBuilder* aBuilder) {
  nsISelection* sel = aBuilder->GetBoundingSelection();
  return !sel || IsVisibleInSelection(sel);
}

bool
nsIFrame::IsVisibleOrCollapsedForPainting(nsDisplayListBuilder* aBuilder) {
  if (!GetStyleVisibility()->IsVisibleOrCollapsed())
    return false;
  nsISelection* sel = aBuilder->GetBoundingSelection();
  return !sel || IsVisibleInSelection(sel);
}

bool
nsIFrame::IsVisibleInSelection(nsISelection* aSelection)
{
  if (!GetContent() || !GetContent()->IsSelectionDescendant()) {
    return false;
  }
  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mContent));
  bool vis;
  nsresult rv = aSelection->ContainsNode(node, true, &vis);
  return NS_FAILED(rv) || vis;
}

 bool
nsFrame::IsEmpty()
{
  return false;
}

bool
nsIFrame::CachedIsEmpty()
{
  NS_PRECONDITION(!(GetStateBits() & NS_FRAME_IS_DIRTY),
                  "Must only be called on reflowed lines");
  return IsEmpty();
}

 bool
nsFrame::IsSelfEmpty()
{
  return false;
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
nsIFrame::GetConstFrameSelection() const
{
  nsIFrame* frame = const_cast<nsIFrame*>(this);
  while (frame && (frame->GetStateBits() & NS_FRAME_INDEPENDENT_SELECTION)) {
    nsITextControlFrame* tcf = do_QueryFrame(frame);
    if (tcf) {
      return tcf->GetOwnedFrameSelection();
    }
    frame = frame->GetParent();
  }

  return PresContext()->PresShell()->ConstFrameSelection();
}

#ifdef DEBUG
NS_IMETHODIMP
nsFrame::DumpRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent)
{
  IndentBy(out, aIndent);
  fprintf(out, "<frame va=\"%p\" type=\"", (void*)this);
  nsAutoString name;
  GetFrameName(name);
  XMLQuote(name);
  fputs(NS_LossyConvertUTF16toASCII(name).get(), out);
  fprintf(out, "\" state=\"%016llx\" parent=\"%p\">\n",
          (unsigned long long)GetDebugStateBits(), (void*)mParent);

  aIndent++;
  DumpBaseRegressionData(aPresContext, out, aIndent);
  aIndent--;

  IndentBy(out, aIndent);
  fprintf(out, "</frame>\n");

  return NS_OK;
}

void
nsFrame::DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent)
{
  if (GetNextSibling()) {
    IndentBy(out, aIndent);
    fprintf(out, "<next-sibling va=\"%p\"/>\n", (void*)GetNextSibling());
  }

  if (HasView()) {
    IndentBy(out, aIndent);
    fprintf(out, "<view va=\"%p\">\n", (void*)GetView());
    aIndent++;
    
    aIndent--;
    IndentBy(out, aIndent);
    fprintf(out, "</view>\n");
  }

  IndentBy(out, aIndent);
  fprintf(out, "<bbox x=\"%d\" y=\"%d\" w=\"%d\" h=\"%d\"/>\n",
          mRect.x, mRect.y, mRect.width, mRect.height);

  
  ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    IndentBy(out, aIndent);
    if (lists.CurrentID() != kPrincipalList) {
      fprintf(out, "<child-list name=\"%s\">\n", mozilla::layout::ChildListName(lists.CurrentID()));
    }
    else {
      fprintf(out, "<child-list>\n");
    }
    aIndent++;
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* kid = childFrames.get();
      kid->DumpRegressionData(aPresContext, out, aIndent);
    }
    aIndent--;
    IndentBy(out, aIndent);
    fprintf(out, "</child-list>\n");
  }
}
#endif

bool
nsIFrame::IsFrameSelected() const
{
  NS_ASSERTION(!GetContent() || GetContent()->IsSelectionDescendant(),
               "use the public IsSelected() instead");
  return nsRange::IsNodeSelected(GetContent(), 0,
                                 GetContent()->GetChildCount());
}

NS_IMETHODIMP
nsFrame::GetPointFromOffset(int32_t inOffset, nsPoint* outPoint)
{
  NS_PRECONDITION(outPoint != nullptr, "Null parameter");
  nsRect contentRect = GetContentRect() - GetPosition();
  nsPoint pt = contentRect.TopLeft();
  if (mContent)
  {
    nsIContent* newContent = mContent->GetParent();
    if (newContent){
      int32_t newOffset = newContent->IndexOf(mContent);

      bool isRTL = (NS_GET_EMBEDDING_LEVEL(this) & 1) == 1;
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
nsFrame::GetChildFrameContainingOffset(int32_t inContentOffset, bool inHint, int32_t* outFrameContentOffset, nsIFrame **outChildFrame)
{
  NS_PRECONDITION(outChildFrame && outFrameContentOffset, "Null parameter");
  *outFrameContentOffset = (int32_t)inHint;
  
  
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
                                        int32_t aLineStart, 
                                        int8_t aOutSideLimit
                                        )
{
  
  if (!aBlockFrame || !aPos)
    return NS_ERROR_NULL_POINTER;

  aPos->mResultFrame = nullptr;
  aPos->mResultContent = nullptr;
  aPos->mAttachForward = (aPos->mDirection == eDirNext);

  nsAutoLineIterator it = aBlockFrame->GetLineIterator();
  if (!it)
    return NS_ERROR_FAILURE;
  int32_t searchingLine = aLineStart;
  int32_t countLines = it->GetNumLines();
  if (aOutSideLimit > 0) 
    searchingLine = countLines;
  else if (aOutSideLimit <0)
    searchingLine = -1;
  else 
    if ((aPos->mDirection == eDirPrevious && searchingLine == 0) || 
       (aPos->mDirection == eDirNext && searchingLine >= (countLines -1) )){
      
           return NS_ERROR_FAILURE;
    }
  int32_t lineFrameCount;
  nsIFrame *resultFrame = nullptr;
  nsIFrame *farStoppingFrame = nullptr; 
  nsIFrame *nearStoppingFrame = nullptr; 
  nsIFrame *firstFrame;
  nsIFrame *lastFrame;
  nsRect  rect;
  bool isBeforeFirstFrame, isAfterLastFrame;
  bool found = false;

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
    uint32_t lineFlags;
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
      nsView * view; 
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
                                    false, 
                                    aPos->mScrollViewStop,
                                    false     
                                    );
      if (NS_FAILED(result))
        return result;

      nsIFrame *storeOldResultFrame = resultFrame;
      while ( !found ){
        nsPoint point;
        point.x = aPos->mDesiredX;

        nsRect tempRect = resultFrame->GetRect();
        nsPoint offset;
        nsView * view; 
        result = resultFrame->GetOffsetFromView(offset, &view);
        if (NS_FAILED(result))
          return result;
        point.y = tempRect.height + offset.y;

        
        
        nsIPresShell *shell = aPresContext->GetPresShell();
        if (!shell)
          return NS_ERROR_FAILURE;
        int16_t isEditor = shell->GetSelectionFlags();
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
                  aPos->mAttachForward = false;
                  if ((point.x - offset.x+ tempRect.x)>tempRect.width)
                  {
                    aPos->mContentOffset++;
                    aPos->mAttachForward = true;
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
          nsView* view;
          nsPoint offset;
          resultFrame->GetOffsetFromView(offset, &view);
          ContentOffsets offsets =
              resultFrame->GetContentOffsetsFromPoint(point - offset);
          aPos->mResultContent = offsets.content;
          aPos->mContentOffset = offsets.offset;
          aPos->mAttachForward = offsets.associateWithNext;
          if (offsets.content)
          {
            bool selectable;
            resultFrame->IsSelectable(&selectable, nullptr);
            if (selectable)
            {
              found = true;
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
                                      false, 
                                      aPos->mScrollViewStop,
                                      false     
                                      );
      }
      while ( !found ){
        nsPoint point(aPos->mDesiredX, 0);
        nsView* view;
        nsPoint offset;
        resultFrame->GetOffsetFromView(offset, &view);
        ContentOffsets offsets =
            resultFrame->GetContentOffsetsFromPoint(point - offset);
        aPos->mResultContent = offsets.content;
        aPos->mContentOffset = offsets.offset;
        aPos->mAttachForward = offsets.associateWithNext;
        if (offsets.content)
        {
          bool selectable;
          resultFrame->IsSelectable(&selectable, nullptr);
          if (selectable)
          {
            found = true;
            if (resultFrame == farStoppingFrame)
              aPos->mAttachForward = false;
            else
              aPos->mAttachForward = true;
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

nsIFrame::CaretPosition
nsIFrame::GetExtremeCaretPosition(bool aStart)
{
  CaretPosition result;

  FrameTarget targetFrame = DrillDownToSelectionFrame(this, !aStart, 0);
  FrameContentRange range = GetRangeForFrame(targetFrame.frame);
  result.mResultContent = range.content;
  result.mContentOffset = aStart ? range.start : range.end;
  return result;
}



static nsContentAndOffset
FindBlockFrameOrBR(nsIFrame* aFrame, nsDirection aDirection)
{
  nsContentAndOffset result;
  result.mContent =  nullptr;
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
    int32_t startOffset, endOffset;
    aFrame->GetOffsets(startOffset, endOffset);
    result.mContent = aFrame->GetContent();
    result.mOffset = endOffset - (aDirection == eDirPrevious ? 0 : 1);
    return result;
  }

  
  if (aDirection == eDirPrevious) {
    nsIFrame* child = aFrame->GetLastChild(nsIFrame::kPrincipalList);
    while(child && !result.mContent) {
      result = FindBlockFrameOrBR(child, aDirection);
      child = child->GetPrevSibling();
    }
  } else { 
    nsIFrame* child = aFrame->GetFirstPrincipalChild();
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
  blockFrameOrBR.mContent = nullptr;
  bool reachedBlockAncestor = false;

  
  
  
  
  
  if (aPos->mDirection == eDirPrevious) {
    while (!reachedBlockAncestor) {
      nsIFrame* parent = frame->GetParent();
      
      if (!frame->mContent || !frame->mContent->GetParent()) {
        reachedBlockAncestor = true;
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
      reachedBlockAncestor = (nsLayoutUtils::GetAsBlock(frame) != nullptr);
    }
    if (reachedBlockAncestor) { 
      aPos->mResultContent = frame->GetContent();
      aPos->mContentOffset = 0;
    }
  } else { 
    while (!reachedBlockAncestor) {
      nsIFrame* parent = frame->GetParent();
      
      if (!frame->mContent || !frame->mContent->GetParent()) {
        reachedBlockAncestor = true;
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
      reachedBlockAncestor = (nsLayoutUtils::GetAsBlock(frame) != nullptr);
    }
    if (reachedBlockAncestor) { 
      aPos->mResultContent = frame->GetContent();
      if (aPos->mResultContent)
        aPos->mContentOffset = aPos->mResultContent->GetChildCount();
    }
  }
  return NS_OK;
}


static bool IsMovingInFrameDirection(nsIFrame* frame, nsDirection aDirection, bool aVisual)
{
  bool isReverseDirection = aVisual ?
    (NS_GET_EMBEDDING_LEVEL(frame) & 1) != (NS_GET_BASE_LEVEL(frame) & 1) : false;
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
  int32_t offset = aPos->mStartOffset - range.start;
  nsIFrame* current = this;
  
  switch (aPos->mAmount) {
    case eSelectCharacter:
    case eSelectCluster:
    {
      bool eatingNonRenderableWS = false;
      bool done = false;
      bool jumpedLine = false;
      
      while (!done) {
        bool movingInFrameDirection =
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
            eatingNonRenderableWS = true;
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
      
      
      
      
      bool wordSelectEatSpace;
      if (aPos->mWordMovementType != eDefaultBehavior) {
        
        
        
        wordSelectEatSpace = ((aPos->mWordMovementType == eEndWord) == (aPos->mDirection == eDirPrevious));
      }
      else {
        
        
        
        wordSelectEatSpace = aPos->mDirection == eDirNext &&
          Preferences::GetBool("layout.word_select.eat_space_to_next_word");
      }
      
      
      
      
      
      
      
      
      
      PeekWordState state;
      int32_t offsetAdjustment = 0;
      bool done = false;
      while (!done) {
        bool movingInFrameDirection =
          IsMovingInFrameDirection(current, aPos->mDirection, aPos->mVisual);
        
        done = current->PeekOffsetWord(movingInFrameDirection, wordSelectEatSpace,
                                       aPos->mIsKeyboardSelect, &offset, &state);
        
        if (!done) {
          nsIFrame* nextFrame;
          int32_t nextFrameOffset;
          bool jumpedLine;
          result =
            current->GetFrameFromDirection(aPos->mDirection, aPos->mVisual,
                                           aPos->mJumpLines, aPos->mScrollViewStop,
                                           &nextFrame, &nextFrameOffset, &jumpedLine);
          
          
          if (NS_FAILED(result) ||
              (jumpedLine && !wordSelectEatSpace && state.mSawBeforeType)) {
            done = true;
            
            
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
        int32_t thisLine = nsFrame::GetLineNumber(blockFrame, aPos->mScrollViewStop, &blockFrame);
        if (thisLine < 0) 
          return  NS_ERROR_FAILURE;
        iter = blockFrame->GetLineIterator();
        NS_ASSERTION(iter, "GetLineNumber() succeeded but no block frame?");
        result = NS_OK;

        int edgeCase = 0;
        
        bool doneLooping = false;
        
        
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
            aPos->mResultFrame = nullptr;
            if (aPos->mDirection == eDirPrevious)
              thisLine--;
            else
              thisLine++;
          }
          else 
            doneLooping = true; 

          lastFrame = aPos->mResultFrame; 

          if (NS_SUCCEEDED(result) && aPos->mResultFrame 
            && blockFrame != aPos->mResultFrame)
          {





            bool searchTableBool = false;
            if (aPos->mResultFrame->GetType() == nsGkAtoms::tableOuterFrame ||
                aPos->mResultFrame->GetType() == nsGkAtoms::tableCellFrame)
            {
              nsIFrame *frame = aPos->mResultFrame->GetFirstPrincipalChild();
              
              while(frame) 
              {
                iter = frame->GetLineIterator();
                if (iter)
                {
                  aPos->mResultFrame = frame;
                  searchTableBool = true;
                  result = NS_OK;
                  break; 
                }
                result = NS_ERROR_FAILURE;
                frame = frame->GetFirstPrincipalChild();
              }
            }

            if (!searchTableBool) {
              iter = aPos->mResultFrame->GetLineIterator();
              result = iter ? NS_OK : NS_ERROR_FAILURE;
            }
            if (NS_SUCCEEDED(result) && iter)
            {
              doneLooping = false;
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
      int32_t thisLine = nsFrame::GetLineNumber(blockFrame, aPos->mScrollViewStop, &blockFrame);
      if (thisLine < 0)
        return NS_ERROR_FAILURE;
      nsAutoLineIterator it = blockFrame->GetLineIterator();
      NS_ASSERTION(it, "GetLineNumber() succeeded but no block frame?");

      int32_t lineFrameCount;
      nsIFrame *firstFrame;
      nsRect usedRect;
      uint32_t lineFlags;
      nsIFrame* baseFrame = nullptr;
      bool endOfLine = (eSelectEndLine == aPos->mAmount);
      
#ifdef IBMBIDI
      if (aPos->mVisual && PresContext()->BidiEnabled()) {
        bool lineIsRTL = it->GetDirection();
        bool isReordered;
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
        for (int32_t count = lineFrameCount; count;
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
                                                          endOfLine, 0);
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
      NS_ASSERTION(false, "Invalid amount");
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

bool
nsFrame::PeekOffsetNoAmount(bool aForward, int32_t* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return true;
}

bool
nsFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                             bool aRespectClusters)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  int32_t startOffset = *aOffset;
  
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    *aOffset = 1 - startOffset;
    return true;
  }
  return false;
}

bool
nsFrame::PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
                        int32_t* aOffset, PeekWordState* aState)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  int32_t startOffset = *aOffset;
  
  aState->mContext.Truncate();
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    if (!aState->mAtStart) {
      if (aState->mLastCharWasPunctuation) {
        
        if (BreakWordBetweenPunctuation(aState, aForward, false, false, aIsKeyboardSelect))
          return true;
      } else {
        
        if (aWordSelectEatSpace && aState->mSawBeforeType)
          return true;
      }
    }
    
    *aOffset = 1 - startOffset;
    aState->Update(false, 
                   false     
                   );
    if (!aWordSelectEatSpace)
      aState->SetSawBeforeType();
  }
  return false;
}

bool
nsFrame::BreakWordBetweenPunctuation(const PeekWordState* aState,
                                     bool aForward,
                                     bool aPunctAfter, bool aWhitespaceAfter,
                                     bool aIsKeyboardSelect)
{
  NS_ASSERTION(aPunctAfter != aState->mLastCharWasPunctuation,
               "Call this only at punctuation boundaries");
  if (aState->mLastCharWasWhitespace) {
    
    return true;
  }
  if (!Preferences::GetBool("layout.word_select.stop_at_punctuation")) {
    
    
    return false;
  }
  if (!aIsKeyboardSelect) {
    
    return true;
  }
  bool afterPunct = aForward ? aState->mLastCharWasPunctuation : aPunctAfter;
  if (!afterPunct) {
    
    return false;
  }
  
  
  return aState->mSeenNonPunctuationSinceWhitespace;
}

NS_IMETHODIMP
nsFrame::CheckVisibility(nsPresContext* , int32_t , int32_t , bool , bool *, bool *)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


int32_t
nsFrame::GetLineNumber(nsIFrame *aFrame, bool aLockScroll, nsIFrame** aContainingBlock)
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
nsIFrame::GetFrameFromDirection(nsDirection aDirection, bool aVisual,
                                bool aJumpLines, bool aScrollViewStop, 
                                nsIFrame** aOutFrame, int32_t* aOutOffset, bool* aOutJumpedLine)
{
  nsresult result;

  if (!aOutFrame || !aOutOffset || !aOutJumpedLine)
    return NS_ERROR_NULL_POINTER;
  
  nsPresContext* presContext = PresContext();
  *aOutFrame = nullptr;
  *aOutOffset = 0;
  *aOutJumpedLine = false;

  
  bool selectable = false;
  nsIFrame *traversedFrame = this;
  while (!selectable) {
    nsIFrame *blockFrame;
    
    int32_t thisLine = nsFrame::GetLineNumber(traversedFrame, aScrollViewStop, &blockFrame);
    if (thisLine < 0)
      return NS_ERROR_FAILURE;

    nsAutoLineIterator it = blockFrame->GetLineIterator();
    NS_ASSERTION(it, "GetLineNumber() succeeded but no block frame?");

    bool atLineEdge;
    nsIFrame *firstFrame;
    nsIFrame *lastFrame;
#ifdef IBMBIDI
    if (aVisual && presContext->BidiEnabled()) {
      bool lineIsRTL = it->GetDirection();
      bool isReordered;
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
        atLineEdge = true;
      }
    } else
#endif
    {
      nsRect  nonUsedRect;
      int32_t lineFrameCount;
      uint32_t lineFlags;
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
      *aOutJumpedLine = true;
      if (!aJumpLines)
        return NS_ERROR_FAILURE; 
    }

    nsCOMPtr<nsIFrameEnumerator> frameTraversal;
    result = NS_NewFrameTraversal(getter_AddRefs(frameTraversal),
                                  presContext, traversedFrame,
                                  eLeaf,
                                  aVisual && presContext->BidiEnabled(),
                                  aScrollViewStop,
                                  true     
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
    traversedFrame->IsSelectable(&selectable, nullptr);
  } 

  *aOutOffset = (aDirection == eDirNext) ? 0 : -1;

#ifdef IBMBIDI
  if (aVisual) {
    uint8_t newLevel = NS_GET_EMBEDDING_LEVEL(traversedFrame);
    uint8_t newBaseLevel = NS_GET_BASE_LEVEL(traversedFrame);
    if ((newLevel & 1) != (newBaseLevel & 1)) 
      *aOutOffset = -1 - *aOutOffset;
  }
#endif
  *aOutFrame = traversedFrame;
  return NS_OK;
}

nsView* nsIFrame::GetClosestView(nsPoint* aOffset) const
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
  return nullptr;
}


 void
nsFrame::ChildIsDirty(nsIFrame* aChild)
{
  NS_NOTREACHED("should never be called on a frame that doesn't inherit from "
                "nsContainerFrame");
}


#ifdef ACCESSIBILITY
a11y::AccType
nsFrame::AccessibleType()
{
  return a11y::eNoType;
}
#endif

NS_DECLARE_FRAME_PROPERTY(OverflowAreasProperty,
                          nsIFrame::DestroyOverflowAreas)

bool
nsIFrame::ClearOverflowRects()
{
  if (mOverflow.mType == NS_FRAME_OVERFLOW_NONE) {
    return false;
  }
  if (mOverflow.mType == NS_FRAME_OVERFLOW_LARGE) {
    Properties().Delete(OverflowAreasProperty());
  }
  mOverflow.mType = NS_FRAME_OVERFLOW_NONE;
  return true;
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




bool
nsIFrame::SetOverflowAreas(const nsOverflowAreas& aOverflowAreas)
{
  if (mOverflow.mType == NS_FRAME_OVERFLOW_LARGE) {
    nsOverflowAreas *overflow =
      static_cast<nsOverflowAreas*>(Properties().Get(OverflowAreasProperty()));
    bool changed = *overflow != aOverflowAreas;
    *overflow = aOverflowAreas;

    
    
    return changed;
  }

  const nsRect& vis = aOverflowAreas.VisualOverflow();
  uint32_t l = -vis.x, 
           t = -vis.y, 
           r = vis.XMost() - mRect.width, 
           b = vis.YMost() - mRect.height; 
  if (aOverflowAreas.ScrollableOverflow().IsEqualEdges(nsRect(nsPoint(0, 0), GetSize())) &&
      l <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      t <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      r <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      b <= NS_FRAME_OVERFLOW_DELTA_MAX &&
      
      
      
      
      
      
      
      
      (l | t | r | b) != 0) {
    VisualDeltas oldDeltas = mOverflow.mVisualDeltas;
    
    
    
    
    mOverflow.mVisualDeltas.mLeft   = l;
    mOverflow.mVisualDeltas.mTop    = t;
    mOverflow.mVisualDeltas.mRight  = r;
    mOverflow.mVisualDeltas.mBottom = b;
    
    return oldDeltas != mOverflow.mVisualDeltas;
  } else {
    bool changed = !aOverflowAreas.ScrollableOverflow().IsEqualEdges(nsRect(nsPoint(0, 0), GetSize())) ||
      !aOverflowAreas.VisualOverflow().IsEqualEdges(GetVisualOverflowFromDeltas());

    
    mOverflow.mType = NS_FRAME_OVERFLOW_LARGE;
    nsOverflowAreas* overflow = GetOverflowAreasProperty();
    NS_ASSERTION(overflow, "should have created areas");
    *overflow = aOverflowAreas;
    return changed;
  }
}

inline bool
IsInlineFrame(nsIFrame *aFrame)
{
  nsIAtom *type = aFrame->GetType();
  return type == nsGkAtoms::inlineFrame;
}

bool
nsIFrame::FinishAndStoreOverflow(nsOverflowAreas& aOverflowAreas,
                                 nsSize aNewSize, nsSize* aOldSize)
{
  NS_ASSERTION(!((GetStateBits() & NS_FRAME_SVG_LAYOUT) &&
                 (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)),
               "Don't call - overflow rects not maintained on these SVG frames");

  nsRect bounds(nsPoint(0, 0), aNewSize);
  
  
  if ((Preserves3D() || HasPerspective()) && (!aOverflowAreas.VisualOverflow().IsEqualEdges(bounds) ||
                        !aOverflowAreas.ScrollableOverflow().IsEqualEdges(bounds))) {
    nsOverflowAreas* initial =
      static_cast<nsOverflowAreas*>(Properties().Get(nsIFrame::InitialOverflowProperty()));
    if (!initial) {
      Properties().Set(nsIFrame::InitialOverflowProperty(),
                       new nsOverflowAreas(aOverflowAreas));
    } else if (initial != &aOverflowAreas) {
      *initial = aOverflowAreas;
    }
  }

  
  
  
  
  
  
  
  
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    DebugOnly<nsRect*> r = &aOverflowAreas.Overflow(otype);
    NS_ASSERTION(aNewSize.width == 0 || aNewSize.height == 0 ||
                 r->width == nscoord_MAX || r->height == nscoord_MAX ||
                 (mState & NS_FRAME_SVG_LAYOUT) ||
                 r->Contains(nsRect(nsPoint(0,0), aNewSize)),
                 "Computed overflow area must contain frame bounds");
  }

  
  
  
  
  const nsStyleDisplay* disp = GetStyleDisplay();
  NS_ASSERTION((disp->mOverflowY == NS_STYLE_OVERFLOW_CLIP) ==
               (disp->mOverflowX == NS_STYLE_OVERFLOW_CLIP),
               "If one overflow is clip, the other should be too");
  if (nsFrame::ApplyOverflowClipping(this, disp)) {
    
    aOverflowAreas.SetAllTo(bounds);
  }

  
  
  
  
  
  
  if ((aNewSize.width != 0 || !IsInlineFrame(this)) &&
      !(GetStateBits() & NS_FRAME_SVG_LAYOUT)) {
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o.UnionRectEdges(o, bounds);
    }
  }

  
  
  if (!IsBoxWrapped() && IsThemed(disp)) {
    nsRect r(bounds);
    nsPresContext *presContext = PresContext();
    if (presContext->GetTheme()->
          GetWidgetOverflow(presContext->DeviceContext(), this,
                            disp->mAppearance, &r)) {
      nsRect& vo = aOverflowAreas.VisualOverflow();
      vo.UnionRectEdges(vo, r);
    }
  }

  
  aOverflowAreas.VisualOverflow() =
    ComputeOutlineAndEffectsRect(this, aOverflowAreas.VisualOverflow(), 
                                 aNewSize, true);

  
  nsRect clipPropClipRect;
  bool hasClipPropClip = GetClipPropClipRect(disp, &clipPropClipRect, aNewSize);
  if (hasClipPropClip) {
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o.IntersectRect(o, clipPropClipRect);
    }
  }

  
  bool hasTransform = IsTransformed();
  nsSize oldSize = aOldSize ? *aOldSize : mRect.Size();
  bool sizeChanged = (oldSize != aNewSize);
  if (hasTransform) {
    Properties().Set(nsIFrame::PreTransformOverflowAreasProperty(),
                     new nsOverflowAreas(aOverflowAreas));
    



    nsRect newBounds(nsPoint(0, 0), aNewSize);
    
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o = nsDisplayTransform::TransformRect(o, this, nsPoint(0, 0), &newBounds);
    }
    if (Preserves3DChildren()) {
      ComputePreserve3DChildrenOverflow(aOverflowAreas, newBounds);
    } else if (sizeChanged && ChildrenHavePerspective()) {
      RecomputePerspectiveChildrenOverflow(this->GetStyleContext(), &newBounds);
    }
  } else {
    Properties().Delete(nsIFrame::PreTransformOverflowAreasProperty());
    if (ChildrenHavePerspective() && sizeChanged) {
      nsRect newBounds(nsPoint(0, 0), aNewSize);
      RecomputePerspectiveChildrenOverflow(this->GetStyleContext(), &newBounds);
    }
  }

  bool anyOverflowChanged;
  if (aOverflowAreas != nsOverflowAreas(bounds, bounds)) {
    anyOverflowChanged = SetOverflowAreas(aOverflowAreas);
  } else {
    anyOverflowChanged = ClearOverflowRects();
  }

  if (anyOverflowChanged) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }
  return anyOverflowChanged;
}

void
nsIFrame::RecomputePerspectiveChildrenOverflow(const nsStyleContext* aStartStyle, const nsRect* aBounds)
{
  
  nsSize oldSize = GetSize();
  if (aBounds) {
    SetSize(aBounds->Size());
  }
  nsIFrame::ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if ((child->GetStateBits() & NS_FRAME_SVG_LAYOUT) &&
          (child->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
        continue; 
      }
      if (child->HasPerspective()) {
        nsOverflowAreas* overflow = 
          static_cast<nsOverflowAreas*>(child->Properties().Get(nsIFrame::InitialOverflowProperty()));
        nsRect bounds(nsPoint(0, 0), child->GetSize());
        if (overflow) {
          nsOverflowAreas overflowCopy = *overflow;
          child->FinishAndStoreOverflow(overflowCopy, bounds.Size());
        } else {
          nsOverflowAreas boundsOverflow;
          boundsOverflow.SetAllTo(bounds);
          child->FinishAndStoreOverflow(boundsOverflow, bounds.Size());
        }
      } else if (child->GetStyleContext()->GetParent() == aStartStyle ||
                 child->GetStyleContext() == aStartStyle) {
        
        
        
        
        
        child->RecomputePerspectiveChildrenOverflow(aStartStyle, nullptr);
      }
    }
  }
  
  SetSize(oldSize);
}







static void
RecomputePreserve3DChildrenOverflow(nsIFrame* aFrame, const nsRect* aBounds)
{
  
  nsSize oldSize = aFrame->GetSize();
  if (aBounds) {
    aFrame->SetSize(aBounds->Size());
  }
  nsIFrame::ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      if ((child->GetStateBits() & NS_FRAME_SVG_LAYOUT) &&
          (child->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
        continue; 
      }
      if (child->Preserves3DChildren()) {
        RecomputePreserve3DChildrenOverflow(child, NULL);
      } else if (child->Preserves3D()) {
        nsOverflowAreas* overflow = 
          static_cast<nsOverflowAreas*>(child->Properties().Get(nsIFrame::InitialOverflowProperty()));
        nsRect bounds(nsPoint(0, 0), child->GetSize());
        if (overflow) {
          nsOverflowAreas overflowCopy = *overflow;
          child->FinishAndStoreOverflow(overflowCopy, bounds.Size());
        } else {
          nsOverflowAreas boundsOverflow;
          boundsOverflow.SetAllTo(bounds);
          child->FinishAndStoreOverflow(boundsOverflow, bounds.Size());
        }
      }
    }
  }
  
  aFrame->SetSize(oldSize);
 
  
  
  if (!aBounds) {
    nsOverflowAreas* overflow = 
      static_cast<nsOverflowAreas*>(aFrame->Properties().Get(nsIFrame::InitialOverflowProperty()));
    nsRect bounds(nsPoint(0, 0), aFrame->GetSize());
    if (overflow) {
      nsOverflowAreas overflowCopy = *overflow;
      overflowCopy.UnionAllWith(bounds); 
      aFrame->FinishAndStoreOverflow(overflowCopy, bounds.Size());
    } else {
      nsOverflowAreas boundsOverflow;
      boundsOverflow.SetAllTo(bounds);
      aFrame->FinishAndStoreOverflow(boundsOverflow, bounds.Size());
    }
  }
}

void
nsIFrame::ComputePreserve3DChildrenOverflow(nsOverflowAreas& aOverflowAreas, const nsRect& aBounds)
{
  
  
  

  
  
  
  if (!Preserves3D()) {
    RecomputePreserve3DChildrenOverflow(this, &aBounds);
  }

  nsRect childVisual;
  nsRect childScrollable;
  nsIFrame::ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* child = childFrames.get();
      nsPoint offset = child->GetPosition();
      nsRect visual = child->GetVisualOverflowRect();
      nsRect scrollable = child->GetScrollableOverflowRect();
      visual.MoveBy(offset);
      scrollable.MoveBy(offset);
      if (child->Preserves3D()) {
        childVisual = childVisual.Union(visual);
        childScrollable = childScrollable.Union(scrollable);
      } else {
        childVisual = 
          childVisual.Union(nsDisplayTransform::TransformRect(visual, 
                            this, nsPoint(0,0), &aBounds));
        childScrollable = 
          childScrollable.Union(nsDisplayTransform::TransformRect(scrollable,
                                this, nsPoint(0,0), &aBounds));
      }
    }
  }

  aOverflowAreas.Overflow(eVisualOverflow) = aOverflowAreas.Overflow(eVisualOverflow).Union(childVisual);
  aOverflowAreas.Overflow(eScrollableOverflow) = aOverflowAreas.Overflow(eScrollableOverflow).Union(childScrollable);
}

void
nsFrame::ConsiderChildOverflow(nsOverflowAreas& aOverflowAreas,
                               nsIFrame* aChildFrame)
{
  aOverflowAreas.UnionWith(aChildFrame->GetOverflowAreas() +
                           aChildFrame->GetPosition());
}









static nsIFrame*
GetIBSpecialSiblingForAnonymousBlock(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Must have a non-null frame!");
  NS_ASSERTION(aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL,
               "GetIBSpecialSibling should not be called on a non-special frame");

  nsIAtom* type = aFrame->GetStyleContext()->GetPseudo();
  if (type != nsCSSAnonBoxes::mozAnonymousBlock &&
      type != nsCSSAnonBoxes::mozAnonymousPositionedBlock) {
    
    return nullptr;
  }

  
  
  aFrame = aFrame->GetFirstContinuation();

  



  nsIFrame *specialSibling = static_cast<nsIFrame*>
    (aFrame->Properties().Get(nsIFrame::IBSplitSpecialPrevSibling()));
  NS_ASSERTION(specialSibling, "Broken frame tree?");
  return specialSibling;
}











static nsIFrame*
GetCorrectedParent(const nsIFrame* aFrame)
{
  nsIFrame *parent = aFrame->GetParent();
  if (!parent) {
    return nullptr;
  }

  
  
  
  nsIAtom* pseudo = aFrame->GetStyleContext()->GetPseudo();
  if (pseudo == nsCSSAnonBoxes::tableOuter) {
    pseudo = aFrame->GetFirstPrincipalChild()->GetStyleContext()->GetPseudo();
  }
  return nsFrame::CorrectStyleParentFrame(parent, pseudo);
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
  return nullptr;
}

nsIFrame*
nsFrame::DoGetParentStyleContextFrame() const
{
  if (mContent && !mContent->GetParent() &&
      !GetStyleContext()->GetPseudo()) {
    
    return nullptr;
  }
  
  if (!(mState & NS_FRAME_OUT_OF_FLOW)) {
    




    if (mState & NS_FRAME_IS_SPECIAL) {
      nsIFrame* specialSibling = GetIBSpecialSiblingForAnonymousBlock(this);
      if (specialSibling) {
        return specialSibling;
      }
    }

    
    
    
    return GetCorrectedParent(this);
  }

  
  
  const nsIFrame* oofFrame = this;
  if ((oofFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) &&
      GetPrevInFlow()) {
    
    
    oofFrame = oofFrame->GetFirstInFlow();
  }
  nsIFrame* placeholder = oofFrame->PresContext()->FrameManager()->
                            GetPlaceholderFrameFor(oofFrame);
  if (!placeholder) {
    NS_NOTREACHED("no placeholder frame for out-of-flow frame");
    return GetCorrectedParent(this);
  }
  return placeholder->GetParentStyleContextFrame();
}

void
nsFrame::GetLastLeaf(nsPresContext* aPresContext, nsIFrame **aFrame)
{
  if (!aFrame || !*aFrame)
    return;
  nsIFrame *child = *aFrame;
  
  while (1){
    child = child->GetFirstPrincipalChild();
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
    child = child->GetFirstPrincipalChild();
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

 bool
nsIFrame::IsFocusable(int32_t *aTabIndex, bool aWithMouse)
{
  int32_t tabIndex = -1;
  if (aTabIndex) {
    *aTabIndex = -1; 
  }
  bool isFocusable = false;

  if (mContent && mContent->IsElement() && IsVisibleConsideringAncestors()) {
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
          scrollFrame->GetScrollbarStyles() != nsIScrollableFrame::ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN, NS_STYLE_OVERFLOW_HIDDEN) &&
          !scrollFrame->GetScrollRange().IsEqualEdges(nsRect(0, 0, 0, 0))) {
          
          isFocusable = true;
          tabIndex = 0;
      }
    }
  }

  if (aTabIndex) {
    *aTabIndex = tabIndex;
  }
  return isFocusable;
}





bool
nsIFrame::HasTerminalNewline() const
{
  return false;
}

static uint8_t
ConvertSVGDominantBaselineToVerticalAlign(uint8_t aDominantBaseline)
{
  
  switch (aDominantBaseline) {
  case NS_STYLE_DOMINANT_BASELINE_HANGING:
  case NS_STYLE_DOMINANT_BASELINE_TEXT_BEFORE_EDGE:
    return NS_STYLE_VERTICAL_ALIGN_TEXT_TOP;
  case NS_STYLE_DOMINANT_BASELINE_TEXT_AFTER_EDGE:
  case NS_STYLE_DOMINANT_BASELINE_IDEOGRAPHIC:
    return NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM;
  case NS_STYLE_DOMINANT_BASELINE_CENTRAL:
  case NS_STYLE_DOMINANT_BASELINE_MIDDLE:
  case NS_STYLE_DOMINANT_BASELINE_MATHEMATICAL:
    return NS_STYLE_VERTICAL_ALIGN_MIDDLE;
  case NS_STYLE_DOMINANT_BASELINE_AUTO:
  case NS_STYLE_DOMINANT_BASELINE_ALPHABETIC:
    return NS_STYLE_VERTICAL_ALIGN_BASELINE;
  case NS_STYLE_DOMINANT_BASELINE_USE_SCRIPT:
  case NS_STYLE_DOMINANT_BASELINE_NO_CHANGE:
  case NS_STYLE_DOMINANT_BASELINE_RESET_SIZE:
    
    
    
    return NS_STYLE_VERTICAL_ALIGN_BASELINE;
  default:
    NS_NOTREACHED("unexpected aDominantBaseline value");
    return NS_STYLE_VERTICAL_ALIGN_BASELINE;
  }
}

uint8_t
nsIFrame::VerticalAlignEnum() const
{
  if (mState & NS_FRAME_IS_SVG_TEXT) {
    uint8_t dominantBaseline;
    for (const nsIFrame* frame = this; frame; frame = frame->GetParent()) {
      dominantBaseline = frame->GetStyleSVGReset()->mDominantBaseline;
      if (dominantBaseline != NS_STYLE_DOMINANT_BASELINE_AUTO ||
          frame->GetType() == nsGkAtoms::svgTextFrame) {
        break;
      }
    }
    return ConvertSVGDominantBaselineToVerticalAlign(dominantBaseline);
  }

  const nsStyleCoord& verticalAlign =
    GetStyleContext()->GetStyleTextReset()->mVerticalAlign;
  if (verticalAlign.GetUnit() == eStyleUnit_Enumerated) {
    return verticalAlign.GetIntValue();
  }

  return eInvalidVerticalAlign;
}


void nsFrame::FillCursorInformationFromStyle(const nsStyleUserInterface* ui,
                                             nsIFrame::Cursor& aCursor)
{
  aCursor.mCursor = ui->mCursor;
  aCursor.mHaveHotspot = false;
  aCursor.mHotspotX = aCursor.mHotspotY = 0.0f;

  for (nsCursorImage *item = ui->mCursorArray,
                 *item_end = ui->mCursorArray + ui->mCursorArrayLength;
       item < item_end; ++item) {
    uint32_t status;
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

    
    nsRect rect = GetRect();

    nsMargin bp(0,0,0,0);
    GetBorderAndPadding(bp);

    {
      
      
      AutoMaybeDisableFontInflation an(this);

      metrics->mBlockPrefSize.width =
        GetPrefWidth(rendContext) + bp.LeftRight();
      metrics->mBlockMinSize.width =
        GetMinWidth(rendContext) + bp.LeftRight();
    }

    
    nsHTMLReflowMetrics desiredSize;
    rv = BoxReflow(aState, presContext, desiredSize, rendContext,
                   rect.x, rect.y,
                   metrics->mBlockPrefSize.width, NS_UNCONSTRAINEDSIZE);

    metrics->mBlockMinSize.height = 0;
    
    
    nsAutoLineIterator lines = GetLineIterator();
    if (lines) 
    {
      metrics->mBlockMinSize.height = 0;
      int count = 0;
      nsIFrame* firstFrame = nullptr;
      int32_t framesOnLine;
      nsRect lineBounds;
      uint32_t lineFlags;

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
  return nullptr;
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

  if (IsCollapsed())
    return size;

  
  bool widthSet, heightSet;
  bool completelyRedefined = nsIFrame::AddCSSPrefSize(this, size, widthSet, heightSet);

  
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

  if (IsCollapsed())
    return size;

  
  bool widthSet, heightSet;
  bool completelyRedefined =
    nsIFrame::AddCSSMinSize(aState, this, size, widthSet, heightSet);

  
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

  if (IsCollapsed())
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

  if (IsCollapsed()) {
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

    if (IsCollapsed()) {
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

  if (HasAbsolutelyPositionedChildren()) {
    
    nsHTMLReflowState reflowState(aState.PresContext(), this,
                                  aState.GetRenderingContext(),
                                  nsSize(size.width, NS_UNCONSTRAINEDSIZE),
                                  nsHTMLReflowState::DUMMY_PARENT_REFLOW_STATE);

    
    
    nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;
    ReflowAbsoluteFrames(aState.PresContext(), desiredSize,
                         reflowState, reflowStatus);
  }

  nsSize oldSize(ourRect.Size());
  FinishAndStoreOverflow(desiredSize.mOverflowAreas, size, &oldSize);

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
                   bool                     aMoveFrame)
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

  bool needsReflow = NS_SUBTREE_DIRTY(this);

  
  
  if (!needsReflow) {
      
      if (aWidth != NS_INTRINSICSIZE && aHeight != NS_INTRINSICSIZE) {
      
          
          if ((metrics->mLastSize.width == 0 || metrics->mLastSize.height == 0) && (aWidth == 0 || aHeight == 0)) {
               needsReflow = false;
               aDesiredSize.width = aWidth; 
               aDesiredSize.height = aHeight; 
               SetSize(nsSize(aDesiredSize.width, aDesiredSize.height));
          } else {
            aDesiredSize.width = metrics->mLastSize.width;
            aDesiredSize.height = metrics->mLastSize.height;

            
            
            if (metrics->mLastSize.width == aWidth && metrics->mLastSize.height == aHeight)
                  needsReflow = false;
            else
                  needsReflow = true;
   
          }
      } else {
          
          
         needsReflow = true;
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
                                        parentSize,
                                        nsHTMLReflowState::DUMMY_PARENT_REFLOW_STATE);
    parentFrame->RemoveStateBits(~nsFrameState(0));
    parentFrame->AddStateBits(savedState);

    
    if (parentSize.width != NS_INTRINSICSIZE)
      parentReflowState.SetComputedWidth(std::max(parentSize.width, 0));
    if (parentSize.height != NS_INTRINSICSIZE)
      parentReflowState.SetComputedHeight(std::max(parentSize.height, 0));
    parentReflowState.mComputedMargin.SizeTo(0, 0, 0, 0);
    
    parentFrame->GetPadding(parentReflowState.mComputedPadding);
    parentFrame->GetBorder(parentReflowState.mComputedBorderPadding);
    parentReflowState.mComputedBorderPadding +=
      parentReflowState.mComputedPadding;

    
    
    nsSize availSize(aWidth, NS_INTRINSICSIZE);
    nsHTMLReflowState reflowState(aPresContext, this, aRenderingContext,
                                  availSize,
                                  nsHTMLReflowState::DUMMY_PARENT_REFLOW_STATE);

    
    
    const nsHTMLReflowState *outerReflowState = aState.OuterReflowState();
    NS_ASSERTION(!outerReflowState || outerReflowState->frame != this,
                 "in and out of XUL on a single frame?");
    if (outerReflowState && outerReflowState->frame == parentFrame) {
      
      
      
      
      
      reflowState.parentReflowState = outerReflowState;
      reflowState.mCBReflowState = outerReflowState;
    } else {
      reflowState.parentReflowState = &parentReflowState;
      reflowState.mCBReflowState = &parentReflowState;
    }
    reflowState.mReflowDepth = aState.GetReflowDepth();

    
    
    if (aWidth != NS_INTRINSICSIZE) {
      nscoord computedWidth =
        aWidth - reflowState.mComputedBorderPadding.LeftRight();
      computedWidth = std::max(computedWidth, 0);
      reflowState.SetComputedWidth(computedWidth);
    }

    
    
    
    
    
    if (!IsFrameOfType(eBlockFrame)) {
      if (aHeight != NS_INTRINSICSIZE) {
        nscoord computedHeight =
          aHeight - reflowState.mComputedBorderPadding.TopBottom();
        computedHeight = std::max(computedHeight, 0);
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
                      false).height
          );
      }
    }

    
    
    
    
    
    
    if (metrics->mLastSize.width != aWidth) {
      reflowState.mFlags.mHResize = true;

      
      
      
      if (nsLayoutUtils::FontSizeInflationEnabled(aPresContext)) {
        AddStateBits(NS_FRAME_IS_DIRTY);
      }
    }
    if (metrics->mLastSize.height != aHeight)
      reflowState.mFlags.mVResize = true;

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

    uint32_t layoutFlags = aState.LayoutFlags();
    nsContainerFrame::FinishReflowChild(this, aPresContext, &reflowState,
                                        aDesiredSize, aX, aY, layoutFlags | NS_FRAME_NO_MOVE_FRAME);

    
    if (IsCollapsed()) {
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





static void
AddInPopupStateBitToDescendants(nsIFrame* aFrame)
{
  aFrame->AddStateBits(NS_FRAME_IN_POPUP);

  nsAutoTArray<nsIFrame::ChildList,4> childListArray;
  aFrame->GetCrossDocChildLists(&childListArray);

  nsIFrame::ChildListArrayIterator lists(childListArray);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      AddInPopupStateBitToDescendants(childFrames.get());
    }
  }
}






static void
RemoveInPopupStateBitFromDescendants(nsIFrame* aFrame)
{
  if (!aFrame->HasAnyStateBits(NS_FRAME_IN_POPUP) ||
      aFrame->GetType() == nsGkAtoms::listControlFrame ||
      aFrame->GetType() == nsGkAtoms::menuPopupFrame) {
    return;
  }

  aFrame->RemoveStateBits(NS_FRAME_IN_POPUP);

  nsAutoTArray<nsIFrame::ChildList,4> childListArray;
  aFrame->GetCrossDocChildLists(&childListArray);

  nsIFrame::ChildListArrayIterator lists(childListArray);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      RemoveInPopupStateBitFromDescendants(childFrames.get());
    }
  }
}

void
nsFrame::SetParent(nsIFrame* aParent)
{
  bool wasBoxWrapped = IsBoxWrapped();
  mParent = aParent;
  if (!wasBoxWrapped && IsBoxWrapped()) {
    InitBoxMetrics(true);
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
  
  if (HasInvalidFrameInSubtree()) {
    for (nsIFrame* f = aParent;
         f && !f->HasAnyStateBits(NS_FRAME_DESCENDANT_NEEDS_PAINT);
         f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
      f->AddStateBits(NS_FRAME_DESCENDANT_NEEDS_PAINT);
    }
  }

  if (aParent->HasAnyStateBits(NS_FRAME_IN_POPUP)) {
    AddInPopupStateBitToDescendants(this);
  } else {
    RemoveInPopupStateBitFromDescendants(this);
  }
  
  
  
  
  if (aParent->HasAnyStateBits(NS_FRAME_ALL_DESCENDANTS_NEED_PAINT)) {
    InvalidateFrame();
  }
}

void
nsFrame::InitBoxMetrics(bool aClear)
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

void
nsIFrame::CreateOwnLayerIfNeeded(nsDisplayListBuilder* aBuilder, 
                                 nsDisplayList* aList)
{
  if (GetContent()->HasAttr(kNameSpaceID_None, nsGkAtoms::layer)) {
    aList->AppendNewToTop(new (aBuilder) 
        nsDisplayOwnLayer(aBuilder, this, aList));
  }
}


#ifdef DEBUG_REFLOW
int32_t gIndent2 = 0;

void
nsAdaptorAddIndents()
{
    for(int32_t i=0; i < gIndent2; i++)
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

#ifdef DEBUG
static void
GetTagName(nsFrame* aFrame, nsIContent* aContent, int aResultSize,
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
nsFrame::Trace(const char* aMethod, bool aEnter)
{
  if (NS_FRAME_LOG_TEST(gLogModule, NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s", tagbuf, aEnter ? "enter" : "exit", aMethod);
  }
}

void
nsFrame::Trace(const char* aMethod, bool aEnter, nsReflowStatus aStatus)
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
  bool RuleMatches(DR_Rule&          aRule,
                     DR_FrameTreeNode& aNode);
  bool GetToken(FILE* aFile,
                  char* aBuf,
                  size_t aBufSize);
  DR_Rule* ParseRule(FILE* aFile);
  void ParseRulesFile();
  void AddRule(nsTArray<DR_Rule*>& aRules,
               DR_Rule&            aRule);
  bool IsWhiteSpace(int c);
  bool GetNumber(char*    aBuf, 
                 int32_t&  aNumber);
  void PrettyUC(nscoord aSize,
                char*   aBuf);
  void PrintMargin(const char* tag, const nsMargin* aMargin);
  void DisplayFrameTypeInfo(nsIFrame* aFrame,
                            int32_t   aIndent);
  void DeleteTreeNode(DR_FrameTreeNode& aNode);

  bool        mInited;
  bool        mActive;
  int32_t     mCount;
  int32_t     mAssert;
  int32_t     mIndent;
  bool        mIndentUndisplayedFrames;
  bool        mDisplayPixelErrors;
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
  DR_Rule() : mLength(0), mTarget(nullptr), mDisplay(false) {
    MOZ_COUNT_CTOR(DR_Rule);
  }
  ~DR_Rule() {
    if (mTarget) mTarget->Destroy();
    MOZ_COUNT_DTOR(DR_Rule);
  }
  void AddPart(nsIAtom* aFrameType);

  uint32_t      mLength;
  DR_RulePart*  mTarget;
  bool          mDisplay;
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
      int32_t numElements;
      numElements = mRules.Length();
      for (int32_t i = numElements - 1; i >= 0; i--) {
        delete mRules.ElementAt(i);
      }
   }

  nsIAtom*    mType;
  char        mNameAbbrev[16];
  char        mName[32];
  nsTArray<DR_Rule*> mRules;
private:
  DR_FrameTypeInfo& operator=(const DR_FrameTypeInfo&) MOZ_DELETE;
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
  bool              mDisplay;
  uint32_t          mIndent;
};



DR_State::DR_State() 
: mInited(false), mActive(false), mCount(0), mAssert(-1), mIndent(0), 
  mIndentUndisplayedFrames(false), mDisplayPixelErrors(false)
{
  MOZ_COUNT_CTOR(DR_State);
}

void DR_State::Init() 
{
  char* env = PR_GetEnv("GECKO_DISPLAY_REFLOW_ASSERT");
  int32_t num;
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
  mInited = true;
}

DR_State::~DR_State()
{
  MOZ_COUNT_DTOR(DR_State);
  int32_t numElements, i;
  numElements = mWildRules.Length();
  for (i = numElements - 1; i >= 0; i--) {
    delete mWildRules.ElementAt(i);
  }
  numElements = mFrameTreeLeaves.Length();
  for (i = numElements - 1; i >= 0; i--) {
    delete mFrameTreeLeaves.ElementAt(i);
  }
}

bool DR_State::GetNumber(char*     aBuf, 
                           int32_t&  aNumber)
{
  if (sscanf(aBuf, "%d", &aNumber) > 0) 
    return true;
  else 
    return false;
}

bool DR_State::IsWhiteSpace(int c) {
  return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

bool DR_State::GetToken(FILE* aFile,
                          char* aBuf,
                          size_t aBufSize)
{
  bool haveToken = false;
  aBuf[0] = 0;
  
  int c = -1;
  for (c = getc(aFile); (c > 0) && IsWhiteSpace(c); c = getc(aFile)) {
  }

  if (c > 0) {
    haveToken = true;
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
  int32_t doDisplay;
  DR_Rule* rule = nullptr;
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
        rule->AddPart(nullptr);
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
  int32_t numRules = aRules.Length();
  for (int32_t ruleX = 0; ruleX < numRules; ruleX++) {
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
          mActive = true;
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
  int32_t numEntries = mFrameTypeTable.Length();
  NS_ASSERTION(numEntries != 0, "empty FrameTypeTable");
  for (int32_t i = 0; i < numEntries; i++) {
    DR_FrameTypeInfo& info = mFrameTypeTable.ElementAt(i);
    if (info.mType == aFrameType) {
      return &info;
    }
  }
  return &mFrameTypeTable.ElementAt(numEntries - 1); 
}

DR_FrameTypeInfo* DR_State::GetFrameTypeInfo(char* aFrameName)
{
  int32_t numEntries = mFrameTypeTable.Length();
  NS_ASSERTION(numEntries != 0, "empty FrameTypeTable");
  for (int32_t i = 0; i < numEntries; i++) {
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
  AddFrameTypeInfo(nullptr,                               "unknown",   "unknown");
}


void DR_State::DisplayFrameTypeInfo(nsIFrame* aFrame,
                                    int32_t   aIndent)
{ 
  DR_FrameTypeInfo* frameTypeInfo = GetFrameTypeInfo(aFrame->GetType());
  if (frameTypeInfo) {
    for (int32_t i = 0; i < aIndent; i++) {
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

bool DR_State::RuleMatches(DR_Rule&          aRule,
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
          return false;
        }
      }
      else NS_ASSERTION(false, "program error");
    }
    
  }
  return true;
}

void DR_State::FindMatchingRule(DR_FrameTreeNode& aNode)
{
  if (!aNode.mFrame) {
    NS_ASSERTION(false, "invalid DR_FrameTreeNode \n");
    return;
  }

  bool matchingRule = false;

  DR_FrameTypeInfo* info = GetFrameTypeInfo(aNode.mFrame->GetType());
  NS_ASSERTION(info, "program error");
  int32_t numRules = info->mRules.Length();
  for (int32_t ruleX = 0; ruleX < numRules; ruleX++) {
    DR_Rule* rule = info->mRules.ElementAt(ruleX);
    if (rule && RuleMatches(*rule, aNode)) {
      aNode.mDisplay = rule->mDisplay;
      matchingRule = true;
      break;
    }
  }
  if (!matchingRule) {
    int32_t numWildRules = mWildRules.Length();
    for (int32_t ruleX = 0; ruleX < numWildRules; ruleX++) {
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
    parentFrame = (parentRS) ? parentRS->frame : nullptr;
  } else {
    parentFrame = aFrame->GetParent();
  }

  
  DR_FrameTreeNode* parentNode = nullptr;
  
  DR_FrameTreeNode* lastLeaf = nullptr;
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
  int32_t numLeaves = mFrameTreeLeaves.Length();
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
                int32_t aPixelToTwips)
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
                                    bool                     aChanged)
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
      int32_t p2t = aPresContext->AppUnitsPerDevPixel();
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
  if (!DR_state->mActive) return nullptr;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, &aReflowState);
  if (treeNode) {
    DisplayReflowEnterPrint(aPresContext, aFrame, aReflowState, *treeNode, false);
  }
  return treeNode;
}

void* nsFrame::DisplayLayoutEnter(nsIFrame* aFrame)
{
  if (!DR_state->mInited) DR_state->Init();
  if (!DR_state->mActive) return nullptr;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nullptr);
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
  if (!DR_state->mActive) return nullptr;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nullptr);
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
  if (!DR_state->mActive) return nullptr;

  NS_ASSERTION(aFrame, "invalid call");

  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nullptr);
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
      int32_t p2t = aPresContext->AppUnitsPerDevPixel();
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
  DR_state = nullptr;
}

void DR_cookie::Change() const
{
  DR_FrameTreeNode* treeNode = (DR_FrameTreeNode*)mValue;
  if (treeNode && treeNode->mDisplay) {
    DisplayReflowEnterPrint(mPresContext, mFrame, mReflowState, *treeNode, true);
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
  if (!DR_state->mActive) return nullptr;

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
  if (!DR_state->mActive) return nullptr;

  
  DR_FrameTreeNode* treeNode = DR_state->CreateTreeNode(aFrame, nullptr);
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
  if (!DR_state->mActive) return nullptr;

  
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
    if (aFrame->IsAbsolutelyPositioned())
      printf(" abspos");
    if (aFrame->IsFloating())
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
    if (disp->mDisplay >= ArrayLength(displayTypes))
      printf(" display=%u", disp->mDisplay);
    else
      printf(" display=%s", displayTypes[disp->mDisplay]);

    
    const char *const cssFrameTypes[] = {
      "unknown", "inline", "block", "floating", "absolute", "internal-table"
    };
    nsCSSFrameType bareType = NS_FRAME_GET_TYPE(aState->mFrameType);
    bool repNoBlock = NS_FRAME_IS_REPLACED_NOBLOCK(aState->mFrameType);
    bool repBlock = NS_FRAME_IS_REPLACED_CONTAINS_BLOCK(aState->mFrameType);

    if (bareType >= ArrayLength(cssFrameTypes)) {
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
