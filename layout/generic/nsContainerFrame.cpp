






#include "nsContainerFrame.h"

#include "nsAbsoluteContainingBlock.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsStyleConsts.h"
#include "nsView.h"
#include "nsIPresShell.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsViewManager.h"
#include "nsIWidget.h"
#include "nsCSSRendering.h"
#include "nsError.h"
#include "nsDisplayList.h"
#include "nsIBaseWindow.h"
#include "nsBoxLayoutState.h"
#include "nsCSSFrameConstructor.h"
#include "nsBlockFrame.h"
#include "mozilla/AutoRestore.h"
#include "nsIFrameInlines.h"
#include "nsPrintfCString.h"
#include <algorithm>

#ifdef DEBUG
#undef NOISY
#else
#undef NOISY
#endif

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layout;

NS_IMPL_FRAMEARENA_HELPERS(nsContainerFrame)

nsContainerFrame::~nsContainerFrame()
{
}

NS_QUERYFRAME_HEAD(nsContainerFrame)
  NS_QUERYFRAME_ENTRY(nsContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSplittableFrame)

void
nsContainerFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  nsSplittableFrame::Init(aContent, aParent, aPrevInFlow);
  if (aPrevInFlow) {
    
    
    
    if (aPrevInFlow->GetStateBits() & NS_FRAME_HAS_CHILD_WITH_VIEW)
      AddStateBits(NS_FRAME_HAS_CHILD_WITH_VIEW);
  }
}

void
nsContainerFrame::SetInitialChildList(ChildListID  aListID,
                                      nsFrameList& aChildList)
{
  MOZ_ASSERT(mFrames.IsEmpty(),
             "unexpected second call to SetInitialChildList");
  MOZ_ASSERT(aListID == kPrincipalList, "unexpected child list");
#ifdef DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  mFrames.SetFrames(aChildList);
}

void
nsContainerFrame::AppendFrames(ChildListID  aListID,
                               nsFrameList& aFrameList)
{
  MOZ_ASSERT(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
             "unexpected child list");

  if (MOZ_UNLIKELY(aFrameList.IsEmpty())) {
    return;
  }

  DrainSelfOverflowList(); 
  mFrames.AppendFrames(this, aFrameList);

  if (aListID != kNoReflowPrincipalList) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
  }
}

void
nsContainerFrame::InsertFrames(ChildListID aListID,
                               nsIFrame* aPrevFrame,
                               nsFrameList& aFrameList)
{
  MOZ_ASSERT(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
             "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == this,
               "inserting after sibling frame with different parent");

  if (MOZ_UNLIKELY(aFrameList.IsEmpty())) {
    return;
  }

  DrainSelfOverflowList(); 
  mFrames.InsertFrames(this, aPrevFrame, aFrameList);

  if (aListID != kNoReflowPrincipalList) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                       NS_FRAME_HAS_DIRTY_CHILDREN);
  }
}

void
nsContainerFrame::RemoveFrame(ChildListID aListID,
                              nsIFrame* aOldFrame)
{
  MOZ_ASSERT(aListID == kPrincipalList || aListID == kNoReflowPrincipalList,
             "unexpected child list");

  
  
  
  bool generateReflowCommand = true;
  if (kNoReflowPrincipalList == aListID) {
    generateReflowCommand = false;
  }
  nsIPresShell* shell = PresContext()->PresShell();
  nsContainerFrame* lastParent = nullptr;
  while (aOldFrame) {
    nsIFrame* oldFrameNextContinuation = aOldFrame->GetNextContinuation();
    nsContainerFrame* parent = aOldFrame->GetParent();
    
    
    
    parent->StealFrame(aOldFrame, true);
    aOldFrame->Destroy();
    aOldFrame = oldFrameNextContinuation;
    if (parent != lastParent && generateReflowCommand) {
      shell->FrameNeedsReflow(parent, nsIPresShell::eTreeChange,
                              NS_FRAME_HAS_DIRTY_CHILDREN);
      lastParent = parent;
    }
  }
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
                                             nsIPresShell* aPresShell,
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
      frameList->Delete(aPresShell);
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

  
  nsPresContext* pc = PresContext();
  nsIPresShell* shell = pc->PresShell();
  FramePropertyTable* props = pc->PropertyTable();
  SafelyDestroyFrameListProp(aDestructRoot, shell, props, OverflowProperty());

  MOZ_ASSERT(IsFrameOfType(nsIFrame::eCanContainOverflowContainers) ||
             !(props->Get(this, nsContainerFrame::OverflowContainersProperty()) ||
               props->Get(this, nsContainerFrame::ExcessOverflowContainersProperty())),
             "this type of frame should't have overflow containers");

  SafelyDestroyFrameListProp(aDestructRoot, shell, props,
                             OverflowContainersProperty());
  SafelyDestroyFrameListProp(aDestructRoot, shell, props,
                             ExcessOverflowContainersProperty());

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
      nsFrameList* list = GetPropTableFrames(OverflowContainersProperty());
      return list ? *list : nsFrameList::EmptyList();
    }
    case kExcessOverflowContainersList: {
      nsFrameList* list =
        GetPropTableFrames(ExcessOverflowContainersProperty());
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
  NS_ASSERTION(NS_SUBTREE_DIRTY(aChild), "child isn't actually dirty");

  AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
}

