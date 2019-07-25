







































#include "nsHTMLContainerFrame.h"
#include "nsFirstLetterFrame.h"
#include "nsRenderingContext.h"
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
#include "nsWidgetsCID.h"
#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "gfxFont.h"
#include "nsCSSFrameConstructor.h"
#include "nsDisplayList.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsDisplayList.h"
#include "nsCSSRendering.h"

class nsDisplayTextDecoration : public nsDisplayItem {
public:
  nsDisplayTextDecoration(nsDisplayListBuilder* aBuilder,
                          nsHTMLContainerFrame* aFrame, PRUint8 aDecoration,
                          nscolor aColor, PRUint8 aStyle, nsLineBox* aLine)
    : nsDisplayItem(aBuilder, aFrame), mLine(aLine), mColor(aColor),
      mDecoration(aDecoration), mStyle(aStyle) {
    MOZ_COUNT_CTOR(nsDisplayTextDecoration);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTextDecoration() {
    MOZ_COUNT_DTOR(nsDisplayTextDecoration);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  NS_DISPLAY_DECL_NAME("TextDecoration", TYPE_TEXT_DECORATION)

  virtual PRUint32 GetPerFrameKey()
  {
    return TYPE_TEXT_DECORATION | (mDecoration << TYPE_BITS);
  }

private:
  nsLineBox* mLine;
  nscolor    mColor;
  PRUint8    mDecoration;
  PRUint8    mStyle;
};

void
nsDisplayTextDecoration::Paint(nsDisplayListBuilder* aBuilder,
                               nsRenderingContext* aCtx)
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

  nsPoint pt = ToReferenceFrame();
  nsHTMLContainerFrame* f = static_cast<nsHTMLContainerFrame*>(mFrame);
  if (mDecoration == NS_STYLE_TEXT_DECORATION_UNDERLINE) {
    gfxFloat underlineOffset = fontGroup->GetUnderlineOffset();
    f->PaintTextDecorationLine(aCtx->ThebesContext(), pt, mLine, mColor,
                               mStyle, underlineOffset, ascent,
                               metrics.underlineSize, mDecoration);
  } else if (mDecoration == NS_STYLE_TEXT_DECORATION_OVERLINE) {
    f->PaintTextDecorationLine(aCtx->ThebesContext(), pt, mLine, mColor,
                               mStyle, metrics.maxAscent, ascent,
                               metrics.underlineSize, mDecoration);
  } else {
    f->PaintTextDecorationLine(aCtx->ThebesContext(), pt, mLine, mColor,
                               mStyle, metrics.strikeoutOffset,
                               ascent, metrics.strikeoutSize, mDecoration);
  }
}

nsRect
nsDisplayTextDecoration::GetBounds(nsDisplayListBuilder* aBuilder)
{
  return mFrame->GetVisualOverflowRect() + ToReferenceFrame();
}

class nsDisplayTextShadow : public nsDisplayItem {
public:
  nsDisplayTextShadow(nsDisplayListBuilder* aBuilder,
                      nsHTMLContainerFrame* aFrame,
                      const PRUint8 aDecoration, PRUint8 aUnderlineStyle,
                      PRUint8 aOverlineStyle, PRUint8 aStrikeThroughStyle,
                      nsLineBox* aLine)
    : nsDisplayItem(aBuilder, aFrame), mLine(aLine),
      mDecorationFlags(aDecoration), mUnderlineStyle(aUnderlineStyle),
      mOverlineStyle(aOverlineStyle), mStrikeThroughStyle(aStrikeThroughStyle) {
    MOZ_COUNT_CTOR(nsDisplayTextShadow);
  }
  virtual ~nsDisplayTextShadow() {
    MOZ_COUNT_DTOR(nsDisplayTextShadow);
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  NS_DISPLAY_DECL_NAME("TextShadowContainer", TYPE_TEXT_SHADOW)
private:
  nsLineBox*    mLine;
  PRUint8       mDecorationFlags;
  PRUint8       mUnderlineStyle;
  PRUint8       mOverlineStyle;
  PRUint8       mStrikeThroughStyle;
};

void
nsDisplayTextShadow::Paint(nsDisplayListBuilder* aBuilder,
                           nsRenderingContext* aCtx)
{
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
  nsPresContext* presContext = mFrame->PresContext();
  gfxContext* thebesCtx = aCtx->ThebesContext();

  gfxFloat ascent;
  gfxFloat lineWidth;
  nscoord start;
  if (mLine) {
    
    nscoord width = mLine->mBounds.width;
    start = mLine->mBounds.x;
    f->AdjustForTextIndent(mLine, start, width);
    if (width <= 0)
      return;

    lineWidth = presContext->AppUnitsToGfxUnits(width);
    ascent = presContext->AppUnitsToGfxUnits(mLine->GetAscent());
  } else {
    
    lineWidth = presContext->AppUnitsToGfxUnits(mFrame->GetContentRect().width);

    
    
    
    if (mFrame->GetType() == nsGkAtoms::letterFrame) {
      
      
      
      nsFirstLetterFrame* letterFrame = static_cast<nsFirstLetterFrame*>(mFrame);
      nscoord tmp = letterFrame->GetFirstLetterBaseline();
      tmp -= letterFrame->GetUsedBorderAndPadding().top;
      ascent = presContext->AppUnitsToGfxUnits(tmp);
    } else {
      ascent = metrics.maxAscent;
    }
  }

  nsCSSShadowArray* shadowList = mFrame->GetStyleText()->mTextShadow;
  NS_ABORT_IF_FALSE(shadowList,
                    "Why did we make a display list item if we have no shadows?");

  
  
  nsRect underlineRect;
  nsRect overlineRect;
  nsRect lineThroughRect;
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_UNDERLINE) {
    gfxSize size(lineWidth, metrics.underlineSize);
    underlineRect = nsCSSRendering::GetTextDecorationRect(presContext, size,
                       ascent, underlineOffset,
                       NS_STYLE_TEXT_DECORATION_UNDERLINE, mUnderlineStyle);
  }
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_OVERLINE) {
    gfxSize size(lineWidth, metrics.underlineSize);
    overlineRect = nsCSSRendering::GetTextDecorationRect(presContext, size,
                       ascent, metrics.maxAscent,
                       NS_STYLE_TEXT_DECORATION_OVERLINE, mOverlineStyle);
  }
  if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_LINE_THROUGH) {
    gfxSize size(lineWidth, metrics.strikeoutSize);
    lineThroughRect = nsCSSRendering::GetTextDecorationRect(presContext, size,
                       ascent, metrics.strikeoutOffset,
                       NS_STYLE_TEXT_DECORATION_LINE_THROUGH,
                       mStrikeThroughStyle);
  }

  for (PRUint32 i = shadowList->Length(); i > 0; --i) {
    nsCSSShadowItem* shadow = shadowList->ShadowAt(i - 1);

    nscolor shadowColor =
      shadow->mHasColor ? shadow->mColor : mFrame->GetStyleColor()->mColor;

    nsPoint pt = ToReferenceFrame() +
      nsPoint(shadow->mXOffset, shadow->mYOffset);
    nsPoint linePt;
    if (mLine) {
      linePt = nsPoint(start + pt.x, mLine->mBounds.y + pt.y);
    } else {
      linePt = mFrame->GetContentRect().TopLeft() - mFrame->GetPosition() + pt;
    }

    nsRect shadowRect(0, 0, 0, 0);
    if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_UNDERLINE) {
      shadowRect.UnionRect(shadowRect, underlineRect + linePt);
    }
    if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_OVERLINE) {
      shadowRect.UnionRect(shadowRect, overlineRect + linePt);
    }
    if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_LINE_THROUGH) {
      shadowRect.UnionRect(shadowRect, lineThroughRect + linePt);
    }

    gfxContextAutoSaveRestore save(thebesCtx);
    thebesCtx->NewPath();
    thebesCtx->SetColor(gfxRGBA(shadowColor));

    
    nsContextBoxBlur contextBoxBlur;
    gfxContext* shadowCtx = contextBoxBlur.Init(shadowRect, 0, shadow->mRadius,
                                                presContext->AppUnitsPerDevPixel(),
                                                thebesCtx, mVisibleRect, nsnull);
    if (!shadowCtx) {
      continue;
    }

    if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_UNDERLINE) {
      f->PaintTextDecorationLine(shadowCtx, pt, mLine, shadowColor,
                                 mUnderlineStyle, underlineOffset, ascent,
                                 metrics.underlineSize, NS_STYLE_TEXT_DECORATION_UNDERLINE);
    }
    if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_OVERLINE) {
      f->PaintTextDecorationLine(shadowCtx, pt, mLine, shadowColor,
                                 mOverlineStyle, metrics.maxAscent, ascent,
                                 metrics.underlineSize, NS_STYLE_TEXT_DECORATION_OVERLINE);
    }
    if (mDecorationFlags & NS_STYLE_TEXT_DECORATION_LINE_THROUGH) {
      f->PaintTextDecorationLine(shadowCtx, pt, mLine, shadowColor,
                                 mStrikeThroughStyle, metrics.strikeoutOffset,
                                 ascent, metrics.strikeoutSize,
                                 NS_STYLE_TEXT_DECORATION_LINE_THROUGH);
    }

    contextBoxBlur.DoPaint();
  }
}

