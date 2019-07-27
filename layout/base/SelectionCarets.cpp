





#include "SelectionCarets.h"

#include "gfxPrefs.h"
#include "nsBidiPresUtils.h"
#include "nsCanvasFrame.h"
#include "nsCaret.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsDOMTokenList.h"
#include "nsFocusManager.h"
#include "nsFrame.h"
#include "nsIDocument.h"
#include "nsIDocShell.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNodeFilter.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsRect.h"
#include "nsView.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/ScrollViewChangeEvent.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/dom/TreeWalker.h"
#include "mozilla/Preferences.h"
#include "mozilla/TouchEvents.h"
#include "TouchCaret.h"
#include "nsFrameSelection.h"

using namespace mozilla;
using namespace mozilla::dom;



static const int32_t kMoveStartTolerancePx = 5;

static const int32_t kScrollEndTimerDelay = 300;






static bool kSupportNonEditableFields = false;

NS_IMPL_ISUPPORTS(SelectionCarets,
                  nsISelectionListener,
                  nsIScrollObserver,
                  nsISupportsWeakReference)

 int32_t SelectionCarets::sSelectionCaretsInflateSize = 0;

SelectionCarets::SelectionCarets(nsIPresShell *aPresShell)
  : mActiveTouchId(-1)
  , mCaretCenterToDownPointOffsetY(0)
  , mDragMode(NONE)
  , mAPZenabled(false)
  , mEndCaretVisible(false)
  , mStartCaretVisible(false)
  , mVisible(false)
{
  MOZ_ASSERT(NS_IsMainThread());

  static bool addedPref = false;
  if (!addedPref) {
    Preferences::AddIntVarCache(&sSelectionCaretsInflateSize,
                                "selectioncaret.inflatesize.threshold");
    Preferences::AddBoolVarCache(&kSupportNonEditableFields,
                                 "selectioncaret.noneditable");
    addedPref = true;
  }

  mPresShell = aPresShell;
}

SelectionCarets::~SelectionCarets()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mLongTapDetectorTimer) {
    mLongTapDetectorTimer->Cancel();
    mLongTapDetectorTimer = nullptr;
  }

  if (mScrollEndDetectorTimer) {
    mScrollEndDetectorTimer->Cancel();
    mScrollEndDetectorTimer = nullptr;
  }

  mPresShell = nullptr;
}

