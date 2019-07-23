







































#include "nsHTMLContainerFrame.h"
#include "nsFirstLetterFrame.h"
#include "nsIRenderingContext.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIContent.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsILinkHandler.h"
#include "nsGUIEvent.h"
#include "nsIDocument.h"
#include "nsIURL.h"
#include "nsPlaceholderFrame.h"
#include "nsHTMLParts.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIDOMEvent.h"
#include "nsIScrollableView.h"
#include "nsWidgetsCID.h"
#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsIThebesFontMetrics.h"
#include "gfxFont.h"
#include "nsCSSFrameConstructor.h"
#include "nsDisplayList.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsDisplayList.h"
#include "nsCSSRendering.h"

class nsDisplayTextDecoration : public nsDisplayItem {
public:
  nsDisplayTextDecoration(nsHTMLContainerFrame* aFrame, PRUint8 aDecoration,
                          nscolor aColor, nsLineBox* aLine)
    : nsDisplayItem(aFrame), mLine(aLine), mColor(aColor),
      mDecoration(aDecoration) {
    MOZ_COUNT_CTOR(nsDisplayTextDecoration);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTextDecoration() {
    MOZ_COUNT_DTOR(nsDisplayTextDecoration);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  NS_DISPLAY_DECL_NAME("TextDecoration")
private:
  nsLineBox*            mLine;
  nscolor               mColor;
  PRUint8               mDecoration;
};

void
nsDisplayTextDecoration::Paint(nsDisplayListBuilder* aBuilder,
                               nsIRenderingContext* aCtx)
{
  nsCOMPtr<nsIFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(mFrame, getter_AddRefs(fm));
  nsIThebesFontMetrics* tfm = static_cast<nsIThebesFontMetrics*>(fm.get());
  gfxFontGroup* fontGroup = tfm->GetThebesFontGroup();
  gfxFont* firstFont = fontGroup->GetFontAt(0);
  if (!firstFont)
    return; 
  const gfxFont::Metrics& metrics = firstFont->GetMetrics();

  gfxFloat ascent;
  
  
  
  if (mFrame->GetType() == nsGkAtoms::letterFrame) {
    
    
    
    nsFirstLetterFrame* letterFrame = static_cast<nsFirstLetterFrame*>(mFrame);
    nscoord tmp = letterFrame->GetFirstLetterBaseline();
    tmp -= letterFrame->GetUsedBorderAndPadding().top;
    ascent = letterFrame->PresContext()->AppUnitsToGfxUnits(tmp);
  } else {
    ascent = metrics.maxAscent;
  }

  nsPoint pt = aBuilder->ToReferenceFrame(mFrame);

  nsHTMLContainerFrame* f = static_cast<nsHTMLContainerFrame*>(mFrame);
  if (mDecoration == NS_STYLE_TEXT_DECORATION_UNDERLINE) {
    gfxFloat underlineOffset = fontGroup->GetUnderlineOffset();
    f->PaintTextDecorationLine(aCtx->ThebesContext(), pt, mLine, mColor,
                               underlineOffset, ascent,
                               metrics.underlineSize, mDecoration);
  } else if (mDecoration == NS_STYLE_TEXT_DECORATION_OVERLINE) {
    f->PaintTextDecorationLine(aCtx->ThebesContext(), pt, mLine, mColor,
                               metrics.maxAscent, ascent,
                               metrics.underlineSize, mDecoration);
  } else {
    f->PaintTextDecorationLine(aCtx->ThebesContext(), pt, mLine, mColor,
                               metrics.strikeoutOffset, ascent,
                               metrics.strikeoutSize, mDecoration);
  }
}

nsRect
nsDisplayTextDecoration::GetBounds(nsDisplayListBuilder* aBuilder)
{
  return mFrame->GetOverflowRect() + aBuilder->ToReferenceFrame(mFrame);
}

class nsDisplayTextShadow : public nsDisplayItem {
public:
  nsDisplayTextShadow(nsHTMLContainerFrame* aFrame, const PRUint8 aDecoration,
                      const nscolor& aColor, nsLineBox* aLine,
                      const nscoord& aBlurRadius, const nsPoint& aOffset)
    : nsDisplayItem(aFrame), mLine(aLine), mColor(aColor),
      mDecorationFlags(aDecoration),
      mBlurRadius(aBlurRadius), mOffset(aOffset) {
    MOZ_COUNT_CTOR(nsDisplayTextShadow);
  }
  virtual ~nsDisplayTextShadow() {
    MOZ_COUNT_DTOR(nsDisplayTextShadow);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsIRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  NS_DISPLAY_DECL_NAME("TextShadow")
private:
  nsLineBox*    mLine;
  nscolor       mColor;
  PRUint8       mDecorationFlags;
  nscoord       mBlurRadius;
  nsPoint       mOffset;
};

void
nsDisplayTextShadow::Paint(nsDisplayListBuilder* aBuilder,
                           nsIRenderingContext* aCtx)
{
  mBlurRadius = NS_MAX(mBlurRadius, 0);

  nsCOMPtr<nsIFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(mFrame, getter_AddRefs(fm));
  nsIThebesFontMetrics* tfm = static_cast<nsIThebesFontMetrics*>(fm.get());
  gfxFontGroup* fontGroup = tfm->GetThebesFontGroup();
  gfxFont* firstFont = fontGroup->GetFontAt(0);
  if (!firstFont)
    return; 

  const gfxFont::Metrics& metrics = firstFont->GetMetrics();
  gfxFloat underlineOffset = fontGroup->GetUnderlineOffset();

  nsHTMLContainerFrame* f = static_cast<nsHTMLContainerFrame*>(mFrame);
  nsPoint pt = aBuilder->ToReferenceFrame(mFrame) + mOffset;
  nsPresContext* presContext = mFrame->PresContext();

  gfxFloat lineWidth;
  nsPoint linePt;
  gfxFloat ascent;
  if (mLine) {
    
    nscoord start = mLine->mBounds.x;
    nscoord width = mLine->mBounds.width;
    f->AdjustForTextIndent(mLine, start, width);
    if (width <= 0)
      return;

    lineWidth = presContext->AppUnitsToGfxUnits(width);
    linePt = nsPoint(start + pt.x, mLine->mBounds.y + pt.y);
    ascent = presContext->AppUnitsToGfxUnits(mLine->GetAscent());
  } else {
    
    nsRect contentRect = mFrame->GetContentRect() - mFrame->GetPosition() + pt;

    lineWidth = presContext->AppUnitsToGfxUnits(contentRect.width);
    linePt = contentRect.TopLeft();

    
    
    
    if (mFrame->GetType() == nsGkAtoms::letterFrame) {
      
      
      
      nsFirstLetterFrame* letterFrame = static_cast<nsFirstLetterFrame*>(mFrame);
      nscoord tmp = letterFrame->GetFirstLetterBaseline();
      tmp -= letterFrame->GetUsedBorderAndPadding().top;
      ascent = presContext->AppUnitsToGfxUnits(tmp);
    } else {
      ascent = metrics.maxAscent;
    }
  }

  gfxContext* thebesCtx = aCtx->ThebesContext();
  nsRect shadowRect(0, 0, 0, 0);

  
  
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_UNDERLINE) {
    gfxSize size(lineWidth, metrics.underlineSize);
    nsRect rect = nsCSSRendering::GetTextDecorationRect(presContext, size,
                     ascent, underlineOffset, NS_STYLE_TEXT_DECORATION_UNDERLINE,
                     nsCSSRendering::DECORATION_STYLE_SOLID);
    shadowRect.UnionRect(shadowRect, rect + linePt);
  }
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_OVERLINE) {
    gfxSize size(lineWidth, metrics.underlineSize);
    nsRect rect = nsCSSRendering::GetTextDecorationRect(presContext, size,
                     ascent, metrics.maxAscent, NS_STYLE_TEXT_DECORATION_OVERLINE,
                     nsCSSRendering::DECORATION_STYLE_SOLID);
    shadowRect.UnionRect(shadowRect, rect + linePt);
  }
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_LINE_THROUGH) {
    gfxSize size(lineWidth, metrics.strikeoutSize);
    nsRect rect = nsCSSRendering::GetTextDecorationRect(presContext, size,
                     ascent, metrics.strikeoutOffset, NS_STYLE_TEXT_DECORATION_LINE_THROUGH,
                     nsCSSRendering::DECORATION_STYLE_SOLID);
    shadowRect.UnionRect(shadowRect, rect + linePt);
  }

  
  nsContextBoxBlur contextBoxBlur;
  gfxContext* shadowCtx = contextBoxBlur.Init(shadowRect, mBlurRadius,
                                              presContext->AppUnitsPerDevPixel(),
                                              thebesCtx, mVisibleRect);
  if (!shadowCtx)
    return;

