







































#include "nsLayoutUtils.h"
#include "nsIFontMetrics.h"
#include "nsIFormControlFrame.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsFrameList.h"
#include "nsGkAtoms.h"
#include "nsIAtom.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSAnonBoxes.h"
#include "nsIView.h"
#include "nsIScrollableView.h"
#include "nsPlaceholderFrame.h"
#include "nsIScrollableFrame.h"
#include "nsCSSFrameConstructor.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEvent.h"
#include "nsGUIEvent.h"
#include "nsDisplayList.h"
#include "nsRegion.h"
#include "nsFrameManager.h"
#include "nsBlockFrame.h"
#include "nsBidiPresUtils.h"
#include "imgIContainer.h"
#include "gfxRect.h"
#include "gfxContext.h"
#include "gfxFont.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCSSRendering.h"
#include "nsContentUtils.h"
#include "nsThemeConstants.h"
#include "nsPIDOMWindow.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWidget.h"
#include "gfxMatrix.h"
#include "gfxTypes.h"
#include "gfxUserFontSet.h"
#include "nsTArray.h"
#include "nsTextFragment.h"
#include "nsICanvasElement.h"
#include "nsICanvasRenderingContextInternal.h"
#include "gfxPlatform.h"
#include "nsClientRect.h"
#ifdef MOZ_MEDIA
#include "nsHTMLVideoElement.h"
#endif
#include "imgIRequest.h"
#include "imgIContainer.h"
#include "nsIImageLoadingContent.h"
#include "nsCOMPtr.h"

#ifdef MOZ_SVG
#include "nsSVGUtils.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGForeignObjectFrame.h"
#include "nsSVGOuterSVGFrame.h"
#endif





PRBool nsLayoutUtils::sDisableGetUsedXAssertions = PR_FALSE;

nsIFrame*
nsLayoutUtils::GetLastContinuationWithChild(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");
  aFrame = aFrame->GetLastContinuation();
  while (!aFrame->GetFirstChild(nsnull) &&
         aFrame->GetPrevContinuation()) {
    aFrame = aFrame->GetPrevContinuation();
  }
  return aFrame;
}








static nsIFrame*
GetFirstChildFrame(nsIFrame*       aFrame,
                   nsIContent*     aContent)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  
  nsIFrame* childFrame = aFrame->GetFirstChild(nsnull);

  
  
  
  if (childFrame &&
      childFrame->IsPseudoFrame(aContent) &&
      !childFrame->IsGeneratedContentFrame()) {
    return GetFirstChildFrame(childFrame, aContent);
  }

  return childFrame;
}








static nsIFrame*
GetLastChildFrame(nsIFrame*       aFrame,
                  nsIContent*     aContent)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  
  nsIFrame* lastParentContinuation = nsLayoutUtils::GetLastContinuationWithChild(aFrame);

  nsIFrame* lastChildFrame = lastParentContinuation->GetLastChild(nsnull);
  if (lastChildFrame) {
    
    
    lastChildFrame = lastChildFrame->GetFirstContinuation();
    
    
    
    
    if (lastChildFrame &&
        lastChildFrame->IsPseudoFrame(aContent) &&
        !lastChildFrame->IsGeneratedContentFrame()) {
      return GetLastChildFrame(lastChildFrame, aContent);
    }

    return lastChildFrame;
  }

  return nsnull;
}


nsIFrame*
nsLayoutUtils::GetBeforeFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");
  NS_ASSERTION(!aFrame->GetPrevContinuation(),
               "aFrame must be first continuation");
  
  nsIFrame* firstFrame = GetFirstChildFrame(aFrame, aFrame->GetContent());

  if (firstFrame && IsGeneratedContentFor(nsnull, firstFrame,
                                          nsCSSPseudoElements::before)) {
    return firstFrame;
  }

  return nsnull;
}


nsIFrame*
nsLayoutUtils::GetAfterFrame(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  nsIFrame* lastFrame = GetLastChildFrame(aFrame, aFrame->GetContent());

  if (lastFrame && IsGeneratedContentFor(nsnull, lastFrame,
                                         nsCSSPseudoElements::after)) {
    return lastFrame;
  }

  return nsnull;
}


nsIFrame*
nsLayoutUtils::GetClosestFrameOfType(nsIFrame* aFrame, nsIAtom* aFrameType)
{
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent()) {
    if (frame->GetType() == aFrameType) {
      return frame;
    }
  }
  return nsnull;
}

nsIFrame*
nsLayoutUtils::GetFloatFromPlaceholder(nsIFrame* aFrame) {
  NS_ASSERTION(nsGkAtoms::placeholderFrame == aFrame->GetType(),
               "Must have a placeholder here");
  if (aFrame->GetStateBits() & PLACEHOLDER_FOR_FLOAT) {
    nsIFrame *outOfFlowFrame =
      nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    NS_ASSERTION(outOfFlowFrame->GetStyleDisplay()->IsFloating(),
                 "How did that happen?");
    return outOfFlowFrame;
  }

  return nsnull;
}


PRBool
nsLayoutUtils::IsGeneratedContentFor(nsIContent* aContent,
                                     nsIFrame* aFrame,
                                     nsIAtom* aPseudoElement)
{
  NS_PRECONDITION(aFrame, "Must have a frame");
  NS_PRECONDITION(aPseudoElement, "Must have a pseudo name");

  if (!aFrame->IsGeneratedContentFrame()) {
    return PR_FALSE;
  }
  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "Generated content can't be root frame");
  if (parent->IsGeneratedContentFrame()) {
    
    return PR_FALSE;
  }
  
  if (aContent && parent->GetContent() != aContent) {
    return PR_FALSE;
  }

  return (aFrame->GetContent()->Tag() == nsGkAtoms::mozgeneratedcontentbefore) ==
    (aPseudoElement == nsCSSPseudoElements::before);
}


nsIFrame*
nsLayoutUtils::GetCrossDocParentFrame(const nsIFrame* aFrame,
                                      nsPoint* aExtraOffset)
{
  nsIFrame* p = aFrame->GetParent();
  if (p)
    return p;

  nsIView* v = aFrame->GetView();
  if (!v)
    return nsnull;
  v = v->GetParent(); 
  if (!v)
    return nsnull;
  if (aExtraOffset) {
    *aExtraOffset += v->GetPosition();
  }
  v = v->GetParent(); 
  if (!v)
    return nsnull;
  return static_cast<nsIFrame*>(v->GetClientData());
}


PRBool
nsLayoutUtils::IsProperAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                             nsIFrame* aCommonAncestor)
{
  if (aFrame == aCommonAncestor)
    return PR_FALSE;
  
  nsIFrame* parentFrame = GetCrossDocParentFrame(aFrame);

  while (parentFrame != aCommonAncestor) {
    if (parentFrame == aAncestorFrame)
      return PR_TRUE;

    parentFrame = GetCrossDocParentFrame(parentFrame);
  }

  return PR_FALSE;
}


PRBool
nsLayoutUtils::IsAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                       nsIFrame* aCommonAncestor)
{
  if (aFrame == aAncestorFrame)
    return PR_TRUE;
  return IsProperAncestorFrameCrossDoc(aAncestorFrame, aFrame, aCommonAncestor);
}


PRBool
nsLayoutUtils::IsProperAncestorFrame(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                     nsIFrame* aCommonAncestor)
{
  if (aFrame == aCommonAncestor) {
    return PR_FALSE;
  }
  
  nsIFrame* parentFrame = aFrame->GetParent();

  while (parentFrame != aCommonAncestor) {
    if (parentFrame == aAncestorFrame) {
      return PR_TRUE;
    }

    parentFrame = parentFrame->GetParent();
  }

  return PR_FALSE;
}


PRInt32
nsLayoutUtils::DoCompareTreePosition(nsIContent* aContent1,
                                     nsIContent* aContent2,
                                     PRInt32 aIf1Ancestor,
                                     PRInt32 aIf2Ancestor,
                                     const nsIContent* aCommonAncestor)
{
  NS_PRECONDITION(aContent1, "aContent1 must not be null");
  NS_PRECONDITION(aContent2, "aContent2 must not be null");

  nsAutoTArray<nsINode*, 32> content1Ancestors;
  nsINode* c1;
  for (c1 = aContent1; c1 && c1 != aCommonAncestor; c1 = c1->GetNodeParent()) {
    content1Ancestors.AppendElement(c1);
  }
  if (!c1 && aCommonAncestor) {
    
    
    aCommonAncestor = nsnull;
  }

  nsAutoTArray<nsINode*, 32> content2Ancestors;
  nsINode* c2;
  for (c2 = aContent2; c2 && c2 != aCommonAncestor; c2 = c2->GetNodeParent()) {
    content2Ancestors.AppendElement(c2);
  }
  if (!c2 && aCommonAncestor) {
    
    
    return DoCompareTreePosition(aContent1, aContent2,
                                 aIf1Ancestor, aIf2Ancestor, nsnull);
  }
  
  int last1 = content1Ancestors.Length() - 1;
  int last2 = content2Ancestors.Length() - 1;
  nsINode* content1Ancestor = nsnull;
  nsINode* content2Ancestor = nsnull;
  while (last1 >= 0 && last2 >= 0
         && ((content1Ancestor = content1Ancestors.ElementAt(last1)) ==
             (content2Ancestor = content2Ancestors.ElementAt(last2)))) {
    last1--;
    last2--;
  }

  if (last1 < 0) {
    if (last2 < 0) {
      NS_ASSERTION(aContent1 == aContent2, "internal error?");
      return 0;
    }
    
    return aIf1Ancestor;
  }

  if (last2 < 0) {
    
    return aIf2Ancestor;
  }

  
  nsINode* parent = content1Ancestor->GetNodeParent();
  NS_ASSERTION(parent, "no common ancestor at all???");
  if (!parent) { 
    return 0;
  }

  PRInt32 index1 = parent->IndexOf(content1Ancestor);
  PRInt32 index2 = parent->IndexOf(content2Ancestor);
  if (index1 < 0 || index2 < 0) {
    
    return 0;
  }

  return index1 - index2;
}

static nsIFrame* FillAncestors(nsIFrame* aFrame,
                               nsIFrame* aStopAtAncestor, nsFrameManager* aFrameManager,
                               nsTArray<nsIFrame*>* aAncestors)
{
  while (aFrame && aFrame != aStopAtAncestor) {
    aAncestors->AppendElement(aFrame);
    aFrame = nsLayoutUtils::GetParentOrPlaceholderFor(aFrameManager, aFrame);
  }
  return aFrame;
}


static PRBool IsFrameAfter(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  nsIFrame* f = aFrame2;
  do {
    f = f->GetNextSibling();
    if (f == aFrame1)
      return PR_TRUE;
  } while (f);
  return PR_FALSE;
}


PRInt32
nsLayoutUtils::DoCompareTreePosition(nsIFrame* aFrame1,
                                     nsIFrame* aFrame2,
                                     PRInt32 aIf1Ancestor,
                                     PRInt32 aIf2Ancestor,
                                     nsIFrame* aCommonAncestor)
{
  NS_PRECONDITION(aFrame1, "aFrame1 must not be null");
  NS_PRECONDITION(aFrame2, "aFrame2 must not be null");

  nsPresContext* presContext = aFrame1->PresContext();
  if (presContext != aFrame2->PresContext()) {
    NS_ERROR("no common ancestor at all, different documents");
    return 0;
  }
  nsFrameManager* frameManager = presContext->PresShell()->FrameManager();

  nsAutoTArray<nsIFrame*,20> frame1Ancestors;
  if (!FillAncestors(aFrame1, aCommonAncestor, frameManager, &frame1Ancestors)) {
    
    
    aCommonAncestor = nsnull;
  }

  nsAutoTArray<nsIFrame*,20> frame2Ancestors;
  if (!FillAncestors(aFrame2, aCommonAncestor, frameManager, &frame2Ancestors) &&
      aCommonAncestor) {
    
    
    return DoCompareTreePosition(aFrame1, aFrame2,
                                 aIf1Ancestor, aIf2Ancestor, nsnull);
  }

  PRInt32 last1 = PRInt32(frame1Ancestors.Length()) - 1;
  PRInt32 last2 = PRInt32(frame2Ancestors.Length()) - 1;
  while (last1 >= 0 && last2 >= 0 &&
         frame1Ancestors[last1] == frame2Ancestors[last2]) {
    last1--;
    last2--;
  }

  if (last1 < 0) {
    if (last2 < 0) {
      NS_ASSERTION(aFrame1 == aFrame2, "internal error?");
      return 0;
    }
    
    return aIf1Ancestor;
  }

  if (last2 < 0) {
    
    return aIf2Ancestor;
  }

  nsIFrame* ancestor1 = frame1Ancestors[last1];
  nsIFrame* ancestor2 = frame2Ancestors[last2];
  
  if (IsFrameAfter(ancestor2, ancestor1))
    return -1;
  if (IsFrameAfter(ancestor1, ancestor2))
    return 1;
  NS_WARNING("Frames were in different child lists???");
  return 0;
}


nsIFrame* nsLayoutUtils::GetLastSibling(nsIFrame* aFrame) {
  if (!aFrame) {
    return nsnull;
  }

  nsIFrame* next;
  while ((next = aFrame->GetNextSibling()) != nsnull) {
    aFrame = next;
  }
  return aFrame;
}


nsIView*
nsLayoutUtils::FindSiblingViewFor(nsIView* aParentView, nsIFrame* aFrame) {
  nsIFrame* parentViewFrame = static_cast<nsIFrame*>(aParentView->GetClientData());
  nsIContent* parentViewContent = parentViewFrame ? parentViewFrame->GetContent() : nsnull;
  for (nsIView* insertBefore = aParentView->GetFirstChild(); insertBefore;
       insertBefore = insertBefore->GetNextSibling()) {
    nsIFrame* f = static_cast<nsIFrame*>(insertBefore->GetClientData());
    if (!f) {
      
      for (nsIView* searchView = insertBefore->GetParent(); searchView;
           searchView = searchView->GetParent()) {
        f = static_cast<nsIFrame*>(searchView->GetClientData());
        if (f) {
          break;
        }
      }
      NS_ASSERTION(f, "Can't find a frame anywhere!");
    }
    if (!f || !aFrame->GetContent() || !f->GetContent() ||
        CompareTreePosition(aFrame->GetContent(), f->GetContent(), parentViewContent) > 0) {
      
      
      return insertBefore;
    }
  }
  return nsnull;
}


