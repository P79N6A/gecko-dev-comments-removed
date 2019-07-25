



































#include "nsCOMPtr.h" 
#include "nsReadableUtils.h"
#include "nsSimplePageSequence.h"
#include "nsPresContext.h"
#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsIPrintSettings.h"
#include "nsPageFrame.h"
#include "nsStyleConsts.h"
#include "nsRegion.h"
#include "nsCSSFrameConstructor.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "mozilla/Preferences.h"


#include "nsDateTimeFormatCID.h"

#define OFFSET_NOT_SET -1


#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"

using namespace mozilla;

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
  mExtraMargin(0,0,0,0),
  mEdgePaperMargin(0,0,0,0),
  mPageContentXMost(0),
  mPageContentSize(0)
{
}

nsSharedPageData::~nsSharedPageData()
{
  nsMemory::Free(mDateTimeStr);
  delete mHeadFootFont;
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
  nscoord halfInch = PresContext()->CSSTwipsToAppUnits(NS_INCHES_TO_TWIPS(0.5));
  mMargin.SizeTo(halfInch, halfInch, halfInch, halfInch);

  
  mPageData = new nsSharedPageData();
  mPageData->mHeadFootFont = new nsFont(*PresContext()->GetDefaultFont(kGenericFont_serif));
  mPageData->mHeadFootFont->size = nsPresContext::CSSPointsToAppUnits(10);

  nsresult rv;
  mPageData->mPrintOptions = do_GetService(sPrintOptionsContractID, &rv);

  
  SetPageNumberFormat("pagenumber",  "%1$d", true);
  SetPageNumberFormat("pageofpages", "%1$d of %2$d", false);
}

nsSimplePageSequenceFrame::~nsSimplePageSequenceFrame()
{
  delete mPageData;
}

NS_QUERYFRAME_HEAD(nsSimplePageSequenceFrame)
  NS_QUERYFRAME_ENTRY(nsIPageSequenceFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)