  thebesCtx->Save();
  thebesCtx->NewPath();
  thebesCtx->SetColor(gfxRGBA(mColor));

  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_UNDERLINE) {
    f->PaintTextDecorationLine(shadowCtx, pt, mLine, mColor,
                               underlineOffset, ascent,
                               metrics.underlineSize, NS_STYLE_TEXT_DECORATION_UNDERLINE);
  }
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_OVERLINE) {
    f->PaintTextDecorationLine(shadowCtx, pt, mLine, mColor,
                               metrics.maxAscent, ascent,
                               metrics.underlineSize, NS_STYLE_TEXT_DECORATION_OVERLINE);
  }
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_LINE_THROUGH) {
    f->PaintTextDecorationLine(shadowCtx, pt, mLine, mColor,
                               metrics.strikeoutOffset, ascent,
                               metrics.strikeoutSize, NS_STYLE_TEXT_DECORATION_LINE_THROUGH);
  }

  contextBoxBlur.DoPaint();
  thebesCtx->Restore();
}

nsRect
nsDisplayTextShadow::GetBounds(nsDisplayListBuilder* aBuilder)
{
  
  return mFrame->GetOverflowRect() + aBuilder->ToReferenceFrame(mFrame);
}

nsresult
nsHTMLContainerFrame::DisplayTextDecorations(nsDisplayListBuilder* aBuilder,
                                             nsDisplayList* aBelowTextDecorations,
                                             nsDisplayList* aAboveTextDecorations,
                                             nsLineBox* aLine)
{
  if (eCompatibility_NavQuirks == PresContext()->CompatibilityMode())
    return NS_OK;
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;
  
  
  
  
  nscolor underColor, overColor, strikeColor;
  PRUint8 decorations = NS_STYLE_TEXT_DECORATION_NONE;
  GetTextDecorations(PresContext(), aLine != nsnull, decorations, underColor, 
                     overColor, strikeColor);

  if (decorations == NS_STYLE_TEXT_DECORATION_NONE)
    return NS_OK;

  
  
  const nsStyleText* textStyle = GetStyleText();

  if (textStyle->mTextShadow) {
    for (PRUint32 i = textStyle->mTextShadow->Length(); i > 0; --i) {
      nsCSSShadowItem* shadow = textStyle->mTextShadow->ShadowAt(i - 1);
      nscoord blurRadius = shadow->mRadius;
      nscolor shadowColor;

      if (shadow->mHasColor)
        shadowColor = shadow->mColor;
      else
        shadowColor = GetStyleColor()->mColor;

      nsPoint offset(shadow->mXOffset, shadow->mYOffset);

      
      nsresult rv = aBelowTextDecorations->AppendNewToTop(new (aBuilder)
        nsDisplayTextShadow(this, decorations, shadowColor, aLine, blurRadius, offset));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (decorations & NS_STYLE_TEXT_DECORATION_UNDERLINE) {
    nsresult rv = aBelowTextDecorations->AppendNewToTop(new (aBuilder)
      nsDisplayTextDecoration(this, NS_STYLE_TEXT_DECORATION_UNDERLINE, underColor, aLine));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (decorations & NS_STYLE_TEXT_DECORATION_OVERLINE) {
    nsresult rv = aBelowTextDecorations->AppendNewToTop(new (aBuilder)
      nsDisplayTextDecoration(this, NS_STYLE_TEXT_DECORATION_OVERLINE, overColor, aLine));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (decorations & NS_STYLE_TEXT_DECORATION_LINE_THROUGH) {
    nsresult rv = aAboveTextDecorations->AppendNewToTop(new (aBuilder)
      nsDisplayTextDecoration(this, NS_STYLE_TEXT_DECORATION_LINE_THROUGH, strikeColor, aLine));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
nsHTMLContainerFrame::DisplayTextDecorationsAndChildren(
    nsDisplayListBuilder* aBuilder, const nsRect& aDirtyRect,
    const nsDisplayListSet& aLists)
{
  nsDisplayList aboveChildrenDecorations;
  nsresult rv = DisplayTextDecorations(aBuilder, aLists.Content(),
      &aboveChildrenDecorations, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists,
                                           DISPLAY_CHILD_INLINE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  aLists.Content()->AppendToTop(&aboveChildrenDecorations);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLContainerFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                       const nsRect&           aDirtyRect,
                                       const nsDisplayListSet& aLists) {
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  return DisplayTextDecorationsAndChildren(aBuilder, aDirtyRect, aLists);
}

static PRBool 
HasTextFrameDescendantOrInFlow(nsIFrame* aFrame);

 void
nsHTMLContainerFrame::PaintTextDecorationLine(
                   gfxContext* aCtx, 
                   const nsPoint& aPt,
                   nsLineBox* aLine,
                   nscolor aColor, 
                   gfxFloat aOffset, 
                   gfxFloat aAscent, 
                   gfxFloat aSize,
                   const PRUint8 aDecoration) 
{
  NS_ASSERTION(!aLine, "Should not have passed a linebox to a non-block frame");
  nsMargin bp = GetUsedBorderAndPadding();
  PRIntn skip = GetSkipSides();
  NS_FOR_CSS_SIDES(side) {
    if (skip & (1 << side)) {
      bp.side(side) = 0;
    }
  }
  nscoord innerWidth = mRect.width - bp.left - bp.right;
  gfxPoint pt(PresContext()->AppUnitsToGfxUnits(bp.left + aPt.x),
              PresContext()->AppUnitsToGfxUnits(bp.top + aPt.y));
  gfxSize size(PresContext()->AppUnitsToGfxUnits(innerWidth), aSize);
  nsCSSRendering::PaintDecorationLine(aCtx, aColor, pt, size, aAscent, aOffset,
                    aDecoration, nsCSSRendering::DECORATION_STYLE_SOLID);
}

 void
nsHTMLContainerFrame::AdjustForTextIndent(const nsLineBox* aLine,
                                          nscoord& start,
                                          nscoord& width)
{
  
  
  
}

void
nsHTMLContainerFrame::GetTextDecorations(nsPresContext* aPresContext, 
                                         PRBool aIsBlock,
                                         PRUint8& aDecorations,
                                         nscolor& aUnderColor, 
                                         nscolor& aOverColor, 
                                         nscolor& aStrikeColor)
{
  aDecorations = NS_STYLE_TEXT_DECORATION_NONE;
  if (!mStyleContext->HasTextDecorations()) {
    
    
    return; 
  }

  
  PRUint8 decorMask = NS_STYLE_TEXT_DECORATION_UNDERLINE |
                      NS_STYLE_TEXT_DECORATION_OVERLINE |
                      NS_STYLE_TEXT_DECORATION_LINE_THROUGH; 

  if (!aIsBlock) {
    aDecorations = GetStyleTextReset()->mTextDecoration  & decorMask;
    if (aDecorations) {
      const nsStyleColor* styleColor = GetStyleColor();
      aUnderColor = styleColor->mColor;
      aOverColor = styleColor->mColor;
      aStrikeColor = styleColor->mColor;
    }
  }
  else {
    
    for (nsIFrame *frame = this; frame && decorMask; frame = frame->GetParent()) {
      

      nsStyleContext* styleContext = frame->GetStyleContext();
      const nsStyleDisplay* styleDisplay = styleContext->GetStyleDisplay();
      if (!styleDisplay->IsBlockInside() &&
          styleDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL &&
          styleDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CAPTION) {
        
        
        
        break;
      }

      const nsStyleTextReset* styleText = styleContext->GetStyleTextReset();
      PRUint8 decors = decorMask & styleText->mTextDecoration;
      if (decors) {
        
        nscolor color = styleContext->GetStyleColor()->mColor;

        if (NS_STYLE_TEXT_DECORATION_UNDERLINE & decors) {
          aUnderColor = color;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
          aDecorations |= NS_STYLE_TEXT_DECORATION_UNDERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_OVERLINE & decors) {
          aOverColor = color;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_OVERLINE;
          aDecorations |= NS_STYLE_TEXT_DECORATION_OVERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_LINE_THROUGH & decors) {
          aStrikeColor = color;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
          aDecorations |= NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
        }
      }
    }
  }
  
  if (aDecorations) {
    
    if (!HasTextFrameDescendantOrInFlow(this)) {
      aDecorations = NS_STYLE_TEXT_DECORATION_NONE;
    }
  }
}

static PRBool 
HasTextFrameDescendant(nsIFrame* aParent)
{
  for (nsIFrame* kid = aParent->GetFirstChild(nsnull); kid;
       kid = kid->GetNextSibling())
  {
    if (kid->GetType() == nsGkAtoms::textFrame) {
      
      
      
      if (!kid->IsEmpty()) {
        return PR_TRUE;
      }
    }
    if (HasTextFrameDescendant(kid)) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

static PRBool 
HasTextFrameDescendantOrInFlow(nsIFrame* aFrame)
{
  for (nsIFrame *f = aFrame->GetFirstInFlow(); f; f = f->GetNextInFlow()) {
    if (HasTextFrameDescendant(f))
      return PR_TRUE;
  }
  return PR_FALSE;
}






nsresult
nsHTMLContainerFrame::CreateNextInFlow(nsPresContext* aPresContext,
                                       nsIFrame*       aFrame,
                                       nsIFrame*&      aNextInFlowResult)
{
  NS_PRECONDITION(GetType() != nsGkAtoms::blockFrame,
                  "you should have called nsBlockFrame::CreateContinuationFor instead");
  NS_PRECONDITION(mFrames.ContainsFrame(aFrame), "expected an in-flow child frame");

  aNextInFlowResult = nsnull;

  nsIFrame* nextInFlow = aFrame->GetNextInFlow();
  if (nsnull == nextInFlow) {
    
    
    nsresult rv = aPresContext->PresShell()->FrameConstructor()->
      CreateContinuingFrame(aPresContext, aFrame, this, &nextInFlow);
    if (NS_FAILED(rv)) {
      return rv;
    }
    mFrames.InsertFrame(nsnull, aFrame, nextInFlow);

    NS_FRAME_LOG(NS_FRAME_TRACE_NEW_FRAMES,
       ("nsHTMLContainerFrame::CreateNextInFlow: frame=%p nextInFlow=%p",
        aFrame, nextInFlow));

    aNextInFlowResult = nextInFlow;
  }
  return NS_OK;
}

static nsresult
ReparentFrameViewTo(nsIFrame*       aFrame,
                    nsIViewManager* aViewManager,
                    nsIView*        aNewParentView,
                    nsIView*        aOldParentView)
{

  
  

  
  if (aFrame->HasView()) {
#ifdef MOZ_XUL
    if (aFrame->GetType() == nsGkAtoms::menuPopupFrame) {
      
      return NS_OK;
    }
#endif
    nsIView* view = aFrame->GetView();
    
    
    

    aViewManager->RemoveChild(view);
    
    
    nsIView* insertBefore = nsLayoutUtils::FindSiblingViewFor(aNewParentView, aFrame);
    aViewManager->InsertChild(aNewParentView, view, insertBefore, insertBefore != nsnull);
  } else {
    PRInt32 listIndex = 0;
    nsIAtom* listName = nsnull;
    
    
    do {
      
      
      nsIFrame* childFrame = aFrame->GetFirstChild(listName);
      for (; childFrame; childFrame = childFrame->GetNextSibling()) {
        ReparentFrameViewTo(childFrame, aViewManager,
                            aNewParentView, aOldParentView);
      }
      listName = aFrame->GetAdditionalChildListName(listIndex++);
    } while (listName);
  }

  return NS_OK;
}

nsresult
nsHTMLContainerFrame::ReparentFrameView(nsPresContext* aPresContext,
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

  
  
  nsIView* oldParentView = aOldParentFrame->GetClosestView();
  nsIView* newParentView = aNewParentFrame->GetClosestView();
  
  
  
  
  if (oldParentView != newParentView) {
    
    return ReparentFrameViewTo(aChildFrame, oldParentView->GetViewManager(), newParentView,
                               oldParentView);
  }

  return NS_OK;
}

nsresult
nsHTMLContainerFrame::ReparentFrameViewList(nsPresContext*     aPresContext,
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

  
  
  nsIView* oldParentView = aOldParentFrame->GetClosestView();
  nsIView* newParentView = aNewParentFrame->GetClosestView();
  
  
  
  
  if (oldParentView != newParentView) {
    nsIViewManager* viewManager = oldParentView->GetViewManager();

    
    for (nsFrameList::Enumerator e(aChildFrameList); !e.AtEnd(); e.Next()) {
      ReparentFrameViewTo(e.get(), viewManager, newParentView, oldParentView);
    }
  }

  return NS_OK;
}

nsresult
nsHTMLContainerFrame::CreateViewForFrame(nsIFrame* aFrame,
                                         PRBool aForce)
{
  if (aFrame->HasView()) {
    return NS_OK;
  }

  
  if (!(aForce || FrameNeedsView(aFrame))) {
    
    return NS_OK;
  }

  nsIView* parentView = aFrame->GetParent()->GetParentViewForChildFrame(aFrame);
  NS_ASSERTION(parentView, "no parent with view");

  nsIViewManager* viewManager = parentView->GetViewManager();
  NS_ASSERTION(viewManager, "null view manager");
    
  
  nsIView* view = viewManager->CreateView(aFrame->GetRect(), parentView);
  if (!view)
    return NS_ERROR_OUT_OF_MEMORY;

  SyncFrameViewProperties(aFrame->PresContext(), aFrame, nsnull, view);

  
  
  nsIScrollableView*  scrollingView = parentView->ToScrollableView();
  if (scrollingView) {
    scrollingView->SetScrolledView(view);
  } else {
    nsIView* insertBefore = nsLayoutUtils::FindSiblingViewFor(parentView, aFrame);
    
    
    
    viewManager->InsertChild(parentView, view, insertBefore, insertBefore != nsnull);
  }

  
  
  
  
  
  
  
  
  ReparentFrameViewTo(aFrame, viewManager, view, parentView);

  
  aFrame->SetView(view);

  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
               ("nsHTMLContainerFrame::CreateViewForFrame: frame=%p view=%p",
                aFrame));
  return NS_OK;
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLContainerFrame)
