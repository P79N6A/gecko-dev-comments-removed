











































#include "nsDisplayList.h"

#include "nsCSSRendering.h"
#include "nsISelectionController.h"
#include "nsIPresShell.h"
#include "nsRegion.h"
#include "nsFrameManager.h"
#include "gfxContext.h"
#include "nsStyleStructInlines.h"
#include "nsStyleTransformMatrix.h"
#include "gfxMatrix.h"
#ifdef MOZ_SVG
#include "nsSVGIntegrationUtils.h"
#endif
#include "nsLayoutUtils.h"
#include "nsIScrollableFrame.h"
#include "nsThemeConstants.h"

#include "imgIContainer.h"
#include "nsIInterfaceRequestorUtils.h"
#include "BasicLayers.h"

using namespace mozilla;
using namespace mozilla::layers;

nsDisplayListBuilder::nsDisplayListBuilder(nsIFrame* aReferenceFrame,
    PRBool aIsForEvents, PRBool aBuildCaret)
    : mReferenceFrame(aReferenceFrame),
      mIgnoreScrollFrame(nsnull),
      mCurrentTableItem(nsnull),
      mBuildCaret(aBuildCaret),
      mEventDelivery(aIsForEvents),
      mIgnoreSuppression(PR_FALSE),
      mHadToIgnoreSuppression(PR_FALSE),
      mIsAtRootOfPseudoStackingContext(PR_FALSE),
      mSelectedFramesOnly(PR_FALSE),
      mAccurateVisibleRegions(PR_FALSE),
      mInTransform(PR_FALSE),
      mSyncDecodeImages(PR_FALSE),
      mIsPaintingToWindow(PR_FALSE) {
  MOZ_COUNT_CTOR(nsDisplayListBuilder);
  PL_InitArenaPool(&mPool, "displayListArena", 1024,
                   NS_MAX(NS_ALIGNMENT_OF(void*),NS_ALIGNMENT_OF(double))-1);

  nsPresContext* pc = aReferenceFrame->PresContext();
  nsIPresShell *shell = pc->PresShell();
  if (pc->IsRenderingOnlySelection()) {
    nsCOMPtr<nsISelectionController> selcon(do_QueryInterface(shell));
    if (selcon) {
      selcon->GetSelection(nsISelectionController::SELECTION_NORMAL,
                           getter_AddRefs(mBoundingSelection));
    }
  }

  LayerBuilder()->Init(this);

  PR_STATIC_ASSERT(nsDisplayItem::TYPE_MAX < (1 << nsDisplayItem::TYPE_BITS));
}

static void MarkFrameForDisplay(nsIFrame* aFrame, nsIFrame* aStopAtFrame) {
  nsFrameManager* frameManager = aFrame->PresContext()->PresShell()->FrameManager();

  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetParentOrPlaceholderFor(frameManager, f)) {
    if (f->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO)
      return;
    f->AddStateBits(NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO);
    if (f == aStopAtFrame) {
      
      break;
    }
  }
}

static void MarkOutOfFlowFrameForDisplay(nsIFrame* aDirtyFrame, nsIFrame* aFrame,
                                         const nsRect& aDirtyRect) {
  nsRect dirty = aDirtyRect - aFrame->GetOffsetTo(aDirtyFrame);
  nsRect overflowRect = aFrame->GetVisualOverflowRect();
  if (!dirty.IntersectRect(dirty, overflowRect))
    return;
  aFrame->Properties().Set(nsDisplayListBuilder::OutOfFlowDirtyRectProperty(),
                           new nsRect(dirty));

  MarkFrameForDisplay(aFrame, aDirtyFrame);
}

static void UnmarkFrameForDisplay(nsIFrame* aFrame) {
  nsPresContext* presContext = aFrame->PresContext();
  presContext->PropertyTable()->
    Delete(aFrame, nsDisplayListBuilder::OutOfFlowDirtyRectProperty());

  nsFrameManager* frameManager = presContext->PresShell()->FrameManager();

  for (nsIFrame* f = aFrame; f;
       f = nsLayoutUtils::GetParentOrPlaceholderFor(frameManager, f)) {
    if (!(f->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO))
      return;
    f->RemoveStateBits(NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO);
  }
}

nsDisplayListBuilder::~nsDisplayListBuilder() {
  NS_ASSERTION(mFramesMarkedForDisplay.Length() == 0,
               "All frames should have been unmarked");
  NS_ASSERTION(mPresShellStates.Length() == 0,
               "All presshells should have been exited");
  NS_ASSERTION(!mCurrentTableItem, "No table item should be active");

  PL_FreeArenaPool(&mPool);
  PL_FinishArenaPool(&mPool);
  MOZ_COUNT_DTOR(nsDisplayListBuilder);
}

PRUint32
nsDisplayListBuilder::GetBackgroundPaintFlags() {
  PRUint32 flags = 0;
  if (mSyncDecodeImages) {
    flags |= nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES;
  }
  return flags;
}

void
nsDisplayListBuilder::SubtractFromVisibleRegion(nsRegion* aVisibleRegion,
                                                const nsRegion& aRegion)
{
  nsRegion tmp;
  tmp.Sub(*aVisibleRegion, aRegion);
  
  
  if (GetAccurateVisibleRegions() || tmp.GetNumRects() <= 15) {
    *aVisibleRegion = tmp;
  }
}

nsCaret *
nsDisplayListBuilder::GetCaret() {
  nsRefPtr<nsCaret> caret = CurrentPresShellState()->mPresShell->GetCaret();
  return caret;
}

void
nsDisplayListBuilder::EnterPresShell(nsIFrame* aReferenceFrame,
                                     const nsRect& aDirtyRect) {
  PresShellState* state = mPresShellStates.AppendElement();
  if (!state)
    return;
  state->mPresShell = aReferenceFrame->PresContext()->PresShell();
  state->mCaretFrame = nsnull;
  state->mFirstFrameMarkedForDisplay = mFramesMarkedForDisplay.Length();

  state->mPresShell->UpdateCanvasBackground();

  if (mIsPaintingToWindow) {
    state->mPresShell->IncrementPaintCount();
  }

  PRBool buildCaret = mBuildCaret;
  if (mIgnoreSuppression || !state->mPresShell->IsPaintingSuppressed()) {
    if (state->mPresShell->IsPaintingSuppressed()) {
      mHadToIgnoreSuppression = PR_TRUE;
    }
    state->mIsBackgroundOnly = PR_FALSE;
  } else {
    state->mIsBackgroundOnly = PR_TRUE;
    buildCaret = PR_FALSE;
  }

  if (!buildCaret)
    return;

  nsRefPtr<nsCaret> caret = state->mPresShell->GetCaret();
  state->mCaretFrame = caret->GetCaretFrame();

  if (state->mCaretFrame) {
    
    nsRect caretRect =
      caret->GetCaretRect() + state->mCaretFrame->GetOffsetTo(aReferenceFrame);
    if (caretRect.Intersects(aDirtyRect)) {
      
      mFramesMarkedForDisplay.AppendElement(state->mCaretFrame);
      MarkFrameForDisplay(state->mCaretFrame, nsnull);
    }
  }
}

void
nsDisplayListBuilder::LeavePresShell(nsIFrame* aReferenceFrame,
                                     const nsRect& aDirtyRect) {
  if (CurrentPresShellState()->mPresShell != aReferenceFrame->PresContext()->PresShell()) {
    
    
    return;
  }

  
  PRUint32 firstFrameForShell = CurrentPresShellState()->mFirstFrameMarkedForDisplay;
  for (PRUint32 i = firstFrameForShell;
       i < mFramesMarkedForDisplay.Length(); ++i) {
    UnmarkFrameForDisplay(mFramesMarkedForDisplay[i]);
  }
  mFramesMarkedForDisplay.SetLength(firstFrameForShell);
  mPresShellStates.SetLength(mPresShellStates.Length() - 1);
}

void
nsDisplayListBuilder::MarkFramesForDisplayList(nsIFrame* aDirtyFrame,
                                               const nsFrameList& aFrames,
                                               const nsRect& aDirtyRect) {
  for (nsFrameList::Enumerator e(aFrames); !e.AtEnd(); e.Next()) {
    mFramesMarkedForDisplay.AppendElement(e.get());
    MarkOutOfFlowFrameForDisplay(aDirtyFrame, e.get(), aDirtyRect);
  }
}

void*
nsDisplayListBuilder::Allocate(size_t aSize) {
  void *tmp;
  PL_ARENA_ALLOCATE(tmp, &mPool, aSize);
  return tmp;
}

void nsDisplayListSet::MoveTo(const nsDisplayListSet& aDestination) const
{
  aDestination.BorderBackground()->AppendToTop(BorderBackground());
  aDestination.BlockBorderBackgrounds()->AppendToTop(BlockBorderBackgrounds());
  aDestination.Floats()->AppendToTop(Floats());
  aDestination.Content()->AppendToTop(Content());
  aDestination.PositionedDescendants()->AppendToTop(PositionedDescendants());
  aDestination.Outlines()->AppendToTop(Outlines());
}