void
nsSimplePageSequenceFrame::SetDesiredSize(nsHTMLReflowMetrics& aDesiredSize,
                                          const nsHTMLReflowState& aReflowState,
                                          nscoord aWidth,
                                          nscoord aHeight)
{
    
    
    
    
    aDesiredSize.width = NS_MAX(aReflowState.availableWidth,
                                nscoord(aWidth * PresContext()->GetPrintPreviewScale()));
    aDesiredSize.height = NS_MAX(aReflowState.ComputedHeight(),
                                 nscoord(aHeight * PresContext()->GetPrintPreviewScale()));
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
    
    SetDesiredSize(aDesiredSize, aReflowState, mSize.width, mSize.height);
    aDesiredSize.SetOverflowAreasToDesiredBounds();
    FinishAndStoreOverflow(&aDesiredSize);
    return NS_OK;
  }

  
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
    mMargin = aPresContext->CSSTwipsToAppUnits(marginTwips + unwriteableTwips);

    PRInt16 printType;
    mPageData->mPrintSettings->GetPrintRange(&printType);
    mPrintRangeType = printType;

    nsIntMargin edgeTwips;
    mPageData->mPrintSettings->GetEdgeInTwips(edgeTwips);

    
    PRInt32 inchInTwips = NS_INCHES_TO_INT_TWIPS(3.0);
    edgeTwips.top = NS_MIN(NS_MAX(edgeTwips.top, 0), inchInTwips);
    edgeTwips.bottom = NS_MIN(NS_MAX(edgeTwips.bottom, 0), inchInTwips);
    edgeTwips.left = NS_MIN(NS_MAX(edgeTwips.left, 0), inchInTwips);
    edgeTwips.right = NS_MIN(NS_MAX(edgeTwips.right, 0), inchInTwips);

    mPageData->mEdgePaperMargin =
      aPresContext->CSSTwipsToAppUnits(edgeTwips + unwriteableTwips);
  }

  
  
  
  

  nsSize pageSize = aPresContext->GetPageSize();

  mPageData->mReflowSize = pageSize;
  
  
  
  if (nsIPrintSettings::kRangeSelection == mPrintRangeType) {
    mPageData->mReflowSize.height = NS_UNCONSTRAINEDSIZE;
  }
  mPageData->mReflowMargin = mMargin;

  
  
  nscoord extraThreshold = NS_MAX(pageSize.width, pageSize.height)/10;
  PRInt32 gapInTwips = Preferences::GetInt("print.print_extra_margin");
  gapInTwips = NS_MAX(0, gapInTwips);

  nscoord extraGap = aPresContext->CSSTwipsToAppUnits(gapInTwips);
  extraGap = NS_MIN(extraGap, extraThreshold); 

  nsMargin extraMargin(0,0,0,0);
  if (aPresContext->IsScreen()) {
    extraMargin.SizeTo(extraGap, extraGap, extraGap, extraGap);
  }

  mPageData->mExtraMargin = extraMargin;

  
  
  
  nscoord y = 0;
  nscoord maxXMost = 0;

  nsSize availSize(pageSize.width + extraMargin.LeftRight(),
                   pageSize.height + extraMargin.TopBottom());

  
  nsHTMLReflowMetrics kidSize;
  for (nsIFrame* kidFrame = mFrames.FirstChild(); nsnull != kidFrame; ) {
    
    nsPageFrame * pf = static_cast<nsPageFrame*>(kidFrame);
    pf->SetSharedPageData(mPageData);

    
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                     availSize);
    nsReflowStatus  status;

    kidReflowState.SetComputedWidth(kidReflowState.availableWidth);
    
    PR_PL(("AV W: %d   H: %d\n", kidReflowState.availableWidth, kidReflowState.availableHeight));

    nsMargin pageCSSMargin = kidReflowState.mComputedMargin;
    y += pageCSSMargin.top;
    const nscoord x = pageCSSMargin.left;

    
    
    ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, x, y, 0, status);

    FinishReflowChild(kidFrame, aPresContext, nsnull, kidSize, x, y, 0);
    y += kidSize.height;
    y += pageCSSMargin.bottom;

    maxXMost = NS_MAX(maxXMost, x + kidSize.width + pageCSSMargin.right);

    
    nsIFrame* kidNextInFlow = kidFrame->GetNextInFlow();

    if (NS_FRAME_IS_FULLY_COMPLETE(status)) {
      NS_ASSERTION(!kidNextInFlow, "bad child flow list");
    } else if (!kidNextInFlow) {
      
      
      nsIFrame* continuingPage;
      nsresult rv = aPresContext->PresShell()->FrameConstructor()->
        CreateContinuingFrame(aPresContext, kidFrame, this, &continuingPage);
      if (NS_FAILED(rv)) {
        break;
      }

      
      mFrames.InsertFrame(nsnull, kidFrame, continuingPage);
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

  
  
  
  SetDesiredSize(aDesiredSize, aReflowState, maxXMost, y);

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aDesiredSize);

  
  
  mSize.width  = maxXMost;
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
nsSimplePageSequenceFrame::IsDoingPrintRange(bool* aDoing)
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
nsSimplePageSequenceFrame::SetPageNumberFormat(const char* aPropName, const char* aDefPropVal, bool aPageNumOnly)
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

  bool printEvenPages, printOddPages;
  mPageData->mPrintSettings->GetPrintOptions(nsIPrintSettings::kPrintEvenPages, &printEvenPages);
  mPageData->mPrintSettings->GetPrintOptions(nsIPrintSettings::kPrintOddPages, &printOddPages);

  
  nsDeviceContext *dc = PresContext()->DeviceContext();

  nsresult rv = NS_OK;

  
  mPrintThisPage = true;

  
  
  if (mDoingPageRange) {
    if (mPageNum < mFromPageNum) {
      mPrintThisPage = false;
    } else if (mPageNum > mToPageNum) {
      mPageNum++;
      mCurrentPageFrame = nsnull;
      return NS_OK;
    }
  }

  
  if (mPageNum & 0x1) {
    if (!printOddPages) {
      mPrintThisPage = false;  
    }
  } else {
    if (!printEvenPages) {
      mPrintThisPage = false;  
    }
  }
  
  if (nsIPrintSettings::kRangeSelection == mPrintRangeType) {
    mPrintThisPage = true;
  }

  if (mPrintThisPage) {
    
    
    
    
    
    
    bool    continuePrinting = true;
    nscoord width, height;
    width = PresContext()->GetPageSize().width;
    height = PresContext()->GetPageSize().height;
    height -= mMargin.top + mMargin.bottom;
    width  -= mMargin.left + mMargin.right;
    nscoord selectionY = height;
    nsIFrame* conFrame = mCurrentPageFrame->GetFirstPrincipalChild();
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

      PR_PL(("SeqFr::PrintNextPage -> %p PageNo: %d", pf, mPageNum));

      nsRefPtr<nsRenderingContext> renderingContext;
      dc->CreateRenderingContext(*getter_AddRefs(renderingContext));
      NS_ENSURE_TRUE(renderingContext, NS_ERROR_OUT_OF_MEMORY);

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
        continuePrinting = false;
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

static void PaintPageSequence(nsIFrame* aFrame, nsRenderingContext* aCtx,
                             const nsRect& aDirtyRect, nsPoint aPt)
{
  static_cast<nsSimplePageSequenceFrame*>(aFrame)->PaintPageSequence(*aCtx, aDirtyRect, aPt);
}


void
nsSimplePageSequenceFrame::PaintPageSequence(nsRenderingContext& aRenderingContext,
                                             const nsRect&        aDirtyRect,
                                             nsPoint              aPt) {
  nsRect rect = aDirtyRect;
  float scale = PresContext()->GetPrintPreviewScale();
  aRenderingContext.PushState();
  nsPoint framePos = aPt;
  aRenderingContext.Translate(framePos);
  rect -= framePos;
  aRenderingContext.Scale(scale, scale);
  rect.ScaleRoundOut(1.0f / scale);

  
  
  nsIFrame* child = GetFirstPrincipalChild();
  while (child) {
    nsPoint pt = child->GetPosition();
    
    aRenderingContext.PushState();
    aRenderingContext.Translate(pt);
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
        nsDisplayGeneric(aBuilder, this, ::PaintPageSequence, "PageSequence",
                         nsDisplayItem::TYPE_PAGE_SEQUENCE));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsIAtom*
nsSimplePageSequenceFrame::GetType() const
{
  return nsGkAtoms::sequenceFrame; 
}


void
nsSimplePageSequenceFrame::SetPageNumberFormat(PRUnichar * aFormatStr, bool aForPageNumOnly)
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
