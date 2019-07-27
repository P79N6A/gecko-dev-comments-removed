




#include "nsPageFrame.h"

#include "mozilla/gfx/2D.h"
#include "nsDeviceContext.h"
#include "nsFontMetrics.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsPageContentFrame.h"
#include "nsDisplayList.h"
#include "nsLayoutUtils.h" 
#include "nsSimplePageSequenceFrame.h" 
#include "nsTextFormatter.h" 
#include "nsBidiUtils.h"
#include "nsIPrintSettings.h"

#include "prlog.h"
#ifdef PR_LOGGING 
extern PRLogModuleInfo *GetLayoutPrintingLog();
#define PR_PL(_p1)  PR_LOG(GetLayoutPrintingLog(), PR_LOG_DEBUG, _p1)
#else
#define PR_PL(_p1)
#endif

using namespace mozilla;
using namespace mozilla::gfx;

nsPageFrame*
NS_NewPageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPageFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPageFrame)

NS_QUERYFRAME_HEAD(nsPageFrame)
  NS_QUERYFRAME_ENTRY(nsPageFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

nsPageFrame::nsPageFrame(nsStyleContext* aContext)
: nsContainerFrame(aContext)
{
}

nsPageFrame::~nsPageFrame()
{
}

void
nsPageFrame::Reflow(nsPresContext*           aPresContext,
                                  nsHTMLReflowMetrics&     aDesiredSize,
                                  const nsHTMLReflowState& aReflowState,
                                  nsReflowStatus&          aStatus)
{
  MarkInReflow();
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
      aDesiredSize.ClearSize();
      NS_WARNING("Reflow aborted; no space for content");
      return;
    }

    nsHTMLReflowState kidReflowState(aPresContext, aReflowState, frame,
                                     LogicalSize(frame->GetWritingMode(),
                                                 maxSize));
    kidReflowState.mFlags.mIsTopOfPage = true;
    kidReflowState.mFlags.mTableIsSplittable = true;

    
    
    nsMargin pageContentMargin;
    const nsStyleSides& marginStyle = kidReflowState.mStyleMargin->mMargin;
    NS_FOR_CSS_SIDES(side) {
      if (marginStyle.GetUnit(side) == eStyleUnit_Auto) {
        pageContentMargin.Side(side) = mPD->mReflowMargin.Side(side);
      } else {
        pageContentMargin.Side(side) = kidReflowState.ComputedPhysicalMargin().Side(side);
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

    
    FinishReflowChild(frame, aPresContext, aDesiredSize, &kidReflowState, xc, yc, 0);

    NS_ASSERTION(!NS_FRAME_IS_FULLY_COMPLETE(aStatus) ||
                 !frame->GetNextInFlow(), "bad child flow list");
  }
  PR_PL(("PageFrame::Reflow %p ", this));
  PR_PL(("[%d,%d][%d,%d]\n", aDesiredSize.Width(), aDesiredSize.Height(),
         aReflowState.AvailableWidth(), aReflowState.AvailableHeight()));

  
  WritingMode wm = aReflowState.GetWritingMode();
  aDesiredSize.ISize(wm) = aReflowState.AvailableISize();
  if (aReflowState.AvailableBSize() != NS_UNCONSTRAINEDSIZE) {
    aDesiredSize.BSize(wm) = aReflowState.AvailableBSize();
  }

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aDesiredSize);

  PR_PL(("PageFrame::Reflow %p ", this));
  PR_PL(("[%d,%d]\n", aReflowState.AvailableWidth(), aReflowState.AvailableHeight()));

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsIAtom*
nsPageFrame::GetType() const
{
  return nsGkAtoms::pageFrame; 
}

#ifdef DEBUG_FRAME_DUMP
nsresult
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
    aNewStr.ReplaceSubstring(kDate.get(), mPD->mDateTimeStr.get());
  }

  
  
  
  
  
  NS_NAMED_LITERAL_STRING(kPageAndTotal, "&PT");
  if (aStr.Find(kPageAndTotal) != kNotFound) {
    char16_t * uStr = nsTextFormatter::smprintf(mPD->mPageNumAndTotalsFormat.get(), mPageNum, mTotNumPages);
    aNewStr.ReplaceSubstring(kPageAndTotal.get(), uStr);
    free(uStr);
  }

  
  
  NS_NAMED_LITERAL_STRING(kPage, "&P");
  if (aStr.Find(kPage) != kNotFound) {
    char16_t * uStr = nsTextFormatter::smprintf(mPD->mPageNumFormat.get(), mPageNum);
    aNewStr.ReplaceSubstring(kPage.get(), uStr);
    free(uStr);
  }

  NS_NAMED_LITERAL_STRING(kTitle, "&T");
  if (aStr.Find(kTitle) != kNotFound) {
    aNewStr.ReplaceSubstring(kTitle.get(), mPD->mDocTitle.get());
  }

  NS_NAMED_LITERAL_STRING(kDocURL, "&U");
  if (aStr.Find(kDocURL) != kNotFound) {
    aNewStr.ReplaceSubstring(kDocURL.get(), mPD->mDocURL.get());
  }

  NS_NAMED_LITERAL_STRING(kPageTotal, "&L");
  if (aStr.Find(kPageTotal) != kNotFound) {
    char16_t * uStr = nsTextFormatter::smprintf(mPD->mPageNumFormat.get(), mTotNumPages);
    aNewStr.ReplaceSubstring(kPageTotal.get(), uStr);
    free(uStr);
  }
}



