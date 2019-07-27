






#include "nsCanvasFrame.h"
#include "nsContainerFrame.h"
#include "nsCSSRendering.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsPresShell.h"
#include "nsIPresShell.h"
#include "nsDisplayList.h"
#include "nsCSSFrameConstructor.h"
#include "nsFrameManager.h"
#include "gfxPlatform.h"
#include "nsPrintfCString.h"

#include "nsContentList.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsStyleSet.h"

#include "nsIScrollableFrame.h"
#ifdef DEBUG_CANVAS_FOCUS
#include "nsIDocShell.h"
#endif



using namespace mozilla;
using namespace mozilla::layout;
using namespace mozilla::gfx;

nsCanvasFrame*
NS_NewCanvasFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsCanvasFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsCanvasFrame)

NS_QUERYFRAME_HEAD(nsCanvasFrame)
  NS_QUERYFRAME_ENTRY(nsCanvasFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

nsresult
nsCanvasFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  if (!mContent) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocument> doc = mContent->OwnerDoc();
  nsresult rv = NS_OK;
  ErrorResult er;
  
  if (PresShell::TouchCaretPrefEnabled()) {
    nsRefPtr<dom::NodeInfo> nodeInfo;

    
    nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::div, nullptr,
                                                   kNameSpaceID_XHTML,
                                                   nsIDOMNode::ELEMENT_NODE);
    NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

    rv = NS_NewHTMLElement(getter_AddRefs(mTouchCaretElement), nodeInfo.forget(),
                           mozilla::dom::NOT_FROM_PARSER);
    NS_ENSURE_SUCCESS(rv, rv);
    aElements.AppendElement(mTouchCaretElement);

    
    mTouchCaretElement->SetAttribute(NS_LITERAL_STRING("_moz_anonclass"),
                                     NS_LITERAL_STRING("mozTouchCaret"), er);
    NS_ENSURE_SUCCESS(er.ErrorCode(), er.ErrorCode());

    
    nsAutoString classValue;
    classValue.AppendLiteral("moz-touchcaret hidden");
    rv = mTouchCaretElement->SetAttr(kNameSpaceID_None, nsGkAtoms::_class,
                                     classValue, true);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (PresShell::SelectionCaretPrefEnabled()) {
    
    mSelectionCaretsStartElement = doc->CreateHTMLElement(nsGkAtoms::div);
    aElements.AppendElement(mSelectionCaretsStartElement);

    mSelectionCaretsEndElement = doc->CreateHTMLElement(nsGkAtoms::div);
    aElements.AppendElement(mSelectionCaretsEndElement);

    mSelectionCaretsStartElement->SetAttribute(NS_LITERAL_STRING("_moz_anonclass"),
                                               NS_LITERAL_STRING("mozTouchCaret"), er);
    rv = mSelectionCaretsStartElement->SetAttr(kNameSpaceID_None, nsGkAtoms::_class,
                                               NS_LITERAL_STRING("moz-selectioncaret-left hidden"),
                                               true);
    NS_ENSURE_SUCCESS(rv, rv);

    mSelectionCaretsEndElement->SetAttribute(NS_LITERAL_STRING("_moz_anonclass"),
                                             NS_LITERAL_STRING("mozTouchCaret"), er);
    rv = mSelectionCaretsEndElement->SetAttr(kNameSpaceID_None, nsGkAtoms::_class,
                                             NS_LITERAL_STRING("moz-selectioncaret-right hidden"),
                                             true);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
nsCanvasFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements, uint32_t aFilter)
{
  if (mTouchCaretElement) {
    aElements.AppendElement(mTouchCaretElement);
  }

  if (mSelectionCaretsStartElement) {
    aElements.AppendElement(mSelectionCaretsStartElement);
  }

  if (mSelectionCaretsEndElement) {
    aElements.AppendElement(mSelectionCaretsEndElement);
  }
}

void
nsCanvasFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsIScrollableFrame* sf =
    PresContext()->GetPresShell()->GetRootScrollFrameAsScrollable();
  if (sf) {
    sf->RemoveScrollPositionListener(this);
  }

  nsContentUtils::DestroyAnonymousContent(&mTouchCaretElement);
  nsContentUtils::DestroyAnonymousContent(&mSelectionCaretsStartElement);
  nsContentUtils::DestroyAnonymousContent(&mSelectionCaretsEndElement);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

void
nsCanvasFrame::ScrollPositionWillChange(nscoord aX, nscoord aY)
{
  if (mDoPaintFocus) {
    mDoPaintFocus = false;
    PresContext()->FrameManager()->GetRootFrame()->InvalidateFrameSubtree();
  }
}

NS_IMETHODIMP
nsCanvasFrame::SetHasFocus(bool aHasFocus)
{
  if (mDoPaintFocus != aHasFocus) {
    mDoPaintFocus = aHasFocus;
    PresContext()->FrameManager()->GetRootFrame()->InvalidateFrameSubtree();

    if (!mAddedScrollPositionListener) {
      nsIScrollableFrame* sf =
        PresContext()->GetPresShell()->GetRootScrollFrameAsScrollable();
      if (sf) {
        sf->AddScrollPositionListener(this);
        mAddedScrollPositionListener = true;
      }
    }
  }
  return NS_OK;
}

#ifdef DEBUG
void
nsCanvasFrame::SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList)
{
  NS_ASSERTION(aListID != kPrincipalList ||
               aChildList.IsEmpty() || aChildList.OnlyChild(),
               "Primary child list can have at most one frame in it");
  nsContainerFrame::SetInitialChildList(aListID, aChildList);
}

