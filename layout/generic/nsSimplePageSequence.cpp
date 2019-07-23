



































#include "nsCOMPtr.h" 
#include "nsReadableUtils.h"
#include "nsSimplePageSequence.h"
#include "nsPresContext.h"
#include "gfxContext.h"
#include "nsIRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsIDeviceContext.h"
#include "nsIPresShell.h"
#include "nsIFontMetrics.h"
#include "nsIPrintSettings.h"
#include "nsPageFrame.h"
#include "nsStyleConsts.h"
#include "nsRegion.h"
#include "nsCSSFrameConstructor.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"


#include "nsDateTimeFormatCID.h"

#define OFFSET_NOT_SET -1


#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"

static const char sPrintOptionsContractID[] = "@mozilla.org/gfx/printsettings-service;1";



#include "prlog.h"
#ifdef PR_LOGGING 
PRLogModuleInfo * kLayoutPrintingLogMod = PR_NewLogModule("printing-layout");
#define PR_PL(_p1)  PR_LOG(kLayoutPrintingLogMod, PR_LOG_DEBUG, _p1)
#else
#define PR_PL(_p1)
#endif



nsSharedPageData::nsSharedPageData() :
  mDateTimeStr(nsnull),
  mHeadFootFont(nsnull),
  mPageNumFormat(nsnull),
  mPageNumAndTotalsFormat(nsnull),
  mDocTitle(nsnull),
  mDocURL(nsnull),
  mReflowSize(0,0),
  mReflowMargin(0,0,0,0),
  mShadowSize(0,0),
  mExtraMargin(0,0,0,0),
  mEdgePaperMargin(0,0,0,0),
  mPageContentXMost(0),
  mPageContentSize(0)
{
}

nsSharedPageData::~nsSharedPageData()
{
  nsMemory::Free(mDateTimeStr);
  if (mHeadFootFont) delete mHeadFootFont;
  nsMemory::Free(mPageNumFormat);
  nsMemory::Free(mPageNumAndTotalsFormat);
  if (mDocTitle) nsMemory::Free(mDocTitle);
  if (mDocURL) nsMemory::Free(mDocURL);
}

nsIFrame*
NS_NewSimplePageSequenceFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSimplePageSequenceFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSimplePageSequenceFrame)

nsSimplePageSequenceFrame::nsSimplePageSequenceFrame(nsStyleContext* aContext) :
  nsContainerFrame(aContext),
  mTotalPages(-1),
  mSelectionHeight(-1),
  mYSelOffset(0)
{
  nscoord halfInch = PresContext()->TwipsToAppUnits(NS_INCHES_TO_TWIPS(0.5));
  mMargin.SizeTo(halfInch, halfInch, halfInch, halfInch);

  
  mPageData = new nsSharedPageData();
  mPageData->mHeadFootFont = new nsFont(*PresContext()->GetDefaultFont(kGenericFont_serif));
  mPageData->mHeadFootFont->size = PresContext()->PointsToAppUnits(10);

  nsresult rv;
  mPageData->mPrintOptions = do_GetService(sPrintOptionsContractID, &rv);

  
  SetPageNumberFormat("pagenumber",  "%1$d", PR_TRUE);
  SetPageNumberFormat("pageofpages", "%1$d of %2$d", PR_FALSE);
}

nsSimplePageSequenceFrame::~nsSimplePageSequenceFrame()
{
  if (mPageData) delete mPageData;
}

NS_QUERYFRAME_HEAD(nsSimplePageSequenceFrame)
  NS_QUERYFRAME_ENTRY(nsIPageSequenceFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)