void
nsDisplayList::FlattenTo(nsTArray<nsDisplayItem*>* aElements) {
  nsDisplayItem* item;
  while ((item = RemoveBottom()) != nsnull) {
    if (item->GetType() == nsDisplayItem::TYPE_WRAP_LIST) {
      item->GetList()->FlattenTo(aElements);
      item->~nsDisplayItem();
    } else {
      aElements->AppendElement(item);
    }
  }
}

nsRect
nsDisplayList::GetBounds(nsDisplayListBuilder* aBuilder) const {
  nsRect bounds;
  for (nsDisplayItem* i = GetBottom(); i != nsnull; i = i->GetAbove()) {
    bounds.UnionRect(bounds, i->GetBounds(aBuilder));
  }
  return bounds;
}

PRBool
nsDisplayList::ComputeVisibilityForRoot(nsDisplayListBuilder* aBuilder,
                                        nsRegion* aVisibleRegion) {
  nsRegion r;
  r.And(*aVisibleRegion, GetBounds(aBuilder));
  return ComputeVisibilityForSublist(aBuilder, aVisibleRegion, r.GetBounds());
}

PRBool
nsDisplayList::ComputeVisibilityForSublist(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion,
                                           const nsRect& aListVisibleBounds) {
#ifdef DEBUG
  nsRegion r;
  r.And(*aVisibleRegion, GetBounds(aBuilder));
  NS_ASSERTION(r.GetBounds() == aListVisibleBounds,
               "bad aListVisibleBounds");
#endif
  mVisibleRect = aListVisibleBounds;
  PRBool anyVisible = PR_FALSE;

  nsAutoTArray<nsDisplayItem*, 512> elements;
  FlattenTo(&elements);

  for (PRInt32 i = elements.Length() - 1; i >= 0; --i) {
    nsDisplayItem* item = elements[i];
    nsDisplayItem* belowItem = i < 1 ? nsnull : elements[i - 1];

    if (belowItem && item->TryMerge(aBuilder, belowItem)) {
      belowItem->~nsDisplayItem();
      elements.ReplaceElementsAt(i - 1, 1, item);
      continue;
    }

    nsRect bounds = item->GetBounds(aBuilder);

    nsRegion itemVisible;
    itemVisible.And(*aVisibleRegion, bounds);
    item->mVisibleRect = itemVisible.GetBounds();

    if (item->ComputeVisibility(aBuilder, aVisibleRegion)) {
      anyVisible = PR_TRUE;
      nsIFrame* f = item->GetUnderlyingFrame();
      if (item->IsOpaque(aBuilder) && f) {
        
        aBuilder->SubtractFromVisibleRegion(aVisibleRegion, nsRegion(bounds));
      }
    }
    AppendToBottom(item);
  }

  mIsOpaque = !aVisibleRegion->Intersects(mVisibleRect);
#ifdef DEBUG
  mDidComputeVisibility = PR_TRUE;
#endif
  return anyVisible;
}

void nsDisplayList::PaintRoot(nsDisplayListBuilder* aBuilder,
                              nsIRenderingContext* aCtx,
                              PRUint32 aFlags) const {
  PaintForFrame(aBuilder, aCtx, aBuilder->ReferenceFrame(), aFlags);
}






void nsDisplayList::PaintForFrame(nsDisplayListBuilder* aBuilder,
                                  nsIRenderingContext* aCtx,
                                  nsIFrame* aForFrame,
                                  PRUint32 aFlags) const {
  NS_ASSERTION(mDidComputeVisibility,
               "Must call ComputeVisibility before calling Paint");

  nsRefPtr<LayerManager> layerManager;
  if (aFlags & PAINT_USE_WIDGET_LAYERS) {
    nsIFrame* referenceFrame = aBuilder->ReferenceFrame();
    NS_ASSERTION(referenceFrame == nsLayoutUtils::GetDisplayRootFrame(referenceFrame),
                 "Reference frame must be a display root for us to use the layer manager");
    nsIWidget* window = referenceFrame->GetNearestWidget();
    if (window) {
      layerManager = window->GetLayerManager();
      if (layerManager) {
        aBuilder->LayerBuilder()->WillBeginRetainedLayerTransaction(layerManager);
      }
    }
  }
  if (!layerManager) {
    if (!aCtx) {
      NS_WARNING("Nowhere to paint into");
      return;
    }
    layerManager = new BasicLayerManager();
    if (!layerManager)
      return;
  }

  if (aFlags & PAINT_FLUSH_LAYERS) {
    FrameLayerBuilder::InvalidateAllLayers(layerManager);
  }

  if (aCtx) {
    layerManager->BeginTransactionWithTarget(aCtx->ThebesContext());
  } else {
    layerManager->BeginTransaction();
  }

  nsRefPtr<ContainerLayer> root = aBuilder->LayerBuilder()->
    BuildContainerLayerFor(aBuilder, layerManager, aForFrame, nsnull, *this);
  if (!root)
    return;

  nsPresContext* presContext = aForFrame->PresContext();
  nsIPresShell* presShell = presContext->GetPresShell();

  nsIntRect visible = mVisibleRect.ToNearestPixels(presContext->AppUnitsPerDevPixel());
  root->SetVisibleRegion(nsIntRegion(visible));

  
  FrameMetrics metrics;

  PRInt32 auPerCSSPixel = nsPresContext::AppUnitsPerCSSPixel();
  metrics.mViewportSize =
    presContext->GetVisibleArea().ToNearestPixels(auPerCSSPixel).Size();
  if (presShell->UsingDisplayPort()) {
    metrics.mDisplayPort =
      presShell->GetDisplayPort().ToNearestPixels(auPerCSSPixel);
  }

  nsIScrollableFrame* rootScrollableFrame =
    presShell->GetRootScrollFrameAsScrollable();
  if (rootScrollableFrame) {
    metrics.mViewportScrollOffset =
      rootScrollableFrame->GetScrollPosition().ToNearestPixels(auPerCSSPixel);
  }

  root->SetFrameMetrics(metrics);

  
  if (LayerManager::LAYERS_BASIC == layerManager->GetBackendType()) {
    BasicLayerManager* basicManager =
      static_cast<BasicLayerManager*>(layerManager.get());
    
    
    basicManager->SetResolution(presShell->GetXResolution(),
                                presShell->GetYResolution());
  }

  layerManager->SetRoot(root);
  aBuilder->LayerBuilder()->WillEndTransaction(layerManager);
  layerManager->EndTransaction(FrameLayerBuilder::DrawThebesLayer,
                               aBuilder);
  aBuilder->LayerBuilder()->DidEndTransaction(layerManager);

  if (aFlags & PAINT_FLUSH_LAYERS) {
    FrameLayerBuilder::InvalidateAllLayers(layerManager);
  }

  nsCSSRendering::DidPaint();
}

PRUint32 nsDisplayList::Count() const {
  PRUint32 count = 0;
  for (nsDisplayItem* i = GetBottom(); i; i = i->GetAbove()) {
    ++count;
  }
  return count;
}

nsDisplayItem* nsDisplayList::RemoveBottom() {
  nsDisplayItem* item = mSentinel.mAbove;
  if (!item)
    return nsnull;
  mSentinel.mAbove = item->mAbove;
  if (item == mTop) {
    
    mTop = &mSentinel;
  }
  item->mAbove = nsnull;
  return item;
}

void nsDisplayList::DeleteAll() {
  nsDisplayItem* item;
  while ((item = RemoveBottom()) != nsnull) {
    item->~nsDisplayItem();
  }
}

void nsDisplayList::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                            nsDisplayItem::HitTestState* aState,
                            nsTArray<nsIFrame*> *aOutFrames) const {
  PRInt32 itemBufferStart = aState->mItemBuffer.Length();
  nsDisplayItem* item;
  for (item = GetBottom(); item; item = item->GetAbove()) {
    aState->mItemBuffer.AppendElement(item);
  }
  for (PRInt32 i = aState->mItemBuffer.Length() - 1; i >= itemBufferStart; --i) {
    
    
    item = aState->mItemBuffer[i];
    aState->mItemBuffer.SetLength(i);

    if (aRect.Intersects(item->GetBounds(aBuilder))) {
      nsTArray<nsIFrame*> outFrames;
      item->HitTest(aBuilder, aRect, aState, &outFrames);

      for (PRUint32 j = 0; j < outFrames.Length(); j++) {
        nsIFrame *f = outFrames.ElementAt(j);
        
        if (!f->GetMouseThrough() &&
            f->GetStyleVisibility()->mPointerEvents != NS_STYLE_POINTER_EVENTS_NONE) {
          aOutFrames->AppendElement(f);
        }
      }

    }
  }
  NS_ASSERTION(aState->mItemBuffer.Length() == PRUint32(itemBufferStart),
               "How did we forget to pop some elements?");
}

