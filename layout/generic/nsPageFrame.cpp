




































#include "nsPageFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsIRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDeviceContext.h"
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
#include "nsIFontMetrics.h"
#include "nsIPrintSettings.h"
#include "nsRegion.h"

#include "prlog.h"
#ifdef PR_LOGGING 
extern PRLogModuleInfo * kLayoutPrintingLogMod;
#define PR_PL(_p1)  PR_LOG(kLayoutPrintingLogMod, PR_LOG_DEBUG, _p1)
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
      avHeight = mPD->mReflowSize.height - mPD->mReflowMargin.TopBottom();
    }
    nsSize  maxSize(mPD->mReflowSize.width - mPD->mReflowMargin.LeftRight(),
                    avHeight);
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
    kidReflowState.mFlags.mIsTopOfPage = PR_TRUE;
    kidReflowState.mFlags.mTableIsSplittable = PR_TRUE;

    
    nscoord xc = mPD->mReflowMargin.left + mPD->mExtraMargin.left;
    nscoord yc = mPD->mReflowMargin.top + mPD->mExtraMargin.top;

    
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

 PRBool
nsPageFrame::IsContainingBlock() const
{
  return PR_TRUE;
}

void 
nsPageFrame::ProcessSpecialCodes(const nsString& aStr, nsString& aNewStr)
{

  aNewStr = aStr;

  
  
  NS_NAMED_LITERAL_STRING(kDate, "&D");
  if (aStr.Find(kDate) != kNotFound) {
    if (mPD->mDateTimeStr != nsnull) {
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
    if (mPD->mDocTitle != nsnull) {
      aNewStr.ReplaceSubstring(kTitle.get(), mPD->mDocTitle);
    } else {
      aNewStr.ReplaceSubstring(kTitle.get(), EmptyString().get());
    }
  }

  NS_NAMED_LITERAL_STRING(kDocURL, "&U");
  if (aStr.Find(kDocURL) != kNotFound) {
    if (mPD->mDocURL != nsnull) {
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



nscoord nsPageFrame::GetXPosition(nsIRenderingContext& aRenderingContext, 
                                  const nsRect&        aRect, 
                                  PRInt32              aJust,
                                  const nsString&      aStr)
{
  nscoord width = nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                                aStr.get(), aStr.Length());

  nscoord x = aRect.x;
  switch (aJust) {
    case nsIPrintSettings::kJustLeft:
      x += mPD->mExtraMargin.left + mPD->mEdgePaperMargin.left;
      break;

    case nsIPrintSettings::kJustCenter:
      x += (aRect.width - width) / 2;
      break;

    case nsIPrintSettings::kJustRight:
      x += aRect.width - width - mPD->mExtraMargin.right - mPD->mEdgePaperMargin.right;
      break;
  } 

  return x;
}










void
nsPageFrame::DrawHeaderFooter(nsIRenderingContext& aRenderingContext,
                              nsHeaderFooterEnum   aHeaderFooter,
                              const nsString&      aStrLeft,
                              const nsString&      aStrCenter,
                              const nsString&      aStrRight,
                              const nsRect&        aRect,
                              nscoord              aAscent,
                              nscoord              aHeight)
{
  PRInt32 numStrs = 0;
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
nsPageFrame::DrawHeaderFooter(nsIRenderingContext& aRenderingContext,
                              nsHeaderFooterEnum   aHeaderFooter,
                              PRInt32              aJust,
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

    PRInt32 indx;
    PRInt32 textWidth = 0;
    const PRUnichar* text = str.get();

    PRInt32 len = (PRInt32)str.Length();
    if (len == 0) {
      return; 
    }
    
    if (nsLayoutUtils::BinarySearchForPosition(&aRenderingContext, text, 0, 0, 0, len,
                                PRInt32(contentWidth), indx, textWidth)) {
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
      y = aRect.y + mPD->mExtraMargin.top + mPD->mEdgePaperMargin.top;
    } else {
      y = aRect.YMost() - aHeight - mPD->mExtraMargin.bottom - mPD->mEdgePaperMargin.bottom;
    }

    
    aRenderingContext.PushState();
    aRenderingContext.SetColor(NS_RGB(0,0,0));
    aRenderingContext.SetClipRect(aRect, nsClipCombine_kIntersect);
    nsLayoutUtils::DrawString(this, &aRenderingContext, str.get(), str.Length(), nsPoint(x, y + aAscent));
    aRenderingContext.PopState();
  }
}

static void PaintPrintPreviewBackground(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                                        const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsPageFrame*>(aFrame)->PaintPrintPreviewBackground(*aCtx, aPt);
}

static void PaintPageContent(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                             const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsPageFrame*>(aFrame)->PaintPageContent(*aCtx, aDirtyRect, aPt);
}

static void PaintHeaderFooter(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                              const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsPageFrame*>(aFrame)->PaintHeaderFooter(*aCtx, aPt);
}


NS_IMETHODIMP
nsPageFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  nsDisplayListCollection set;
  nsresult rv;

  if (PresContext()->IsScreen()) {
    rv = set.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(this, ::PaintPrintPreviewBackground, "PrintPreviewBackground"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = set.BorderBackground()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(this, ::PaintPageContent, "PageContent"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (PresContext()->IsRootPaginatedDocument()) {
    rv = set.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(this, ::PaintHeaderFooter, "HeaderFooter"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  set.MoveTo(aLists);
  return NS_OK;
}


void
nsPageFrame::SetPageNumInfo(PRInt32 aPageNumber, PRInt32 aTotalPages) 
{ 
  mPageNum     = aPageNumber; 
  mTotNumPages = aTotalPages;
}


void
nsPageFrame::PaintPrintPreviewBackground(nsIRenderingContext& aRenderingContext,
                                         nsPoint aPt)
{
  
  aRenderingContext.SetColor(NS_RGB(255,255,255));
  
  
  
  nsRect rect(aPt, GetSize());
  rect.width  -= mPD->mShadowSize.width;
  rect.height -= mPD->mShadowSize.height;
  aRenderingContext.FillRect(rect);
  
  aRenderingContext.SetColor(NS_RGB(0,0,0));
  aRenderingContext.DrawRect(rect);

  if (mPD->mShadowSize.width > 0 && mPD->mShadowSize.height > 0) {
    aRenderingContext.SetColor(NS_RGB(51,51,51));
    nsRect r(aPt.x,aPt.y, mRect.width, mRect.height);
    nsRect shadowRect;
    shadowRect.x = r.x + r.width - mPD->mShadowSize.width;
    shadowRect.y = r.y + mPD->mShadowSize.height;
    shadowRect.width  = mPD->mShadowSize.width;
    shadowRect.height = r.height - mPD->mShadowSize.height;
    aRenderingContext.FillRect(shadowRect);

    shadowRect.x = r.x + mPD->mShadowSize.width;
    shadowRect.y = r.y + r.height - mPD->mShadowSize.height;
    shadowRect.width  = r.width - mPD->mShadowSize.width;
    shadowRect.height = mPD->mShadowSize.height;
    aRenderingContext.FillRect(shadowRect);
  }
}

void
nsPageFrame::PaintHeaderFooter(nsIRenderingContext& aRenderingContext,
                               nsPoint aPt)
{
  nsPresContext* pc = PresContext();

  if (!mPD->mPrintSettings) {
    if (pc->Type() == nsPresContext::eContext_PrintPreview || pc->IsDynamic())
      mPD->mPrintSettings = pc->GetPrintSettings();
    if (!mPD->mPrintSettings)
      return;
  }

  nsRect rect(aPt.x, aPt.y, mRect.width - mPD->mShadowSize.width,
              mRect.height - mPD->mShadowSize.height);

  aRenderingContext.SetColor(NS_RGB(0,0,0));

  
  nsCOMPtr<nsIFontMetrics> fontMet;
  pc->DeviceContext()->GetMetricsFor(*mPD->mHeadFootFont, nsnull,
                                     pc->GetUserFontSet(),
                                     *getter_AddRefs(fontMet));

  aRenderingContext.SetFont(fontMet);

  nscoord ascent = 0;
  nscoord visibleHeight = 0;
  if (fontMet) {
    fontMet->GetHeight(visibleHeight);
    fontMet->GetMaxAscent(ascent);
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
nsPageFrame::PaintPageContent(nsIRenderingContext& aRenderingContext,
                              const nsRect&        aDirtyRect,
                              nsPoint              aPt) {
  nsIFrame* pageContentFrame  = mFrames.FirstChild();
  nsRect rect = aDirtyRect;
  float scale = PresContext()->GetPageScale();
  aRenderingContext.PushState();
  nsPoint framePos = aPt + pageContentFrame->GetOffsetTo(this);
  aRenderingContext.Translate(framePos.x, framePos.y);
  
  
  rect -= framePos;
  aRenderingContext.Scale(scale, scale);
  rect.ScaleRoundOut(1.0f / scale);
  
  
  nsRect clipRect(nsPoint(0, 0), pageContentFrame->GetSize());
  
  
  nscoord expectedPageContentHeight = 
    NSToCoordCeil((GetSize().height - mPD->mReflowMargin.TopBottom()) / scale);
  if (clipRect.height > expectedPageContentHeight) {
    
    
    NS_ASSERTION(mPageNum > 0, "page num should be positive");
    
    
    
    
    clipRect.y = NSToCoordCeil((-pageContentFrame->GetRect().y + 
                                mPD->mReflowMargin.top) / scale);
    clipRect.height = expectedPageContentHeight;
    NS_ASSERTION(clipRect.y < pageContentFrame->GetSize().height,
                 "Should be clipping to region inside the page content bounds");
  }
  aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);

  nsRect backgroundRect = nsRect(nsPoint(0, 0), pageContentFrame->GetSize());
  nsCSSRendering::PaintBackground(PresContext(), aRenderingContext, this,
                                  rect, backgroundRect,
                                  nsCSSRendering::PAINTBG_SYNC_DECODE_IMAGES);

  nsLayoutUtils::PaintFrame(&aRenderingContext, pageContentFrame,
                            nsRegion(rect), NS_RGBA(0,0,0,0),
                            nsLayoutUtils::PAINT_SYNC_DECODE_IMAGES);

  aRenderingContext.PopState();
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
  nsLeafFrame(aContext), mHaveReflowed(PR_FALSE)
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

  
  
  mHaveReflowed = PR_TRUE;
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
