




#include "nsPageFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsCSSFrameConstructor.h"
#include "nsReadableUtils.h"
#include "nsPageContentFrame.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h" 
#include "nsCSSRendering.h"
#include "nsSimplePageSequence.h" 
#include "nsTextFormatter.h" 
#ifdef IBMBIDI
#include "nsBidiUtils.h"
#endif
#include "nsIPrintSettings.h"
#include "nsRegion.h"

#include "prlog.h"
#ifdef PR_LOGGING 
extern PRLogModuleInfo *GetLayoutPrintingLog();
#define PR_PL(_p1)  PR_LOG(GetLayoutPrintingLog(), PR_LOG_DEBUG, _p1)
#else
#define PR_PL(_p1)
#endif

nsIFrame*
NS_NewPageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPageFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPageFrame)

nsPageFrame::nsPageFrame(nsStyleContext* aContext)
: nsContainerFrame(aContext)
{
}

nsPageFrame::~nsPageFrame()
{
}

NS_IMETHODIMP nsPageFrame::Reflow(nsPresContext*           aPresContext,
                                  nsHTMLReflowMetrics&     aDesiredSize,
                                  const nsHTMLReflowState& aReflowState,
                                  nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPageFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  aStatus = NS_FRAME_COMPLETE;  

  NS_ASSERTION(mFrames.FirstChild() &&
               nsGkAtoms::pageContentFrame == mFrames.FirstChild()->GetType(),
               "pageFrame must have a pageContentFrame child");

  
  
  if (mFrames.NotEmpty()) {
    nsIFrame* frame = mFrames.FirstChild();
    
    
    
    nscoord avHeight;
    if (mPD->mReflowSize.height == NS_UNCONSTRAINEDSIZE) {
      avHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      avHeight = mPD->mReflowSize.height;
    }
    nsSize  maxSize(mPD->mReflowSize.width, avHeight);
    float scale = aPresContext->GetPageScale();
    maxSize.width = NSToCoordCeil(maxSize.width / scale);
    if (maxSize.height != NS_UNCONSTRAINEDSIZE) {
      maxSize.height = NSToCoordCeil(maxSize.height / scale);
    }
    
    nscoord onePixelInTwips = nsPresContext::CSSPixelsToAppUnits(1);
    
    
    
    if (maxSize.width < onePixelInTwips || maxSize.height < onePixelInTwips) {
      aDesiredSize.width  = 0;
      aDesiredSize.height = 0;
      NS_WARNING("Reflow aborted; no space for content");
      return NS_OK;
    }

    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame, maxSize);
    kidReflowState.mFlags.mIsTopOfPage = true;
    kidReflowState.mFlags.mTableIsSplittable = true;

    
    
    nsMargin pageContentMargin;
    const nsStyleSides& marginStyle = kidReflowState.mStyleMargin->mMargin;
    NS_FOR_CSS_SIDES(side) {
      if (marginStyle.GetUnit(side) == eStyleUnit_Auto) {
        pageContentMargin.Side(side) = mPD->mReflowMargin.Side(side);
      } else {
        pageContentMargin.Side(side) = kidReflowState.mComputedMargin.Side(side);
      }
    }


    nscoord maxWidth = maxSize.width - pageContentMargin.LeftRight() / scale;
    nscoord maxHeight;
    if (maxSize.height == NS_UNCONSTRAINEDSIZE) {
      maxHeight = NS_UNCONSTRAINEDSIZE;
    } else {
      maxHeight = maxSize.height - pageContentMargin.TopBottom() / scale;
    }

    
    
    if (maxWidth < onePixelInTwips ||
       (maxHeight != NS_UNCONSTRAINEDSIZE && maxHeight < onePixelInTwips)) {
      NS_FOR_CSS_SIDES(side) {
        pageContentMargin.Side(side) = mPD->mReflowMargin.Side(side);
      }
      maxWidth = maxSize.width - pageContentMargin.LeftRight() / scale;
      if (maxHeight != NS_UNCONSTRAINEDSIZE) {
        maxHeight = maxSize.height - pageContentMargin.TopBottom() / scale;
      }
    }

    kidReflowState.SetComputedWidth(maxWidth);
    kidReflowState.SetComputedHeight(maxHeight);

    
    nscoord xc = pageContentMargin.left;
    nscoord yc = pageContentMargin.top;

    
    ReflowChild(frame, aPresContext, aDesiredSize, kidReflowState, xc, yc, 0, aStatus);

    
    FinishReflowChild(frame, aPresContext, &kidReflowState, aDesiredSize, xc, yc, 0);

    NS_ASSERTION(!NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
                 !frame->GetNextInFlow(), "bad child flow list");
  }
  PR_PL(("PageFrame::Reflow %p ", this));
  PR_PL(("[%d,%d][%d,%d]\n", aDesiredSize.width, aDesiredSize.height, aReflowState.availableWidth, aReflowState.availableHeight));

  
  aDesiredSize.width = aReflowState.availableWidth;
  if (aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE) {
    aDesiredSize.height = aReflowState.availableHeight;
  }

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aDesiredSize);

  PR_PL(("PageFrame::Reflow %p ", this));
  PR_PL(("[%d,%d]\n", aReflowState.availableWidth, aReflowState.availableHeight));

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