static void Sort(nsDisplayList* aList, PRInt32 aCount, nsDisplayList::SortLEQ aCmp,
                 void* aClosure) {
  if (aCount < 2)
    return;

  nsDisplayList list1;
  nsDisplayList list2;
  int i;
  PRInt32 half = aCount/2;
  PRBool sorted = PR_TRUE;
  nsDisplayItem* prev = nsnull;
  for (i = 0; i < aCount; ++i) {
    nsDisplayItem* item = aList->RemoveBottom();
    (i < half ? &list1 : &list2)->AppendToTop(item);
    if (sorted && prev && !aCmp(prev, item, aClosure)) {
      sorted = PR_FALSE;
    }
    prev = item;
  }
  if (sorted) {
    aList->AppendToTop(&list1);
    aList->AppendToTop(&list2);
    return;
  }
  
  Sort(&list1, half, aCmp, aClosure);
  Sort(&list2, aCount - half, aCmp, aClosure);

  for (i = 0; i < aCount; ++i) {
    if (list1.GetBottom() &&
        (!list2.GetBottom() ||
         aCmp(list1.GetBottom(), list2.GetBottom(), aClosure))) {
      aList->AppendToTop(list1.RemoveBottom());
    } else {
      aList->AppendToTop(list2.RemoveBottom());
    }
  }
}

static PRBool IsContentLEQ(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                           void* aClosure) {
  
  
  return nsLayoutUtils::CompareTreePosition(
      aItem1->GetUnderlyingFrame()->GetContent(),
      aItem2->GetUnderlyingFrame()->GetContent(),
      static_cast<nsIContent*>(aClosure)) <= 0;
}

static PRBool IsZOrderLEQ(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                          void* aClosure) {
  
  
  
  PRInt32 index1 = nsLayoutUtils::GetZIndex(aItem1->GetUnderlyingFrame());
  PRInt32 index2 = nsLayoutUtils::GetZIndex(aItem2->GetUnderlyingFrame());
  if (index1 == index2)
    return IsContentLEQ(aItem1, aItem2, aClosure);
  return index1 < index2;
}

void nsDisplayList::ExplodeAnonymousChildLists(nsDisplayListBuilder* aBuilder) {
  
  PRBool anyAnonymousItems = PR_FALSE;
  nsDisplayItem* i;
  for (i = GetBottom(); i != nsnull; i = i->GetAbove()) {
    if (!i->GetUnderlyingFrame()) {
      anyAnonymousItems = PR_TRUE;
      break;
    }
  }
  if (!anyAnonymousItems)
    return;

  nsDisplayList tmp;
  while ((i = RemoveBottom()) != nsnull) {
    if (i->GetUnderlyingFrame()) {
      tmp.AppendToTop(i);
    } else {
      nsDisplayList* list = i->GetList();
      NS_ASSERTION(list, "leaf items can't be anonymous");
      list->ExplodeAnonymousChildLists(aBuilder);
      nsDisplayItem* j;
      while ((j = list->RemoveBottom()) != nsnull) {
        tmp.AppendToTop(static_cast<nsDisplayWrapList*>(i)->
            WrapWithClone(aBuilder, j));
      }
      i->~nsDisplayItem();
    }
  }
  
  AppendToTop(&tmp);
}

void nsDisplayList::SortByZOrder(nsDisplayListBuilder* aBuilder,
                                 nsIContent* aCommonAncestor) {
  Sort(aBuilder, IsZOrderLEQ, aCommonAncestor);
}

void nsDisplayList::SortByContentOrder(nsDisplayListBuilder* aBuilder,
                                       nsIContent* aCommonAncestor) {
  Sort(aBuilder, IsContentLEQ, aCommonAncestor);
}

void nsDisplayList::Sort(nsDisplayListBuilder* aBuilder,
                         SortLEQ aCmp, void* aClosure) {
  ExplodeAnonymousChildLists(aBuilder);
  ::Sort(this, Count(), aCmp, aClosure);
}

PRBool nsDisplayItem::RecomputeVisibility(nsDisplayListBuilder* aBuilder,
                                          nsRegion* aVisibleRegion) {
  nsRect bounds = GetBounds(aBuilder);

  nsRegion itemVisible;
  itemVisible.And(*aVisibleRegion, bounds);
  mVisibleRect = itemVisible.GetBounds();

  if (!ComputeVisibility(aBuilder, aVisibleRegion))
    return PR_FALSE;

  if (IsOpaque(aBuilder)) {
    aVisibleRegion->Sub(*aVisibleRegion, bounds);
  }
  return PR_TRUE;
}

void nsDisplaySolidColor::Paint(nsDisplayListBuilder* aBuilder,
                                nsIRenderingContext* aCtx) {
  aCtx->SetColor(mColor);
  aCtx->FillRect(mVisibleRect);
}

static void
RegisterThemeWidgetGeometry(nsIFrame* aFrame)
{
  nsPresContext* presContext = aFrame->PresContext();
  nsITheme* theme = presContext->GetTheme();
  if (!theme)
    return;

  nsIFrame* displayRoot = nsLayoutUtils::GetDisplayRootFrame(aFrame);
  nsIWidget* widget = displayRoot->GetNearestWidget();
  
  
  if (!widget)
    return;

  for (nsIFrame* f = aFrame; f; f = f->GetParent()) {
    
    if (f->IsTransformed())
      return;
    
    if (!f->GetParent() && f != displayRoot)
      return;
  }

  nsRect borderBox(aFrame->GetOffsetTo(displayRoot), aFrame->GetSize());
  theme->RegisterWidgetGeometry(widget,
      aFrame->GetStyleDisplay()->mAppearance,
      borderBox.ToNearestPixels(presContext->AppUnitsPerDevPixel()));
}

nsDisplayBackground::nsDisplayBackground(nsDisplayListBuilder* aBuilder,
                                         nsIFrame* aFrame)
  : nsDisplayItem(aBuilder, aFrame)
{
  MOZ_COUNT_CTOR(nsDisplayBackground);
  const nsStyleDisplay* disp = mFrame->GetStyleDisplay();
  mIsThemed = mFrame->IsThemed(disp, &mThemeTransparency);

  
  if (mIsThemed &&
      (disp->mAppearance == NS_THEME_MOZ_MAC_UNIFIED_TOOLBAR ||
       disp->mAppearance == NS_THEME_TOOLBAR)) {
    RegisterThemeWidgetGeometry(aFrame);
  }
}


static PRBool
CheckCorner(nscoord aXOffset, nscoord aYOffset,
            nscoord aXRadius, nscoord aYRadius)
{
  NS_ABORT_IF_FALSE(aXOffset > 0 && aYOffset > 0,
                    "must not pass nonpositives to CheckCorner");
  NS_ABORT_IF_FALSE(aXRadius >= 0 && aYRadius >= 0,
                    "must not pass negatives to CheckCorner");

  
  
  
  if (aXOffset >= aXRadius || aYOffset >= aYRadius)
    return PR_TRUE;

  
  
  float scaledX = float(aXRadius - aXOffset) / float(aXRadius);
  float scaledY = float(aYRadius - aYOffset) / float(aYRadius);
  return scaledX * scaledX + scaledY * scaledY < 1.0f;
}









static PRBool
RoundedRectIntersectsRect(const nsRect& aRoundedRect, nscoord aRadii[8],
                          const nsRect& aTestRect)
{
  NS_ABORT_IF_FALSE(aTestRect.Intersects(aRoundedRect),
                    "we should already have tested basic rect intersection");

  
  
  nsMargin insets;
  insets.top = aTestRect.YMost() - aRoundedRect.y;
  insets.right = aRoundedRect.XMost() - aTestRect.x;
  insets.bottom = aRoundedRect.YMost() - aTestRect.y;
  insets.left = aTestRect.XMost() - aRoundedRect.x;

  
  
  
  return CheckCorner(insets.left, insets.top,
                     aRadii[NS_CORNER_TOP_LEFT_X],
                     aRadii[NS_CORNER_TOP_LEFT_Y]) &&
         CheckCorner(insets.right, insets.top,
                     aRadii[NS_CORNER_TOP_RIGHT_X],
                     aRadii[NS_CORNER_TOP_RIGHT_Y]) &&
         CheckCorner(insets.right, insets.bottom,
                     aRadii[NS_CORNER_BOTTOM_RIGHT_X],
                     aRadii[NS_CORNER_BOTTOM_RIGHT_Y]) &&
         CheckCorner(insets.left, insets.bottom,
                     aRadii[NS_CORNER_BOTTOM_LEFT_X],
                     aRadii[NS_CORNER_BOTTOM_LEFT_Y]);
}




static PRBool
RoundedBorderIntersectsRect(nsIFrame* aFrame,
                            const nsPoint& aFrameToReferenceFrame,
                            const nsRect& aTestRect)
{
  NS_ABORT_IF_FALSE(nsRect(aFrameToReferenceFrame,
                           aFrame->GetSize()).Intersects(aTestRect),
                    "must intersect non-rounded rect");
  nscoord radii[8];
  return !aFrame->GetBorderRadii(radii) ||
         RoundedRectIntersectsRect(nsRect(aFrameToReferenceFrame,
                                          aFrame->GetSize()),
                                   radii, aTestRect);
}