nsEventStatus
SelectionCarets::HandleEvent(WidgetEvent* aEvent)
{
  WidgetMouseEvent *mouseEvent = aEvent->AsMouseEvent();
  if (mouseEvent && mouseEvent->reason == WidgetMouseEvent::eSynthesized) {
    return nsEventStatus_eIgnore;
  }

  WidgetTouchEvent *touchEvent = aEvent->AsTouchEvent();
  nsIntPoint movePoint;
  int32_t nowTouchId = -1;
  if (touchEvent && !touchEvent->touches.IsEmpty()) {
    
    if (mActiveTouchId >= 0) {
      for (uint32_t i = 0; i < touchEvent->touches.Length(); ++i) {
        if (touchEvent->touches[i]->Identifier() == mActiveTouchId) {
          movePoint = touchEvent->touches[i]->mRefPoint;
          nowTouchId = touchEvent->touches[i]->Identifier();
          break;
        }
      }

      
      if (nowTouchId == -1) {
        return nsEventStatus_eConsumeNoDefault;
      }
    } else {
      movePoint = touchEvent->touches[0]->mRefPoint;
      nowTouchId = touchEvent->touches[0]->Identifier();
    }
  } else if (mouseEvent) {
    movePoint = LayoutDeviceIntPoint::ToUntyped(mouseEvent->AsGUIEvent()->refPoint);
  }

  
  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();
  if (!canvasFrame) {
    return nsEventStatus_eIgnore;
  }
  nsPoint ptInCanvas =
    nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, movePoint, canvasFrame);

  if (aEvent->message == NS_TOUCH_START ||
      (aEvent->message == NS_MOUSE_BUTTON_DOWN &&
       mouseEvent->button == WidgetMouseEvent::eLeftButton)) {
    
    if (aEvent->message == NS_TOUCH_START && mActiveTouchId >= 0) {
      return nsEventStatus_eConsumeNoDefault;
    }

    mActiveTouchId = nowTouchId;
    mDownPoint = ptInCanvas;
    if (IsOnStartFrame(ptInCanvas)) {
      mDragMode = START_FRAME;
      mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - ptInCanvas.y;
      SetSelectionDirection(false);
      SetSelectionDragState(true);
      return nsEventStatus_eConsumeNoDefault;
    } else if (IsOnEndFrame(ptInCanvas)) {
      mDragMode = END_FRAME;
      mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - ptInCanvas.y;
      SetSelectionDirection(true);
      SetSelectionDragState(true);
      return nsEventStatus_eConsumeNoDefault;
    } else {
      mDragMode = NONE;
      mActiveTouchId = -1;
      SetVisibility(false);
      LaunchLongTapDetector();
    }
  } else if (aEvent->message == NS_TOUCH_END ||
             aEvent->message == NS_TOUCH_CANCEL ||
             aEvent->message == NS_MOUSE_BUTTON_UP) {
    CancelLongTapDetector();
    if (mDragMode != NONE) {
      
      if (mActiveTouchId == nowTouchId) {
        SetSelectionDragState(false);
        mDragMode = NONE;
        mActiveTouchId = -1;
      }
      return nsEventStatus_eConsumeNoDefault;
    }
  } else if (aEvent->message == NS_TOUCH_MOVE ||
             aEvent->message == NS_MOUSE_MOVE) {
    if (mDragMode == START_FRAME || mDragMode == END_FRAME) {
      if (mActiveTouchId == nowTouchId) {
        ptInCanvas.y += mCaretCenterToDownPointOffsetY;
        return DragSelection(ptInCanvas);
      }

      return nsEventStatus_eConsumeNoDefault;
    }

    nsPoint delta = mDownPoint - ptInCanvas;
    if (NS_hypot(delta.x, delta.y) >
          nsPresContext::AppUnitsPerCSSPixel() * kMoveStartTolerancePx) {
      CancelLongTapDetector();
    }
  } else if (aEvent->message == NS_MOUSE_MOZLONGTAP) {
    if (!mVisible) {
      SelectWord();
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

static void
SetElementVisibility(dom::Element* aElement, bool aVisible)
{
  if (!aElement) {
    return;
  }

  ErrorResult err;
  aElement->ClassList()->Toggle(NS_LITERAL_STRING("hidden"),
                                   dom::Optional<bool>(!aVisible), err);
}

void
SelectionCarets::SetVisibility(bool aVisible)
{
  if (!mPresShell) {
    return;
  }

  if (mVisible == aVisible) {
    return;
  }
  mVisible = aVisible;

  dom::Element* startElement = mPresShell->GetSelectionCaretsStartElement();
  SetElementVisibility(startElement, mVisible && mStartCaretVisible);

  dom::Element* endElement = mPresShell->GetSelectionCaretsEndElement();
  SetElementVisibility(endElement, mVisible && mEndCaretVisible);

  
  
  
  
  mPresShell->SetMayHaveTouchCaret(mVisible);
}

void
SelectionCarets::SetStartFrameVisibility(bool aVisible)
{
  mStartCaretVisible = aVisible;
  dom::Element* element = mPresShell->GetSelectionCaretsStartElement();
  SetElementVisibility(element, mVisible && mStartCaretVisible);
}

void
SelectionCarets::SetEndFrameVisibility(bool aVisible)
{
  mEndCaretVisible = aVisible;
  dom::Element* element = mPresShell->GetSelectionCaretsEndElement();
  SetElementVisibility(element, mVisible && mEndCaretVisible);
}

void
SelectionCarets::SetTilted(bool aIsTilt)
{
  dom::Element* startElement = mPresShell->GetSelectionCaretsStartElement();
  dom::Element* endElement = mPresShell->GetSelectionCaretsEndElement();

  if (!startElement || !endElement) {
    return;
  }

  ErrorResult err;
  startElement->ClassList()->Toggle(NS_LITERAL_STRING("tilt"),
                                       dom::Optional<bool>(aIsTilt), err);

  endElement->ClassList()->Toggle(NS_LITERAL_STRING("tilt"),
                                     dom::Optional<bool>(aIsTilt), err);
}

static void
SetCaretDirection(dom::Element* aElement, bool aIsRight)
{
  MOZ_ASSERT(aElement);

  ErrorResult err;
  if (aIsRight) {
    aElement->ClassList()->Add(NS_LITERAL_STRING("moz-selectioncaret-right"), err);
    aElement->ClassList()->Remove(NS_LITERAL_STRING("moz-selectioncaret-left"), err);
  } else {
    aElement->ClassList()->Add(NS_LITERAL_STRING("moz-selectioncaret-left"), err);
    aElement->ClassList()->Remove(NS_LITERAL_STRING("moz-selectioncaret-right"), err);
  }
}

static bool
IsRightToLeft(nsIFrame* aFrame)
{
  MOZ_ASSERT(aFrame);

  return aFrame->IsFrameOfType(nsIFrame::eLineParticipant) ?
    (nsBidiPresUtils::GetFrameEmbeddingLevel(aFrame) & 1) :
    aFrame->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL;
}





static void
ReduceRectToVerticalEdge(nsRect& aRect, bool aToRightEdge)
{
  if (aToRightEdge) {
    aRect.x = aRect.XMost() - AppUnitsPerCSSPixel();
  }
  aRect.width = AppUnitsPerCSSPixel();
}

static nsIFrame*
FindFirstNodeWithFrame(nsIDocument* aDocument,
                       nsRange* aRange,
                       nsFrameSelection* aFrameSelection,
                       bool aBackward,
                       int& aOutOffset)
{
  if (!aDocument || !aRange || !aFrameSelection) {
    return nullptr;
  }

  nsCOMPtr<nsINode> startNode =
    do_QueryInterface(aBackward ? aRange->GetEndParent() : aRange->GetStartParent());
  nsCOMPtr<nsINode> endNode =
    do_QueryInterface(aBackward ? aRange->GetStartParent() : aRange->GetEndParent());
  int32_t offset = aBackward ? aRange->EndOffset() : aRange->StartOffset();

  nsCOMPtr<nsIContent> startContent = do_QueryInterface(startNode);
  CaretAssociationHint hintStart =
    aBackward ? CARET_ASSOCIATE_BEFORE : CARET_ASSOCIATE_AFTER;
  nsIFrame* startFrame = aFrameSelection->GetFrameForNodeOffset(startContent,
                                                                offset,
                                                                hintStart,
                                                                &aOutOffset);

  if (startFrame) {
    return startFrame;
  }

  ErrorResult err;
  nsRefPtr<dom::TreeWalker> walker =
    aDocument->CreateTreeWalker(*startNode,
                                nsIDOMNodeFilter::SHOW_ALL,
                                nullptr,
                                err);

  if (!walker) {
    return nullptr;
  }

  startFrame = startContent ? startContent->GetPrimaryFrame() : nullptr;
  while (!startFrame && startNode != endNode) {
    if (aBackward) {
      startNode = walker->PreviousNode(err);
    } else {
      startNode = walker->NextNode(err);
    }

    if (!startNode) {
      break;
    }

    startContent = do_QueryInterface(startNode);
    startFrame = startContent ? startContent->GetPrimaryFrame() : nullptr;
  }
  return startFrame;
}

void
SelectionCarets::UpdateSelectionCarets()
{
  if (!mPresShell) {
    return;
  }

  nsRefPtr<dom::Selection> selection = GetSelection();
  if (!selection) {
    SetVisibility(false);
    return;
  }

  if (selection->GetRangeCount() <= 0) {
    SetVisibility(false);
    return;
  }

  nsRefPtr<nsRange> range = selection->GetRangeAt(0);
  if (range->Collapsed()) {
    SetVisibility(false);
    return;
  }

  nsLayoutUtils::FirstAndLastRectCollector collector;
  nsRange::CollectClientRects(&collector, range,
                              range->GetStartParent(), range->StartOffset(),
                              range->GetEndParent(), range->EndOffset(), true, true);

  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();
  nsIFrame* rootFrame = mPresShell->GetRootFrame();

  if (!canvasFrame || !rootFrame) {
    SetVisibility(false);
    return;
  }

  
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  int32_t startOffset;
  nsIFrame* startFrame = FindFirstNodeWithFrame(mPresShell->GetDocument(),
                                                range, fs, false, startOffset);

  int32_t endOffset;
  nsIFrame* endFrame = FindFirstNodeWithFrame(mPresShell->GetDocument(),
                                              range, fs, true, endOffset);

  if (!startFrame || !endFrame) {
    SetVisibility(false);
    return;
  }

  
  
  if (!kSupportNonEditableFields &&
      (!startFrame->GetContent()->IsEditable() ||
       !endFrame->GetContent()->IsEditable())) {
    return;
  }

  
  if (nsLayoutUtils::CompareTreePosition(startFrame, endFrame) > 0) {
    SetVisibility(false);
    return;
  }

  bool startFrameIsRTL = IsRightToLeft(startFrame);
  bool endFrameIsRTL = IsRightToLeft(endFrame);

  
  
  ReduceRectToVerticalEdge(collector.mFirstRect, startFrameIsRTL);

  
  
  ReduceRectToVerticalEdge(collector.mLastRect, !endFrameIsRTL);

  nsLayoutUtils::TransformRect(rootFrame, canvasFrame, collector.mFirstRect);
  nsLayoutUtils::TransformRect(rootFrame, canvasFrame, collector.mLastRect);

  nsAutoTArray<nsIFrame*, 16> hitFramesInFirstRect;
  nsLayoutUtils::GetFramesForArea(canvasFrame,
    collector.mFirstRect,
    hitFramesInFirstRect,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION |
      nsLayoutUtils::IGNORE_CROSS_DOC |
      nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME);

  nsAutoTArray<nsIFrame*, 16> hitFramesInLastRect;
  nsLayoutUtils::GetFramesForArea(canvasFrame,
    collector.mLastRect,
    hitFramesInLastRect,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION |
      nsLayoutUtils::IGNORE_CROSS_DOC |
      nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME);

  SetStartFrameVisibility(hitFramesInFirstRect.Contains(startFrame));
  SetEndFrameVisibility(hitFramesInLastRect.Contains(endFrame));

  SetStartFramePos(collector.mFirstRect.BottomLeft());
  SetEndFramePos(collector.mLastRect.BottomRight());
  SetVisibility(true);

  
  bool isTilt = false;
  if (startFrame && endFrame) {
    
    
    
    
    
    
    
    
    nsPeekOffsetStruct posNext(eSelectCluster,
                               eDirNext,
                               startOffset,
                               0,
                               false,
                               true,  
                               false,
                               false);

    nsPeekOffsetStruct posPrev(eSelectCluster,
                               eDirPrevious,
                               endOffset,
                               0,
                               false,
                               true,  
                               false,
                               false);
    startFrame->PeekOffset(&posNext);
    endFrame->PeekOffset(&posPrev);

    if (posNext.mResultContent && posPrev.mResultContent &&
        nsContentUtils::ComparePoints(posNext.mResultContent, posNext.mContentOffset,
                                      posPrev.mResultContent, posPrev.mContentOffset) > 0) {
      isTilt = true;
    }
  }

  SetCaretDirection(mPresShell->GetSelectionCaretsStartElement(), startFrameIsRTL);
  SetCaretDirection(mPresShell->GetSelectionCaretsEndElement(), !endFrameIsRTL);
  SetTilted(isTilt);
}

nsresult
SelectionCarets::SelectWord()
{
  if (!mPresShell) {
    return NS_OK;
  }

  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();
  if (!canvasFrame) {
    return NS_OK;
  }

  
  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(canvasFrame, mDownPoint,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC);
  if (!ptFrame) {
    return NS_OK;
  }

  
  
  if (!kSupportNonEditableFields && !ptFrame->GetContent()->IsEditable()) {
    return NS_OK;
  }

  nsPoint ptInFrame = mDownPoint;
  nsLayoutUtils::TransformPoint(canvasFrame, ptFrame, ptInFrame);

  
  
  
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  nsIContent* editingHost = ptFrame->GetContent()->GetEditingHost();
  if (editingHost) {
    nsCOMPtr<nsIDOMElement> elt = do_QueryInterface(editingHost->GetParent());
    if (elt) {
      fm->SetFocus(elt, 0);
    }
  } else {
    nsIContent* focusedContent = GetFocusedContent();
    if (focusedContent && focusedContent->GetTextEditorRootContent()) {
      nsIDOMWindow* win = mPresShell->GetDocument()->GetWindow();
      if (win) {
        fm->ClearFocus(win);
      }
    }
  }

  SetSelectionDragState(true);
  nsFrame* frame = static_cast<nsFrame*>(ptFrame);
  nsresult rs = frame->SelectByTypeAtPoint(mPresShell->GetPresContext(), ptInFrame,
                                           eSelectWord, eSelectWord, 0);
  SetSelectionDragState(false);

  
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  fs->MaintainSelection();
  return rs;
}






static bool
CompareRangeWithContentOffset(nsRange* aRange,
                              nsFrameSelection* aSelection,
                              nsIFrame::ContentOffsets& aOffsets,
                              SelectionCarets::DragMode aDragMode)
{
  MOZ_ASSERT(aDragMode != SelectionCarets::NONE);
  nsINode* node = nullptr;
  int32_t nodeOffset = 0;
  CaretAssociationHint hint;
  nsDirection dir;

  if (aDragMode == SelectionCarets::START_FRAME) {
    
    node = aRange->GetEndParent();
    nodeOffset = aRange->EndOffset();
    hint = CARET_ASSOCIATE_BEFORE;
    dir = eDirPrevious;
  } else {
    
    node = aRange->GetStartParent();
    nodeOffset = aRange->StartOffset();
    hint =  CARET_ASSOCIATE_AFTER;
    dir = eDirNext;
  }
  nsCOMPtr<nsIContent> content = do_QueryInterface(node);

  int32_t offset = 0;
  nsIFrame* theFrame =
    aSelection->GetFrameForNodeOffset(content, nodeOffset, hint, &offset);

  if (!theFrame) {
    return false;
  }

  
  nsPeekOffsetStruct pos(eSelectCluster,
                         dir,
                         offset,
                         0,
                         true,
                         true,  
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
  if ((aDragMode == SelectionCarets::START_FRAME && result == 1) ||
      (aDragMode == SelectionCarets::END_FRAME && result == -1)) {
    aOffsets.content = pos.mResultContent;
    aOffsets.offset = pos.mContentOffset;
    aOffsets.secondaryOffset = pos.mContentOffset;
  }

  return true;
}

nsEventStatus
SelectionCarets::DragSelection(const nsPoint &movePoint)
{
  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();
  if (!canvasFrame) {
    return nsEventStatus_eConsumeNoDefault;
  }

  
  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(canvasFrame, movePoint,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC);
  if (!ptFrame) {
    return nsEventStatus_eConsumeNoDefault;
  }

  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();

  nsresult result;
  nsIFrame *newFrame = nullptr;
  nsPoint newPoint;
  nsPoint ptInFrame = movePoint;
  nsLayoutUtils::TransformPoint(canvasFrame, ptFrame, ptInFrame);
  result = fs->ConstrainFrameAndPointToAnchorSubtree(ptFrame, ptInFrame, &newFrame, newPoint);
  if (NS_FAILED(result) || !newFrame) {
    return nsEventStatus_eConsumeNoDefault;
  }

  bool selectable;
  newFrame->IsSelectable(&selectable, nullptr);
  if (!selectable) {
    return nsEventStatus_eConsumeNoDefault;
  }

  nsFrame::ContentOffsets offsets =
    newFrame->GetContentOffsetsFromPoint(newPoint);
  if (!offsets.content) {
    return nsEventStatus_eConsumeNoDefault;
  }

  nsRefPtr<dom::Selection> selection = GetSelection();
  if (selection->GetRangeCount() <= 0) {
    return nsEventStatus_eConsumeNoDefault;
  }

  nsRefPtr<nsRange> range = selection->GetRangeAt(0);
  if (!CompareRangeWithContentOffset(range, fs, offsets, mDragMode)) {
    return nsEventStatus_eConsumeNoDefault;
  }

  nsIFrame* anchorFrame;
  selection->GetPrimaryFrameForAnchorNode(&anchorFrame);
  if (!anchorFrame) {
    return nsEventStatus_eConsumeNoDefault;
  }

  
  nsIFrame *scrollable =
    nsLayoutUtils::GetClosestFrameOfType(anchorFrame, nsGkAtoms::scrollFrame);
  nsWeakFrame weakScrollable = scrollable;
  fs->HandleClick(offsets.content, offsets.StartOffset(),
                  offsets.EndOffset(),
                  true,
                  false,
                  offsets.associate);
  if (!weakScrollable.IsAlive()) {
    return nsEventStatus_eConsumeNoDefault;
  }

  
  nsIScrollableFrame *saf = do_QueryFrame(scrollable);
  nsIFrame *capturingFrame = saf->GetScrolledFrame();
  nsPoint ptInScrolled = movePoint;
  nsLayoutUtils::TransformPoint(canvasFrame, capturingFrame, ptInScrolled);
  fs->StartAutoScrollTimer(capturingFrame, ptInScrolled, TouchCaret::sAutoScrollTimerDelay);
  UpdateSelectionCarets();
  return nsEventStatus_eConsumeNoDefault;
}

nscoord
SelectionCarets::GetCaretYCenterPosition()
{
  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();

  if (!canvasFrame) {
    return 0;
  }

  nsRefPtr<dom::Selection> selection = GetSelection();
  if (selection->GetRangeCount() <= 0) {
    return 0;
  }

  nsRefPtr<nsRange> range = selection->GetRangeAt(0);
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();

  MOZ_ASSERT(mDragMode != NONE);
  nsCOMPtr<nsIContent> node;
  uint32_t nodeOffset;
  if (mDragMode == START_FRAME) {
    node = do_QueryInterface(range->GetStartParent());
    nodeOffset = range->StartOffset();
  } else {
    node = do_QueryInterface(range->GetEndParent());
    nodeOffset = range->EndOffset();
  }

  int32_t offset;
  CaretAssociationHint hint =
    mDragMode == START_FRAME ? CARET_ASSOCIATE_AFTER : CARET_ASSOCIATE_BEFORE;
  nsIFrame* theFrame =
    fs->GetFrameForNodeOffset(node, nodeOffset, hint, &offset);

  if (!theFrame) {
    return 0;
  }
  nsRect frameRect = theFrame->GetRectRelativeToSelf();
  nsLayoutUtils::TransformRect(theFrame, canvasFrame, frameRect);
  return frameRect.Center().y;
}

void
SelectionCarets::SetSelectionDragState(bool aState)
{
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  fs->SetDragState(aState);
}

void
SelectionCarets::SetSelectionDirection(bool aForward)
{
  nsRefPtr<dom::Selection> selection = GetSelection();
  selection->SetDirection(aForward ? eDirNext : eDirPrevious);
}

static void
SetFramePos(dom::Element* aElement, const nsPoint& aPosition)
{
  if (!aElement) {
    return;
  }

  nsAutoString styleStr;
  styleStr.AppendLiteral("left:");
  styleStr.AppendFloat(nsPresContext::AppUnitsToFloatCSSPixels(aPosition.x));
  styleStr.AppendLiteral("px;top:");
  styleStr.AppendFloat(nsPresContext::AppUnitsToFloatCSSPixels(aPosition.y));
  styleStr.AppendLiteral("px;");

  aElement->SetAttr(kNameSpaceID_None, nsGkAtoms::style, styleStr, true);
}

void
SelectionCarets::SetStartFramePos(const nsPoint& aPosition)
{
  SetFramePos(mPresShell->GetSelectionCaretsStartElement(), aPosition);
}

void
SelectionCarets::SetEndFramePos(const nsPoint& aPosition)
{
  SetFramePos(mPresShell->GetSelectionCaretsEndElement(), aPosition);
}

bool
SelectionCarets::IsOnStartFrame(const nsPoint& aPosition)
{
  return mVisible &&
    nsLayoutUtils::ContainsPoint(GetStartFrameRect(), aPosition,
                                 SelectionCaretsInflateSize());
}

bool
SelectionCarets::IsOnEndFrame(const nsPoint& aPosition)
{
  return mVisible &&
    nsLayoutUtils::ContainsPoint(GetEndFrameRect(), aPosition,
                                 SelectionCaretsInflateSize());
}

nsRect
SelectionCarets::GetStartFrameRect()
{
  dom::Element* element = mPresShell->GetSelectionCaretsStartElement();
  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();
  return nsLayoutUtils::GetRectRelativeToFrame(element, canvasFrame);
}

nsRect
SelectionCarets::GetEndFrameRect()
{
  dom::Element* element = mPresShell->GetSelectionCaretsEndElement();
  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();
  return nsLayoutUtils::GetRectRelativeToFrame(element, canvasFrame);
}

nsIContent*
SelectionCarets::GetFocusedContent()
{
  nsFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    return fm->GetFocusedContent();
  }

  return nullptr;
}

Selection*
SelectionCarets::GetSelection()
{
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  if (fs) {
    return fs->GetSelection(nsISelectionController::SELECTION_NORMAL);
  }
  return nullptr;
}

already_AddRefed<nsFrameSelection>
SelectionCarets::GetFrameSelection()
{
  nsIContent* focusNode = GetFocusedContent();
  if (focusNode) {
    nsIFrame* focusFrame = focusNode->GetPrimaryFrame();
    if (!focusFrame) {
      return nullptr;
    }
    return focusFrame->GetFrameSelection();
  } else {
    return mPresShell->FrameSelection();
  }
}

nsresult
SelectionCarets::NotifySelectionChanged(nsIDOMDocument* aDoc,
                                       nsISelection* aSel,
                                       int16_t aReason)
{
  bool isCollapsed;
  aSel->GetIsCollapsed(&isCollapsed);
  if (isCollapsed) {
    SetVisibility(false);
    return NS_OK;
  }
  if (!aReason || (aReason & (nsISelectionListener::DRAG_REASON |
                               nsISelectionListener::KEYPRESS_REASON |
                               nsISelectionListener::MOUSEDOWN_REASON))) {
    SetVisibility(false);
  } else {
    UpdateSelectionCarets();
  }
  return NS_OK;
}

static void
DispatchScrollViewChangeEvent(nsIPresShell *aPresShell, const dom::ScrollState aState, const mozilla::CSSIntPoint aScrollPos)
{
  nsCOMPtr<nsIDocument> doc = aPresShell->GetDocument();
  if (doc) {
    bool ret;
    ScrollViewChangeEventInit detail;
    detail.mBubbles = true;
    detail.mCancelable = false;
    detail.mState = aState;
    detail.mScrollX = aScrollPos.x;
    detail.mScrollY = aScrollPos.y;
    nsRefPtr<ScrollViewChangeEvent> event =
      ScrollViewChangeEvent::Constructor(doc, NS_LITERAL_STRING("scrollviewchange"), detail);

    event->SetTrusted(true);
    event->GetInternalNSEvent()->mFlags.mOnlyChromeDispatch = true;
    doc->DispatchEvent(event, &ret);
  }
}

void
SelectionCarets::AsyncPanZoomStarted(const mozilla::CSSIntPoint aScrollPos)
{
  
  
  mAPZenabled = true;
  SetVisibility(false);
  DispatchScrollViewChangeEvent(mPresShell, dom::ScrollState::Started, aScrollPos);
}

void
SelectionCarets::AsyncPanZoomStopped(const mozilla::CSSIntPoint aScrollPos)
{
  UpdateSelectionCarets();
  DispatchScrollViewChangeEvent(mPresShell, dom::ScrollState::Stopped, aScrollPos);
}

void
SelectionCarets::ScrollPositionChanged()
{
  if (!mAPZenabled && mVisible) {
    SetVisibility(false);
    
    LaunchScrollEndDetector();
  }
}

void
SelectionCarets::LaunchLongTapDetector()
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return;
  }

  if (!mLongTapDetectorTimer) {
    mLongTapDetectorTimer = do_CreateInstance("@mozilla.org/timer;1");
  }

  MOZ_ASSERT(mLongTapDetectorTimer);
  CancelLongTapDetector();
  int32_t longTapDelay = gfxPrefs::UiClickHoldContextMenusDelay();
  mLongTapDetectorTimer->InitWithFuncCallback(FireLongTap,
                                              this,
                                              longTapDelay,
                                              nsITimer::TYPE_ONE_SHOT);
}