nsIScrollableFrame*
nsLayoutUtils::GetScrollableFrameFor(nsIFrame *aScrolledFrame)
{
  nsIFrame *frame = aScrolledFrame->GetParent();
  if (!frame) {
    return nsnull;
  }
  nsIScrollableFrame *sf = do_QueryFrame(frame);
  return sf;
}


nsIScrollableFrame*
nsLayoutUtils::GetScrollableFrameFor(nsIScrollableView *aScrollableView)
{
  nsIFrame *frame = GetFrameFor(aScrollableView->View()->GetParent());
  nsIScrollableFrame *sf = do_QueryFrame(frame);
  return sf;
}


nsPresContext::ScrollbarStyles
nsLayoutUtils::ScrollbarStylesOfView(nsIScrollableView *aScrollableView)
{
  nsIScrollableFrame *sf = GetScrollableFrameFor(aScrollableView);
  return sf ? sf->GetScrollbarStyles() :
              nsPresContext::ScrollbarStyles(NS_STYLE_OVERFLOW_HIDDEN,
                                             NS_STYLE_OVERFLOW_HIDDEN);
}


nsIScrollableView*
nsLayoutUtils::GetNearestScrollingView(nsIView* aView, Direction aDirection)
{
  
  
  
  
  
  NS_ASSERTION(aView, "GetNearestScrollingView expects a non-null view");
  nsIScrollableView* scrollableView = nsnull;
  for (; aView; aView = aView->GetParent()) {
    scrollableView = aView->ToScrollableView();
    if (scrollableView) {
      nsPresContext::ScrollbarStyles ss =
        nsLayoutUtils::ScrollbarStylesOfView(scrollableView);
      nsIScrollableFrame *scrollableFrame = GetScrollableFrameFor(scrollableView);
      NS_ASSERTION(scrollableFrame, "Must have scrollable frame for view!");
      nsMargin margin = scrollableFrame->GetActualScrollbarSizes();
      
      nscoord totalWidth, totalHeight;
      scrollableView->GetContainerSize(&totalWidth, &totalHeight);
      
      nsSize visibleSize = aView->GetBounds().Size();
      
      
      
      if (aDirection != eHorizontal &&
          ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN &&
          (aDirection == eEither || totalHeight > visibleSize.height || margin.LeftRight()))
        break;
      if (aDirection != eVertical &&
          ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN &&
          (aDirection == eEither || totalWidth > visibleSize.width || margin.TopBottom()))
        break;
    }
  }
  return scrollableView;
}

nsPoint
nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(nsIDOMEvent* aDOMEvent, nsIFrame* aFrame)
{
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aDOMEvent));
  NS_ASSERTION(privateEvent, "bad implementation");
  if (!privateEvent)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  nsEvent *event = privateEvent->GetInternalNSEvent();
  if (!event)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  return GetEventCoordinatesRelativeTo(event, aFrame);
}

nsPoint
nsLayoutUtils::GetEventCoordinatesRelativeTo(const nsEvent* aEvent, nsIFrame* aFrame)
{
  if (!aEvent || (aEvent->eventStructType != NS_MOUSE_EVENT && 
                  aEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
                  aEvent->eventStructType != NS_DRAG_EVENT &&
                  aEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT &&
                  aEvent->eventStructType != NS_GESTURENOTIFY_EVENT &&
                  aEvent->eventStructType != NS_QUERY_CONTENT_EVENT))
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  const nsGUIEvent* GUIEvent = static_cast<const nsGUIEvent*>(aEvent);
  if (!GUIEvent->widget)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  



  nsIFrame* rootFrame = aFrame;
  PRBool transformFound = PR_FALSE;

  for (nsIFrame* f = aFrame; f; f = GetCrossDocParentFrame(f)) {
    if (f->IsTransformed())
      transformFound = PR_TRUE;

    rootFrame = f;
  }

  nsIView* rootView = rootFrame->GetView();
  if (!rootView)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  nsPoint widgetToView = TranslateWidgetToView(rootFrame->PresContext(),
                               GUIEvent->widget, GUIEvent->refPoint,
                               rootView);

  if (widgetToView == nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE))
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  


  if (transformFound)
    return InvertTransformsToRoot(aFrame, widgetToView);
  
  


  return widgetToView - aFrame->GetOffsetTo(rootFrame);
}

gfxMatrix
nsLayoutUtils::ChangeMatrixBasis(const gfxPoint &aOrigin,
                                 const gfxMatrix &aMatrix)
{
  




  gfxMatrix worldToOrigin(1.0, 0.0, 0.0, 1.0, -aOrigin.x, -aOrigin.y);
  gfxMatrix originToWorld(1.0, 0.0, 0.0, 1.0,  aOrigin.x,  aOrigin.y);

  
  gfxMatrix result(worldToOrigin);
  result.Multiply(aMatrix);
  result.Multiply(originToWorld);
  return result;
}






static void ConstrainToCoordValues(gfxFloat &aVal)
{
  if (aVal <= nscoord_MIN)
    aVal = nscoord_MIN;
  else if (aVal >= nscoord_MAX)
    aVal = nscoord_MAX;
}

nsRect
nsLayoutUtils::RoundGfxRectToAppRect(const gfxRect &aRect, float aFactor)
{ 
   
  gfxRect scaledRect(aRect.pos.x * aFactor, aRect.pos.y * aFactor,
                     aRect.size.width * aFactor,
                     aRect.size.height * aFactor);
  
  
  scaledRect.RoundOut();

  
  ConstrainToCoordValues(scaledRect.pos.x);
  ConstrainToCoordValues(scaledRect.pos.y);
  ConstrainToCoordValues(scaledRect.size.width);
  ConstrainToCoordValues(scaledRect.size.height);
  
  
  return nsRect(nscoord(scaledRect.pos.x), nscoord(scaledRect.pos.y),
                nscoord(scaledRect.size.width), nscoord(scaledRect.size.height));
}

nsRect
nsLayoutUtils::MatrixTransformRect(const nsRect &aBounds,
                                   const gfxMatrix &aMatrix, float aFactor)
{
  gfxRect image = aMatrix.TransformBounds(gfxRect(NSAppUnitsToFloatPixels(aBounds.x, aFactor),
                                                  NSAppUnitsToFloatPixels(aBounds.y, aFactor),
                                                  NSAppUnitsToFloatPixels(aBounds.width, aFactor),
                                                  NSAppUnitsToFloatPixels(aBounds.height, aFactor)));
  
  return RoundGfxRectToAppRect(image, aFactor);
}

nsPoint
nsLayoutUtils::MatrixTransformPoint(const nsPoint &aPoint,
                                    const gfxMatrix &aMatrix, float aFactor)
{
  gfxPoint image = aMatrix.Transform(gfxPoint(NSAppUnitsToFloatPixels(aPoint.x, aFactor),
                                              NSAppUnitsToFloatPixels(aPoint.y, aFactor)));
  return nsPoint(NSFloatPixelsToAppUnits(float(image.x), aFactor),
                 NSFloatPixelsToAppUnits(float(image.y), aFactor));
}







static gfxMatrix GetCTMAt(nsIFrame *aFrame)
{
  gfxMatrix ctm;

  





  while (aFrame)
    ctm *= aFrame->GetTransformMatrix(&aFrame);
  return ctm;
}

nsPoint
nsLayoutUtils::InvertTransformsToRoot(nsIFrame *aFrame,
                                      const nsPoint &aPoint)
{
  NS_PRECONDITION(aFrame, "Why are you inverting transforms when there is no frame?");

  


  gfxMatrix ctm = GetCTMAt(aFrame);

  
  if (ctm.IsSingular())
    return nsPoint(0, 0);

  
  return MatrixTransformPoint(aPoint, ctm.Invert(), aFrame->PresContext()->AppUnitsPerDevPixel());
}

nsPoint
nsLayoutUtils::GetEventCoordinatesForNearestView(nsEvent* aEvent,
                                                 nsIFrame* aFrame,
                                                 nsIView** aView)
{
  if (!aEvent || (aEvent->eventStructType != NS_MOUSE_EVENT && 
                  aEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
                  aEvent->eventStructType != NS_DRAG_EVENT))
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  nsGUIEvent* GUIEvent = static_cast<nsGUIEvent*>(aEvent);
  if (!GUIEvent->widget)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  nsPoint viewToFrame;
  nsIView* frameView;
  aFrame->GetOffsetFromView(viewToFrame, &frameView);
  if (aView)
    *aView = frameView;

  return TranslateWidgetToView(aFrame->PresContext(), GUIEvent->widget,
                               GUIEvent->refPoint, frameView);
}

static nsIntPoint GetWidgetOffset(nsIWidget* aWidget, nsIWidget*& aRootWidget) {
  nsIntPoint offset(0, 0);
  nsIWidget* parent = aWidget->GetParent();
  while (parent) {
    nsIntRect bounds;
    aWidget->GetBounds(bounds);
    offset += bounds.TopLeft();
    aWidget = parent;
    parent = aWidget->GetParent();
  }
  aRootWidget = aWidget;
  return offset;
}

nsPoint
nsLayoutUtils::TranslateWidgetToView(nsPresContext* aPresContext, 
                                     nsIWidget* aWidget, nsIntPoint aPt,
                                     nsIView* aView)
{
  nsPoint viewOffset;
  nsIWidget* viewWidget = aView->GetNearestWidget(&viewOffset);

  nsIWidget* fromRoot;
  nsIntPoint fromOffset = GetWidgetOffset(aWidget, fromRoot);
  nsIWidget* toRoot;
  nsIntPoint toOffset = GetWidgetOffset(viewWidget, toRoot);

  nsIntPoint widgetPoint;
  if (fromRoot == toRoot) {
    widgetPoint = aPt + fromOffset - toOffset;
  } else {
    nsIntPoint screenPoint = aWidget->WidgetToScreenOffset();
    widgetPoint = aPt + screenPoint - viewWidget->WidgetToScreenOffset();
  }

  nsPoint widgetAppUnits(aPresContext->DevPixelsToAppUnits(widgetPoint.x),
                         aPresContext->DevPixelsToAppUnits(widgetPoint.y));
  return widgetAppUnits - viewOffset;
}



PRUint8
nsLayoutUtils::CombineBreakType(PRUint8 aOrigBreakType,
                                PRUint8 aNewBreakType)
{
  PRUint8 breakType = aOrigBreakType;
  switch(breakType) {
  case NS_STYLE_CLEAR_LEFT:
    if ((NS_STYLE_CLEAR_RIGHT          == aNewBreakType) ||
        (NS_STYLE_CLEAR_LEFT_AND_RIGHT == aNewBreakType)) {
      breakType = NS_STYLE_CLEAR_LEFT_AND_RIGHT;
    }
    break;
  case NS_STYLE_CLEAR_RIGHT:
    if ((NS_STYLE_CLEAR_LEFT           == aNewBreakType) ||
        (NS_STYLE_CLEAR_LEFT_AND_RIGHT == aNewBreakType)) {
      breakType = NS_STYLE_CLEAR_LEFT_AND_RIGHT;
    }
    break;
  case NS_STYLE_CLEAR_NONE:
    if ((NS_STYLE_CLEAR_LEFT           == aNewBreakType) ||
        (NS_STYLE_CLEAR_RIGHT          == aNewBreakType) ||
        (NS_STYLE_CLEAR_LEFT_AND_RIGHT == aNewBreakType)) {
      breakType = aNewBreakType;
    }
  }
  return breakType;
}

#ifdef DEBUG
#include <stdio.h>

static PRBool gDumpPaintList = PR_FALSE;
static PRBool gDumpEventList = PR_FALSE;
static PRBool gDumpRepaintRegionForCopy = PR_FALSE;
#endif

nsIFrame*
nsLayoutUtils::GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt,
                                PRBool aShouldIgnoreSuppression,
                                PRBool aIgnoreRootScrollFrame)
{
  nsDisplayListBuilder builder(aFrame, PR_TRUE, PR_FALSE);
  nsDisplayList list;
  nsRect target(aPt, nsSize(1, 1));

  if (aShouldIgnoreSuppression)
    builder.IgnorePaintSuppression();

  if (aIgnoreRootScrollFrame) {
    nsIFrame* rootScrollFrame =
      aFrame->PresContext()->PresShell()->GetRootScrollFrame();
    if (rootScrollFrame) {
      builder.SetIgnoreScrollFrame(rootScrollFrame);
    }
  }

  builder.EnterPresShell(aFrame, target);

  nsresult rv =
    aFrame->BuildDisplayListForStackingContext(&builder, target, &list);

  builder.LeavePresShell(aFrame, target);
  NS_ENSURE_SUCCESS(rv, nsnull);

#ifdef DEBUG
  if (gDumpEventList) {
    fprintf(stderr, "Event handling --- (%d,%d):\n", aPt.x, aPt.y);
    nsFrame::PrintDisplayList(&builder, list);
  }
#endif

  nsDisplayItem::HitTestState hitTestState;
  nsIFrame* result = list.HitTest(&builder, aPt, &hitTestState);
  list.DeleteAll();
  return result;
}











static void
PruneDisplayListForExtraPage(nsDisplayListBuilder* aBuilder,
        nsIFrame* aExtraPage, nscoord aY, nsDisplayList* aList)
{
  nsDisplayList newList;
  
  nsIFrame* mainPage = aBuilder->ReferenceFrame();

  while (PR_TRUE) {
    nsDisplayItem* i = aList->RemoveBottom();
    if (!i)
      break;
    nsDisplayList* subList = i->GetList();
    if (subList) {
      PruneDisplayListForExtraPage(aBuilder, aExtraPage, aY, subList);
      if (i->GetType() == nsDisplayItem::TYPE_CLIP) {
        
        
        
        
        
        
        
        
        
        
        
        nsDisplayClip* clip = static_cast<nsDisplayClip*>(i);
        clip->SetClipRect(clip->GetClipRect() + nsPoint(0, aY) -
                aExtraPage->GetOffsetTo(mainPage));
      }
      newList.AppendToTop(i);
    } else {
      nsIFrame* f = i->GetUnderlyingFrame();
      if (f && nsLayoutUtils::IsProperAncestorFrameCrossDoc(mainPage, f)) {
        
        newList.AppendToTop(i);
      } else {
        
        
        i->~nsDisplayItem();
      }
    }
  }
  aList->AppendToTop(&newList);
}

static nsresult
BuildDisplayListForExtraPage(nsDisplayListBuilder* aBuilder,
        nsIFrame* aPage, nscoord aY, nsDisplayList* aList)
{
  nsDisplayList list;
  
  
  
  
  
  
  nsresult rv = aPage->BuildDisplayListForStackingContext(aBuilder, nsRect(), &list);
  if (NS_FAILED(rv))
    return rv;
  PruneDisplayListForExtraPage(aBuilder, aPage, aY, &list);
  aList->AppendToTop(&list);
  return NS_OK;
}