static PRBool RoundedRectContainsRect(const nsRect& aRoundedRect,
                                      const nscoord aRadii[8],
                                      const nsRect& aContainedRect) {
  
  
  nsRect rectFullHeight = aRoundedRect;
  nscoord xDiff = NS_MAX(aRadii[NS_CORNER_TOP_LEFT_X], aRadii[NS_CORNER_BOTTOM_LEFT_X]);
  rectFullHeight.x += xDiff;
  rectFullHeight.width -= NS_MAX(aRadii[NS_CORNER_TOP_RIGHT_X],
                                 aRadii[NS_CORNER_BOTTOM_RIGHT_X]) + xDiff;
  if (rectFullHeight.Contains(aContainedRect))
    return PR_TRUE;

  nsRect rectFullWidth = aRoundedRect;
  nscoord yDiff = NS_MAX(aRadii[NS_CORNER_TOP_LEFT_Y], aRadii[NS_CORNER_TOP_RIGHT_Y]);
  rectFullWidth.y += yDiff;
  rectFullWidth.height -= NS_MAX(aRadii[NS_CORNER_BOTTOM_LEFT_Y],
                                 aRadii[NS_CORNER_BOTTOM_RIGHT_Y]) + yDiff;
  if (rectFullWidth.Contains(aContainedRect))
    return PR_TRUE;

  return PR_FALSE;
}

void
nsDisplayBackground::HitTest(nsDisplayListBuilder* aBuilder,
                             const nsRect& aRect,
                             HitTestState* aState,
                             nsTArray<nsIFrame*> *aOutFrames)
{
  
  
  
  if (!mIsThemed &&
      !RoundedBorderIntersectsRect(mFrame, ToReferenceFrame(), aRect)) {
    
    return;
  }

  aOutFrames->AppendElement(mFrame);
}

PRBool
nsDisplayBackground::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                       nsRegion* aVisibleRegion)
{
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion))
    return PR_FALSE;

  
  
  
  nsStyleContext* bgSC;
  return mIsThemed ||
    nsCSSRendering::FindBackground(mFrame->PresContext(), mFrame, &bgSC);
}

PRBool
nsDisplayBackground::IsOpaque(nsDisplayListBuilder* aBuilder) {
  
  if (mIsThemed)
    return mThemeTransparency == nsITheme::eOpaque;

  nsStyleContext* bgSC;
  if (!nsCSSRendering::FindBackground(mFrame->PresContext(), mFrame, &bgSC))
    return PR_FALSE;
  const nsStyleBackground* bg = bgSC->GetStyleBackground();

  const nsStyleBackground::Layer& bottomLayer = bg->BottomLayer();

  
  if (bottomLayer.mClip != NS_STYLE_BG_CLIP_BORDER ||
      nsLayoutUtils::HasNonZeroCorner(mFrame->GetStyleBorder()->mBorderRadius))
    return PR_FALSE;

  if (NS_GET_A(bg->mBackgroundColor) == 255 &&
      !nsCSSRendering::IsCanvasFrame(mFrame))
    return PR_TRUE;

  return bottomLayer.mRepeat == NS_STYLE_BG_REPEAT_XY &&
         bottomLayer.mImage.IsOpaque();
}

PRBool
nsDisplayBackground::IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) {
  
  if (mIsThemed)
    return PR_FALSE;

  nsStyleContext *bgSC;
  PRBool hasBG =
    nsCSSRendering::FindBackground(mFrame->PresContext(), mFrame, &bgSC);
  if (!hasBG) {
    *aColor = NS_RGBA(0,0,0,0);
    return PR_TRUE;
  }
  const nsStyleBackground* bg = bgSC->GetStyleBackground();
  if (bg->BottomLayer().mImage.IsEmpty() &&
      bg->mImageCount == 1 &&
      !nsLayoutUtils::HasNonZeroCorner(mFrame->GetStyleBorder()->mBorderRadius) &&
      bg->BottomLayer().mClip == NS_STYLE_BG_CLIP_BORDER) {
    
    
    
    *aColor = nsCSSRendering::IsCanvasFrame(mFrame) ? NS_RGBA(0,0,0,0)
        : bg->mBackgroundColor;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
nsDisplayBackground::IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                    nsIFrame* aFrame)
{
  
  if (mIsThemed)
    return PR_FALSE;

  nsPresContext* presContext = mFrame->PresContext();
  nsStyleContext *bgSC;
  PRBool hasBG =
    nsCSSRendering::FindBackground(presContext, mFrame, &bgSC);
  if (!hasBG)
    return PR_FALSE;
  const nsStyleBackground* bg = bgSC->GetStyleBackground();
  if (!bg->HasFixedBackground())
    return PR_FALSE;

  
  
  
  return aFrame->GetParent() &&
    (aFrame == mFrame ||
     nsLayoutUtils::IsProperAncestorFrame(aFrame, mFrame));
}

PRBool
nsDisplayBackground::IsFixedAndCoveringViewport(nsDisplayListBuilder* aBuilder)
{
  if (mIsThemed)
    return PR_FALSE;

  nsPresContext* presContext = mFrame->PresContext();
  nsStyleContext* bgSC;
  PRBool hasBG =
    nsCSSRendering::FindBackground(presContext, mFrame, &bgSC);
  if (!hasBG)
    return PR_FALSE;

  const nsStyleBackground* bg = bgSC->GetStyleBackground();
  if (!bg->HasFixedBackground())
    return PR_FALSE;

  NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, bg) {
    const nsStyleBackground::Layer& layer = bg->mLayers[i];
    if (layer.mAttachment != NS_STYLE_BG_ATTACHMENT_FIXED &&
        !layer.mImage.IsEmpty()) {
      return PR_FALSE;
    }
    if (layer.mClip != NS_STYLE_BG_CLIP_BORDER)
      return PR_FALSE;
  }

  if (nsLayoutUtils::HasNonZeroCorner(mFrame->GetStyleBorder()->mBorderRadius))
    return PR_FALSE;

  nsRect bounds = GetBounds(aBuilder);
  nsIFrame* rootScrollFrame = presContext->PresShell()->GetRootScrollFrame();
  if (!rootScrollFrame)
    return PR_FALSE;
  nsIScrollableFrame* scrollable = do_QueryFrame(rootScrollFrame);
  nsRect scrollport = scrollable->GetScrollPortRect() +
    aBuilder->ToReferenceFrame(rootScrollFrame);
  return bounds.Contains(scrollport);
}

void
nsDisplayBackground::Paint(nsDisplayListBuilder* aBuilder,
                           nsIRenderingContext* aCtx) {
  nsPoint offset = ToReferenceFrame();
  PRUint32 flags = aBuilder->GetBackgroundPaintFlags();
  nsDisplayItem* nextItem = GetAbove();
  if (nextItem && nextItem->GetUnderlyingFrame() == mFrame &&
      nextItem->GetType() == TYPE_BORDER) {
    flags |= nsCSSRendering::PAINTBG_WILL_PAINT_BORDER;
  }
  nsCSSRendering::PaintBackground(mFrame->PresContext(), *aCtx, mFrame,
                                  mVisibleRect,
                                  nsRect(offset, mFrame->GetSize()),
                                  flags);
}

nsRect
nsDisplayBackground::GetBounds(nsDisplayListBuilder* aBuilder) {
  if (mIsThemed) {
    nsRect r(nsPoint(0,0), mFrame->GetSize());
    nsPresContext* presContext = mFrame->PresContext();
    presContext->GetTheme()->
        GetWidgetOverflow(presContext->DeviceContext(), mFrame,
                          mFrame->GetStyleDisplay()->mAppearance, &r);
    return r + ToReferenceFrame();
  }

  return nsRect(ToReferenceFrame(), mFrame->GetSize());
}

nsRect
nsDisplayOutline::GetBounds(nsDisplayListBuilder* aBuilder) {
  return mFrame->GetVisualOverflowRect() + ToReferenceFrame();
}

void
nsDisplayOutline::Paint(nsDisplayListBuilder* aBuilder,
                        nsIRenderingContext* aCtx) {
  
  nsPoint offset = ToReferenceFrame();
  nsCSSRendering::PaintOutline(mFrame->PresContext(), *aCtx, mFrame,
                               mVisibleRect,
                               nsRect(offset, mFrame->GetSize()),
                               mFrame->GetStyleContext());
}