nsresult
nsSimplePageSequenceFrame::CreateContinuingPageFrame(nsPresContext* aPresContext,
                                                     nsIFrame*       aPageFrame,
                                                     nsIFrame**      aContinuingPage)
{
  
  return aPresContext->PresShell()->FrameConstructor()->
    CreateContinuingFrame(aPresContext, aPageFrame, this, aContinuingPage);
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::Reflow(nsPresContext*          aPresContext,
                                  nsHTMLReflowMetrics&     aDesiredSize,
                                  const nsHTMLReflowState& aReflowState,
                                  nsReflowStatus&          aStatus)
{
  NS_PRECONDITION(aPresContext->IsRootPaginatedDocument(),
                  "A Page Sequence is only for real pages");
  DO_GLOBAL_REFLOW_COUNT("nsSimplePageSequenceFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE_REFLOW_IN("nsSimplePageSequenceFrame::Reflow");

  aStatus = NS_FRAME_COMPLETE;  

  
  
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    aDesiredSize.height  = mSize.height * PresContext()->GetPrintPreviewScale();
    aDesiredSize.width   = mSize.width * PresContext()->GetPrintPreviewScale();
    aDesiredSize.mOverflowArea = nsRect(0, 0, aDesiredSize.width,
                                        aDesiredSize.height);
    FinishAndStoreOverflow(&aDesiredSize);
    return NS_OK;
  }

  PRBool isPrintPreview =
    aPresContext->Type() == nsPresContext::eContext_PrintPreview;

  
  if (!mPageData->mPrintSettings &&
      aPresContext->Medium() == nsGkAtoms::print) {
      mPageData->mPrintSettings = aPresContext->GetPrintSettings();
  }

  
  if (mPageData->mPrintSettings) {
    nsIntMargin unwriteableTwips;
    mPageData->mPrintSettings->GetUnwriteableMarginInTwips(unwriteableTwips);
    NS_ASSERTION(unwriteableTwips.left  >= 0 && unwriteableTwips.top >= 0 &&
                 unwriteableTwips.right >= 0 && unwriteableTwips.bottom >= 0,
                 "Unwriteable twips should be non-negative");

    nsIntMargin marginTwips;
    mPageData->mPrintSettings->GetMarginInTwips(marginTwips);
    mMargin = aPresContext->TwipsToAppUnits(marginTwips + unwriteableTwips);

    PRInt16 printType;
    mPageData->mPrintSettings->GetPrintRange(&printType);
    mPrintRangeType = printType;

    nsIntMargin edgeTwips;
    mPageData->mPrintSettings->GetEdgeInTwips(edgeTwips);

    
    PRInt32 inchInTwips = NS_INCHES_TO_TWIPS(3.0);
    edgeTwips.top = PR_MIN(PR_MAX(edgeTwips.top, 0), inchInTwips);
    edgeTwips.bottom = PR_MIN(PR_MAX(edgeTwips.bottom, 0), inchInTwips);
    edgeTwips.left = PR_MIN(PR_MAX(edgeTwips.left, 0), inchInTwips);
    edgeTwips.right = PR_MIN(PR_MAX(edgeTwips.right, 0), inchInTwips);

    mPageData->mEdgePaperMargin =
      aPresContext->TwipsToAppUnits(edgeTwips + unwriteableTwips);
  }

  
  
  
  

  nsSize pageSize = aPresContext->GetPageSize();

  mPageData->mReflowSize = pageSize;
  
  
  
  if (nsIPrintSettings::kRangeSelection == mPrintRangeType) {
    mPageData->mReflowSize.height = NS_UNCONSTRAINEDSIZE;
  }
  mPageData->mReflowMargin = mMargin;

  
  
  nscoord extraThreshold = PR_MAX(pageSize.width, pageSize.height)/10;
  PRInt32 gapInTwips = nsContentUtils::GetIntPref("print.print_extra_margin");
  gapInTwips = PR_MAX(0, gapInTwips);

  nscoord extraGap = aPresContext->TwipsToAppUnits(gapInTwips);
  extraGap = PR_MIN(extraGap, extraThreshold); 

  nscoord  deadSpaceGap = 0;
  if (isPrintPreview) {
    GetDeadSpaceValue(&gapInTwips);
    deadSpaceGap = aPresContext->TwipsToAppUnits(gapInTwips);
  }

  nsMargin extraMargin(0,0,0,0);
  nsSize   shadowSize(0,0);
  if (aPresContext->IsScreen()) {
    extraMargin.SizeTo(extraGap, extraGap, extraGap, extraGap);
    nscoord fourPixels = nsPresContext::CSSPixelsToAppUnits(4);
    shadowSize.SizeTo(fourPixels, fourPixels);
  }

  mPageData->mShadowSize      = shadowSize;
  mPageData->mExtraMargin     = extraMargin;

  const nscoord x = deadSpaceGap;
  nscoord y = deadSpaceGap;

  nsSize availSize(pageSize.width + shadowSize.width + extraMargin.LeftRight(),
                   pageSize.height + shadowSize.height +
                   extraMargin.TopBottom());

  
  nsHTMLReflowMetrics kidSize;
  for (nsIFrame* kidFrame = mFrames.FirstChild(); nsnull != kidFrame; ) {
    
    nsPageFrame * pf = static_cast<nsPageFrame*>(kidFrame);
    pf->SetSharedPageData(mPageData);

    
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                     availSize);
    nsReflowStatus  status;

    kidReflowState.SetComputedWidth(kidReflowState.availableWidth);
    
    PR_PL(("AV W: %d   H: %d\n", kidReflowState.availableWidth, kidReflowState.availableHeight));

    
    
    ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, x, y, 0, status);

    FinishReflowChild(kidFrame, aPresContext, nsnull, kidSize, x, y, 0);
    y += kidSize.height;

    
    y += deadSpaceGap;

    
    nsIFrame* kidNextInFlow = kidFrame->GetNextInFlow();

    if (NS_FRAME_IS_FULLY_COMPLETE(status)) {
      NS_ASSERTION(nsnull == kidNextInFlow, "bad child flow list");
    } else if (nsnull == kidNextInFlow) {
      
      
      nsIFrame* continuingPage;
      nsresult rv = CreateContinuingPageFrame(aPresContext, kidFrame,
                                              &continuingPage);
      if (NS_FAILED(rv)) {
        break;
      }

      
      kidFrame->SetNextSibling(continuingPage);
    }

    
    kidFrame = kidFrame->GetNextSibling();
  }

  
  nsIFrame* page;
  PRInt32 pageTot = 0;
  for (page = mFrames.FirstChild(); page; page = page->GetNextSibling()) {
    pageTot++;
  }

  
  PRInt32 pageNum = 1;
  for (page = mFrames.FirstChild(); page; page = page->GetNextSibling()) {
    nsPageFrame * pf = static_cast<nsPageFrame*>(page);
    if (pf != nsnull) {
      pf->SetPageNumInfo(pageNum, pageTot);
    }
    pageNum++;
  }

  
  if (!mDateFormatter)
    mDateFormatter = do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID);

  NS_ENSURE_TRUE(mDateFormatter, NS_ERROR_FAILURE);

  nsAutoString formattedDateString;
  time_t ltime;
  time( &ltime );
  if (NS_SUCCEEDED(mDateFormatter->FormatTime(nsnull ,
                                              kDateFormatShort,
                                              kTimeFormatNoSeconds,
                                              ltime,
                                              formattedDateString))) {
    PRUnichar * uStr = ToNewUnicode(formattedDateString);
    SetDateTimeStr(uStr); 
  }

  
  
  
  nscoord w = (x + availSize.width + deadSpaceGap);
  aDesiredSize.height  = y * PresContext()->GetPrintPreviewScale(); 
  aDesiredSize.width   = w * PresContext()->GetPrintPreviewScale();

  aDesiredSize.mOverflowArea = nsRect(0, 0, aDesiredSize.width,
                                      aDesiredSize.height);
  FinishAndStoreOverflow(&aDesiredSize);

  
  
  mSize.width  = w;
  mSize.height = y;

  NS_FRAME_TRACE_REFLOW_OUT("nsSimplePageSequeceFrame::Reflow", aStatus);
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}