static nsIFrame*
GetNextPage(nsIFrame* aPageContentFrame)
{
  
  nsIFrame* pageFrame = aPageContentFrame->GetParent();
  NS_ASSERTION(pageFrame->GetType() == nsGkAtoms::pageFrame,
               "pageContentFrame has unexpected parent");
  nsIFrame* nextPageFrame = pageFrame->GetNextSibling();
  if (!nextPageFrame)
    return nsnull;
  NS_ASSERTION(nextPageFrame->GetType() == nsGkAtoms::pageFrame,
               "pageFrame's sibling is not a page frame...");
  nsIFrame* f = nextPageFrame->GetFirstChild(nsnull);
  NS_ASSERTION(f, "pageFrame has no page content frame!");
  NS_ASSERTION(f->GetType() == nsGkAtoms::pageContentFrame,
               "pageFrame's child is not page content!");
  return f;
}

nsresult
nsLayoutUtils::PaintFrame(nsIRenderingContext* aRenderingContext, nsIFrame* aFrame,
                          const nsRegion& aDirtyRegion, nscolor aBackstop,
                          PRUint32 aFlags)
{
  nsAutoDisableGetUsedXAssertions disableAssert;

  nsDisplayListBuilder builder(aFrame, PR_FALSE, PR_TRUE);
  nsDisplayList list;
  nsRect dirtyRect = aDirtyRegion.GetBounds();
  if (aFlags & PAINT_IN_TRANSFORM) {
    builder.SetInTransform(PR_TRUE);
  }
  if (aFlags & PAINT_SYNC_DECODE_IMAGES) {
    builder.SetSyncDecodeImages(PR_TRUE);
  }
  nsresult rv;

  builder.EnterPresShell(aFrame, dirtyRect);

  rv = aFrame->BuildDisplayListForStackingContext(&builder, dirtyRect, &list);

  if (NS_SUCCEEDED(rv) && aFrame->GetType() == nsGkAtoms::pageContentFrame) {
    
    
    
    
    
    
    
    nsIFrame* page = aFrame;
    nscoord y = aFrame->GetSize().height;
    while ((page = GetNextPage(page)) != nsnull) {
      rv = BuildDisplayListForExtraPage(&builder, page, y, &list);
      if (NS_FAILED(rv))
        break;
      y += page->GetSize().height;
    }
  }

  nsIAtom* frameType = aFrame->GetType();
  
  
  if (frameType == nsGkAtoms::viewportFrame &&
      aFrame->PresContext()->IsRootPaginatedDocument() &&
      (aFrame->PresContext()->Type() == nsPresContext::eContext_PrintPreview ||
       aFrame->PresContext()->Type() == nsPresContext::eContext_PageLayout)) {
    nsRect bounds = nsRect(builder.ToReferenceFrame(aFrame),
                           aFrame->GetSize());
    rv = list.AppendNewToBottom(new (&builder) nsDisplaySolidColor(
           aFrame, bounds, NS_RGB(115, 115, 115)));
  } else if (frameType != nsGkAtoms::pageFrame) {
    
    
    
    
    

    
    rv = aFrame->PresContext()->PresShell()->AddCanvasBackgroundColorItem(
           builder, list, aFrame, nsnull, aBackstop);
  }

  builder.LeavePresShell(aFrame, dirtyRect);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  if (gDumpPaintList) {
    fprintf(stderr, "Painting --- before optimization (dirty %d,%d,%d,%d):\n",
            dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height);
    nsFrame::PrintDisplayList(&builder, list);
  }
#endif

  nsRegion visibleRegion = aDirtyRegion;
  list.ComputeVisibility(&builder, &visibleRegion, nsnull);

#ifdef DEBUG
  if (gDumpPaintList) {
    fprintf(stderr, "Painting --- after optimization:\n");
    nsFrame::PrintDisplayList(&builder, list);
  }
#endif

  list.Paint(&builder, aRenderingContext);
  
  list.DeleteAll();
  return NS_OK;
}

static void
AccumulateItemInRegion(nsRegion* aRegion, const nsRect& aUpdateRect,
                       const nsRect& aItemRect, const nsRect& aExclude,
                       nsDisplayItem* aItem)
{
  nsRect damageRect;
  if (damageRect.IntersectRect(aUpdateRect, aItemRect)) {
    nsRegion r;
    r.Sub(damageRect, aExclude);
#ifdef DEBUG
    if (gDumpRepaintRegionForCopy) {
      nsRect bounds = r.GetBounds();
      fprintf(stderr, "Adding rect %d,%d,%d,%d for frame %p\n",
              bounds.x, bounds.y, bounds.width, bounds.height,
              (void*)aItem->GetUnderlyingFrame());
    }
#endif
    aRegion->Or(*aRegion, r);
  }
}

static void
AddItemsToRegion(nsDisplayListBuilder* aBuilder, nsDisplayList* aList,
                 const nsRect& aUpdateRect, const nsRect& aClipRect, nsPoint aDelta,
                 nsRegion* aRegion)
{
  for (nsDisplayItem* item = aList->GetBottom(); item; item = item->GetAbove()) {
    nsDisplayList* sublist = item->GetList();
    if (sublist) {
      nsDisplayItem::Type type = item->GetType();
#ifdef MOZ_SVG
      if (type == nsDisplayItem::TYPE_SVG_EFFECTS) {
        nsDisplaySVGEffects* effectsItem = static_cast<nsDisplaySVGEffects*>(item);
        if (!aBuilder->IsMovingFrame(effectsItem->GetEffectsFrame())) {
          
          nsRect r;
          r.IntersectRect(aClipRect, effectsItem->GetBounds(aBuilder));
          aRegion->Or(*aRegion, r);
        }
      } else
#endif
      if (type == nsDisplayItem::TYPE_CLIP) {
        nsDisplayClip* clipItem = static_cast<nsDisplayClip*>(item);
        nsRect clip = aClipRect;
        
        
        
        nsIFrame* clipFrame = clipItem->GetClippingFrame();
        if (!aBuilder->IsMovingFrame(clipFrame)) {
          nscoord appUnitsPerDevPixel = clipFrame->PresContext()->AppUnitsPerDevPixel();
          
          
          
          nsRect snappedClip =
            clipItem->GetClipRect().ToNearestPixels(appUnitsPerDevPixel).
            ToAppUnits(appUnitsPerDevPixel);
          clip.IntersectRect(clip, snappedClip);

          
          nsRegion clippedOutSource;
          clippedOutSource.Sub(aUpdateRect - aDelta, clip);
          clippedOutSource.MoveBy(aDelta);
          aRegion->Or(*aRegion, clippedOutSource);

          
          nsRegion clippedOutDestination;
          clippedOutDestination.Sub(aUpdateRect, clip);
          aRegion->Or(*aRegion, clippedOutDestination);
        }
        AddItemsToRegion(aBuilder, sublist, aUpdateRect, clip, aDelta, aRegion);
      } else {
        
        AddItemsToRegion(aBuilder, sublist, aUpdateRect, aClipRect, aDelta, aRegion);
      }
    } else {
      nsRect r;
      if (r.IntersectRect(aClipRect, item->GetBounds(aBuilder))) {
        nsIFrame* f = item->GetUnderlyingFrame();
        NS_ASSERTION(f, "Must have an underlying frame for leaf item");
        nsRect exclude;
        if (aBuilder->IsMovingFrame(f)) {
          if (item->IsVaryingRelativeToMovingFrame(aBuilder)) {
            
            
            AccumulateItemInRegion(aRegion, aUpdateRect, r, exclude, item);
          }
        } else {
          
          if (item->IsUniform(aBuilder)) {
            
            
            exclude.IntersectRect(r, r + aDelta);
          }
          
          AccumulateItemInRegion(aRegion, aUpdateRect, r, exclude, item);
          
          
          
          AccumulateItemInRegion(aRegion, aUpdateRect, r + aDelta,
                                 exclude, item);
        }
      }
    }
  }
}

nsresult
nsLayoutUtils::ComputeRepaintRegionForCopy(nsIFrame* aRootFrame,
                                           nsIFrame* aMovingFrame,
                                           nsPoint aDelta,
                                           const nsRect& aUpdateRect,
                                           nsRegion* aBlitRegion,
                                           nsRegion* aRepaintRegion)
{
  NS_ASSERTION(aRootFrame != aMovingFrame,
               "The root frame shouldn't be the one that's moving, that makes no sense");

  nsAutoDisableGetUsedXAssertions disableAssert;

  
  
  
  
  
  
  
  
  
  
  
  
  nsDisplayListBuilder builder(aRootFrame, PR_FALSE, PR_TRUE);
  
  
  
  nsRegion visibleRegionOfMovingContent;
  builder.SetMovingFrame(aMovingFrame, aDelta, &visibleRegionOfMovingContent);
  nsDisplayList list;

  builder.EnterPresShell(aRootFrame, aUpdateRect);

  nsresult rv =
    aRootFrame->BuildDisplayListForStackingContext(&builder, aUpdateRect, &list);

  builder.LeavePresShell(aRootFrame, aUpdateRect);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  if (gDumpRepaintRegionForCopy) {
    fprintf(stderr,
            "Repaint region for copy --- before optimization (area %d,%d,%d,%d, frame %p):\n",
            aUpdateRect.x, aUpdateRect.y, aUpdateRect.width, aUpdateRect.height,
            (void*)aMovingFrame);
    nsFrame::PrintDisplayList(&builder, list);
  }
#endif

  
  
  nsRegion visibleRegion(aUpdateRect);
  nsRegion visibleRegionBeforeMove(aUpdateRect);
  list.ComputeVisibility(&builder, &visibleRegion, &visibleRegionBeforeMove);

#ifdef DEBUG
  if (gDumpRepaintRegionForCopy) {
    fprintf(stderr, "Repaint region for copy --- after optimization:\n");
    nsFrame::PrintDisplayList(&builder, list);
  }
#endif

  
  
  
  
  
  
  
  nsRegion scrolledOutOfView;
  scrolledOutOfView.Sub(aUpdateRect, aUpdateRect - aDelta);
  visibleRegionOfMovingContent.Or(visibleRegionOfMovingContent, scrolledOutOfView);

  aRepaintRegion->SetEmpty();
  
  
  
  
  
  
  
  
  
  AddItemsToRegion(&builder, &list, aUpdateRect, aUpdateRect, aDelta, aRepaintRegion);
  
  list.DeleteAll();

  
  
  
  visibleRegionOfMovingContent.And(visibleRegionOfMovingContent, aUpdateRect);
  aRepaintRegion->And(*aRepaintRegion, visibleRegionOfMovingContent);
  aBlitRegion->Sub(visibleRegionOfMovingContent, *aRepaintRegion);
  return NS_OK;
}

PRInt32
nsLayoutUtils::GetZIndex(nsIFrame* aFrame) {
  if (!aFrame->GetStyleDisplay()->IsPositioned())
    return 0;

  const nsStylePosition* position =
    aFrame->GetStylePosition();
  if (position->mZIndex.GetUnit() == eStyleUnit_Integer)
    return position->mZIndex.GetIntValue();

  
  return 0;
}













