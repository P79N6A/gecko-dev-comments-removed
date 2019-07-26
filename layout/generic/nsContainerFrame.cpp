






#include "nsContainerFrame.h"

#include "nsAbsoluteContainingBlock.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsGUIEvent.h"
#include "nsStyleConsts.h"
#include "nsView.h"
#include "nsFrameManager.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsCSSAnonBoxes.h"
#include "nsViewManager.h"
#include "nsIWidget.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"
#include "nsCSSRendering.h"
#include "nsTransform2D.h"
#include "nsRegion.h"
#include "nsError.h"
#include "nsDisplayList.h"
#include "nsListControlFrame.h"
#include "nsIBaseWindow.h"
#include "nsThemeConstants.h"
#include "nsBoxLayoutState.h"
#include "nsRenderingContext.h"
#include "nsCSSFrameConstructor.h"
#include "mozilla/dom/Element.h"
#include <algorithm>

#ifdef DEBUG
#undef NOISY
#else
#undef NOISY
#endif

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_FRAMEARENA_HELPERS(nsContainerFrame)

nsContainerFrame::~nsContainerFrame()
{
}

NS_QUERYFRAME_HEAD(nsContainerFrame)
  NS_QUERYFRAME_ENTRY(nsContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSplittableFrame)

NS_IMETHODIMP
nsContainerFrame::Init(nsIContent* aContent,
                       nsIFrame*   aParent,
                       nsIFrame*   aPrevInFlow)
{
  nsresult rv = nsSplittableFrame::Init(aContent, aParent, aPrevInFlow);
  if (aPrevInFlow) {
    
    
    
    if (aPrevInFlow->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW)
      AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
  }
  return rv;
}

NS_IMETHODIMP
nsContainerFrame::SetInitialChildList(ChildListID  aListID,
                                      nsFrameList& aChildList)
{
  nsresult  result;
  if (mFrames.NotEmpty()) {
    
    
    NS_NOTREACHED("unexpected second call to SetInitialChildList");
    result = NS_ERROR_UNEXPECTED;
  } else if (aListID != kPrincipalList) {
    
    NS_NOTREACHED("unknown frame list");
    result = NS_ERROR_INVALID_ARG;
  } else {
#ifdef DEBUG
    nsFrame::VerifyDirtyBitSet(aChildList);
#endif
    mFrames.SetFrames(aChildList);
    result = NS_OK;
  }
  return result;
}