nscoord nsPageFrame::GetXPosition(nsRenderingContext& aRenderingContext,
                                  nsFontMetrics&       aFontMetrics,
                                  const nsRect&        aRect, 
                                  int32_t              aJust,
                                  const nsString&      aStr)
{
  nscoord width = nsLayoutUtils::AppUnitWidthOfStringBidi(aStr, this,
                                                          aFontMetrics,
                                                          aRenderingContext);
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
                              nsFontMetrics&       aFontMetrics,
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
    DrawHeaderFooter(aRenderingContext, aFontMetrics, aHeaderFooter,
                     nsIPrintSettings::kJustLeft, aStrLeft, aRect, aAscent,
                     aHeight, strSpace);
  }
  if (!aStrCenter.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aFontMetrics, aHeaderFooter,
                     nsIPrintSettings::kJustCenter, aStrCenter, aRect, aAscent,
                     aHeight, strSpace);
  }
  if (!aStrRight.IsEmpty()) {
    DrawHeaderFooter(aRenderingContext, aFontMetrics, aHeaderFooter,
                     nsIPrintSettings::kJustRight, aStrRight, aRect, aAscent,
                     aHeight, strSpace);
  }
}










void
nsPageFrame::DrawHeaderFooter(nsRenderingContext& aRenderingContext,
                              nsFontMetrics&       aFontMetrics,
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
    const char16_t* text = str.get();

    int32_t len = (int32_t)str.Length();
    if (len == 0) {
      return; 
    }
    
    if (nsLayoutUtils::BinarySearchForPosition(&aRenderingContext, aFontMetrics,
                                               text, 0, 0, 0, len,
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

    
    nscoord x = GetXPosition(aRenderingContext, aFontMetrics, aRect, aJust, str);
    nscoord y;
    if (aHeaderFooter == eHeader) {
      y = aRect.y + mPD->mEdgePaperMargin.top;
    } else {
      y = aRect.YMost() - aHeight - mPD->mEdgePaperMargin.bottom;
    }

    DrawTarget* drawTarget = aRenderingContext.GetDrawTarget();
    gfxContext* gfx = aRenderingContext.ThebesContext();

    
    gfx->Save();
    gfx->Clip(NSRectToSnappedRect(aRect, PresContext()->AppUnitsPerDevPixel(),
                                  *drawTarget));
    aRenderingContext.ThebesContext()->SetColor(NS_RGB(0,0,0));
    nsLayoutUtils::DrawString(this, aFontMetrics, &aRenderingContext,
                              str.get(), str.Length(),
                              nsPoint(x, y + aAscent));
    gfx->Restore();
  }
}








static void
PruneDisplayListForExtraPage(nsDisplayListBuilder* aBuilder,
                             nsPageFrame* aPage, nsIFrame* aExtraPage,
                             nsDisplayList* aList)
{
  nsDisplayList newList;

  while (true) {
    nsDisplayItem* i = aList->RemoveBottom();
    if (!i)
      break;
    nsDisplayList* subList = i->GetSameCoordinateSystemChildren();
    if (subList) {
      PruneDisplayListForExtraPage(aBuilder, aPage, aExtraPage, subList);
      i->UpdateBounds(aBuilder);
    } else {
      nsIFrame* f = i->Frame();
      if (!nsLayoutUtils::IsProperAncestorFrameCrossDoc(aPage, f)) {
        
        
        i->~nsDisplayItem();
        continue;
      }
    }
    newList.AppendToTop(i);
  }
  aList->AppendToTop(&newList);
}

static void
BuildDisplayListForExtraPage(nsDisplayListBuilder* aBuilder,
                             nsPageFrame* aPage, nsIFrame* aExtraPage,
                             const nsRect& aDirtyRect, nsDisplayList* aList)
{
  
  
  
  
  if (!aExtraPage->HasAnyStateBits(NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO)) {
    return;
  }
  nsDisplayList list;
  aExtraPage->BuildDisplayListForStackingContext(aBuilder, aDirtyRect, &list);
  PruneDisplayListForExtraPage(aBuilder, aPage, aExtraPage, &list);
  aList->AppendToTop(&list);
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

static gfx::Matrix4x4 ComputePageTransform(nsIFrame* aFrame, float aAppUnitsPerPixel)
{
  float scale = aFrame->PresContext()->GetPageScale();
  return gfx::Matrix4x4::Scaling(scale, scale, 1);
}

class nsDisplayHeaderFooter : public nsDisplayItem {
public:
  nsDisplayHeaderFooter(nsDisplayListBuilder* aBuilder, nsPageFrame *aFrame)
    : nsDisplayItem(aBuilder, aFrame)
    , mDisableSubpixelAA(false)
  {
    MOZ_COUNT_CTOR(nsDisplayHeaderFooter);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayHeaderFooter() {
    MOZ_COUNT_DTOR(nsDisplayHeaderFooter);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override {
#ifdef DEBUG
    nsPageFrame* pageFrame = do_QueryFrame(mFrame);
    MOZ_ASSERT(pageFrame, "We should have an nsPageFrame");
#endif
    static_cast<nsPageFrame*>(mFrame)->
      PaintHeaderFooter(*aCtx, ToReferenceFrame(), mDisableSubpixelAA);
  }
  NS_DISPLAY_DECL_NAME("HeaderFooter", nsDisplayItem::TYPE_HEADER_FOOTER)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) override {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }

  virtual void DisableComponentAlpha() override {
    mDisableSubpixelAA = true;
  }
protected:
  bool mDisableSubpixelAA;
};


void
nsPageFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  nsDisplayListCollection set;

  if (PresContext()->IsScreen()) {
    DisplayBorderBackgroundOutline(aBuilder, aLists);
  }

  nsIFrame *child = mFrames.FirstChild();
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

  nsDisplayList content;
  {
    DisplayListClipState::AutoSaveRestore clipState(aBuilder);

    
    
    clipState.Clear();
    clipState.ClipContainingBlockDescendants(clipRect, nullptr);

    nsRect dirtyRect = child->GetVisualOverflowRectRelativeToSelf();
    child->BuildDisplayListForStackingContext(aBuilder, dirtyRect, &content);

    
    
    
    
    
    
    
    nsIFrame* page = child;
    while ((page = GetNextPage(page)) != nullptr) {
      BuildDisplayListForExtraPage(aBuilder, this, page,
          dirtyRect + child->GetOffsetTo(page), &content);
    }

    
    
    
    nsDisplayListBuilder::AutoBuildingDisplayList
      building(aBuilder, child, dirtyRect, true);

    
    
    
    nsRect backgroundRect =
      nsRect(aBuilder->ToReferenceFrame(child), child->GetSize());
    PresContext()->GetPresShell()->AddCanvasBackgroundColorItem(
      *aBuilder, content, child, backgroundRect, NS_RGBA(0,0,0,0));
  }

  content.AppendNewToTop(new (aBuilder) nsDisplayTransform(aBuilder, child,
      &content, content.GetVisibleRect(), ::ComputePageTransform));

  set.Content()->AppendToTop(&content);

  if (PresContext()->IsRootPaginatedDocument()) {
    set.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayHeaderFooter(aBuilder, this));
  }

  set.MoveTo(aLists);
}


void
nsPageFrame::SetPageNumInfo(int32_t aPageNumber, int32_t aTotalPages) 
{ 
  mPageNum     = aPageNumber; 
  mTotNumPages = aTotalPages;
}


void
nsPageFrame::PaintHeaderFooter(nsRenderingContext& aRenderingContext,
                               nsPoint aPt, bool aDisableSubpixelAA)
{
  nsPresContext* pc = PresContext();

  if (!mPD->mPrintSettings) {
    if (pc->Type() == nsPresContext::eContext_PrintPreview || pc->IsDynamic())
      mPD->mPrintSettings = pc->GetPrintSettings();
    if (!mPD->mPrintSettings)
      return;
  }

  nsRect rect(aPt, mRect.Size());
  aRenderingContext.ThebesContext()->SetColor(NS_RGB(0,0,0));

  gfxContextAutoDisableSubpixelAntialiasing disable(aRenderingContext.ThebesContext(), aDisableSubpixelAA);

  
  nsRefPtr<nsFontMetrics> fontMet;
  pc->DeviceContext()->GetMetricsFor(mPD->mHeadFootFont, nullptr, false,
                                     gfxFont::eHorizontal,
                                     pc->GetUserFontSet(),
                                     pc->GetTextPerfMetrics(),
                                     *getter_AddRefs(fontMet));

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
  DrawHeaderFooter(aRenderingContext, *fontMet, eHeader,
                   headerLeft, headerCenter, headerRight,
                   rect, ascent, visibleHeight);

  nsXPIDLString footerLeft, footerCenter, footerRight;
  mPD->mPrintSettings->GetFooterStrLeft(getter_Copies(footerLeft));
  mPD->mPrintSettings->GetFooterStrCenter(getter_Copies(footerCenter));
  mPD->mPrintSettings->GetFooterStrRight(getter_Copies(footerRight));
  DrawHeaderFooter(aRenderingContext, *fontMet, eFooter,
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
nsPageBreakFrame::GetIntrinsicISize()
{
  return nsPresContext::CSSPixelsToAppUnits(1);
}

nscoord
nsPageBreakFrame::GetIntrinsicBSize()
{
  return 0;
}

void
nsPageBreakFrame::Reflow(nsPresContext*           aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsPageBreakFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  
  
  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize finalSize(wm, GetIntrinsicISize(),
                        aReflowState.AvailableBSize() == NS_UNCONSTRAINEDSIZE ?
                          0 : aReflowState.AvailableBSize());
  
  finalSize.BSize(wm) -=
    finalSize.BSize(wm) % nsPresContext::CSSPixelsToAppUnits(1);
  aDesiredSize.SetSize(wm, finalSize);

  
  
  mHaveReflowed = true;
  aStatus = NS_FRAME_COMPLETE; 
}

nsIAtom*
nsPageBreakFrame::GetType() const
{
  return nsGkAtoms::pageBreakFrame; 
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsPageBreakFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("PageBreak"), aResult);
}
#endif