PRBool
nsLayoutUtils::BinarySearchForPosition(nsIRenderingContext* aRendContext, 
                        const PRUnichar* aText,
                        PRInt32    aBaseWidth,
                        PRInt32    aBaseInx,
                        PRInt32    aStartInx, 
                        PRInt32    aEndInx, 
                        PRInt32    aCursorPos, 
                        PRInt32&   aIndex,
                        PRInt32&   aTextWidth)
{
  PRInt32 range = aEndInx - aStartInx;
  if ((range == 1) || (range == 2 && NS_IS_HIGH_SURROGATE(aText[aStartInx]))) {
    aIndex   = aStartInx + aBaseInx;
    aRendContext->GetWidth(aText, aIndex, aTextWidth);
    return PR_TRUE;
  }

  PRInt32 inx = aStartInx + (range / 2);

  
  if (NS_IS_HIGH_SURROGATE(aText[inx-1]))
    inx++;

  PRInt32 textWidth = 0;
  aRendContext->GetWidth(aText, inx, textWidth);

  PRInt32 fullWidth = aBaseWidth + textWidth;
  if (fullWidth == aCursorPos) {
    aTextWidth = textWidth;
    aIndex = inx;
    return PR_TRUE;
  } else if (aCursorPos < fullWidth) {
    aTextWidth = aBaseWidth;
    if (BinarySearchForPosition(aRendContext, aText, aBaseWidth, aBaseInx, aStartInx, inx, aCursorPos, aIndex, aTextWidth)) {
      return PR_TRUE;
    }
  } else {
    aTextWidth = fullWidth;
    if (BinarySearchForPosition(aRendContext, aText, aBaseWidth, aBaseInx, inx, aEndInx, aCursorPos, aIndex, aTextWidth)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

static void
AddBoxesForFrame(nsIFrame* aFrame,
                 nsLayoutUtils::BoxCallback* aCallback)
{
  nsIAtom* pseudoType = aFrame->GetStyleContext()->GetPseudo();

  if (pseudoType == nsCSSAnonBoxes::tableOuter) {
    AddBoxesForFrame(aFrame->GetFirstChild(nsnull), aCallback);
    nsIFrame* kid = aFrame->GetFirstChild(nsGkAtoms::captionList);
    if (kid) {
      AddBoxesForFrame(kid, aCallback);
    }
  } else if (pseudoType == nsCSSAnonBoxes::mozAnonymousBlock ||
             pseudoType == nsCSSAnonBoxes::mozAnonymousPositionedBlock ||
             pseudoType == nsCSSAnonBoxes::mozMathMLAnonymousBlock ||
             pseudoType == nsCSSAnonBoxes::mozXULAnonymousBlock) {
    for (nsIFrame* kid = aFrame->GetFirstChild(nsnull); kid; kid = kid->GetNextSibling()) {
      AddBoxesForFrame(kid, aCallback);
    }
  } else {
    aCallback->AddBox(aFrame);
  }
}

void
nsLayoutUtils::GetAllInFlowBoxes(nsIFrame* aFrame, BoxCallback* aCallback)
{
  while (aFrame) {
    AddBoxesForFrame(aFrame, aCallback);
    aFrame = nsLayoutUtils::GetNextContinuationOrSpecialSibling(aFrame);
  }
}

struct BoxToBorderRect : public nsLayoutUtils::BoxCallback {
  nsIFrame*                    mRelativeTo;
  nsLayoutUtils::RectCallback* mCallback;

  BoxToBorderRect(nsIFrame* aRelativeTo, nsLayoutUtils::RectCallback* aCallback)
    : mRelativeTo(aRelativeTo), mCallback(aCallback) {}

  virtual void AddBox(nsIFrame* aFrame) {
#ifdef MOZ_SVG
    nsRect r;
    nsIFrame* outer = nsSVGUtils::GetOuterSVGFrameAndCoveredRegion(aFrame, &r);
    if (outer) {
      mCallback->AddRect(r + outer->GetOffsetTo(mRelativeTo));
    } else
#endif
      mCallback->AddRect(nsRect(aFrame->GetOffsetTo(mRelativeTo), aFrame->GetSize()));
  }
};

void
nsLayoutUtils::GetAllInFlowRects(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                 RectCallback* aCallback)
{
  BoxToBorderRect converter(aRelativeTo, aCallback);
  GetAllInFlowBoxes(aFrame, &converter);
}

nsLayoutUtils::RectAccumulator::RectAccumulator() : mSeenFirstRect(PR_FALSE) {}

void nsLayoutUtils::RectAccumulator::AddRect(const nsRect& aRect) {
  mResultRect.UnionRect(mResultRect, aRect);
  if (!mSeenFirstRect) {
    mSeenFirstRect = PR_TRUE;
    mFirstRect = aRect;
  }
}

nsLayoutUtils::RectListBuilder::RectListBuilder(nsClientRectList* aList)
  : mRectList(aList), mRV(NS_OK) {}

void nsLayoutUtils::RectListBuilder::AddRect(const nsRect& aRect) {
  nsRefPtr<nsClientRect> rect = new nsClientRect();
  if (!rect) {
    mRV = NS_ERROR_OUT_OF_MEMORY;
    return;
  }

  rect->SetLayoutRect(aRect);
  mRectList->Append(rect);
}

nsIFrame* nsLayoutUtils::GetContainingBlockForClientRect(nsIFrame* aFrame)
{
  
  while (aFrame->GetParent() &&
         !aFrame->IsFrameOfType(nsIFrame::eSVGForeignObject)) {
    aFrame = aFrame->GetParent();
  }

  return aFrame;
}

nsRect
nsLayoutUtils::GetAllInFlowRectsUnion(nsIFrame* aFrame, nsIFrame* aRelativeTo) {
  RectAccumulator accumulator;
  GetAllInFlowRects(aFrame, aRelativeTo, &accumulator);
  return accumulator.mResultRect.IsEmpty() ? accumulator.mFirstRect
          : accumulator.mResultRect;
}

nsRect
nsLayoutUtils::GetTextShadowRectsUnion(const nsRect& aTextAndDecorationsRect,
                                       nsIFrame* aFrame)
{
  const nsStyleText* textStyle = aFrame->GetStyleText();
  if (!textStyle->mTextShadow)
    return aTextAndDecorationsRect;

  nsRect resultRect = aTextAndDecorationsRect;
  for (PRUint32 i = 0; i < textStyle->mTextShadow->Length(); ++i) {
    nsRect tmpRect(aTextAndDecorationsRect);
    nsCSSShadowItem* shadow = textStyle->mTextShadow->ShadowAt(i);

    tmpRect.MoveBy(nsPoint(shadow->mXOffset, shadow->mYOffset));
    tmpRect.Inflate(shadow->mRadius, shadow->mRadius);

    resultRect.UnionRect(resultRect, tmpRect);
  }
  return resultRect;
}

nsresult
nsLayoutUtils::GetFontMetricsForFrame(nsIFrame* aFrame,
                                      nsIFontMetrics** aFontMetrics)
{
  return nsLayoutUtils::GetFontMetricsForStyleContext(aFrame->GetStyleContext(),
                                                      aFontMetrics);
}

nsresult
nsLayoutUtils::GetFontMetricsForStyleContext(nsStyleContext* aStyleContext,
                                             nsIFontMetrics** aFontMetrics)
{
  
  gfxUserFontSet* fs = aStyleContext->PresContext()->GetUserFontSet();
  
  return aStyleContext->PresContext()->DeviceContext()->GetMetricsFor(
                  aStyleContext->GetStyleFont()->mFont,
                  aStyleContext->GetStyleVisibility()->mLangGroup,
                  fs, *aFontMetrics);
}

nsIFrame*
nsLayoutUtils::FindChildContainingDescendant(nsIFrame* aParent, nsIFrame* aDescendantFrame)
{
  nsIFrame* result = aDescendantFrame;

  while (result) {
    nsIFrame* parent = result->GetParent();
    if (parent == aParent) {
      break;
    }

    
    result = parent;
  }

  return result;
}

nsBlockFrame*
nsLayoutUtils::GetAsBlock(nsIFrame* aFrame)
{
  nsBlockFrame* block = do_QueryFrame(aFrame);
  return block;
}

nsBlockFrame*
nsLayoutUtils::FindNearestBlockAncestor(nsIFrame* aFrame)
{
  nsIFrame* nextAncestor;
  for (nextAncestor = aFrame->GetParent(); nextAncestor;
       nextAncestor = nextAncestor->GetParent()) {
    nsBlockFrame* block = GetAsBlock(nextAncestor);
    if (block)
      return block;
  }
  return nsnull;
}

nsIFrame*
nsLayoutUtils::GetNonGeneratedAncestor(nsIFrame* aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT))
    return aFrame;

  nsFrameManager* frameManager = aFrame->PresContext()->FrameManager();
  nsIFrame* f = aFrame;
  do {
    f = GetParentOrPlaceholderFor(frameManager, f);
  } while (f->GetStateBits() & NS_FRAME_GENERATED_CONTENT);
  return f;
}

nsIFrame*
nsLayoutUtils::GetParentOrPlaceholderFor(nsFrameManager* aFrameManager,
                                         nsIFrame* aFrame)
{
  if ((aFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
      && !aFrame->GetPrevInFlow()) {
    return aFrameManager->GetPlaceholderFrameFor(aFrame);
  }
  return aFrame->GetParent();
}

nsIFrame*
nsLayoutUtils::GetClosestCommonAncestorViaPlaceholders(nsIFrame* aFrame1,
                                                       nsIFrame* aFrame2,
                                                       nsIFrame* aKnownCommonAncestorHint)
{
  NS_PRECONDITION(aFrame1, "aFrame1 must not be null");
  NS_PRECONDITION(aFrame2, "aFrame2 must not be null");

  nsPresContext* presContext = aFrame1->PresContext();
  if (presContext != aFrame2->PresContext()) {
    
    return nsnull;
  }
  nsFrameManager* frameManager = presContext->PresShell()->FrameManager();

  nsAutoTArray<nsIFrame*, 8> frame1Ancestors;
  nsIFrame* f1;
  for (f1 = aFrame1; f1 && f1 != aKnownCommonAncestorHint;
       f1 = GetParentOrPlaceholderFor(frameManager, f1)) {
    frame1Ancestors.AppendElement(f1);
  }
  if (!f1 && aKnownCommonAncestorHint) {
    
    
    aKnownCommonAncestorHint = nsnull;
  }

  nsAutoTArray<nsIFrame*, 8> frame2Ancestors;
  nsIFrame* f2;
  for (f2 = aFrame2; f2 && f2 != aKnownCommonAncestorHint;
       f2 = GetParentOrPlaceholderFor(frameManager, f2)) {
    frame2Ancestors.AppendElement(f2);
  }
  if (!f2 && aKnownCommonAncestorHint) {
    
    
    return GetClosestCommonAncestorViaPlaceholders(aFrame1, aFrame2, nsnull);
  }

  
  
  
  
  nsIFrame* lastCommonFrame = aKnownCommonAncestorHint;
  PRInt32 last1 = frame1Ancestors.Length() - 1;
  PRInt32 last2 = frame2Ancestors.Length() - 1;
  while (last1 >= 0 && last2 >= 0) {
    nsIFrame* frame1 = frame1Ancestors.ElementAt(last1);
    if (frame1 != frame2Ancestors.ElementAt(last2))
      break;
    lastCommonFrame = frame1;
    last1--;
    last2--;
  }
  return lastCommonFrame;
}

nsIFrame*
nsLayoutUtils::GetNextContinuationOrSpecialSibling(nsIFrame *aFrame)
{
  nsIFrame *result = aFrame->GetNextContinuation();
  if (result)
    return result;

  if ((aFrame->GetStateBits() & NS_FRAME_IS_SPECIAL) != 0) {
    
    
    aFrame = aFrame->GetFirstContinuation();

    void* value = aFrame->GetProperty(nsGkAtoms::IBSplitSpecialSibling);
    return static_cast<nsIFrame*>(value);
  }

  return nsnull;
}

nsIFrame*
nsLayoutUtils::GetFirstContinuationOrSpecialSibling(nsIFrame *aFrame)
{
  nsIFrame *result = aFrame->GetFirstContinuation();
  if (result->GetStateBits() & NS_FRAME_IS_SPECIAL) {
    while (PR_TRUE) {
      nsIFrame *f = static_cast<nsIFrame*>
        (result->GetProperty(nsGkAtoms::IBSplitSpecialPrevSibling));
      if (!f)
        break;
      result = f;
    }
  }

  return result;
}

PRBool
nsLayoutUtils::IsViewportScrollbarFrame(nsIFrame* aFrame)
{
  if (!aFrame)
    return PR_FALSE;

  nsIFrame* rootScrollFrame =
    aFrame->PresContext()->PresShell()->GetRootScrollFrame();
  if (!rootScrollFrame)
    return PR_FALSE;

  nsIScrollableFrame* rootScrollableFrame = do_QueryFrame(rootScrollFrame);
  NS_ASSERTION(rootScrollableFrame, "The root scorollable frame is null");

  if (!IsProperAncestorFrame(rootScrollFrame, aFrame))
    return PR_FALSE;

  nsIFrame* rootScrolledFrame = rootScrollableFrame->GetScrolledFrame();
  return !(rootScrolledFrame == aFrame ||
           IsProperAncestorFrame(rootScrolledFrame, aFrame));
}

static nscoord AddPercents(nsLayoutUtils::IntrinsicWidthType aType,
                           nscoord aCurrent, float aPercent)
{
  nscoord result = aCurrent;
  if (aPercent > 0.0f && aType == nsLayoutUtils::PREF_WIDTH) {
    
    
    if (aPercent >= 1.0f)
      result = nscoord_MAX;
    else
      result = NSToCoordRound(float(result) / (1.0f - aPercent));
  }
  return result;
}

static PRBool GetAbsoluteCoord(const nsStyleCoord& aStyle, nscoord& aResult)
{
  if (eStyleUnit_Coord != aStyle.GetUnit())
    return PR_FALSE;
  
  aResult = aStyle.GetCoordValue();
  return PR_TRUE;
}

static PRBool
GetPercentHeight(const nsStyleCoord& aStyle,
                 nsIFrame* aFrame,
                 nscoord& aResult)
{
  if (eStyleUnit_Percent != aStyle.GetUnit())
    return PR_FALSE;

  nsIFrame *f;
  for (f = aFrame->GetParent(); f && !f->IsContainingBlock();
       f = f->GetParent())
    ;
  if (!f) {
    NS_NOTREACHED("top of frame tree not a containing block");
    return PR_FALSE;
  }

  const nsStylePosition *pos = f->GetStylePosition();
  nscoord h;
  if (!GetAbsoluteCoord(pos->mHeight, h) &&
      !GetPercentHeight(pos->mHeight, f, h)) {
    NS_ASSERTION(pos->mHeight.GetUnit() == eStyleUnit_Auto ||
                 pos->mHeight.GetUnit() == eStyleUnit_Percent,
                 "unknown height unit");
    
    
    
    
    
    return PR_FALSE;
  }

  nscoord maxh;
  if (GetAbsoluteCoord(pos->mMaxHeight, maxh) ||
      GetPercentHeight(pos->mMaxHeight, f, maxh)) {
    if (maxh < h)
      h = maxh;
  } else {
    NS_ASSERTION(pos->mMaxHeight.GetUnit() == eStyleUnit_None ||
                 pos->mMaxHeight.GetUnit() == eStyleUnit_Percent,
                 "unknown max-height unit");
  }

  nscoord minh;
  if (GetAbsoluteCoord(pos->mMinHeight, minh) ||
      GetPercentHeight(pos->mMinHeight, f, minh)) {
    if (minh > h)
      h = minh;
  } else {
    NS_ASSERTION(pos->mMinHeight.GetUnit() == eStyleUnit_Percent,
                 "unknown min-height unit");
  }

  aResult = NSToCoordRound(aStyle.GetPercentValue() * h);
  return PR_TRUE;
}





enum eWidthProperty { PROP_WIDTH, PROP_MAX_WIDTH, PROP_MIN_WIDTH };
static PRBool
GetIntrinsicCoord(const nsStyleCoord& aStyle,
                  nsIRenderingContext* aRenderingContext,
                  nsIFrame* aFrame,
                  eWidthProperty aProperty,
                  nscoord& aResult)
{
  NS_PRECONDITION(aProperty == PROP_WIDTH || aProperty == PROP_MAX_WIDTH ||
                  aProperty == PROP_MIN_WIDTH, "unexpected property");
  if (aStyle.GetUnit() != eStyleUnit_Enumerated)
    return PR_FALSE;
  PRInt32 val = aStyle.GetIntValue();
  NS_ASSERTION(val == NS_STYLE_WIDTH_MAX_CONTENT ||
               val == NS_STYLE_WIDTH_MIN_CONTENT ||
               val == NS_STYLE_WIDTH_FIT_CONTENT ||
               val == NS_STYLE_WIDTH_AVAILABLE,
               "unexpected enumerated value for width property");
  if (val == NS_STYLE_WIDTH_AVAILABLE)
    return PR_FALSE;
  if (val == NS_STYLE_WIDTH_FIT_CONTENT) {
    if (aProperty == PROP_WIDTH)
      return PR_FALSE; 
    if (aProperty == PROP_MAX_WIDTH)
      
      val = NS_STYLE_WIDTH_MAX_CONTENT;
    else
      
      val = NS_STYLE_WIDTH_MIN_CONTENT;
  }

  NS_ASSERTION(val == NS_STYLE_WIDTH_MAX_CONTENT ||
               val == NS_STYLE_WIDTH_MIN_CONTENT,
               "should have reduced everything remaining to one of these");
  if (val == NS_STYLE_WIDTH_MAX_CONTENT)
    aResult = aFrame->GetPrefWidth(aRenderingContext);
  else
    aResult = aFrame->GetMinWidth(aRenderingContext);
  return PR_TRUE;
}

#undef  DEBUG_INTRINSIC_WIDTH

#ifdef DEBUG_INTRINSIC_WIDTH
static PRInt32 gNoiseIndent = 0;
#endif

 nscoord
nsLayoutUtils::IntrinsicForContainer(nsIRenderingContext *aRenderingContext,
                                     nsIFrame *aFrame,
                                     IntrinsicWidthType aType)
{
  NS_PRECONDITION(aFrame, "null frame");
  NS_PRECONDITION(aType == MIN_WIDTH || aType == PREF_WIDTH, "bad type");

#ifdef DEBUG_INTRINSIC_WIDTH
  nsFrame::IndentBy(stdout, gNoiseIndent);
  static_cast<nsFrame*>(aFrame)->ListTag(stdout);
  printf(" %s intrinsic width for container:\n",
         aType == MIN_WIDTH ? "min" : "pref");
#endif

  nsIFrame::IntrinsicWidthOffsetData offsets =
    aFrame->IntrinsicWidthOffsets(aRenderingContext);

  const nsStylePosition *stylePos = aFrame->GetStylePosition();
  PRUint8 boxSizing = stylePos->mBoxSizing;
  const nsStyleCoord &styleWidth = stylePos->mWidth;
  const nsStyleCoord &styleMinWidth = stylePos->mMinWidth;
  const nsStyleCoord &styleMaxWidth = stylePos->mMaxWidth;

  
  
  
  
  
  
  
  
  
  
  nscoord result = 0, min = 0;

  
  
  
  if (styleWidth.GetUnit() == eStyleUnit_Enumerated &&
      (styleWidth.GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
       styleWidth.GetIntValue() == NS_STYLE_WIDTH_MIN_CONTENT)) {
    
    
    
    
    boxSizing = NS_STYLE_BOX_SIZING_CONTENT;
  } else if (styleWidth.GetUnit() != eStyleUnit_Coord &&
             (styleMinWidth.GetUnit() != eStyleUnit_Coord ||
              styleMaxWidth.GetUnit() != eStyleUnit_Coord ||
              styleMaxWidth.GetCoordValue() > styleMinWidth.GetCoordValue())) {
#ifdef DEBUG_INTRINSIC_WIDTH
    ++gNoiseIndent;
#endif
    if (aType == MIN_WIDTH)
      result = aFrame->GetMinWidth(aRenderingContext);
    else
      result = aFrame->GetPrefWidth(aRenderingContext);
#ifdef DEBUG_INTRINSIC_WIDTH
    --gNoiseIndent;
    nsFrame::IndentBy(stdout, gNoiseIndent);
    static_cast<nsFrame*>(aFrame)->ListTag(stdout);
    printf(" %s intrinsic width from frame is %d.\n",
           aType == MIN_WIDTH ? "min" : "pref", result);
#endif

    
    
    const nsStyleCoord &styleHeight = stylePos->mHeight;
    const nsStyleCoord &styleMinHeight = stylePos->mMinHeight;
    const nsStyleCoord &styleMaxHeight = stylePos->mMaxHeight;
    if (styleHeight.GetUnit() != eStyleUnit_Auto ||
        !(styleMinHeight.GetUnit() == eStyleUnit_Coord &&
          styleMinHeight.GetCoordValue() == 0) ||
        styleMaxHeight.GetUnit() != eStyleUnit_None) {

      nsSize ratio = aFrame->GetIntrinsicRatio();

      if (ratio.height != 0) {

        nscoord h;
        if (GetAbsoluteCoord(styleHeight, h) ||
            GetPercentHeight(styleHeight, aFrame, h)) {
          result =
            NSToCoordRound(h * (float(ratio.width) / float(ratio.height)));
        }

        if (GetAbsoluteCoord(styleMaxHeight, h) ||
            GetPercentHeight(styleMaxHeight, aFrame, h)) {
          h = NSToCoordRound(h * (float(ratio.width) / float(ratio.height)));
          if (h < result)
            result = h;
        }

        if (GetAbsoluteCoord(styleMinHeight, h) ||
            GetPercentHeight(styleMinHeight, aFrame, h)) {
          h = NSToCoordRound(h * (float(ratio.width) / float(ratio.height)));
          if (h > result)
            result = h;
        }
      }
    }
  }
      
  if (aFrame->GetType() == nsGkAtoms::tableFrame) {
    
    
    min = aFrame->GetMinWidth(aRenderingContext);
  }

  
  
  
  
  
  
  
  nscoord coordOutsideWidth = offsets.hPadding;
  float pctOutsideWidth = offsets.hPctPadding;

  float pctTotal = 0.0f;

  if (boxSizing == NS_STYLE_BOX_SIZING_PADDING) {
    min += coordOutsideWidth;
    result = NSCoordSaturatingAdd(result, coordOutsideWidth);
    pctTotal += pctOutsideWidth;

    coordOutsideWidth = 0;
    pctOutsideWidth = 0.0f;
  }

  coordOutsideWidth += offsets.hBorder;

  if (boxSizing == NS_STYLE_BOX_SIZING_BORDER) {
    min += coordOutsideWidth;
    result = NSCoordSaturatingAdd(result, coordOutsideWidth);
    pctTotal += pctOutsideWidth;

    coordOutsideWidth = 0;
    pctOutsideWidth = 0.0f;
  }

  coordOutsideWidth += offsets.hMargin;
  pctOutsideWidth += offsets.hPctMargin;

  min += coordOutsideWidth;
  result = NSCoordSaturatingAdd(result, coordOutsideWidth);
  pctTotal += pctOutsideWidth;

  nscoord w;
  if (GetAbsoluteCoord(styleWidth, w) ||
      GetIntrinsicCoord(styleWidth, aRenderingContext, aFrame,
                        PROP_WIDTH, w)) {
    result = AddPercents(aType, w + coordOutsideWidth, pctOutsideWidth);
  }
  else if (aType == MIN_WIDTH && eStyleUnit_Percent == styleWidth.GetUnit() &&
           aFrame->IsFrameOfType(nsIFrame::eReplaced)) {
    
    result = 0; 
  }
  else {
    result = AddPercents(aType, result, pctTotal);
  }

  nscoord maxw;
  if (GetAbsoluteCoord(styleMaxWidth, maxw) ||
      GetIntrinsicCoord(styleMaxWidth, aRenderingContext, aFrame,
                        PROP_MAX_WIDTH, maxw)) {
    maxw = AddPercents(aType, maxw + coordOutsideWidth, pctOutsideWidth);
    if (result > maxw)
      result = maxw;
  }

  nscoord minw;
  if (GetAbsoluteCoord(styleMinWidth, minw) ||
      GetIntrinsicCoord(styleMinWidth, aRenderingContext, aFrame,
                        PROP_MIN_WIDTH, minw)) {
    minw = AddPercents(aType, minw + coordOutsideWidth, pctOutsideWidth);
    if (result < minw)
      result = minw;
  }

  min = AddPercents(aType, min, pctTotal);
  if (result < min)
    result = min;

  const nsStyleDisplay *disp = aFrame->GetStyleDisplay();
  if (aFrame->IsThemed(disp)) {
    nsIntSize size(0, 0);
    PRBool canOverride = PR_TRUE;
    nsPresContext *presContext = aFrame->PresContext();
    presContext->GetTheme()->
      GetMinimumWidgetSize(aRenderingContext, aFrame, disp->mAppearance,
                           &size, &canOverride);

    nscoord themeWidth = presContext->DevPixelsToAppUnits(size.width);

    
    themeWidth += offsets.hMargin;
    themeWidth = AddPercents(aType, themeWidth, offsets.hPctMargin);

    if (themeWidth > result || !canOverride)
      result = themeWidth;
  }

#ifdef DEBUG_INTRINSIC_WIDTH
  nsFrame::IndentBy(stdout, gNoiseIndent);
  static_cast<nsFrame*>(aFrame)->ListTag(stdout);
  printf(" %s intrinsic width for container is %d twips.\n",
         aType == MIN_WIDTH ? "min" : "pref", result);
#endif

  return result;
}

 nscoord
nsLayoutUtils::ComputeWidthDependentValue(
                 nscoord              aContainingBlockWidth,
                 const nsStyleCoord&  aCoord)
{
  NS_PRECONDITION(aContainingBlockWidth != NS_UNCONSTRAINEDSIZE,
                  "unconstrained widths no longer supported");

  if (eStyleUnit_Coord == aCoord.GetUnit()) {
    return aCoord.GetCoordValue();
  }
  if (eStyleUnit_Percent == aCoord.GetUnit()) {
    return NSToCoordFloor(aContainingBlockWidth * aCoord.GetPercentValue());
  }
  NS_ASSERTION(aCoord.GetUnit() == eStyleUnit_None ||
               aCoord.GetUnit() == eStyleUnit_Auto,
               "unexpected width value");
  return 0;
}

 nscoord
nsLayoutUtils::ComputeWidthValue(
                 nsIRenderingContext* aRenderingContext,
                 nsIFrame*            aFrame,
                 nscoord              aContainingBlockWidth,
                 nscoord              aContentEdgeToBoxSizing,
                 nscoord              aBoxSizingToMarginEdge,
                 const nsStyleCoord&  aCoord)
{
  NS_PRECONDITION(aFrame, "non-null frame expected");
  NS_PRECONDITION(aRenderingContext, "non-null rendering context expected");
  NS_WARN_IF_FALSE(aContainingBlockWidth != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  NS_PRECONDITION(aContainingBlockWidth >= 0,
                  "width less than zero");

  nscoord result;
  if (eStyleUnit_Coord == aCoord.GetUnit()) {
    result = aCoord.GetCoordValue();
    NS_ASSERTION(result >= 0, "width less than zero");
    result -= aContentEdgeToBoxSizing;
  } else if (eStyleUnit_Percent == aCoord.GetUnit()) {
    NS_ASSERTION(aCoord.GetPercentValue() >= 0.0f, "width less than zero");
    result = NSToCoordFloor(aContainingBlockWidth * aCoord.GetPercentValue()) -
             aContentEdgeToBoxSizing;
  } else if (eStyleUnit_Enumerated == aCoord.GetUnit()) {
    PRInt32 val = aCoord.GetIntValue();
    switch (val) {
      case NS_STYLE_WIDTH_MAX_CONTENT:
        result = aFrame->GetPrefWidth(aRenderingContext);
        NS_ASSERTION(result >= 0, "width less than zero");
        break;
      case NS_STYLE_WIDTH_MIN_CONTENT:
        result = aFrame->GetMinWidth(aRenderingContext);
        NS_ASSERTION(result >= 0, "width less than zero");
        break;
      case NS_STYLE_WIDTH_FIT_CONTENT:
        {
          nscoord pref = aFrame->GetPrefWidth(aRenderingContext),
                   min = aFrame->GetMinWidth(aRenderingContext),
                  fill = aContainingBlockWidth -
                         (aBoxSizingToMarginEdge + aContentEdgeToBoxSizing);
          result = NS_MAX(min, NS_MIN(pref, fill));
          NS_ASSERTION(result >= 0, "width less than zero");
        }
        break;
      case NS_STYLE_WIDTH_AVAILABLE:
        result = aContainingBlockWidth -
                 (aBoxSizingToMarginEdge + aContentEdgeToBoxSizing);
    }
  } else {
    NS_NOTREACHED("unexpected width value");
    result = 0;
  }
  if (result < 0)
    result = 0;
  return result;
}


 nscoord
nsLayoutUtils::ComputeHeightDependentValue(
                 nscoord              aContainingBlockHeight,
                 const nsStyleCoord&  aCoord)
{
  if (eStyleUnit_Coord == aCoord.GetUnit()) {
    return aCoord.GetCoordValue();
  }
  if (eStyleUnit_Percent == aCoord.GetUnit()) {
    
    
    
    
    
    
    
    NS_PRECONDITION(NS_AUTOHEIGHT != aContainingBlockHeight,
                    "unexpected 'containing block height'");

    if (NS_AUTOHEIGHT != aContainingBlockHeight) {
      return NSToCoordFloor(aContainingBlockHeight * aCoord.GetPercentValue());
    }
  }
  NS_ASSERTION(aCoord.GetUnit() == eStyleUnit_None ||
               aCoord.GetUnit() == eStyleUnit_Auto,
               "unexpected height value");
  return 0;
}

inline PRBool
IsAutoHeight(const nsStyleCoord &aCoord, nscoord aCBHeight)
{
  nsStyleUnit unit = aCoord.GetUnit();
  return unit == eStyleUnit_Auto ||  
         unit == eStyleUnit_None ||  
         (unit == eStyleUnit_Percent && 
          aCBHeight == NS_AUTOHEIGHT);
}

#define MULDIV(a,b,c) (nscoord(PRInt64(a) * PRInt64(b) / PRInt64(c)))

 nsSize
nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(
                   nsIRenderingContext* aRenderingContext, nsIFrame* aFrame,
                   const nsIFrame::IntrinsicSize& aIntrinsicSize,
                   nsSize aIntrinsicRatio, nsSize aCBSize,
                   nsSize aMargin, nsSize aBorder, nsSize aPadding)
{
  const nsStylePosition *stylePos = aFrame->GetStylePosition();
  
  
  

  
  
  

  const PRBool isAutoWidth = stylePos->mWidth.GetUnit() == eStyleUnit_Auto;
  const PRBool isAutoHeight = IsAutoHeight(stylePos->mHeight, aCBSize.height);

  nsSize boxSizingAdjust(0,0);
  switch (stylePos->mBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      boxSizingAdjust += aBorder;
      
    case NS_STYLE_BOX_SIZING_PADDING:
      boxSizingAdjust += aPadding;
  }
  nscoord boxSizingToMarginEdgeWidth =
    aMargin.width + aBorder.width + aPadding.width - boxSizingAdjust.width;

  nscoord width, minWidth, maxWidth, height, minHeight, maxHeight;

  if (!isAutoWidth) {
    width = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
              aFrame, aCBSize.width, boxSizingAdjust.width,
              boxSizingToMarginEdgeWidth, stylePos->mWidth);
    NS_ASSERTION(width >= 0, "negative result from ComputeWidthValue");
  }

  if (stylePos->mMaxWidth.GetUnit() != eStyleUnit_None) {
    maxWidth = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
                 aFrame, aCBSize.width, boxSizingAdjust.width,
                 boxSizingToMarginEdgeWidth, stylePos->mMaxWidth);
    NS_ASSERTION(maxWidth >= 0, "negative result from ComputeWidthValue");
  } else {
    maxWidth = nscoord_MAX;
  }

  minWidth = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
               aFrame, aCBSize.width, boxSizingAdjust.width,
               boxSizingToMarginEdgeWidth, stylePos->mMinWidth);
  NS_ASSERTION(minWidth >= 0, "negative result from ComputeWidthValue");

  if (!isAutoHeight) {
    height = nsLayoutUtils::
      ComputeHeightDependentValue(aCBSize.height, stylePos->mHeight) -
      boxSizingAdjust.height;
    if (height < 0)
      height = 0;
  }

  if (!IsAutoHeight(stylePos->mMaxHeight, aCBSize.height)) {
    maxHeight = nsLayoutUtils::
      ComputeHeightDependentValue(aCBSize.height, stylePos->mMaxHeight) -
      boxSizingAdjust.height;
    if (maxHeight < 0)
      maxHeight = 0;
  } else {
    maxHeight = nscoord_MAX;
  }

  if (!IsAutoHeight(stylePos->mMinHeight, aCBSize.height)) {
    minHeight = nsLayoutUtils::
      ComputeHeightDependentValue(aCBSize.height, stylePos->mMinHeight) -
      boxSizingAdjust.height;
    if (minHeight < 0)
      minHeight = 0;
  } else {
    minHeight = 0;
  }

  

  NS_ASSERTION(aCBSize.width != NS_UNCONSTRAINEDSIZE,
               "Our containing block must not have unconstrained width!");

  PRBool hasIntrinsicWidth, hasIntrinsicHeight;
  nscoord intrinsicWidth, intrinsicHeight;

  if (aIntrinsicSize.width.GetUnit() == eStyleUnit_Coord ||
      aIntrinsicSize.width.GetUnit() == eStyleUnit_Percent) {
    hasIntrinsicWidth = PR_TRUE;
    intrinsicWidth = nsLayoutUtils::ComputeWidthValue(aRenderingContext,
                           aFrame, aCBSize.width, 0, boxSizingAdjust.width +
                           boxSizingToMarginEdgeWidth, aIntrinsicSize.width);
  } else {
    hasIntrinsicWidth = PR_FALSE;
    intrinsicWidth = 0;
  }

  if (aIntrinsicSize.height.GetUnit() == eStyleUnit_Coord ||
      (aIntrinsicSize.height.GetUnit() == eStyleUnit_Percent &&
       aCBSize.height != NS_AUTOHEIGHT)) {
    hasIntrinsicHeight = PR_TRUE;
    intrinsicHeight = nsLayoutUtils::
      ComputeHeightDependentValue(aCBSize.height, aIntrinsicSize.height);
    if (intrinsicHeight < 0)
      intrinsicHeight = 0;
  } else {
    hasIntrinsicHeight = PR_FALSE;
    intrinsicHeight = 0;
  }

  NS_ASSERTION(aIntrinsicRatio.width >= 0 && aIntrinsicRatio.height >= 0,
               "Intrinsic ratio has a negative component!");

  

  if (isAutoWidth) {
    if (isAutoHeight) {

      

      

      nscoord tentWidth, tentHeight;

      if (hasIntrinsicWidth) {
        tentWidth = intrinsicWidth;
      } else if (hasIntrinsicHeight && aIntrinsicRatio.height > 0) {
        tentWidth = MULDIV(intrinsicHeight, aIntrinsicRatio.width, aIntrinsicRatio.height);
      } else if (aIntrinsicRatio.width > 0) {
        tentWidth = aCBSize.width - boxSizingToMarginEdgeWidth; 
        if (tentWidth < 0) tentWidth = 0;
      } else {
        tentWidth = nsPresContext::CSSPixelsToAppUnits(300);
      }

      if (hasIntrinsicHeight) {
        tentHeight = intrinsicHeight;
      } else if (aIntrinsicRatio.width > 0) {
        tentHeight = MULDIV(tentWidth, aIntrinsicRatio.height, aIntrinsicRatio.width);
      } else {
        tentHeight = nsPresContext::CSSPixelsToAppUnits(150);
      }

      

      if (minWidth > maxWidth)
        maxWidth = minWidth;
      if (minHeight > maxHeight)
        maxHeight = minHeight;

      nscoord heightAtMaxWidth, heightAtMinWidth,
              widthAtMaxHeight, widthAtMinHeight;

      if (tentWidth > 0) {
        heightAtMaxWidth = MULDIV(maxWidth, tentHeight, tentWidth);
        if (heightAtMaxWidth < minHeight)
          heightAtMaxWidth = minHeight;
        heightAtMinWidth = MULDIV(minWidth, tentHeight, tentWidth);
        if (heightAtMinWidth > maxHeight)
          heightAtMinWidth = maxHeight;
      } else {
        heightAtMaxWidth = tentHeight;
        heightAtMinWidth = tentHeight;
      }

      if (tentHeight > 0) {
        widthAtMaxHeight = MULDIV(maxHeight, tentWidth, tentHeight);
        if (widthAtMaxHeight < minWidth)
          widthAtMaxHeight = minWidth;
        widthAtMinHeight = MULDIV(minHeight, tentWidth, tentHeight);
        if (widthAtMinHeight > maxWidth)
          widthAtMinHeight = maxWidth;
      } else {
        widthAtMaxHeight = tentWidth;
        widthAtMinHeight = tentWidth;
      }

      

      if (tentWidth > maxWidth) {
        if (tentHeight > maxHeight) {
          if (PRInt64(maxWidth) * PRInt64(tentHeight) <=
              PRInt64(maxHeight) * PRInt64(tentWidth)) {
            width = maxWidth;
            height = heightAtMaxWidth;
          } else {
            width = widthAtMaxHeight;
            height = maxHeight;
          }
        } else {
          
          
          
          width = maxWidth;
          height = heightAtMaxWidth;
        }
      } else if (tentWidth < minWidth) {
        if (tentHeight < minHeight) {
          if (PRInt64(minWidth) * PRInt64(tentHeight) <= 
              PRInt64(minHeight) * PRInt64(tentWidth)) {
            width = widthAtMinHeight;
            height = minHeight;
          } else {
            width = minWidth;
            height = heightAtMinWidth;
          }
        } else {
          
          
          
          width = minWidth;
          height = heightAtMinWidth;
        }
      } else {
        if (tentHeight > maxHeight) {
          width = widthAtMaxHeight;
          height = maxHeight;
        } else if (tentHeight < minHeight) {
          width = widthAtMinHeight;
          height = minHeight;
        } else {
          width = tentWidth;
          height = tentHeight;
        }
      }

    } else {

      
      height = NS_CSS_MINMAX(height, minHeight, maxHeight);
      if (aIntrinsicRatio.height > 0) {
        width = MULDIV(height, aIntrinsicRatio.width, aIntrinsicRatio.height);
      } else if (hasIntrinsicWidth) {
        width = intrinsicWidth;
      } else {
        width = nsPresContext::CSSPixelsToAppUnits(300);
      }
      width = NS_CSS_MINMAX(width, minWidth, maxWidth);

    }
  } else {
    if (isAutoHeight) {

      
      width = NS_CSS_MINMAX(width, minWidth, maxWidth);
      if (aIntrinsicRatio.width > 0) {
        height = MULDIV(width, aIntrinsicRatio.height, aIntrinsicRatio.width);
      } else if (hasIntrinsicHeight) {
        height = intrinsicHeight;
      } else {
        height = nsPresContext::CSSPixelsToAppUnits(150);
      }
      height = NS_CSS_MINMAX(height, minHeight, maxHeight);

    } else {

      
      width = NS_CSS_MINMAX(width, minWidth, maxWidth);
      height = NS_CSS_MINMAX(height, minHeight, maxHeight);

    }
  }

  return nsSize(width, height);
}

 nscoord