nsRect
nsDisplayTextShadow::GetBounds(nsDisplayListBuilder* aBuilder)
{
  
  return mFrame->GetVisualOverflowRect() + ToReferenceFrame();
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

  
  nsCOMPtr<nsIFontMetrics> fm;
  nsresult rv = nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  NS_ENSURE_SUCCESS(rv, rv);
  nsIThebesFontMetrics* tfm = static_cast<nsIThebesFontMetrics*>(fm.get());
  if (tfm->GetThebesFontGroup()->ShouldSkipDrawing())
    return NS_OK;

  
  
  
  nscolor underColor, overColor, strikeColor;
  PRUint8 underStyle, overStyle, strikeStyle;
  PRUint8 decorations = NS_STYLE_TEXT_DECORATION_NONE;
  GetTextDecorations(PresContext(), aLine != nsnull, decorations, underColor, 
                     overColor, strikeColor, underStyle, overStyle,
                     strikeStyle);

  if (decorations == NS_STYLE_TEXT_DECORATION_NONE)
    return NS_OK;

  
  
  
  if (GetStyleText()->mTextShadow) {
    rv = aBelowTextDecorations->AppendNewToTop(new (aBuilder)
      nsDisplayTextShadow(aBuilder, this, decorations, underStyle, overStyle,
                          strikeStyle, aLine));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (decorations & NS_STYLE_TEXT_DECORATION_UNDERLINE) {
    rv = aBelowTextDecorations->AppendNewToTop(new (aBuilder)
      nsDisplayTextDecoration(aBuilder, this, NS_STYLE_TEXT_DECORATION_UNDERLINE,
                              underColor, underStyle, aLine));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (decorations & NS_STYLE_TEXT_DECORATION_OVERLINE) {
    rv = aBelowTextDecorations->AppendNewToTop(new (aBuilder)
      nsDisplayTextDecoration(aBuilder, this, NS_STYLE_TEXT_DECORATION_OVERLINE,
                              overColor, overStyle, aLine));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (decorations & NS_STYLE_TEXT_DECORATION_LINE_THROUGH) {
    rv = aAboveTextDecorations->AppendNewToTop(new (aBuilder)
      nsDisplayTextDecoration(aBuilder, this, NS_STYLE_TEXT_DECORATION_LINE_THROUGH,
                              strikeColor, strikeStyle, aLine));
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
                   PRUint8 aStyle,
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
                                      aDecoration, aStyle);
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
                                         nscolor& aStrikeColor,
                                         PRUint8& aUnderStyle,
                                         PRUint8& aOverStyle,
                                         PRUint8& aStrikeStyle)
{
  aDecorations = NS_STYLE_TEXT_DECORATION_NONE;
  if (!mStyleContext->HasTextDecorations()) {
    
    
    return; 
  }

  if (!aIsBlock) {
    const nsStyleTextReset* styleTextReset = this->GetStyleTextReset();
    aDecorations = styleTextReset->mTextDecoration &
                   NS_STYLE_TEXT_DECORATION_LINES_MASK;
    if (aDecorations) {
      nscolor color =
        this->GetVisitedDependentColor(eCSSProperty_text_decoration_color);
      aUnderColor = aOverColor = aStrikeColor = color;
      aUnderStyle = aOverStyle = aStrikeStyle =
        styleTextReset->GetDecorationStyle();
    }
  }
  else {
    
    
    
    
    
    
    PRUint8 decorMask = NS_STYLE_TEXT_DECORATION_LINES_MASK;

    
    for (nsIFrame* frame = this; frame; frame = frame->GetParent()) {
      const nsStyleTextReset* styleTextReset = frame->GetStyleTextReset();
      PRUint8 decors = styleTextReset->mTextDecoration & decorMask;
      if (decors) {
        
        nscolor color = frame->GetVisitedDependentColor(
                                 eCSSProperty_text_decoration_color);
        PRUint8 style = styleTextReset->GetDecorationStyle();

        if (NS_STYLE_TEXT_DECORATION_UNDERLINE & decors) {
          aUnderColor = color;
          aUnderStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
          aDecorations |= NS_STYLE_TEXT_DECORATION_UNDERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_OVERLINE & decors) {
          aOverColor = color;
          aOverStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_OVERLINE;
          aDecorations |= NS_STYLE_TEXT_DECORATION_OVERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_LINE_THROUGH & decors) {
          aStrikeColor = color;
          aStrikeStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
          aDecorations |= NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
        }
      }
      
      
      if (!decorMask) {
        break;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      const nsStyleDisplay* styleDisplay = frame->GetStyleDisplay();
      if (styleDisplay->IsFloating() ||
          styleDisplay->IsAbsolutelyPositioned() ||
          styleDisplay->IsInlineOutside()) {
        break;
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

  
  if (!aForce && !aFrame->NeedsView()) {
    
    return NS_OK;
  }

  nsIView* parentView = aFrame->GetParent()->GetClosestView();
  NS_ASSERTION(parentView, "no parent with view");

  nsIViewManager* viewManager = parentView->GetViewManager();
  NS_ASSERTION(viewManager, "null view manager");
    
  
  nsIView* view = viewManager->CreateView(aFrame->GetRect(), parentView);
  if (!view)
    return NS_ERROR_OUT_OF_MEMORY;

  SyncFrameViewProperties(aFrame->PresContext(), aFrame, nsnull, view);

  nsIView* insertBefore = nsLayoutUtils::FindSiblingViewFor(parentView, aFrame);
  
  
  
  viewManager->InsertChild(parentView, view, insertBefore, insertBefore != nsnull);

  
  
  
  
  
  
  
  
  ReparentFrameViewTo(aFrame, viewManager, view, parentView);

  
  aFrame->SetView(view);

  NS_FRAME_LOG(NS_FRAME_TRACE_CALLS,
               ("nsHTMLContainerFrame::CreateViewForFrame: frame=%p view=%p",
                aFrame));
  return NS_OK;
}

NS_IMPL_FRAMEARENA_HELPERS(nsHTMLContainerFrame)