nsIAtom*
nsPageFrame::GetType() const
{
  return nsGkAtoms::pageFrame; 
}

#ifdef DEBUG
NS_IMETHODIMP
nsPageFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Page"), aResult);
}
#endif

void 
nsPageFrame::ProcessSpecialCodes(const nsString& aStr, nsString& aNewStr)
{

  aNewStr = aStr;

  
  
  NS_NAMED_LITERAL_STRING(kDate, "&D");
  if (aStr.Find(kDate) != kNotFound) {
    if (mPD->mDateTimeStr != nullptr) {
      aNewStr.ReplaceSubstring(kDate.get(), mPD->mDateTimeStr);
    } else {
      aNewStr.ReplaceSubstring(kDate.get(), EmptyString().get());
    }
  }

  
  
  
  
  
  NS_NAMED_LITERAL_STRING(kPageAndTotal, "&PT");
  if (aStr.Find(kPageAndTotal) != kNotFound) {
    PRUnichar * uStr = nsTextFormatter::smprintf(mPD->mPageNumAndTotalsFormat, mPageNum, mTotNumPages);
    aNewStr.ReplaceSubstring(kPageAndTotal.get(), uStr);
    nsMemory::Free(uStr);
  }

  
  
  NS_NAMED_LITERAL_STRING(kPage, "&P");
  if (aStr.Find(kPage) != kNotFound) {
    PRUnichar * uStr = nsTextFormatter::smprintf(mPD->mPageNumFormat, mPageNum);
    aNewStr.ReplaceSubstring(kPage.get(), uStr);
    nsMemory::Free(uStr);
  }

  NS_NAMED_LITERAL_STRING(kTitle, "&T");
  if (aStr.Find(kTitle) != kNotFound) {
    if (mPD->mDocTitle != nullptr) {
      aNewStr.ReplaceSubstring(kTitle.get(), mPD->mDocTitle);
    } else {
      aNewStr.ReplaceSubstring(kTitle.get(), EmptyString().get());
    }
  }

  NS_NAMED_LITERAL_STRING(kDocURL, "&U");
  if (aStr.Find(kDocURL) != kNotFound) {
    if (mPD->mDocURL != nullptr) {
      aNewStr.ReplaceSubstring(kDocURL.get(), mPD->mDocURL);
    } else {
      aNewStr.ReplaceSubstring(kDocURL.get(), EmptyString().get());
    }
  }

  NS_NAMED_LITERAL_STRING(kPageTotal, "&L");
  if (aStr.Find(kPageTotal) != kNotFound) {
    PRUnichar * uStr = nsTextFormatter::smprintf(mPD->mPageNumFormat, mTotNumPages);
    aNewStr.ReplaceSubstring(kPageTotal.get(), uStr);
    nsMemory::Free(uStr);
  }
}



nscoord nsPageFrame::GetXPosition(nsRenderingContext& aRenderingContext, 
                                  const nsRect&        aRect, 
                                  int32_t              aJust,
                                  const nsString&      aStr)
{
  nscoord width = nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                                aStr.get(), aStr.Length());

  nscoord x = aRect.x;
  switch (aJust) {
    case nsIPrintSettings::kJustLeft:
      x += mPD->mEdgePaperMargin.left;
      break;

    case nsIPrintSettings::kJustCenter:
      x += (aRect.width - width) / 2;
      break;

    case nsIPrintSettings::kJustRight:
      x += aRect.width - width - mPD->mEdgePaperMargin.right;
      break;
  } 

  return x;
}










