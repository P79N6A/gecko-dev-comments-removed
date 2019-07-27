



#ifndef nsSimplePageSequenceFrame_h___
#define nsSimplePageSequenceFrame_h___

#include "mozilla/Attributes.h"
#include "nsIPageSequenceFrame.h"
#include "nsContainerFrame.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h"

class nsIDateTimeFormat;

namespace mozilla {
namespace dom {

class HTMLCanvasElement;

}
}





class nsSharedPageData {
public:
  
  
  nsSharedPageData() : mShrinkToFitRatio(1.0f) {}

  nsString    mDateTimeStr;
  nsString    mPageNumFormat;
  nsString    mPageNumAndTotalsFormat;
  nsString    mDocTitle;
  nsString    mDocURL;
  nsFont      mHeadFootFont;

  nsSize      mReflowSize;
  nsMargin    mReflowMargin;
  
  
  
  nsMargin    mEdgePaperMargin;

  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsIPrintOptions> mPrintOptions;

  
  
  
  float mShrinkToFitRatio;
};


class nsSimplePageSequenceFrame : public nsContainerFrame,
                                  public nsIPageSequenceFrame {
public:
  friend nsSimplePageSequenceFrame* NS_NewSimplePageSequenceFrame(nsIPresShell* aPresShell,
                                                                  nsStyleContext* aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void Reflow(nsPresContext*      aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aMaxSize,
                      nsReflowStatus&      aStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  
  NS_IMETHOD SetPageNo(int32_t aPageNo) { return NS_OK;}
  NS_IMETHOD SetSelectionHeight(nscoord aYOffset, nscoord aHeight) MOZ_OVERRIDE { mYSelOffset = aYOffset; mSelectionHeight = aHeight; return NS_OK; }
  NS_IMETHOD SetTotalNumPages(int32_t aTotal) MOZ_OVERRIDE { mTotalPages = aTotal; return NS_OK; }
  
  
  NS_IMETHOD GetSTFPercent(float& aSTFPercent) MOZ_OVERRIDE;

  
  NS_IMETHOD StartPrint(nsPresContext*    aPresContext,
                        nsIPrintSettings* aPrintSettings,
                        const nsAString&  aDocTitle,
                        const nsAString&  aDocURL) MOZ_OVERRIDE;
  NS_IMETHOD PrePrintNextPage(nsITimerCallback* aCallback, bool* aDone) MOZ_OVERRIDE;
  NS_IMETHOD PrintNextPage() MOZ_OVERRIDE;
  NS_IMETHOD ResetPrintCanvasList() MOZ_OVERRIDE;
  NS_IMETHOD GetCurrentPageNum(int32_t* aPageNum) MOZ_OVERRIDE;
  NS_IMETHOD GetNumPages(int32_t* aNumPages) MOZ_OVERRIDE;
  NS_IMETHOD IsDoingPrintRange(bool* aDoing) MOZ_OVERRIDE;
  NS_IMETHOD GetPrintRange(int32_t* aFromPage, int32_t* aToPage) MOZ_OVERRIDE;
  NS_IMETHOD DoPageEnd() MOZ_OVERRIDE;

  
  
  virtual bool HonorPrintBackgroundSettings() MOZ_OVERRIDE { return false; }

  virtual bool HasTransformGetter() const MOZ_OVERRIDE { return true; }

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult  GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  explicit nsSimplePageSequenceFrame(nsStyleContext* aContext);
  virtual ~nsSimplePageSequenceFrame();

  void SetPageNumberFormat(const char* aPropName, const char* aDefPropVal, bool aPageNumOnly);

  
  void SetDateTimeStr(const nsAString& aDateTimeStr);
  void SetPageNumberFormat(const nsAString& aFormatStr, bool aForPageNumOnly);

  
  
  void SetDesiredSize(nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nscoord aWidth, nscoord aHeight);

  void DetermineWhetherToPrintPage();
  nsIFrame* GetCurrentPageFrame();

  nsMargin mMargin;

  
  nsCOMPtr<nsIDateTimeFormat> mDateFormatter;

  nsSize       mSize;
  nsSharedPageData* mPageData; 

  
  int32_t      mPageNum;
  int32_t      mTotalPages;
  int32_t      mPrintRangeType;
  int32_t      mFromPageNum;
  int32_t      mToPageNum;
  nsTArray<int32_t> mPageRanges;
  nsTArray<nsRefPtr<mozilla::dom::HTMLCanvasElement> > mCurrentCanvasList;

  
  nscoord      mSelectionHeight;
  nscoord      mYSelOffset;

  
  bool mPrintThisPage;
  bool mDoingPageRange;

  bool mIsPrintingSelection;

  bool mCalledBeginPage;

  bool mCurrentCanvasListSetup;
};

#endif 

