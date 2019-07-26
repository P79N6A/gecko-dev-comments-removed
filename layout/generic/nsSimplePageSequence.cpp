



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
#include "nsSubDocumentFrame.h"
#include "nsStyleConsts.h"
#include "nsRegion.h"
#include "nsCSSFrameConstructor.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "mozilla/Preferences.h"
#include "nsHTMLCanvasFrame.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsICanvasRenderingContextInternal.h"
#include <algorithm>


#include "nsDateTimeFormatCID.h"

#define OFFSET_NOT_SET -1


#include "nsIPrintOptions.h"
#include "nsGfxCIID.h"
#include "nsIServiceManager.h"

using namespace mozilla;
using namespace mozilla::dom;

static const char sPrintOptionsContractID[] = "@mozilla.org/gfx/printsettings-service;1";



#include "prlog.h"
#ifdef PR_LOGGING 
PRLogModuleInfo *
GetLayoutPrintingLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("printing-layout");
  return sLog;
}
#define PR_PL(_p1)  PR_LOG(GetLayoutPrintingLog(), PR_LOG_DEBUG, _p1)
#else
#define PR_PL(_p1)
#endif



nsSharedPageData::nsSharedPageData() :
  mDateTimeStr(nullptr),
  mHeadFootFont(nullptr),
  mPageNumFormat(nullptr),
  mPageNumAndTotalsFormat(nullptr),
  mDocTitle(nullptr),
  mDocURL(nullptr),
  mReflowSize(0,0),
  mReflowMargin(0,0,0,0),
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
  mYSelOffset(0),
  mCalledBeginPage(false),
  mCurrentCanvasListSetup(false)
{
  nscoord halfInch = PresContext()->CSSTwipsToAppUnits(NS_INCHES_TO_TWIPS(0.5));
  mMargin.SizeTo(halfInch, halfInch, halfInch, halfInch);

  
  mPageData = new nsSharedPageData();
  mPageData->mHeadFootFont =
    new nsFont(*PresContext()->GetDefaultFont(kGenericFont_serif,
                                              aContext->GetStyleFont()->mLanguage));
  mPageData->mHeadFootFont->size = nsPresContext::CSSPointsToAppUnits(10);

  nsresult rv;
  mPageData->mPrintOptions = do_GetService(sPrintOptionsContractID, &rv);

  
  SetPageNumberFormat("pagenumber",  "%1$d", true);
  SetPageNumberFormat("pageofpages", "%1$d of %2$d", false);
}

