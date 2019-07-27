







#include "nsFrame.h"

#include <stdarg.h>
#include <algorithm>

#include "gfxUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/PathHelpers.h"

#include "nsCOMPtr.h"
#include "nsFrameList.h"
#include "nsPlaceholderFrame.h"
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
#include "nsStyleConsts.h"
#include "nsIPresShell.h"
#include "prlog.h"
#include "prprf.h"
#include "nsFrameManager.h"
#include "nsLayoutUtils.h"
#include "RestyleManager.h"

#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsFrameSelection.h"
#include "nsGkAtoms.h"
#include "nsHtml5Atoms.h"
#include "nsCSSAnonBoxes.h"

#include "nsFrameTraversal.h"
#include "nsRange.h"
#include "nsITextControlFrame.h"
#include "nsNameSpaceManager.h"
#include "nsIPercentHeightObserver.h"
#include "nsStyleStructInlines.h"
#include "FrameLayerBuilder.h"

#include "nsBidiPresUtils.h"


#include "imgIContainer.h"
#include "imgIRequest.h"
#include "nsError.h"
#include "nsContainerFrame.h"
#include "nsBoxLayoutState.h"
#include "nsBlockFrame.h"
#include "nsDisplayList.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGEffects.h"
#include "nsChangeHint.h"
#include "nsDeckFrame.h"
#include "nsSubDocumentFrame.h"
#include "SVGTextFrame.h"

#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "nsAbsoluteContainingBlock.h"
#include "StickyScrollContainer.h"
#include "nsFontInflationData.h"
#include "gfxASurface.h"
#include "nsRegion.h"
#include "nsIFrameInlines.h"

#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/EventListenerManager.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/EventStates.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/css/ImageLoader.h"
#include "mozilla/gfx/Tools.h"
#include "nsPrintfCString.h"
#include "ActiveLayerTracker.h"

#include "nsITheme.h"
#include "nsThemeConstants.h"

using namespace mozilla;
using namespace mozilla::css;
using namespace mozilla::dom;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using namespace mozilla::layout;

namespace mozilla {
namespace gfx {
class VRHMDInfo;
}
}


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




#define REVERSED_DIRECTION_FRAME(frame) \
  (!IS_SAME_DIRECTION(NS_GET_EMBEDDING_LEVEL(frame), NS_GET_BASE_LEVEL(frame)))

#include "nsILineIterator.h"


#if 0
static void RefreshContentFrames(nsPresContext* aPresContext, nsIContent * aStartContent, nsIContent * aEndContent);
#endif

#include "prenv.h"

NS_DECLARE_FRAME_PROPERTY(BoxMetricsProperty, DeleteValue<nsBoxLayoutMetrics>)

static void
InitBoxMetrics(nsIFrame* aFrame, bool aClear)
{
  FrameProperties props = aFrame->Properties();
  if (aClear) {
    props.Delete(BoxMetricsProperty());
  }

  nsBoxLayoutMetrics *metrics = new nsBoxLayoutMetrics();
  props.Set(BoxMetricsProperty(), metrics);

  static_cast<nsFrame*>(aFrame)->nsFrame::MarkIntrinsicISizesDirty();
  metrics->mBlockAscent = 0;
  metrics->mLastSize.SizeTo(0, 0);
}

static bool
IsBoxWrapped(const nsIFrame* aFrame)
{
  return aFrame->GetParent() &&
         aFrame->GetParent()->IsBoxFrame() &&
         !aFrame->IsBoxFrame();
}



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

#endif

NS_DECLARE_FRAME_PROPERTY(AbsoluteContainingBlockProperty,
                          DeleteValue<nsAbsoluteContainingBlock>)

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
  if (!StyleVisibility()->IsVisible()) {
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

      if (!parent->StyleVisibility()->IsVisible())
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

void
nsIFrame::ContentStatesChanged(mozilla::EventStates aStates)
{
}

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
nsWeakFrame::Init(nsIFrame* aFrame)
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
#ifdef DEBUG
  mStyleContext->FrameAddRef();
#endif
}

nsFrame::~nsFrame()
{
  MOZ_COUNT_DTOR(nsFrame);

  NS_IF_RELEASE(mContent);
#ifdef DEBUG
  mStyleContext->FrameRelease();
#endif
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
                   aFrame->StyleDisplay()->IsRubyDisplayType() ||
                   (aFrame->IsFloating() &&
                    aFrame->GetType() == nsGkAtoms::letterFrame) ||
                   
                   
                   
                   (aFrame->GetParent()->GetContent() == content) ||
                   (content && (content->IsAnyOfHTMLElements(nsGkAtoms::option,
                                                             nsGkAtoms::optgroup,
                                                             nsGkAtoms::select) ||
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

void
nsFrame::Init(nsIContent*       aContent,
              nsContainerFrame* aParent,
              nsIFrame*         aPrevInFlow)
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
                       NS_FRAME_PART_OF_IBSPLIT |
                       NS_FRAME_MAY_BE_TRANSFORMED |
                       NS_FRAME_MAY_HAVE_GENERATED_CONTENT |
                       NS_FRAME_CAN_HAVE_ABSPOS_CHILDREN);
  } else {
    PresContext()->ConstructedFrame();
  }
  if (GetParent()) {
    nsFrameState state = GetParent()->GetStateBits();

    
    mState |= state & (NS_FRAME_INDEPENDENT_SELECTION |
                       NS_FRAME_GENERATED_CONTENT |
                       NS_FRAME_IS_SVG_TEXT |
                       NS_FRAME_IN_POPUP |
                       NS_FRAME_IS_NONDISPLAY);
  }
  const nsStyleDisplay *disp = StyleDisplay();
  if (disp->HasTransform(this)) {
    
    
    mState |= NS_FRAME_MAY_BE_TRANSFORMED;
  }
  if (disp->mPosition == NS_STYLE_POSITION_STICKY &&
      !aPrevInFlow &&
      !(mState & NS_FRAME_IS_NONDISPLAY) &&
      !disp->IsInnerTableStyle()) {
    
    
    
    
    
    
    
    StickyScrollContainer* ssc =
      StickyScrollContainer::GetStickyScrollContainerForFrame(this);
    if (ssc) {
      ssc->AddFrame(this);
    }
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

  if (aContent && aContent->GetProperty(nsGkAtoms::vr_state) != nullptr) {
    AddStateBits(NS_FRAME_HAS_VR_CONTENT);
  }

  DidSetStyleContext(nullptr);

  if (::IsBoxWrapped(this))
    ::InitBoxMetrics(this, false);
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

  if (StyleDisplay()->mPosition == NS_STYLE_POSITION_STICKY) {
    StickyScrollContainer* ssc =
      StickyScrollContainer::GetStickyScrollContainerForFrame(this);
    if (ssc) {
      ssc->RemoveFrame(this);
    }
  }

  
  
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

  
  
  
  if (mState & NS_FRAME_PART_OF_IBSPLIT) {
    
    nsIFrame* prevSib = static_cast<nsIFrame*>
      (Properties().Get(nsIFrame::IBSplitPrevSibling()));
    if (prevSib) {
      NS_WARN_IF_FALSE(this ==
         prevSib->Properties().Get(nsIFrame::IBSplitSibling()),
         "IB sibling chain is inconsistent");
      prevSib->Properties().Delete(nsIFrame::IBSplitSibling());
    }

    
    nsIFrame* nextSib = static_cast<nsIFrame*>
      (Properties().Get(nsIFrame::IBSplitSibling()));
    if (nextSib) {
      NS_WARN_IF_FALSE(this ==
         nextSib->Properties().Get(nsIFrame::IBSplitPrevSibling()),
         "IB sibling chain is inconsistent");
      nextSib->Properties().Delete(nsIFrame::IBSplitPrevSibling());
    }
  }

  bool isPrimaryFrame = (mContent && mContent->GetPrimaryFrame() == this);
  if (isPrimaryFrame) {
    
    
    ActiveLayerTracker::TransferActivityToContent(this, mContent);

    
    
    
    
    RestyleManager::ReframingStyleContexts* rsc =
      presContext->RestyleManager()->GetReframingStyleContexts();
    if (rsc) {
      rsc->Put(mContent, mStyleContext);
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

  
  if (isPrimaryFrame) {
    mContent->SetPrimaryFrame(nullptr);
  }

  
  
  
  
  
  
  

  nsQueryFrame::FrameIID id = GetFrameId();
  this->~nsFrame();

  
  
  shell->FreeFrame(id, this);
}

nsresult
nsFrame::GetOffsets(int32_t &aStart, int32_t &aEnd) const
{
  aStart = 0;
  aEnd = 0;
  return NS_OK;
}


 void
nsFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (IsSVGText()) {
    SVGTextFrame* svgTextFrame = static_cast<SVGTextFrame*>(
        nsLayoutUtils::GetClosestFrameOfType(this, nsGkAtoms::svgTextFrame));
    nsIFrame* anonBlock = svgTextFrame->GetFirstPrincipalChild();
    
    
    
    
    
    
    
    
    
    if (anonBlock && !(anonBlock->GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
        (svgTextFrame->GetStateBits() & NS_FRAME_IS_NONDISPLAY) &&
        !(svgTextFrame->GetStateBits() & NS_STATE_SVG_TEXT_IN_REFLOW)) {
      svgTextFrame->ScheduleReflowSVGNonDisplayText();
    }
  }

  ImageLoader* imageLoader = PresContext()->Document()->StyleImageLoader();

  
  
  
  
  
  
  
  
  const nsStyleBackground *oldBG = aOldStyleContext ?
                                   aOldStyleContext->StyleBackground() :
                                   nullptr;
  const nsStyleBackground *newBG = StyleBackground();
  if (oldBG) {
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, oldBG) {
      
      if (i >= newBG->mImageCount ||
          !oldBG->mLayers[i].mImage.ImageDataEquals(newBG->mLayers[i].mImage)) {
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
        !newBG->mLayers[i].mImage.ImageDataEquals(oldBG->mLayers[i].mImage)) {
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
      if ((!StyleMargin()->GetMargin(newValue) || oldValue != newValue) &&
          !props.Get(UsedMarginProperty())) {
        props.Set(UsedMarginProperty(), new nsMargin(oldValue));
      }
    }

    const nsStylePadding* oldPadding = aOldStyleContext->PeekStylePadding();
    if (oldPadding && oldPadding->GetPadding(oldValue)) {
      if ((!StylePadding()->GetPadding(newValue) || oldValue != newValue) &&
          !props.Get(UsedPaddingProperty())) {
        props.Set(UsedPaddingProperty(), new nsMargin(oldValue));
      }
    }

    const nsStyleBorder* oldBorder = aOldStyleContext->PeekStyleBorder();
    if (oldBorder) {
      oldValue = oldBorder->GetComputedBorder();
      newValue = StyleBorder()->GetComputedBorder();
      if (oldValue != newValue &&
          !props.Get(UsedBorderProperty())) {
        props.Set(UsedBorderProperty(), new nsMargin(oldValue));
      }
    }
  }

  imgIRequest *oldBorderImage = aOldStyleContext
    ? aOldStyleContext->StyleBorder()->GetBorderImageRequest()
    : nullptr;
  imgIRequest *newBorderImage = StyleBorder()->GetBorderImageRequest();
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (oldBorderImage != newBorderImage) {
    
    if (oldBorderImage) {
      imageLoader->DisassociateRequestFromFrame(oldBorderImage, this);
    }
    if (newBorderImage) {
      imageLoader->AssociateRequestToFrame(newBorderImage, this);
    }
  }

  
  
  
  
  if (StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
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
      IsSVGText())
    return margin;

  nsMargin *m = static_cast<nsMargin*>
                           (Properties().Get(UsedMarginProperty()));
  if (m) {
    margin = *m;
  } else {
    DebugOnly<bool> hasMargin = StyleMargin()->GetMargin(margin);
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
      IsSVGText())
    return border;

  
  nsIFrame *mutable_this = const_cast<nsIFrame*>(this);

  const nsStyleDisplay *disp = StyleDisplay();
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
    border = StyleBorder()->GetComputedBorder();
  }
  return border;
}

 nsMargin
nsIFrame::GetUsedPadding() const
{
  nsMargin padding(0, 0, 0, 0);
  if (((mState & NS_FRAME_FIRST_REFLOW) &&
       !(mState & NS_FRAME_IN_REFLOW)) ||
      IsSVGText())
    return padding;

  
  nsIFrame *mutable_this = const_cast<nsIFrame*>(this);

  const nsStyleDisplay *disp = StyleDisplay();
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
    DebugOnly<bool> hasPadding = StylePadding()->GetPadding(padding);
    NS_ASSERTION(hasPadding, "We should have padding here! (out of memory?)");
  }
  return padding;
}

nsIFrame::Sides
nsIFrame::GetSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (MOZ_UNLIKELY(StyleBorder()->mBoxDecorationBreak ==
                     NS_STYLE_BOX_DECORATION_BREAK_CLONE)) {
    return Sides();
  }

  
  
  WritingMode writingMode = GetWritingMode();
  LogicalSides logicalSkip = GetLogicalSkipSides(aReflowState);
  Sides skip;

  if (logicalSkip.BStart()) {
    if (writingMode.IsVertical()) {
      skip |= writingMode.IsVerticalLR() ? eSideBitsLeft : eSideBitsRight;
    } else {
      skip |= eSideBitsTop;
    }
  }

  if (logicalSkip.BEnd()) {
    if (writingMode.IsVertical()) {
      skip |= writingMode.IsVerticalLR() ? eSideBitsRight : eSideBitsLeft;
    } else {
      skip |= eSideBitsBottom;
    }
  }

  if (logicalSkip.IStart()) {
    if (writingMode.IsVertical()) {
      skip |= eSideBitsTop;
    } else {
      skip |= writingMode.IsBidiLTR() ? eSideBitsLeft : eSideBitsRight;
    }
  }

  if (logicalSkip.IEnd()) {
    if (writingMode.IsVertical()) {
      skip |= eSideBitsBottom;
    } else {
      skip |= writingMode.IsBidiLTR() ? eSideBitsRight : eSideBitsLeft;
    }
  }
  return skip;
}

nsRect
nsIFrame::GetPaddingRectRelativeToSelf() const
{
  nsMargin border(GetUsedBorder());
  border.ApplySkipSides(GetSkipSides());
  nsRect r(0, 0, mRect.width, mRect.height);
  r.Deflate(border);
  return r;
}

nsRect
nsIFrame::GetPaddingRect() const
{
  return GetPaddingRectRelativeToSelf() + GetPosition();
}

WritingMode
nsIFrame::GetWritingMode(nsIFrame* aSubFrame) const
{
  WritingMode writingMode = GetWritingMode();

  if (!writingMode.IsVertical() &&
      (StyleTextReset()->mUnicodeBidi & NS_STYLE_UNICODE_BIDI_PLAINTEXT)) {
    nsBidiLevel frameLevel = nsBidiPresUtils::GetFrameBaseLevel(aSubFrame);
    writingMode.SetDirectionFromBidiLevel(frameLevel);
  }

  return writingMode;
}

nsRect
nsIFrame::GetMarginRectRelativeToSelf() const
{
  nsMargin m = GetUsedMargin();
  m.ApplySkipSides(GetSkipSides());
  nsRect r(0, 0, mRect.width, mRect.height);
  r.Inflate(m);
  return r;
}

bool
nsIFrame::IsTransformed() const
{
  return ((mState & NS_FRAME_MAY_BE_TRANSFORMED) &&
          (StyleDisplay()->HasTransform(this) ||
           IsSVGTransformed() ||
           (mContent &&
            nsLayoutUtils::HasAnimationsForCompositor(mContent,
                                                      eCSSProperty_transform) &&
            IsFrameOfType(eSupportsCSSTransforms) &&
            mContent->GetPrimaryFrame() == this)));
}

bool
nsIFrame::HasOpacityInternal(float aThreshold) const
{
  MOZ_ASSERT(0.0 <= aThreshold && aThreshold <= 1.0, "Invalid argument");
  const nsStyleDisplay* displayStyle = StyleDisplay();
  return StyleDisplay()->mOpacity < aThreshold ||
         (displayStyle->mWillChangeBitField & NS_STYLE_WILL_CHANGE_OPACITY) ||
         (mContent &&
           nsLayoutUtils::HasAnimationsForCompositor(mContent,
                                                     eCSSProperty_opacity) &&
           mContent->GetPrimaryFrame() == this);
}

bool
nsIFrame::IsSVGTransformed(gfx::Matrix *aOwnTransforms,
                           gfx::Matrix *aFromParentTransforms) const
{
  return false;
}

bool
nsIFrame::Preserves3DChildren() const
{
  const nsStyleDisplay* disp = StyleDisplay();
  if (disp->mTransformStyle != NS_STYLE_TRANSFORM_STYLE_PRESERVE_3D ||
      !IsFrameOfType(nsIFrame::eSupportsCSSTransforms)) {
    return false;
  }

  
  if (GetType() == nsGkAtoms::scrollFrame) {
    return false;
  }

  nsRect temp;
  return !nsFrame::ShouldApplyOverflowClipping(this, disp) &&
         !GetClipPropClipRect(disp, &temp, GetSize()) &&
         !nsSVGIntegrationUtils::UsingEffectsForFrame(this);
}

bool
nsIFrame::Preserves3D() const
{
  if (!GetParent() || !GetParent()->Preserves3DChildren()) {
    return false;
  }
  return StyleDisplay()->HasTransform(this) || StyleDisplay()->BackfaceIsHidden();
}

bool
nsIFrame::HasPerspective() const
{
  if (!IsTransformed()) {
    return false;
  }
  nsStyleContext* parentStyleContext = StyleContext()->GetParent();
  if (!parentStyleContext) {
    return false;
  }
  const nsStyleDisplay* parentDisp = parentStyleContext->StyleDisplay();
  return parentDisp->mChildPerspective.GetUnit() == eStyleUnit_Coord;
}

bool
nsIFrame::ChildrenHavePerspective() const
{
  return StyleDisplay()->HasPerspectiveStyle();
}