void
nsCanvasFrame::AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList)
{
  MOZ_ASSERT(aListID == kPrincipalList, "unexpected child list");
  if (!mFrames.IsEmpty()) {
    for (nsFrameList::Enumerator e(aFrameList); !e.AtEnd(); e.Next()) {
      
      
      MOZ_ASSERT(e.get()->GetContent()->IsInNativeAnonymousSubtree(),
                 "invalid child list");
    }
  }
  nsFrame::VerifyDirtyBitSet(aFrameList);
  nsContainerFrame::AppendFrames(aListID, aFrameList);
}

void
nsCanvasFrame::InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  
  
  MOZ_ASSERT(!aPrevFrame, "unexpected previous sibling frame");
  AppendFrames(aListID, aFrameList);
}

void
nsCanvasFrame::RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame)
{
  MOZ_ASSERT(aListID == kPrincipalList, "unexpected child list");
  nsContainerFrame::RemoveFrame(aListID, aOldFrame);
}
#endif

nsRect nsCanvasFrame::CanvasArea() const
{
  
  
  nsRect result(GetVisualOverflowRect());

  nsIScrollableFrame *scrollableFrame = do_QueryFrame(GetParent());
  if (scrollableFrame) {
    nsRect portRect = scrollableFrame->GetScrollPortRect();
    result.UnionRect(result, nsRect(nsPoint(0, 0), portRect.Size()));
  }
  return result;
}

void
nsDisplayCanvasBackgroundColor::Paint(nsDisplayListBuilder* aBuilder,
                                      nsRenderingContext* aCtx)
{
  nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
  nsPoint offset = ToReferenceFrame();
  nsRect bgClipRect = frame->CanvasArea() + offset;
  if (NS_GET_A(mColor) > 0) {
    aCtx->SetColor(mColor);
    aCtx->FillRect(bgClipRect);
  }
}

#ifdef MOZ_DUMP_PAINTING
void
nsDisplayCanvasBackgroundColor::WriteDebugInfo(nsACString& aTo)
{
  aTo += nsPrintfCString(" (rgba %d,%d,%d,%d)",
          NS_GET_R(mColor), NS_GET_G(mColor),
          NS_GET_B(mColor), NS_GET_A(mColor));
}
#endif

static void BlitSurface(DrawTarget* aDest, const gfxRect& aRect, DrawTarget* aSource)
{
  RefPtr<SourceSurface> source = aSource->Snapshot();
  aDest->DrawSurface(source,
                     Rect(aRect.x, aRect.y, aRect.width, aRect.height),
                     Rect(0, 0, aRect.width, aRect.height));
}