nsLayoutUtils::MinWidthFromInline(nsIFrame* aFrame,
                                  nsIRenderingContext* aRenderingContext)
{
  nsIFrame::InlineMinWidthData data;
  DISPLAY_MIN_WIDTH(aFrame, data.prevLines);
  aFrame->AddInlineMinWidth(aRenderingContext, &data);
  data.ForceBreak(aRenderingContext);
  return data.prevLines;
}

 nscoord
nsLayoutUtils::PrefWidthFromInline(nsIFrame* aFrame,
                                   nsIRenderingContext* aRenderingContext)
{
  nsIFrame::InlinePrefWidthData data;
  DISPLAY_PREF_WIDTH(aFrame, data.prevLines);
  aFrame->AddInlinePrefWidth(aRenderingContext, &data);
  data.ForceBreak(aRenderingContext);
  return data.prevLines;
}

void
nsLayoutUtils::DrawString(const nsIFrame*      aFrame,
                          nsIRenderingContext* aContext,
                          const PRUnichar*     aString,
                          PRInt32              aLength,
                          nsPoint              aPoint,
                          PRUint8              aDirection)
{
#ifdef IBMBIDI
  nsresult rv = NS_ERROR_FAILURE;
  nsPresContext* presContext = aFrame->PresContext();
  if (presContext->BidiEnabled()) {
    nsBidiPresUtils* bidiUtils = presContext->GetBidiUtils();

    if (bidiUtils) {
      if (aDirection == NS_STYLE_DIRECTION_INHERIT) {
        aDirection = aFrame->GetStyleVisibility()->mDirection;
      }
      nsBidiDirection direction =
        (NS_STYLE_DIRECTION_RTL == aDirection) ?
        NSBIDI_RTL : NSBIDI_LTR;
      rv = bidiUtils->RenderText(aString, aLength, direction,
                                 presContext, *aContext,
                                 aPoint.x, aPoint.y);
    }
  }
  if (NS_FAILED(rv))
#endif 
  { 
    aContext->SetTextRunRTL(PR_FALSE);
    aContext->DrawString(aString, aLength, aPoint.x, aPoint.y);
  }
}

