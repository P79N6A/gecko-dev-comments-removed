





#include "prlog.h"
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

#ifdef PR_LOGGING
static PRLogModuleInfo* gSelectionCaretsLog;
static const char* kSelectionCaretsLogModuleName = "SelectionCarets";



#define SELECTIONCARETS_LOG(message, ...)                                      \
  PR_LOG(gSelectionCaretsLog, PR_LOG_DEBUG,                                    \
         ("SelectionCarets (%p): %s:%d : " message "\n", this, __FUNCTION__,   \
          __LINE__, ##__VA_ARGS__));

#define SELECTIONCARETS_LOG_STATIC(message, ...)                               \
  PR_LOG(gSelectionCaretsLog, PR_LOG_DEBUG,                                    \
         ("SelectionCarets: %s:%d : " message "\n", __FUNCTION__, __LINE__,    \
          ##__VA_ARGS__));
#else
#define SELECTIONCARETS_LOG(message, ...)
#define SELECTIONCARETS_LOG_STATIC(message, ...)
#endif 



static const int32_t kMoveStartTolerancePx = 5;

static const int32_t kScrollEndTimerDelay = 300;






static bool kSupportNonEditableFields = false;

NS_IMPL_ISUPPORTS(SelectionCarets,
                  nsIReflowObserver,
                  nsISelectionListener,
                  nsIScrollObserver,
                  nsISupportsWeakReference)

 int32_t SelectionCarets::sSelectionCaretsInflateSize = 0;

SelectionCarets::SelectionCarets(nsIPresShell* aPresShell)
  : mPresShell(aPresShell)
  , mActiveTouchId(-1)
  , mCaretCenterToDownPointOffsetY(0)
  , mDragMode(NONE)
  , mAsyncPanZoomEnabled(false)
  , mEndCaretVisible(false)
  , mStartCaretVisible(false)
  , mVisible(false)
{
  MOZ_ASSERT(NS_IsMainThread());

#ifdef PR_LOGGING
  if (!gSelectionCaretsLog) {
    gSelectionCaretsLog = PR_NewLogModule(kSelectionCaretsLogModuleName);
  }
#endif

  SELECTIONCARETS_LOG("Constructor, PresShell=%p", mPresShell);

  static bool addedPref = false;
  if (!addedPref) {
    Preferences::AddIntVarCache(&sSelectionCaretsInflateSize,
                                "selectioncaret.inflatesize.threshold");
    Preferences::AddBoolVarCache(&kSupportNonEditableFields,
                                 "selectioncaret.noneditable");
    addedPref = true;
  }
}

void
SelectionCarets::Init()
{
  nsPresContext* presContext = mPresShell->GetPresContext();
  MOZ_ASSERT(presContext, "PresContext should be given in PresShell::Init()");

  nsIDocShell* docShell = presContext->GetDocShell();
  if (!docShell) {
    return;
  }

  docShell->GetAsyncPanZoomEnabled(&mAsyncPanZoomEnabled);
  mAsyncPanZoomEnabled = mAsyncPanZoomEnabled && gfxPrefs::AsyncPanZoomEnabled();

  docShell->AddWeakReflowObserver(this);
  docShell->AddWeakScrollObserver(this);
}

SelectionCarets::~SelectionCarets()
{
  SELECTIONCARETS_LOG("Destructor");
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

void
SelectionCarets::Terminate()
{
  nsPresContext* presContext = mPresShell->GetPresContext();
  MOZ_ASSERT(presContext, "PresContext should be given in PresShell::Init()");

  nsIDocShell* docShell = presContext->GetDocShell();
  if (docShell) {
    docShell->RemoveWeakReflowObserver(this);
    docShell->RemoveWeakScrollObserver(this);
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

  
  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  if (!rootFrame) {
    return nsEventStatus_eIgnore;
  }
  nsPoint ptInRoot =
    nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, movePoint, rootFrame);

  if (aEvent->message == NS_TOUCH_START ||
      (aEvent->message == NS_MOUSE_BUTTON_DOWN &&
       mouseEvent->button == WidgetMouseEvent::eLeftButton)) {
    
    if (aEvent->message == NS_TOUCH_START && mActiveTouchId >= 0) {
      return nsEventStatus_eConsumeNoDefault;
    }

    mActiveTouchId = nowTouchId;
    mDownPoint = ptInRoot;
    if (IsOnStartFrame(ptInRoot)) {
      mDragMode = START_FRAME;
      mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - ptInRoot.y;
      SetSelectionDirection(false);
      SetSelectionDragState(true);
      return nsEventStatus_eConsumeNoDefault;
    } else if (IsOnEndFrame(ptInRoot)) {
      mDragMode = END_FRAME;
      mCaretCenterToDownPointOffsetY = GetCaretYCenterPosition() - ptInRoot.y;
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
        ptInRoot.y += mCaretCenterToDownPointOffsetY;
        return DragSelection(ptInRoot);
      }

      return nsEventStatus_eConsumeNoDefault;
    }

    nsPoint delta = mDownPoint - ptInRoot;
    if (NS_hypot(delta.x, delta.y) >
          nsPresContext::AppUnitsPerCSSPixel() * kMoveStartTolerancePx) {
      CancelLongTapDetector();
    }
  } else if (aEvent->message == NS_MOUSE_MOZLONGTAP) {
    if (!mVisible) {
      SELECTIONCARETS_LOG("SelectWord from APZ");
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
    SELECTIONCARETS_LOG("Set visibility %s, same as the old one",
                        (aVisible ? "shown" : "hidden"));
    return;
  }

  mVisible = aVisible;
  SELECTIONCARETS_LOG("Set visibility %s", (mVisible ? "shown" : "hidden"));

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
  SELECTIONCARETS_LOG("Set start frame visibility %s",
                      (mStartCaretVisible ? "shown" : "hidden"));

  dom::Element* element = mPresShell->GetSelectionCaretsStartElement();
  SetElementVisibility(element, mVisible && mStartCaretVisible);
}

void
SelectionCarets::SetEndFrameVisibility(bool aVisible)
{
  mEndCaretVisible = aVisible;
  SELECTIONCARETS_LOG("Set end frame visibility %s",
                      (mStartCaretVisible ? "shown" : "hidden"));

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

  SELECTIONCARETS_LOG("Set tilted selection carets %s",
                      (aIsTilt ? "enabled" : "disabled"));

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
    SELECTIONCARETS_LOG("Cannot get selection!");
    SetVisibility(false);
    return;
  }

  if (selection->IsCollapsed()) {
    SELECTIONCARETS_LOG("Selection is collapsed!");
    SetVisibility(false);
    return;
  }

  int32_t rangeCount = selection->GetRangeCount();
  nsRefPtr<nsRange> firstRange = selection->GetRangeAt(0);
  nsRefPtr<nsRange> lastRange = selection->GetRangeAt(rangeCount - 1);

  nsIFrame* canvasFrame = mPresShell->GetCanvasFrame();
  nsIFrame* rootFrame = mPresShell->GetRootFrame();

  if (!canvasFrame || !rootFrame) {
    SetVisibility(false);
    return;
  }

  
  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();
  int32_t startOffset;
  nsIFrame* startFrame = FindFirstNodeWithFrame(mPresShell->GetDocument(),
                                                firstRange, fs, false, startOffset);

  int32_t endOffset;
  nsIFrame* endFrame = FindFirstNodeWithFrame(mPresShell->GetDocument(),
                                              lastRange, fs, true, endOffset);

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

  mPresShell->FlushPendingNotifications(Flush_Layout);
  nsRect firstRectInRootFrame =
    nsCaret::GetGeometryForFrame(startFrame, startOffset, nullptr);
  nsRect lastRectInRootFrame =
    nsCaret::GetGeometryForFrame(endFrame, endOffset, nullptr);

  
  
  firstRectInRootFrame = firstRectInRootFrame.ForceInside(startFrame->GetRectRelativeToSelf());
  lastRectInRootFrame = lastRectInRootFrame.ForceInside(endFrame->GetRectRelativeToSelf());
  nsRect firstRectInCanvasFrame = firstRectInRootFrame;
  nsRect lastRectInCanvasFrame =lastRectInRootFrame;
  nsLayoutUtils::TransformRect(startFrame, rootFrame, firstRectInRootFrame);
  nsLayoutUtils::TransformRect(endFrame, rootFrame, lastRectInRootFrame);
  nsLayoutUtils::TransformRect(startFrame, canvasFrame, firstRectInCanvasFrame);
  nsLayoutUtils::TransformRect(endFrame, canvasFrame, lastRectInCanvasFrame);

  firstRectInRootFrame.Inflate(AppUnitsPerCSSPixel(), 0);
  lastRectInRootFrame.Inflate(AppUnitsPerCSSPixel(), 0);

  nsAutoTArray<nsIFrame*, 16> hitFramesInFirstRect;
  nsLayoutUtils::GetFramesForArea(rootFrame,
    firstRectInRootFrame,
    hitFramesInFirstRect,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION |
      nsLayoutUtils::IGNORE_CROSS_DOC |
      nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME);

  nsAutoTArray<nsIFrame*, 16> hitFramesInLastRect;
  nsLayoutUtils::GetFramesForArea(rootFrame,
    lastRectInRootFrame,
    hitFramesInLastRect,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION |
      nsLayoutUtils::IGNORE_CROSS_DOC |
      nsLayoutUtils::IGNORE_ROOT_SCROLL_FRAME);

  SetStartFrameVisibility(hitFramesInFirstRect.Contains(startFrame));
  SetEndFrameVisibility(hitFramesInLastRect.Contains(endFrame));

  SetStartFramePos(firstRectInCanvasFrame.BottomLeft());
  SetEndFramePos(lastRectInCanvasFrame.BottomRight());
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

  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  if (!rootFrame) {
    return NS_OK;
  }

  
  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(rootFrame, mDownPoint,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC);
  if (!ptFrame) {
    return NS_OK;
  }

  
  
  if (!kSupportNonEditableFields && !ptFrame->GetContent()->IsEditable()) {
    return NS_OK;
  }

  nsPoint ptInFrame = mDownPoint;
  nsLayoutUtils::TransformPoint(rootFrame, ptFrame, ptInFrame);

  
  
  
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

#ifdef DEBUG_FRAME_DUMP
  nsCString frameTag;
  frame->ListTag(frameTag);
  SELECTIONCARETS_LOG("Frame=%s, ptInFrame=(%d, %d)", frameTag.get(),
                      ptInFrame.x, ptInFrame.y);
#endif

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
  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  if (!rootFrame) {
    return nsEventStatus_eConsumeNoDefault;
  }

  
  nsIFrame *ptFrame = nsLayoutUtils::GetFrameForPoint(rootFrame, movePoint,
    nsLayoutUtils::IGNORE_PAINT_SUPPRESSION | nsLayoutUtils::IGNORE_CROSS_DOC);
  if (!ptFrame) {
    return nsEventStatus_eConsumeNoDefault;
  }

  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();

  nsresult result;
  nsIFrame *newFrame = nullptr;
  nsPoint newPoint;
  nsPoint ptInFrame = movePoint;
  nsLayoutUtils::TransformPoint(rootFrame, ptFrame, ptInFrame);
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
  int32_t rangeCount = selection->GetRangeCount();
  if (rangeCount <= 0) {
    return nsEventStatus_eConsumeNoDefault;
  }

  nsRefPtr<nsRange> range = mDragMode == START_FRAME ?
    selection->GetRangeAt(0) : selection->GetRangeAt(rangeCount - 1);
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
  nsLayoutUtils::TransformPoint(rootFrame, capturingFrame, ptInScrolled);
  fs->StartAutoScrollTimer(capturingFrame, ptInScrolled, TouchCaret::sAutoScrollTimerDelay);
  UpdateSelectionCarets();
  return nsEventStatus_eConsumeNoDefault;
}

nscoord
SelectionCarets::GetCaretYCenterPosition()
{
  nsIFrame* rootFrame = mPresShell->GetRootFrame();

  if (!rootFrame) {
    return 0;
  }

  nsRefPtr<dom::Selection> selection = GetSelection();
  int32_t rangeCount = selection->GetRangeCount();
  if (rangeCount <= 0) {
    return 0;
  }

  nsRefPtr<nsFrameSelection> fs = GetFrameSelection();

  MOZ_ASSERT(mDragMode != NONE);
  nsCOMPtr<nsIContent> node;
  uint32_t nodeOffset;
  if (mDragMode == START_FRAME) {
    nsRefPtr<nsRange> range = selection->GetRangeAt(0);
    node = do_QueryInterface(range->GetStartParent());
    nodeOffset = range->StartOffset();
  } else {
    nsRefPtr<nsRange> range = selection->GetRangeAt(rangeCount - 1);
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
  nsLayoutUtils::TransformRect(theFrame, rootFrame, frameRect);
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
  styleStr.AppendLiteral("left: ");
  styleStr.AppendFloat(nsPresContext::AppUnitsToFloatCSSPixels(aPosition.x));
  styleStr.AppendLiteral("px; top: ");
  styleStr.AppendFloat(nsPresContext::AppUnitsToFloatCSSPixels(aPosition.y));
  styleStr.AppendLiteral("px;");

  SELECTIONCARETS_LOG_STATIC("Set style: %s",
                             NS_ConvertUTF16toUTF8(styleStr).get());

  aElement->SetAttr(kNameSpaceID_None, nsGkAtoms::style, styleStr, true);
}

void
SelectionCarets::SetStartFramePos(const nsPoint& aPosition)
{
  SELECTIONCARETS_LOG("x=%d, y=%d", aPosition.x, aPosition.y);
  SetFramePos(mPresShell->GetSelectionCaretsStartElement(), aPosition);
}

void
SelectionCarets::SetEndFramePos(const nsPoint& aPosition)
{
  SELECTIONCARETS_LOG("x=%d, y=%d", aPosition.y, aPosition.y);
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
  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  return nsLayoutUtils::GetRectRelativeToFrame(element, rootFrame);
}

nsRect
SelectionCarets::GetEndFrameRect()
{
  dom::Element* element = mPresShell->GetSelectionCaretsEndElement();
  nsIFrame* rootFrame = mPresShell->GetRootFrame();
  return nsLayoutUtils::GetRectRelativeToFrame(element, rootFrame);
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
  SELECTIONCARETS_LOG("aSel (%p), Reason=%d", aSel, aReason);
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
  SetVisibility(false);

  SELECTIONCARETS_LOG("Dispatch scroll started with position x=%d, y=%d",
                      aScrollPos.x, aScrollPos.y);
  DispatchScrollViewChangeEvent(mPresShell, dom::ScrollState::Started, aScrollPos);
}

void
SelectionCarets::AsyncPanZoomStopped(const mozilla::CSSIntPoint aScrollPos)
{
  UpdateSelectionCarets();

  SELECTIONCARETS_LOG("Dispatch scroll stopped with position x=%d, y=%d",
                      aScrollPos.x, aScrollPos.y);
  DispatchScrollViewChangeEvent(mPresShell, dom::ScrollState::Stopped, aScrollPos);
}

void
SelectionCarets::ScrollPositionChanged()
{
  if (!mAsyncPanZoomEnabled && mVisible) {
    SetVisibility(false);
    

    SELECTIONCARETS_LOG("Launch scroll end detector");
    LaunchScrollEndDetector();
  }
}

void
SelectionCarets::LaunchLongTapDetector()
{
  if (mAsyncPanZoomEnabled) {
    return;
  }

  if (!mLongTapDetectorTimer) {
    mLongTapDetectorTimer = do_CreateInstance("@mozilla.org/timer;1");
  }

  MOZ_ASSERT(mLongTapDetectorTimer);
  CancelLongTapDetector();
  int32_t longTapDelay = gfxPrefs::UiClickHoldContextMenusDelay();

  SELECTIONCARETS_LOG("Will fire long tap after %d ms", longTapDelay);
  mLongTapDetectorTimer->InitWithFuncCallback(FireLongTap,
                                              this,
                                              longTapDelay,
                                              nsITimer::TYPE_ONE_SHOT);
}

void
SelectionCarets::CancelLongTapDetector()
{
  if (mAsyncPanZoomEnabled) {
    return;
  }

  if (!mLongTapDetectorTimer) {
    return;
  }

  SELECTIONCARETS_LOG("Cancel long tap detector!");
  mLongTapDetectorTimer->Cancel();
}

void
SelectionCarets::FireLongTap(nsITimer* aTimer, void* aSelectionCarets)
{
  nsRefPtr<SelectionCarets> self = static_cast<SelectionCarets*>(aSelectionCarets);
  NS_PRECONDITION(aTimer == self->mLongTapDetectorTimer,
                  "Unexpected timer");

  SELECTIONCARETS_LOG_STATIC("SelectWord from non-APZ");
  self->SelectWord();
}

void
SelectionCarets::LaunchScrollEndDetector()
{
  if (!mScrollEndDetectorTimer) {
    mScrollEndDetectorTimer = do_CreateInstance("@mozilla.org/timer;1");
  }

  MOZ_ASSERT(mScrollEndDetectorTimer);

  SELECTIONCARETS_LOG("Will fire scroll end after %d ms", kScrollEndTimerDelay);
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

  SELECTIONCARETS_LOG_STATIC("Update selection carets!");
  self->SetVisibility(true);
  self->UpdateSelectionCarets();
}

NS_IMETHODIMP
SelectionCarets::Reflow(DOMHighResTimeStamp aStart, DOMHighResTimeStamp aEnd)
{
  if (mVisible) {
    SELECTIONCARETS_LOG("Update selection carets after reflow!");
    UpdateSelectionCarets();
  }
  return NS_OK;
}

NS_IMETHODIMP
SelectionCarets::ReflowInterruptible(DOMHighResTimeStamp aStart,
                                     DOMHighResTimeStamp aEnd)
{
  return Reflow(aStart, aEnd);
}