PRBool
nsDisplayOutline::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion) {
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion))
    return PR_FALSE;

  const nsStyleOutline* outline = mFrame->GetStyleOutline();
  nsRect borderBox(ToReferenceFrame(), mFrame->GetSize());
  if (borderBox.Contains(aVisibleRegion->GetBounds()) &&
      !nsLayoutUtils::HasNonZeroCorner(outline->mOutlineRadius)) {
    if (outline->mOutlineOffset >= 0) {
      
      
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

void
nsDisplayEventReceiver::HitTest(nsDisplayListBuilder* aBuilder,
                                const nsRect& aRect,
                                HitTestState* aState,
                                nsTArray<nsIFrame*> *aOutFrames)
{
  if (!RoundedBorderIntersectsRect(mFrame, ToReferenceFrame(), aRect)) {
    
    return;
  }

  aOutFrames->AppendElement(mFrame);
}

void
nsDisplayCaret::Paint(nsDisplayListBuilder* aBuilder,
                      nsIRenderingContext* aCtx) {
  
  
  mCaret->PaintCaret(aBuilder, aCtx, mFrame, ToReferenceFrame());
}

PRBool
nsDisplayBorder::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion) {
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion))
    return PR_FALSE;

  nsRect paddingRect = mFrame->GetPaddingRect() - mFrame->GetPosition() +
    ToReferenceFrame();
  const nsStyleBorder *styleBorder;
  if (paddingRect.Contains(aVisibleRegion->GetBounds()) &&
      !(styleBorder = mFrame->GetStyleBorder())->IsBorderImageLoaded() &&
      !nsLayoutUtils::HasNonZeroCorner(styleBorder->mBorderRadius)) {
    
    
    
    
    
    
    return PR_FALSE;
  }

  return PR_TRUE;
}

void
nsDisplayBorder::Paint(nsDisplayListBuilder* aBuilder,
                       nsIRenderingContext* aCtx) {
  nsPoint offset = ToReferenceFrame();
  nsCSSRendering::PaintBorder(mFrame->PresContext(), *aCtx, mFrame,
                              mVisibleRect,
                              nsRect(offset, mFrame->GetSize()),
                              mFrame->GetStyleContext(),
                              mFrame->GetSkipSides());
}






static void
ComputeDisjointRectangles(const nsRegion& aRegion,
                          nsTArray<nsRect>* aRects) {
  nscoord accumulationMargin = nsPresContext::CSSPixelsToAppUnits(25);
  nsRect accumulated;
  nsRegionRectIterator iter(aRegion);
  while (PR_TRUE) {
    const nsRect* r = iter.Next();
    if (r && !accumulated.IsEmpty() &&
        accumulated.YMost() >= r->y - accumulationMargin) {
      accumulated.UnionRect(accumulated, *r);
      continue;
    }

    if (!accumulated.IsEmpty()) {
      aRects->AppendElement(accumulated);
      accumulated.Empty();
    }

    if (!r)
      break;

    accumulated = *r;
  }
}

void
nsDisplayBoxShadowOuter::Paint(nsDisplayListBuilder* aBuilder,
                               nsIRenderingContext* aCtx) {
  nsPoint offset = ToReferenceFrame();
  nsRect borderRect = nsRect(offset, mFrame->GetSize());
  nsPresContext* presContext = mFrame->PresContext();
  nsAutoTArray<nsRect,10> rects;
  ComputeDisjointRectangles(mVisibleRegion, &rects);

  for (PRUint32 i = 0; i < rects.Length(); ++i) {
    aCtx->PushState();
    aCtx->SetClipRect(rects[i], nsClipCombine_kIntersect);
    nsCSSRendering::PaintBoxShadowOuter(presContext, *aCtx, mFrame,
                                        borderRect, rects[i]);
    aCtx->PopState();
  }
}

nsRect
nsDisplayBoxShadowOuter::GetBounds(nsDisplayListBuilder* aBuilder) {
  return mFrame->GetVisualOverflowRect() + ToReferenceFrame();
}

PRBool
nsDisplayBoxShadowOuter::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion) {
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion))
    return PR_FALSE;

  
  mVisibleRegion.And(*aVisibleRegion, mVisibleRect);

  nsPoint origin = ToReferenceFrame();
  nsRect visibleBounds = aVisibleRegion->GetBounds();
  nsRect frameRect(origin, mFrame->GetSize());
  if (!frameRect.Contains(visibleBounds))
    return PR_TRUE;

  
  
  nscoord twipsRadii[8];
  PRBool hasBorderRadii = mFrame->GetBorderRadii(twipsRadii);
  if (!hasBorderRadii)
    return PR_FALSE;

  return !RoundedRectContainsRect(frameRect, twipsRadii, visibleBounds);
}

void
nsDisplayBoxShadowInner::Paint(nsDisplayListBuilder* aBuilder,
                               nsIRenderingContext* aCtx) {
  nsPoint offset = ToReferenceFrame();
  nsRect borderRect = nsRect(offset, mFrame->GetSize());
  nsPresContext* presContext = mFrame->PresContext();
  nsAutoTArray<nsRect,10> rects;
  ComputeDisjointRectangles(mVisibleRegion, &rects);

  for (PRUint32 i = 0; i < rects.Length(); ++i) {
    aCtx->PushState();
    aCtx->SetClipRect(rects[i], nsClipCombine_kIntersect);
    nsCSSRendering::PaintBoxShadowInner(presContext, *aCtx, mFrame,
                                        borderRect, rects[i]);
    aCtx->PopState();
  }
}

PRBool
nsDisplayBoxShadowInner::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion) {
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion))
    return PR_FALSE;

  
  mVisibleRegion.And(*aVisibleRegion, mVisibleRect);
  return PR_TRUE;
}

nsDisplayWrapList::nsDisplayWrapList(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame, nsDisplayList* aList)
  : nsDisplayItem(aBuilder, aFrame) {
  mList.AppendToTop(aList);
}

nsDisplayWrapList::nsDisplayWrapList(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame, nsDisplayItem* aItem)
  : nsDisplayItem(aBuilder, aFrame) {
  mList.AppendToTop(aItem);
}

nsDisplayWrapList::~nsDisplayWrapList() {
  mList.DeleteAll();
}

void
nsDisplayWrapList::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                           HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {
  mList.HitTest(aBuilder, aRect, aState, aOutFrames);
}

nsRect
nsDisplayWrapList::GetBounds(nsDisplayListBuilder* aBuilder) {
  return mList.GetBounds(aBuilder);
}

PRBool
nsDisplayWrapList::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                     nsRegion* aVisibleRegion) {
  return mList.ComputeVisibilityForSublist(aBuilder, aVisibleRegion,
                                           mVisibleRect);
}

PRBool
nsDisplayWrapList::IsOpaque(nsDisplayListBuilder* aBuilder) {
  return mList.IsOpaque();
}

PRBool nsDisplayWrapList::IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) {
  
  return PR_FALSE;
}

PRBool nsDisplayWrapList::IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                         nsIFrame* aFrame) {
  NS_WARNING("nsDisplayWrapList::IsVaryingRelativeToMovingFrame called unexpectedly");
  
  return PR_TRUE;
}

void nsDisplayWrapList::Paint(nsDisplayListBuilder* aBuilder,
                              nsIRenderingContext* aCtx) {
  NS_ERROR("nsDisplayWrapList should have been flattened away for painting");
}