nscoord
nsLayoutUtils::GetStringWidth(const nsIFrame*      aFrame,
                              nsIRenderingContext* aContext,
                              const PRUnichar*     aString,
                              PRInt32              aLength)
{
#ifdef IBMBIDI
  nsPresContext* presContext = aFrame->PresContext();
  if (presContext->BidiEnabled()) {
    nsBidiPresUtils* bidiUtils = presContext->GetBidiUtils();

    if (bidiUtils) {
      const nsStyleVisibility* vis = aFrame->GetStyleVisibility();
      nsBidiDirection direction =
        (NS_STYLE_DIRECTION_RTL == vis->mDirection) ?
        NSBIDI_RTL : NSBIDI_LTR;
      return bidiUtils->MeasureTextWidth(aString, aLength,
                                         direction, presContext, *aContext);
    }
  }
#endif 
  aContext->SetTextRunRTL(PR_FALSE);
  nscoord width;
  aContext->GetWidth(aString, aLength, width);
  return width;
}

 nscoord
nsLayoutUtils::GetCenteredFontBaseline(nsIFontMetrics* aFontMetrics,
                                       nscoord         aLineHeight)
{
  nscoord fontAscent, fontHeight;
  aFontMetrics->GetMaxAscent(fontAscent);
  aFontMetrics->GetHeight(fontHeight);

  nscoord leading = aLineHeight - fontHeight;
  return fontAscent + leading/2;
}


 PRBool
nsLayoutUtils::GetFirstLineBaseline(const nsIFrame* aFrame, nscoord* aResult)
{
  LinePosition position;
  if (!GetFirstLinePosition(aFrame, &position))
    return PR_FALSE;
  *aResult = position.mBaseline;
  return PR_TRUE;
}

 PRBool