NS_IMETHODIMP
nsContainerFrame::AppendFrames(ChildListID  aListID,
                               nsFrameList& aFrameList)
{
  if (aListID != kPrincipalList) {
#ifdef IBMBIDI
    if (aListID != kNoReflowPrincipalList)
#endif
    {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }
  if (aFrameList.NotEmpty()) {
    mFrames.AppendFrames(this, aFrameList);

    
#ifdef IBMBIDI
    if (aListID == kPrincipalList)
#endif
    {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsContainerFrame::InsertFrames(ChildListID aListID,
                               nsIFrame* aPrevFrame,
                               nsFrameList& aFrameList)
{
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if (aListID != kPrincipalList) {
#ifdef IBMBIDI
    if (aListID != kNoReflowPrincipalList)
#endif
    {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }
  if (aFrameList.NotEmpty()) {
    
    mFrames.InsertFrames(this, aPrevFrame, aFrameList);

#ifdef IBMBIDI
    if (aListID == kPrincipalList)
#endif
    {
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsContainerFrame::RemoveFrame(ChildListID aListID,
                              nsIFrame* aOldFrame)
{
  if (aListID != kPrincipalList) {
#ifdef IBMBIDI
    if (kNoReflowPrincipalList != aListID)
#endif
    {
      NS_ERROR("unexpected child list");
      return NS_ERROR_INVALID_ARG;
    }
  }

  
  
  
  bool generateReflowCommand = true;
#ifdef IBMBIDI
  if (kNoReflowPrincipalList == aListID) {
    generateReflowCommand = false;
  }
#endif
  nsPresContext* pc = PresContext();
  nsContainerFrame* lastParent = nullptr;
  while (aOldFrame) {
    
    
    
    
    nsIFrame* oldFrameNextContinuation = aOldFrame->GetNextContinuation();
    nsContainerFrame* parent =
      static_cast<nsContainerFrame*>(aOldFrame->GetParent());
    parent->StealFrame(pc, aOldFrame, true);
    aOldFrame->Destroy();
    aOldFrame = oldFrameNextContinuation;
    if (parent != lastParent && generateReflowCommand) {
      pc->PresShell()->
        FrameNeedsReflow(parent, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
      lastParent = parent;
    }
  }
  return NS_OK;
}

void
nsContainerFrame::DestroyAbsoluteFrames(nsIFrame* aDestructRoot)
{
  if (IsAbsoluteContainer()) {
    GetAbsoluteContainingBlock()->DestroyFrames(this, aDestructRoot);
    MarkAsNotAbsoluteContainingBlock();
  }
}

void
nsContainerFrame::SafelyDestroyFrameListProp(nsIFrame* aDestructRoot,
                                             FramePropertyTable* aPropTable,
                                             const FramePropertyDescriptor* aProp)
{
  
  
  
  while (nsFrameList* frameList =
           static_cast<nsFrameList*>(aPropTable->Get(this, aProp))) {
    nsIFrame* frame = frameList->RemoveFirstChild();
    if (MOZ_LIKELY(frame)) {
      frame->DestroyFrom(aDestructRoot);
    } else {
      aPropTable->Remove(this, aProp);
      delete frameList;
      return;
    }
  }
}

void
nsContainerFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (HasView()) {
    GetView()->SetFrame(nullptr);
  }

  DestroyAbsoluteFrames(aDestructRoot);

  
  mFrames.DestroyFramesFrom(aDestructRoot);

  
  nsPresContext* prescontext = PresContext();

  DestroyOverflowList(prescontext, aDestructRoot);

  if (IsFrameOfType(nsIFrame::eCanContainOverflowContainers)) {
    FramePropertyTable* props = prescontext->PropertyTable();
    SafelyDestroyFrameListProp(aDestructRoot, props,
                               OverflowContainersProperty());
    SafelyDestroyFrameListProp(aDestructRoot, props,
                               ExcessOverflowContainersProperty());
  }

  
  nsSplittableFrame::DestroyFrom(aDestructRoot);
}




const nsFrameList&
nsContainerFrame::GetChildList(ChildListID aListID) const
{
  
  switch (aListID) {
    case kPrincipalList:
      return mFrames;
    case kOverflowList: {
      nsFrameList* list = GetOverflowFrames();
      return list ? *list : nsFrameList::EmptyList();
    }
    case kOverflowContainersList: {
      nsFrameList* list =
        GetPropTableFrames(PresContext(), OverflowContainersProperty());
      return list ? *list : nsFrameList::EmptyList();
    }
    case kExcessOverflowContainersList: {
      nsFrameList* list =
        GetPropTableFrames(PresContext(), ExcessOverflowContainersProperty());
      return list ? *list : nsFrameList::EmptyList();
    }
    default:
      return nsSplittableFrame::GetChildList(aListID);
  }
}

static void AppendIfNonempty(const nsIFrame* aFrame,
                            FramePropertyTable* aPropTable,
                            const FramePropertyDescriptor* aProperty,
                            nsTArray<nsIFrame::ChildList>* aLists,
                            nsIFrame::ChildListID aListID)
{
  nsFrameList* list = static_cast<nsFrameList*>(
    aPropTable->Get(aFrame, aProperty));
  if (list) {
    list->AppendIfNonempty(aLists, aListID);
  }
}

void
nsContainerFrame::GetChildLists(nsTArray<ChildList>* aLists) const
{
  mFrames.AppendIfNonempty(aLists, kPrincipalList);
  FramePropertyTable* propTable = PresContext()->PropertyTable();
  ::AppendIfNonempty(this, propTable, OverflowProperty(),
                     aLists, kOverflowList);
  if (IsFrameOfType(nsIFrame::eCanContainOverflowContainers)) {
    ::AppendIfNonempty(this, propTable, OverflowContainersProperty(),
                       aLists, kOverflowContainersList);
    ::AppendIfNonempty(this, propTable, ExcessOverflowContainersProperty(),
                       aLists, kExcessOverflowContainersList);
  }
  nsSplittableFrame::GetChildLists(aLists);
}




void
nsContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                   const nsRect&           aDirtyRect,
                                   const nsDisplayListSet& aLists)
{
  DisplayBorderBackgroundOutline(aBuilder, aLists);

  BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists);
}

void
nsContainerFrame::BuildDisplayListForNonBlockChildren(nsDisplayListBuilder*   aBuilder,
                                                      const nsRect&           aDirtyRect,
                                                      const nsDisplayListSet& aLists,
                                                      uint32_t                aFlags)
{
  nsIFrame* kid = mFrames.FirstChild();
  
  nsDisplayListSet set(aLists, aLists.Content());
  
  while (kid) {
    BuildDisplayListForChild(aBuilder, kid, aDirtyRect, set, aFlags);
    kid = kid->GetNextSibling();
  }
}

 void
nsContainerFrame::ChildIsDirty(nsIFrame* aChild)
{
  AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
}

bool
nsContainerFrame::IsLeaf() const
{
  return false;
}

bool
nsContainerFrame::PeekOffsetNoAmount(bool aForward, int32_t* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return false;
}

bool
nsContainerFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                      bool aRespectClusters)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return false;
}




static nsresult
ReparentFrameViewTo(nsIFrame*       aFrame,
                    nsViewManager* aViewManager,
                    nsView*        aNewParentView,
                    nsView*        aOldParentView)
{

  
  

  
  if (aFrame->HasView()) {
#ifdef MOZ_XUL
    if (aFrame->GetType() == nsGkAtoms::menuPopupFrame) {
      
      return NS_OK;
    }
#endif
    nsView* view = aFrame->GetView();
    
    
    

    aViewManager->RemoveChild(view);
    
    
    nsView* insertBefore = nsLayoutUtils::FindSiblingViewFor(aNewParentView, aFrame);
    aViewManager->InsertChild(aNewParentView, view, insertBefore, insertBefore != nullptr);
  } else {
    nsIFrame::ChildListIterator lists(aFrame);
    for (; !lists.IsDone(); lists.Next()) {
      
      
      nsFrameList::Enumerator childFrames(lists.CurrentList());
      for (; !childFrames.AtEnd(); childFrames.Next()) {
        ReparentFrameViewTo(childFrames.get(), aViewManager,
                            aNewParentView, aOldParentView);
      }
    }
  }

  return NS_OK;
}

nsresult
nsContainerFrame::CreateViewForFrame(nsIFrame* aFrame,
                                     bool aForce)
{
  if (aFrame->HasView()) {
    return NS_OK;
  }

  
  if (!aForce && !aFrame->NeedsView()) {
    
    return NS_OK;
  }

  nsView* parentView = aFrame->GetParent()->GetClosestView();
  NS_ASSERTION(parentView, "no parent with view");

  nsViewManager* viewManager = parentView->GetViewManager();
  NS_ASSERTION(viewManager, "null view manager");

  
  nsView* view = viewManager->CreateView(aFrame->GetRect(), parentView);
  if (!view)
    return NS_ERROR_OUT_OF_MEMORY;

  SyncFrameViewProperties(aFrame->PresContext(), aFrame, nullptr, view);

  nsView* insertBefore = nsLayoutUtils::FindSiblingViewFor(parentView, aFrame);
  
  
  
  viewManager->InsertChild(parentView, view, insertBefore, insertBefore != nullptr);

  
  
  
  
  
  
  
  
  ReparentFrameViewTo(aFrame, viewManager, view, parentView);

  
  aFrame->SetView(view);

  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
               ("nsContainerFrame::CreateViewForFrame: frame=%p view=%p",
                aFrame));
  return NS_OK;
}






void
nsContainerFrame::PositionFrameView(nsIFrame* aKidFrame)
{
  nsIFrame* parentFrame = aKidFrame->GetParent();
  if (!aKidFrame->HasView() || !parentFrame)
    return;

  nsView* view = aKidFrame->GetView();
  nsViewManager* vm = view->GetViewManager();
  nsPoint pt;
  nsView* ancestorView = parentFrame->GetClosestView(&pt);

  if (ancestorView != view->GetParent()) {
    NS_ASSERTION(ancestorView == view->GetParent()->GetParent(),
                 "Allowed only one anonymous view between frames");
    
    
    return;
  }

  pt += aKidFrame->GetPosition();
  vm->MoveViewTo(view, pt.x, pt.y);
}

nsresult
nsContainerFrame::ReparentFrameView(nsPresContext* aPresContext,
                                    nsIFrame*       aChildFrame,
                                    nsIFrame*       aOldParentFrame,
                                    nsIFrame*       aNewParentFrame)
{
  NS_PRECONDITION(aChildFrame, "null child frame pointer");
  NS_PRECONDITION(aOldParentFrame, "null old parent frame pointer");
  NS_PRECONDITION(aNewParentFrame, "null new parent frame pointer");
  NS_PRECONDITION(aOldParentFrame != aNewParentFrame, "same old and new parent frame");

  
  while (!aOldParentFrame->HasView() && !aNewParentFrame->HasView()) {
    
    
    
    
    
    
    
    aOldParentFrame = aOldParentFrame->GetParent();
    aNewParentFrame = aNewParentFrame->GetParent();
    
    
    
    NS_ASSERTION(aOldParentFrame && aNewParentFrame, "didn't find view");

    
    if (aOldParentFrame == aNewParentFrame) {
      break;
    }
  }

  
  if (aOldParentFrame == aNewParentFrame) {
    
    
    
    
    return NS_OK;
  }

  
  
  nsView* oldParentView = aOldParentFrame->GetClosestView();
  nsView* newParentView = aNewParentFrame->GetClosestView();
  
  
  
  
  if (oldParentView != newParentView) {
    
    return ReparentFrameViewTo(aChildFrame, oldParentView->GetViewManager(), newParentView,
                               oldParentView);
  }

  return NS_OK;
}

nsresult
nsContainerFrame::ReparentFrameViewList(nsPresContext*     aPresContext,
                                        const nsFrameList& aChildFrameList,
                                        nsIFrame*          aOldParentFrame,
                                        nsIFrame*          aNewParentFrame)
{
  NS_PRECONDITION(aChildFrameList.NotEmpty(), "empty child frame list");
  NS_PRECONDITION(aOldParentFrame, "null old parent frame pointer");
  NS_PRECONDITION(aNewParentFrame, "null new parent frame pointer");
  NS_PRECONDITION(aOldParentFrame != aNewParentFrame, "same old and new parent frame");

  
  while (!aOldParentFrame->HasView() && !aNewParentFrame->HasView()) {
    
    
    
    
    
    
    
    aOldParentFrame = aOldParentFrame->GetParent();
    aNewParentFrame = aNewParentFrame->GetParent();
    
    
    
    NS_ASSERTION(aOldParentFrame && aNewParentFrame, "didn't find view");

    
    if (aOldParentFrame == aNewParentFrame) {
      break;
    }
  }


  
  if (aOldParentFrame == aNewParentFrame) {
    
    
    
    
    return NS_OK;
  }

  
  
  nsView* oldParentView = aOldParentFrame->GetClosestView();
  nsView* newParentView = aNewParentFrame->GetClosestView();
  
  
  
  
  if (oldParentView != newParentView) {
    nsViewManager* viewManager = oldParentView->GetViewManager();

    
    for (nsFrameList::Enumerator e(aChildFrameList); !e.AtEnd(); e.Next()) {
      ReparentFrameViewTo(e.get(), viewManager, newParentView, oldParentView);
    }
  }

  return NS_OK;
}

static nsIWidget*
GetPresContextContainerWidget(nsPresContext* aPresContext)
{
  nsCOMPtr<nsISupports> container = aPresContext->Document()->GetContainer();
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(container);
  if (!baseWindow)
    return nullptr;

  nsCOMPtr<nsIWidget> mainWidget;
  baseWindow->GetMainWidget(getter_AddRefs(mainWidget));
  return mainWidget;
}

static bool
IsTopLevelWidget(nsIWidget* aWidget)
{
  nsWindowType windowType;
  aWidget->GetWindowType(windowType);
  return windowType == eWindowType_toplevel ||
         windowType == eWindowType_dialog ||
         windowType == eWindowType_sheet;
  
}

void
nsContainerFrame::SyncWindowProperties(nsPresContext*       aPresContext,
                                       nsIFrame*            aFrame,
                                       nsView*             aView,
                                       nsRenderingContext*  aRC)
{
#ifdef MOZ_XUL
  if (!aView || !nsCSSRendering::IsCanvasFrame(aFrame) || !aView->HasWidget())
    return;

  nsIWidget* windowWidget = GetPresContextContainerWidget(aPresContext);
  if (!windowWidget || !IsTopLevelWidget(windowWidget))
    return;

  nsViewManager* vm = aView->GetViewManager();
  nsView* rootView = vm->GetRootView();

  if (aView != rootView)
    return;

  Element* rootElement = aPresContext->Document()->GetRootElement();
  if (!rootElement || !rootElement->IsXUL()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return;
  }

  nsIFrame *rootFrame = aPresContext->PresShell()->FrameConstructor()->GetRootElementStyleFrame();
  if (!rootFrame)
    return;

  nsTransparencyMode mode = nsLayoutUtils::GetFrameTransparency(aFrame, rootFrame);
  nsIWidget* viewWidget = aView->GetWidget();
  viewWidget->SetTransparencyMode(mode);
  windowWidget->SetWindowShadowStyle(rootFrame->StyleUIReset()->mWindowShadow);

  if (!aRC)
    return;
  
  nsBoxLayoutState aState(aPresContext, aRC);
  nsSize minSize = rootFrame->GetMinSize(aState);
  nsSize maxSize = rootFrame->GetMaxSize(aState);

  SetSizeConstraints(aPresContext, windowWidget, minSize, maxSize);
#endif
}

void nsContainerFrame::SetSizeConstraints(nsPresContext* aPresContext,
                                          nsIWidget* aWidget,
                                          const nsSize& aMinSize,
                                          const nsSize& aMaxSize)
{
  nsIntSize devMinSize(aPresContext->AppUnitsToDevPixels(aMinSize.width),
                       aPresContext->AppUnitsToDevPixels(aMinSize.height));
  nsIntSize devMaxSize(aMaxSize.width == NS_INTRINSICSIZE ? NS_MAXSIZE :
                         aPresContext->AppUnitsToDevPixels(aMaxSize.width),
                       aMaxSize.height == NS_INTRINSICSIZE ? NS_MAXSIZE :
                         aPresContext->AppUnitsToDevPixels(aMaxSize.height));
  widget::SizeConstraints constraints(devMinSize, devMaxSize);

  
  
  
  nsIntSize windowSize = aWidget->ClientToWindowSize(nsIntSize(200, 200));
  if (constraints.mMinSize.width)
    constraints.mMinSize.width += windowSize.width - 200;
  if (constraints.mMinSize.height)
    constraints.mMinSize.height += windowSize.height - 200;
  if (constraints.mMaxSize.width != NS_MAXSIZE)
    constraints.mMaxSize.width += windowSize.width - 200;
  if (constraints.mMaxSize.height != NS_MAXSIZE)
    constraints.mMaxSize.height += windowSize.height - 200;

  aWidget->SetSizeConstraints(constraints);
}

void
nsContainerFrame::SyncFrameViewAfterReflow(nsPresContext* aPresContext,
                                           nsIFrame*       aFrame,
                                           nsView*        aView,
                                           const nsRect&   aVisualOverflowArea,
                                           uint32_t        aFlags)
{
  if (!aView) {
    return;
  }

  
  if (0 == (aFlags & NS_FRAME_NO_MOVE_VIEW)) {
    PositionFrameView(aFrame);
  }

  if (0 == (aFlags & NS_FRAME_NO_SIZE_VIEW)) {
    nsViewManager* vm = aView->GetViewManager();

    vm->ResizeView(aView, aVisualOverflowArea, true);
  }
}

void
nsContainerFrame::SyncFrameViewProperties(nsPresContext*  aPresContext,
                                          nsIFrame*        aFrame,
                                          nsStyleContext*  aStyleContext,
                                          nsView*         aView,
                                          uint32_t         aFlags)
{
  NS_ASSERTION(!aStyleContext || aFrame->StyleContext() == aStyleContext,
               "Wrong style context for frame?");

  if (!aView) {
    return;
  }

  nsViewManager* vm = aView->GetViewManager();

  if (nullptr == aStyleContext) {
    aStyleContext = aFrame->StyleContext();
  }

  
  if (0 == (aFlags & NS_FRAME_NO_VISIBILITY) &&
      !aFrame->SupportsVisibilityHidden()) {
    
    vm->SetViewVisibility(aView,
        aStyleContext->StyleVisibility()->IsVisible()
            ? nsViewVisibility_kShow : nsViewVisibility_kHide);
  }

  
  
  bool isPositioned = aFrame->IsPositioned();

  int32_t zIndex = 0;
  bool    autoZIndex = false;

  if (!isPositioned) {
    autoZIndex = true;
  } else {
    
    const nsStylePosition* position = aStyleContext->StylePosition();

    if (position->mZIndex.GetUnit() == eStyleUnit_Integer) {
      zIndex = position->mZIndex.GetIntValue();
    } else if (position->mZIndex.GetUnit() == eStyleUnit_Auto) {
      autoZIndex = true;
    }
  }

  vm->SetViewZIndex(aView, autoZIndex, zIndex, isPositioned);
}

static nscoord GetCoord(const nsStyleCoord& aCoord, nscoord aIfNotCoord)
{
  if (aCoord.ConvertsToLength()) {
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, 0);
  }
  return aIfNotCoord;
}

void
nsContainerFrame::DoInlineIntrinsicWidth(nsRenderingContext *aRenderingContext,
                                         InlineIntrinsicWidthData *aData,
                                         nsLayoutUtils::IntrinsicWidthType aType)
{
  if (GetPrevInFlow())
    return; 

  NS_PRECONDITION(aType == nsLayoutUtils::MIN_WIDTH ||
                  aType == nsLayoutUtils::PREF_WIDTH, "bad type");

  mozilla::css::Side startSide, endSide;
  if (StyleVisibility()->mDirection == NS_STYLE_DIRECTION_LTR) {
    startSide = NS_SIDE_LEFT;
    endSide = NS_SIDE_RIGHT;
  } else {
    startSide = NS_SIDE_RIGHT;
    endSide = NS_SIDE_LEFT;
  }

  const nsStylePadding *stylePadding = StylePadding();
  const nsStyleBorder *styleBorder = StyleBorder();
  const nsStyleMargin *styleMargin = StyleMargin();

  
  
  
  
  
  
  
  if (!GetPrevContinuation()) {
    aData->currentLine +=
      
      std::max(GetCoord(stylePadding->mPadding.Get(startSide), 0), 0) +
      styleBorder->GetComputedBorderWidth(startSide) +
      GetCoord(styleMargin->mMargin.Get(startSide), 0);
  }

  const nsLineList_iterator* savedLine = aData->line;
  nsIFrame* const savedLineContainer = aData->lineContainer;

  nsContainerFrame *lastInFlow;
  for (nsContainerFrame *nif = this; nif;
       nif = static_cast<nsContainerFrame*>(nif->GetNextInFlow())) {
    for (nsIFrame *kid = nif->mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {
      if (aType == nsLayoutUtils::MIN_WIDTH)
        kid->AddInlineMinWidth(aRenderingContext,
                               static_cast<InlineMinWidthData*>(aData));
      else
        kid->AddInlinePrefWidth(aRenderingContext,
                                static_cast<InlinePrefWidthData*>(aData));
    }

    
    
    aData->line = nullptr;
    aData->lineContainer = nullptr;

    lastInFlow = nif;
  }

  aData->line = savedLine;
  aData->lineContainer = savedLineContainer;

  
  
  
  
  
  
  
  if (!lastInFlow->GetNextContinuation()) {
    aData->currentLine +=
      
      std::max(GetCoord(stylePadding->mPadding.Get(endSide), 0), 0) +
      styleBorder->GetComputedBorderWidth(endSide) +
      GetCoord(styleMargin->mMargin.Get(endSide), 0);
  }
}

 nsSize
nsContainerFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                  nsSize aCBSize, nscoord aAvailableWidth,
                                  nsSize aMargin, nsSize aBorder,
                                  nsSize aPadding, bool aShrinkWrap)
{
  nsSize result(0xdeadbeef, NS_UNCONSTRAINEDSIZE);
  nscoord availBased = aAvailableWidth - aMargin.width - aBorder.width -
                       aPadding.width;
  
  if (aShrinkWrap || IsFrameOfType(eReplaced)) {
    
    if (StylePosition()->mWidth.GetUnit() == eStyleUnit_Auto) {
      result.width = ShrinkWidthToFit(aRenderingContext, availBased);
    }
  } else {
    result.width = availBased;
  }
  return result;
}






nsresult
nsContainerFrame::ReflowChild(nsIFrame*                aKidFrame,
                              nsPresContext*           aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              nscoord                  aX,
                              nscoord                  aY,
                              uint32_t                 aFlags,
                              nsReflowStatus&          aStatus,
                              nsOverflowContinuationTracker* aTracker)
{
  NS_PRECONDITION(aReflowState.frame == aKidFrame, "bad reflow state");

  nsresult  result;

  
  
  aKidFrame->WillReflow(aPresContext);

  if (NS_FRAME_NO_MOVE_FRAME != (aFlags & NS_FRAME_NO_MOVE_FRAME)) {
    aKidFrame->SetPosition(nsPoint(aX, aY));
  }

  if (0 == (aFlags & NS_FRAME_NO_MOVE_VIEW)) {
    PositionFrameView(aKidFrame);
  }

  
  result = aKidFrame->Reflow(aPresContext, aDesiredSize, aReflowState,
                             aStatus);

  
  
  if (NS_SUCCEEDED(result) && NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
    nsIFrame* kidNextInFlow = aKidFrame->GetNextInFlow();
    if (nullptr != kidNextInFlow) {
      
      
      
      if (aTracker) aTracker->Finish(aKidFrame);
      static_cast<nsContainerFrame*>(kidNextInFlow->GetParent())
        ->DeleteNextInFlowChild(aPresContext, kidNextInFlow, true);
    }
  }
  return result;
}






void
nsContainerFrame::PositionChildViews(nsIFrame* aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW)) {
    return;
  }

  
  
  
  
  ChildListIterator lists(aFrame);
  for (; !lists.IsDone(); lists.Next()) {
    if (lists.CurrentID() == kPopupList) {
      continue;
    }
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      
      
      nsIFrame* childFrame = childFrames.get();
      if (childFrame->HasView()) {
        PositionFrameView(childFrame);
      } else {
        PositionChildViews(childFrame);
      }
    }
  }
}


