void
nsDisplayCanvasBackgroundImage::Paint(nsDisplayListBuilder* aBuilder,
                                      nsRenderingContext* aCtx)
{
  nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
  nsPoint offset = ToReferenceFrame();
  nsRect bgClipRect = frame->CanvasArea() + offset;

  nsRefPtr<nsRenderingContext> context;
  nsRefPtr<gfxContext> dest = aCtx->ThebesContext();
  RefPtr<DrawTarget> dt;
  gfxRect destRect;
#ifndef MOZ_GFX_OPTIMIZE_MOBILE
  if (IsSingleFixedPositionImage(aBuilder, bgClipRect, &destRect) &&
      aBuilder->IsPaintingToWindow() && !aBuilder->IsCompositingCheap() &&
      !dest->CurrentMatrix().HasNonIntegerTranslation()) {
    
    
    destRect.Round();
    dt = static_cast<DrawTarget*>(Frame()->Properties().Get(nsIFrame::CachedBackgroundImageDT()));
    DrawTarget* destDT = dest->GetDrawTarget();
    if (dt) {
      BlitSurface(destDT, destRect, dt);
      return;
    }
    dt = destDT->CreateSimilarDrawTarget(IntSize(ceil(destRect.width), ceil(destRect.height)), SurfaceFormat::B8G8R8A8);
    if (dt) {
      nsRefPtr<gfxContext> ctx = new gfxContext(dt);
      ctx->SetMatrix(
        ctx->CurrentMatrix().Translate(-destRect.x, -destRect.y));
      context = new nsRenderingContext();
      context->Init(aCtx->DeviceContext(), ctx);
    }
  }
#endif

  PaintInternal(aBuilder,
                dt ? context.get() : aCtx,
                dt ? bgClipRect: mVisibleRect,
                &bgClipRect);

  if (dt) {
    BlitSurface(dest->GetDrawTarget(), destRect, dt);
    frame->Properties().Set(nsIFrame::CachedBackgroundImageDT(), dt.forget().drop());
  }
}

void
nsDisplayCanvasThemedBackground::Paint(nsDisplayListBuilder* aBuilder,
                                       nsRenderingContext* aCtx)
{
  nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
  nsPoint offset = ToReferenceFrame();
  nsRect bgClipRect = frame->CanvasArea() + offset;

  PaintInternal(aBuilder, aCtx, mVisibleRect, &bgClipRect);
}






class nsDisplayCanvasFocus : public nsDisplayItem {
public:
  nsDisplayCanvasFocus(nsDisplayListBuilder* aBuilder, nsCanvasFrame *aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayCanvasFocus);
  }
  virtual ~nsDisplayCanvasFocus() {
    MOZ_COUNT_DTOR(nsDisplayCanvasFocus);
  }

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = false;
    
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    return frame->CanvasArea() + ToReferenceFrame();
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE
  {
    nsCanvasFrame* frame = static_cast<nsCanvasFrame*>(mFrame);
    frame->PaintFocus(*aCtx, ToReferenceFrame());
  }

  NS_DISPLAY_DECL_NAME("CanvasFocus", TYPE_CANVAS_FOCUS)
};

void
nsCanvasFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  if (GetPrevInFlow()) {
    DisplayOverflowContainers(aBuilder, aDirtyRect, aLists);
  }

  
  
  
  
  
  
  
  if (IsVisibleForPainting(aBuilder)) {
    nsStyleContext* bgSC;
    const nsStyleBackground* bg = nullptr;
    bool isThemed = IsThemed();
    if (!isThemed && nsCSSRendering::FindBackground(this, &bgSC)) {
      bg = bgSC->StyleBackground();
    }
    aLists.BorderBackground()->AppendNewToTop(
        new (aBuilder) nsDisplayCanvasBackgroundColor(aBuilder, this));
  
    if (isThemed) {
      aLists.BorderBackground()->AppendNewToTop(
        new (aBuilder) nsDisplayCanvasThemedBackground(aBuilder, this));
      return;
    }

    if (!bg) {
      return;
    }

    bool needBlendContainer = false;

    
    NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, bg) {
      if (bg->mLayers[i].mImage.IsEmpty()) {
        continue;
      }
      if (bg->mLayers[i].mBlendMode != NS_STYLE_BLEND_NORMAL) {
        needBlendContainer = true;
      }
      aLists.BorderBackground()->AppendNewToTop(
        new (aBuilder) nsDisplayCanvasBackgroundImage(aBuilder, this, i, bg));
    }

    if (needBlendContainer) {
      aLists.BorderBackground()->AppendNewToTop(
        new (aBuilder) nsDisplayBlendContainer(aBuilder, this, aLists.BorderBackground()));
    }
  }

  nsIFrame* kid;
  for (kid = GetFirstPrincipalChild(); kid; kid = kid->GetNextSibling()) {
    
    BuildDisplayListForChild(aBuilder, kid, aDirtyRect, aLists);
  }