nsRect
nsIFrame::GetContentRectRelativeToSelf() const
{
  nsMargin bp(GetUsedBorderAndPadding());
  bp.ApplySkipSides(GetSkipSides());
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
                             Sides aSkipSides,
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

  if (aSkipSides.Top()) {
    aRadii[NS_CORNER_TOP_LEFT_X] = 0;
    aRadii[NS_CORNER_TOP_LEFT_Y] = 0;
    aRadii[NS_CORNER_TOP_RIGHT_X] = 0;
    aRadii[NS_CORNER_TOP_RIGHT_Y] = 0;
  }

  if (aSkipSides.Right()) {
    aRadii[NS_CORNER_TOP_RIGHT_X] = 0;
    aRadii[NS_CORNER_TOP_RIGHT_Y] = 0;
    aRadii[NS_CORNER_BOTTOM_RIGHT_X] = 0;
    aRadii[NS_CORNER_BOTTOM_RIGHT_Y] = 0;
  }

  if (aSkipSides.Bottom()) {
    aRadii[NS_CORNER_BOTTOM_RIGHT_X] = 0;
    aRadii[NS_CORNER_BOTTOM_RIGHT_Y] = 0;
    aRadii[NS_CORNER_BOTTOM_LEFT_X] = 0;
    aRadii[NS_CORNER_BOTTOM_LEFT_Y] = 0;
  }

  if (aSkipSides.Left()) {
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
nsIFrame::GetBorderRadii(const nsSize& aFrameSize, const nsSize& aBorderArea,
                         Sides aSkipSides, nscoord aRadii[8]) const
{
  if (IsThemed()) {
    
    
    
    
    
    
    
    NS_FOR_CSS_HALF_CORNERS(corner) {
      aRadii[corner] = 0;
    }
    return false;
  }
  return ComputeBorderRadii(StyleBorder()->mBorderRadius,
                            aFrameSize, aBorderArea,
                            aSkipSides, aRadii);
}

bool
nsIFrame::GetBorderRadii(nscoord aRadii[8]) const
{
  nsSize sz = GetSize();
  return GetBorderRadii(sz, sz, GetSkipSides(), aRadii);
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
nsFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  NS_ASSERTION(!NS_SUBTREE_DIRTY(this),
               "frame must not be dirty");
  
  
  if (aWritingMode.IsLineInverted()) {
    return -GetLogicalUsedMargin(aWritingMode).BStart(aWritingMode);
  }
  
  
  return BSize(aWritingMode) +
         GetLogicalUsedMargin(aWritingMode).BEnd(aWritingMode);
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
                     nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("SelectionOverlay", TYPE_SELECTION_OVERLAY)
private:
  int16_t mSelectionValue;
};

void nsDisplaySelectionOverlay::Paint(nsDisplayListBuilder* aBuilder,
                                      nsRenderingContext* aCtx)
{
  DrawTarget& aDrawTarget = *aCtx->GetDrawTarget();

  LookAndFeel::ColorID colorID;
  if (mSelectionValue == nsISelectionController::SELECTION_ON) {
    colorID = LookAndFeel::eColorID_TextSelectBackground;
  } else if (mSelectionValue == nsISelectionController::SELECTION_ATTENTION) {
    colorID = LookAndFeel::eColorID_TextSelectBackgroundAttention;
  } else {
    colorID = LookAndFeel::eColorID_TextSelectBackgroundDisabled;
  }

  Color c = Color::FromABGR(LookAndFeel::GetColor(colorID, NS_RGB(255, 255, 255)));
  c.a = .5;
  ColorPattern color(ToDeviceColor(c));

  nsIntRect pxRect =
    mVisibleRect.ToOutsidePixels(mFrame->PresContext()->AppUnitsPerDevPixel());
  Rect rect(pxRect.x, pxRect.y, pxRect.width, pxRect.height);
  MaybeSnapToDevicePixels(rect, aDrawTarget);

  aDrawTarget.FillRect(rect, color);
}





void
nsFrame::DisplaySelectionOverlay(nsDisplayListBuilder*   aBuilder,
                                 nsDisplayList*          aList,
                                 uint16_t                aContentType)
{
  if (!IsSelected() || !IsVisibleForPainting(aBuilder))
    return;
    
  nsPresContext* presContext = PresContext();
  nsIPresShell *shell = presContext->PresShell();
  if (!shell)
    return;

  int16_t displaySelection = shell->GetSelectionFlags();
  if (!(displaySelection & aContentType))
    return;

  const nsFrameSelection* frameSelection = GetConstFrameSelection();
  int16_t selectionValue = frameSelection->GetDisplaySelection();

  if (selectionValue <= nsISelectionController::SELECTION_HIDDEN)
    return; 

  nsIContent *newContent = mContent->GetParent();

  
  int32_t offset = 0;
  if (newContent) {
    
    offset = newContent->IndexOf(mContent);
  }

  SelectionDetails *details;
  
  details = frameSelection->LookUpSelection(newContent, offset, 1, false);
  if (!details)
    return;
  
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
    
    return;
  }

  aList->AppendNewToTop(new (aBuilder)
    nsDisplaySelectionOverlay(aBuilder, this, selectionValue));
}

void
nsFrame::DisplayOutlineUnconditional(nsDisplayListBuilder*   aBuilder,
                                     const nsDisplayListSet& aLists)
{
  if (StyleOutline()->GetOutlineStyle() == NS_STYLE_BORDER_STYLE_NONE)
    return;

  aLists.Outlines()->AppendNewToTop(
    new (aBuilder) nsDisplayOutline(aBuilder, this));
}

void
nsFrame::DisplayOutline(nsDisplayListBuilder*   aBuilder,
                        const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  DisplayOutlineUnconditional(aBuilder, aLists);
}

void
nsIFrame::DisplayCaret(nsDisplayListBuilder* aBuilder,
                       const nsRect& aDirtyRect, nsDisplayList* aList)
{
  if (!IsVisibleForPainting(aBuilder))
    return;

  aList->AppendNewToTop(new (aBuilder) nsDisplayCaret(aBuilder, this));
}

nscolor
nsIFrame::GetCaretColorAt(int32_t aOffset)
{
  
  return StyleColor()->mColor;
}

bool
nsFrame::DisplayBackgroundUnconditional(nsDisplayListBuilder* aBuilder,
                                        const nsDisplayListSet& aLists,
                                        bool aForceBackground)
{
  
  
  
  if (aBuilder->IsForEventDelivery() || aForceBackground ||
      !StyleBackground()->IsTransparent() || StyleDisplay()->mAppearance) {
    return nsDisplayBackgroundImage::AppendBackgroundItemsToTop(
        aBuilder, this, aLists.BorderBackground());
  }
  return false;
}

void
nsFrame::DisplayBorderBackgroundOutline(nsDisplayListBuilder*   aBuilder,
                                        const nsDisplayListSet& aLists,
                                        bool                    aForceBackground)
{
  
  
  
  if (!IsVisibleForPainting(aBuilder))
    return;

  nsCSSShadowArray* shadows = StyleBorder()->mBoxShadow;
  if (shadows && shadows->HasShadowWithInset(false)) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayBoxShadowOuter(aBuilder, this));
  }

  bool bgIsThemed = DisplayBackgroundUnconditional(aBuilder, aLists,
                                                   aForceBackground);

  if (shadows && shadows->HasShadowWithInset(true)) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayBoxShadowInner(aBuilder, this));
  }

  
  
  if (!bgIsThemed && StyleBorder()->HasBorder()) {
    aLists.BorderBackground()->AppendNewToTop(new (aBuilder)
      nsDisplayBorder(aBuilder, this));
  }

  DisplayOutlineUnconditional(aBuilder, aLists);
}

inline static bool IsSVGContentWithCSSClip(const nsIFrame *aFrame)
{
  
  
  
  
  
  return (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) &&
          aFrame->GetContent()->IsAnyOfSVGElements(nsGkAtoms::svg,
                                                   nsGkAtoms::foreignObject);
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
  if (MOZ_LIKELY(StyleBorder()->mBoxDecorationBreak ==
                   NS_STYLE_BOX_DECORATION_BREAK_SLICE)) {
    
    
    nscoord y = 0;
    for (nsIFrame* f = GetPrevContinuation(); f; f = f->GetPrevContinuation()) {
      y += f->GetRect().height;
    }
    aRect->MoveBy(nsPoint(0, -y));
  }

  if (NS_STYLE_CLIP_RIGHT_AUTO & aDisp->mClipFlags) {
    aRect->width = aSize.width - aRect->x;
  }
  if (NS_STYLE_CLIP_BOTTOM_AUTO & aDisp->mClipFlags) {
    aRect->height = aSize.height - aRect->y;
  }
  return true;
}







static bool
ApplyClipPropClipping(nsDisplayListBuilder* aBuilder,
                      const nsIFrame* aFrame,
                      const nsStyleDisplay* aDisp,
                      nsRect* aRect,
                      DisplayListClipState::AutoSaveRestore& aClipState)
{
  if (!aFrame->GetClipPropClipRect(aDisp, aRect, aFrame->GetSize()))
    return false;

  nsRect clipRect = *aRect + aBuilder->ToReferenceFrame(aFrame);
  aClipState.ClipContentDescendants(clipRect);
  return true;
}







static void
ApplyOverflowClipping(nsDisplayListBuilder* aBuilder,
                      const nsIFrame* aFrame,
                      const nsStyleDisplay* aDisp,
                      DisplayListClipState::AutoClipMultiple& aClipState)
{
  
  
  
  
  
  if (!nsFrame::ShouldApplyOverflowClipping(aFrame, aDisp)) {
    return;
  }
  nsRect clipRect;
  bool haveRadii = false;
  nscoord radii[8];
  if (aFrame->StyleDisplay()->mOverflowClipBox ==
        NS_STYLE_OVERFLOW_CLIP_BOX_PADDING_BOX) {
    clipRect = aFrame->GetPaddingRectRelativeToSelf() +
      aBuilder->ToReferenceFrame(aFrame);
    haveRadii = aFrame->GetPaddingBoxBorderRadii(radii);
  } else {
    clipRect = aFrame->GetContentRectRelativeToSelf() +
      aBuilder->ToReferenceFrame(aFrame);
    
  }
  aClipState.ClipContainingBlockDescendantsExtra(clipRect, haveRadii ? radii : nullptr);
}

#ifdef DEBUG
static void PaintDebugBorder(nsIFrame* aFrame, nsRenderingContext* aCtx,
     const nsRect& aDirtyRect, nsPoint aPt) {
  nsRect r(aPt, aFrame->GetSize());
  DrawTarget* drawTarget = aCtx->GetDrawTarget();
  int32_t appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  Color blueOrRed(aFrame->HasView() ? Color(0.f, 0.f, 1.f, 1.f) :
                                      Color(1.f, 0.f, 0.f, 1.f));
  drawTarget->StrokeRect(NSRectToRect(r, appUnitsPerDevPixel),
                         ColorPattern(ToDeviceColor(blueOrRed)));
}

static void PaintEventTargetBorder(nsIFrame* aFrame, nsRenderingContext* aCtx,
     const nsRect& aDirtyRect, nsPoint aPt) {
  nsRect r(aPt, aFrame->GetSize());
  DrawTarget* drawTarget = aCtx->GetDrawTarget();
  int32_t appUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  ColorPattern purple(ToDeviceColor(Color(.5f, 0.f, .5f, 1.f)));
  drawTarget->StrokeRect(NSRectToRect(r, appUnitsPerDevPixel), purple);
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
WrapPreserve3DListInternal(nsIFrame* aFrame, nsDisplayListBuilder *aBuilder,
                           nsDisplayList *aList, nsDisplayList *aOutput,
                           uint32_t& aIndex, nsDisplayList* aTemp)
{
  if (aIndex > nsDisplayTransform::INDEX_MAX) {
    return NS_OK;
  }

  nsresult rv = NS_OK;
  while (nsDisplayItem *item = aList->RemoveBottom()) {
    nsIFrame *childFrame = item->Frame();

    
    
    

    if (childFrame->GetParent() &&
        (childFrame->GetParent()->Preserves3DChildren() || childFrame == aFrame)) {
      switch (item->GetType()) {
        case nsDisplayItem::TYPE_TRANSFORM: {
          if (!aTemp->IsEmpty()) {
            
            aOutput->AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder,
                aFrame, aTemp, aTemp->GetVisibleRect(), aIndex++));
          }
          
          
          
          NS_ASSERTION(!item->GetClip().HasClip(), "Unexpected clip on item");
          const DisplayItemClip* clip = aBuilder->ClipState().GetCurrentCombinedClip(aBuilder);
          if (clip) {
            item->SetClip(aBuilder, *clip);
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
            
            aOutput->AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder,
                aFrame, aTemp, aTemp->GetVisibleRect(), aIndex++));
          }
          nsDisplayOpacity *opacity = static_cast<nsDisplayOpacity*>(item);
          nsDisplayList output;
          
          
          
          rv = WrapPreserve3DListInternal(aFrame, aBuilder,
              opacity->GetChildren(), &output, aIndex, aTemp);
          if (!aTemp->IsEmpty()) {
            output.AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder,
                aFrame, aTemp, aTemp->GetVisibleRect(), aIndex++));
          }

          opacity->SetVisibleRect(output.GetVisibleRect());
          opacity->SetReferenceFrame(output.GetBottom()->ReferenceFrame());
          opacity->GetChildren()->AppendToTop(&output);
          opacity->UpdateBounds(aBuilder);
          aOutput->AppendToTop(item);
          break;
        }
        default: {
          if (childFrame->StyleDisplay()->BackfaceIsHidden()) {
            if (!aTemp->IsEmpty()) {
              aOutput->AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder,
                  aFrame, aTemp, aTemp->GetVisibleRect(), aIndex++));
            }

            aOutput->AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder,
                childFrame, item, item->GetVisibleRect(), aIndex++));
          } else {
            aTemp->AppendToTop(item);
          }
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

static bool
IsScrollFrameActive(nsDisplayListBuilder* aBuilder, nsIScrollableFrame* aScrollableFrame)
{
  return aScrollableFrame && aScrollableFrame->IsScrollingActive(aBuilder);
}

static nsresult
WrapPreserve3DList(nsIFrame* aFrame, nsDisplayListBuilder* aBuilder,
                   nsDisplayList *aList)
{
  uint32_t index = 0;
  nsDisplayList temp;
  nsDisplayList output;
  nsresult rv = WrapPreserve3DListInternal(aFrame, aBuilder, aList, &output,
      index, &temp);

  if (!temp.IsEmpty()) {
    output.AppendToTop(new (aBuilder) nsDisplayTransform(aBuilder, aFrame,
        &temp, temp.GetVisibleRect(), index++));
  }

  aList->AppendToTop(&output);
  return rv;
}

class AutoSaveRestoreBlendMode
{
  nsDisplayListBuilder& mBuilder;
  EnumSet<gfx::CompositionOp> mSavedBlendModes;
public:
  explicit AutoSaveRestoreBlendMode(nsDisplayListBuilder& aBuilder)
    : mBuilder(aBuilder)
    , mSavedBlendModes(aBuilder.ContainedBlendModes())
  { }

  ~AutoSaveRestoreBlendMode() {
    mBuilder.SetContainsBlendModes(mSavedBlendModes);
  }
};