PRBool nsDisplayWrapList::ChildrenCanBeInactive(nsDisplayListBuilder* aBuilder,
                                                LayerManager* aManager,
                                                const nsDisplayList& aList,
                                                nsIFrame* aActiveScrolledRoot) {
  for (nsDisplayItem* i = aList.GetBottom(); i; i = i->GetAbove()) {
    nsIFrame* f = i->GetUnderlyingFrame();
    if (f) {
      nsIFrame* activeScrolledRoot =
        nsLayoutUtils::GetActiveScrolledRootFor(f, nsnull);
      if (activeScrolledRoot != aActiveScrolledRoot)
        return PR_FALSE;
    }

    LayerState state = i->GetLayerState(aBuilder, aManager);
    if (state == LAYER_ACTIVE)
      return PR_FALSE;
    if (state == LAYER_NONE) {
      nsDisplayList* list = i->GetList();
      if (list && !ChildrenCanBeInactive(aBuilder, aManager, *list, aActiveScrolledRoot))
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

PRBool nsDisplayWrapList::HasText()
{
  for (nsDisplayItem* i = mList.GetBottom(); i; i = i->GetAbove()) {
    if (i->HasText()) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

static nsresult
WrapDisplayList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                nsDisplayList* aList, nsDisplayWrapper* aWrapper) {
  if (!aList->GetTop())
    return NS_OK;
  nsDisplayItem* item = aWrapper->WrapList(aBuilder, aFrame, aList);
  if (!item)
    return NS_ERROR_OUT_OF_MEMORY;
  
  aList->AppendToTop(item);
  return NS_OK;
}

static nsresult
WrapEachDisplayItem(nsDisplayListBuilder* aBuilder,
                    nsDisplayList* aList, nsDisplayWrapper* aWrapper) {
  nsDisplayList newList;
  nsDisplayItem* item;
  while ((item = aList->RemoveBottom())) {
    item = aWrapper->WrapItem(aBuilder, item);
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;
    newList.AppendToTop(item);
  }
  
  aList->AppendToTop(&newList);
  return NS_OK;
}

nsresult nsDisplayWrapper::WrapLists(nsDisplayListBuilder* aBuilder,
    nsIFrame* aFrame, const nsDisplayListSet& aIn, const nsDisplayListSet& aOut)
{
  nsresult rv = WrapListsInPlace(aBuilder, aFrame, aIn);
  NS_ENSURE_SUCCESS(rv, rv);

  if (&aOut == &aIn)
    return NS_OK;
  aOut.BorderBackground()->AppendToTop(aIn.BorderBackground());
  aOut.BlockBorderBackgrounds()->AppendToTop(aIn.BlockBorderBackgrounds());
  aOut.Floats()->AppendToTop(aIn.Floats());
  aOut.Content()->AppendToTop(aIn.Content());
  aOut.PositionedDescendants()->AppendToTop(aIn.PositionedDescendants());
  aOut.Outlines()->AppendToTop(aIn.Outlines());
  return NS_OK;
}

nsresult nsDisplayWrapper::WrapListsInPlace(nsDisplayListBuilder* aBuilder,
    nsIFrame* aFrame, const nsDisplayListSet& aLists)
{
  nsresult rv;
  if (WrapBorderBackground()) {
    
    rv = WrapDisplayList(aBuilder, aFrame, aLists.BorderBackground(), this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  rv = WrapDisplayList(aBuilder, aFrame, aLists.BlockBorderBackgrounds(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = WrapEachDisplayItem(aBuilder, aLists.Floats(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = WrapDisplayList(aBuilder, aFrame, aLists.Content(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = WrapEachDisplayItem(aBuilder, aLists.PositionedDescendants(), this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return WrapEachDisplayItem(aBuilder, aLists.Outlines(), this);
}

nsDisplayOpacity::nsDisplayOpacity(nsDisplayListBuilder* aBuilder,
                                   nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aBuilder, aFrame, aList) {
  MOZ_COUNT_CTOR(nsDisplayOpacity);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayOpacity::~nsDisplayOpacity() {
  MOZ_COUNT_DTOR(nsDisplayOpacity);
}
#endif

PRBool nsDisplayOpacity::IsOpaque(nsDisplayListBuilder* aBuilder) {
  
  
  return PR_FALSE;
}


already_AddRefed<Layer>
nsDisplayOpacity::BuildLayer(nsDisplayListBuilder* aBuilder,
                             LayerManager* aManager) {
  nsRefPtr<Layer> layer = aBuilder->LayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, mList);
  if (!layer)
    return nsnull;

  layer->SetOpacity(mFrame->GetStyleDisplay()->mOpacity);
  return layer.forget();
}

nsDisplayItem::LayerState
nsDisplayOpacity::GetLayerState(nsDisplayListBuilder* aBuilder,
                                LayerManager* aManager) {
  if (mFrame->AreLayersMarkedActive())
    return LAYER_ACTIVE;
  nsIFrame* activeScrolledRoot =
    nsLayoutUtils::GetActiveScrolledRootFor(mFrame, nsnull);
  return !ChildrenCanBeInactive(aBuilder, aManager, mList, activeScrolledRoot)
      ? LAYER_ACTIVE : LAYER_INACTIVE;
}

PRBool nsDisplayOpacity::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion) {
  
  
  
  
  
  nsRect bounds = GetBounds(aBuilder);
  nsRegion visibleUnderChildren;
  visibleUnderChildren.And(*aVisibleRegion, bounds);
  return
    nsDisplayWrapList::ComputeVisibility(aBuilder, &visibleUnderChildren);
}

PRBool nsDisplayOpacity::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
  if (aItem->GetType() != TYPE_OPACITY)
    return PR_FALSE;
  
  
  
  if (aItem->GetUnderlyingFrame()->GetContent() != mFrame->GetContent())
    return PR_FALSE;
  mList.AppendToBottom(&static_cast<nsDisplayOpacity*>(aItem)->mList);
  return PR_TRUE;
}

nsDisplayOwnLayer::nsDisplayOwnLayer(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aBuilder, aFrame, aList) {
  MOZ_COUNT_CTOR(nsDisplayOwnLayer);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayOwnLayer::~nsDisplayOwnLayer() {
  MOZ_COUNT_DTOR(nsDisplayOwnLayer);
}
#endif


already_AddRefed<Layer>
nsDisplayOwnLayer::BuildLayer(nsDisplayListBuilder* aBuilder,
                              LayerManager* aManager) {
  nsRefPtr<Layer> layer = aBuilder->LayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, mList);
  return layer.forget();
}

nsDisplayClip::nsDisplayClip(nsDisplayListBuilder* aBuilder,
                             nsIFrame* aFrame, nsDisplayItem* aItem,
                             const nsRect& aRect)
   : nsDisplayWrapList(aBuilder, aFrame, aItem),
     mClip(aRect) {
  MOZ_COUNT_CTOR(nsDisplayClip);
}

nsDisplayClip::nsDisplayClip(nsDisplayListBuilder* aBuilder,
                             nsIFrame* aFrame, nsDisplayList* aList,
                             const nsRect& aRect)
   : nsDisplayWrapList(aBuilder, aFrame, aList),
     mClip(aRect) {
  MOZ_COUNT_CTOR(nsDisplayClip);
}

nsRect nsDisplayClip::GetBounds(nsDisplayListBuilder* aBuilder) {
  nsRect r = nsDisplayWrapList::GetBounds(aBuilder);
  r.IntersectRect(mClip, r);
  return r;
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayClip::~nsDisplayClip() {
  MOZ_COUNT_DTOR(nsDisplayClip);
}
#endif

void nsDisplayClip::Paint(nsDisplayListBuilder* aBuilder,
                          nsIRenderingContext* aCtx) {
  NS_ERROR("nsDisplayClip should have been flattened away for painting");
}

PRBool nsDisplayClip::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                        nsRegion* aVisibleRegion) {
  nsRegion clipped;
  clipped.And(*aVisibleRegion, mClip);

  nsRegion finalClipped(clipped);
  PRBool anyVisible =
    nsDisplayWrapList::ComputeVisibility(aBuilder, &finalClipped);

  nsRegion removed;
  removed.Sub(clipped, finalClipped);
  aBuilder->SubtractFromVisibleRegion(aVisibleRegion, removed);

  return anyVisible;
}

PRBool nsDisplayClip::TryMerge(nsDisplayListBuilder* aBuilder,
                               nsDisplayItem* aItem) {
  if (aItem->GetType() != TYPE_CLIP)
    return PR_FALSE;
  nsDisplayClip* other = static_cast<nsDisplayClip*>(aItem);
  if (other->mClip != mClip)
    return PR_FALSE;
  mList.AppendToBottom(&other->mList);
  return PR_TRUE;
}

nsDisplayWrapList* nsDisplayClip::WrapWithClone(nsDisplayListBuilder* aBuilder,
                                                nsDisplayItem* aItem) {
  return new (aBuilder)
    nsDisplayClip(aBuilder, aItem->GetUnderlyingFrame(), aItem, mClip);
}

nsDisplayClipRoundedRect::nsDisplayClipRoundedRect(
                             nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                             nsDisplayItem* aItem,
                             const nsRect& aRect, nscoord aRadii[8])
    : nsDisplayClip(aBuilder, aFrame, aItem, aRect)
{
  MOZ_COUNT_CTOR(nsDisplayClipRoundedRect);
  memcpy(mRadii, aRadii, sizeof(mRadii));
}

nsDisplayClipRoundedRect::nsDisplayClipRoundedRect(
                             nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                             nsDisplayList* aList,
                             const nsRect& aRect, nscoord aRadii[8])
    : nsDisplayClip(aBuilder, aFrame, aList, aRect)
{
  MOZ_COUNT_CTOR(nsDisplayClipRoundedRect);
  memcpy(mRadii, aRadii, sizeof(mRadii));
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayClipRoundedRect::~nsDisplayClipRoundedRect()
{
  MOZ_COUNT_DTOR(nsDisplayClipRoundedRect);
}
#endif

PRBool nsDisplayClipRoundedRect::IsOpaque(nsDisplayListBuilder* aBuilder)
{
  return PR_FALSE;
}

void
nsDisplayClipRoundedRect::HitTest(nsDisplayListBuilder* aBuilder,
                                  const nsRect& aRect, HitTestState* aState,
                                  nsTArray<nsIFrame*> *aOutFrames)
{
  if (!RoundedRectIntersectsRect(mClip, mRadii, aRect)) {
    

    
    
    
    
    return;
  }

  mList.HitTest(aBuilder, aRect, aState, aOutFrames);
}

nsDisplayWrapList*
nsDisplayClipRoundedRect::WrapWithClone(nsDisplayListBuilder* aBuilder,
                                        nsDisplayItem* aItem) {
  return new (aBuilder)
    nsDisplayClipRoundedRect(aBuilder, aItem->GetUnderlyingFrame(), aItem,
                             mClip, mRadii);
}

PRBool nsDisplayClipRoundedRect::ComputeVisibility(
                                    nsDisplayListBuilder* aBuilder,
                                    nsRegion* aVisibleRegion)
{
  nsRegion clipped;
  clipped.And(*aVisibleRegion, mClip);

  return nsDisplayWrapList::ComputeVisibility(aBuilder, &clipped);
  
  
}

PRBool nsDisplayClipRoundedRect::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem)
{
  if (aItem->GetType() != TYPE_CLIP_ROUNDED_RECT)
    return PR_FALSE;
  nsDisplayClipRoundedRect* other =
    static_cast<nsDisplayClipRoundedRect*>(aItem);
  if (mClip != other->mClip || mRadii != other->mRadii)
    return PR_FALSE;
  mList.AppendToBottom(&other->mList);
  return PR_TRUE;
}

nsDisplayZoom::nsDisplayZoom(nsDisplayListBuilder* aBuilder,
                             nsIFrame* aFrame, nsDisplayList* aList,
                             PRInt32 aAPD, PRInt32 aParentAPD)
    : nsDisplayOwnLayer(aBuilder, aFrame, aList), mAPD(aAPD),
      mParentAPD(aParentAPD) {
  MOZ_COUNT_CTOR(nsDisplayZoom);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplayZoom::~nsDisplayZoom() {
  MOZ_COUNT_DTOR(nsDisplayZoom);
}
#endif

nsRect nsDisplayZoom::GetBounds(nsDisplayListBuilder* aBuilder)
{
  nsRect bounds = nsDisplayWrapList::GetBounds(aBuilder);
  return bounds.ConvertAppUnitsRoundOut(mAPD, mParentAPD);
}

void nsDisplayZoom::HitTest(nsDisplayListBuilder *aBuilder,
                            const nsRect& aRect,
                            HitTestState *aState,
                            nsTArray<nsIFrame*> *aOutFrames)
{
  nsRect rect;
  
  
  if (aRect.width == 1 && aRect.height == 1) {
    rect.MoveTo(aRect.TopLeft().ConvertAppUnits(mParentAPD, mAPD));
    rect.width = rect.height = 1;
  } else {
    rect = aRect.ConvertAppUnitsRoundOut(mParentAPD, mAPD);
  }
  mList.HitTest(aBuilder, rect, aState, aOutFrames);
}

void nsDisplayZoom::Paint(nsDisplayListBuilder* aBuilder,
                          nsIRenderingContext* aCtx)
{
  mList.PaintForFrame(aBuilder, aCtx, mFrame, nsDisplayList::PAINT_DEFAULT);
}

PRBool nsDisplayZoom::ComputeVisibility(nsDisplayListBuilder *aBuilder,
                                        nsRegion *aVisibleRegion)
{
  
  nsRegion visibleRegion =
    aVisibleRegion->ConvertAppUnitsRoundOut(mParentAPD, mAPD);
  nsRegion originalVisibleRegion = visibleRegion;

  nsRect transformedVisibleRect =
    mVisibleRect.ConvertAppUnitsRoundOut(mParentAPD, mAPD);
  PRBool retval =
    mList.ComputeVisibilityForSublist(aBuilder, &visibleRegion,
                                      transformedVisibleRect);

  nsRegion removed;
  
  removed.Sub(originalVisibleRegion, visibleRegion);
  
  removed = removed.ConvertAppUnitsRoundIn(mAPD, mParentAPD);
  
  
  aBuilder->SubtractFromVisibleRegion(aVisibleRegion, removed);

  return retval;
}













#undef  UNIFIED_CONTINUATIONS
#undef  DEBUG_HIT










#ifndef UNIFIED_CONTINUATIONS

nsRect
nsDisplayTransform::GetFrameBoundsForTransform(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Can't get the bounds of a nonexistent frame!");
  return nsRect(nsPoint(0, 0), aFrame->GetSize());
}

#else

nsRect
nsDisplayTransform::GetFrameBoundsForTransform(const nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "Can't get the bounds of a nonexistent frame!");

  nsRect result;
  
  


  for (const nsIFrame *currFrame = aFrame->GetFirstContinuation();
       currFrame != nsnull;
       currFrame = currFrame->GetNextContinuation())
    {
      


      result.UnionRect(result, nsRect(currFrame->GetOffsetTo(aFrame),
                                      currFrame->GetSize()));
    }

  return result;
}

#endif





static
gfxPoint GetDeltaToMozTransformOrigin(const nsIFrame* aFrame,
                                      float aFactor,
                                      const nsRect* aBoundsOverride)
{
  NS_PRECONDITION(aFrame, "Can't get delta for a null frame!");
  NS_PRECONDITION(aFrame->GetStyleDisplay()->HasTransform(),
                  "Can't get a delta for an untransformed frame!");

  



  const nsStyleDisplay* display = aFrame->GetStyleDisplay();
  nsRect boundingRect = (aBoundsOverride ? *aBoundsOverride :
                         nsDisplayTransform::GetFrameBoundsForTransform(aFrame));

  
  gfxPoint result;
  gfxFloat* coords[2] = {&result.x, &result.y};
  const nscoord* dimensions[2] =
    {&boundingRect.width, &boundingRect.height};

  for (PRUint8 index = 0; index < 2; ++index) {
    


    const nsStyleCoord &coord = display->mTransformOrigin[index];
    if (coord.GetUnit() == eStyleUnit_Calc) {
      const nsStyleCoord::Calc *calc = coord.GetCalcValue();
      *coords[index] = NSAppUnitsToFloatPixels(*dimensions[index], aFactor) *
                         calc->mPercent +
                       NSAppUnitsToFloatPixels(calc->mLength, aFactor);
    } else if (coord.GetUnit() == eStyleUnit_Percent) {
      *coords[index] = NSAppUnitsToFloatPixels(*dimensions[index], aFactor) *
        coord.GetPercentValue();
    } else {
      NS_ABORT_IF_FALSE(coord.GetUnit() == eStyleUnit_Coord, "unexpected unit");
      *coords[index] = NSAppUnitsToFloatPixels(coord.GetCoordValue(), aFactor);
    }
  }
  
  
  result.x += NSAppUnitsToFloatPixels(boundingRect.x, aFactor);
  result.y += NSAppUnitsToFloatPixels(boundingRect.y, aFactor);

  return result;
}





gfxMatrix
nsDisplayTransform::GetResultingTransformMatrix(const nsIFrame* aFrame,
                                                const nsPoint &aOrigin,
                                                float aFactor,
                                                const nsRect* aBoundsOverride)
{
  NS_PRECONDITION(aFrame, "Cannot get transform matrix for a null frame!");
  NS_PRECONDITION(aFrame->GetStyleDisplay()->HasTransform(),
                  "Cannot get transform matrix if frame isn't transformed!");

  


  gfxPoint toMozOrigin = GetDeltaToMozTransformOrigin(aFrame, aFactor, aBoundsOverride);
  gfxPoint newOrigin = gfxPoint(NSAppUnitsToFloatPixels(aOrigin.x, aFactor),
                                NSAppUnitsToFloatPixels(aOrigin.y, aFactor));

  


  const nsStyleDisplay* disp = aFrame->GetStyleDisplay();
  nsRect bounds = (aBoundsOverride ? *aBoundsOverride :
                   nsDisplayTransform::GetFrameBoundsForTransform(aFrame));

  
  return nsLayoutUtils::ChangeMatrixBasis
    (newOrigin + toMozOrigin, disp->mTransform.GetThebesMatrix(bounds, aFactor));
}

already_AddRefed<Layer> nsDisplayTransform::BuildLayer(nsDisplayListBuilder *aBuilder,
                                                       LayerManager *aManager)
{
  gfxMatrix newTransformMatrix =
    GetResultingTransformMatrix(mFrame, ToReferenceFrame(),
                                 mFrame->PresContext()->AppUnitsPerDevPixel(),
                                nsnull);
  if (newTransformMatrix.IsSingular())
    return nsnull;

  nsRefPtr<Layer> layer = aBuilder->LayerBuilder()->
    BuildContainerLayerFor(aBuilder, aManager, mFrame, this, *mStoredList.GetList());
  if (!layer)
    return nsnull;
 
  layer->SetTransform(gfx3DMatrix::From2D(newTransformMatrix));
  return layer.forget();
}

nsDisplayItem::LayerState
nsDisplayTransform::GetLayerState(nsDisplayListBuilder* aBuilder,
                                  LayerManager* aManager) {
  if (mFrame->AreLayersMarkedActive())
    return LAYER_ACTIVE;
  nsIFrame* activeScrolledRoot =
    nsLayoutUtils::GetActiveScrolledRootFor(mFrame, nsnull);
  return !mStoredList.ChildrenCanBeInactive(aBuilder, 
                                             aManager, 
                                             *mStoredList.GetList(), 
                                             activeScrolledRoot)
      ? LAYER_ACTIVE : LAYER_INACTIVE;
}

PRBool nsDisplayTransform::ComputeVisibility(nsDisplayListBuilder *aBuilder,
                                             nsRegion *aVisibleRegion)
{
  


  nsRegion untransformedVisible =
    UntransformRect(mVisibleRect, mFrame, ToReferenceFrame());
  
  
  
  mStoredList.RecomputeVisibility(aBuilder, &untransformedVisible);
  return PR_TRUE;
}

#ifdef DEBUG_HIT
#include <time.h>
#endif


void nsDisplayTransform::HitTest(nsDisplayListBuilder *aBuilder,
                                 const nsRect& aRect,
                                 HitTestState *aState,
                                 nsTArray<nsIFrame*> *aOutFrames)
{
  






  float factor = nsPresContext::AppUnitsPerCSSPixel();
  gfxMatrix matrix =
    GetResultingTransformMatrix(mFrame, ToReferenceFrame(),
                                factor, nsnull);
  if (matrix.IsSingular())
    return;

  



  matrix.Invert();

  
  nsRect resultingRect;
  if (aRect.width == 1 && aRect.height == 1) {
    gfxPoint point = matrix.Transform(gfxPoint(NSAppUnitsToFloatPixels(aRect.x, factor),
                                               NSAppUnitsToFloatPixels(aRect.y, factor)));

    resultingRect = nsRect(NSFloatPixelsToAppUnits(float(point.x), factor),
                           NSFloatPixelsToAppUnits(float(point.y), factor),
                           1, 1);

  } else {
    gfxRect originalRect(NSAppUnitsToFloatPixels(aRect.x, factor),
                         NSAppUnitsToFloatPixels(aRect.y, factor),
                         NSAppUnitsToFloatPixels(aRect.width, factor),
                         NSAppUnitsToFloatPixels(aRect.height, factor));

    gfxRect rect = matrix.TransformBounds(originalRect);

    resultingRect = nsRect(NSFloatPixelsToAppUnits(float(rect.X()), factor),
                           NSFloatPixelsToAppUnits(float(rect.Y()), factor),
                           NSFloatPixelsToAppUnits(float(rect.Width()), factor),
                           NSFloatPixelsToAppUnits(float(rect.Height()), factor));
  }
  

#ifdef DEBUG_HIT
  printf("Frame: %p\n", dynamic_cast<void *>(mFrame));
  printf("  Untransformed point: (%f, %f)\n", resultingRect.X(), resultingRect.Y());
  PRUint32 originalFrameCount = aOutFrames.Length();
#endif

  mStoredList.HitTest(aBuilder, resultingRect, aState, aOutFrames);

#ifdef DEBUG_HIT
  if (originalFrameCount != aOutFrames.Length())
    printf("  Hit! Time: %f, first frame: %p\n", static_cast<double>(clock()),
           dynamic_cast<void *>(aOutFrames.ElementAt(0)));
  printf("=== end of hit test ===\n");
#endif

}




nsRect nsDisplayTransform::GetBounds(nsDisplayListBuilder *aBuilder)
{
  return mFrame->GetVisualOverflowRect() + ToReferenceFrame();
}











PRBool nsDisplayTransform::IsOpaque(nsDisplayListBuilder *aBuilder)
{
  const nsStyleDisplay* disp = mFrame->GetStyleDisplay();
  return disp->mTransform.GetMainMatrixEntry(1) == 0.0f &&
    disp->mTransform.GetMainMatrixEntry(2) == 0.0f &&
    mStoredList.IsOpaque(aBuilder);
}





PRBool nsDisplayTransform::IsUniform(nsDisplayListBuilder *aBuilder, nscolor* aColor)
{
  const nsStyleDisplay* disp = mFrame->GetStyleDisplay();
  return disp->mTransform.GetMainMatrixEntry(1) == 0.0f &&
    disp->mTransform.GetMainMatrixEntry(2) == 0.0f &&
    mStoredList.IsUniform(aBuilder, aColor);
}





#ifndef UNIFIED_CONTINUATIONS

PRBool
nsDisplayTransform::TryMerge(nsDisplayListBuilder *aBuilder,
                             nsDisplayItem *aItem)
{
  return PR_FALSE;
}

#else

PRBool
nsDisplayTransform::TryMerge(nsDisplayListBuilder *aBuilder,
                             nsDisplayItem *aItem)
{
  NS_PRECONDITION(aItem, "Why did you try merging with a null item?");
  NS_PRECONDITION(aBuilder, "Why did you try merging with a null builder?");

  
  if (aItem->GetType() != TYPE_TRANSFORM)
    return PR_FALSE;

  
  if (aItem->GetUnderlyingFrame()->GetContent() != mFrame->GetContent())
    return PR_FALSE;

  


  mStoredList.GetList()->
    AppendToBottom(&static_cast<nsDisplayTransform *>(aItem)->mStoredList);
  return PR_TRUE;
}

#endif















nsRect nsDisplayTransform::TransformRect(const nsRect &aUntransformedBounds,
                                         const nsIFrame* aFrame,
                                         const nsPoint &aOrigin,
                                         const nsRect* aBoundsOverride)
{
  NS_PRECONDITION(aFrame, "Can't take the transform based on a null frame!");
  NS_PRECONDITION(aFrame->GetStyleDisplay()->HasTransform(),
                  "Cannot transform a rectangle if there's no transformation!");

  float factor = nsPresContext::AppUnitsPerCSSPixel();
  return nsLayoutUtils::MatrixTransformRect
    (aUntransformedBounds,
     GetResultingTransformMatrix(aFrame, aOrigin, factor, aBoundsOverride),
     factor);
}

nsRect nsDisplayTransform::UntransformRect(const nsRect &aUntransformedBounds,
                                           const nsIFrame* aFrame,
                                           const nsPoint &aOrigin)
{
  NS_PRECONDITION(aFrame, "Can't take the transform based on a null frame!");
  NS_PRECONDITION(aFrame->GetStyleDisplay()->HasTransform(),
                  "Cannot transform a rectangle if there's no transformation!");


  


  float factor = nsPresContext::AppUnitsPerCSSPixel();
  gfxMatrix matrix = GetResultingTransformMatrix(aFrame, aOrigin, factor, nsnull);
  if (matrix.IsSingular())
    return nsRect();

  
  matrix.Invert();

  return nsLayoutUtils::MatrixTransformRect(aUntransformedBounds, matrix,
                                            factor);
}

#ifdef MOZ_SVG
nsDisplaySVGEffects::nsDisplaySVGEffects(nsDisplayListBuilder* aBuilder,
                                         nsIFrame* aFrame, nsDisplayList* aList)
    : nsDisplayWrapList(aBuilder, aFrame, aList), mEffectsFrame(aFrame),
      mBounds(aFrame->GetVisualOverflowRectRelativeToSelf())
{
  MOZ_COUNT_CTOR(nsDisplaySVGEffects);
}

#ifdef NS_BUILD_REFCNT_LOGGING
nsDisplaySVGEffects::~nsDisplaySVGEffects()
{
  MOZ_COUNT_DTOR(nsDisplaySVGEffects);
}
#endif

PRBool nsDisplaySVGEffects::IsOpaque(nsDisplayListBuilder* aBuilder)
{
  return PR_FALSE;
}

void
nsDisplaySVGEffects::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                             HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsPoint rectCenter(aRect.x + aRect.width / 2, aRect.y + aRect.height / 2);
  if (nsSVGIntegrationUtils::HitTestFrameForEffects(mEffectsFrame,
      rectCenter - aBuilder->ToReferenceFrame(mEffectsFrame))) {
    mList.HitTest(aBuilder, aRect, aState, aOutFrames);
  }
}

void nsDisplaySVGEffects::Paint(nsDisplayListBuilder* aBuilder,
                                nsIRenderingContext* aCtx)
{
  nsSVGIntegrationUtils::PaintFramesWithEffects(aCtx,
          mEffectsFrame, mVisibleRect, aBuilder, &mList);
}

PRBool nsDisplaySVGEffects::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                              nsRegion* aVisibleRegion) {
  nsPoint offset = aBuilder->ToReferenceFrame(mEffectsFrame);
  nsRect dirtyRect =
    nsSVGIntegrationUtils::GetRequiredSourceForInvalidArea(mEffectsFrame,
                                                           mVisibleRect - offset) +
    offset;

  
  
  nsRegion childrenVisible(dirtyRect);
  nsRect r;
  r.IntersectRect(dirtyRect, mList.GetBounds(aBuilder));
  mList.ComputeVisibilityForSublist(aBuilder, &childrenVisible, r);
  return PR_TRUE;
}

PRBool nsDisplaySVGEffects::TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem)
{
  if (aItem->GetType() != TYPE_SVG_EFFECTS)
    return PR_FALSE;
  
  
  
  if (aItem->GetUnderlyingFrame()->GetContent() != mFrame->GetContent())
    return PR_FALSE;
  nsDisplaySVGEffects* other = static_cast<nsDisplaySVGEffects*>(aItem);
  mList.AppendToBottom(&other->mList);
  mBounds.UnionRect(mBounds,
    other->mBounds + other->mEffectsFrame->GetOffsetTo(mEffectsFrame));
  return PR_TRUE;
}
#endif