void
SelectionCarets::CancelLongTapDetector()
{
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    return;
  }

  if (!mLongTapDetectorTimer) {
    return;
  }

  mLongTapDetectorTimer->Cancel();
}

void
SelectionCarets::FireLongTap(nsITimer* aTimer, void* aSelectionCarets)
{
  nsRefPtr<SelectionCarets> self = static_cast<SelectionCarets*>(aSelectionCarets);
  NS_PRECONDITION(aTimer == self->mLongTapDetectorTimer,
                  "Unexpected timer");

  self->SelectWord();
}

void
SelectionCarets::LaunchScrollEndDetector()
{
  if (!mScrollEndDetectorTimer) {
    mScrollEndDetectorTimer = do_CreateInstance("@mozilla.org/timer;1");
  }

  MOZ_ASSERT(mScrollEndDetectorTimer);
  mScrollEndDetectorTimer->InitWithFuncCallback(FireScrollEnd,
                                                this,
                                                kScrollEndTimerDelay,
                                                nsITimer::TYPE_ONE_SHOT);
}

void
SelectionCarets::FireScrollEnd(nsITimer* aTimer, void* aSelectionCarets)
{
  nsRefPtr<SelectionCarets> self = static_cast<SelectionCarets*>(aSelectionCarets);
  NS_PRECONDITION(aTimer == self->mScrollEndDetectorTimer,
                  "Unexpected timer");
  self->SetVisibility(true);
  self->UpdateSelectionCarets();
}