static void
CheckForApzAwareEventHandlers(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
{
  nsIContent* content = aFrame->GetContent();
  if (!content) {
    return;
  }
  EventListenerManager* elm = nsContentUtils::GetExistingListenerManagerForNode(content);
  if (nsLayoutUtils::HasApzAwareListeners(elm)) {
    aBuilder->SetAncestorHasApzAwareEventHandler(true);
  }
}

void
nsIFrame::BuildDisplayListForStackingContext(nsDisplayListBuilder* aBuilder,
                                             const nsRect&         aDirtyRect,
                                             nsDisplayList*        aList) {
  if (GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
    return;

  
  
  if (IsFrameOfType(eReplaced) && !IsVisibleForPainting(aBuilder))
    return;

  const nsStyleDisplay* disp = StyleDisplay();
  
  
  
  
  bool needEventRegions = aBuilder->IsBuildingLayerEventRegions() &&
      StyleVisibility()->GetEffectivePointerEvents(this) != NS_STYLE_POINTER_EVENTS_NONE;
  if (disp->mOpacity == 0.0 && aBuilder->IsForPainting() &&
      !aBuilder->WillComputePluginGeometry() &&
      !(disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_OPACITY) &&
      !nsLayoutUtils::HasAnimations(mContent, eCSSProperty_opacity) &&
      !needEventRegions) {
    return;
  }

  if (disp->mWillChangeBitField != 0) {
    aBuilder->AddToWillChangeBudget(this, GetSize());
  }

  nsRect dirtyRect = aDirtyRect;

  bool inTransform = aBuilder->IsInTransform();
  bool isTransformed = IsTransformed();
  
  
  
  AutoSaveRestoreBlendMode autoRestoreBlendMode(*aBuilder);
  aBuilder->SetContainsBlendModes(BlendModeSet());
 
  nsRect dirtyRectOutsideTransform = dirtyRect;
  if (isTransformed) {
    const nsRect overflow = GetVisualOverflowRectRelativeToSelf();
    if (aBuilder->IsForPainting() &&
        nsDisplayTransform::ShouldPrerenderTransformedContent(aBuilder, this)) {
      dirtyRect = overflow;
    } else {
      if (overflow.IsEmpty() && !Preserves3DChildren()) {
        return;
      }

      nsRect untransformedDirtyRect;
      if (nsDisplayTransform::UntransformRect(dirtyRect, overflow, this,
            nsPoint(0,0), &untransformedDirtyRect)) {
        dirtyRect = untransformedDirtyRect;
      } else {
        NS_WARNING("Unable to untransform dirty rect!");
        
        dirtyRect.SetEmpty();
      }
    }
    inTransform = true;
  }

  bool usingSVGEffects = nsSVGIntegrationUtils::UsingEffectsForFrame(this);
  nsRect dirtyRectOutsideSVGEffects = dirtyRect;
  if (usingSVGEffects) {
    dirtyRect =
      nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(this, dirtyRect);
  }

  bool useOpacity = HasVisualOpacity() && !nsSVGUtils::CanOptimizeOpacity(this);
  bool useBlendMode = disp->mMixBlendMode != NS_STYLE_BLEND_NORMAL;
  bool useStickyPosition = disp->mPosition == NS_STYLE_POSITION_STICKY &&
    IsScrollFrameActive(aBuilder,
                        nsLayoutUtils::GetNearestScrollableFrame(GetParent(),
                        nsLayoutUtils::SCROLLABLE_SAME_DOC |
                        nsLayoutUtils::SCROLLABLE_INCLUDE_HIDDEN));

  nsDisplayListBuilder::AutoBuildingDisplayList
    buildingDisplayList(aBuilder, this, dirtyRect, true);

  mozilla::gfx::VRHMDInfo* vrHMDInfo = nullptr;
  if ((GetStateBits() & NS_FRAME_HAS_VR_CONTENT)) {
    vrHMDInfo = static_cast<mozilla::gfx::VRHMDInfo*>(mContent->GetProperty(nsGkAtoms::vr_state));
  }

  DisplayListClipState::AutoSaveRestore clipState(aBuilder);

  if (isTransformed || useBlendMode || usingSVGEffects || useStickyPosition) {
    
    
    
    
    
    clipState.Clear();
  }

  nsDisplayListCollection set;
  {
    DisplayListClipState::AutoSaveRestore nestedClipState(aBuilder);
    nsDisplayListBuilder::AutoInTransformSetter
      inTransformSetter(aBuilder, inTransform);
    CheckForApzAwareEventHandlers(aBuilder, this);

    nsRect clipPropClip;
    if (ApplyClipPropClipping(aBuilder, this, disp, &clipPropClip,
                              nestedClipState)) {
      dirtyRect.IntersectRect(dirtyRect, clipPropClip);
    }

    MarkAbsoluteFramesForDisplayList(aBuilder, dirtyRect);

    
    
    if (Preserves3DChildren()) {
      aBuilder->MarkPreserve3DFramesForDisplayList(this, aDirtyRect);
    }

    if (aBuilder->IsBuildingLayerEventRegions()) {
      nsDisplayLayerEventRegions* eventRegions =
        new (aBuilder) nsDisplayLayerEventRegions(aBuilder, this);
      aBuilder->SetLayerEventRegions(eventRegions);
      set.BorderBackground()->AppendNewToTop(eventRegions);
    }
    aBuilder->AdjustWindowDraggingRegion(this);
    BuildDisplayList(aBuilder, dirtyRect, set);
  }

  if (aBuilder->IsBackgroundOnly()) {
    set.BlockBorderBackgrounds()->DeleteAll();
    set.Floats()->DeleteAll();
    set.Content()->DeleteAll();
    set.PositionedDescendants()->DeleteAll();
    set.Outlines()->DeleteAll();
  }

  
  
  
  
  
  
  
  set.PositionedDescendants()->SortByZOrder(aBuilder);

  nsDisplayList resultList;
  
  
  resultList.AppendToTop(set.BorderBackground());
  
  for (;;) {
    nsDisplayItem* item = set.PositionedDescendants()->GetBottom();
    if (item && item->ZIndex() < 0) {
      set.PositionedDescendants()->RemoveBottom();
      resultList.AppendToTop(item);
      continue;
    }
    break;
  }
  
  resultList.AppendToTop(set.BlockBorderBackgrounds());
  
  resultList.AppendToTop(set.Floats());
  
  resultList.AppendToTop(set.Content());
  
  
  
  
  
  nsIContent* content = GetContent();
  if (!content) {
    content = PresContext()->Document()->GetRootElement();
  }
  if (content) {
    set.Outlines()->SortByContentOrder(aBuilder, content);
  }
#ifdef DEBUG
  DisplayDebugBorders(aBuilder, this, set);
#endif
  resultList.AppendToTop(set.Outlines());
  
  resultList.AppendToTop(set.PositionedDescendants());

  if (!isTransformed) {
    
    
    clipState.Restore();
  }

  




  if (usingSVGEffects) {
    
    buildingDisplayList.SetDirtyRect(dirtyRectOutsideSVGEffects);
    
    resultList.AppendNewToTop(
        new (aBuilder) nsDisplaySVGEffects(aBuilder, this, &resultList));
  }
  


  else if (useOpacity && !resultList.IsEmpty()) {
    
    
    
    DisplayListClipState::AutoSaveRestore opacityClipState(aBuilder);
    opacityClipState.Clear();
    resultList.AppendNewToTop(
        new (aBuilder) nsDisplayOpacity(aBuilder, this, &resultList));
  }
  

  if (useStickyPosition) {
    resultList.AppendNewToTop(
        new (aBuilder) nsDisplayStickyPosition(aBuilder, this, &resultList));
  }

  










  if (isTransformed && !resultList.IsEmpty()) {
    
    clipState.Restore();
    
    
    buildingDisplayList.SetDirtyRect(dirtyRectOutsideTransform);
    
    
    const nsIFrame* outerReferenceFrame =
      aBuilder->FindReferenceFrameFor(nsLayoutUtils::GetTransformRootFrame(this));
    buildingDisplayList.SetReferenceFrameAndCurrentOffset(outerReferenceFrame,
      GetOffsetToCrossDoc(outerReferenceFrame));

    if (Preserves3DChildren()) {
      WrapPreserve3DList(this, aBuilder, &resultList);
    } else {
      resultList.AppendNewToTop(
        new (aBuilder) nsDisplayTransform(aBuilder, this, &resultList, dirtyRect));
    }
  }

  

  if (vrHMDInfo && !resultList.IsEmpty()) {
    resultList.AppendNewToTop(
      new (aBuilder) nsDisplayVR(aBuilder, this, &resultList, vrHMDInfo));
  }

  







  if (aBuilder->ContainsBlendMode()) {
      resultList.AppendNewToTop(
        new (aBuilder) nsDisplayBlendContainer(aBuilder, this, &resultList, aBuilder->ContainedBlendModes()));
  }

  




  if (useBlendMode && !resultList.IsEmpty()) {
    resultList.AppendNewToTop(
        new (aBuilder) nsDisplayMixBlendMode(aBuilder, this, &resultList));
  }

  CreateOwnLayerIfNeeded(aBuilder, &resultList);

  aList->AppendToTop(&resultList);
}

static nsDisplayItem*
WrapInWrapList(nsDisplayListBuilder* aBuilder,
               nsIFrame* aFrame, nsDisplayList* aList)
{
  nsDisplayItem* item = aList->GetBottom();
  if (!item || item->GetAbove() || item->Frame() != aFrame) {
    return new (aBuilder) nsDisplayWrapList(aBuilder, aFrame, aList);
  }
  aList->RemoveBottom();
  return item;
}

void
nsIFrame::BuildDisplayListForChild(nsDisplayListBuilder*   aBuilder,
                                   nsIFrame*               aChild,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists,
                                   uint32_t                aFlags) {
  
  
  if (aBuilder->IsBackgroundOnly())
    return;

  nsIFrame* child = aChild;
  if (child->GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
    return;
  
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
  nsDisplayListBuilder::OutOfFlowDisplayData* savedOutOfFlowData = nullptr;
  if (childType == nsGkAtoms::placeholderFrame) {
    nsPlaceholderFrame* placeholder = static_cast<nsPlaceholderFrame*>(child);
    child = placeholder->GetOutOfFlowFrame();
    NS_ASSERTION(child, "No out of flow frame?");
    
    
    
    if (!child || nsLayoutUtils::IsPopup(child) ||
        (child->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT))
      return;
    
    
    
    childType = nullptr;
    
    if (child->GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE)
      return;
    savedOutOfFlowData = static_cast<nsDisplayListBuilder::OutOfFlowDisplayData*>
      (child->Properties().Get(nsDisplayListBuilder::OutOfFlowDisplayDataProperty()));
    if (savedOutOfFlowData) {
      dirty = savedOutOfFlowData->mDirtyRect;
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

  NS_ASSERTION(childType != nsGkAtoms::placeholderFrame,
               "Should have dealt with placeholders already");
  if (aBuilder->GetSelectedFramesOnly() &&
      child->IsLeaf() &&
      !aChild->IsSelected()) {
    return;
  }

  if (aBuilder->GetIncludeAllOutOfFlows() &&
      (child->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    dirty = child->GetVisualOverflowRect();
  } else if (!(child->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO)) {
    
    

    
    
    
    
    
    
    
    
    nsIPresShell* shell = PresContext()->PresShell();
    bool keepDescending = child == aBuilder->GetIgnoreScrollFrame() ||
        (shell->IgnoringViewportScrolling() && child == shell->GetRootScrollFrame());
    if (!keepDescending) {
      nsRect childDirty;
      if (!childDirty.IntersectRect(dirty, child->GetVisualOverflowRect()))
        return;
      
      
      
      
      
    }
  }

  
  
  const nsStyleDisplay* ourDisp = StyleDisplay();
  
  
  if (IsThemed(ourDisp) &&
      !PresContext()->GetTheme()->WidgetIsContainer(ourDisp->mAppearance))
    return;

  
  
  const nsStyleDisplay* disp = child->StyleDisplay();
  const nsStylePosition* pos = child->StylePosition();
  bool isVisuallyAtomic = child->HasOpacity()
    || child->IsTransformed()
    
    
    || disp->mChildPerspective.GetUnit() == eStyleUnit_Coord
    || disp->mMixBlendMode != NS_STYLE_BLEND_NORMAL
    || nsSVGIntegrationUtils::UsingEffectsForFrame(child)
    || (child->GetStateBits() & NS_FRAME_HAS_VR_CONTENT);

  bool isPositioned = disp->IsAbsPosContainingBlock(child);
  bool isStackingContext =
    (isPositioned && (disp->mPosition == NS_STYLE_POSITION_STICKY ||
                      pos->mZIndex.GetUnit() == eStyleUnit_Integer)) ||
     (disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_STACKING_CONTEXT) ||
     disp->mIsolation != NS_STYLE_ISOLATION_AUTO ||
     isVisuallyAtomic || (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT);

  if (isVisuallyAtomic || isPositioned || (!isSVG && disp->IsFloating(child)) ||
      ((disp->mClipFlags & NS_STYLE_CLIP_RECT) &&
       IsSVGContentWithCSSClip(child)) ||
       disp->mIsolation != NS_STYLE_ISOLATION_AUTO ||
       (disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_STACKING_CONTEXT) ||
      (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    pseudoStackingContext = true;
  }
  NS_ASSERTION(!isStackingContext || pseudoStackingContext,
               "Stacking contexts must also be pseudo-stacking-contexts");

  nsDisplayListBuilder::AutoBuildingDisplayList
    buildingForChild(aBuilder, child, dirty, pseudoStackingContext);
  DisplayListClipState::AutoClipMultiple clipState(aBuilder);
  CheckForApzAwareEventHandlers(aBuilder, child);

  if (savedOutOfFlowData) {
    clipState.SetClipForContainingBlockDescendants(
      &savedOutOfFlowData->mContainingBlockClip);
  }

  
  
  
  
  
  
  
  
  nsIFrame* parent = child->GetParent();
  const nsStyleDisplay* parentDisp =
    parent == this ? ourDisp : parent->StyleDisplay();
  ApplyOverflowClipping(aBuilder, parent, parentDisp, clipState);

  nsDisplayList list;
  nsDisplayList extraPositionedDescendants;
  if (isStackingContext) {
    if (disp->mMixBlendMode != NS_STYLE_BLEND_NORMAL) {
      aBuilder->SetContainsBlendMode(disp->mMixBlendMode);
    }
    
    
    
    child->BuildDisplayListForStackingContext(aBuilder, dirty, &list);
    aBuilder->DisplayCaret(child, dirty, &list);
  } else {
    nsRect clipRect;
    if (ApplyClipPropClipping(aBuilder, child, disp, &clipRect, clipState)) {
      
      
      dirty.IntersectRect(dirty, clipRect);
    }

    child->MarkAbsoluteFramesForDisplayList(aBuilder, dirty);

    if (!pseudoStackingContext) {
      
      
      

      if (aBuilder->IsBuildingLayerEventRegions()) {
        MOZ_ASSERT(buildingForChild.GetPrevAnimatedGeometryRoot() ==
                   aBuilder->FindAnimatedGeometryRootFor(child->GetParent()));

        
        
        nsIFrame *animatedGeometryRoot = aBuilder->FindAnimatedGeometryRootFor(child);
        if (animatedGeometryRoot != buildingForChild.GetPrevAnimatedGeometryRoot()) {
          nsDisplayLayerEventRegions* eventRegions =
            new (aBuilder) nsDisplayLayerEventRegions(aBuilder, child);
          aBuilder->SetLayerEventRegions(eventRegions);
          aLists.BorderBackground()->AppendNewToTop(eventRegions);
        }
      }

      nsDisplayLayerEventRegions* eventRegions = aBuilder->GetLayerEventRegions();
      if (eventRegions) {
        eventRegions->AddFrame(aBuilder, child);
      }
      aBuilder->AdjustWindowDraggingRegion(child);
      child->BuildDisplayList(aBuilder, dirty, aLists);
      aBuilder->DisplayCaret(child, dirty, aLists.Content());
#ifdef DEBUG
      DisplayDebugBorders(aBuilder, child, aLists);
#endif
      return;
    }

    
    
    
    
    nsDisplayListCollection pseudoStack;
    if (aBuilder->IsBuildingLayerEventRegions()) {
      nsDisplayLayerEventRegions* eventRegions =
        new (aBuilder) nsDisplayLayerEventRegions(aBuilder, child);
      aBuilder->SetLayerEventRegions(eventRegions);
      pseudoStack.BorderBackground()->AppendNewToTop(eventRegions);
    }
    aBuilder->AdjustWindowDraggingRegion(child);
    child->BuildDisplayList(aBuilder, dirty, pseudoStack);
    aBuilder->DisplayCaret(child, dirty, pseudoStack.Content());

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
    
  
  
  clipState.Clear();

  if (isPositioned || isVisuallyAtomic ||
      (aFlags & DISPLAY_CHILD_FORCE_STACKING_CONTEXT)) {
    
    
    if (!list.IsEmpty()) {
      nsDisplayItem* item = WrapInWrapList(aBuilder, child, &list);
      if (isSVG) {
        aLists.Content()->AppendNewToTop(item);
      } else {
        aLists.PositionedDescendants()->AppendNewToTop(item);
      }
    }
  } else if (!isSVG && disp->IsFloating(child)) {
    if (!list.IsEmpty()) {
      aLists.Floats()->AppendNewToTop(WrapInWrapList(aBuilder, child, &list));
    }
  } else {
    aLists.Content()->AppendToTop(&list);
  }
  
  
  
  
  
  aLists.PositionedDescendants()->AppendToTop(&extraPositionedDescendants);
}

void
nsIFrame::MarkAbsoluteFramesForDisplayList(nsDisplayListBuilder* aBuilder,
                                           const nsRect& aDirtyRect)
{
  if (IsAbsoluteContainer()) {
    aBuilder->MarkFramesForDisplayList(this, GetAbsoluteContainingBlock()->GetChildList(), aDirtyRect);
  }
}

nsresult  
nsFrame::GetContentForEvent(WidgetEvent* aEvent,
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
    nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
      new AsyncEventDispatcher(target, aDOMEventName, true, false);
    DebugOnly<nsresult> rv = asyncDispatcher->PostDOMEvent();
    NS_ASSERTION(NS_SUCCEEDED(rv), "AsyncEventDispatcher failed to dispatch");
  }
}

nsresult
nsFrame::HandleEvent(nsPresContext* aPresContext, 
                     WidgetGUIEvent* aEvent,
                     nsEventStatus* aEventStatus)
{

  if (aEvent->message == NS_MOUSE_MOVE) {
    
    
    return HandleDrag(aPresContext, aEvent, aEventStatus);
  }

  if ((aEvent->mClass == eMouseEventClass &&
       aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton) ||
      aEvent->mClass == eTouchEventClass) {
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN || aEvent->message == NS_TOUCH_START) {
      HandlePress(aPresContext, aEvent, aEventStatus);
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP || aEvent->message == NS_TOUCH_END) {
      HandleRelease(aPresContext, aEvent, aEventStatus);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrame::GetDataForTableSelection(const nsFrameSelection* aFrameSelection,
                                  nsIPresShell* aPresShell,
                                  WidgetMouseEvent* aMouseEvent, 
                                  nsIContent** aParentContent,
                                  int32_t* aContentOffset,
                                  int32_t* aTarget)
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
       aMouseEvent->button == WidgetMouseEvent::eLeftButton) ||
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

  
  parentContent.forget(aParentContent);

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

nsresult
nsFrame::IsSelectable(bool* aSelectable, uint8_t* aSelectStyle) const
{
  if (!aSelectable) 
    return NS_ERROR_NULL_POINTER;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  uint8_t selectStyle  = NS_STYLE_USER_SELECT_AUTO;
  nsIFrame* frame      = const_cast<nsFrame*>(this);

  while (frame) {
    const nsStyleUIReset* userinterface = frame->StyleUIReset();
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
    frame = nsLayoutUtils::GetParentOrPlaceholderFor(frame);
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
                     WidgetGUIEvent* aEvent,
                     nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  NS_ENSURE_ARG_POINTER(aEvent);
  if (aEvent->mClass == eTouchEventClass) {
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

  WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();

  if (!mouseEvent->IsAlt()) {
    for (nsIContent* content = mContent; content;
         content = content->GetParent()) {
      if (nsContentUtils::ContentIsDraggable(content) &&
          !content->IsEditable()) {
        
        if ((mRect - GetPosition()).Contains(
              nsLayoutUtils::GetEventCoordinatesRelativeTo(mouseEvent, this))) {
          return NS_OK;
        }
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

  
  
  
  
  
  bool hasCapturedContent = false;
  if (!nsIPresShell::GetCapturingContent()) {
    nsIScrollableFrame* scrollFrame =
      nsLayoutUtils::GetNearestScrollableFrame(this,
        nsLayoutUtils::SCROLLABLE_SAME_DOC |
        nsLayoutUtils::SCROLLABLE_INCLUDE_HIDDEN);
    if (scrollFrame) {
      nsIFrame* capturingFrame = do_QueryFrame(scrollFrame);
      nsIPresShell::SetCapturingContent(capturingFrame->GetContent(),
                                        CAPTURE_IGNOREALLOWED);
      hasCapturedContent = true;
    }
  }

  
  
  const nsFrameSelection* frameselection = nullptr;
  if (useFrameSelection)
    frameselection = GetConstFrameSelection();
  else
    frameselection = shell->ConstFrameSelection();

  if (!frameselection || frameselection->GetDisplaySelection() == nsISelectionController::SELECTION_OFF)
    return NS_OK;

#ifdef XP_MACOSX
  if (mouseEvent->IsControl())
    return NS_OK;
  bool control = mouseEvent->IsMeta();
#else
  bool control = mouseEvent->IsControl();
#endif

  nsRefPtr<nsFrameSelection> fc = const_cast<nsFrameSelection*>(frameselection);
  if (mouseEvent->clickCount > 1) {
    
    
    fc->SetDragState(true);
    fc->SetMouseDoubleDown(true);
    return HandleMultiplePress(aPresContext, mouseEvent, aEventStatus, control);
  }

  nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(mouseEvent, this);
  ContentOffsets offsets = GetContentOffsetsFromPoint(pt, SKIP_HIDDEN);

  if (!offsets.content)
    return NS_ERROR_FAILURE;

  
  
  
  
  if (!offsets.content->IsEditable() &&
      Preferences::GetBool("browser.ignoreNativeFrameTextSelection", false)) {
    
    
    
    if (hasCapturedContent) {
      nsIPresShell::SetCapturingContent(nullptr, 0);
    }

    return fc->HandleClick(offsets.content, offsets.StartOffset(),
                           offsets.EndOffset(), false, false,
                           offsets.associate);
  }

  
  nsCOMPtr<nsIContent>parentContent;
  int32_t  contentOffset;
  int32_t target;
  rv = GetDataForTableSelection(frameselection, shell, mouseEvent,
                                getter_AddRefs(parentContent), &contentOffset,
                                &target);
  if (NS_SUCCEEDED(rv) && parentContent)
  {
    fc->SetDragState(true);
    return fc->HandleTableSelection(parentContent, contentOffset, target,
                                    mouseEvent);
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
      fc->SetDragState(false);
      fc->SetDelayedCaretData(mouseEvent);
      return NS_OK;
    }
  }

  fc->SetDragState(true);

  
  
  rv = fc->HandleClick(offsets.content, offsets.StartOffset(),
                       offsets.EndOffset(), mouseEvent->IsShift(), control,
                       offsets.associate);

  if (NS_FAILED(rv))
    return rv;

  if (offsets.offset != offsets.secondaryOffset)
    fc->MaintainSelection();

  if (isEditor && !mouseEvent->IsShift() &&
      (offsets.EndOffset() - offsets.StartOffset()) == 1)
  {
    
    
    
    
    
    fc->SetDragState(false);
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
                          offsets.associate, &offset);
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
                             WidgetGUIEvent* aEvent,
                             nsEventStatus* aEventStatus,
                             bool aControlHeld)
{
  NS_ENSURE_ARG_POINTER(aEvent);
  NS_ENSURE_ARG_POINTER(aEventStatus);

  if (nsEventStatus_eConsumeNoDefault == *aEventStatus ||
      DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }

  
  
  
  
  nsSelectionAmount beginAmount, endAmount;
  WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
  if (!mouseEvent) {
    return NS_OK;
  }

  if (mouseEvent->clickCount == 4) {
    beginAmount = endAmount = eSelectParagraph;
  } else if (mouseEvent->clickCount == 3) {
    if (Preferences::GetBool("browser.triple_click_selects_paragraph")) {
      beginAmount = endAmount = eSelectParagraph;
    } else {
      beginAmount = eSelectBeginLine;
      endAmount = eSelectEndLine;
    }
  } else if (mouseEvent->clickCount == 2) {
    
    beginAmount = endAmount = eSelectWord;
  } else {
    return NS_OK;
  }

  nsPoint relPoint =
    nsLayoutUtils::GetEventCoordinatesRelativeTo(mouseEvent, this);
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
                           nsPoint(0, 0),
                           aJumpLines,
                           true,  
                           false,
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
                              nsPoint(0, 0),
                              aJumpLines,
                              true,  
                              false,
                              false,
                              false);
  rv = baseFrame->PeekOffset(&startpos);
  if (NS_FAILED(rv))
    return rv;

  nsPeekOffsetStruct endpos(aAmountForward,
                            eDirNext,
                            aStartPos,
                            nsPoint(0, 0),
                            aJumpLines,
                            true,  
                            false,
                            false,
                            false);
  rv = PeekOffset(&endpos);
  if (NS_FAILED(rv))
    return rv;

  
  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();

  rv = frameSelection->HandleClick(startpos.mResultContent,
                                   startpos.mContentOffset, startpos.mContentOffset,
                                   false, (aSelectFlags & SELECT_ACCUMULATE),
                                   CARET_ASSOCIATE_AFTER);
  if (NS_FAILED(rv))
    return rv;

  rv = frameSelection->HandleClick(endpos.mResultContent,
                                   endpos.mContentOffset, endpos.mContentOffset,
                                   true, false,
                                   CARET_ASSOCIATE_BEFORE);
  if (NS_FAILED(rv))
    return rv;

  
  return frameSelection->MaintainSelection(aAmountBack);
}

NS_IMETHODIMP nsFrame::HandleDrag(nsPresContext* aPresContext, 
                                  WidgetGUIEvent* aEvent,
                                  nsEventStatus* aEventStatus)
{
  MOZ_ASSERT(aEvent->mClass == eMouseEventClass,
             "HandleDrag can only handle mouse event");

  bool selectable;
  IsSelectable(&selectable, nullptr);

  
  
  
  if (!selectable)
    return NS_OK;
  if (DisplaySelection(aPresContext) == nsISelectionController::SELECTION_OFF) {
    return NS_OK;
  }
  nsIPresShell *presShell = aPresContext->PresShell();

  nsRefPtr<nsFrameSelection> frameselection = GetFrameSelection();
  bool mouseDown = frameselection->GetDragState();
  if (!mouseDown)
    return NS_OK;

  frameselection->StopAutoScrollTimer();

  
  nsCOMPtr<nsIContent> parentContent;
  int32_t contentOffset;
  int32_t target;
  WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
  nsresult result;
  result = GetDataForTableSelection(frameselection, presShell, mouseEvent,
                                    getter_AddRefs(parentContent),
                                    &contentOffset, &target);      

  nsWeakFrame weakThis = this;
  if (NS_SUCCEEDED(result) && parentContent) {
    frameselection->HandleTableSelection(parentContent, contentOffset, target,
                                         mouseEvent);
  } else {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(mouseEvent, this);
    frameselection->HandleDrag(this, pt);
  }

  
  
  if (!weakThis.IsAlive()) {
    return NS_OK;
  }

  
  nsIScrollableFrame* scrollFrame =
    nsLayoutUtils::GetNearestScrollableFrame(this,
        nsLayoutUtils::SCROLLABLE_SAME_DOC |
        nsLayoutUtils::SCROLLABLE_INCLUDE_HIDDEN);

  if (scrollFrame) {
    nsIFrame* capturingFrame = scrollFrame->GetScrolledFrame();
    if (capturingFrame) {
      nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(mouseEvent,
                                                                capturingFrame);
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
                     WidgetGUIEvent*           aEvent,
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

      
      
      
      
      
      
      
      
      
      aFrameSelection->SetDragState(true);

      rv = aFrameSelection->HandleClick(aOffsets.content,
                                        aOffsets.StartOffset(),
                                        aOffsets.EndOffset(),
                                        aFrameSelection->IsShiftDownInDelayedCaretData(),
                                        false,
                                        aOffsets.associate);
      if (NS_FAILED(rv)) {
        return rv;
      }
    } else if (aParentContentForTableSel) {
      aFrameSelection->SetDragState(false);
      rv = aFrameSelection->HandleTableSelection(
                              aParentContentForTableSel,
                              aContentOffsetForTableSel,
                              aTargetForTableSel,
                              aEvent->AsMouseEvent());
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
    aFrameSelection->SetDelayedCaretData(0);
  }

  aFrameSelection->SetDragState(false);
  aFrameSelection->StopAutoScrollTimer();

  return NS_OK;
}

NS_IMETHODIMP nsFrame::HandleRelease(nsPresContext* aPresContext,
                                     WidgetGUIEvent* aEvent,
                                     nsEventStatus* aEventStatus)
{
  if (aEvent->mClass != eMouseEventClass) {
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
      
      
      

      if (frameselection->MouseDownRecorded()) {
        nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, this);
        offsets = GetContentOffsetsFromPoint(pt, SKIP_HIDDEN);
        handleTableSelection = false;
      } else {
        GetDataForTableSelection(frameselection, PresContext()->PresShell(),
                                 aEvent->AsMouseEvent(),
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
    frameSelection->SetDragState(false);
    frameSelection->StopAutoScrollTimer();
    nsIScrollableFrame* scrollFrame =
      nsLayoutUtils::GetNearestScrollableFrame(this,
        nsLayoutUtils::SCROLLABLE_SAME_DOC |
        nsLayoutUtils::SCROLLABLE_INCLUDE_HIDDEN);
    if (scrollFrame) {
      
      
      scrollFrame->ScrollSnap();
    }
  }

  
  

  return selectionOff
    ? NS_OK
    : HandleFrameSelection(frameselection, offsets, handleTableSelection,
                           contentOffsetForTableSel, targetForTableSel,
                           parentContent, aEvent, aEventStatus);
}

struct MOZ_STACK_CLASS FrameContentRange {
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
      !aFrame->StyleVisibility()->IsVisible()) {
    return false;
  }
  return !aFrame->IsGeneratedContentFrame() &&
    aFrame->StyleUIReset()->mUserSelect != NS_STYLE_USER_SELECT_NONE;
}

static bool SelectionDescendToKids(nsIFrame* aFrame) {
  uint8_t style = aFrame->StyleUIReset()->mUserSelect;
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
  nsIFrame *closestFromIStart = nullptr, *closestFromIEnd = nullptr;
  nscoord closestIStart = aLine->IStart(), closestIEnd = aLine->IEnd();
  WritingMode wm = aLine->mWritingMode;
  LogicalPoint pt(wm, aPoint, aLine->mContainerWidth);
  bool canSkipBr = false;
  for (int32_t n = aLine->GetChildCount(); n;
       --n, frame = frame->GetNextSibling()) {
    
    
    if (!SelfIsSelectable(frame, aFlags) || frame->IsEmpty() ||
        (canSkipBr && frame->GetType() == nsGkAtoms::brFrame)) {
      continue;
    }
    canSkipBr = true;
    LogicalRect frameRect = LogicalRect(wm, frame->GetRect(),
                                        aLine->mContainerWidth);
    if (pt.I(wm) >= frameRect.IStart(wm)) {
      if (pt.I(wm) < frameRect.IEnd(wm)) {
        return GetSelectionClosestFrameForChild(frame, aPoint, aFlags);
      }
      if (frameRect.IEnd(wm) >= closestIStart) {
        closestFromIStart = frame;
        closestIStart = frameRect.IEnd(wm);
      }
    } else {
      if (frameRect.IStart(wm) <= closestIEnd) {
        closestFromIEnd = frame;
        closestIEnd = frameRect.IStart(wm);
      }
    }
  }
  if (!closestFromIStart && !closestFromIEnd) {
    
    
    return FrameTarget::Null();
  }
  if (closestFromIStart &&
      (!closestFromIEnd ||
       (abs(pt.I(wm) - closestIStart) <= abs(pt.I(wm) - closestIEnd)))) {
    return GetSelectionClosestFrameForChild(closestFromIStart, aPoint,
                                            aFlags);
  }
  return GetSelectionClosestFrameForChild(closestFromIEnd, aPoint, aFlags);
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
  
  WritingMode wm = curLine->mWritingMode;
  LogicalPoint pt(wm, aPoint, curLine->mContainerWidth);
  while (curLine != end) {
    
    nscoord BCoord = pt.B(wm) - curLine->BStart();
    nscoord BSize = curLine->BSize();
    if (BCoord >= 0 && BCoord < BSize) {
      closestLine = curLine;
      break; 
    }
    if (BCoord < 0)
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
      if (pt.B(wm) - prevLine->BEnd() < nextLine->BStart() - pt.B(wm))
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
    offsets.associate = CARET_ASSOCIATE_AFTER;
    return offsets;
  }

  
  nsRect rect(nsPoint(0, 0), aFrame->GetSize());

  bool isBlock = aFrame->GetDisplay() != NS_STYLE_DISPLAY_INLINE;
  bool isRtl = (aFrame->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL);
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
  offsets.associate =
      offsets.offset == range.start ? CARET_ASSOCIATE_AFTER : CARET_ASSOCIATE_BEFORE;
  return offsets;
}

static nsIFrame* AdjustFrameForSelectionStyles(nsIFrame* aFrame) {
  nsIFrame* adjustedFrame = aFrame;
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent())
  {
    
    
    if (frame->StyleUIReset()->mUserSelect == NS_STYLE_USER_SELECT_ALL ||
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

    
    
    if (adjustedFrame && adjustedFrame->StyleUIReset()->mUserSelect ==
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
    offsets.associate = CARET_ASSOCIATE_AFTER;
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
    offsets.associate = offsets.offset == range.start ?
        CARET_ASSOCIATE_AFTER : CARET_ASSOCIATE_BEFORE;
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

void
nsIFrame::AssociateImage(const nsStyleImage& aImage, nsPresContext* aPresContext)
{
  if (aImage.GetType() != eStyleImageType_Image) {
    return;
  }

  imgIRequest *req = aImage.GetImageData();
  mozilla::css::ImageLoader* loader =
    aPresContext->Document()->StyleImageLoader();

  
  loader->AssociateRequestToFrame(req, this);
}

nsresult
nsFrame::GetCursor(const nsPoint& aPoint,
                   nsIFrame::Cursor& aCursor)
{
  FillCursorInformationFromStyle(StyleUserInterface(), aCursor);
  if (NS_STYLE_CURSOR_AUTO == aCursor.mCursor) {
    
    aCursor.mCursor =
      (mContent && mContent->IsEditable())
      ? NS_STYLE_CURSOR_TEXT : NS_STYLE_CURSOR_DEFAULT;
  }
  if (NS_STYLE_CURSOR_TEXT == aCursor.mCursor &&
      GetWritingMode().IsVertical()) {
    
    
    aCursor.mCursor = NS_STYLE_CURSOR_VERTICAL_TEXT;
  }

  return NS_OK;
}



 void
nsFrame::MarkIntrinsicISizesDirty()
{
  
  
  if (::IsBoxWrapped(this)) {
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
nsFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
nsFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;
  DISPLAY_PREF_WIDTH(this, result);
  return result;
}

 void
nsFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                           nsIFrame::InlineMinISizeData *aData)
{
  NS_ASSERTION(GetParent(), "Must have a parent if we get here!");
  nsIFrame* parent = GetParent();
  bool canBreak = !CanContinueTextRun() &&
    !parent->StyleContext()->ShouldSuppressLineBreak() &&
    parent->StyleText()->WhiteSpaceCanWrap(parent);
  
  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = false;
  aData->trailingTextFrame = nullptr;
  aData->currentLine += nsLayoutUtils::IntrinsicForContainer(aRenderingContext,
                            this, nsLayoutUtils::MIN_ISIZE);
  aData->atStartOfLine = false;
  if (canBreak)
    aData->OptionallyBreak(aRenderingContext);
}

 void
nsFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                            nsIFrame::InlinePrefISizeData *aData)
{
  aData->trailingWhitespace = 0;
  aData->skipWhitespace = false;
  nscoord myPref = nsLayoutUtils::IntrinsicForContainer(aRenderingContext, 
                       this, nsLayoutUtils::PREF_ISIZE);
  aData->currentLine = NSCoordSaturatingAdd(aData->currentLine, myPref);
}

void
nsIFrame::InlineMinISizeData::ForceBreak(nsRenderingContext *aRenderingContext)
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
nsIFrame::InlineMinISizeData::OptionallyBreak(nsRenderingContext *aRenderingContext,
                                              nscoord aHyphenWidth)
{
  trailingTextFrame = nullptr;

  
  
  
  
  
  if (currentLine + aHyphenWidth < 0 || atStartOfLine)
    return;
  currentLine += aHyphenWidth;
  ForceBreak(aRenderingContext);
}

void
nsIFrame::InlinePrefISizeData::ForceBreak(nsRenderingContext *aRenderingContext)
{
  if (floats.Length() != 0) {
            
            
    nscoord floats_done = 0,
            
            
            floats_cur_left = 0,
            floats_cur_right = 0;

    for (uint32_t i = 0, i_end = floats.Length(); i != i_end; ++i) {
      const FloatInfo& floatInfo = floats[i];
      const nsStyleDisplay *floatDisp = floatInfo.Frame()->StyleDisplay();
      if (floatDisp->mBreakType == NS_STYLE_CLEAR_LEFT ||
          floatDisp->mBreakType == NS_STYLE_CLEAR_RIGHT ||
          floatDisp->mBreakType == NS_STYLE_CLEAR_BOTH) {
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

 nsIFrame::IntrinsicISizeOffsetData
nsFrame::IntrinsicISizeOffsets(nsRenderingContext* aRenderingContext)
{
  IntrinsicISizeOffsetData result;

  WritingMode wm = GetWritingMode();

  const nsStyleMargin *styleMargin = StyleMargin();
  AddCoord(wm.IsVertical() ? styleMargin->mMargin.GetTop()
                           : styleMargin->mMargin.GetLeft(),
           aRenderingContext, this, &result.hMargin, &result.hPctMargin,
           false);
  AddCoord(wm.IsVertical() ? styleMargin->mMargin.GetBottom()
                           : styleMargin->mMargin.GetRight(),
           aRenderingContext, this, &result.hMargin, &result.hPctMargin,
           false);

  const nsStylePadding *stylePadding = StylePadding();
  AddCoord(wm.IsVertical() ? stylePadding->mPadding.GetTop()
                           : stylePadding->mPadding.GetLeft(),
           aRenderingContext, this, &result.hPadding, &result.hPctPadding,
           true);
  AddCoord(wm.IsVertical() ? stylePadding->mPadding.GetBottom()
                           : stylePadding->mPadding.GetRight(),
           aRenderingContext, this, &result.hPadding, &result.hPctPadding,
           true);

  const nsStyleBorder *styleBorder = StyleBorder();
  result.hBorder += styleBorder->GetComputedBorderWidth(
    wm.PhysicalSideForInlineAxis(eLogicalEdgeStart));
  result.hBorder += styleBorder->GetComputedBorderWidth(
    wm.PhysicalSideForInlineAxis(eLogicalEdgeEnd));

  const nsStyleDisplay *disp = StyleDisplay();
  if (IsThemed(disp)) {
    nsPresContext *presContext = PresContext();

    nsIntMargin border;
    presContext->GetTheme()->GetWidgetBorder(presContext->DeviceContext(),
                                             this, disp->mAppearance,
                                             &border);
    result.hBorder =
      presContext->DevPixelsToAppUnits(wm.IsVertical() ? border.TopBottom()
                                                       : border.LeftRight());

    nsIntMargin padding;
    if (presContext->GetTheme()->GetWidgetPadding(presContext->DeviceContext(),
                                                  this, disp->mAppearance,
                                                  &padding)) {
      result.hPadding =
        presContext->DevPixelsToAppUnits(wm.IsVertical() ? padding.TopBottom()
                                                         : padding.LeftRight());
      result.hPctPadding = 0;
    }
  }

  return result;
}

 IntrinsicSize
nsFrame::GetIntrinsicSize()
{
  return IntrinsicSize(); 
}

 nsSize
nsFrame::GetIntrinsicRatio()
{
  return nsSize(0, 0);
}


LogicalSize
nsFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                     WritingMode aWM,
                     const LogicalSize& aCBSize,
                     nscoord aAvailableISize,
                     const LogicalSize& aMargin,
                     const LogicalSize& aBorder,
                     const LogicalSize& aPadding,
                     ComputeSizeFlags aFlags)
{
  LogicalSize result = ComputeAutoSize(aRenderingContext, aWM,
                                       aCBSize, aAvailableISize,
                                       aMargin, aBorder, aPadding,
                                       aFlags & ComputeSizeFlags::eShrinkWrap);
  LogicalSize boxSizingAdjust(aWM);
  const nsStylePosition *stylePos = StylePosition();

  switch (stylePos->mBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      boxSizingAdjust += aBorder;
      
    case NS_STYLE_BOX_SIZING_PADDING:
      boxSizingAdjust += aPadding;
  }
  nscoord boxSizingToMarginEdgeISize =
    aMargin.ISize(aWM) + aBorder.ISize(aWM) + aPadding.ISize(aWM) -
    boxSizingAdjust.ISize(aWM);

  bool isVertical = aWM.IsVertical();

  const nsStyleCoord* inlineStyleCoord =
    isVertical ? &(stylePos->mHeight) : &(stylePos->mWidth);
  const nsStyleCoord* blockStyleCoord =
    isVertical ? &(stylePos->mWidth) : &(stylePos->mHeight);

  bool isFlexItem = IsFlexItem();
  bool isInlineFlexItem = false;
 
  if (isFlexItem) {
    
    
    
    
    uint32_t flexDirection = GetParent()->StylePosition()->mFlexDirection;
    isInlineFlexItem =
      flexDirection == NS_STYLE_FLEX_DIRECTION_ROW ||
      flexDirection == NS_STYLE_FLEX_DIRECTION_ROW_REVERSE;

    
    
    
    const nsStyleCoord* flexBasis = &(stylePos->mFlexBasis);
    if (flexBasis->GetUnit() != eStyleUnit_Auto) {
      if (isInlineFlexItem) {
        inlineStyleCoord = flexBasis;
      } else {
        
        
        
        
        
        
        
        if (flexBasis->GetUnit() != eStyleUnit_Enumerated) {
          blockStyleCoord = flexBasis;
        }
      }
    }
  }

  

  if (inlineStyleCoord->GetUnit() != eStyleUnit_Auto) {
    result.ISize(aWM) =
      nsLayoutUtils::ComputeISizeValue(aRenderingContext, this,
        aCBSize.ISize(aWM), boxSizingAdjust.ISize(aWM), boxSizingToMarginEdgeISize,
        *inlineStyleCoord);
  }

  const nsStyleCoord& maxISizeCoord =
    isVertical ? stylePos->mMaxHeight : stylePos->mMaxWidth;

  
  
  
  if (maxISizeCoord.GetUnit() != eStyleUnit_None &&
      !(isFlexItem && isInlineFlexItem)) {
    nscoord maxISize =
      nsLayoutUtils::ComputeISizeValue(aRenderingContext, this,
        aCBSize.ISize(aWM), boxSizingAdjust.ISize(aWM), boxSizingToMarginEdgeISize,
        maxISizeCoord);
    result.ISize(aWM) = std::min(maxISize, result.ISize(aWM));
  }

  const nsStyleCoord& minISizeCoord =
    isVertical ? stylePos->mMinHeight : stylePos->mMinWidth;

  nscoord minISize;
  if (minISizeCoord.GetUnit() != eStyleUnit_Auto &&
      !(isFlexItem && isInlineFlexItem)) {
    minISize =
      nsLayoutUtils::ComputeISizeValue(aRenderingContext, this,
        aCBSize.ISize(aWM), boxSizingAdjust.ISize(aWM), boxSizingToMarginEdgeISize,
        minISizeCoord);
  } else {
    
    
    
    
    
    minISize = 0;
  }
  result.ISize(aWM) = std::max(minISize, result.ISize(aWM));

  
  
  
  
  if (!nsLayoutUtils::IsAutoBSize(*blockStyleCoord, aCBSize.BSize(aWM)) &&
      !(aFlags & nsIFrame::eUseAutoHeight)) {
    result.BSize(aWM) =
      nsLayoutUtils::ComputeBSizeValue(aCBSize.BSize(aWM),
                                       boxSizingAdjust.BSize(aWM),
                                       *blockStyleCoord);
  }

  const nsStyleCoord& maxBSizeCoord =
    isVertical ? stylePos->mMaxWidth : stylePos->mMaxHeight;

  if (result.BSize(aWM) != NS_UNCONSTRAINEDSIZE) {
    if (!nsLayoutUtils::IsAutoBSize(maxBSizeCoord, aCBSize.BSize(aWM)) &&
        !(isFlexItem && !isInlineFlexItem)) {
      nscoord maxBSize =
        nsLayoutUtils::ComputeBSizeValue(aCBSize.BSize(aWM),
                                         boxSizingAdjust.BSize(aWM),
                                         maxBSizeCoord);
      result.BSize(aWM) = std::min(maxBSize, result.BSize(aWM));
    }

    const nsStyleCoord& minBSizeCoord =
      isVertical ? stylePos->mMinWidth : stylePos->mMinHeight;

    if (!nsLayoutUtils::IsAutoBSize(minBSizeCoord, aCBSize.BSize(aWM)) &&
        !(isFlexItem && !isInlineFlexItem)) {
      nscoord minBSize =
        nsLayoutUtils::ComputeBSizeValue(aCBSize.BSize(aWM),
                                         boxSizingAdjust.BSize(aWM),
                                         minBSizeCoord);
      result.BSize(aWM) = std::max(minBSize, result.BSize(aWM));
    }
  }

  const nsStyleDisplay *disp = StyleDisplay();
  if (IsThemed(disp)) {
    LayoutDeviceIntSize widget;
    bool canOverride = true;
    nsPresContext *presContext = PresContext();
    presContext->GetTheme()->
      GetMinimumWidgetSize(presContext, this, disp->mAppearance,
                           &widget, &canOverride);

    
    nsSize size;
    size.width = presContext->DevPixelsToAppUnits(widget.width);
    size.height = presContext->DevPixelsToAppUnits(widget.height);

    
    size.width -= aBorder.Width(aWM) + aPadding.Width(aWM);
    size.height -= aBorder.Height(aWM) + aPadding.Height(aWM);

    if (size.height > result.Height(aWM) || !canOverride) {
      result.Height(aWM) = size.height;
    }
    if (size.width > result.Width(aWM) || !canOverride) {
      result.Width(aWM) = size.width;
    }
  }

  result.ISize(aWM) = std::max(0, result.ISize(aWM));
  result.BSize(aWM) = std::max(0, result.BSize(aWM));

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
  if (StyleOutline()->GetOutlineStyle() != NS_STYLE_BORDER_STYLE_NONE ||
      StyleBorder()->HasBorder() || !StyleBackground()->IsTransparent() ||
      StyleDisplay()->mAppearance) {
    
    
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

 nsresult
nsIFrame::GetPrefWidthTightBounds(nsRenderingContext* aContext,
                                  nscoord* aX,
                                  nscoord* aXMost)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


LogicalSize
nsFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                         WritingMode aWM,
                         const mozilla::LogicalSize& aCBSize,
                         nscoord aAvailableISize,
                         const mozilla::LogicalSize& aMargin,
                         const mozilla::LogicalSize& aBorder,
                         const mozilla::LogicalSize& aPadding,
                         bool aShrinkWrap)
{
  
  LogicalSize result(aWM, 0xdeadbeef, NS_UNCONSTRAINEDSIZE);

  
  const nsStyleCoord& inlineStyleCoord =
    aWM.IsVertical() ? StylePosition()->mHeight : StylePosition()->mWidth;
  if (inlineStyleCoord.GetUnit() == eStyleUnit_Auto) {
    nscoord availBased = aAvailableISize - aMargin.ISize(aWM) -
                         aBorder.ISize(aWM) - aPadding.ISize(aWM);
    result.ISize(aWM) = ShrinkWidthToFit(aRenderingContext, availBased);
  }
  return result;
}

nscoord
nsFrame::ShrinkWidthToFit(nsRenderingContext *aRenderingContext,
                          nscoord aWidthInCB)
{
  
  
  AutoMaybeDisableFontInflation an(this);

  nscoord result;
  nscoord minWidth = GetMinISize(aRenderingContext);
  if (minWidth > aWidthInCB) {
    result = minWidth;
  } else {
    nscoord prefWidth = GetPrefISize(aRenderingContext);
    if (prefWidth > aWidthInCB) {
      result = aWidthInCB;
    } else {
      result = prefWidth;
    }
  }
  return result;
}

void
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

  aPresContext->ReflowedFrame();
}

void
nsFrame::FinishReflowWithAbsoluteFrames(nsPresContext*           aPresContext,
                                        nsHTMLReflowMetrics&     aDesiredSize,
                                        const nsHTMLReflowState& aReflowState,
                                        nsReflowStatus&          aStatus,
                                        bool                     aConstrainHeight)
{
  ReflowAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus, aConstrainHeight);

  FinishAndStoreOverflow(&aDesiredSize);
}

void
nsFrame::ReflowAbsoluteFrames(nsPresContext*           aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nsReflowStatus&          aStatus,
                              bool                     aConstrainHeight)
{
  if (HasAbsolutelyPositionedChildren()) {
    nsAbsoluteContainingBlock* absoluteContainer = GetAbsoluteContainingBlock();

    
    

    
    nsMargin computedBorder =
      aReflowState.ComputedPhysicalBorderPadding() - aReflowState.ComputedPhysicalPadding();
    nscoord containingBlockWidth =
      aDesiredSize.Width() - computedBorder.LeftRight();
    nscoord containingBlockHeight =
      aDesiredSize.Height() - computedBorder.TopBottom();

    nsContainerFrame* container = do_QueryFrame(this);
    NS_ASSERTION(container, "Abs-pos children only supported on container frames for now");

    nsRect containingBlock(0, 0, containingBlockWidth, containingBlockHeight);
    absoluteContainer->Reflow(container, aPresContext, aReflowState, aStatus,
                              containingBlock,
                              aConstrainHeight, true, true, 
                              &aDesiredSize.mOverflowAreas);
  }
}

void
nsFrame::PushDirtyBitToAbsoluteFrames()
{
  if (!(GetStateBits() & NS_FRAME_IS_DIRTY)) {
    return;  
  }
  if (!HasAbsolutelyPositionedChildren()) {
    return;  
  }
  GetAbsoluteContainingBlock()->MarkAllFramesDirty();
}

 bool
nsFrame::CanContinueTextRun() const
{
  
  
  return false;
}

void
nsFrame::Reflow(nsPresContext*          aPresContext,
                nsHTMLReflowMetrics&     aDesiredSize,
                const nsHTMLReflowState& aReflowState,
                nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsFrame");
  aDesiredSize.ClearSize();
  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsresult
nsFrame::CharacterDataChanged(CharacterDataChangeInfo* aInfo)
{
  NS_NOTREACHED("should only be called for text frames");
  return NS_OK;
}

nsresult
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

void
nsFrame::SetPrevContinuation(nsIFrame* aPrevContinuation)
{
  MOZ_ASSERT(false, "not splittable");
}

nsIFrame* nsFrame::GetNextContinuation() const
{
  return nullptr;
}

void
nsFrame::SetNextContinuation(nsIFrame*)
{
  MOZ_ASSERT(false, "not splittable");
}

nsIFrame* nsFrame::GetPrevInFlowVirtual() const
{
  return nullptr;
}

void
nsFrame::SetPrevInFlow(nsIFrame* aPrevInFlow)
{
  MOZ_ASSERT(false, "not splittable");
}

nsIFrame* nsFrame::GetNextInFlowVirtual() const
{
  return nullptr;
}

void
nsFrame::SetNextInFlow(nsIFrame*)
{
  MOZ_ASSERT(false, "not splittable");
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

#ifdef DEBUG
    nsIAtom* frameType = GetType();
    NS_ASSERTION(frameType == nsGkAtoms::scrollFrame ||
                 frameType == nsGkAtoms::subDocumentFrame ||
                 frameType == nsGkAtoms::listControlFrame ||
                 frameType == nsGkAtoms::objectFrame ||
                 frameType == nsGkAtoms::viewportFrame ||
                 frameType == nsGkAtoms::menuPopupFrame,
                 "Only specific frame types can have an nsView");
#endif

    
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
  for (nsIFrame* f = GetParent(); nullptr != f; f = f->GetParent()) {
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
        
        offset += docOffset.ScaleToOtherAppUnits(currAPD, aAPD);
        docOffset.x = docOffset.y = 0;
      }
      currAPD = newAPD;
      docOffset += newOffset;
    }
  }
  if (f == aOther) {
    offset += docOffset.ScaleToOtherAppUnits(currAPD, aAPD);
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
      LayoutDeviceIntPoint rootDevPx = rootWidget->WidgetToScreenOffset();
      rootScreenPos.x = presContext->DevPixelsToAppUnits(rootDevPx.x);
      rootScreenPos.y = presContext->DevPixelsToAppUnits(rootDevPx.y);
    }
  }

  return nsRect(rootScreenPos + GetOffsetTo(rootFrame), GetSize());
}



void
nsIFrame::GetOffsetFromView(nsPoint& aOffset, nsView** aView) const
{
  NS_PRECONDITION(nullptr != aView, "null OUT parameter pointer");
  nsIFrame* frame = const_cast<nsIFrame*>(this);

  *aView = nullptr;
  aOffset.MoveTo(0, 0);
  do {
    aOffset += frame->GetPosition();
    frame = frame->GetParent();
  } while (frame && !frame->HasView());

  if (frame) {
    *aView = frame->GetView();
  }
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

Matrix4x4
nsIFrame::GetTransformMatrix(const nsIFrame* aStopAtAncestor,
                             nsIFrame** aOutAncestor)
{
  NS_PRECONDITION(aOutAncestor, "Need a place to put the ancestor!");

  



  if (IsTransformed()) {
    


    NS_ASSERTION(nsLayoutUtils::GetCrossDocParentFrame(this),
                 "Cannot transform the viewport frame!");
    int32_t scaleFactor = PresContext()->AppUnitsPerDevPixel();

    Matrix4x4 result = ToMatrix4x4(
      nsDisplayTransform::GetResultingTransformMatrix(this, nsPoint(0, 0), scaleFactor, nullptr, aOutAncestor));
    
    nsPoint delta = GetOffsetToCrossDoc(*aOutAncestor);
    
    result.PostTranslate(NSAppUnitsToFloatPixels(delta.x, scaleFactor),
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

        Matrix4x4 transformToTop;
        transformToTop._41 = translation.x;
        transformToTop._42 = translation.y;

        *aOutAncestor = docRootFrame;
        Matrix4x4 docRootTransformToTop =
          nsLayoutUtils::GetTransformToAncestor(docRootFrame, nullptr);
        if (docRootTransformToTop.IsSingular()) {
          NS_WARNING("Containing document is invisible, we can't compute a valid transform");
        } else {
          docRootTransformToTop.Invert();
          return transformToTop * docRootTransformToTop;
        }
      }
    }
  }

  *aOutAncestor = nsLayoutUtils::GetCrossDocParentFrame(this);

  







  if (!*aOutAncestor)
    return Matrix4x4();

  
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
  return Matrix4x4::Translation(NSAppUnitsToFloatPixels(delta.x, scaleFactor),
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
nsIFrame::TryUpdateTransformOnly(Layer** aLayerResult)
{
  Layer* layer = FrameLayerBuilder::GetDedicatedLayer(
    this, nsDisplayItem::TYPE_TRANSFORM);
  if (!layer || !layer->HasUserData(LayerIsPrerenderedDataKey())) {
    
    
    
    return false;
  }

  gfx::Matrix4x4 transform3d;
  if (!nsLayoutUtils::GetLayerTransformForFrame(this, &transform3d)) {
    
    
    
    return false;
  }
  gfx::Matrix transform;
  gfx::Matrix previousTransform;
  
  
  
  
  
  
  
 static const gfx::Float kError = 0.0001f;
  if (!transform3d.Is2D(&transform) ||
      !layer->GetBaseTransform().Is2D(&previousTransform) ||
      !gfx::FuzzyEqual(transform._11, previousTransform._11, kError) ||
      !gfx::FuzzyEqual(transform._22, previousTransform._22, kError) ||
      !gfx::FuzzyEqual(transform._21, previousTransform._21, kError) ||
      !gfx::FuzzyEqual(transform._12, previousTransform._12, kError)) {
    return false;
  }
  layer->SetBaseTransformForNextTransaction(transform3d);
  *aLayerResult = layer;
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
nsIFrame::SchedulePaint(PaintType aType)
{
  nsIFrame *displayRoot = nsLayoutUtils::GetDisplayRootFrame(this);
  nsPresContext *pres = displayRoot->PresContext()->GetRootPresContext();

  
  
  if (!pres || (pres->Document() && pres->Document()->IsResourceDoc())) {
    return;
  }
  if (!pres->GetContainerWeak()) {
    NS_WARNING("Shouldn't call SchedulePaint in a detached pres context");
    return;
  }

  pres->PresShell()->ScheduleViewManagerFlush(aType == PAINT_DELAYED_COMPRESS ?
                                              nsIPresShell::PAINT_DELAYED_COMPRESS :
                                              nsIPresShell::PAINT_DEFAULT);

  if (aType == PAINT_DELAYED_COMPRESS) {
    return;
  }

  if (aType == PAINT_DEFAULT) {
    displayRoot->AddStateBits(NS_FRAME_UPDATE_LAYER_TREE);
  }
  nsIPresShell* shell = PresContext()->PresShell();
  if (shell) {
    shell->AddInvalidateHiddenPresShellObserver(pres->RefreshDriver());
  }
}

Layer*
nsIFrame::InvalidateLayer(uint32_t aDisplayItemKey,
                          const nsIntRect* aDamageRect,
                          const nsRect* aFrameDamageRect,
                          uint32_t aFlags )
{
  NS_ASSERTION(aDisplayItemKey > 0, "Need a key");

  Layer* layer = FrameLayerBuilder::GetDedicatedLayer(this, aDisplayItemKey);

  
  
  if ((aFlags & UPDATE_IS_ASYNC) && layer &&
      layer->Manager()->GetBackendType() == LayersBackend::LAYERS_CLIENT) {
    return layer;
  }

  if (!layer) {
    if (aFrameDamageRect && aFrameDamageRect->IsEmpty()) {
      return nullptr;
    }

    
    
    
    
    
    
    
    
    uint32_t displayItemKey = aDisplayItemKey;
    if (aDisplayItemKey == nsDisplayItem::TYPE_PLUGIN ||
        aDisplayItemKey == nsDisplayItem::TYPE_REMOTE) {
      displayItemKey = 0;
    }

    if (aFrameDamageRect) {
      InvalidateFrameWithRect(*aFrameDamageRect, displayItemKey);
    } else {
      InvalidateFrame(displayItemKey);
    }

    return nullptr;
  }

  if (aDamageRect && aDamageRect->IsEmpty()) {
    return layer;
  }

  if (aDamageRect) {
    layer->AddInvalidRect(*aDamageRect);
  } else {
    layer->SetInvalidRectToVisibleRegion();
  }

  SchedulePaint(PAINT_COMPOSITE_ONLY);
  return layer;
}

static nsRect
ComputeEffectsRect(nsIFrame* aFrame, const nsRect& aOverflowRect,
                   const nsSize& aNewSize)
{
  nsRect r = aOverflowRect;

  if (aFrame->GetStateBits() & NS_FRAME_SVG_LAYOUT) {
    
    
    
    if (aFrame->StyleSVGReset()->HasFilters()) {
      aFrame->Properties().
        Set(nsIFrame::PreEffectsBBoxProperty(), new nsRect(r));
      r = nsSVGUtils::GetPostFilterVisualOverflowRect(aFrame, aOverflowRect);
    }
    return r;
  }

  
  r.UnionRect(r, nsLayoutUtils::GetBoxShadowRectForFrame(aFrame, aNewSize));

  
  
  

  
  
  
  
  
  
  
  
  const nsStyleBorder* styleBorder = aFrame->StyleBorder();
  nsMargin outsetMargin = styleBorder->GetImageOutset();

  if (outsetMargin != nsMargin(0, 0, 0, 0)) {
    nsRect outsetRect(nsPoint(0, 0), aNewSize);
    outsetRect.Inflate(outsetMargin);
    r.UnionRect(r, outsetRect);
  }

  
  
  
  
  
  

  if (nsSVGIntegrationUtils::UsingEffectsForFrame(aFrame)) {
    aFrame->Properties().
      Set(nsIFrame::PreEffectsBBoxProperty(), new nsRect(r));
    r = nsSVGIntegrationUtils::ComputePostEffectsVisualOverflowRect(aFrame, r);
  }

  return r;
}

void
nsIFrame::MovePositionBy(const nsPoint& aTranslation)
{
  nsPoint position = GetNormalPosition() + aTranslation;

  const nsMargin* computedOffsets = nullptr;
  if (IsRelativelyPositioned()) {
    computedOffsets = static_cast<nsMargin*>
      (Properties().Get(nsIFrame::ComputedOffsetProperty()));
  }
  nsHTMLReflowState::ApplyRelativePositioning(this, computedOffsets ?
                                              *computedOffsets : nsMargin(),
                                              &position);
  SetPosition(position);
}

nsRect
nsIFrame::GetNormalRect() const
{
  
  
  nsPoint* normalPosition = static_cast<nsPoint*>
    (Properties().Get(NormalPositionProperty()));
  if (normalPosition) {
    return nsRect(*normalPosition, GetSize());
  }
  return GetRect();
}

nsPoint
nsIFrame::GetNormalPosition() const
{
  
  
  nsPoint* normalPosition = static_cast<nsPoint*>
    (Properties().Get(NormalPositionProperty()));
  if (normalPosition) {
    return *normalPosition;
  }
  return GetPosition();
}

nsPoint
nsIFrame::GetPositionIgnoringScrolling()
{
  return GetParent() ? GetParent()->GetPositionOfChildIgnoringScrolling(this)
    : GetPosition();
}

nsRect
nsIFrame::GetOverflowRect(nsOverflowType aType) const
{
  MOZ_ASSERT(aType == eVisualOverflow || aType == eScrollableOverflow,
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

nsOverflowAreas
nsIFrame::GetOverflowAreasRelativeToSelf() const
{
  if (IsTransformed()) {
    nsOverflowAreas* preTransformOverflows = static_cast<nsOverflowAreas*>
      (Properties().Get(PreTransformOverflowAreasProperty()));
    if (preTransformOverflows) {
      return nsOverflowAreas(preTransformOverflows->VisualOverflow(),
                             preTransformOverflows->ScrollableOverflow());
    }
  }
  return nsOverflowAreas(GetVisualOverflowRect(),
                         GetScrollableOverflowRect());
}

nsRect
nsIFrame::GetScrollableOverflowRectRelativeToParent() const
{
  return GetScrollableOverflowRect() + mRect.TopLeft();
}

nsRect
nsIFrame::GetVisualOverflowRectRelativeToParent() const
{
  return GetVisualOverflowRect() + mRect.TopLeft();
}

nsRect
nsIFrame::GetScrollableOverflowRectRelativeToSelf() const
{
  if (IsTransformed()) {
    nsOverflowAreas* preTransformOverflows = static_cast<nsOverflowAreas*>
      (Properties().Get(PreTransformOverflowAreasProperty()));
    if (preTransformOverflows)
      return preTransformOverflows->ScrollableOverflow();
  }
  return GetScrollableOverflowRect();
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

inline static bool
FrameMaintainsOverflow(nsIFrame* aFrame)
{
  return (aFrame->GetStateBits() &
          (NS_FRAME_SVG_LAYOUT | NS_FRAME_IS_NONDISPLAY)) !=
         (NS_FRAME_SVG_LAYOUT | NS_FRAME_IS_NONDISPLAY);
}

 bool
nsFrame::UpdateOverflow()
{
  MOZ_ASSERT(FrameMaintainsOverflow(this),
             "Non-display SVG do not maintain visual overflow rects");

  nsRect rect(nsPoint(0, 0), GetSize());
  nsOverflowAreas overflowAreas(rect, rect);

  if (!DoesClipChildren() &&
      !(IsCollapsed() && (IsBoxFrame() || ::IsBoxWrapped(this)))) {
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
    aMetrics.ClearSize();
    aMetrics.SetBlockStartAscent(0);
    aMetrics.mCarriedOutBEndMargin.Zero();
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
  nsIAtom *pseudoType = StyleContext()->GetPseudo();
  return (pseudoType == nsCSSAnonBoxes::mozAnonymousBlock ||
          pseudoType == nsCSSAnonBoxes::mozAnonymousPositionedBlock ||
          pseudoType == nsCSSAnonBoxes::buttonContent ||
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

#ifdef DEBUG_FRAME_DUMP

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
  ((nsFrame*)aFrame)->List(stderr);
}

void
nsIFrame::ListTag(nsACString& aTo) const
{
  ListTag(aTo, this);
}


void
nsIFrame::ListTag(nsACString& aTo, const nsIFrame* aFrame) {
  nsAutoString tmp;
  aFrame->GetFrameName(tmp);
  aTo += NS_ConvertUTF16toUTF8(tmp).get();
  aTo += nsPrintfCString("@%p", static_cast<const void*>(aFrame));
}


void
nsIFrame::ListGeneric(nsACString& aTo, const char* aPrefix, uint32_t aFlags) const
{
  aTo =+ aPrefix;
  ListTag(aTo);
  if (HasView()) {
    aTo += nsPrintfCString(" [view=%p]", static_cast<void*>(GetView()));
  }
  if (GetNextSibling()) {
    aTo += nsPrintfCString(" next=%p", static_cast<void*>(GetNextSibling()));
  }
  if (GetPrevContinuation()) {
    bool fluid = GetPrevInFlow() == GetPrevContinuation();
    aTo += nsPrintfCString(" prev-%s=%p", fluid?"in-flow":"continuation",
            static_cast<void*>(GetPrevContinuation()));
  }
  if (GetNextContinuation()) {
    bool fluid = GetNextInFlow() == GetNextContinuation();
    aTo += nsPrintfCString(" next-%s=%p", fluid?"in-flow":"continuation",
            static_cast<void*>(GetNextContinuation()));
  }
  void* IBsibling = Properties().Get(IBSplitSibling());
  if (IBsibling) {
    aTo += nsPrintfCString(" IBSplitSibling=%p", IBsibling);
  }
  void* IBprevsibling = Properties().Get(IBSplitPrevSibling());
  if (IBprevsibling) {
    aTo += nsPrintfCString(" IBSplitPrevSibling=%p", IBprevsibling);
  }
  aTo += nsPrintfCString(" {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  nsIFrame* f = const_cast<nsIFrame*>(this);
  if (f->HasOverflowAreas()) {
    nsRect vo = f->GetVisualOverflowRect();
    if (!vo.IsEqualEdges(mRect)) {
      aTo += nsPrintfCString(" vis-overflow=%d,%d,%d,%d", vo.x, vo.y, vo.width, vo.height);
    }
    nsRect so = f->GetScrollableOverflowRect();
    if (!so.IsEqualEdges(mRect)) {
      aTo += nsPrintfCString(" scr-overflow=%d,%d,%d,%d", so.x, so.y, so.width, so.height);
    }
  }
  if (0 != mState) {
    aTo += nsPrintfCString(" [state=%016llx]", (unsigned long long)mState);
  }
  if (IsTransformed()) {
    aTo += nsPrintfCString(" transformed");
  }
  if (ChildrenHavePerspective()) {
    aTo += nsPrintfCString(" perspective");
  }
  if (Preserves3DChildren()) {
    aTo += nsPrintfCString(" preserves-3d-children");
  }
  if (Preserves3D()) {
    aTo += nsPrintfCString(" preserves-3d");
  }
  if (mContent) {
    aTo += nsPrintfCString(" [content=%p]", static_cast<void*>(mContent));
  }
  aTo += nsPrintfCString(" [sc=%p", static_cast<void*>(mStyleContext));
  if (mStyleContext) {
    nsIAtom* pseudoTag = mStyleContext->GetPseudo();
    if (pseudoTag) {
      nsAutoString atomString;
      pseudoTag->ToString(atomString);
      aTo += nsPrintfCString("%s", NS_LossyConvertUTF16toASCII(atomString).get());
    }
    if (!mStyleContext->GetParent() ||
        (GetParent() && GetParent()->StyleContext() != mStyleContext->GetParent())) {
      aTo += nsPrintfCString("^%p", mStyleContext->GetParent());
      if (mStyleContext->GetParent()) {
        aTo += nsPrintfCString("^%p", mStyleContext->GetParent()->GetParent());
        if (mStyleContext->GetParent()->GetParent()) {
          aTo += nsPrintfCString("^%p", mStyleContext->GetParent()->GetParent()->GetParent());
        }
      }
    }
  }
  aTo += "]";
}

void
nsIFrame::List(FILE* out, const char* aPrefix, uint32_t aFlags) const
{
  nsCString str;
  ListGeneric(str, aPrefix, aFlags);
  fprintf_stderr(out, "%s\n", str.get());
}

nsresult
nsFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Frame"), aResult);
}

nsresult
nsFrame::MakeFrameName(const nsAString& aType, nsAString& aResult) const
{
  aResult = aType;
  if (mContent && !mContent->IsNodeOfType(nsINode::eTEXT)) {
    nsAutoString buf;
    mContent->NodeInfo()->NameAtom()->ToString(buf);
    if (GetType() == nsGkAtoms::subDocumentFrame) {
      nsAutoString src;
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src);
      buf.AppendLiteral(" src=");
      buf.Append(src);
    }
    aResult.Append('(');
    aResult.Append(buf);
    aResult.Append(')');
  }
  char buf[40];
  PR_snprintf(buf, sizeof(buf), "(%d)", ContentIndexInContainer(this));
  AppendASCIItoUTF16(buf, aResult);
  return NS_OK;
}

void
nsIFrame::DumpFrameTree()
{
  RootFrameList(PresContext(), stderr);
}

void
nsIFrame::DumpFrameTreeLimited()
{
  List(stderr);
}

void
nsIFrame::RootFrameList(nsPresContext* aPresContext, FILE* out, const char* aPrefix)
{
  if (!aPresContext || !out)
    return;

  nsIPresShell *shell = aPresContext->GetPresShell();
  if (shell) {
    nsIFrame* frame = shell->FrameManager()->GetRootFrame();
    if(frame) {
      frame->List(out, aPrefix);
    }
  }
}
#endif

#ifdef DEBUG
nsFrameState
nsFrame::GetDebugStateBits() const
{
  
  
  
  
  
  
  
#define IRRELEVANT_FRAME_STATE_FLAGS NS_FRAME_EXTERNAL_REFERENCE

#define FRAME_STATE_MASK (~(IRRELEVANT_FRAME_STATE_FLAGS))

  return GetStateBits() & FRAME_STATE_MASK;
}

void
nsFrame::XMLQuote(nsString& aString)
{
  int32_t i, len = aString.Length();
  for (i = 0; i < len; i++) {
    char16_t ch = aString.CharAt(i);
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
  if (!StyleVisibility()->IsVisible())
    return false;
  nsISelection* sel = aBuilder->GetBoundingSelection();
  return !sel || IsVisibleInSelection(sel);
}

bool
nsIFrame::IsVisibleForPainting() {
  if (!StyleVisibility()->IsVisible())
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
  if (!StyleVisibility()->IsVisibleOrCollapsed())
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

nsresult
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
  nsRefPtr<nsFrameSelection> fs =
    const_cast<nsFrameSelection*>(GetConstFrameSelection());
  return fs.forget();
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
nsresult
nsFrame::DumpRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent)
{
  IndentBy(out, aIndent);
  fprintf(out, "<frame va=\"%p\" type=\"", (void*)this);
  nsAutoString name;
  GetFrameName(name);
  XMLQuote(name);
  fputs(NS_LossyConvertUTF16toASCII(name).get(), out);
  fprintf(out, "\" state=\"%016llx\" parent=\"%p\">\n",
          (unsigned long long)GetDebugStateBits(), (void*)GetParent());

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

nsresult
nsFrame::GetPointFromOffset(int32_t inOffset, nsPoint* outPoint)
{
  NS_PRECONDITION(outPoint != nullptr, "Null parameter");
  nsRect contentRect = GetContentRectRelativeToSelf();
  nsPoint pt = contentRect.TopLeft();
  if (mContent)
  {
    nsIContent* newContent = mContent->GetParent();
    if (newContent){
      int32_t newOffset = newContent->IndexOf(mContent);

      
      
      
      
      
      bool hasEmbeddingLevel;
      nsBidiLevel embeddingLevel =
        NS_PTR_TO_INT32(Properties().Get(nsIFrame::EmbeddingLevelProperty(),
                                         &hasEmbeddingLevel));
      bool isRTL = hasEmbeddingLevel
        ? IS_LEVEL_RTL(embeddingLevel)
        : StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
      if ((!isRTL && inOffset > newOffset) ||
          (isRTL && inOffset <= newOffset)) {
        pt = contentRect.TopRight();
      }
    }
  }
  *outPoint = pt;
  return NS_OK;
}

nsresult
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
  aPos->mAttach =
      aPos->mDirection == eDirNext ? CARET_ASSOCIATE_AFTER : CARET_ASSOCIATE_BEFORE;

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
    result = it->GetLine(searchingLine, &firstFrame, &lineFrameCount,
                         rect);
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
      nsPoint newDesiredPos =
        aPos->mDesiredPos - offset; 
      result = it->FindFrameAt(searchingLine, newDesiredPos, &resultFrame,
                               &isBeforeFirstFrame, &isAfterLastFrame);
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
        nsRect tempRect = resultFrame->GetRect();
        nsPoint offset;
        nsView * view; 
        resultFrame->GetOffsetFromView(offset, &view);
        if (!view) {
          return NS_ERROR_FAILURE;
        }
        if (resultFrame->GetWritingMode().IsVertical()) {
          point.y = aPos->mDesiredPos.y;
          point.x = tempRect.width + offset.x;
        } else {
          point.y = tempRect.height + offset.y;
          point.x = aPos->mDesiredPos.x;
        }

        
        
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
                  aPos->mAttach = CARET_ASSOCIATE_BEFORE;
                  if ((point.x - offset.x+ tempRect.x)>tempRect.width)
                  {
                    aPos->mContentOffset++;
                    aPos->mAttach = CARET_ASSOCIATE_AFTER;
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
          aPos->mAttach = offsets.associate;
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
        nsPoint point = aPos->mDesiredPos;
        nsView* view;
        nsPoint offset;
        resultFrame->GetOffsetFromView(offset, &view);
        ContentOffsets offsets =
            resultFrame->GetContentOffsetsFromPoint(point - offset);
        aPos->mResultContent = offsets.content;
        aPos->mContentOffset = offsets.offset;
        aPos->mAttach = offsets.associate;
        if (offsets.content)
        {
          bool selectable;
          resultFrame->IsSelectable(&selectable, nullptr);
          if (selectable)
          {
            found = true;
            if (resultFrame == farStoppingFrame)
              aPos->mAttach = CARET_ASSOCIATE_BEFORE;
            else
              aPos->mAttach = CARET_ASSOCIATE_AFTER;
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
      aPos->mAttach = aPos->mDirection == eDirNext ?
          CARET_ASSOCIATE_BEFORE : CARET_ASSOCIATE_AFTER;
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
  
  
  
  
  
  
  if ((nsLayoutUtils::GetAsBlock(aFrame) &&
       !(aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT)) ||
      aFrame->GetType() == nsGkAtoms::brFrame) {
    nsIContent* content = aFrame->GetContent();
    result.mContent = content->GetParent();
    
    
    
    NS_ASSERTION(result.mContent, "Unexpected orphan content");
    if (result.mContent)
      result.mOffset = result.mContent->IndexOf(content) + 
        (aDirection == eDirPrevious ? 1 : 0);
    return result;
  }

  
  if (aFrame->HasSignificantTerminalNewline()) {
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
  bool isReverseDirection = aVisual && REVERSED_DIRECTION_FRAME(frame);
  return aDirection == (isReverseDirection ? eDirPrevious : eDirNext);
}

nsresult
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
      nsIFrame::FrameSearchResult peekSearchState = CONTINUE;
      bool jumpedLine = false;
      bool movedOverNonSelectableText = false;
      
      while (peekSearchState != FOUND) {
        bool movingInFrameDirection =
          IsMovingInFrameDirection(current, aPos->mDirection, aPos->mVisual);

        if (eatingNonRenderableWS)
          peekSearchState = current->PeekOffsetNoAmount(movingInFrameDirection, &offset); 
        else
          peekSearchState = current->PeekOffsetCharacter(movingInFrameDirection, &offset,
                                              aPos->mAmount == eSelectCluster);

        movedOverNonSelectableText |= (peekSearchState == CONTINUE_UNSELECTABLE);

        if (peekSearchState != FOUND) {
          bool movedOverNonSelectable = false;
          result =
            current->GetFrameFromDirection(aPos->mDirection, aPos->mVisual,
                                           aPos->mJumpLines, aPos->mScrollViewStop,
                                           &current, &offset, &jumpedLine,
                                           &movedOverNonSelectable);
          if (NS_FAILED(result))
            return result;

          
          
          if (jumpedLine)
            eatingNonRenderableWS = true;

          
          if (movedOverNonSelectable) {
            movedOverNonSelectableText = true;
          }
        }

        
        
        
        if (peekSearchState == FOUND && movedOverNonSelectableText &&
            !aPos->mExtend)
        {
          int32_t start, end;
          current->GetOffsets(start, end);
          offset = aPos->mDirection == eDirNext ? 0 : end - start;
        }
      }

      
      range = GetRangeForFrame(current);
      aPos->mResultFrame = current;
      aPos->mResultContent = range.content;
      
      aPos->mContentOffset = offset < 0 ? range.end : range.start + offset;
      
      
      
      if (offset < 0 && jumpedLine &&
          aPos->mDirection == eDirPrevious &&
          current->HasSignificantTerminalNewline()) {
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
                                       aPos->mIsKeyboardSelect, &offset, &state) == FOUND;
        
        if (!done) {
          nsIFrame* nextFrame;
          int32_t nextFrameOffset;
          bool jumpedLine, movedOverNonSelectableText;
          result =
            current->GetFrameFromDirection(aPos->mDirection, aPos->mVisual,
                                           aPos->mJumpLines, aPos->mScrollViewStop,
                                           &nextFrame, &nextFrameOffset, &jumpedLine,
                                           &movedOverNonSelectableText);
          
          
          if (NS_FAILED(result) ||
              (jumpedLine && !wordSelectEatSpace && state.mSawBeforeType)) {
            done = true;
            
            
            if (jumpedLine && wordSelectEatSpace &&
                current->HasSignificantTerminalNewline()) {
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
      nsIFrame* baseFrame = nullptr;
      bool endOfLine = (eSelectEndLine == aPos->mAmount);

      if (aPos->mVisual && PresContext()->BidiEnabled()) {
        bool lineIsRTL = it->GetDirection();
        bool isReordered;
        nsIFrame *lastFrame;
        result = it->CheckLineOrder(thisLine, &isReordered, &firstFrame, &lastFrame);
        baseFrame = endOfLine ? lastFrame : firstFrame;
        if (baseFrame) {
          bool frameIsRTL =
            (nsBidiPresUtils::FrameDirection(baseFrame) == NSBIDI_RTL);
          
          
          
          if (frameIsRTL != lineIsRTL) {
            endOfLine = !endOfLine;
          }
        }
      } else {
        it->GetLine(thisLine, &firstFrame, &lineFrameCount, usedRect);

        nsIFrame* frame = firstFrame;
        for (int32_t count = lineFrameCount; count;
             --count, frame = frame->GetNextSibling()) {
          if (!frame->IsGeneratedContentFrame()) {
            
            
            if (endOfLine && lineFrameCount > 1 &&
                frame->GetType() == nsGkAtoms::brFrame) {
              continue;
            }
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
      if (endOfLine && targetFrame.frame->HasSignificantTerminalNewline()) {
        
        
        --aPos->mContentOffset;
      }
      aPos->mResultFrame = targetFrame.frame;
      aPos->mAttach = aPos->mContentOffset == range.start ?
          CARET_ASSOCIATE_AFTER : CARET_ASSOCIATE_BEFORE;
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

nsIFrame::FrameSearchResult
nsFrame::PeekOffsetNoAmount(bool aForward, int32_t* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return FOUND;
}

nsIFrame::FrameSearchResult
nsFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                             bool aRespectClusters)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  int32_t startOffset = *aOffset;
  
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    *aOffset = 1 - startOffset;
    return FOUND;
  }
  return CONTINUE;
}

nsIFrame::FrameSearchResult
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
          return FOUND;
      } else {
        
        if (aWordSelectEatSpace && aState->mSawBeforeType)
          return FOUND;
      }
    }
    
    *aOffset = 1 - startOffset;
    aState->Update(false, 
                   false     
                   );
    if (!aWordSelectEatSpace)
      aState->SetSawBeforeType();
  }
  return CONTINUE;
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
    
    
    return aWhitespaceAfter;
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

nsresult
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
        
        thisBlock = thisBlock->FirstInFlow();
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
                                nsIFrame** aOutFrame, int32_t* aOutOffset,
                                bool* aOutJumpedLine, bool* aOutMovedOverNonSelectableText)
{
  nsresult result;

  if (!aOutFrame || !aOutOffset || !aOutJumpedLine)
    return NS_ERROR_NULL_POINTER;
  
  nsPresContext* presContext = PresContext();
  *aOutFrame = nullptr;
  *aOutOffset = 0;
  *aOutJumpedLine = false;
  *aOutMovedOverNonSelectableText = false;

  
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
    if (aVisual && presContext->BidiEnabled()) {
      bool lineIsRTL = it->GetDirection();
      bool isReordered;
      result = it->CheckLineOrder(thisLine, &isReordered, &firstFrame, &lastFrame);
      nsIFrame** framePtr = aDirection == eDirPrevious ? &firstFrame : &lastFrame;
      if (*framePtr) {
        bool frameIsRTL =
          (nsBidiPresUtils::FrameDirection(*framePtr) == NSBIDI_RTL);
        if ((frameIsRTL == lineIsRTL) == (aDirection == eDirPrevious)) {
          nsFrame::GetFirstLeaf(presContext, framePtr);
        } else {
          nsFrame::GetLastLeaf(presContext, framePtr);
        }
        atLineEdge = *framePtr == traversedFrame;
      } else {
        atLineEdge = true;
      }
    } else {
      nsRect  nonUsedRect;
      int32_t lineFrameCount;
      result = it->GetLine(thisLine, &firstFrame, &lineFrameCount,
                           nonUsedRect);
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

    
    if (!traversedFrame ||
        (!traversedFrame->IsGeneratedContentFrame() &&
         traversedFrame->GetContent()->IsRootOfNativeAnonymousSubtree())) {
      return NS_ERROR_FAILURE;
    }

    
    if (atLineEdge && aDirection == eDirPrevious &&
        traversedFrame->GetType() == nsGkAtoms::brFrame) {
      int32_t lineFrameCount;
      nsIFrame *currentBlockFrame, *currentFirstFrame;
      nsRect usedRect;
      int32_t currentLine = nsFrame::GetLineNumber(traversedFrame, aScrollViewStop, &currentBlockFrame);
      nsAutoLineIterator iter = currentBlockFrame->GetLineIterator();
      result = iter->GetLine(currentLine, &currentFirstFrame, &lineFrameCount, usedRect);
      if (NS_FAILED(result)) {
        return result;
      }
      if (lineFrameCount > 1) {
        continue;
      }
    }

    traversedFrame->IsSelectable(&selectable, nullptr);
    if (!selectable) {
      *aOutMovedOverNonSelectableText = true;
    }
  } 

  *aOutOffset = (aDirection == eDirNext) ? 0 : -1;

  if (aVisual && REVERSED_DIRECTION_FRAME(traversedFrame)) {
    
    *aOutOffset = -1 - *aOutOffset;
  }
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
  if (IsTableCaption() && !GetRect().IsEmpty()) {
    return a11y::eHTMLCaptionType;
  }
  return a11y::eNoType;
}
#endif

NS_DECLARE_FRAME_PROPERTY(OverflowAreasProperty, DeleteValue<nsOverflowAreas>)

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






static nsRect
UnionBorderBoxes(nsIFrame* aFrame, bool aApplyTransform,
                 const nsSize* aSizeOverride = nullptr,
                 const nsOverflowAreas* aOverflowOverride = nullptr)
{
  const nsRect bounds(nsPoint(0, 0),
                      aSizeOverride ? *aSizeOverride : aFrame->GetSize());

  
  
  nsRect u;
  bool doTransform = aApplyTransform && aFrame->IsTransformed();
  if (doTransform) {
    u = nsDisplayTransform::TransformRect(bounds, aFrame,
                                          nsPoint(0, 0), &bounds);
  } else {
    u = bounds;
  }

  
  
  
  if (aOverflowOverride) {
    if (!doTransform &&
        bounds.IsEqualEdges(aOverflowOverride->VisualOverflow()) &&
        bounds.IsEqualEdges(aOverflowOverride->ScrollableOverflow())) {
      return u;
    }
  } else {
    if (!doTransform &&
        bounds.IsEqualEdges(aFrame->GetVisualOverflowRect()) &&
        bounds.IsEqualEdges(aFrame->GetScrollableOverflowRect())) {
      return u;
    }
  }
  const nsStyleDisplay* disp = aFrame->StyleDisplay();
  nsIAtom* fType = aFrame->GetType();
  if (nsFrame::ShouldApplyOverflowClipping(aFrame, disp) ||
      fType == nsGkAtoms::scrollFrame ||
      fType == nsGkAtoms::svgOuterSVGFrame) {
    return u;
  }

  nsRect clipPropClipRect;
  bool hasClipPropClip =
    aFrame->GetClipPropClipRect(disp, &clipPropClipRect, bounds.Size());

  
  const nsIFrame::ChildListIDs skip(nsIFrame::kPopupList |
                                    nsIFrame::kSelectPopupList);
  for (nsIFrame::ChildListIterator childLists(aFrame);
       !childLists.IsDone(); childLists.Next()) {
    if (skip.Contains(childLists.CurrentID())) {
      continue;
    }

    nsFrameList children = childLists.CurrentList();
    for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
      nsIFrame* child = e.get();
      
      
      
      
      
      
      
      
      
      nsRect childRect = UnionBorderBoxes(child, true) +
                         child->GetPosition();

      if (hasClipPropClip) {
        
        childRect.IntersectRect(childRect, clipPropClipRect);
      }

      
      
      
      
      
      
      if (doTransform && !child->Preserves3D()) {
        childRect = nsDisplayTransform::TransformRect(childRect, aFrame,
                                                      nsPoint(0, 0), &bounds);
      }
      u.UnionRectEdges(u, childRect);
    }
  }

  return u;
}

static void
ComputeAndIncludeOutlineArea(nsIFrame* aFrame, nsOverflowAreas& aOverflowAreas,
                             const nsSize& aNewSize)
{
  const nsStyleOutline* outline = aFrame->StyleOutline();
  const uint8_t outlineStyle = outline->GetOutlineStyle();
  if (outlineStyle == NS_STYLE_BORDER_STYLE_NONE) {
    return;
  }

  nscoord width;
  DebugOnly<bool> result = outline->GetOutlineWidth(width);
  NS_ASSERTION(result, "GetOutlineWidth had no cached outline width");
  if (width <= 0 && outlineStyle != NS_STYLE_BORDER_STYLE_AUTO) {
    return;
  }

  
  
  
  
  
  
  nsIFrame *frameForArea = aFrame;
  do {
    nsIAtom *pseudoType = frameForArea->StyleContext()->GetPseudo();
    if (pseudoType != nsCSSAnonBoxes::mozAnonymousBlock &&
        pseudoType != nsCSSAnonBoxes::mozAnonymousPositionedBlock)
      break;
    
    frameForArea = frameForArea->GetFirstPrincipalChild();
    NS_ASSERTION(frameForArea, "anonymous block with no children?");
  } while (frameForArea);

  
  
  
  
  
  
  
  nsRect innerRect;
  if (frameForArea == aFrame) {
    innerRect = UnionBorderBoxes(aFrame, false, &aNewSize, &aOverflowAreas);
  } else {
    for (; frameForArea; frameForArea = frameForArea->GetNextSibling()) {
      nsRect r(UnionBorderBoxes(frameForArea, true));

      
      
      
      for (nsIFrame *f = frameForArea, *parent = f->GetParent();
           ;
           f = parent, parent = f->GetParent()) {
        r += f->GetPosition();
        if (parent == aFrame) {
          break;
        }
        if (parent->IsTransformed() && !f->Preserves3D()) {
          r = nsDisplayTransform::TransformRect(r, parent, nsPoint(0, 0));
        }
      }

      innerRect.UnionRect(innerRect, r);
    }
  }

  
  aFrame->Properties().Set(nsIFrame::OutlineInnerRectProperty(),
                           new nsRect(innerRect));
  const nscoord offset = outline->mOutlineOffset;
  nsRect outerRect(innerRect);
  bool useOutlineAuto = false;
  if (nsLayoutUtils::IsOutlineStyleAutoEnabled()) {
    useOutlineAuto = outlineStyle == NS_STYLE_BORDER_STYLE_AUTO;
    if (MOZ_UNLIKELY(useOutlineAuto)) {
      nsPresContext* presContext = aFrame->PresContext();
      nsITheme* theme = presContext->GetTheme();
      if (theme && theme->ThemeSupportsWidget(presContext, aFrame,
                                              NS_THEME_FOCUS_OUTLINE)) {
        outerRect.Inflate(offset);
        theme->GetWidgetOverflow(presContext->DeviceContext(), aFrame,
                                 NS_THEME_FOCUS_OUTLINE, &outerRect);
      } else {
        useOutlineAuto = false;
      }
    }
  }
  if (MOZ_LIKELY(!useOutlineAuto)) {
    outerRect.Inflate(width + offset);
  }

  nsRect& vo = aOverflowAreas.VisualOverflow();
  vo.UnionRectEdges(vo, innerRect.Union(outerRect));
}

bool
nsIFrame::FinishAndStoreOverflow(nsOverflowAreas& aOverflowAreas,
                                 nsSize aNewSize, nsSize* aOldSize)
{
  NS_ASSERTION(FrameMaintainsOverflow(this),
               "Don't call - overflow rects not maintained on these SVG frames");

  nsRect bounds(nsPoint(0, 0), aNewSize);
  
  
  if (Preserves3D() || HasPerspective() || IsTransformed()) {
    if (!aOverflowAreas.VisualOverflow().IsEqualEdges(bounds) ||
        !aOverflowAreas.ScrollableOverflow().IsEqualEdges(bounds)) {
      nsOverflowAreas* initial =
        static_cast<nsOverflowAreas*>(Properties().Get(nsIFrame::InitialOverflowProperty()));
      if (!initial) {
        Properties().Set(nsIFrame::InitialOverflowProperty(),
                         new nsOverflowAreas(aOverflowAreas));
      } else if (initial != &aOverflowAreas) {
        *initial = aOverflowAreas;
      }
    } else {
      Properties().Delete(nsIFrame::InitialOverflowProperty());
    }
#ifdef DEBUG
    Properties().Set(nsIFrame::DebugInitialOverflowPropertyApplied(), nullptr);
#endif
  } else {
#ifdef DEBUG
    Properties().Delete(nsIFrame::DebugInitialOverflowPropertyApplied());
#endif
  }

  
  
  
  
  
  
  
  
  NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
    DebugOnly<nsRect*> r = &aOverflowAreas.Overflow(otype);
    NS_ASSERTION(aNewSize.width == 0 || aNewSize.height == 0 ||
                 r->width == nscoord_MAX || r->height == nscoord_MAX ||
                 (mState & NS_FRAME_SVG_LAYOUT) ||
                 r->Contains(nsRect(nsPoint(0,0), aNewSize)),
                 "Computed overflow area must contain frame bounds");
  }

  
  
  
  
  const nsStyleDisplay* disp = StyleDisplay();
  NS_ASSERTION((disp->mOverflowY == NS_STYLE_OVERFLOW_CLIP) ==
               (disp->mOverflowX == NS_STYLE_OVERFLOW_CLIP),
               "If one overflow is clip, the other should be too");
  if (nsFrame::ShouldApplyOverflowClipping(this, disp)) {
    
    aOverflowAreas.SetAllTo(bounds);
  }

  
  
  
  
  
  
  if ((aNewSize.width != 0 || !IsInlineFrame(this)) &&
      !(GetStateBits() & NS_FRAME_SVG_LAYOUT)) {
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = aOverflowAreas.Overflow(otype);
      o.UnionRectEdges(o, bounds);
    }
  }

  
  
  if (!::IsBoxWrapped(this) && IsThemed(disp)) {
    nsRect r(bounds);
    nsPresContext *presContext = PresContext();
    if (presContext->GetTheme()->
          GetWidgetOverflow(presContext->DeviceContext(), this,
                            disp->mAppearance, &r)) {
      nsRect& vo = aOverflowAreas.VisualOverflow();
      vo.UnionRectEdges(vo, r);
    }
  }

  ComputeAndIncludeOutlineArea(this, aOverflowAreas, aNewSize);

  
  aOverflowAreas.VisualOverflow() =
    ComputeEffectsRect(this, aOverflowAreas.VisualOverflow(), aNewSize);

  
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
      RecomputePerspectiveChildrenOverflow(this->StyleContext(), &newBounds);
    }
  } else {
    Properties().Delete(nsIFrame::PreTransformOverflowAreasProperty());
    if (ChildrenHavePerspective() && sizeChanged) {
      nsRect newBounds(nsPoint(0, 0), aNewSize);
      RecomputePerspectiveChildrenOverflow(this->StyleContext(), &newBounds);
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
      if (!FrameMaintainsOverflow(child)) {
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
      } else if (child->StyleContext()->GetParent() == aStartStyle ||
                 child->StyleContext() == aStartStyle) {
        
        
        
        
        
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
      if (!FrameMaintainsOverflow(child)) {
        continue; 
      }
      if (child->Preserves3DChildren()) {
        RecomputePreserve3DChildrenOverflow(child, nullptr);
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

uint32_t
nsIFrame::GetDepthInFrameTree() const
{
  uint32_t result = 0;
  for (nsContainerFrame* ancestor = GetParent(); ancestor;
       ancestor = ancestor->GetParent()) {
    result++;
  }
  return result;
}

void
nsFrame::ConsiderChildOverflow(nsOverflowAreas& aOverflowAreas,
                               nsIFrame* aChildFrame)
{
  aOverflowAreas.UnionWith(aChildFrame->GetOverflowAreas() +
                           aChildFrame->GetPosition());
}










static nsIFrame*
GetIBSplitSiblingForAnonymousBlock(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Must have a non-null frame!");
  NS_ASSERTION(aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT,
               "GetIBSplitSibling should only be called on ib-split frames");

  nsIAtom* type = aFrame->StyleContext()->GetPseudo();
  if (type != nsCSSAnonBoxes::mozAnonymousBlock &&
      type != nsCSSAnonBoxes::mozAnonymousPositionedBlock) {
    
    return nullptr;
  }

  
  
  aFrame = aFrame->FirstContinuation();

  



  nsIFrame *ibSplitSibling = static_cast<nsIFrame*>
    (aFrame->Properties().Get(nsIFrame::IBSplitPrevSibling()));
  NS_ASSERTION(ibSplitSibling, "Broken frame tree?");
  return ibSplitSibling;
}











static nsIFrame*
GetCorrectedParent(const nsIFrame* aFrame)
{
  nsIFrame* parent = aFrame->GetParent();
  if (!parent) {
    return nullptr;
  }

  
  
  if (aFrame->IsTableCaption()) {
    nsIFrame* innerTable = parent->GetFirstPrincipalChild();
    if (!innerTable->StyleContext()->GetPseudo()) {
      return innerTable;
    }
  }

  
  
  
  nsIAtom* pseudo = aFrame->StyleContext()->GetPseudo();
  if (pseudo == nsCSSAnonBoxes::tableOuter) {
    pseudo = aFrame->GetFirstPrincipalChild()->StyleContext()->GetPseudo();
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
                 "Should have dealt with kids that have "
                 "NS_FRAME_PART_OF_IBSPLIT elsewhere");
    return aProspectiveParent;
  }

  
  
  
  nsIFrame* parent = aProspectiveParent;
  do {
    if (parent->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) {
      nsIFrame* sibling = GetIBSplitSiblingForAnonymousBlock(parent);

      if (sibling) {
        
        
        parent = sibling;
      }
    }
      
    nsIAtom* parentPseudo = parent->StyleContext()->GetPseudo();
    if (!parentPseudo ||
        (!nsCSSAnonBoxes::IsAnonBox(parentPseudo) &&
         
         
         
         
         aChildPseudo != nsGkAtoms::placeholderFrame)) {
      return parent;
    }

    parent = parent->GetParent();
  } while (parent);

  if (aProspectiveParent->StyleContext()->GetPseudo() ==
      nsCSSAnonBoxes::viewportScroll) {
    
    
    return aProspectiveParent;
  }

  
  
  
  NS_ASSERTION(aProspectiveParent->GetType() == nsGkAtoms::canvasFrame,
               "Should have found a parent before this");
  return nullptr;
}

nsStyleContext*
nsFrame::DoGetParentStyleContext(nsIFrame** aProviderFrame) const
{
  *aProviderFrame = nullptr;
  nsFrameManager* fm = PresContext()->FrameManager();
  if (MOZ_LIKELY(mContent)) {
    nsIContent* parentContent = mContent->GetFlattenedTreeParent();
    if (MOZ_LIKELY(parentContent)) {
      nsIAtom* pseudo = StyleContext()->GetPseudo();
      if (!pseudo || !mContent->IsElement() ||
          !nsCSSAnonBoxes::IsAnonBox(pseudo) ||
          

          pseudo == nsCSSAnonBoxes::tableOuter) {
        nsStyleContext* sc = fm->GetDisplayContentsStyleFor(parentContent);
        if (MOZ_UNLIKELY(sc)) {
          return sc;
        }
      }
    } else {
      if (!StyleContext()->GetPseudo()) {
        
        return nullptr;
      }
    }
  }

  if (!(mState & NS_FRAME_OUT_OF_FLOW)) {
    




    if (mState & NS_FRAME_PART_OF_IBSPLIT) {
      nsIFrame* ibSplitSibling = GetIBSplitSiblingForAnonymousBlock(this);
      if (ibSplitSibling) {
        return (*aProviderFrame = ibSplitSibling)->StyleContext();
      }
    }

    
    
    
    *aProviderFrame = GetCorrectedParent(this);
    return *aProviderFrame ? (*aProviderFrame)->StyleContext() : nullptr;
  }

  
  
  
  nsIFrame* placeholder = fm->GetPlaceholderFrameFor(FirstInFlow());
  if (!placeholder) {
    NS_NOTREACHED("no placeholder frame for out-of-flow frame");
    *aProviderFrame = GetCorrectedParent(this);
    return *aProviderFrame ? (*aProviderFrame)->StyleContext() : nullptr;
  }
  return placeholder->GetParentStyleContext(aProviderFrame);
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

 bool
nsIFrame::IsFocusable(int32_t *aTabIndex, bool aWithMouse)
{
  int32_t tabIndex = -1;
  if (aTabIndex) {
    *aTabIndex = -1; 
  }
  bool isFocusable = false;

  if (mContent && mContent->IsElement() && IsVisibleConsideringAncestors()) {
    const nsStyleUserInterface* ui = StyleUserInterface();
    if (ui->mUserFocus != NS_STYLE_USER_FOCUS_IGNORE &&
        ui->mUserFocus != NS_STYLE_USER_FOCUS_NONE) {
      
      tabIndex = 0;
    }
    isFocusable = mContent->IsFocusable(&tabIndex, aWithMouse);
    if (!isFocusable && !aWithMouse &&
        GetType() == nsGkAtoms::scrollFrame &&
        mContent->IsHTMLElement() &&
        !mContent->IsRootOfNativeAnonymousSubtree() &&
        mContent->GetParent() &&
        !mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
      
      
      
      
      
      
      
      nsIScrollableFrame *scrollFrame = do_QueryFrame(this);
      if (scrollFrame &&
          !scrollFrame->GetScrollbarStyles().IsHiddenInBothDirections() &&
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
nsIFrame::HasSignificantTerminalNewline() const
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
  if (IsSVGText()) {
    uint8_t dominantBaseline;
    for (const nsIFrame* frame = this; frame; frame = frame->GetParent()) {
      dominantBaseline = frame->StyleSVGReset()->mDominantBaseline;
      if (dominantBaseline != NS_STYLE_DOMINANT_BASELINE_AUTO ||
          frame->GetType() == nsGkAtoms::svgTextFrame) {
        break;
      }
    }
    return ConvertSVGDominantBaselineToVerticalAlign(dominantBaseline);
  }

  const nsStyleCoord& verticalAlign = StyleTextReset()->mVerticalAlign;
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
        GetPrefISize(rendContext) + bp.LeftRight();
      metrics->mBlockMinSize.width =
        GetMinISize(rendContext) + bp.LeftRight();
    }

    
    const WritingMode wm = aState.OuterReflowState() ?
      aState.OuterReflowState()->GetWritingMode() : GetWritingMode();
    nsHTMLReflowMetrics desiredSize(wm);
    BoxReflow(aState, presContext, desiredSize, rendContext,
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

      do {
         lines->GetLine(count, &firstFrame, &framesOnLine, lineBounds);

         if (lineBounds.height > metrics->mBlockMinSize.height)
           metrics->mBlockMinSize.height = lineBounds.height;

         count++;
      } while(firstFrame);
    } else {
      metrics->mBlockMinSize.height = desiredSize.Height();
    }

    metrics->mBlockPrefSize.height = metrics->mBlockMinSize.height;

    if (desiredSize.BlockStartAscent() ==
        nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
      if (!nsLayoutUtils::GetFirstLineBaseline(wm, this,
                                               &metrics->mBlockAscent))
        metrics->mBlockAscent = GetLogicalBaseline(wm);
    } else {
      metrics->mBlockAscent = desiredSize.BlockStartAscent();
    }

#ifdef DEBUG_adaptor
    printf("min=(%d,%d), pref=(%d,%d), ascent=%d\n", metrics->mBlockMinSize.width,
                                                     metrics->mBlockMinSize.height,
                                                     metrics->mBlockPrefSize.width,
                                                     metrics->mBlockPrefSize.height,
                                                     metrics->mBlockAscent);
#endif
  }

  return NS_OK;
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
  WritingMode ourWM = GetWritingMode();
  const WritingMode outerWM = aState.OuterReflowState() ?
    aState.OuterReflowState()->GetWritingMode() : ourWM;
  nsHTMLReflowMetrics desiredSize(outerWM);
  LogicalSize ourSize = GetLogicalSize(outerWM);

  if (rendContext) {

    BoxReflow(aState, presContext, desiredSize, rendContext,
              ourRect.x, ourRect.y, ourRect.width, ourRect.height);

    if (IsCollapsed()) {
      SetSize(nsSize(0, 0));
    } else {

      
      
      
      if (desiredSize.ISize(outerWM) > ourSize.ISize(outerWM) ||
          desiredSize.BSize(outerWM) > ourSize.BSize(outerWM)) {

#ifdef DEBUG_GROW
        DumpBox(stdout);
        printf(" GREW from (%d,%d) -> (%d,%d)\n",
               ourSize.ISize(outerWM), ourSize.BSize(outerWM),
               desiredSize.ISize(outerWM), desiredSize.BSize(outerWM));
#endif

        if (desiredSize.ISize(outerWM) > ourSize.ISize(outerWM)) {
          ourSize.ISize(outerWM) = desiredSize.ISize(outerWM);
        }

        if (desiredSize.BSize(outerWM) > ourSize.BSize(outerWM)) {
          ourSize.BSize(outerWM) = desiredSize.BSize(outerWM);
        }
      }

      
      
      SetSize(ourSize.ConvertTo(ourWM, outerWM));
    }
  }

  
  LogicalSize size(GetLogicalSize(outerWM));
  desiredSize.ISize(outerWM) = size.ISize(outerWM);
  desiredSize.BSize(outerWM) = size.BSize(outerWM);
  desiredSize.UnionOverflowAreasWithDesiredBounds();

  if (HasAbsolutelyPositionedChildren()) {
    
    nsHTMLReflowState reflowState(aState.PresContext(), this,
                                  aState.GetRenderingContext(),
                                  LogicalSize(ourWM, ISize(),
                                              NS_UNCONSTRAINEDSIZE),
                                  nsHTMLReflowState::DUMMY_PARENT_REFLOW_STATE);

    AddStateBits(NS_FRAME_IN_REFLOW);
    
    
    nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;
    ReflowAbsoluteFrames(aState.PresContext(), desiredSize,
                         reflowState, reflowStatus);
    RemoveStateBits(NS_FRAME_IN_REFLOW);
  }

  nsSize oldSize(ourRect.Size());
  FinishAndStoreOverflow(desiredSize.mOverflowAreas,
                         size.GetPhysicalSize(outerWM), &oldSize);

  SyncLayout(aState);

  return NS_OK;
}

void
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
  WritingMode wm = aDesiredSize.GetWritingMode();

  bool needsReflow = NS_SUBTREE_DIRTY(this);

  
  
  if (!needsReflow) {
      
      if (aWidth != NS_INTRINSICSIZE && aHeight != NS_INTRINSICSIZE) {
      
          
          if ((metrics->mLastSize.width == 0 || metrics->mLastSize.height == 0) && (aWidth == 0 || aHeight == 0)) {
               needsReflow = false;
               aDesiredSize.Width() = aWidth; 
               aDesiredSize.Height() = aHeight; 
               SetSize(aDesiredSize.Size(wm).ConvertTo(GetWritingMode(), wm));
          } else {
            aDesiredSize.Width() = metrics->mLastSize.width;
            aDesiredSize.Height() = metrics->mLastSize.height;

            
            
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

    aDesiredSize.ClearSize();

    

    
    
    nsMargin margin(0,0,0,0);
    GetMargin(margin);

    nsSize parentSize(aWidth, aHeight);
    if (parentSize.height != NS_INTRINSICSIZE)
      parentSize.height += margin.TopBottom();
    if (parentSize.width != NS_INTRINSICSIZE)
      parentSize.width += margin.LeftRight();

    nsIFrame *parentFrame = GetParent();
    nsFrameState savedState = parentFrame->GetStateBits();
    WritingMode parentWM = parentFrame->GetWritingMode();
    nsHTMLReflowState
      parentReflowState(aPresContext, parentFrame, aRenderingContext,
                        LogicalSize(parentWM, parentSize),
                        nsHTMLReflowState::DUMMY_PARENT_REFLOW_STATE);
    parentFrame->RemoveStateBits(~nsFrameState(0));
    parentFrame->AddStateBits(savedState);

    
    if (parentSize.width != NS_INTRINSICSIZE)
      parentReflowState.SetComputedWidth(std::max(parentSize.width, 0));
    if (parentSize.height != NS_INTRINSICSIZE)
      parentReflowState.SetComputedHeight(std::max(parentSize.height, 0));
    parentReflowState.ComputedPhysicalMargin().SizeTo(0, 0, 0, 0);
    
    parentFrame->GetPadding(parentReflowState.ComputedPhysicalPadding());
    parentFrame->GetBorder(parentReflowState.ComputedPhysicalBorderPadding());
    parentReflowState.ComputedPhysicalBorderPadding() +=
      parentReflowState.ComputedPhysicalPadding();

    
    
    const nsHTMLReflowState *outerReflowState = aState.OuterReflowState();
    NS_ASSERTION(!outerReflowState || outerReflowState->frame != this,
                 "in and out of XUL on a single frame?");
    const nsHTMLReflowState* parentRS;
    if (outerReflowState && outerReflowState->frame == parentFrame) {
      
      
      
      
      
      parentRS = outerReflowState;
    } else {
      parentRS = &parentReflowState;
    }

    
    
    WritingMode wm = GetWritingMode();
    LogicalSize logicalSize(wm, nsSize(aWidth, aHeight));
    logicalSize.BSize(wm) = NS_INTRINSICSIZE;
    nsHTMLReflowState reflowState(aPresContext, *parentRS, this,
                                  logicalSize, -1, -1,
                                  nsHTMLReflowState::DUMMY_PARENT_REFLOW_STATE);

    
    
    
    reflowState.mCBReflowState = parentRS;

    reflowState.mReflowDepth = aState.GetReflowDepth();

    
    
    if (aWidth != NS_INTRINSICSIZE) {
      nscoord computedWidth =
        aWidth - reflowState.ComputedPhysicalBorderPadding().LeftRight();
      computedWidth = std::max(computedWidth, 0);
      reflowState.SetComputedWidth(computedWidth);
    }

    
    
    
    
    
    if (!IsFrameOfType(eBlockFrame)) {
      if (aHeight != NS_INTRINSICSIZE) {
        nscoord computedHeight =
          aHeight - reflowState.ComputedPhysicalBorderPadding().TopBottom();
        computedHeight = std::max(computedHeight, 0);
        reflowState.SetComputedHeight(computedHeight);
      } else {
        reflowState.SetComputedHeight(
          ComputeSize(aRenderingContext, wm,
                      logicalSize,
                      logicalSize.ISize(wm),
                      reflowState.ComputedLogicalMargin().Size(wm),
                      reflowState.ComputedLogicalBorderPadding().Size(wm) -
                        reflowState.ComputedLogicalPadding().Size(wm),
                      reflowState.ComputedLogicalPadding().Size(wm),
                      ComputeSizeFlags::eDefault).Height(wm));
      }
    }

    
    
    
    
    
    
    if (metrics->mLastSize.width != aWidth) {
      reflowState.SetHResize(true);

      
      
      
      if (nsLayoutUtils::FontSizeInflationEnabled(aPresContext)) {
        AddStateBits(NS_FRAME_IS_DIRTY);
      }
    }
    if (metrics->mLastSize.height != aHeight)
      reflowState.SetVResize(true);

    #ifdef DEBUG_REFLOW
      nsAdaptorAddIndents();
      printf("Size=(%d,%d)\n",reflowState.ComputedWidth(),
             reflowState.ComputedHeight());
      nsAdaptorAddIndents();
      nsAdaptorPrintReason(reflowState);
      printf("\n");
    #endif

       

    Reflow(aPresContext, aDesiredSize, reflowState, status);

    NS_ASSERTION(NS_FRAME_IS_COMPLETE(status), "bad status");

    uint32_t layoutFlags = aState.LayoutFlags();
    nsContainerFrame::FinishReflowChild(this, aPresContext, aDesiredSize,
                                        &reflowState, aX, aY, layoutFlags | NS_FRAME_NO_MOVE_FRAME);

    
    if (IsCollapsed()) {
      metrics->mAscent = 0;
    } else {
      if (aDesiredSize.BlockStartAscent() ==
          nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
        if (!nsLayoutUtils::GetFirstLineBaseline(wm, this, &metrics->mAscent))
          metrics->mAscent = GetLogicalBaseline(wm);
      } else
        metrics->mAscent = aDesiredSize.BlockStartAscent();
    }

  } else {
    aDesiredSize.SetBlockStartAscent(metrics->mBlockAscent);
  }

#ifdef DEBUG_REFLOW
  if (aHeight != NS_INTRINSICSIZE && aDesiredSize.Height() != aHeight)
  {
          nsAdaptorAddIndents();
          printf("*****got taller!*****\n");
         
  }
  if (aWidth != NS_INTRINSICSIZE && aDesiredSize.Width() != aWidth)
  {
          nsAdaptorAddIndents();
          printf("*****got wider!******\n");
         
  }
#endif

  if (aWidth == NS_INTRINSICSIZE)
     aWidth = aDesiredSize.Width();

  if (aHeight == NS_INTRINSICSIZE)
     aHeight = aDesiredSize.Height();

  metrics->mLastSize.width = aDesiredSize.Width();
  metrics->mLastSize.height = aDesiredSize.Height();

#ifdef DEBUG_REFLOW
  gIndent2--;
#endif
}

nsBoxLayoutMetrics*
nsFrame::BoxMetrics() const
{
  nsBoxLayoutMetrics* metrics =
    static_cast<nsBoxLayoutMetrics*>(Properties().Get(BoxMetricsProperty()));
  NS_ASSERTION(metrics, "A box layout method was called but InitBoxMetrics was never called");
  return metrics;
}

 void
nsIFrame::AddInPopupStateBitToDescendants(nsIFrame* aFrame)
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

 void
nsIFrame::RemoveInPopupStateBitFromDescendants(nsIFrame* aFrame)
{
  if (!aFrame->HasAnyStateBits(NS_FRAME_IN_POPUP) ||
      nsLayoutUtils::IsPopup(aFrame)) {
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
nsIFrame::SetParent(nsContainerFrame* aParent)
{
  
  mParent = aParent;
  if (::IsBoxWrapped(this)) {
    ::InitBoxMetrics(this, true);
  } else {
    
    
    
    
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
nsIFrame::CreateOwnLayerIfNeeded(nsDisplayListBuilder* aBuilder, 
                                 nsDisplayList* aList)
{
  if (GetContent() &&
      GetContent()->IsXULElement() &&
      GetContent()->HasAttr(kNameSpaceID_None, nsGkAtoms::layer)) {
    aList->AppendNewToTop(new (aBuilder) 
        nsDisplayOwnLayer(aBuilder, this, aList));
  }
}

bool
nsIFrame::IsSelected() const
{
  return (GetContent() && GetContent()->IsSelectionDescendant()) ?
    IsFrameSelected() : false;
}

 void
nsIFrame::DestroyContentArray(void* aPropertyValue)
{
  typedef nsTArray<nsIContent*> T;
  T* arr = static_cast<T*>(aPropertyValue);
  for (T::size_type i = 0; i < arr->Length(); ++i) {
    nsIContent* content = (*arr)[i];
    content->UnbindFromTree();
    NS_RELEASE(content);
  }
  delete arr;
}

bool
nsIFrame::IsPseudoStackingContextFromStyle() {
  const nsStyleDisplay* disp = StyleDisplay();
  
  
  return disp->mOpacity != 1.0f ||
         disp->IsAbsPosContainingBlock(this) ||
         disp->IsFloating(this) ||
         (disp->mWillChangeBitField & NS_STYLE_WILL_CHANGE_STACKING_CONTEXT);
}

Element*
nsIFrame::GetPseudoElement(nsCSSPseudoElements::Type aType)
{
  nsIFrame* frame = nullptr;

  if (aType == nsCSSPseudoElements::ePseudo_before) {
    frame = nsLayoutUtils::GetBeforeFrame(this);
  } else if (aType == nsCSSPseudoElements::ePseudo_after) {
    frame = nsLayoutUtils::GetAfterFrame(this);
  }

  if (frame) {
    nsIContent* content = frame->GetContent();
    if (content->IsElement()) {
      return content->AsElement();
    }
  }
  
  return nullptr;
}

nsIFrame::CaretPosition::CaretPosition()
  : mContentOffset(0)
{
}

nsIFrame::CaretPosition::~CaretPosition()
{
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
                nsAtomCString(aContent->NodeInfo()->NameAtom()).get(), aFrame);
  }
  else {
    PR_snprintf(aResult, aResultSize, "@%p", aFrame);
  }
}

void
nsFrame::Trace(const char* aMethod, bool aEnter)
{
  if (NS_FRAME_LOG_TEST(GetLogModuleInfo(), NS_FRAME_TRACE_CALLS)) {
    char tagbuf[40];
    GetTagName(this, mContent, sizeof(tagbuf), tagbuf);
    PR_LogPrint("%s: %s %s", tagbuf, aEnter ? "enter" : "exit", aMethod);
  }
}

void
nsFrame::Trace(const char* aMethod, bool aEnter, nsReflowStatus aStatus)
{
  if (NS_FRAME_LOG_TEST(GetLogModuleInfo(), NS_FRAME_TRACE_CALLS)) {
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
  if (NS_FRAME_LOG_TEST(GetLogModuleInfo(), NS_FRAME_TRACE_CALLS)) {
    
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
  mValue = nsFrame::DisplayIntrinsicISizeEnter(mFrame, mType);
}

DR_intrinsic_width_cookie::~DR_intrinsic_width_cookie()
{
  MOZ_COUNT_DTOR(DR_intrinsic_width_cookie);
  nsFrame::DisplayIntrinsicISizeExit(mFrame, mType, mResult, mValue);
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
                     nscoord                  aHorizontalPercentBasis,
                     nscoord                  aVerticalPercentBasis,
                     const nsMargin*          aMargin,
                     const nsMargin*          aPadding)
  : mFrame(aFrame)
  , mState(aState)
{
  MOZ_COUNT_CTOR(DR_init_offsets_cookie);
  mValue = nsCSSOffsetState::DisplayInitOffsetsEnter(mFrame, mState,
                                                     aHorizontalPercentBasis,
                                                     aVerticalPercentBasis,
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
  explicit DR_RulePart(nsIAtom* aFrameType) : mFrameType(aFrameType), mNext(0) {}
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
  DR_FrameTypeInfo& operator=(const DR_FrameTypeInfo&) = delete;
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
  AddFrameTypeInfo(nsGkAtoms::colorControlFrame,     "color",     "colorControl");
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

    DR_state->PrettyUC(aReflowState.AvailableWidth(), width);
    DR_state->PrettyUC(aReflowState.AvailableHeight(), height);
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

    if (aReflowState.IsHResize())
      printf("h-resize ");

    if (aReflowState.IsVResize())
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
      CheckPixelError(aReflowState.AvailableWidth(), p2t);
      CheckPixelError(aReflowState.AvailableHeight(), p2t);
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

void* nsFrame::DisplayIntrinsicISizeEnter(nsIFrame* aFrame,
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
    DR_state->PrettyUC(aMetrics.Width(), width);
    DR_state->PrettyUC(aMetrics.Height(), height);
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
      CheckPixelError(aMetrics.Width(), p2t);
      CheckPixelError(aMetrics.Height(), p2t);
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

void nsFrame::DisplayIntrinsicISizeExit(nsIFrame*            aFrame,
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

    DR_state->PrettyUC(aState->AvailableWidth(), width);
    DR_state->PrettyUC(aState->AvailableHeight(), height);
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
    DR_state->PrettyUC(aState->ComputedMinWidth(), cmiw);
    DR_state->PrettyUC(aState->ComputedWidth(), cw);
    DR_state->PrettyUC(aState->ComputedMaxWidth(), cmxw);
    DR_state->PrettyUC(aState->ComputedMinHeight(), cmih);
    DR_state->PrettyUC(aState->ComputedHeight(), ch);
    DR_state->PrettyUC(aState->ComputedMaxHeight(), cmxh);
    printf("InitConstraints= cw=(%s <= %s <= %s) ch=(%s <= %s <= %s)",
           cmiw, cw, cmxw, cmih, ch, cmxh);
    DR_state->PrintMargin("co", &aState->ComputedPhysicalOffsets());
    putchar('\n');
  }
  DR_state->DeleteTreeNode(*treeNode);
}


 void*
nsCSSOffsetState::DisplayInitOffsetsEnter(nsIFrame* aFrame,
                                          nsCSSOffsetState* aState,
                                          nscoord aHorizontalPercentBasis,
                                          nscoord aVerticalPercentBasis,
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

    char horizPctBasisStr[16];
    char vertPctBasisStr[16];
    DR_state->PrettyUC(aHorizontalPercentBasis, horizPctBasisStr);
    DR_state->PrettyUC(aVerticalPercentBasis,   vertPctBasisStr);
    printf("InitOffsets pct_basis=%s,%s", horizPctBasisStr, vertPctBasisStr);

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
    DR_state->PrintMargin("m", &aState->ComputedPhysicalMargin());
    DR_state->PrintMargin("p", &aState->ComputedPhysicalPadding());
    DR_state->PrintMargin("p+b", &aState->ComputedPhysicalBorderPadding());
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