#ifdef DEBUG_CANVAS_FOCUS
  nsCOMPtr<nsIContent> focusContent;
  aPresContext->EventStateManager()->
    GetFocusedContent(getter_AddRefs(focusContent));

  bool hasFocus = false;
  nsCOMPtr<nsISupports> container;
  aPresContext->GetContainer(getter_AddRefs(container));
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(container));
  if (docShell) {
    docShell->GetHasFocus(&hasFocus);
    printf("%p - nsCanvasFrame::Paint R:%d,%d,%d,%d  DR: %d,%d,%d,%d\n", this, 
            mRect.x, mRect.y, mRect.width, mRect.height,
            aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  }
  printf("%p - Focus: %s   c: %p  DoPaint:%s\n", docShell.get(), hasFocus?"Y":"N", 
         focusContent.get(), mDoPaintFocus?"Y":"N");
#endif

  if (!mDoPaintFocus)
    return;
  
  if (!StyleVisibility()->IsVisible())
    return;
  
  aLists.Outlines()->AppendNewToTop(new (aBuilder)
    nsDisplayCanvasFocus(aBuilder, this));
}

void
nsCanvasFrame::PaintFocus(nsRenderingContext& aRenderingContext, nsPoint aPt)
{
  nsRect focusRect(aPt, GetSize());

  nsIScrollableFrame *scrollableFrame = do_QueryFrame(GetParent());
  if (scrollableFrame) {
    nsRect portRect = scrollableFrame->GetScrollPortRect();
    focusRect.width = portRect.width;
    focusRect.height = portRect.height;
    focusRect.MoveBy(scrollableFrame->GetScrollPosition());
  }

 
 
  nsIFrame* root = mFrames.FirstChild();
  const nsStyleColor* color = root ? root->StyleColor() : StyleColor();
  if (!color) {
    NS_ERROR("current color cannot be found");
    return;
  }

  nsCSSRendering::PaintFocus(PresContext(), aRenderingContext,
                             focusRect, color->mColor);
}

 nscoord
nsCanvasFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetMinISize(aRenderingContext);
  return result;
}

 nscoord
nsCanvasFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);
  if (mFrames.IsEmpty())
    result = 0;
  else
    result = mFrames.FirstChild()->GetPrefISize(aRenderingContext);
  return result;
}