#ifdef DEBUG
NS_IMETHODIMP
nsSimplePageSequenceFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("SimplePageSequence"), aResult);
}
#endif




NS_IMETHODIMP
nsSimplePageSequenceFrame::GetCurrentPageNum(PRInt32* aPageNum)
{
  NS_ENSURE_ARG_POINTER(aPageNum);

  *aPageNum = mPageNum;
  return NS_OK;
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::GetNumPages(PRInt32* aNumPages)
{
  NS_ENSURE_ARG_POINTER(aNumPages);

  *aNumPages = mTotalPages;
  return NS_OK;
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::IsDoingPrintRange(PRBool* aDoing)
{
  NS_ENSURE_ARG_POINTER(aDoing);

  *aDoing = mDoingPageRange;
  return NS_OK;
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::GetPrintRange(PRInt32* aFromPage, PRInt32* aToPage)
{
  NS_ENSURE_ARG_POINTER(aFromPage);
  NS_ENSURE_ARG_POINTER(aToPage);

  *aFromPage = mFromPageNum;
  *aToPage   = mToPageNum;
  return NS_OK;
}


void 
nsSimplePageSequenceFrame::SetPageNumberFormat(const char* aPropName, const char* aDefPropVal, PRBool aPageNumOnly)
{
  
  nsXPIDLString pageNumberFormat;
  
  nsresult rv =
    nsContentUtils::GetLocalizedString(nsContentUtils::ePRINTING_PROPERTIES,
                                       aPropName, pageNumberFormat);
  if (NS_FAILED(rv)) { 
    pageNumberFormat.AssignASCII(aDefPropVal);
  }

  
  PRUnichar* uStr = ToNewUnicode(pageNumberFormat);
  if (uStr != nsnull) {
    SetPageNumberFormat(uStr, aPageNumOnly); 
  }

}

NS_IMETHODIMP
nsSimplePageSequenceFrame::StartPrint(nsPresContext*   aPresContext,
                                      nsIPrintSettings* aPrintSettings,
                                      PRUnichar*        aDocTitle,
                                      PRUnichar*        aDocURL)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  NS_ENSURE_ARG_POINTER(aPrintSettings);

  if (!mPageData->mPrintSettings) {
    mPageData->mPrintSettings = aPrintSettings;
  }

  
  if (aDocTitle) mPageData->mDocTitle = aDocTitle;
  if (aDocURL) mPageData->mDocURL   = aDocURL;

  aPrintSettings->GetStartPageRange(&mFromPageNum);
  aPrintSettings->GetEndPageRange(&mToPageNum);

  mDoingPageRange = nsIPrintSettings::kRangeSpecifiedPageRange == mPrintRangeType ||
                    nsIPrintSettings::kRangeSelection == mPrintRangeType;

  
  
  PRInt32 totalPages = mFrames.GetLength();

  if (mDoingPageRange) {
    if (mFromPageNum > totalPages) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  
  nsresult rv = NS_OK;

  
  aPresContext->SetIsRenderingOnlySelection(nsIPrintSettings::kRangeSelection == mPrintRangeType);


  if (mDoingPageRange) {
    
    
    nscoord height = aPresContext->GetPageSize().height;

    PRInt32 pageNum = 1;
    nscoord y = 0;

    for (nsIFrame* page = mFrames.FirstChild(); page;
         page = page->GetNextSibling()) {
      if (pageNum >= mFromPageNum && pageNum <= mToPageNum) {
        nsRect rect = page->GetRect();
        rect.y = y;
        rect.height = height;
        page->SetRect(rect);
        y += rect.height + mMargin.top + mMargin.bottom;
      }
      pageNum++;
    }

    
    if (nsIPrintSettings::kRangeSelection != mPrintRangeType) {
      totalPages = pageNum - 1;
    }
  }

  mPageNum          = 1;
  mCurrentPageFrame = mFrames.FirstChild();

  if (mTotalPages == -1) {
    mTotalPages = totalPages;
  }

  return rv;
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::PrintNextPage()
{
  
  
  
  
  
  
  
  
  
  

  if (mCurrentPageFrame == nsnull) {
    return NS_ERROR_FAILURE;
  }

  PRBool printEvenPages, printOddPages;
  mPageData->mPrintSettings->GetPrintOptions(nsIPrintSettings::kPrintEvenPages, &printEvenPages);
  mPageData->mPrintSettings->GetPrintOptions(nsIPrintSettings::kPrintOddPages, &printOddPages);

  
  nsIDeviceContext *dc = PresContext()->DeviceContext();

  nsresult rv = NS_OK;

  
  mPrintThisPage = PR_TRUE;

  
  
  if (mDoingPageRange) {
    if (mPageNum < mFromPageNum) {
      mPrintThisPage = PR_FALSE;
    } else if (mPageNum > mToPageNum) {
      mPageNum++;
      mCurrentPageFrame = nsnull;
      return NS_OK;
    }
  }

  
  if (mPageNum & 0x1) {
    if (!printOddPages) {
      mPrintThisPage = PR_FALSE;  
    }
  } else {
    if (!printEvenPages) {
      mPrintThisPage = PR_FALSE;  
    }
  }
  
  if (nsIPrintSettings::kRangeSelection == mPrintRangeType) {
    mPrintThisPage = PR_TRUE;
  }

  if (mPrintThisPage) {
    
    
    
    
    
    
    PRBool  continuePrinting = PR_TRUE;
    nscoord width, height;
    width = PresContext()->GetPageSize().width;
    height = PresContext()->GetPageSize().height;
    height -= mMargin.top + mMargin.bottom;
    width  -= mMargin.left + mMargin.right;
    nscoord selectionY = height;
    nsIFrame* conFrame = mCurrentPageFrame->GetFirstChild(nsnull);
    if (mSelectionHeight >= 0) {
      conFrame->SetPosition(conFrame->GetPosition() + nsPoint(0, -mYSelOffset));
      nsContainerFrame::PositionChildViews(conFrame);
    }

    
    nsPageFrame * pf = static_cast<nsPageFrame*>(mCurrentPageFrame);
    pf->SetPageNumInfo(mPageNum, mTotalPages);
    pf->SetSharedPageData(mPageData);

    PRInt32 printedPageNum = 1;
    while (continuePrinting) {
      if (PresContext()->IsRootPaginatedDocument()) {
        PR_PL(("\n"));
        PR_PL(("***************** BeginPage *****************\n"));
        rv = dc->BeginPage();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      PR_PL(("SeqFr::Paint -> %p PageNo: %d", pf, mPageNum));

      nsCOMPtr<nsIRenderingContext> renderingContext;
      PresContext()->PresShell()->
              CreateRenderingContext(mCurrentPageFrame,
                                     getter_AddRefs(renderingContext));

#if defined(XP_UNIX) && !defined(XP_MACOSX)
      
      PRInt32 orientation;
      mPageData->mPrintSettings->GetOrientation(&orientation);
      if (nsIPrintSettings::kLandscapeOrientation == orientation) {
        
        float offset = POINTS_PER_INCH_FLOAT *
           (mCurrentPageFrame->GetSize().height / float(dc->AppUnitsPerInch()));
        renderingContext->ThebesContext()->Translate(gfxPoint(offset, 0));
        renderingContext->ThebesContext()->Rotate(M_PI/2);
      }
#endif 

      nsRect drawingRect(nsPoint(0, 0),
                         mCurrentPageFrame->GetSize());
      nsRegion drawingRegion(drawingRect);
      nsLayoutUtils::PaintFrame(renderingContext, mCurrentPageFrame,
                                drawingRegion, NS_RGBA(0,0,0,0),
                                nsLayoutUtils::PAINT_SYNC_DECODE_IMAGES);

      if (mSelectionHeight >= 0 && selectionY < mSelectionHeight) {
        selectionY += height;
        printedPageNum++;
        pf->SetPageNumInfo(printedPageNum, mTotalPages);
        conFrame->SetPosition(conFrame->GetPosition() + nsPoint(0, -height));
        nsContainerFrame::PositionChildViews(conFrame);

        PR_PL(("***************** End Page (PrintNextPage) *****************\n"));
        rv = dc->EndPage();
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        continuePrinting = PR_FALSE;
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::DoPageEnd()
{
  nsresult rv = NS_OK;
  if (PresContext()->IsRootPaginatedDocument() && mPrintThisPage) {
    PR_PL(("***************** End Page (DoPageEnd) *****************\n"));
    rv = PresContext()->DeviceContext()->EndPage();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mPageNum++;

  if (mCurrentPageFrame) {
    mCurrentPageFrame = mCurrentPageFrame->GetNextSibling();
  }
  
  return rv;
}

static void PaintPageSequence(nsIFrame* aFrame, nsIRenderingContext* aCtx,
                             const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsSimplePageSequenceFrame*>(aFrame)->PaintPageSequence(*aCtx, aDirtyRect, aPt);
}


void
nsSimplePageSequenceFrame::PaintPageSequence(nsIRenderingContext& aRenderingContext,
                                             const nsRect&        aDirtyRect,
                                             nsPoint              aPt) {
  nsRect rect = aDirtyRect;
  float scale = PresContext()->GetPrintPreviewScale();
  aRenderingContext.PushState();
  nsPoint framePos = aPt;
  aRenderingContext.Translate(framePos.x, framePos.y);
  rect -= framePos;
  aRenderingContext.Scale(scale, scale);
  rect.ScaleRoundOut(1.0f / scale);

  
  
  nsIFrame* child = GetFirstChild(nsnull);
  while (child) {
    nsPoint pt = child->GetPosition();
    
    aRenderingContext.PushState();
    aRenderingContext.Translate(pt.x, pt.y);
    nsLayoutUtils::PaintFrame(&aRenderingContext, child,
                              nsRegion(rect - pt), NS_RGBA(0,0,0,0),
                              nsLayoutUtils::PAINT_SYNC_DECODE_IMAGES);
    aRenderingContext.PopState();
    child = child->GetNextSibling();
  }

  aRenderingContext.PopState();
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(this, ::PaintPageSequence, "PageSequence"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsIAtom*
nsSimplePageSequenceFrame::GetType() const
{
  return nsGkAtoms::sequenceFrame; 
}


void
nsSimplePageSequenceFrame::SetPageNumberFormat(PRUnichar * aFormatStr, PRBool aForPageNumOnly)
{ 
  NS_ASSERTION(aFormatStr != nsnull, "Format string cannot be null!");
  NS_ASSERTION(mPageData != nsnull, "mPageData string cannot be null!");

  if (aForPageNumOnly) {
    if (mPageData->mPageNumFormat != nsnull) {
      nsMemory::Free(mPageData->mPageNumFormat);
    }
    mPageData->mPageNumFormat = aFormatStr;
  } else {
    if (mPageData->mPageNumAndTotalsFormat != nsnull) {
      nsMemory::Free(mPageData->mPageNumAndTotalsFormat);
    }
    mPageData->mPageNumAndTotalsFormat = aFormatStr;
  }
}


void
nsSimplePageSequenceFrame::SetDateTimeStr(PRUnichar * aDateTimeStr)
{ 
  NS_ASSERTION(aDateTimeStr != nsnull, "DateTime string cannot be null!");
  NS_ASSERTION(mPageData != nsnull, "mPageData string cannot be null!");

  if (mPageData->mDateTimeStr != nsnull) {
    nsMemory::Free(mPageData->mDateTimeStr);
  }
  mPageData->mDateTimeStr = aDateTimeStr;
}






NS_IMETHODIMP
nsSimplePageSequenceFrame::GetSTFPercent(float& aSTFPercent)
{
  NS_ENSURE_TRUE(mPageData, NS_ERROR_UNEXPECTED);
  aSTFPercent = 1.0f;
  if (mPageData && (mPageData->mPageContentXMost > mPageData->mPageContentSize)) {
    aSTFPercent = float(mPageData->mPageContentSize) / float(mPageData->mPageContentXMost);
  }
  return NS_OK;
}
