





#include "AccessibleCaretManager.h"

#include "AccessibleCaret.h"
#include "AccessibleCaretEventHub.h"
#include "AccessibleCaretLogger.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/dom/TreeWalker.h"
#include "nsCaret.h"
#include "nsContentUtils.h"
#include "nsFocusManager.h"
#include "nsFrame.h"
#include "nsFrameSelection.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {

#undef AC_LOG
#define AC_LOG(message, ...)                                                   \
  AC_LOG_BASE("AccessibleCaretManager (%p): " message, this, ##__VA_ARGS__);

#undef AC_LOGV
#define AC_LOGV(message, ...)                                                  \
  AC_LOGV_BASE("AccessibleCaretManager (%p): " message, this, ##__VA_ARGS__);

using namespace dom;
using Appearance = AccessibleCaret::Appearance;
using PositionChangedResult = AccessibleCaret::PositionChangedResult;

AccessibleCaretManager::AccessibleCaretManager(nsIPresShell* aPresShell)
  : mPresShell(aPresShell)
{
  if (mPresShell) {
    mFirstCaret = MakeUnique<AccessibleCaret>(mPresShell);
    mSecondCaret = MakeUnique<AccessibleCaret>(mPresShell);

    mCaretTimeoutTimer = do_CreateInstance("@mozilla.org/timer;1");
  }
}

AccessibleCaretManager::~AccessibleCaretManager()
{
  CancelCaretTimeoutTimer();
}

nsresult
AccessibleCaretManager::OnSelectionChanged(nsIDOMDocument* aDoc,
                                           nsISelection* aSel, int16_t aReason)
{
  AC_LOG("aSel: %p, GetSelection(): %p, aReason: %d", aSel, GetSelection(),
         aReason);

  if (aSel != GetSelection()) {
    return NS_OK;
  }

  
  if (aReason == nsISelectionListener::NO_REASON) {
    HideCarets();
    return NS_OK;
  }

  
  if (aReason & nsISelectionListener::KEYPRESS_REASON) {
    HideCarets();
    return NS_OK;
  }

  
  if (aReason & (nsISelectionListener::COLLAPSETOSTART_REASON |
                 nsISelectionListener::COLLAPSETOEND_REASON)) {
    HideCarets();
    return NS_OK;
  }

  UpdateCarets();
  return NS_OK;
}

void
AccessibleCaretManager::HideCarets()
{
  if (mFirstCaret->IsLogicallyVisible() || mSecondCaret->IsLogicallyVisible()) {
    AC_LOG("%s", __FUNCTION__);
    mFirstCaret->SetAppearance(Appearance::None);
    mSecondCaret->SetAppearance(Appearance::None);
    CancelCaretTimeoutTimer();
  }
}

void
AccessibleCaretManager::UpdateCarets()
{
  mCaretMode = GetCaretMode();

  switch (mCaretMode) {
  case CaretMode::None:
    HideCarets();
    break;
  case CaretMode::Cursor:
    UpdateCaretsForCursorMode();
    break;
  case CaretMode::Selection:
    UpdateCaretsForSelectionMode();
    break;
  }
}

void
AccessibleCaretManager::UpdateCaretsForCursorMode()
{
  AC_LOG("%s, selection: %p", __FUNCTION__, GetSelection());

  nsRefPtr<nsCaret> caret = mPresShell->GetCaret();
  if (!caret || !caret->IsVisible()) {
    HideCarets();
    return;
  }

  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  Selection* selection = GetSelection();
  if (!fs || !selection) {
    HideCarets();
    return;
  }

  nsINode* focusNode = selection->GetFocusNode();
  nsIContent* focusContent = focusNode->AsContent();
  uint32_t focusOffset = selection->FocusOffset();

  nsIFrame* frame = nullptr;
  int32_t offset = 0;
  nsresult rv = nsCaret::GetCaretFrameForNodeOffset(
    fs, focusContent, focusOffset, fs->GetHint(), fs->GetCaretBidiLevel(),
    &frame, &offset);

  if (NS_FAILED(rv) || !frame) {
    HideCarets();
    return;
  }

  Element* editingHost = frame->GetContent()->GetEditingHost();
  if (!editingHost) {
    HideCarets();
    return;
  }

  
  
  
  mFirstCaret->SetPosition(frame, offset);
  mFirstCaret->SetSelectionBarEnabled(false);
  if (nsContentUtils::HasNonEmptyTextContent(
        editingHost, nsContentUtils::eRecurseIntoChildren)) {
    mFirstCaret->SetAppearance(Appearance::Normal);
    LaunchCaretTimeoutTimer();
  } else {
    mFirstCaret->SetAppearance(Appearance::NormalNotShown);
  }
  mSecondCaret->SetAppearance(Appearance::None);
}

void
AccessibleCaretManager::UpdateCaretsForSelectionMode()
{
  AC_LOG("%s, selection: %p", __FUNCTION__, GetSelection());

  int32_t startOffset = 0;
  nsIFrame* startFrame = FindFirstNodeWithFrame(false, &startOffset);

  int32_t endOffset = 0;
  nsIFrame* endFrame = FindFirstNodeWithFrame(true, &endOffset);

  if (!startFrame || !endFrame ||
      nsLayoutUtils::CompareTreePosition(startFrame, endFrame) > 0) {
    HideCarets();
    return;
  }

  auto updateSingleCaret = [](AccessibleCaret * aCaret, nsIFrame * aFrame,
                              int32_t aOffset)->PositionChangedResult
  {
    PositionChangedResult result = aCaret->SetPosition(aFrame, aOffset);
    aCaret->SetSelectionBarEnabled(true);
    switch (result) {
    case PositionChangedResult::NotChanged:
      
      break;
    case PositionChangedResult::Changed:
      aCaret->SetAppearance(Appearance::Normal);
      break;
    case PositionChangedResult::Invisible:
      aCaret->SetAppearance(Appearance::NormalNotShown);
      break;
    }
    return result;
  };

  PositionChangedResult firstCaretResult =
    updateSingleCaret(mFirstCaret.get(), startFrame, startOffset);
  PositionChangedResult secondCaretResult =
    updateSingleCaret(mSecondCaret.get(), endFrame, endOffset);

  if (firstCaretResult == PositionChangedResult::Changed ||
      secondCaretResult == PositionChangedResult::Changed) {
    
    mPresShell->FlushPendingNotifications(Flush_Layout);
  }

  UpdateCaretsForTilt();
}

void
AccessibleCaretManager::UpdateCaretsForTilt()
{
  if (mFirstCaret->IsVisuallyVisible() && mSecondCaret->IsVisuallyVisible()) {
    if (mFirstCaret->Intersects(*mSecondCaret)) {
      if (mFirstCaret->LogicalPosition().x <=
          mSecondCaret->LogicalPosition().x) {
        mFirstCaret->SetAppearance(Appearance::Left);
        mSecondCaret->SetAppearance(Appearance::Right);
      } else {
        mFirstCaret->SetAppearance(Appearance::Right);
        mSecondCaret->SetAppearance(Appearance::Left);
      }
    } else {
      mFirstCaret->SetAppearance(Appearance::Normal);
      mSecondCaret->SetAppearance(Appearance::Normal);
    }
  }
}

nsresult
AccessibleCaretManager::PressCaret(const nsPoint& aPoint)
{
  nsresult rv = NS_ERROR_FAILURE;

  if (mFirstCaret->Contains(aPoint)) {
    mActiveCaret = mFirstCaret.get();
    SetSelectionDirection(eDirPrevious);
  } else if (mSecondCaret->Contains(aPoint)) {
    mActiveCaret = mSecondCaret.get();
    SetSelectionDirection(eDirNext);
  }

  if (mActiveCaret) {
    mOffsetYToCaretLogicalPosition =
      mActiveCaret->LogicalPosition().y - aPoint.y;
    SetSelectionDragState(true);
    CancelCaretTimeoutTimer();
    rv = NS_OK;
  }

  return rv;
}

nsresult
AccessibleCaretManager::DragCaret(const nsPoint& aPoint)
{
  MOZ_ASSERT(mActiveCaret);
  MOZ_ASSERT(GetCaretMode() != CaretMode::None);

  nsPoint point(aPoint.x, aPoint.y + mOffsetYToCaretLogicalPosition);
  DragCaretInternal(point);
  UpdateCarets();
  return NS_OK;
}

nsresult
AccessibleCaretManager::ReleaseCaret()
{
  MOZ_ASSERT(mActiveCaret);

  mActiveCaret = nullptr;
  SetSelectionDragState(false);
  LaunchCaretTimeoutTimer();
  return NS_OK;
}

nsresult
AccessibleCaretManager::TapCaret(const nsPoint& aPoint)
{
  MOZ_ASSERT(GetCaretMode() != CaretMode::None);

  nsresult rv = NS_ERROR_FAILURE;

  if (GetCaretMode() == CaretMode::Cursor) {
    rv = NS_OK;
  }

  return rv;
}

nsresult
AccessibleCaretManager::SelectWordOrShortcut(const nsPoint& aPoint)
{
  if (!mPresShell) {
    return NS_ERROR_UNEXPECTED;
  }

  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  if (!rootFrame) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsIFrame* ptFrame = nsLayoutUtils::GetFrameForPoint(rootFrame, aPoint,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC);
  if (!ptFrame) {
    return NS_ERROR_FAILURE;
  }

  bool selectable;
  ptFrame->IsSelectable(&selectable, nullptr);
  if (!selectable) {
    return NS_ERROR_FAILURE;
  }

  nsPoint ptInFrame = aPoint;
  nsLayoutUtils::TransformPoint(rootFrame, ptFrame, ptInFrame);

  nsIContent* editingHost = ptFrame->GetContent()->GetEditingHost();
  if (ChangeFocus(ptFrame) &&
      (editingHost && !nsContentUtils::HasNonEmptyTextContent(
                         editingHost, nsContentUtils::eRecurseIntoChildren))) {
    
    AC_LOG("%s, Cannot select word bacause content is empty", __FUNCTION__);
    return NS_OK;
  }

  nsresult rv = SelectWord(ptFrame, ptInFrame);
  UpdateCarets();
  return rv;
}

void
AccessibleCaretManager::OnScrollStart()
{
  AC_LOG("%s", __FUNCTION__);

  HideCarets();
}

void
AccessibleCaretManager::OnScrollEnd()
{
  if (mCaretMode != GetCaretMode()) {
    return;
  }

  if (GetCaretMode() == CaretMode::Cursor) {
    AC_LOG("%s: HideCarets()", __FUNCTION__);
    HideCarets();
  } else {
    AC_LOG("%s: UpdateCarets()", __FUNCTION__);
    UpdateCarets();
  }
}

void
AccessibleCaretManager::OnScrolling()
{
  if (mCaretMode != GetCaretMode()) {
    return;
  }

  if (GetCaretMode() == CaretMode::Cursor) {
    AC_LOG("%s: HideCarets()", __FUNCTION__);
    HideCarets();
  } else {
    AC_LOG("%s: UpdateCarets()", __FUNCTION__);
    UpdateCarets();
  }
}

void
AccessibleCaretManager::OnScrollPositionChanged()
{
  if (mCaretMode != GetCaretMode()) {
    return;
  }

  AC_LOG("%s: UpdateCarets()", __FUNCTION__);
  UpdateCarets();
}

void
AccessibleCaretManager::OnReflow()
{
  if (mCaretMode != GetCaretMode()) {
    return;
  }

  if (mFirstCaret->IsVisuallyVisible() || mSecondCaret->IsVisuallyVisible()) {
    AC_LOG("%s: UpdateCarets()", __FUNCTION__);
    UpdateCarets();
  }
}

void
AccessibleCaretManager::OnBlur()
{
  AC_LOG("%s: HideCarets()", __FUNCTION__);
  HideCarets();
}

void
AccessibleCaretManager::OnKeyboardEvent()
{
  if (GetCaretMode() == CaretMode::Cursor) {
    AC_LOG("%s: HideCarets()", __FUNCTION__);
    HideCarets();
  }
}

nsIContent*
AccessibleCaretManager::GetFocusedContent() const
{
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  MOZ_ASSERT(fm);
  return fm->GetFocusedContent();
}

Selection*
AccessibleCaretManager::GetSelection() const
{
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  if (!fs) {
    return nullptr;
  }
  return fs->GetSelection(nsISelectionController::SELECTION_NORMAL);
}

already_AddRefed<nsFrameSelection>
AccessibleCaretManager::GetFrameSelection() const
{
  nsIContent* focusedContent = GetFocusedContent();
  if (focusedContent) {
    nsIFrame* focusFrame = focusedContent->GetPrimaryFrame();
    if (!focusFrame) {
      return nullptr;
    }

    
    
    nsRefPtr<nsFrameSelection> fs = focusFrame->GetFrameSelection();
    if (!fs || fs->GetShell() != mPresShell) {
      return nullptr;
    }

    return fs.forget();
  } else {
    
    return mPresShell->FrameSelection();
  }
}

AccessibleCaretManager::CaretMode
AccessibleCaretManager::GetCaretMode() const
{
  Selection* selection = GetSelection();
  if (!selection) {
    return CaretMode::None;
  }

  uint32_t rangeCount = selection->RangeCount();
  if (rangeCount <= 0) {
    return CaretMode::None;
  }

  if (selection->IsCollapsed()) {
    return CaretMode::Cursor;
  }

  return CaretMode::Selection;
}

bool
AccessibleCaretManager::ChangeFocus(nsIFrame* aFrame) const
{
  nsIFrame* currFrame = aFrame;
  nsIContent* newFocusContent = nullptr;
  while (currFrame) {
    int32_t tabIndexUnused = 0;
    if (currFrame->IsFocusable(&tabIndexUnused, true)) {
      newFocusContent = currFrame->GetContent();
      nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(newFocusContent));
      if (domElement)
        break;
    }
    currFrame = currFrame->GetParent();
  }

  
  
  
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  if (newFocusContent && currFrame) {
    nsCOMPtr<nsIDOMElement> domElement(do_QueryInterface(newFocusContent));
    fm->SetFocus(domElement, 0);
  } else {
    nsIContent* focusedContent = GetFocusedContent();
    if (focusedContent) {
      
      nsGenericHTMLElement* focusedGeneric =
        nsGenericHTMLElement::FromContent(focusedContent);
      if (focusedContent->GetTextEditorRootContent() ||
          (focusedGeneric && focusedGeneric->IsContentEditable())) {
        nsIDOMWindow* win = mPresShell->GetDocument()->GetWindow();
        if (win) {
          fm->ClearFocus(win);
        }
      }
    }
  }

  return (newFocusContent && currFrame);
}

nsresult
AccessibleCaretManager::SelectWord(nsIFrame* aFrame, const nsPoint& aPoint) const
{
  SetSelectionDragState(true);
  nsFrame* frame = static_cast<nsFrame*>(aFrame);
  nsresult rs = frame->SelectByTypeAtPoint(mPresShell->GetPresContext(), aPoint,
                                           eSelectWord, eSelectWord, 0);

#ifdef DEBUG_FRAME_DUMP
  nsCString frameTag;
  frame->ListTag(frameTag);
  AC_LOG("Frame=%s, ptInFrame=(%d, %d)", frameTag.get(), aPoint.x, aPoint.y);
#endif

  SetSelectionDragState(false);
  ClearMaintainedSelection();

  return rs;
}

void
AccessibleCaretManager::SetSelectionDragState(bool aState) const
{
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  if (fs) {
    fs->SetDragState(aState);
  }
}

void
AccessibleCaretManager::SetSelectionDirection(nsDirection aDir) const
{
  Selection* selection = GetSelection();
  if (selection) {
    selection->AdjustAnchorFocusForMultiRange(aDir);
  }
}

void
AccessibleCaretManager::ClearMaintainedSelection() const
{
  
  
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  if (fs) {
    fs->MaintainSelection(eSelectNoAmount);
  }
}

nsIFrame*
AccessibleCaretManager::FindFirstNodeWithFrame(bool aBackward,
                                               int32_t* aOutOffset) const
{
  if (!mPresShell) {
    return nullptr;
  }

  nsRefPtr<Selection> selection = GetSelection();
  if (!selection) {
    return nullptr;
  }

  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  if (!fs) {
    return nullptr;
  }

  uint32_t rangeCount = selection->RangeCount();
  if (rangeCount <= 0) {
    return nullptr;
  }

  nsRange* range = selection->GetRangeAt(aBackward ? rangeCount - 1 : 0);
  nsRefPtr<nsINode> startNode =
    aBackward ? range->GetEndParent() : range->GetStartParent();
  nsRefPtr<nsINode> endNode =
    aBackward ? range->GetStartParent() : range->GetEndParent();
  int32_t offset = aBackward ? range->EndOffset() : range->StartOffset();
  nsCOMPtr<nsIContent> startContent = do_QueryInterface(startNode);
  CaretAssociationHint hintStart =
    aBackward ? CARET_ASSOCIATE_BEFORE : CARET_ASSOCIATE_AFTER;
  nsIFrame* startFrame =
    fs->GetFrameForNodeOffset(startContent, offset, hintStart, aOutOffset);

  if (startFrame) {
    return startFrame;
  }

  ErrorResult err;
  nsRefPtr<TreeWalker> walker = mPresShell->GetDocument()->CreateTreeWalker(
    *startNode, nsIDOMNodeFilter::SHOW_ALL, nullptr, err);

  if (!walker) {
    return nullptr;
  }

  startFrame = startContent ? startContent->GetPrimaryFrame() : nullptr;
  while (!startFrame && startNode != endNode) {
    startNode = aBackward ? walker->PreviousNode(err) : walker->NextNode(err);

    if (!startNode) {
      break;
    }

    startContent = startNode->AsContent();
    startFrame = startContent ? startContent->GetPrimaryFrame() : nullptr;
  }
  return startFrame;
}

bool
AccessibleCaretManager::CompareRangeWithContentOffset(nsIFrame::ContentOffsets& aOffsets)
{
  Selection* selection = GetSelection();
  if (!selection) {
    return false;
  }

  uint32_t rangeCount = selection->RangeCount();
  MOZ_ASSERT(rangeCount > 0);

  int32_t rangeIndex = (mActiveCaret == mFirstCaret.get() ? rangeCount - 1 : 0);
  nsRefPtr<nsRange> range = selection->GetRangeAt(rangeIndex);

  nsINode* node = nullptr;
  int32_t nodeOffset = 0;
  CaretAssociationHint hint;
  nsDirection dir;

  if (mActiveCaret == mFirstCaret.get()) {
    
    node = range->GetEndParent();
    nodeOffset = range->EndOffset();
    hint = CARET_ASSOCIATE_BEFORE;
    dir = eDirPrevious;
  } else {
    
    node = range->GetStartParent();
    nodeOffset = range->StartOffset();
    hint = CARET_ASSOCIATE_AFTER;
    dir = eDirNext;
  }
  nsCOMPtr<nsIContent> content = do_QueryInterface(node);

  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  if (!fs) {
    return false;
  }

  int32_t offset = 0;
  nsIFrame* theFrame =
    fs->GetFrameForNodeOffset(content, nodeOffset, hint, &offset);

  if (!theFrame) {
    return false;
  }

  
  nsPeekOffsetStruct pos(eSelectCluster,
                         dir,
                         offset,
                         nsPoint(0, 0),
                         true,
                         true,  
                         false,
                         false,
                         false);
  nsresult rv = theFrame->PeekOffset(&pos);
  if (NS_FAILED(rv)) {
    pos.mResultContent = content;
    pos.mContentOffset = nodeOffset;
  }

  
  int32_t result = nsContentUtils::ComparePoints(aOffsets.content,
                                                 aOffsets.StartOffset(),
                                                 pos.mResultContent,
                                                 pos.mContentOffset);
  if ((mActiveCaret == mFirstCaret.get() && result == 1) ||
      (mActiveCaret == mSecondCaret.get() && result == -1)) {
    aOffsets.content = pos.mResultContent;
    aOffsets.offset = pos.mContentOffset;
    aOffsets.secondaryOffset = pos.mContentOffset;
  }

  return true;
}

nsresult
AccessibleCaretManager::DragCaretInternal(const nsPoint& aPoint)
{
  if (!mPresShell) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  if (!rootFrame) {
    return NS_ERROR_NULL_POINTER;
  }

  nsPoint point = AdjustDragBoundary(aPoint);

  
  nsIFrame* ptFrame = nsLayoutUtils::GetFrameForPoint(
    rootFrame, point,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC);
  if (!ptFrame) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  if (!fs) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult result;
  nsIFrame* newFrame = nullptr;
  nsPoint newPoint;
  nsPoint ptInFrame = point;
  nsLayoutUtils::TransformPoint(rootFrame, ptFrame, ptInFrame);
  result = fs->ConstrainFrameAndPointToAnchorSubtree(ptFrame, ptInFrame,
                                                     &newFrame, newPoint);
  if (NS_FAILED(result) || !newFrame) {
    return NS_ERROR_FAILURE;
  }

  bool selectable;
  newFrame->IsSelectable(&selectable, nullptr);
  if (!selectable) {
    return NS_ERROR_FAILURE;
  }

  nsIFrame::ContentOffsets offsets =
    newFrame->GetContentOffsetsFromPoint(newPoint);
  if (!offsets.content) {
    return NS_ERROR_FAILURE;
  }

  Selection* selection = GetSelection();
  if (!selection) {
    return NS_ERROR_NULL_POINTER;
  }

  if (GetCaretMode() == CaretMode::Selection &&
      !CompareRangeWithContentOffset(offsets)) {
    return NS_ERROR_FAILURE;
  }

  ClearMaintainedSelection();

  nsIFrame* anchorFrame = nullptr;
  selection->GetPrimaryFrameForAnchorNode(&anchorFrame);

  nsIFrame* scrollable =
    nsLayoutUtils::GetClosestFrameOfType(anchorFrame, nsGkAtoms::scrollFrame);
  nsWeakFrame weakScrollable = scrollable;
  fs->HandleClick(offsets.content, offsets.StartOffset(), offsets.EndOffset(),
                  GetCaretMode() == CaretMode::Selection, false,
                  offsets.associate);
  if (!weakScrollable.IsAlive()) {
    return NS_OK;
  }

  
  nsIScrollableFrame* saf = do_QueryFrame(scrollable);
  nsIFrame* capturingFrame = saf->GetScrolledFrame();
  nsPoint ptInScrolled = point;
  nsLayoutUtils::TransformPoint(rootFrame, capturingFrame, ptInScrolled);
  fs->StartAutoScrollTimer(capturingFrame, ptInScrolled, kAutoScrollTimerDelay);
  return NS_OK;
}

nsPoint
AccessibleCaretManager::AdjustDragBoundary(const nsPoint& aPoint) const
{
  
  
  
  
  
  
  
  nsPoint adjustedPoint = aPoint;

  if (GetCaretMode() == CaretMode::Selection) {
    if (mActiveCaret == mFirstCaret.get()) {
      nscoord dragDownBoundaryY = mSecondCaret->LogicalPosition().y;
      if (adjustedPoint.y > dragDownBoundaryY) {
        adjustedPoint.y = dragDownBoundaryY;
      }
    } else {
      nscoord dragUpBoundaryY = mFirstCaret->LogicalPosition().y;
      if (adjustedPoint.y < dragUpBoundaryY) {
        adjustedPoint.y = dragUpBoundaryY;
      }
    }
  }

  return adjustedPoint;
}

uint32_t
AccessibleCaretManager::CaretTimeoutMs() const
{
  static bool added = false;
  static uint32_t caretTimeoutMs = 0;

  if (!added) {
    Preferences::AddUintVarCache(&caretTimeoutMs,
                                 "layout.accessiblecaret.timeout_ms");
    added = true;
  }

  return caretTimeoutMs;
}

void
AccessibleCaretManager::LaunchCaretTimeoutTimer()
{
  if (!mCaretTimeoutTimer || CaretTimeoutMs() == 0 ||
      GetCaretMode() != CaretMode::Cursor || mActiveCaret) {
    return;
  }

  nsTimerCallbackFunc callback = [](nsITimer* aTimer, void* aClosure) {
    auto self = static_cast<AccessibleCaretManager*>(aClosure);
    if (self->GetCaretMode() == CaretMode::Cursor) {
      self->HideCarets();
    }
  };

  mCaretTimeoutTimer->InitWithFuncCallback(callback, this, CaretTimeoutMs(),
                                           nsITimer::TYPE_ONE_SHOT);
}

void
AccessibleCaretManager::CancelCaretTimeoutTimer()
{
  if (mCaretTimeoutTimer) {
    mCaretTimeoutTimer->Cancel();
  }
}

} 