bool
nsContainerFrame::IsLeaf() const
{
  return false;
}

nsIFrame::FrameSearchResult
nsContainerFrame::PeekOffsetNoAmount(bool aForward, int32_t* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return CONTINUE_EMPTY;
}

nsIFrame::FrameSearchResult
nsContainerFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                      bool aRespectClusters)
{
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  
  return CONTINUE_EMPTY;
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

void
nsContainerFrame::CreateViewForFrame(nsIFrame* aFrame,
                                     bool aForce)
{
  if (aFrame->HasView()) {
    return;
  }

  
  if (!aForce && !aFrame->NeedsView()) {
    
    return;
  }

  nsView* parentView = aFrame->GetParent()->GetClosestView();
  NS_ASSERTION(parentView, "no parent with view");

  nsViewManager* viewManager = parentView->GetViewManager();
  NS_ASSERTION(viewManager, "null view manager");

  
  nsView* view = viewManager->CreateView(aFrame->GetRect(), parentView);

  SyncFrameViewProperties(aFrame->PresContext(), aFrame, nullptr, view);

  nsView* insertBefore = nsLayoutUtils::FindSiblingViewFor(parentView, aFrame);
  
  
  
  viewManager->InsertChild(parentView, view, insertBefore, insertBefore != nullptr);

  
  
  
  
  
  
  
  
  ReparentFrameViewTo(aFrame, viewManager, view, parentView);

  
  aFrame->SetView(view);

  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
               ("nsContainerFrame::CreateViewForFrame: frame=%p view=%p",
                aFrame));
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
nsContainerFrame::ReparentFrameView(nsIFrame* aChildFrame,
                                    nsIFrame* aOldParentFrame,
                                    nsIFrame* aNewParentFrame)
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
nsContainerFrame::ReparentFrameViewList(const nsFrameList& aChildFrameList,
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
  nsWindowType windowType = aWidget->WindowType();
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
  if (!rootElement || !rootElement->IsXULElement()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
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
  LayoutDeviceIntSize devMinSize(aPresContext->AppUnitsToDevPixels(aMinSize.width),
                                 aPresContext->AppUnitsToDevPixels(aMinSize.height));
  LayoutDeviceIntSize devMaxSize(aMaxSize.width == NS_INTRINSICSIZE ? NS_MAXSIZE :
                                 aPresContext->AppUnitsToDevPixels(aMaxSize.width),
                                 aMaxSize.height == NS_INTRINSICSIZE ? NS_MAXSIZE :
                                 aPresContext->AppUnitsToDevPixels(aMaxSize.height));

  
  if (devMinSize.width > devMaxSize.width)
    devMaxSize.width = devMinSize.width;
  if (devMinSize.height > devMaxSize.height)
    devMaxSize.height = devMinSize.height;

  widget::SizeConstraints constraints(devMinSize, devMaxSize);

  
  
  
  LayoutDeviceIntSize windowSize =
    aWidget->ClientToWindowSize(LayoutDeviceIntSize(200, 200));
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

  int32_t zIndex = 0;
  bool    autoZIndex = false;

  if (aFrame->IsAbsPosContaininingBlock()) {
    
    const nsStylePosition* position = aStyleContext->StylePosition();

    if (position->mZIndex.GetUnit() == eStyleUnit_Integer) {
      zIndex = position->mZIndex.GetIntValue();
    } else if (position->mZIndex.GetUnit() == eStyleUnit_Auto) {
      autoZIndex = true;
    }
  } else {
    autoZIndex = true;
  }

  vm->SetViewZIndex(aView, autoZIndex, zIndex);
}

static nscoord GetCoord(const nsStyleCoord& aCoord, nscoord aIfNotCoord)
{
  if (aCoord.ConvertsToLength()) {
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, 0);
  }
  return aIfNotCoord;
}

void
nsContainerFrame::DoInlineIntrinsicISize(nsRenderingContext *aRenderingContext,
                                         InlineIntrinsicISizeData *aData,
                                         nsLayoutUtils::IntrinsicISizeType aType)
{
  if (GetPrevInFlow())
    return; 

  NS_PRECONDITION(aType == nsLayoutUtils::MIN_ISIZE ||
                  aType == nsLayoutUtils::PREF_ISIZE, "bad type");

  WritingMode wm = GetWritingMode();
  mozilla::css::Side startSide =
    wm.PhysicalSideForInlineAxis(eLogicalEdgeStart);
  mozilla::css::Side endSide =
    wm.PhysicalSideForInlineAxis(eLogicalEdgeEnd);

  const nsStylePadding *stylePadding = StylePadding();
  const nsStyleBorder *styleBorder = StyleBorder();
  const nsStyleMargin *styleMargin = StyleMargin();

  
  
  
  
  
  
  
  
  
  nscoord clonePBM = 0; 
  const bool sliceBreak =
    styleBorder->mBoxDecorationBreak == NS_STYLE_BOX_DECORATION_BREAK_SLICE;
  if (!GetPrevContinuation()) {
    nscoord startPBM =
      
      std::max(GetCoord(stylePadding->mPadding.Get(startSide), 0), 0) +
      styleBorder->GetComputedBorderWidth(startSide) +
      GetCoord(styleMargin->mMargin.Get(startSide), 0);
    if (MOZ_LIKELY(sliceBreak)) {
      aData->currentLine += startPBM;
    } else {
      clonePBM = startPBM;
    }
  }

  nscoord endPBM =
    
    std::max(GetCoord(stylePadding->mPadding.Get(endSide), 0), 0) +
    styleBorder->GetComputedBorderWidth(endSide) +
    GetCoord(styleMargin->mMargin.Get(endSide), 0);
  if (MOZ_UNLIKELY(!sliceBreak)) {
    clonePBM += endPBM;
  }

  const nsLineList_iterator* savedLine = aData->line;
  nsIFrame* const savedLineContainer = aData->lineContainer;

  nsContainerFrame *lastInFlow;
  for (nsContainerFrame *nif = this; nif;
       nif = static_cast<nsContainerFrame*>(nif->GetNextInFlow())) {
    if (aData->currentLine == 0) {
      aData->currentLine = clonePBM;
    }
    for (nsIFrame *kid = nif->mFrames.FirstChild(); kid;
         kid = kid->GetNextSibling()) {
      if (aType == nsLayoutUtils::MIN_ISIZE)
        kid->AddInlineMinISize(aRenderingContext,
                               static_cast<InlineMinISizeData*>(aData));
      else
        kid->AddInlinePrefISize(aRenderingContext,
                                static_cast<InlinePrefISizeData*>(aData));
    }

    
    
    aData->line = nullptr;
    aData->lineContainer = nullptr;

    lastInFlow = nif;
  }

  aData->line = savedLine;
  aData->lineContainer = savedLineContainer;

  
  
  
  
  
  
  
  if (MOZ_LIKELY(!lastInFlow->GetNextContinuation() && sliceBreak)) {
    aData->currentLine += endPBM;
  }
}


LogicalSize
nsContainerFrame::ComputeAutoSize(nsRenderingContext* aRenderingContext,
                                  WritingMode aWM,
                                  const LogicalSize& aCBSize,
                                  nscoord aAvailableISize,
                                  const LogicalSize& aMargin,
                                  const LogicalSize& aBorder,
                                  const LogicalSize& aPadding,
                                  bool aShrinkWrap)
{
  LogicalSize result(aWM, 0xdeadbeef, NS_UNCONSTRAINEDSIZE);
  nscoord availBased = aAvailableISize - aMargin.ISize(aWM) -
                       aBorder.ISize(aWM) - aPadding.ISize(aWM);
  
  if (aShrinkWrap || IsFrameOfType(eReplaced)) {
    
    const nsStyleCoord& inlineStyleCoord =
      aWM.IsVertical() ? StylePosition()->mHeight : StylePosition()->mWidth;
    if (inlineStyleCoord.GetUnit() == eStyleUnit_Auto) {
      result.ISize(aWM) = ShrinkWidthToFit(aRenderingContext, availBased);
    }
  } else {
    result.ISize(aWM) = availBased;
  }

  if (IsTableCaption()) {
    
    
    AutoMaybeDisableFontInflation an(this);

    
    uint8_t captionSide = StyleTableBorder()->mCaptionSide;
    if (captionSide == NS_STYLE_CAPTION_SIDE_LEFT ||
        captionSide == NS_STYLE_CAPTION_SIDE_RIGHT) {
      result.ISize(aWM) = GetMinISize(aRenderingContext);
    } else if (captionSide == NS_STYLE_CAPTION_SIDE_TOP ||
               captionSide == NS_STYLE_CAPTION_SIDE_BOTTOM) {
      
      
      
      
      
      nscoord min = GetMinISize(aRenderingContext);
      if (min > aCBSize.ISize(aWM)) {
        min = aCBSize.ISize(aWM);
      }
      if (min > result.ISize(aWM)) {
        result.ISize(aWM) = min;
      }
    }
  }
  return result;
}

void
nsContainerFrame::ReflowChild(nsIFrame*                aKidFrame,
                              nsPresContext*           aPresContext,
                              nsHTMLReflowMetrics&     aDesiredSize,
                              const nsHTMLReflowState& aReflowState,
                              const WritingMode&       aWM,
                              const LogicalPoint&      aPos,
                              nscoord                  aContainerWidth,
                              uint32_t                 aFlags,
                              nsReflowStatus&          aStatus,
                              nsOverflowContinuationTracker* aTracker)
{
  NS_PRECONDITION(aReflowState.frame == aKidFrame, "bad reflow state");
  if (aWM.IsVerticalRL() || (!aWM.IsVertical() && !aWM.IsBidiLTR())) {
    NS_ASSERTION(aContainerWidth != NS_UNCONSTRAINEDSIZE,
                 "ReflowChild with unconstrained container width!");
  }

  
  if (NS_FRAME_NO_MOVE_FRAME != (aFlags & NS_FRAME_NO_MOVE_FRAME)) {
    aKidFrame->SetPosition(aWM, aPos, aContainerWidth);
  }

  if (0 == (aFlags & NS_FRAME_NO_MOVE_VIEW)) {
    PositionFrameView(aKidFrame);
  }

  
  aKidFrame->Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
  if (NS_FRAME_IS_FULLY_COMPLETE(aStatus) &&
      !(aFlags & NS_FRAME_NO_DELETE_NEXT_IN_FLOW_CHILD)) {
    nsIFrame* kidNextInFlow = aKidFrame->GetNextInFlow();
    if (kidNextInFlow) {
      
      
      
      nsOverflowContinuationTracker::AutoFinish fini(aTracker, aKidFrame);
      kidNextInFlow->GetParent()->DeleteNextInFlowChild(kidNextInFlow, true);
    }
  }
}



void
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

  
  if (NS_FRAME_NO_MOVE_FRAME != (aFlags & NS_FRAME_NO_MOVE_FRAME)) {
    aKidFrame->SetPosition(nsPoint(aX, aY));
  }

  if (0 == (aFlags & NS_FRAME_NO_MOVE_VIEW)) {
    PositionFrameView(aKidFrame);
  }

  
  aKidFrame->Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
  if (NS_FRAME_IS_FULLY_COMPLETE(aStatus) &&
      !(aFlags & NS_FRAME_NO_DELETE_NEXT_IN_FLOW_CHILD)) {
    nsIFrame* kidNextInFlow = aKidFrame->GetNextInFlow();
    if (kidNextInFlow) {
      
      
      
      nsOverflowContinuationTracker::AutoFinish fini(aTracker, aKidFrame);
      kidNextInFlow->GetParent()->DeleteNextInFlowChild(kidNextInFlow, true);
    }
  }
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


















void
nsContainerFrame::FinishReflowChild(nsIFrame*                  aKidFrame,
                                    nsPresContext*             aPresContext,
                                    const nsHTMLReflowMetrics& aDesiredSize,
                                    const nsHTMLReflowState*   aReflowState,
                                    const WritingMode&         aWM,
                                    const LogicalPoint&        aPos,
                                    nscoord                    aContainerWidth,
                                    uint32_t                   aFlags)
{
  if (aWM.IsVerticalRL() || (!aWM.IsVertical() && !aWM.IsBidiLTR())) {
    NS_ASSERTION(aContainerWidth != NS_UNCONSTRAINEDSIZE,
                 "FinishReflowChild with unconstrained container width!");
  }

  nsPoint curOrigin = aKidFrame->GetPosition();
  WritingMode outerWM = aDesiredSize.GetWritingMode();
  LogicalSize convertedSize = aDesiredSize.Size(outerWM).ConvertTo(aWM,
                                                                   outerWM);

  if (NS_FRAME_NO_MOVE_FRAME != (aFlags & NS_FRAME_NO_MOVE_FRAME)) {
    aKidFrame->SetRect(aWM, LogicalRect(aWM, aPos, convertedSize),
                       aContainerWidth);
  } else {
    aKidFrame->SetSize(aWM, convertedSize);
  }

  if (aKidFrame->HasView()) {
    nsView* view = aKidFrame->GetView();
    
    
    SyncFrameViewAfterReflow(aPresContext, aKidFrame, view,
                             aDesiredSize.VisualOverflow(), aFlags);
  }

  nsPoint newOrigin = aKidFrame->GetPosition();
  if (!(aFlags & NS_FRAME_NO_MOVE_VIEW) && curOrigin != newOrigin) {
    if (!aKidFrame->HasView()) {
      
      
      PositionChildViews(aKidFrame);
    }
  }

  aKidFrame->DidReflow(aPresContext, aReflowState, nsDidReflowStatus::FINISHED);
}



void
nsContainerFrame::FinishReflowChild(nsIFrame*                  aKidFrame,
                                    nsPresContext*             aPresContext,
                                    const nsHTMLReflowMetrics& aDesiredSize,
                                    const nsHTMLReflowState*   aReflowState,
                                    nscoord                    aX,
                                    nscoord                    aY,
                                    uint32_t                   aFlags)
{
  nsPoint curOrigin = aKidFrame->GetPosition();

  if (NS_FRAME_NO_MOVE_FRAME != (aFlags & NS_FRAME_NO_MOVE_FRAME)) {
    aKidFrame->SetRect(nsRect(aX, aY, aDesiredSize.Width(), aDesiredSize.Height()));
  } else {
    aKidFrame->SetSize(nsSize(aDesiredSize.Width(), aDesiredSize.Height()));
  }

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

  aKidFrame->DidReflow(aPresContext, aReflowState, nsDidReflowStatus::FINISHED);
}

void
nsContainerFrame::ReflowOverflowContainerChildren(nsPresContext*           aPresContext,
                                                  const nsHTMLReflowState& aReflowState,
                                                  nsOverflowAreas&         aOverflowRects,
                                                  uint32_t                 aFlags,
                                                  nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aPresContext, "null pointer");

  nsFrameList* overflowContainers =
               GetPropTableFrames(OverflowContainersProperty());

  NS_ASSERTION(!(overflowContainers && GetPrevInFlow()
                 && static_cast<nsContainerFrame*>(GetPrevInFlow())
                      ->GetPropTableFrames(ExcessOverflowContainersProperty())),
               "conflicting overflow containers lists");

  if (!overflowContainers) {
    
    nsContainerFrame* prev = (nsContainerFrame*) GetPrevInFlow();
    if (prev) {
      nsFrameList* excessFrames =
        prev->RemovePropTableFrames(ExcessOverflowContainersProperty());
      if (excessFrames) {
        excessFrames->ApplySetParent(this);
        nsContainerFrame::ReparentFrameViewList(*excessFrames, prev, this);
        overflowContainers = excessFrames;
        SetPropTableFrames(overflowContainers, OverflowContainersProperty());
      }
    }
  }

  
  
  nsFrameList* selfExcessOCFrames =
    RemovePropTableFrames(ExcessOverflowContainersProperty());
  if (selfExcessOCFrames) {
    if (overflowContainers) {
      overflowContainers->AppendFrames(nullptr, *selfExcessOCFrames);
      selfExcessOCFrames->Delete(aPresContext->PresShell());
    } else {
      overflowContainers = selfExcessOCFrames;
      SetPropTableFrames(overflowContainers, OverflowContainersProperty());
    }
  }
  if (!overflowContainers) {
    return; 
  }

  nsOverflowContinuationTracker tracker(this, false, false);
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
      WritingMode wm = frame->GetWritingMode();
      nscoord containerWidth = aReflowState.AvailableSize(wm).Width(wm);
      LogicalRect prevRect = prevInFlow->GetLogicalRect(wm, containerWidth);

      
      LogicalSize availSpace(wm, prevRect.ISize(wm),
                             aReflowState.AvailableSize(wm).BSize(wm));
      nsHTMLReflowMetrics desiredSize(aReflowState);
      nsHTMLReflowState frameState(aPresContext, aReflowState,
                                   frame, availSpace);
      nsReflowStatus frameStatus;

      
      LogicalPoint pos(wm, prevRect.IStart(wm), 0);
      ReflowChild(frame, aPresContext, desiredSize, frameState,
                  wm, pos, containerWidth, aFlags, frameStatus, &tracker);
      
      
      FinishReflowChild(frame, aPresContext, desiredSize, &frameState,
                        wm, pos, containerWidth, aFlags);

      
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
          nif = aPresContext->PresShell()->FrameConstructor()->
            CreateContinuingFrame(aPresContext, frame, this);
        }
        else if (!(nif->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)) {
          
          nsresult rv = nif->GetParent()->StealFrame(nif);
          if (NS_FAILED(rv)) {
            return;
          }
        }

        tracker.Insert(nif, frameStatus);
      }
      NS_MergeReflowStatusInto(&aStatus, frameStatus);
      
      
      
    }
    else {
      tracker.Skip(frame, aStatus);
      if (aReflowState.mFloatManager) {
        nsBlockFrame::RecoverFloatsFor(frame, *aReflowState.mFloatManager,
                                       aReflowState.GetWritingMode(),
                                       aReflowState.ComputedWidth());
      }
    }
    ConsiderChildOverflow(aOverflowRects, frame);
  }
}