nsLayoutUtils::GetFirstLinePosition(const nsIFrame* aFrame,
                                    LinePosition* aResult)
{
  const nsBlockFrame* block = nsLayoutUtils::GetAsBlock(const_cast<nsIFrame*>(aFrame));
  if (!block) {
    
    
    nsIAtom* fType = aFrame->GetType();
    if (fType == nsGkAtoms::tableOuterFrame) {
      aResult->mTop = 0;
      aResult->mBaseline = aFrame->GetBaseline();
      
      
      aResult->mBottom = aFrame->GetSize().height;
      return PR_TRUE;
    }

    
    if (fType == nsGkAtoms::scrollFrame) {
      nsIScrollableFrame *sFrame = do_QueryFrame(const_cast<nsIFrame*>(aFrame));
      if (!sFrame) {
        NS_NOTREACHED("not scroll frame");
      }
      LinePosition kidPosition;
      if (GetFirstLinePosition(sFrame->GetScrolledFrame(), &kidPosition)) {
        
        
        
        *aResult = kidPosition + aFrame->GetUsedBorderAndPadding().top;
        return PR_TRUE;
      }
      return PR_FALSE;
    }

    
    return PR_FALSE;
  }

  for (nsBlockFrame::const_line_iterator line = block->begin_lines(),
                                     line_end = block->end_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame *kid = line->mFirstChild;
      LinePosition kidPosition;
      if (GetFirstLinePosition(kid, &kidPosition)) {
        *aResult = kidPosition + kid->GetPosition().y;
        return PR_TRUE;
      }
    } else {
      
      
      if (line->GetHeight() != 0 || !line->IsEmpty()) {
        nscoord top = line->mBounds.y;
        aResult->mTop = top;
        aResult->mBaseline = top + line->GetAscent();
        aResult->mBottom = top + line->GetHeight();
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

 PRBool
nsLayoutUtils::GetLastLineBaseline(const nsIFrame* aFrame, nscoord* aResult)
{
  const nsBlockFrame* block = nsLayoutUtils::GetAsBlock(const_cast<nsIFrame*>(aFrame));
  if (!block)
    
    return PR_FALSE;

  for (nsBlockFrame::const_reverse_line_iterator line = block->rbegin_lines(),
                                             line_end = block->rend_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame *kid = line->mFirstChild;
      nscoord kidBaseline;
      if (GetLastLineBaseline(kid, &kidBaseline)) {
        *aResult = kidBaseline + kid->GetPosition().y;
        return PR_TRUE;
      } else if (kid->GetType() == nsGkAtoms::scrollFrame) {
        
        
        *aResult = kid->GetRect().YMost();
        return PR_TRUE;
      }
    } else {
      
      
      if (line->GetHeight() != 0 || !line->IsEmpty()) {
        *aResult = line->mBounds.y + line->GetAscent();
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

static nscoord
CalculateBlockContentBottom(nsBlockFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nscoord contentBottom = 0;

  for (nsBlockFrame::line_iterator line = aFrame->begin_lines(),
                                   line_end = aFrame->end_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame* child = line->mFirstChild;
      nscoord offset = child->GetRect().y - child->GetRelativeOffset().y;
      contentBottom = NS_MAX(contentBottom,
                        nsLayoutUtils::CalculateContentBottom(child) + offset);
    }
    else {
      contentBottom = NS_MAX(contentBottom, line->mBounds.YMost());
    }
  }
  return contentBottom;
}

 nscoord
nsLayoutUtils::CalculateContentBottom(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nscoord contentBottom = aFrame->GetRect().height;

  if (aFrame->GetOverflowRect().height > contentBottom) {
    nsBlockFrame* blockFrame = GetAsBlock(aFrame);
    nsIAtom* childList = nsnull;
    PRIntn nextListID = 0;
    do {
      if (childList == nsnull && blockFrame) {
        contentBottom = NS_MAX(contentBottom, CalculateBlockContentBottom(blockFrame));
      }
      else if (childList != nsGkAtoms::overflowList &&
               childList != nsGkAtoms::excessOverflowContainersList &&
               childList != nsGkAtoms::overflowOutOfFlowList)
      {
        for (nsIFrame* child = aFrame->GetFirstChild(childList);
            child; child = child->GetNextSibling())
        {
          nscoord offset = child->GetRect().y - child->GetRelativeOffset().y;
          contentBottom = NS_MAX(contentBottom,
                                 CalculateContentBottom(child) + offset);
        }
      }

      childList = aFrame->GetAdditionalChildListName(nextListID);
      nextListID++;
    } while (childList);
  }

  return contentBottom;
}

 nsIFrame*
nsLayoutUtils::GetClosestLayer(nsIFrame* aFrame)
{
  nsIFrame* layer;
  for (layer = aFrame; layer; layer = layer->GetParent()) {
    if (layer->GetStyleDisplay()->IsPositioned() ||
        (layer->GetParent() &&
          layer->GetParent()->GetType() == nsGkAtoms::scrollFrame))
      break;
  }
  if (layer)
    return layer;
  return aFrame->PresContext()->PresShell()->FrameManager()->GetRootFrame();
}

gfxPattern::GraphicsFilter
nsLayoutUtils::GetGraphicsFilterForFrame(nsIFrame* aForFrame)
{
#ifdef MOZ_SVG
  nsIFrame *frame = nsCSSRendering::IsCanvasFrame(aForFrame) ?
    nsCSSRendering::FindRootFrame(aForFrame) : aForFrame;

  switch (frame->GetStyleSVG()->mImageRendering) {
  case NS_STYLE_IMAGE_RENDERING_OPTIMIZESPEED:
    return gfxPattern::FILTER_FAST;
  case NS_STYLE_IMAGE_RENDERING_OPTIMIZEQUALITY:
    return gfxPattern::FILTER_BEST;
  case NS_STYLE_IMAGE_RENDERING_CRISPEDGES:
    return gfxPattern::FILTER_NEAREST;
  default:
    return gfxPattern::FILTER_GOOD;
  }
#else
  return gfxPattern::FILTER_GOOD;
#endif
}









static gfxPoint
MapToFloatImagePixels(const gfxSize& aSize,
                      const gfxRect& aDest, const gfxPoint& aPt)
{
  return gfxPoint(((aPt.x - aDest.pos.x)*aSize.width)/aDest.size.width,
                  ((aPt.y - aDest.pos.y)*aSize.height)/aDest.size.height);
}









static gfxPoint
MapToFloatUserPixels(const gfxSize& aSize,
                     const gfxRect& aDest, const gfxPoint& aPt)
{
  return gfxPoint(aPt.x*aDest.size.width/aSize.width + aDest.pos.x,
                  aPt.y*aDest.size.height/aSize.height + aDest.pos.y);
}

static nsresult
DrawImageInternal(nsIRenderingContext* aRenderingContext,
                  imgIContainer*       aImage,
                  gfxPattern::GraphicsFilter aGraphicsFilter,
                  const nsRect&        aDest,
                  const nsRect&        aFill,
                  const nsPoint&       aAnchor,
                  const nsRect&        aDirty,
                  const nsIntSize&     aImageSize,
                  PRUint32             aImageFlags)
{
  if (aDest.IsEmpty() || aFill.IsEmpty())
    return NS_OK;

  nsCOMPtr<nsIDeviceContext> dc;
  aRenderingContext->GetDeviceContext(*getter_AddRefs(dc));
  gfxFloat appUnitsPerDevPixel = dc->AppUnitsPerDevPixel();
  gfxContext *ctx = aRenderingContext->ThebesContext();

  gfxRect devPixelDest(aDest.x/appUnitsPerDevPixel,
                       aDest.y/appUnitsPerDevPixel,
                       aDest.width/appUnitsPerDevPixel,
                       aDest.height/appUnitsPerDevPixel);

  
  gfxRect devPixelFill(aFill.x/appUnitsPerDevPixel,
                       aFill.y/appUnitsPerDevPixel,
                       aFill.width/appUnitsPerDevPixel,
                       aFill.height/appUnitsPerDevPixel);
  PRBool ignoreScale = PR_FALSE;
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  ignoreScale = PR_TRUE;
#endif
  gfxRect fill = devPixelFill;
  PRBool didSnap = ctx->UserToDevicePixelSnapped(fill, ignoreScale);

  
  gfxRect dirty(aDirty.x/appUnitsPerDevPixel,
                aDirty.y/appUnitsPerDevPixel,
                aDirty.width/appUnitsPerDevPixel,
                aDirty.height/appUnitsPerDevPixel);

  gfxSize imageSize(aImageSize.width, aImageSize.height);

  
  gfxPoint subimageTopLeft =
    MapToFloatImagePixels(imageSize, devPixelDest, devPixelFill.TopLeft());
  gfxPoint subimageBottomRight =
    MapToFloatImagePixels(imageSize, devPixelDest, devPixelFill.BottomRight());
  nsIntRect intSubimage;
  intSubimage.MoveTo(NSToIntFloor(subimageTopLeft.x),
                     NSToIntFloor(subimageTopLeft.y));
  intSubimage.SizeTo(NSToIntCeil(subimageBottomRight.x) - intSubimage.x,
                     NSToIntCeil(subimageBottomRight.y) - intSubimage.y);

  
  
  
  gfxPoint anchorPoint(aAnchor.x/appUnitsPerDevPixel,
                       aAnchor.y/appUnitsPerDevPixel);
  gfxPoint imageSpaceAnchorPoint =
    MapToFloatImagePixels(imageSize, devPixelDest, anchorPoint);
  gfxContextMatrixAutoSaveRestore saveMatrix(ctx);

  if (didSnap) {
    NS_ASSERTION(!saveMatrix.Matrix().HasNonAxisAlignedTransform(),
                 "How did we snap, then?");
    imageSpaceAnchorPoint.Round();
    anchorPoint = imageSpaceAnchorPoint;
    anchorPoint = MapToFloatUserPixels(imageSize, devPixelDest, anchorPoint);
    anchorPoint = saveMatrix.Matrix().Transform(anchorPoint);
    anchorPoint.Round();

    
    
    dirty = saveMatrix.Matrix().Transform(dirty);

    ctx->IdentityMatrix();
  }

  gfxFloat scaleX = imageSize.width*appUnitsPerDevPixel/aDest.width;
  gfxFloat scaleY = imageSize.height*appUnitsPerDevPixel/aDest.height;
  if (didSnap) {
    
    
    scaleX /= saveMatrix.Matrix().xx;
    scaleY /= saveMatrix.Matrix().yy;
  }
  gfxFloat translateX = imageSpaceAnchorPoint.x - anchorPoint.x*scaleX;
  gfxFloat translateY = imageSpaceAnchorPoint.y - anchorPoint.y*scaleY;
  gfxMatrix transform(scaleX, 0, 0, scaleY, translateX, translateY);

  gfxRect finalFillRect = fill;
  
  
  
  
  
  
  
  if (didSnap && !transform.HasNonIntegerTranslation()) {
    dirty.RoundOut();
    finalFillRect = fill.Intersect(dirty);
  }
  if (finalFillRect.IsEmpty())
    return NS_OK;

  aImage->Draw(ctx, aGraphicsFilter, transform, finalFillRect, intSubimage,
               aImageFlags);
  return NS_OK;
}

 nsresult
nsLayoutUtils::DrawSingleUnscaledImage(nsIRenderingContext* aRenderingContext,
                                       imgIContainer*       aImage,
                                       const nsPoint&       aDest,
                                       const nsRect&        aDirty,
                                       PRUint32             aImageFlags,
                                       const nsRect*        aSourceArea)
{
  nsIntSize imageSize;
  aImage->GetWidth(&imageSize.width);
  aImage->GetHeight(&imageSize.height);
  NS_ENSURE_TRUE(imageSize.width > 0 && imageSize.height > 0, NS_ERROR_FAILURE);

  nscoord appUnitsPerCSSPixel = nsIDeviceContext::AppUnitsPerCSSPixel();
  nsSize size(imageSize.width*appUnitsPerCSSPixel,
              imageSize.height*appUnitsPerCSSPixel);

  nsRect source;
  if (aSourceArea) {
    source = *aSourceArea;
  } else {
    source.SizeTo(size);
  }

  nsRect dest(aDest - source.TopLeft(), size);
  nsRect fill(aDest, source.Size());
  
  
  
  fill.IntersectRect(fill, dest);
  return DrawImageInternal(aRenderingContext, aImage, gfxPattern::FILTER_NEAREST,
                           dest, fill, aDest, aDirty, imageSize, aImageFlags);
}
 
 nsresult
nsLayoutUtils::DrawSingleImage(nsIRenderingContext* aRenderingContext,
                               imgIContainer*       aImage,
                               gfxPattern::GraphicsFilter aGraphicsFilter,
                               const nsRect&        aDest,
                               const nsRect&        aDirty,
                               PRUint32             aImageFlags,
                               const nsRect*        aSourceArea)
{
  nsIntSize imageSize;
  aImage->GetWidth(&imageSize.width);
  aImage->GetHeight(&imageSize.height);
  NS_ENSURE_TRUE(imageSize.width > 0 && imageSize.height > 0, NS_ERROR_FAILURE);

  nsRect source;
  if (aSourceArea) {
    source = *aSourceArea;
  } else {
    nscoord appUnitsPerCSSPixel = nsIDeviceContext::AppUnitsPerCSSPixel();
    source.SizeTo(imageSize.width*appUnitsPerCSSPixel,
                  imageSize.height*appUnitsPerCSSPixel);
  }

  nsRect dest = nsLayoutUtils::GetWholeImageDestination(imageSize, source,
                                                        aDest);
  
  
  
  nsRect fill;
  fill.IntersectRect(aDest, dest);
  return DrawImageInternal(aRenderingContext, aImage, aGraphicsFilter, dest, fill,
                           fill.TopLeft(), aDirty, imageSize, aImageFlags);
}

 nsresult
nsLayoutUtils::DrawImage(nsIRenderingContext* aRenderingContext,
                         imgIContainer*       aImage,
                         gfxPattern::GraphicsFilter aGraphicsFilter,
                         const nsRect&        aDest,
                         const nsRect&        aFill,
                         const nsPoint&       aAnchor,
                         const nsRect&        aDirty,
                         PRUint32             aImageFlags)
{
  nsIntSize imageSize;
  aImage->GetWidth(&imageSize.width);
  aImage->GetHeight(&imageSize.height);
  NS_ENSURE_TRUE(imageSize.width > 0 && imageSize.height > 0, NS_ERROR_FAILURE);

  return DrawImageInternal(aRenderingContext, aImage, aGraphicsFilter,
                           aDest, aFill, aAnchor, aDirty,
                           imageSize, aImageFlags);
}

 nsRect
nsLayoutUtils::GetWholeImageDestination(const nsIntSize& aWholeImageSize,
                                        const nsRect& aImageSourceArea,
                                        const nsRect& aDestArea)
{
  double scaleX = double(aDestArea.width)/aImageSourceArea.width;
  double scaleY = double(aDestArea.height)/aImageSourceArea.height;
  nscoord destOffsetX = NSToCoordRound(aImageSourceArea.x*scaleX);
  nscoord destOffsetY = NSToCoordRound(aImageSourceArea.y*scaleY);
  nscoord appUnitsPerCSSPixel = nsIDeviceContext::AppUnitsPerCSSPixel();
  nscoord wholeSizeX = NSToCoordRound(aWholeImageSize.width*appUnitsPerCSSPixel*scaleX);
  nscoord wholeSizeY = NSToCoordRound(aWholeImageSize.height*appUnitsPerCSSPixel*scaleY);
  return nsRect(aDestArea.TopLeft() - nsPoint(destOffsetX, destOffsetY),
                nsSize(wholeSizeX, wholeSizeY));
}

void
nsLayoutUtils::SetFontFromStyle(nsIRenderingContext* aRC, nsStyleContext* aSC) 
{
  const nsStyleFont* font = aSC->GetStyleFont();
  const nsStyleVisibility* visibility = aSC->GetStyleVisibility();

  aRC->SetFont(font->mFont, visibility->mLangGroup,
               aSC->PresContext()->GetUserFontSet());
}

static PRBool NonZeroStyleCoord(const nsStyleCoord& aCoord)
{
  switch (aCoord.GetUnit()) {
  case eStyleUnit_Percent:
    return aCoord.GetPercentValue() > 0;
  case eStyleUnit_Coord:
    return aCoord.GetCoordValue() > 0;
  default:
    return PR_TRUE;
  }
}

 PRBool
nsLayoutUtils::HasNonZeroCorner(const nsStyleCorners& aCorners)
{
  NS_FOR_CSS_HALF_CORNERS(corner) {
    if (NonZeroStyleCoord(aCorners.Get(corner)))
      return PR_TRUE;
  }
  return PR_FALSE;
}


static PRBool IsCornerAdjacentToSide(PRUint8 aCorner, PRUint8 aSide)
{
  PR_STATIC_ASSERT(NS_SIDE_TOP == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT(NS_SIDE_RIGHT == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT(NS_SIDE_BOTTOM == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT(NS_SIDE_LEFT == NS_CORNER_BOTTOM_LEFT);
  PR_STATIC_ASSERT(NS_SIDE_TOP == ((NS_CORNER_TOP_RIGHT - 1)&3));
  PR_STATIC_ASSERT(NS_SIDE_RIGHT == ((NS_CORNER_BOTTOM_RIGHT - 1)&3));
  PR_STATIC_ASSERT(NS_SIDE_BOTTOM == ((NS_CORNER_BOTTOM_LEFT - 1)&3));
  PR_STATIC_ASSERT(NS_SIDE_LEFT == ((NS_CORNER_TOP_LEFT - 1)&3));

  return aSide == aCorner || aSide == ((aCorner - 1)&3);
}

 PRBool
nsLayoutUtils::HasNonZeroCornerOnSide(const nsStyleCorners& aCorners,
                                      PRUint8 aSide)
{
  PR_STATIC_ASSERT(NS_CORNER_TOP_LEFT_X/2 == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_LEFT_Y/2 == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_RIGHT_X/2 == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_RIGHT_Y/2 == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_RIGHT_X/2 == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_RIGHT_Y/2 == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_LEFT_X/2 == NS_CORNER_BOTTOM_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_LEFT_Y/2 == NS_CORNER_BOTTOM_LEFT);

  NS_FOR_CSS_HALF_CORNERS(corner) {
    
    
    if (NonZeroStyleCoord(aCorners.Get(corner)) &&
        IsCornerAdjacentToSide(corner/2, aSide))
      return PR_TRUE;
  }
  return PR_FALSE;
}

 nsTransparencyMode
nsLayoutUtils::GetFrameTransparency(nsIFrame* aBackgroundFrame,
                                    nsIFrame* aCSSRootFrame) {
  if (aCSSRootFrame->GetStyleContext()->GetStyleDisplay()->mOpacity < 1.0f)
    return eTransparencyTransparent;

  if (HasNonZeroCorner(aCSSRootFrame->GetStyleContext()->GetStyleBorder()->mBorderRadius))
    return eTransparencyTransparent;

  nsTransparencyMode transparency;
  if (aCSSRootFrame->IsThemed(&transparency))
    return transparency;

  if (aCSSRootFrame->GetStyleDisplay()->mAppearance == NS_THEME_WIN_GLASS)
    return eTransparencyGlass;

  
  
  
  if (aBackgroundFrame->GetType() == nsGkAtoms::viewportFrame &&
      !aBackgroundFrame->GetFirstChild(nsnull)) {
    return eTransparencyOpaque;
  }

  const nsStyleBackground* bg;
  if (!nsCSSRendering::FindBackground(aBackgroundFrame->PresContext(),
                                      aBackgroundFrame, &bg)) {
    return eTransparencyTransparent;
  }
  if (NS_GET_A(bg->mBackgroundColor) < 255 ||
      
      bg->BottomLayer().mClip != NS_STYLE_BG_CLIP_BORDER)
    return eTransparencyTransparent;
  return eTransparencyOpaque;
}

static PRBool
IsNonzeroCoord(const nsStyleCoord& aCoord)
{
  if (eStyleUnit_Coord == aCoord.GetUnit())
    return aCoord.GetCoordValue() != 0;
  return PR_FALSE;
}

 PRUint32
nsLayoutUtils::GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                       const nsStyleText* aStyleText,
                                       const nsStyleFont* aStyleFont)
{
  PRUint32 result = 0;
  if (IsNonzeroCoord(aStyleText->mLetterSpacing)) {
    result |= gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES;
  }
#ifdef MOZ_SVG
  switch (aStyleContext->GetStyleSVG()->mTextRendering) {
  case NS_STYLE_TEXT_RENDERING_OPTIMIZESPEED:
    result |= gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    break;
  case NS_STYLE_TEXT_RENDERING_AUTO:
    if (aStyleFont->mFont.size <
        aStyleContext->PresContext()->GetAutoQualityMinFontSize()) {
      result |= gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    }
    break;
  default:
    break;
  }
#endif
  return result;
}

 void
nsLayoutUtils::GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                       nsRect* aHStrip, nsRect* aVStrip) {
  NS_ASSERTION(aR1.TopLeft() == aR2.TopLeft(),
               "expected rects at the same position");
  nsRect unionRect(aR1.x, aR1.y, NS_MAX(aR1.width, aR2.width),
                   NS_MAX(aR1.height, aR2.height));
  nscoord VStripStart = NS_MIN(aR1.width, aR2.width);
  nscoord HStripStart = NS_MIN(aR1.height, aR2.height);
  *aVStrip = unionRect;
  aVStrip->x += VStripStart;
  aVStrip->width -= VStripStart;
  *aHStrip = unionRect;
  aHStrip->y += HStripStart;
  aHStrip->height -= HStripStart;
}

nsIDeviceContext*
nsLayoutUtils::GetDeviceContextForScreenInfo(nsIDocShell* aDocShell)
{
  nsCOMPtr<nsIDocShell> docShell = aDocShell;
  while (docShell) {
    
    
    
    
    nsCOMPtr<nsPIDOMWindow> win = do_GetInterface(docShell);
    if (!win) {
      
      return nsnull;
    }

    win->EnsureSizeUpToDate();

    nsRefPtr<nsPresContext> presContext;
    docShell->GetPresContext(getter_AddRefs(presContext));
    if (presContext) {
      nsIDeviceContext* context = presContext->DeviceContext();
      if (context) {
        return context;
      }
    }

    nsCOMPtr<nsIDocShellTreeItem> curItem = do_QueryInterface(docShell);
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    curItem->GetParent(getter_AddRefs(parentItem));
    docShell = do_QueryInterface(parentItem);
  }

  return nsnull;
}

 PRBool
nsLayoutUtils::IsReallyFixedPos(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame->GetParent(),
                  "IsReallyFixedPos called on frame not in tree");
  NS_PRECONDITION(aFrame->GetStyleDisplay()->mPosition ==
                    NS_STYLE_POSITION_FIXED,
                  "IsReallyFixedPos called on non-'position:fixed' frame");

  nsIAtom *parentType = aFrame->GetParent()->GetType();
  return parentType == nsGkAtoms::viewportFrame ||
         parentType == nsGkAtoms::pageContentFrame;
}

static void DeleteTextFragment(void* aObject, nsIAtom* aPropertyName,
                               void* aPropertyValue, void* aData)
{
  delete static_cast<nsTextFragment*>(aPropertyValue);
}

 nsTextFragment*
nsLayoutUtils::GetTextFragmentForPrinting(const nsIFrame* aFrame)
{
  nsPresContext* presContext = aFrame->PresContext();
  NS_PRECONDITION(!presContext->IsDynamic(),
                  "Shouldn't call this with dynamic PresContext");
#ifdef MOZ_SVG
  NS_PRECONDITION(aFrame->GetType() == nsGkAtoms::textFrame ||
                  aFrame->GetType() == nsGkAtoms::svgGlyphFrame,
                  "Wrong frame type!");
#else
  NS_PRECONDITION(aFrame->GetType() == nsGkAtoms::textFrame,
                  "Wrong frame type!");
#endif 

  nsIContent* content = aFrame->GetContent();
  nsTextFragment* frag =
    static_cast<nsTextFragment*>(presContext->PropertyTable()->
      GetProperty(content, nsGkAtoms::clonedTextForPrint));

  if (!frag) {
    frag = new nsTextFragment();
    NS_ENSURE_TRUE(frag, nsnull);
    *frag = *content->GetText();
    nsresult rv = presContext->PropertyTable()->
                    SetProperty(content, nsGkAtoms::clonedTextForPrint, frag,
                                DeleteTextFragment, nsnull);
    if (NS_FAILED(rv)) {
      delete frag;
      return nsnull;
    }
  }

  return frag;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(nsIDOMElement *aElement,
                                  PRUint32 aSurfaceFlags)
{
  SurfaceFromElementResult result;
  nsresult rv;

  nsCOMPtr<nsINode> node = do_QueryInterface(aElement);

  PRBool forceCopy = (aSurfaceFlags & SFE_WANT_NEW_SURFACE) != 0;
  PRBool wantImageSurface = (aSurfaceFlags & SFE_WANT_IMAGE_SURFACE) != 0;

  
  nsCOMPtr<nsICanvasElement> canvas = do_QueryInterface(aElement);
  if (node && canvas) {
    PRUint32 w, h;
    rv = canvas->GetSize(&w, &h);
    if (NS_FAILED(rv))
      return result;

    nsRefPtr<gfxASurface> surf;

    if (!forceCopy && canvas->CountContexts() == 1) {
      nsICanvasRenderingContextInternal *srcCanvas = canvas->GetContextAtIndex(0);
      rv = srcCanvas->GetThebesSurface(getter_AddRefs(surf));

      if (NS_FAILED(rv))
        surf = nsnull;
    }

    if (surf && wantImageSurface && surf->GetType() != gfxASurface::SurfaceTypeImage)
      surf = nsnull;

    if (!surf) {
      if (wantImageSurface) {
        surf = new gfxImageSurface(gfxIntSize(w, h), gfxASurface::ImageFormatARGB32);
      } else {
        surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(w, h), gfxASurface::ImageFormatARGB32);
      }
                
      nsRefPtr<gfxContext> ctx = new gfxContext(surf);
      rv = canvas->RenderContexts(ctx, gfxPattern::FILTER_NEAREST);
      if (NS_FAILED(rv))
        return result;
    }

    nsCOMPtr<nsIPrincipal> principal = node->NodePrincipal();

    result.mSurface = surf;
    result.mSize = gfxIntSize(w, h);
    result.mPrincipal = node->NodePrincipal();
    result.mIsWriteOnly = canvas->IsWriteOnly();

    return result;
  }

#ifdef MOZ_MEDIA
  
  nsCOMPtr<nsIDOMHTMLVideoElement> ve = do_QueryInterface(aElement);
  if (node && ve) {
    nsHTMLVideoElement *video = static_cast<nsHTMLVideoElement*>(ve.get());

    
    nsCOMPtr<nsIPrincipal> principal = video->GetCurrentPrincipal();
    if (!principal)
      return result;

    PRUint32 w, h;
    rv = video->GetVideoWidth(&w);
    rv |= video->GetVideoHeight(&h);
    if (NS_FAILED(rv))
      return result;

    nsRefPtr<gfxASurface> surf;
    if (wantImageSurface) {
      surf = new gfxImageSurface(gfxIntSize(w, h), gfxASurface::ImageFormatARGB32);
    } else {
      surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(w, h), gfxASurface::ImageFormatARGB32);
    }

    nsRefPtr<gfxContext> ctx = new gfxContext(surf);

    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    video->Paint(ctx, gfxPattern::FILTER_NEAREST, gfxRect(0, 0, w, h));

    result.mSurface = surf;
    result.mSize = gfxIntSize(w, h);
    result.mPrincipal = principal;
    result.mIsWriteOnly = PR_FALSE;

    return result;
  }
#endif

  
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(aElement);

  if (!imageLoader)
    return result;

  nsCOMPtr<imgIRequest> imgRequest;
  rv = imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                               getter_AddRefs(imgRequest));
  if (NS_FAILED(rv) || !imgRequest)
    return result;

  PRUint32 status;
  imgRequest->GetImageStatus(&status);
  if ((status & imgIRequest::STATUS_LOAD_COMPLETE) == 0)
    return result;

  
  
  
  nsCOMPtr<nsIURI> uri;
  rv = imgRequest->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return result;

  PRBool isDataURI = PR_FALSE;
  rv = uri->SchemeIs("data", &isDataURI);
  if (NS_FAILED(rv))
    return result;
    
  
  
  nsCOMPtr<nsIPrincipal> principal;
  if (!isDataURI) {
    rv = imgRequest->GetImagePrincipal(getter_AddRefs(principal));
    if (NS_FAILED(rv) || !principal)
      return result;
  }

  nsCOMPtr<imgIContainer> imgContainer;
  rv = imgRequest->GetImage(getter_AddRefs(imgContainer));
  if (NS_FAILED(rv) || !imgContainer)
    return result;

  PRUint32 whichFrame = (aSurfaceFlags & SFE_WANT_FIRST_FRAME)
                        ? (PRUint32) imgIContainer::FRAME_FIRST
                        : (PRUint32) imgIContainer::FRAME_CURRENT;
  nsRefPtr<gfxASurface> framesurf;
  rv = imgContainer->GetFrame(whichFrame,
                              imgIContainer::FLAG_SYNC_DECODE,
                              getter_AddRefs(framesurf));
  if (NS_FAILED(rv))
    return result;

  PRInt32 imgWidth, imgHeight;
  rv = imgContainer->GetWidth(&imgWidth);
  rv |= imgContainer->GetHeight(&imgHeight);
  if (NS_FAILED(rv))
    return result;

  if (wantImageSurface && framesurf->GetType() != gfxASurface::SurfaceTypeImage) {
    forceCopy = PR_TRUE;
  }

  nsRefPtr<gfxASurface> gfxsurf = framesurf;
  if (forceCopy) {
    if (wantImageSurface) {
      gfxsurf = new gfxImageSurface (gfxIntSize(imgWidth, imgHeight), gfxASurface::ImageFormatARGB32);
    } else {
      gfxsurf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(imgWidth, imgHeight),
                                                                   gfxASurface::ImageFormatARGB32);
    }

    nsRefPtr<gfxContext> ctx = new gfxContext(gfxsurf);

    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->SetSource(framesurf);
    ctx->Paint();
  }

  result.mSurface = gfxsurf;
  result.mSize = gfxIntSize(imgWidth, imgHeight);
  result.mPrincipal = principal;
  result.mIsWriteOnly = PR_FALSE;

  return result;
}

nsSetAttrRunnable::nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                                     const nsAString& aValue)
  : mContent(aContent),
    mAttrName(aAttrName),
    mValue(aValue)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
}

NS_IMETHODIMP
nsSetAttrRunnable::Run()
{
  return mContent->SetAttr(kNameSpaceID_None, mAttrName, mValue, PR_TRUE);
}

nsUnsetAttrRunnable::nsUnsetAttrRunnable(nsIContent* aContent,
                                         nsIAtom* aAttrName)
  : mContent(aContent),
    mAttrName(aAttrName)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
}

NS_IMETHODIMP
nsUnsetAttrRunnable::Run()
{
  return mContent->UnsetAttr(kNameSpaceID_None, mAttrName, PR_TRUE);
}

nsReflowFrameRunnable::nsReflowFrameRunnable(nsIFrame* aFrame,
                          nsIPresShell::IntrinsicDirty aIntrinsicDirty,
                          nsFrameState aBitToAdd)
  : mWeakFrame(aFrame),
    mIntrinsicDirty(aIntrinsicDirty),
    mBitToAdd(aBitToAdd)
{
}

NS_IMETHODIMP
nsReflowFrameRunnable::Run()
{
  if (mWeakFrame.IsAlive()) {
    mWeakFrame->PresContext()->PresShell()->
      FrameNeedsReflow(mWeakFrame, mIntrinsicDirty, mBitToAdd);
  }
  return NS_OK;
}