nsresult
nsContainerFrame::FinishReflowChild(nsIFrame*                  aKidFrame,
                                    nsPresContext*             aPresContext,
                                    const nsHTMLReflowState*   aReflowState,
                                    const nsHTMLReflowMetrics& aDesiredSize,
                                    nscoord                    aX,
                                    nscoord                    aY,
                                    uint32_t                   aFlags)
{
  nsPoint curOrigin = aKidFrame->GetPosition();
  nsRect  bounds(aX, aY, aDesiredSize.width, aDesiredSize.height);

  aKidFrame->SetRect(bounds);

  if (aKidFrame->HasView()) {
    nsView* view = aKidFrame->GetView();
    
    
    SyncFrameViewAfterReflow(aPresContext, aKidFrame, view,
                             aDesiredSize.VisualOverflow(), aFlags);
  }

  if (!(aFlags & NS_FRAME_NO_MOVE_VIEW) &&
      (curOrigin.x != aX || curOrigin.y != aY)) {
    if (!aKidFrame->HasView()) {
      
      
      PositionChildViews(aKidFrame);
    }
  }

  return aKidFrame->DidReflow(aPresContext, aReflowState, nsDidReflowStatus::FINISHED);
}

nsresult
nsContainerFrame::ReflowOverflowContainerChildren(nsPresContext*           aPresContext,
                                                  const nsHTMLReflowState& aReflowState,
                                                  nsOverflowAreas&         aOverflowRects,
                                                  uint32_t                 aFlags,
                                                  nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aPresContext, "null pointer");
  nsresult rv = NS_OK;

  nsFrameList* overflowContainers =
               GetPropTableFrames(aPresContext,
                                  OverflowContainersProperty());

  NS_ASSERTION(!(overflowContainers && GetPrevInFlow()
                 && static_cast<nsContainerFrame*>(GetPrevInFlow())
                      ->GetPropTableFrames(aPresContext,
                          ExcessOverflowContainersProperty())),
               "conflicting overflow containers lists");

  if (!overflowContainers) {
    
    nsContainerFrame* prev = (nsContainerFrame*) GetPrevInFlow();
    if (prev) {
      nsFrameList* excessFrames =
        prev->RemovePropTableFrames(aPresContext,
                                    ExcessOverflowContainersProperty());
      if (excessFrames) {
        excessFrames->ApplySetParent(this);
        nsContainerFrame::ReparentFrameViewList(aPresContext, *excessFrames,
                                                prev, this);
        overflowContainers = excessFrames;
        rv = SetPropTableFrames(aPresContext, overflowContainers,
                                OverflowContainersProperty());
        if (NS_FAILED(rv)) {
          excessFrames->DestroyFrames();
          delete excessFrames;
          return rv;
        }
      }
    }
  }

  if (!overflowContainers)
    return NS_OK; 

  nsOverflowContinuationTracker tracker(aPresContext, this, false, false);
  bool shouldReflowAllKids = aReflowState.ShouldReflowAllKids();

  for (nsIFrame* frame = overflowContainers->FirstChild(); frame;
       frame = frame->GetNextSibling()) {
    if (frame->GetPrevInFlow()->GetParent() != GetPrevInFlow()) {
      
      
      continue;
    }
    
    
    if (shouldReflowAllKids || NS_SUBTREE_DIRTY(frame)) {
      
      nsIFrame* prevInFlow = frame->GetPrevInFlow();
      NS_ASSERTION(prevInFlow,
                   "overflow container frame must have a prev-in-flow");
      NS_ASSERTION(frame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER,
                   "overflow container frame must have overflow container bit set");
      nsRect prevRect = prevInFlow->GetRect();

      
      nsSize availSpace(prevRect.width, aReflowState.availableHeight);
      nsHTMLReflowMetrics desiredSize;
      nsHTMLReflowState frameState(aPresContext, aReflowState,
                                   frame, availSpace);
      nsReflowStatus frameStatus;

      
      rv = ReflowChild(frame, aPresContext, desiredSize, frameState,
                       prevRect.x, 0, aFlags, frameStatus, &tracker);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      rv = FinishReflowChild(frame, aPresContext, &frameState, desiredSize,
                             prevRect.x, 0, aFlags);
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (!NS_FRAME_IS_FULLY_COMPLETE(frameStatus)) {
        if (frame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
          
          
          NS_FRAME_SET_OVERFLOW_INCOMPLETE(frameStatus);
        }
        else {
          NS_ASSERTION(NS_FRAME_IS_COMPLETE(frameStatus),
                       "overflow container frames can't be incomplete, only overflow-incomplete");
        }

        
        nsIFrame* nif = frame->GetNextInFlow();
        if (!nif) {
          NS_ASSERTION(frameStatus & NS_FRAME_REFLOW_NEXTINFLOW,
                       "Someone forgot a REFLOW_NEXTINFLOW flag");
          rv = aPresContext->PresShell()->FrameConstructor()->
                 CreateContinuingFrame(aPresContext, frame, this, &nif);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else if (!(nif->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
          
          rv = static_cast<nsContainerFrame*>(nif->GetParent())
                 ->StealFrame(aPresContext, nif);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        tracker.Insert(nif, frameStatus);
      }
      NS_MergeReflowStatusInto(&aStatus, frameStatus);
      
      
      
    }
    else {
      tracker.Skip(frame, aStatus);
      if (aReflowState.mFloatManager)
        nsBlockFrame::RecoverFloatsFor(frame, *aReflowState.mFloatManager);
    }
    ConsiderChildOverflow(aOverflowRects, frame);
  }

  return NS_OK;
}

void
nsContainerFrame::DisplayOverflowContainers(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsFrameList* overflowconts =
    GetPropTableFrames(PresContext(), OverflowContainersProperty());
  if (overflowconts) {
    for (nsIFrame* frame = overflowconts->FirstChild(); frame;
         frame = frame->GetNextSibling()) {
      BuildDisplayListForChild(aBuilder, frame, aDirtyRect, aLists);
    }
  }
}

static bool
TryRemoveFrame(nsIFrame* aFrame, FramePropertyTable* aPropTable,
               const FramePropertyDescriptor* aProp, nsIFrame* aChildToRemove,
               bool (nsFrameList::*aRemoveMethod)(nsIFrame* aFrame))
{
  nsFrameList* list = static_cast<nsFrameList*>(aPropTable->Get(aFrame, aProp));
  if (list && (list->*aRemoveMethod)(aChildToRemove)) {
    
    if (list->IsEmpty()) {
      aPropTable->Remove(aFrame, aProp);
      delete list;
    }
    return true;
  }
  return false;
}

nsresult
nsContainerFrame::StealFrame(nsPresContext* aPresContext,
                             nsIFrame*      aChild,
                             bool           aForceNormal)
{
#ifdef DEBUG
  if (!mFrames.ContainsFrame(aChild)) {
    FramePropertyTable* propTable = aPresContext->PropertyTable();
    nsFrameList* list = static_cast<nsFrameList*>(
                          propTable->Get(this, OverflowContainersProperty()));
    if (!list || !list->ContainsFrame(aChild)) {
      list = static_cast<nsFrameList*>(
               propTable->Get(this, ExcessOverflowContainersProperty()));
      MOZ_ASSERT(list && list->ContainsFrame(aChild), "aChild is not our child "
                 "or on a frame list not supported by StealFrame");
    }
  }
#endif

  bool removed;
  if ((aChild->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
      && !aForceNormal) {
    FramePropertyTable* propTable = aPresContext->PropertyTable();
    
    removed = ::TryRemoveFrame(this, propTable, OverflowContainersProperty(),
                               aChild, &nsFrameList::StartRemoveFrame);
    if (!removed) {
      
      removed = ::TryRemoveFrame(this, propTable,
                                 ExcessOverflowContainersProperty(),
                                 aChild, &nsFrameList::ContinueRemoveFrame);
    }
  } else {
    removed = mFrames.StartRemoveFrame(aChild);
    if (!removed) {
      
      
      nsFrameList* frameList = GetOverflowFrames();
      if (frameList) {
        removed = frameList->ContinueRemoveFrame(aChild);
        if (frameList->IsEmpty()) {
          DestroyOverflowList(aPresContext, nullptr);
        }
      }
    }
  }

  NS_POSTCONDITION(removed, "StealFrame: can't find aChild");
  return removed ? NS_OK : NS_ERROR_UNEXPECTED;
}

nsFrameList
nsContainerFrame::StealFramesAfter(nsIFrame* aChild)
{
  NS_ASSERTION(!aChild ||
               !(aChild->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER),
               "StealFramesAfter doesn't handle overflow containers");
  NS_ASSERTION(GetType() != nsGkAtoms::blockFrame, "unexpected call");

  if (!aChild) {
    nsFrameList copy(mFrames);
    mFrames.Clear();
    return copy;
  }

  for (nsFrameList::FrameLinkEnumerator iter(mFrames); !iter.AtEnd();
       iter.Next()) {
    if (iter.PrevFrame() == aChild) {
      return mFrames.ExtractTail(iter);
    }
  }

  
  
  nsFrameList* overflowFrames = GetOverflowFrames();
  if (overflowFrames) {
    for (nsFrameList::FrameLinkEnumerator iter(*overflowFrames); !iter.AtEnd();
         iter.Next()) {
      if (iter.PrevFrame() == aChild) {
        return overflowFrames->ExtractTail(iter);
      }
    }
  }

  NS_ERROR("StealFramesAfter: can't find aChild");
  return nsFrameList::EmptyList();
}

void
nsContainerFrame::DestroyOverflowList(nsPresContext* aPresContext,
                                      nsIFrame*      aDestructRoot)
{
  nsFrameList* list =
    RemovePropTableFrames(aPresContext, OverflowProperty());
  if (list) {
    if (aDestructRoot)
      list->DestroyFrom(aDestructRoot);
    else
      list->Destroy();
  }
}






nsresult
nsContainerFrame::CreateNextInFlow(nsPresContext* aPresContext,
                                   nsIFrame*      aFrame,
                                   nsIFrame*&     aNextInFlowResult)
{
  NS_PRECONDITION(GetType() != nsGkAtoms::blockFrame,
                  "you should have called nsBlockFrame::CreateContinuationFor instead");
  NS_PRECONDITION(mFrames.ContainsFrame(aFrame), "expected an in-flow child frame");

  aNextInFlowResult = nullptr;

  nsIFrame* nextInFlow = aFrame->GetNextInFlow();
  if (nullptr == nextInFlow) {
    
    
    nsresult rv = aPresContext->PresShell()->FrameConstructor()->
      CreateContinuingFrame(aPresContext, aFrame, this, &nextInFlow);
    if (NS_FAILED(rv)) {
      return rv;
    }
    mFrames.InsertFrame(nullptr, aFrame, nextInFlow);

    NS_FRAME_LOG(NS_FRAME_TRACE_NEW_FRAMES,
       ("nsContainerFrame::CreateNextInFlow: frame=%p nextInFlow=%p",
        aFrame, nextInFlow));

    aNextInFlowResult = nextInFlow;
  }
  return NS_OK;
}





void
nsContainerFrame::DeleteNextInFlowChild(nsPresContext* aPresContext,
                                        nsIFrame*      aNextInFlow,
                                        bool           aDeletingEmptyFrames)
{
#ifdef DEBUG
  nsIFrame* prevInFlow = aNextInFlow->GetPrevInFlow();
#endif
  NS_PRECONDITION(prevInFlow, "bad prev-in-flow");

  
  
  
  
  nsIFrame* nextNextInFlow = aNextInFlow->GetNextInFlow();
  if (nextNextInFlow) {
    nsAutoTArray<nsIFrame*, 8> frames;
    for (nsIFrame* f = nextNextInFlow; f; f = f->GetNextInFlow()) {
      frames.AppendElement(f);
    }
    for (int32_t i = frames.Length() - 1; i >= 0; --i) {
      nsIFrame* delFrame = frames.ElementAt(i);
      static_cast<nsContainerFrame*>(delFrame->GetParent())
        ->DeleteNextInFlowChild(aPresContext, delFrame, aDeletingEmptyFrames);
    }
  }

  
#ifdef DEBUG
  nsresult rv =
#endif
    StealFrame(aPresContext, aNextInFlow);
  NS_ASSERTION(NS_SUCCEEDED(rv), "StealFrame failure");

#ifdef DEBUG
  if (aDeletingEmptyFrames) {
    nsLayoutUtils::AssertTreeOnlyEmptyNextInFlows(aNextInFlow);
  }
#endif

  
  
  aNextInFlow->Destroy();

  NS_POSTCONDITION(!prevInFlow->GetNextInFlow(), "non null next-in-flow");
}




void
nsContainerFrame::SetOverflowFrames(nsPresContext* aPresContext,
                                    const nsFrameList& aOverflowFrames)
{
  NS_PRECONDITION(aOverflowFrames.NotEmpty(), "Shouldn't be called");
  nsFrameList* newList = new nsFrameList(aOverflowFrames);

  aPresContext->PropertyTable()->Set(this, OverflowProperty(), newList);
}

nsFrameList*
nsContainerFrame::GetPropTableFrames(nsPresContext*                 aPresContext,
                                     const FramePropertyDescriptor* aProperty) const
{
  FramePropertyTable* propTable = aPresContext->PropertyTable();
  return static_cast<nsFrameList*>(propTable->Get(this, aProperty));
}

nsFrameList*
nsContainerFrame::RemovePropTableFrames(nsPresContext*                 aPresContext,
                                        const FramePropertyDescriptor* aProperty)
{
  FramePropertyTable* propTable = aPresContext->PropertyTable();
  return static_cast<nsFrameList*>(propTable->Remove(this, aProperty));
}

bool
nsContainerFrame::RemovePropTableFrame(nsPresContext*                 aPresContext,
                                       nsIFrame*                      aFrame,
                                       const FramePropertyDescriptor* aProperty)
{
  nsFrameList* frameList = RemovePropTableFrames(aPresContext, aProperty);
  if (!frameList) {
    
    return false;
  }
  if (!frameList->RemoveFrameIfPresent(aFrame)) {
    
    SetPropTableFrames(aPresContext, frameList, aProperty);
    return false;
  }

  if (frameList->IsEmpty()) {
    
    delete frameList;
  }
  else {
    
    SetPropTableFrames(aPresContext, frameList, aProperty);
  }
  return true;
}

nsresult
nsContainerFrame::SetPropTableFrames(nsPresContext*                 aPresContext,
                                     nsFrameList*                   aFrameList,
                                     const FramePropertyDescriptor* aProperty)
{
  NS_PRECONDITION(aPresContext && aProperty && aFrameList, "null ptr");
  NS_PRECONDITION(
    (aProperty != nsContainerFrame::OverflowContainersProperty() &&
     aProperty != nsContainerFrame::ExcessOverflowContainersProperty()) ||
    IsFrameOfType(nsIFrame::eCanContainOverflowContainers),
    "this type of frame can't have overflow containers");
  aPresContext->PropertyTable()->Set(this, aProperty, aFrameList);
  return NS_OK;
}















void
nsContainerFrame::PushChildren(nsPresContext* aPresContext,
                               nsIFrame*       aFromChild,
                               nsIFrame*       aPrevSibling)
{
  NS_PRECONDITION(aFromChild, "null pointer");
  NS_PRECONDITION(aPrevSibling, "pushing first child");
  NS_PRECONDITION(aPrevSibling->GetNextSibling() == aFromChild, "bad prev sibling");

  
  nsFrameList tail = mFrames.RemoveFramesAfter(aPrevSibling);

  nsContainerFrame* nextInFlow =
    static_cast<nsContainerFrame*>(GetNextInFlow());
  if (nextInFlow) {
    
    
    
    
    
    for (nsIFrame* f = aFromChild; f; f = f->GetNextSibling()) {
      nsContainerFrame::ReparentFrameView(aPresContext, f, this, nextInFlow);
    }
    nextInFlow->mFrames.InsertFrames(nextInFlow, nullptr, tail);
  }
  else {
    
    SetOverflowFrames(aPresContext, tail);
  }
}









bool
nsContainerFrame::MoveOverflowToChildList(nsPresContext* aPresContext)
{
  bool result = false;

  
  nsContainerFrame* prevInFlow = (nsContainerFrame*)GetPrevInFlow();
  if (nullptr != prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      
      NS_ASSERTION(mFrames.IsEmpty() || GetType() == nsGkAtoms::tableFrame,
                   "bad overflow list");
      
      
      nsContainerFrame::ReparentFrameViewList(aPresContext,
                                              *prevOverflowFrames,
                                              prevInFlow, this);
      mFrames.AppendFrames(this, *prevOverflowFrames);
      result = true;
    }
  }

  
  return DrainSelfOverflowList() || result;
}

bool
nsContainerFrame::DrainSelfOverflowList()
{
  nsAutoPtr<nsFrameList> overflowFrames(StealOverflowFrames());
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    mFrames.AppendFrames(nullptr, *overflowFrames);
    return true;
  }
  return false;
}

nsOverflowContinuationTracker::nsOverflowContinuationTracker(nsPresContext*    aPresContext,
                                                             nsContainerFrame* aFrame,
                                                             bool              aWalkOOFFrames,
                                                             bool              aSkipOverflowContainerChildren)
  : mOverflowContList(nullptr),
    mPrevOverflowCont(nullptr),
    mSentry(nullptr),
    mParent(aFrame),
    mSkipOverflowContainerChildren(aSkipOverflowContainerChildren),
    mWalkOOFFrames(aWalkOOFFrames)
{
  NS_PRECONDITION(aFrame, "null frame pointer");
  nsContainerFrame* next = static_cast<nsContainerFrame*>
                             (aFrame->GetNextInFlow());
  if (next) {
    mOverflowContList = next->GetPropTableFrames(aPresContext,
      nsContainerFrame::OverflowContainersProperty());
    if (mOverflowContList) {
      mParent = next;
      SetUpListWalker();
    }
  }
  if (!mOverflowContList) {
    mOverflowContList = mParent->GetPropTableFrames(aPresContext,
      nsContainerFrame::ExcessOverflowContainersProperty());
    if (mOverflowContList) {
      SetUpListWalker();
    }
  }
}





void
nsOverflowContinuationTracker::SetUpListWalker()
{
  NS_ASSERTION(!mSentry && !mPrevOverflowCont,
               "forgot to reset mSentry or mPrevOverflowCont");
  if (mOverflowContList) {
    nsIFrame* cur = mOverflowContList->FirstChild();
    if (mSkipOverflowContainerChildren) {
      while (cur && (cur->GetPrevInFlow()->GetStateBits()
                     & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
        mPrevOverflowCont = cur;
        cur = cur->GetNextSibling();
      }
      while (cur && (!(cur->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
                     == mWalkOOFFrames)) {
        mPrevOverflowCont = cur;
        cur = cur->GetNextSibling();
      }
    }
    if (cur) {
      mSentry = cur->GetPrevInFlow();
    }
  }
}







void
nsOverflowContinuationTracker::StepForward()
{
  NS_PRECONDITION(mOverflowContList, "null list");

  
  if (mPrevOverflowCont) {
    mPrevOverflowCont = mPrevOverflowCont->GetNextSibling();
  }
  else {
    mPrevOverflowCont = mOverflowContList->FirstChild();
  }

  
  if (mSkipOverflowContainerChildren) {
    nsIFrame* cur = mPrevOverflowCont->GetNextSibling();
    while (cur && (!(cur->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
                   == mWalkOOFFrames)) {
      mPrevOverflowCont = cur;
      cur = cur->GetNextSibling();
    }
  }

  
  mSentry = (mPrevOverflowCont->GetNextSibling())
            ? mPrevOverflowCont->GetNextSibling()->GetPrevInFlow()
            : nullptr;
}

nsresult
nsOverflowContinuationTracker::Insert(nsIFrame*       aOverflowCont,
                                      nsReflowStatus& aReflowStatus)
{
  NS_PRECONDITION(aOverflowCont, "null frame pointer");
  NS_PRECONDITION(!mSkipOverflowContainerChildren || mWalkOOFFrames ==
                  !!(aOverflowCont->GetStateBits() & NS_FRAME_OUT_OF_FLOW),
                  "shouldn't insert frame that doesn't match walker type");
  NS_PRECONDITION(aOverflowCont->GetPrevInFlow(),
                  "overflow containers must have a prev-in-flow");
  nsresult rv = NS_OK;
  bool reparented = false;
  nsPresContext* presContext = aOverflowCont->PresContext();
  const bool addToList = !mSentry || aOverflowCont != mSentry->GetNextInFlow();
  if (addToList) {
    if (aOverflowCont->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
      
      
      NS_ASSERTION(!(mOverflowContList &&
                     mOverflowContList->ContainsFrame(aOverflowCont)),
                   "overflow containers out of order");
      rv = static_cast<nsContainerFrame*>(aOverflowCont->GetParent())
             ->StealFrame(presContext, aOverflowCont);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      aOverflowCont->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
    }
    if (!mOverflowContList) {
      mOverflowContList = new nsFrameList();
      rv = mParent->SetPropTableFrames(presContext, mOverflowContList,
        nsContainerFrame::ExcessOverflowContainersProperty());
      NS_ENSURE_SUCCESS(rv, rv);
      SetUpListWalker();
    }
    if (aOverflowCont->GetParent() != mParent) {
      nsContainerFrame::ReparentFrameView(presContext, aOverflowCont,
                                          aOverflowCont->GetParent(),
                                          mParent);
      reparented = true;
    }
    mOverflowContList->InsertFrame(mParent, mPrevOverflowCont, aOverflowCont);
    aReflowStatus |= NS_FRAME_REFLOW_NEXTINFLOW;
  }

  
  if (aReflowStatus & NS_FRAME_REFLOW_NEXTINFLOW)
    aOverflowCont->AddStateBits(NS_FRAME_IS_DIRTY);

  
  StepForward();
  NS_ASSERTION(mPrevOverflowCont == aOverflowCont ||
               (mSkipOverflowContainerChildren &&
                (mPrevOverflowCont->GetStateBits() & NS_FRAME_OUT_OF_FLOW) !=
                (aOverflowCont->GetStateBits() & NS_FRAME_OUT_OF_FLOW)),
              "OverflowContTracker in unexpected state");

  if (addToList) {
    
    
    
    
    nsIFrame* f = aOverflowCont->GetNextContinuation();
    if (f && (!(f->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) ||
              (!reparented && f->GetParent() == mParent) ||
              (reparented && f->GetParent() != mParent))) {
      if (!(f->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
        nsContainerFrame* parent = static_cast<nsContainerFrame*>(f->GetParent());
        rv = parent->StealFrame(presContext, f);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      Insert(f, aReflowStatus);
    }
  }
  return rv;
}

void
nsOverflowContinuationTracker::Finish(nsIFrame* aChild)
{
  NS_PRECONDITION(aChild, "null ptr");
  NS_PRECONDITION(aChild->GetNextInFlow(),
                  "supposed to call Finish *before* deleting next-in-flow!");

  for (nsIFrame* f = aChild; f; ) {
    
    
    nsIFrame* nif = f->GetNextInFlow();
    if (mOverflowContList &&
        mOverflowContList->FirstChild() == nif &&
        (!nif->GetNextSibling() ||
         nif->GetNextSibling() == nif->GetNextInFlow())) {
      mOverflowContList = nullptr;
      mPrevOverflowCont = nullptr;
      mSentry = nullptr;
      mParent = static_cast<nsContainerFrame*>(f->GetParent());
      break;
    }
    if (f == mSentry) {
      
      nsIFrame* prevOverflowCont = mPrevOverflowCont;
      StepForward();
      if (mPrevOverflowCont == nif) {
        
        
        mPrevOverflowCont = prevOverflowCont;
      }
    }
    f = nif;
  }
}




#ifdef DEBUG
NS_IMETHODIMP
nsContainerFrame::List(FILE* out, int32_t aIndent, uint32_t aFlags) const
{
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", static_cast<void*>(mParent));
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }
  if (GetNextSibling()) {
    fprintf(out, " next=%p", static_cast<void*>(GetNextSibling()));
  }
  if (nullptr != GetPrevContinuation()) {
    fprintf(out, " prev-continuation=%p", static_cast<void*>(GetPrevContinuation()));
  }
  if (nullptr != GetNextContinuation()) {
    fprintf(out, " next-continuation=%p", static_cast<void*>(GetNextContinuation()));
  }
  void* IBsibling = Properties().Get(IBSplitSpecialSibling());
  if (IBsibling) {
    fprintf(out, " IBSplitSpecialSibling=%p", IBsibling);
  }
  void* IBprevsibling = Properties().Get(IBSplitSpecialPrevSibling());
  if (IBprevsibling) {
    fprintf(out, " IBSplitSpecialPrevSibling=%p", IBprevsibling);
  }
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    fprintf(out, " [state=%016llx]", (unsigned long long)mState);
  }
  fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  nsContainerFrame* f = const_cast<nsContainerFrame*>(this);
  if (f->HasOverflowAreas()) {
    nsRect overflowArea = f->GetVisualOverflowRect();
    fprintf(out, " [vis-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
    overflowArea = f->GetScrollableOverflowRect();
    fprintf(out, " [scr-overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  fprintf(out, " [sc=%p]", static_cast<void*>(mStyleContext));
  nsIAtom* pseudoTag = mStyleContext->GetPseudo();
  if (pseudoTag) {
    nsAutoString atomString;
    pseudoTag->ToString(atomString);
    fprintf(out, " pst=%s",
            NS_LossyConvertUTF16toASCII(atomString).get());
  }

  
  bool outputOneList = false;
  ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    if (outputOneList) {
      IndentBy(out, aIndent);
    }
    outputOneList = true;
    fputs(mozilla::layout::ChildListName(lists.CurrentID()), out);
    fputs("<\n", out);
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* kid = childFrames.get();
      
      NS_ASSERTION(kid->GetParent() == this, "bad parent frame pointer");

      
      kid->List(out, aIndent + 1, aFlags);
    }
    IndentBy(out, aIndent);
    fputs(">\n", out);
  }

  if (!outputOneList) {
    fputs("<>\n", out);
  }

  return NS_OK;
}
#endif