void
nsPageFrame::DrawHeaderFooter(nsRenderingContext& aRenderingContext,
                              nsHeaderFooterEnum   aHeaderFooter,
                              const nsString&      aStrLeft,
                              const nsString&      aStrCenter,
                              const nsString&      aStrRight,
                              const nsRect&        aRect,
                              nscoord              aAscent,
                              nscoord              aHeight)
{
  int32_t numStrs = 0;
  if (!aStrLeft.IsEmpty()) numStrs++;
  if (!aStrCenter.IsEmpty()) numStrs++;
  if (!aStrRight.IsEmpty()) numStrs++;

  if (numStrs == 0) return;
  nscoord strSpace = aRect.width / numStrs;

  if (!aStrLeft.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aHeaderFooter,
                     nsIPrintSettings::kJustLeft, aStrLeft, aRect, aAscent,
                     aHeight, strSpace);
  }
  if (!aStrCenter.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aHeaderFooter,
                     nsIPrintSettings::kJustCenter, aStrCenter, aRect, aAscent,
                     aHeight, strSpace);
  }
  if (!aStrRight.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aHeaderFooter,
                     nsIPrintSettings::kJustRight, aStrRight, aRect, aAscent,
                     aHeight, strSpace);
  }
}










void
nsPageFrame::DrawHeaderFooter(nsRenderingContext& aRenderingContext,
                              nsHeaderFooterEnum   aHeaderFooter,
                              int32_t              aJust,
                              const nsString&      aStr,
                              const nsRect&        aRect,
                              nscoord              aAscent,
                              nscoord              aHeight,
                              nscoord              aWidth)
{

  nscoord contentWidth = aWidth - (mPD->mEdgePaperMargin.left + mPD->mEdgePaperMargin.right);

  if ((aHeaderFooter == eHeader && aHeight < mPD->mReflowMargin.top) ||
      (aHeaderFooter == eFooter && aHeight < mPD->mReflowMargin.bottom)) {
    nsAutoString str;
    ProcessSpecialCodes(aStr, str);

    int32_t indx;
    int32_t textWidth = 0;
    const PRUnichar* text = str.get();

    int32_t len = (int32_t)str.Length();
    if (len == 0) {
      return; 
    }
    
    if (nsLayoutUtils::BinarySearchForPosition(&aRenderingContext, text, 0, 0, 0, len,
                                int32_t(contentWidth), indx, textWidth)) {
      if (indx < len-1 ) {
        
        if (indx > 3) {
          
          
          
          
          
          
          str.Truncate(indx-3);
          str.AppendLiteral("...");
        } else {
          
          str.Truncate();
        }
      }
    } else { 
      return; 
    }
    
    if (HasRTLChars(str)) {
      PresContext()->SetBidiEnabled();
    }

    
    nscoord x = GetXPosition(aRenderingContext, aRect, aJust, str);
    nscoord y;
    if (aHeaderFooter == eHeader) {
      y = aRect.y + mPD->mEdgePaperMargin.top;
    } else {
      y = aRect.YMost() - aHeight - mPD->mEdgePaperMargin.bottom;
    }

    
    aRenderingContext.PushState();
    aRenderingContext.SetColor(NS_RGB(0,0,0));
    aRenderingContext.IntersectClip(aRect);
    nsLayoutUtils::DrawString(this, &aRenderingContext, str.get(), str.Length(), nsPoint(x, y + aAscent));
    aRenderingContext.PopState();
  }
}