void
nsCanvasFrame::Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsCanvasFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE_REFLOW_IN("nsCanvasFrame::Reflow");

  
  aStatus = NS_FRAME_COMPLETE;

  nsCanvasFrame* prevCanvasFrame = static_cast<nsCanvasFrame*>
                                               (GetPrevInFlow());
  if (prevCanvasFrame) {
    AutoFrameListPtr overflow(aPresContext,
                              prevCanvasFrame->StealOverflowFrames());
    if (overflow) {
      NS_ASSERTION(overflow->OnlyChild(),
                   "must have doc root as canvas frame's only child");
      nsContainerFrame::ReparentFrameViewList(*overflow, prevCanvasFrame, this);
      
      
      
      mFrames.InsertFrames(this, nullptr, *overflow);
    }
  }

  
  
  
  SetSize(nsSize(aReflowState.ComputedWidth(), aReflowState.ComputedHeight())); 

  
  
  
  
  
  
  
  nsHTMLReflowMetrics kidDesiredSize(aReflowState);
  if (mFrames.IsEmpty()) {
    
    aDesiredSize.Width() = aDesiredSize.Height() = 0;
  } else {
    nsIFrame* kidFrame = mFrames.FirstChild();
    bool kidDirty = (kidFrame->GetStateBits() & NS_FRAME_IS_DIRTY) != 0;

    nsHTMLReflowState
      kidReflowState(aPresContext, aReflowState, kidFrame,
                     aReflowState.AvailableSize(kidFrame->GetWritingMode()));

    if (aReflowState.mFlags.mVResize &&
        (kidFrame->GetStateBits() & NS_FRAME_CONTAINS_RELATIVE_HEIGHT)) {
      
      
      kidReflowState.mFlags.mVResize = true;
    }

    nsPoint kidPt(kidReflowState.ComputedPhysicalMargin().left,
                  kidReflowState.ComputedPhysicalMargin().top);

    kidReflowState.ApplyRelativePositioning(&kidPt);

    
    ReflowChild(kidFrame, aPresContext, kidDesiredSize, kidReflowState,
                kidPt.x, kidPt.y, 0, aStatus);

    
    FinishReflowChild(kidFrame, aPresContext, kidDesiredSize, &kidReflowState,
                      kidPt.x, kidPt.y, 0);

    if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
      nsIFrame* nextFrame = kidFrame->GetNextInFlow();
      NS_ASSERTION(nextFrame || aStatus & NS_FRAME_REFLOW_NEXTINFLOW,
        "If it's incomplete and has no nif yet, it must flag a nif reflow.");
      if (!nextFrame) {
        nextFrame = aPresContext->PresShell()->FrameConstructor()->
          CreateContinuingFrame(aPresContext, kidFrame, this);
        SetOverflowFrames(nsFrameList(nextFrame, nextFrame));
        
        
        
        
      }
      if (NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus)) {
        nextFrame->AddStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER);
      }
    }

    
    
    if (kidDirty) {
      
      
      
      
      
      
      
      
      nsIFrame* viewport = PresContext()->GetPresShell()->GetRootFrame();
      viewport->InvalidateFrame();
    }
    
    
    
    
    WritingMode wm = aReflowState.GetWritingMode();
    LogicalSize finalSize(wm);
    finalSize.ISize(wm) = aReflowState.ComputedISize();
    if (aReflowState.ComputedBSize() == NS_UNCONSTRAINEDSIZE) {
      finalSize.BSize(wm) = kidFrame->GetLogicalSize(wm).BSize(wm) +
        kidReflowState.ComputedLogicalMargin().BStartEnd(wm);
    } else {
      finalSize.BSize(wm) = aReflowState.ComputedBSize();
    }

    aDesiredSize.SetSize(wm, finalSize);
    aDesiredSize.SetOverflowAreasToDesiredBounds();
    aDesiredSize.mOverflowAreas.UnionWith(
      kidDesiredSize.mOverflowAreas + kidPt);
  }

  if (prevCanvasFrame) {
    ReflowOverflowContainerChildren(aPresContext, aReflowState,
                                    aDesiredSize.mOverflowAreas, 0,
                                    aStatus);
  }

  FinishReflowWithAbsoluteFrames(aPresContext, aDesiredSize, aReflowState, aStatus);

  NS_FRAME_TRACE_REFLOW_OUT("nsCanvasFrame::Reflow", aStatus);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsIAtom*
nsCanvasFrame::GetType() const
{
  return nsGkAtoms::canvasFrame;
}

nsresult 
nsCanvasFrame::GetContentForEvent(WidgetEvent* aEvent,
                                  nsIContent** aContent)
{
  NS_ENSURE_ARG_POINTER(aContent);
  nsresult rv = nsFrame::GetContentForEvent(aEvent,
                                            aContent);
  if (NS_FAILED(rv) || !*aContent) {
    nsIFrame* kid = mFrames.FirstChild();
    if (kid) {
      rv = kid->GetContentForEvent(aEvent,
                                   aContent);
    }
  }

  return rv;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsCanvasFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Canvas"), aResult);
}
#endif