void
nsContainerFrame::DisplayOverflowContainers(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsFrameList* overflowconts = GetPropTableFrames(OverflowContainersProperty());
  if (overflowconts) {
    for (nsIFrame* frame = overflowconts->FirstChild(); frame;
         frame = frame->GetNextSibling()) {
      BuildDisplayListForChild(aBuilder, frame, aDirtyRect, aLists);
    }
  }
}

static bool
TryRemoveFrame(nsIFrame* aFrame, FramePropertyTable* aPropTable,
               const FramePropertyDescriptor* aProp, nsIFrame* aChildToRemove)
{
  nsFrameList* list = static_cast<nsFrameList*>(aPropTable->Get(aFrame, aProp));
  if (list && list->StartRemoveFrame(aChildToRemove)) {
    
    if (list->IsEmpty()) {
      aPropTable->Remove(aFrame, aProp);
      list->Delete(aFrame->PresContext()->PresShell());
    }
    return true;
  }
  return false;
}

nsresult
nsContainerFrame::StealFrame(nsIFrame* aChild,
                             bool      aForceNormal)
{
#ifdef DEBUG
  if (!mFrames.ContainsFrame(aChild)) {
    nsFrameList* list = GetOverflowFrames();
    if (!list || !list->ContainsFrame(aChild)) {
      FramePropertyTable* propTable = PresContext()->PropertyTable();
      list = static_cast<nsFrameList*>(
               propTable->Get(this, OverflowContainersProperty()));
      if (!list || !list->ContainsFrame(aChild)) {
        list = static_cast<nsFrameList*>(
                 propTable->Get(this, ExcessOverflowContainersProperty()));
        MOZ_ASSERT(list && list->ContainsFrame(aChild), "aChild isn't our child"
                   " or on a frame list not supported by StealFrame");
      }
    }
  }
#endif

  bool removed;
  if ((aChild->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER)
      && !aForceNormal) {
    FramePropertyTable* propTable = PresContext()->PropertyTable();
    
    removed = ::TryRemoveFrame(this, propTable, OverflowContainersProperty(),
                               aChild);
    if (!removed) {
      
      removed = ::TryRemoveFrame(this, propTable,
                                 ExcessOverflowContainersProperty(),
                                 aChild);
    }
  } else {
    removed = mFrames.StartRemoveFrame(aChild);
    if (!removed) {
      
      
      nsFrameList* frameList = GetOverflowFrames();
      if (frameList) {
        removed = frameList->ContinueRemoveFrame(aChild);
        if (frameList->IsEmpty()) {
          DestroyOverflowList();
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






nsIFrame*
nsContainerFrame::CreateNextInFlow(nsIFrame* aFrame)
{
  NS_PRECONDITION(GetType() != nsGkAtoms::blockFrame,
                  "you should have called nsBlockFrame::CreateContinuationFor instead");
  NS_PRECONDITION(mFrames.ContainsFrame(aFrame), "expected an in-flow child frame");

  nsPresContext* pc = PresContext();
  nsIFrame* nextInFlow = aFrame->GetNextInFlow();
  if (nullptr == nextInFlow) {
    
    
    nextInFlow = pc->PresShell()->FrameConstructor()->
      CreateContinuingFrame(pc, aFrame, this);
    mFrames.InsertFrame(nullptr, aFrame, nextInFlow);

    NS_FRAME_LOG(NS_FRAME_TRACE_NEW_FRAMES,
       ("nsContainerFrame::CreateNextInFlow: frame=%p nextInFlow=%p",
        aFrame, nextInFlow));

    return nextInFlow;
  }
  return nullptr;
}





void
nsContainerFrame::DeleteNextInFlowChild(nsIFrame* aNextInFlow,
                                        bool      aDeletingEmptyFrames)
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
      delFrame->GetParent()->
        DeleteNextInFlowChild(delFrame, aDeletingEmptyFrames);
    }
  }

  
  DebugOnly<nsresult> rv = StealFrame(aNextInFlow);
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
nsContainerFrame::SetOverflowFrames(const nsFrameList& aOverflowFrames)
{
  NS_PRECONDITION(aOverflowFrames.NotEmpty(), "Shouldn't be called");

  nsPresContext* pc = PresContext();
  nsFrameList* newList = new (pc->PresShell()) nsFrameList(aOverflowFrames);

  pc->PropertyTable()->Set(this, OverflowProperty(), newList);
}

nsFrameList*
nsContainerFrame::GetPropTableFrames(const FramePropertyDescriptor* aProperty) const
{
  FramePropertyTable* propTable = PresContext()->PropertyTable();
  return static_cast<nsFrameList*>(propTable->Get(this, aProperty));
}

nsFrameList*
nsContainerFrame::RemovePropTableFrames(const FramePropertyDescriptor* aProperty)
{
  FramePropertyTable* propTable = PresContext()->PropertyTable();
  return static_cast<nsFrameList*>(propTable->Remove(this, aProperty));
}

void
nsContainerFrame::SetPropTableFrames(nsFrameList*                   aFrameList,
                                     const FramePropertyDescriptor* aProperty)
{
  NS_PRECONDITION(aProperty && aFrameList, "null ptr");
  NS_PRECONDITION(
    (aProperty != nsContainerFrame::OverflowContainersProperty() &&
     aProperty != nsContainerFrame::ExcessOverflowContainersProperty()) ||
    IsFrameOfType(nsIFrame::eCanContainOverflowContainers),
    "this type of frame can't have overflow containers");
  MOZ_ASSERT(!GetPropTableFrames(aProperty));
  PresContext()->PropertyTable()->Set(this, aProperty, aFrameList);
}















void
nsContainerFrame::PushChildren(nsIFrame* aFromChild,
                               nsIFrame* aPrevSibling)
{
  NS_PRECONDITION(aFromChild, "null pointer");
  NS_PRECONDITION(aPrevSibling, "pushing first child");
  NS_PRECONDITION(aPrevSibling->GetNextSibling() == aFromChild, "bad prev sibling");

  
  nsFrameList tail = mFrames.RemoveFramesAfter(aPrevSibling);

  nsContainerFrame* nextInFlow =
    static_cast<nsContainerFrame*>(GetNextInFlow());
  if (nextInFlow) {
    
    
    
    
    
    for (nsIFrame* f = aFromChild; f; f = f->GetNextSibling()) {
      nsContainerFrame::ReparentFrameView(f, this, nextInFlow);
    }
    nextInFlow->mFrames.InsertFrames(nextInFlow, nullptr, tail);
  }
  else {
    
    SetOverflowFrames(tail);
  }
}









bool
nsContainerFrame::MoveOverflowToChildList()
{
  bool result = false;

  
  nsContainerFrame* prevInFlow = (nsContainerFrame*)GetPrevInFlow();
  if (nullptr != prevInFlow) {
    AutoFrameListPtr prevOverflowFrames(PresContext(),
                                        prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      
      NS_ASSERTION(mFrames.IsEmpty() || GetType() == nsGkAtoms::tableFrame,
                   "bad overflow list");
      
      
      nsContainerFrame::ReparentFrameViewList(*prevOverflowFrames,
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
  AutoFrameListPtr overflowFrames(PresContext(), StealOverflowFrames());
  if (overflowFrames) {
    mFrames.AppendFrames(nullptr, *overflowFrames);
    return true;
  }
  return false;
}

nsIFrame*
nsContainerFrame::GetNextInFlowChild(ContinuationTraversingState& aState,
                                     bool* aIsInOverflow)
{
  nsContainerFrame*& nextInFlow = aState.mNextInFlow;
  while (nextInFlow) {
    
    nsIFrame* frame = nextInFlow->mFrames.FirstChild();
    if (frame) {
      if (aIsInOverflow) {
        *aIsInOverflow = false;
      }
      return frame;
    }
    
    nsFrameList* overflowFrames = nextInFlow->GetOverflowFrames();
    if (overflowFrames) {
      if (aIsInOverflow) {
        *aIsInOverflow = true;
      }
      return overflowFrames->FirstChild();
    }
    nextInFlow = static_cast<nsContainerFrame*>(nextInFlow->GetNextInFlow());
  }
  return nullptr;
}

nsIFrame*
nsContainerFrame::PullNextInFlowChild(ContinuationTraversingState& aState)
{
  bool isInOverflow;
  nsIFrame* frame = GetNextInFlowChild(aState, &isInOverflow);
  if (frame) {
    nsContainerFrame* nextInFlow = aState.mNextInFlow;
    if (isInOverflow) {
      nsFrameList* overflowFrames = nextInFlow->GetOverflowFrames();
      overflowFrames->RemoveFirstChild();
      if (overflowFrames->IsEmpty()) {
        nextInFlow->DestroyOverflowList();
      }
    } else {
      nextInFlow->mFrames.RemoveFirstChild();
    }

    
    mFrames.AppendFrame(this, frame);
    
    
    nsContainerFrame::ReparentFrameView(frame, nextInFlow, this);
  }
  return frame;
}

bool
nsContainerFrame::ResolvedOrientationIsVertical()
{
  uint8_t orient = StyleDisplay()->mOrient;
  switch (orient) {
    case NS_STYLE_ORIENT_HORIZONTAL:
      return false;
    case NS_STYLE_ORIENT_VERTICAL:
      return true;
    case NS_STYLE_ORIENT_INLINE:
      return GetWritingMode().IsVertical();
    case NS_STYLE_ORIENT_BLOCK:
      return !GetWritingMode().IsVertical();
  }
  NS_NOTREACHED("unexpected -moz-orient value");
  return false;
}

nsOverflowContinuationTracker::nsOverflowContinuationTracker(nsContainerFrame* aFrame,
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
  SetupOverflowContList();
}

void
nsOverflowContinuationTracker::SetupOverflowContList()
{
  NS_PRECONDITION(mParent, "null frame pointer");
  NS_PRECONDITION(!mOverflowContList, "already have list");
  nsContainerFrame* nif =
    static_cast<nsContainerFrame*>(mParent->GetNextInFlow());
  if (nif) {
    mOverflowContList = nif->GetPropTableFrames(
      nsContainerFrame::OverflowContainersProperty());
    if (mOverflowContList) {
      mParent = nif;
      SetUpListWalker();
    }
  }
  if (!mOverflowContList) {
    mOverflowContList = mParent->GetPropTableFrames(
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
  bool addToList = !mSentry || aOverflowCont != mSentry->GetNextInFlow();

  
  
  if (addToList && aOverflowCont->GetParent() == mParent &&
      (aOverflowCont->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) &&
      mOverflowContList && mOverflowContList->ContainsFrame(aOverflowCont)) {
    addToList = false;
    mPrevOverflowCont = aOverflowCont->GetPrevSibling();
  }

  if (addToList) {
    if (aOverflowCont->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
      
      
      NS_ASSERTION(!(mOverflowContList &&
                     mOverflowContList->ContainsFrame(aOverflowCont)),
                   "overflow containers out of order");
      rv = aOverflowCont->GetParent()->StealFrame(aOverflowCont);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      aOverflowCont->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
    }
    if (!mOverflowContList) {
      mOverflowContList = new (presContext->PresShell()) nsFrameList();
      mParent->SetPropTableFrames(mOverflowContList,
        nsContainerFrame::ExcessOverflowContainersProperty());
      SetUpListWalker();
    }
    if (aOverflowCont->GetParent() != mParent) {
      nsContainerFrame::ReparentFrameView(aOverflowCont,
                                          aOverflowCont->GetParent(),
                                          mParent);
      reparented = true;
    }

    
    
    
    nsIFrame* pif = aOverflowCont->GetPrevInFlow();
    nsIFrame* nif = aOverflowCont->GetNextInFlow();
    if ((pif && pif->GetParent() == mParent && pif != mPrevOverflowCont) ||
        (nif && nif->GetParent() == mParent && mPrevOverflowCont)) {
      for (nsFrameList::Enumerator e(*mOverflowContList); !e.AtEnd(); e.Next()) {
        nsIFrame* f = e.get();
        if (f == pif) {
          mPrevOverflowCont = pif;
          break;
        }
        if (f == nif) {
          mPrevOverflowCont = f->GetPrevSibling();
          break;
        }
      }
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
        rv = f->GetParent()->StealFrame(f);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      Insert(f, aReflowStatus);
    }
  }
  return rv;
}

void
nsOverflowContinuationTracker::BeginFinish(nsIFrame* aChild)
{
  NS_PRECONDITION(aChild, "null ptr");
  NS_PRECONDITION(aChild->GetNextInFlow(),
                  "supposed to call Finish *before* deleting next-in-flow!");
  for (nsIFrame* f = aChild; f; f = f->GetNextInFlow()) {
    
    if (f == mPrevOverflowCont) {
      mSentry = nullptr;
      mPrevOverflowCont = nullptr;
      break;
    }
    if (f == mSentry) {
      mSentry = nullptr;
      break;
    }
  }
}

void
nsOverflowContinuationTracker::EndFinish(nsIFrame* aChild)
{
  if (!mOverflowContList) {
    return;
  }
  
  nsPresContext* pc = aChild->PresContext();
  FramePropertyTable* propTable = pc->PropertyTable();
  nsFrameList* eoc = static_cast<nsFrameList*>(propTable->Get(mParent,
                       nsContainerFrame::ExcessOverflowContainersProperty()));
  if (eoc != mOverflowContList) {
    nsFrameList* oc = static_cast<nsFrameList*>(propTable->Get(mParent,
                        nsContainerFrame::OverflowContainersProperty()));
    if (oc != mOverflowContList) {
      
      mPrevOverflowCont = nullptr;
      mSentry = nullptr;
      mParent = aChild->GetParent();
      mOverflowContList = nullptr;
      SetupOverflowContList();
      return;
    }
  }
  
  if (!mSentry) {
    if (!mPrevOverflowCont) {
      SetUpListWalker();
    } else {
      mozilla::AutoRestore<nsIFrame*> saved(mPrevOverflowCont);
      
      mPrevOverflowCont = mPrevOverflowCont->GetPrevSibling();
      StepForward();
    }
  }
}




#ifdef DEBUG_FRAME_DUMP
void
nsContainerFrame::List(FILE* out, const char* aPrefix, uint32_t aFlags) const
{
  nsCString str;
  ListGeneric(str, aPrefix, aFlags);

  
  bool outputOneList = false;
  ChildListIterator lists(this);
  for (; !lists.IsDone(); lists.Next()) {
    if (outputOneList) {
      str += aPrefix;
    }
    if (lists.CurrentID() != kPrincipalList) {
      if (!outputOneList) {
        str += "\n";
        str += aPrefix;
      }
      str += nsPrintfCString("%s %p ", mozilla::layout::ChildListName(lists.CurrentID()),
                             &GetChildList(lists.CurrentID()));
    }
    fprintf_stderr(out, "%s<\n", str.get());
    str = "";
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsIFrame* kid = childFrames.get();
      
      NS_ASSERTION(kid->GetParent() == this, "bad parent frame pointer");

      
      nsCString pfx(aPrefix);
      pfx += "  ";
      kid->List(out, pfx.get(), aFlags);
    }
    fprintf_stderr(out, "%s>\n", aPrefix);
    outputOneList = true;
  }

  if (!outputOneList) {
    fprintf_stderr(out, "%s<>\n", str.get());
  }
}
#endif