static void
PruneDisplayListForExtraPage(nsDisplayListBuilder* aBuilder,
                             nsPageFrame* aPage, nsIFrame* aExtraPage,
                             nscoord aY, nsDisplayList* aList)
{
  nsDisplayList newList;

  while (true) {
    nsDisplayItem* i = aList->RemoveBottom();
    if (!i)
      break;
    nsDisplayList* subList = i->GetSameCoordinateSystemChildren();
    if (subList) {
      PruneDisplayListForExtraPage(aBuilder, aPage, aExtraPage, aY, subList);
      nsDisplayItem::Type type = i->GetType();
      if (type == nsDisplayItem::TYPE_CLIP ||
          type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT) {
        
        
        
        
        
        
        
        
        
        
        
        nsDisplayClip* clip = static_cast<nsDisplayClip*>(i);
        clip->SetClipRect(clip->GetClipRect() + nsPoint(0, aY) -
                aExtraPage->GetOffsetToCrossDoc(aBuilder->FindReferenceFrameFor(aPage)));
      }
      newList.AppendToTop(i);
    } else {
      nsIFrame* f = i->GetUnderlyingFrame();
      if (f && nsLayoutUtils::IsProperAncestorFrameCrossDoc(aPage, f)) {
        
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
                             nsPageFrame* aPage, nsIFrame* aExtraPage,
                             nscoord aY, nsDisplayList* aList)
{
  nsDisplayList list;
  
  
  
  
  
  
  nsresult rv =
    aExtraPage->BuildDisplayListForStackingContext(aBuilder, nsRect(), &list);
  if (NS_FAILED(rv))
    return rv;
  PruneDisplayListForExtraPage(aBuilder, aPage, aExtraPage, aY, &list);
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
    return nullptr;
  NS_ASSERTION(nextPageFrame->GetType() == nsGkAtoms::pageFrame,
               "pageFrame's sibling is not a page frame...");
  nsIFrame* f = nextPageFrame->GetFirstPrincipalChild();
  NS_ASSERTION(f, "pageFrame has no page content frame!");
  NS_ASSERTION(f->GetType() == nsGkAtoms::pageContentFrame,
               "pageFrame's child is not page content!");
  return f;
}

static void PaintHeaderFooter(nsIFrame* aFrame, nsRenderingContext* aCtx,
                              const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsPageFrame*>(aFrame)->PaintHeaderFooter(*aCtx, aPt);
}

static gfx3DMatrix ComputePageTransform(nsIFrame* aFrame, float aAppUnitsPerPixel)
{
  float scale = aFrame->PresContext()->GetPageScale();
  return gfx3DMatrix::ScalingMatrix(scale, scale, 1);
}


NS_IMETHODIMP
nsPageFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  nsDisplayListCollection set;
  nsresult rv;

  if (PresContext()->IsScreen()) {
    rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsDisplayList content;
  nsIFrame *child = mFrames.FirstChild();
  rv = child->BuildDisplayListForStackingContext(aBuilder,
      child->GetVisualOverflowRectRelativeToSelf(), &content);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  nsIFrame* page = child;
  nscoord y = child->GetSize().height;
  while ((page = GetNextPage(page)) != nullptr) {
    rv = BuildDisplayListForExtraPage(aBuilder, this, page, y, &content);
    if (NS_FAILED(rv))
      break;
    y += page->GetSize().height;
  }
  
  
  
  
  nsRect backgroundRect =
    nsRect(aBuilder->ToReferenceFrame(child), child->GetSize());
  rv = PresContext()->GetPresShell()->AddCanvasBackgroundColorItem(
         *aBuilder, content, child, backgroundRect, NS_RGBA(0,0,0,0));

  float scale = PresContext()->GetPageScale();
  nsRect clipRect(nsPoint(0, 0), child->GetSize());
  
  
  nscoord expectedPageContentHeight = NSToCoordCeil(GetSize().height / scale);
  if (clipRect.height > expectedPageContentHeight) {
    
    
    NS_ASSERTION(mPageNum > 0, "page num should be positive");
    
    
    
    
    clipRect.y = NSToCoordCeil((-child->GetRect().y + 
                                mPD->mReflowMargin.top) / scale);
    clipRect.height = expectedPageContentHeight;
    NS_ASSERTION(clipRect.y < child->GetSize().height,
                 "Should be clipping to region inside the page content bounds");
  }
  clipRect += aBuilder->ToReferenceFrame(child);
  rv = content.AppendNewToTop(new (aBuilder) nsDisplayClip(aBuilder, child, &content, clipRect));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = content.AppendNewToTop(new (aBuilder) nsDisplayTransform(aBuilder, child, &content, ::ComputePageTransform));
  NS_ENSURE_SUCCESS(rv, rv);

  set.Content()->AppendToTop(&content);

  if (PresContext()->IsRootPaginatedDocument()) {
    rv = set.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(aBuilder, this, ::PaintHeaderFooter,
                         "HeaderFooter",
                         nsDisplayItem::TYPE_HEADER_FOOTER));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  set.MoveTo(aLists);
  return NS_OK;
}


void
nsPageFrame::SetPageNumInfo(int32_t aPageNumber, int32_t aTotalPages) 
{ 
  mPageNum     = aPageNumber; 
  mTotNumPages = aTotalPages;
}


void
nsPageFrame::PaintHeaderFooter(nsRenderingContext& aRenderingContext,
                               nsPoint aPt)
{
  nsPresContext* pc = PresContext();

  if (!mPD->mPrintSettings) {
    if (pc->Type() == nsPresContext::eContext_PrintPreview || pc->IsDynamic())
      mPD->mPrintSettings = pc->GetPrintSettings();
    if (!mPD->mPrintSettings)
      return;
  }

  nsRect rect(aPt, mRect.Size());
  aRenderingContext.SetColor(NS_RGB(0,0,0));

  
  nsRefPtr<nsFontMetrics> fontMet;
  pc->DeviceContext()->GetMetricsFor(*mPD->mHeadFootFont, nullptr,
                                     pc->GetUserFontSet(),
                                     *getter_AddRefs(fontMet));

  aRenderingContext.SetFont(fontMet);

  nscoord ascent = 0;
  nscoord visibleHeight = 0;
  if (fontMet) {
    visibleHeight = fontMet->MaxHeight();
    ascent = fontMet->MaxAscent();
  }

  
  nsXPIDLString headerLeft, headerCenter, headerRight;
  mPD->mPrintSettings->GetHeaderStrLeft(getter_Copies(headerLeft));
  mPD->mPrintSettings->GetHeaderStrCenter(getter_Copies(headerCenter));
  mPD->mPrintSettings->GetHeaderStrRight(getter_Copies(headerRight));
  DrawHeaderFooter(aRenderingContext, eHeader,
                   headerLeft, headerCenter, headerRight,
                   rect, ascent, visibleHeight);

  nsXPIDLString footerLeft, footerCenter, footerRight;
  mPD->mPrintSettings->GetFooterStrLeft(getter_Copies(footerLeft));
  mPD->mPrintSettings->GetFooterStrCenter(getter_Copies(footerCenter));
  mPD->mPrintSettings->GetFooterStrRight(getter_Copies(footerRight));
  DrawHeaderFooter(aRenderingContext, eFooter,
                   footerLeft, footerCenter, footerRight,
                   rect, ascent, visibleHeight);
}

void
nsPageFrame::SetSharedPageData(nsSharedPageData* aPD) 
{ 
  mPD = aPD;
  
  nsPageContentFrame * pcf = static_cast<nsPageContentFrame*>(mFrames.FirstChild());
  if (pcf) {
    pcf->SetSharedPageData(mPD);
  }

}

nsIFrame*
NS_NewPageBreakFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  NS_PRECONDITION(aPresShell, "null PresShell");
  
  NS_ASSERTION(aPresShell->GetPresContext()->IsPaginated(), "created a page break frame while not printing");

  return new (aPresShell) nsPageBreakFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPageBreakFrame)

nsPageBreakFrame::nsPageBreakFrame(nsStyleContext* aContext) :
  nsLeafFrame(aContext), mHaveReflowed(false)
{
}

nsPageBreakFrame::~nsPageBreakFrame()
{
}

nscoord
nsPageBreakFrame::GetIntrinsicWidth()
{
  return nsPresContext::CSSPixelsToAppUnits(1);
}

nscoord
nsPageBreakFrame::GetIntrinsicHeight()
{
  return 0;
}

nsresult 
nsPageBreakFrame::Reflow(nsPresContext*           aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPageBreakFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  
  
  aDesiredSize.width = GetIntrinsicWidth();
  aDesiredSize.height = (aReflowState.availableHeight == NS_UNCONSTRAINEDSIZE ?
                         0 : aReflowState.availableHeight);
  
  aDesiredSize.height -=
    aDesiredSize.height % nsPresContext::CSSPixelsToAppUnits(1);

  
  
  mHaveReflowed = true;
  aStatus = NS_FRAME_COMPLETE; 
  return NS_OK;
}

nsIAtom*
nsPageBreakFrame::GetType() const
{
  return nsGkAtoms::pageBreakFrame; 
}

#ifdef DEBUG
NS_IMETHODIMP
nsPageBreakFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("PageBreak"), aResult);
}
#endif