nsSimplePageSequenceFrame::~nsSimplePageSequenceFrame()
{
  delete mPageData;
  ResetPrintCanvasList();
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
    
    
    
    
    aDesiredSize.width = std::max(aReflowState.availableWidth,
                                nscoord(aWidth * PresContext()->GetPrintPreviewScale()));
    aDesiredSize.height = std::max(aReflowState.ComputedHeight(),
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

    int16_t printType;
    mPageData->mPrintSettings->GetPrintRange(&printType);
    mPrintRangeType = printType;

    nsIntMargin edgeTwips;
    mPageData->mPrintSettings->GetEdgeInTwips(edgeTwips);

    
    int32_t inchInTwips = NS_INCHES_TO_INT_TWIPS(3.0);
    edgeTwips.top    = clamped(edgeTwips.top,    0, inchInTwips);
    edgeTwips.bottom = clamped(edgeTwips.bottom, 0, inchInTwips);
    edgeTwips.left   = clamped(edgeTwips.left,   0, inchInTwips);
    edgeTwips.right  = clamped(edgeTwips.right,  0, inchInTwips);

    mPageData->mEdgePaperMargin =
      aPresContext->CSSTwipsToAppUnits(edgeTwips + unwriteableTwips);
  }

  
  
  
  

  nsSize pageSize = aPresContext->GetPageSize();

  mPageData->mReflowSize = pageSize;
  
  
  
  if (nsIPrintSettings::kRangeSelection == mPrintRangeType) {
    mPageData->mReflowSize.height = NS_UNCONSTRAINEDSIZE;
  }
  mPageData->mReflowMargin = mMargin;

  
  
  
  nscoord y = 0;
  nscoord maxXMost = 0;

  
  nsHTMLReflowMetrics kidSize;
  for (nsIFrame* kidFrame = mFrames.FirstChild(); nullptr != kidFrame; ) {
    
    nsPageFrame * pf = static_cast<nsPageFrame*>(kidFrame);
    pf->SetSharedPageData(mPageData);

    
    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, kidFrame,
                                     pageSize);
    nsReflowStatus  status;

    kidReflowState.SetComputedWidth(kidReflowState.availableWidth);
    
    PR_PL(("AV W: %d   H: %d\n", kidReflowState.availableWidth, kidReflowState.availableHeight));

    nsMargin pageCSSMargin = kidReflowState.mComputedMargin;
    y += pageCSSMargin.top;
    const nscoord x = pageCSSMargin.left;

    
    
    ReflowChild(kidFrame, aPresContext, kidSize, kidReflowState, x, y, 0, status);

    FinishReflowChild(kidFrame, aPresContext, nullptr, kidSize, x, y, 0);
    y += kidSize.height;
    y += pageCSSMargin.bottom;

    maxXMost = std::max(maxXMost, x + kidSize.width + pageCSSMargin.right);

    
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

      
      mFrames.InsertFrame(nullptr, kidFrame, continuingPage);
    }

    
    kidFrame = kidFrame->GetNextSibling();
  }

  
  nsIFrame* page;
  int32_t pageTot = 0;
  for (page = mFrames.FirstChild(); page; page = page->GetNextSibling()) {
    pageTot++;
  }

  
  int32_t pageNum = 1;
  for (page = mFrames.FirstChild(); page; page = page->GetNextSibling()) {
    nsPageFrame * pf = static_cast<nsPageFrame*>(page);
    if (pf != nullptr) {
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
  if (NS_SUCCEEDED(mDateFormatter->FormatTime(nullptr ,
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
nsSimplePageSequenceFrame::GetCurrentPageNum(int32_t* aPageNum)
{
  NS_ENSURE_ARG_POINTER(aPageNum);

  *aPageNum = mPageNum;
  return NS_OK;
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::GetNumPages(int32_t* aNumPages)
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
nsSimplePageSequenceFrame::GetPrintRange(int32_t* aFromPage, int32_t* aToPage)
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
  if (uStr != nullptr) {
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
  aPrintSettings->GetPageRanges(mPageRanges);

  mDoingPageRange = nsIPrintSettings::kRangeSpecifiedPageRange == mPrintRangeType ||
                    nsIPrintSettings::kRangeSelection == mPrintRangeType;

  
  
  int32_t totalPages = mFrames.GetLength();

  if (mDoingPageRange) {
    if (mFromPageNum > totalPages) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  
  nsresult rv = NS_OK;

  
  aPresContext->SetIsRenderingOnlySelection(nsIPrintSettings::kRangeSelection == mPrintRangeType);


  if (mDoingPageRange) {
    
    
    nscoord height = aPresContext->GetPageSize().height;

    int32_t pageNum = 1;
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

void
GetPrintCanvasElementsInFrame(nsIFrame* aFrame, nsTArray<nsRefPtr<HTMLCanvasElement> >* aArr)
{
  if (!aFrame) {
    return;
  }
  for (nsIFrame::ChildListIterator childLists(aFrame);
    !childLists.IsDone(); childLists.Next()) {

    nsFrameList children = childLists.CurrentList();
    for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
      nsIFrame* child = e.get();

      
      nsHTMLCanvasFrame* canvasFrame = do_QueryFrame(child);

      
      if (canvasFrame) {
        HTMLCanvasElement* canvas =
          HTMLCanvasElement::FromContentOrNull(canvasFrame->GetContent());
        nsCOMPtr<nsIPrintCallback> printCallback;
        if (canvas &&
            NS_SUCCEEDED(canvas->GetMozPrintCallback(getter_AddRefs(printCallback))) &&
            printCallback) {
          aArr->AppendElement(canvas);
          continue;
        }
      }

      if (!child->GetFirstPrincipalChild()) {
        nsSubDocumentFrame* subdocumentFrame = do_QueryFrame(child);
        if (subdocumentFrame) {
          
          nsIFrame* root = subdocumentFrame->GetSubdocumentRootFrame();
          child = root;
        }
      }
      
      
      
      GetPrintCanvasElementsInFrame(child, aArr);
    }
  }
}

void
nsSimplePageSequenceFrame::DetermineWhetherToPrintPage()
{
  
  mPrintThisPage = true;
  bool printEvenPages, printOddPages;
  mPageData->mPrintSettings->GetPrintOptions(nsIPrintSettings::kPrintEvenPages, &printEvenPages);
  mPageData->mPrintSettings->GetPrintOptions(nsIPrintSettings::kPrintOddPages, &printOddPages);

  
  
  if (mDoingPageRange) {
    if (mPageNum < mFromPageNum) {
      mPrintThisPage = false;
    } else if (mPageNum > mToPageNum) {
      mPageNum++;
      mCurrentPageFrame = nullptr;
      mPrintThisPage = false;
      return;
    } else {
      int32_t length = mPageRanges.Length();
    
      
      if (length && (length % 2 == 0)) {
        mPrintThisPage = false;
      
        int32_t i;
        for (i = 0; i < length; i += 2) {          
          if (mPageRanges[i] <= mPageNum && mPageNum <= mPageRanges[i+1]) {
            mPrintThisPage = true;
            break;
          }
        }
      }
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
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::PrePrintNextPage(nsITimerCallback* aCallback, bool* aDone)
{
  if (!mCurrentPageFrame) {
    *aDone = true;
    return NS_ERROR_FAILURE;
  }
  
  DetermineWhetherToPrintPage();
  
  
  
  if (!mPrintThisPage || !PresContext()->IsRootPaginatedDocument()) {
    *aDone = true;
    return NS_OK;
  }

  
  
  if (!mCurrentCanvasListSetup) {
    mCurrentCanvasListSetup = true;
    GetPrintCanvasElementsInFrame(mCurrentPageFrame, &mCurrentCanvasList);

    if (mCurrentCanvasList.Length() != 0) {
      nsresult rv = NS_OK;

      
      nsDeviceContext *dc = PresContext()->DeviceContext();
      PR_PL(("\n"));
      PR_PL(("***************** BeginPage *****************\n"));
      rv = dc->BeginPage();
      NS_ENSURE_SUCCESS(rv, rv);

      mCalledBeginPage = true;
      
      nsRefPtr<nsRenderingContext> renderingContext;
      dc->CreateRenderingContext(*getter_AddRefs(renderingContext));
      NS_ENSURE_TRUE(renderingContext, NS_ERROR_OUT_OF_MEMORY);

      nsRefPtr<gfxASurface> renderingSurface =
          renderingContext->ThebesContext()->CurrentSurface();
      NS_ENSURE_TRUE(renderingSurface, NS_ERROR_OUT_OF_MEMORY);

      for (int32_t i = mCurrentCanvasList.Length() - 1; i >= 0 ; i--) {
        HTMLCanvasElement* canvas = mCurrentCanvasList[i];
        nsIntSize size = canvas->GetSize();

        nsRefPtr<gfxASurface> printSurface = renderingSurface->
           CreateSimilarSurface(
             gfxASurface::CONTENT_COLOR_ALPHA,
             size
           );

        if (!printSurface) {
          continue;
        }

        nsICanvasRenderingContextInternal* ctx = canvas->GetContextAtIndex(0);

        if (!ctx) {
          continue;
        }

          
        ctx->InitializeWithSurface(NULL, printSurface, size.width, size.height);

        
        nsWeakFrame weakFrame = this;
        canvas->DispatchPrintCallback(aCallback);
        NS_ENSURE_STATE(weakFrame.IsAlive());
      }
    }
  }
  uint32_t doneCounter = 0;
  for (int32_t i = mCurrentCanvasList.Length() - 1; i >= 0 ; i--) {
    HTMLCanvasElement* canvas = mCurrentCanvasList[i];

    if (canvas->IsPrintCallbackDone()) {
      doneCounter++;
    }
  }
  
  *aDone = doneCounter == mCurrentCanvasList.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::ResetPrintCanvasList()
{
  for (int32_t i = mCurrentCanvasList.Length() - 1; i >= 0 ; i--) {
    HTMLCanvasElement* canvas = mCurrentCanvasList[i];
    canvas->ResetPrintCallback();
  }

  mCurrentCanvasList.Clear();
  mCurrentCanvasListSetup = false; 
  return NS_OK;
} 

NS_IMETHODIMP
nsSimplePageSequenceFrame::PrintNextPage()
{
  
  
  
  
  
  
  
  
  
  

  if (!mCurrentPageFrame) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;

  DetermineWhetherToPrintPage();

  if (mPrintThisPage) {
    
    nsDeviceContext* dc = PresContext()->DeviceContext();

    
    
    
    
    
    
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

    int32_t printedPageNum = 1;
    while (continuePrinting) {
      if (PresContext()->IsRootPaginatedDocument()) {
        if (!mCalledBeginPage) {
          PR_PL(("\n"));
          PR_PL(("***************** BeginPage *****************\n"));
          rv = dc->BeginPage();
          NS_ENSURE_SUCCESS(rv, rv);
        } else {
          mCalledBeginPage = false;
        }
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

  ResetPrintCanvasList();

  mPageNum++;

  if (mCurrentPageFrame) {
    mCurrentPageFrame = mCurrentPageFrame->GetNextSibling();
  }
  
  return rv;
}

static gfx3DMatrix
ComputePageSequenceTransform(nsIFrame* aFrame, float aAppUnitsPerPixel)
{
  float scale = aFrame->PresContext()->GetPrintPreviewScale();
  return gfx3DMatrix::ScalingMatrix(scale, scale, 1);
}

NS_IMETHODIMP
nsSimplePageSequenceFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                            const nsRect&           aDirtyRect,
                                            const nsDisplayListSet& aLists)
{
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  nsDisplayList content;
  nsIFrame* child = GetFirstPrincipalChild();
  while (child) {
    rv = child->BuildDisplayListForStackingContext(aBuilder,
        child->GetVisualOverflowRectRelativeToSelf(), &content);
    NS_ENSURE_SUCCESS(rv, rv);
    child = child->GetNextSibling();
  }

  rv = content.AppendNewToTop(new (aBuilder)
      nsDisplayTransform(aBuilder, this, &content, ::ComputePageSequenceTransform));
  NS_ENSURE_SUCCESS(rv, rv);

  aLists.Content()->AppendToTop(&content);
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
  NS_ASSERTION(aFormatStr != nullptr, "Format string cannot be null!");
  NS_ASSERTION(mPageData != nullptr, "mPageData string cannot be null!");

  if (aForPageNumOnly) {
    if (mPageData->mPageNumFormat != nullptr) {
      nsMemory::Free(mPageData->mPageNumFormat);
    }
    mPageData->mPageNumFormat = aFormatStr;
  } else {
    if (mPageData->mPageNumAndTotalsFormat != nullptr) {
      nsMemory::Free(mPageData->mPageNumAndTotalsFormat);
    }
    mPageData->mPageNumAndTotalsFormat = aFormatStr;
  }
}


void
nsSimplePageSequenceFrame::SetDateTimeStr(PRUnichar * aDateTimeStr)
{ 
  NS_ASSERTION(aDateTimeStr != nullptr, "DateTime string cannot be null!");
  NS_ASSERTION(mPageData != nullptr, "mPageData string cannot be null!");

  if (mPageData->mDateTimeStr != nullptr) {
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
